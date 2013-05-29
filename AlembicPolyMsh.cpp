#include "stdafx.h"
#include "AlembicPolyMsh.h"
#include "AlembicXform.h"

#include "CommonProfiler.h"
#include "CommonMeshUtilities.h"


using namespace XSI;
using namespace MATH;

AlembicPolyMesh::AlembicPolyMesh(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent)
: AlembicObject(eNode, in_Job, oParent)
{
   AbcG::OPolyMesh mesh(GetMyParent(), eNode->name, GetJob()->GetAnimatedTs());
   mMeshSchema = mesh.getSchema();

   Primitive prim(GetRef(REF_PRIMITIVE));
   Abc::OCompoundProperty argGeomParamsProp = mMeshSchema.getArbGeomParams();
   customAttributes.defineCustomAttributes(prim.GetGeometry(), argGeomParamsProp, mMeshSchema.getMetaData(), GetJob()->GetAnimatedTs());

   m_bDynamicTopologyMesh = false;
}

AlembicPolyMesh::~AlembicPolyMesh()
{
}

Abc::OCompoundProperty AlembicPolyMesh::GetCompound()
{
   return mMeshSchema;
}

XSI::CStatus AlembicPolyMesh::Save(double time)
{
   mMeshSample.reset();

   // store the transform
   Primitive prim(GetRef(REF_PRIMITIVE));
   bool globalSpace = GetJob()->GetOption(L"globalSpace");

   // query the global space
   CTransformation globalXfo;
   if(globalSpace)
      globalXfo = KinematicState(GetRef(REF_GLOBAL_TRANS)).GetTransform(time);
   CTransformation globalRotation;
   globalRotation.SetRotation(globalXfo.GetRotation());

   // store the metadata
   SaveMetaData(GetRef(REF_NODE),this);

   // check if the mesh is animated
   //if(mNumSamples > 0) {
   //   if(!isRefAnimated(GetRef(REF_PRIMITIVE),false,globalSpace))
   //      return CStatus::OK;
   //}

   // check if we just have a pure pointcache (no surface)
   bool purePointCache = (bool)GetJob()->GetOption(L"exportPurePointCache");

   // define additional vectors, necessary for this task
   std::vector<Abc::V3f> posVec;

   // access the mesh
   PolygonMesh mesh = prim.GetGeometry(time);
   CVector3Array pos = mesh.GetVertices().GetPositionArray();
   LONG vertCount = pos.GetCount();

   // prepare the bounding box
   Abc::Box3d bbox;

   // allocate the points and normals
   posVec.resize(vertCount);
   for(LONG i=0;i<vertCount;i++)
   {
      if(globalSpace)
         pos[i] = MapObjectPositionToWorldSpace(globalXfo,pos[i]);
      posVec[i].x = (float)pos[i].GetX();
      posVec[i].y = (float)pos[i].GetY();
      posVec[i].z = (float)pos[i].GetZ();
      bbox.extendBy(posVec[i]);
   }


   // store the positions && bbox
   mMeshSample.setPositions(Abc::P3fArraySample(posVec));
   mMeshSample.setSelfBounds(bbox);

   customAttributes.exportCustomAttributes(mesh);

   // abort here if we are just storing points
   if(purePointCache)
   {
      if(mNumSamples == 0)
      {
         // store a dummy empty topology
         mMeshSample.setFaceCounts(Abc::Int32ArraySample(NULL, 0));
         mMeshSample.setFaceIndices(Abc::Int32ArraySample(NULL, 0));
      }

      mMeshSchema.set(mMeshSample);
      mNumSamples++;
      return CStatus::OK;
   }

   // check if we support changing topology
   bool dynamicTopology = (bool)GetJob()->GetOption(L"exportDynamicTopology");

   // get the faces
   CPolygonFaceRefArray faces = mesh.GetPolygons();
   LONG faceCount = faces.GetCount();
   LONG sampleCount = mesh.GetSamples().GetCount();

   // create a sample look table
   LONG offset = 0;
   CLongArray sampleLookup(sampleCount);
   for(LONG i=0;i<faces.GetCount();i++)
   {
      PolygonFace face(faces[i]);
      CLongArray samples = face.GetSamples().GetIndexArray();
      for(LONG j=samples.GetCount()-1;j>=0;j--)
         sampleLookup[offset++] = samples[j];
   }

   if( !m_bDynamicTopologyMesh && mNumSamples > 0)
   {
      if(mFaceCountVec.size() != faceCount || mFaceIndicesVec.size() != sampleCount){
         ESS_LOG_WARNING("Dynamic Topology Mesh detected");
         m_bDynamicTopologyMesh = true;
      }
   }

   // if we are the first frame!
   if(mNumSamples == 0 || (dynamicTopology))
   {
      // we also need to store the face counts as well as face indices
      mFaceCountVec.resize(faceCount);
      mFaceIndicesVec.resize(sampleCount);

      offset = 0;
      for(LONG i=0;i<faceCount;i++)
      {
         PolygonFace face(faces[i]);
         CLongArray indices = face.GetVertices().GetIndexArray();
         mFaceCountVec[i] = indices.GetCount();
         for(LONG j=indices.GetCount()-1;j>=0;j--)
            mFaceIndicesVec[offset++] = indices[j];
      }

      Abc::Int32ArraySample faceCountSample(mFaceCountVec);
      Abc::Int32ArraySample faceIndicesSample(mFaceIndicesVec);

      mMeshSample.setFaceCounts(faceCountSample);
      mMeshSample.setFaceIndices(faceIndicesSample);
   }

   //these three variables must be scope when the schema set call occurs
   AbcG::ON3fGeomParam::Sample normalSample;
   std::vector<Abc::N3f> indexedNormals;
	std::vector<Abc::uint32_t> normalIndexVec;

   bool exportNormals = GetJob()->GetOption(L"exportNormals");
   if(exportNormals)
   {
      std::vector<Abc::N3f> normalVec;
      CVector3Array normals = mesh.GetVertices().GetNormalArray();

      CGeometryAccessor accessor = mesh.GetGeometryAccessor(siConstructionModeSecondaryShape);
      CRefArray userNormalProps = accessor.GetUserNormals();
      CFloatArray shadingNormals;
      accessor.GetNodeNormals(shadingNormals);
      if(userNormalProps.GetCount() > 0)
      {
         ClusterProperty userNormalProp(userNormalProps[0]);
         Cluster cluster(userNormalProp.GetParent());
         CLongArray elements = cluster.GetElements().GetArray();
         CDoubleArray userNormals = userNormalProp.GetElements().GetArray();
         for(LONG i=0;i<elements.GetCount();i++)
         {
            LONG sampleIndex = elements[i] * 3;
            if(sampleIndex >= shadingNormals.GetCount())
               continue;
            shadingNormals[sampleIndex++] = (float)userNormals[i*3+0];
            shadingNormals[sampleIndex++] = (float)userNormals[i*3+1];
            shadingNormals[sampleIndex++] = (float)userNormals[i*3+2];
         }
      }
      normalVec.resize(shadingNormals.GetCount() / 3);

      for(LONG i=0;i<sampleCount;i++)
      {
         LONG lookedup = sampleLookup[i];
         CVector3 normal;
         normal.PutX(shadingNormals[lookedup * 3 + 0]);
         normal.PutY(shadingNormals[lookedup * 3 + 1]);
         normal.PutZ(shadingNormals[lookedup * 3 + 2]);
         if(globalSpace)
         {
            normal = MapObjectPositionToWorldSpace(globalRotation,normal);
            normal.NormalizeInPlace();
         }
         normalVec[i].x = (float)normal.GetX();
         normalVec[i].y = (float)normal.GetY();
         normalVec[i].z = (float)normal.GetZ();
      }

      createIndexedArray<Abc::N3f, SortableV3f>(mFaceIndicesVec, normalVec, indexedNormals, normalIndexVec);

      normalSample.setScope(AbcG::kFacevaryingScope);
      
      normalSample.setVals(Abc::N3fArraySample(indexedNormals));
      if(normalIndexVec.size() > 0){
         normalSample.setIndices(Abc::UInt32ArraySample(normalIndexVec));
      }
      mMeshSample.setNormals(normalSample);
   }

   // check if we should export the velocities
   if(dynamicTopology)
   {
      ICEAttribute velocitiesAttr = mesh.GetICEAttributeFromName(L"PointVelocity");
      if(velocitiesAttr.IsDefined() && velocitiesAttr.IsValid())
      {
         CICEAttributeDataArrayVector3f velocitiesData;
         velocitiesAttr.GetDataArray(velocitiesData);
         
         bool bAllZero = true;
         {
            ESS_PROFILE_SCOPE("PointVelocity-zero-check");

            bAllZero = true;
            for(ULONG i=0;i<velocitiesData.GetCount(); i++){
               CVector3 vel;
               vel.PutX(velocitiesData[i].GetX());
               vel.PutY(velocitiesData[i].GetY());
               vel.PutZ(velocitiesData[i].GetZ());

               if(vel.GetX() != 0.0 || vel.GetY() != 0.0 || vel.GetZ() != 0.0){
                  bAllZero = false;
               }
            }
         }

         if(velocitiesAttr.IsConstant()){
            ESS_LOG_WARNING("attribute is constant");
         }


         if(!bAllZero)
         {
            mVelocitiesVec.resize(vertCount);
            for(LONG i=0;i<vertCount;i++)
            {
               CVector3 vel;
               vel.PutX(velocitiesData[i].GetX());
               vel.PutY(velocitiesData[i].GetY());
               vel.PutZ(velocitiesData[i].GetZ());
               if(globalSpace)
                  vel = MapObjectPositionToWorldSpace(globalRotation,vel);
               mVelocitiesVec[i].x = (float)vel.GetX();
               mVelocitiesVec[i].y = (float)vel.GetY();
               mVelocitiesVec[i].z = (float)vel.GetZ();
            }

            Abc::V3fArraySample sample = Abc::V3fArraySample(mVelocitiesVec);
            mMeshSample.setVelocities(sample);
         }

      }
   }

   std::vector<std::vector<Abc::V2f> > uvVecs;
   std::vector<std::vector<Abc::uint32_t> > uvIndicesVecs;
   // if we are the first frame!
   if(mNumSamples == 0 || (dynamicTopology))
   {

      // also check if we need to store UV
      CRefArray clusters = mesh.GetClusters();
      if((bool)GetJob()->GetOption(L"exportUVs"))
      {
         CGeometryAccessor accessor = mesh.GetGeometryAccessor(siConstructionModeSecondaryShape);
         CRefArray uvPropRefs = accessor.GetUVs();

         // if we now finally found a valid uvprop
         if(uvPropRefs.GetCount() > 0)
         {
            uvVecs.resize(uvPropRefs.GetCount());
            uvIndicesVecs.resize(uvPropRefs.GetCount());

            // ok, great, we found UVs, let's set them up
            if(mNumSamples == 0)
            {
               // query the names of all uv properties
               std::vector<std::string> uvSetNames;
               for(LONG i=0;i< uvPropRefs.GetCount();i++)
                  uvSetNames.push_back(ClusterProperty(uvPropRefs[i]).GetName().GetAsciiString());

               Abc::OStringArrayProperty uvSetNamesProperty = Abc::OStringArrayProperty(
                  mMeshSchema, ".uvSetNames", mMeshSchema.getMetaData(), GetJob()->GetAnimatedTs() );
               Abc::StringArraySample uvSetNamesSample(uvSetNames);
               uvSetNamesProperty.set(uvSetNamesSample);
            }

            // loop over all uvsets
            for(LONG uvI=0; uvI<uvPropRefs.GetCount(); uvI++)
            {
               std::vector<Abc::V2f> uvVec;
               uvVec.resize(sampleCount);
               
               CDoubleArray uvValues = ClusterProperty(uvPropRefs[uvI]).GetElements().GetArray();

               for(LONG i=0;i<sampleCount;i++)
               {
                  uvVec[i].x = (float)uvValues[sampleLookup[i] * 3 + 0];
                  uvVec[i].y = (float)uvValues[sampleLookup[i] * 3 + 1];
               }

               createIndexedArray<Abc::V2f, SortableV2f>(mFaceIndicesVec, uvVec, uvVecs[uvI], uvIndicesVecs[uvI]);

               AbcG::OV2fGeomParam::Sample uvSample(Abc::V2fArraySample(uvVecs[uvI]),AbcG::kFacevaryingScope);
               if(uvIndicesVecs[uvI].size() > 0){
                  uvSample.setIndices(Abc::UInt32ArraySample(uvIndicesVecs[uvI]));
               }

               if(uvI == 0)
               {
                  mMeshSample.setUVs(uvSample);
               }
               else
               {
                  // create the uv param if required
                  if(mNumSamples == 0)
                  {
                     CString storedUvSetName = CString(L"uv") + CString(uvI);
                     mUvParams.push_back(AbcG::OV2fGeomParam( mMeshSchema, storedUvSetName.GetAsciiString(), uvIndicesVecs[uvI].size() > 0,
                                        AbcG::kFacevaryingScope, 1, GetJob()->GetAnimatedTs()));
                  }
                  mUvParams[uvI-1].set(uvSample);
               }
            }

            // create the uv options
            if(mUvOptionsVec.size() == 0)
            {
				mUvOptionsProperty = Abc::OFloatArrayProperty(mMeshSchema, ".uvOptions", mMeshSchema.getMetaData(), GetJob()->GetAnimatedTs() );

               for(LONG uvI=0;uvI<uvPropRefs.GetCount();uvI++)
               {
                  ClusterProperty clusterProperty = (ClusterProperty) uvPropRefs[uvI];
                  bool subdsmooth = false;
                  if( clusterProperty.GetType() == L"uvspace") {
                     subdsmooth = (bool)clusterProperty.GetParameter(L"subdsmooth").GetValue();      
                     //ESS_LOG_ERROR( "subdsmooth: " << subdsmooth );
                  }

                  CRefArray children = clusterProperty.GetNestedObjects();
                  bool uWrap = false;
                  bool vWrap = false;
                  for(LONG i=0; i<children.GetCount(); i++)
                  {
                     ProjectItem child(children.GetItem(i));
                     CString type = child.GetType();
					// ESS_LOG_ERROR( "  Cluster Property child type: " << type.GetAsciiString() );
                     if(type == L"uvprojdef")
                     {
                        uWrap = (bool)child.GetParameter(L"wrap_u").GetValue();
                        vWrap = (bool)child.GetParameter(L"wrap_v").GetValue();
                        break;
                     }
                  }

                  // uv wrapping
                  mUvOptionsVec.push_back(uWrap ? 1.0f : 0.0f);
                  mUvOptionsVec.push_back(vWrap ? 1.0f : 0.0f);
				      mUvOptionsVec.push_back(subdsmooth ? 1.0f : 0.0f);
               }
               mUvOptionsProperty.set(Abc::FloatArraySample(mUvOptionsVec));
            }
         }
      }
	  

      // set the subd level
      Property geomApproxProp;
      prim.GetParent3DObject().GetPropertyFromName(L"geomapprox",geomApproxProp);

      if(!mFaceVaryingInterpolateBoundaryProperty){
         mFaceVaryingInterpolateBoundaryProperty =
            Abc::OInt32Property( mMeshSchema, ".faceVaryingInterpolateBoundary", mMeshSchema.getMetaData(), GetJob()->GetAnimatedTs() );
      }

	   mFaceVaryingInterpolateBoundaryProperty.set( (Abc::int32_t) geomApproxProp.GetParameterValue(L"gapproxmordrsl") );

      // sweet, now let's have a look at face sets (really only for first sample)
      if(GetJob()->GetOption(L"exportFaceSets") && mNumSamples == 0)
      {
         for(LONG i=0;i<clusters.GetCount();i++)
         {
            Cluster cluster(clusters[i]);
            if(!cluster.GetType().IsEqualNoCase(L"poly"))
               continue;

            CLongArray elements = cluster.GetElements().GetArray();
            if(elements.GetCount() == 0)
               continue;

            std::string name(cluster.GetName().GetAsciiString());

            mFaceSetsVec.push_back(std::vector<Abc::int32_t>());
            std::vector<Abc::int32_t> & faceSetVec = mFaceSetsVec.back();
            for(LONG j=0;j<elements.GetCount();j++)
               faceSetVec.push_back(elements[j]);

            if(faceSetVec.size() > 0)
            {
              AbcG::OFaceSet faceSet = mMeshSchema.createFaceSet(name);
              AbcG::OFaceSetSchema::Sample faceSetSample(Abc::Int32ArraySample(&faceSetVec.front(),faceSetVec.size()));
               faceSet.getSchema().set(faceSetSample);
            }
         }
      }

      // check if we need to export the bindpose (also only for first frame)
      if(GetJob()->GetOption(L"exportBindPose") && prim.GetParent3DObject().GetEnvelopes().GetCount() > 0 && mNumSamples == 0)
      {
         mBindPoseProperty = Abc::OV3fArrayProperty(mMeshSchema, ".bindpose", mMeshSchema.getMetaData(), GetJob()->GetAnimatedTs());

         // store the positions of the modeling stack into here
         PolygonMesh bindPoseGeo = prim.GetGeometry(time, siConstructionModeModeling);
         CVector3Array bindPosePos = bindPoseGeo.GetPoints().GetPositionArray();
         mBindPoseVec.resize((size_t)bindPosePos.GetCount());
         for(LONG i=0;i<bindPosePos.GetCount();i++)
         {
            mBindPoseVec[i].x = (float)bindPosePos[i].GetX();
            mBindPoseVec[i].y = (float)bindPosePos[i].GetY();
            mBindPoseVec[i].z = (float)bindPosePos[i].GetZ();
         }

         Abc::V3fArraySample sample;
         if(mBindPoseVec.size() > 0)
            sample = Abc::V3fArraySample(&mBindPoseVec.front(),mBindPoseVec.size());
         mBindPoseProperty.set(sample);
      }   
   }


   mMeshSchema.set(mMeshSample);

   mNumSamples++;

   return CStatus::OK;
}

ESS_CALLBACK_START( alembic_polymesh_Define, CRef& )
   return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_polymesh_DefineLayout, CRef& )
   return alembicOp_DefineLayout(in_ctxt); 
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_polymesh_Init, CRef& )
   return alembicOp_Init( in_ctxt );
ESS_CALLBACK_END

void findNumberBounds( std::string str, int& nStart, int& nEnd )
{
   bool bFirstFind = false;
   for(int i=(int)str.size()-1; i>=0; i--){
      if( '0' <= str[i] && str[i] <= '9' ){
         if(!bFirstFind){
            bFirstFind = true; 
            nStart = i;
            nEnd = i;
         }
      }
      else{
         if(bFirstFind){
            nStart = i+1;
            break;
         }
      }

   }

}

//we just support padded format for now...
CString replaceNumber( CString pathstr, int frameNum )
{
   std::string path(pathstr.GetAsciiString());

   int nStart = -1;
   int nEnd = -1;

   findNumberBounds(path, nStart, nEnd);

   if( nStart == -1 || nEnd == -1 ){
      return pathstr;
   }

   nEnd++;
   int nLen = (int)path.size() - nEnd;
   int nNumLen = nEnd - nStart;

   std::stringstream ss;
   ss<<path.substr(0, nStart)<<std::setfill('0')<<std::setw(nNumLen)<<frameNum<<path.substr(nEnd);

   return CString(ss.str().c_str());
}



ESS_CALLBACK_START( alembic_polymesh_Update, CRef& )
   ESS_PROFILE_SCOPE("alembic_polymesh_Update");
   OperatorContext ctxt( in_ctxt );

   CString path = ctxt.GetParameterValue(L"path");

   if(ctxt.GetParameterValue(L"multifile")){

      int frameNum = -1;
      alembicOp_getFrameNum( in_ctxt, ctxt.GetParameterValue(L"time"), frameNum );

      if(frameNum != -1){
         //path should equal the first file in the sequence
         //search and replace frame number on filename
         
         path = replaceNumber( path, frameNum );
      }
   }
   CStatus pathEditStat = alembicOp_PathEdit( in_ctxt, path );


   if((bool)ctxt.GetParameterValue(L"muted") )
      return CStatus::OK;

   CString identifier = ctxt.GetParameterValue(L"identifier");

  AbcG::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;


  AbcG::IPolyMesh objMesh;
  AbcG::ISubD objSubD;
   if(AbcG::IPolyMesh::matches(iObj.getMetaData()))
      objMesh =AbcG::IPolyMesh(iObj,Abc::kWrapExisting);
   else
      objSubD =AbcG::ISubD(iObj,Abc::kWrapExisting);
   if(!objMesh.valid() && !objSubD.valid())
      return CStatus::OK;

   AbcA::TimeSamplingPtr timeSampling;
   int nSamples = 0;
   if(objMesh.valid())
   {
      timeSampling = objMesh.getSchema().getTimeSampling();
	  nSamples = (int) objMesh.getSchema().getNumSamples();
   }
   else
   {
      timeSampling = objSubD.getSchema().getTimeSampling();
	  nSamples = (int) objSubD.getSchema().getNumSamples();
   }


   if(ctxt.GetParameterValue(L"multifile")){
      if( alembicOp_TimeSamplingInit(in_ctxt, timeSampling) == CStatus::Fail ){
         return CStatus::OK;
      }
   }


   SampleInfo sampleInfo = getSampleInfo(
     ctxt.GetParameterValue(L"time"),
     timeSampling,
	 nSamples
   );

   Abc::P3fArraySamplePtr meshPos;
   if(objMesh.valid())
   {
     AbcG::IPolyMeshSchema::Sample sample;
      objMesh.getSchema().get(sample,sampleInfo.floorIndex);
      meshPos = sample.getPositions();
   }
   else
   {
     AbcG::ISubDSchema::Sample sample;
      objSubD.getSchema().get(sample,sampleInfo.floorIndex);
      meshPos = sample.getPositions();
   }

   PolygonMesh inMesh = Primitive((CRef)ctxt.GetInputValue(0)).GetGeometry();
   CVector3Array pos = inMesh.GetPoints().GetPositionArray();

   Operator op(ctxt.GetSource());
   updateOperatorInfo( op, sampleInfo, timeSampling, (int) pos.GetCount(), (int) meshPos->size());

   if(pos.GetCount() != meshPos->size())
      return CStatus::OK;

   for(size_t i=0;i<meshPos->size();i++)
      pos[(LONG)i].Set(meshPos->get()[i].x,meshPos->get()[i].y,meshPos->get()[i].z);

   // blend
   if(sampleInfo.alpha != 0.0)
   {
      if(objMesh.valid())
      {
        AbcG::IPolyMeshSchema::Sample sample;
         objMesh.getSchema().get(sample,sampleInfo.ceilIndex);
         meshPos = sample.getPositions();
      }
      else
      {
        AbcG::ISubDSchema::Sample sample;
         objSubD.getSchema().get(sample,sampleInfo.ceilIndex);
         meshPos = sample.getPositions();
      }
      for(size_t i=0;i<meshPos->size();i++)
         pos[(LONG)i].LinearlyInterpolate(pos[(LONG)i],CVector3(meshPos->get()[i].x,meshPos->get()[i].y,meshPos->get()[i].z),sampleInfo.alpha);
   }

   Primitive(ctxt.GetOutputTarget()).GetGeometry().GetPoints().PutPositionArray(pos);

   return CStatus::OK;
ESS_CALLBACK_END


ESS_CALLBACK_START( alembic_polymesh_Term, CRef& )
   return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_normals_Define, CRef& )
   return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_normals_DefineLayout, CRef& )
   return alembicOp_DefineLayout(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_normals_Init, CRef& )
   return alembicOp_Init( in_ctxt );
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_normals_Update, CRef& )
   ESS_PROFILE_SCOPE("alembic_normals_Update");
   OperatorContext ctxt( in_ctxt );

   CString path = ctxt.GetParameterValue(L"path");
   CStatus pathEditStat = alembicOp_PathEdit( in_ctxt, path );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString identifier = ctxt.GetParameterValue(L"identifier");

  AbcG::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
  AbcG::IPolyMesh obj(iObj,Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

   SampleInfo sampleInfo = getSampleInfo(
      ctxt.GetParameterValue(L"time"),
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );

   CDoubleArray normalValues = ClusterProperty((CRef)ctxt.GetInputValue(0)).GetElements().GetArray();
   PolygonMesh mesh = Primitive((CRef)ctxt.GetInputValue(1)).GetGeometry();
   CGeometryAccessor accessor = mesh.GetGeometryAccessor(siConstructionModeModeling);
   CLongArray counts;
   accessor.GetPolygonVerticesCount(counts);

  AbcG::IN3fGeomParam meshNormalsParam = obj.getSchema().getNormalsParam();
   if(meshNormalsParam.valid())
   {
      Abc::N3fArraySamplePtr meshNormals = meshNormalsParam.getExpandedValue(sampleInfo.floorIndex).getVals();

	  Operator op(ctxt.GetSource());
	  updateOperatorInfo( op, sampleInfo, obj.getSchema().getTimeSampling(),
						  (int) normalValues.GetCount(), (int) meshNormals->size()*3);

      if(meshNormals->size() * 3 == normalValues.GetCount())
      {
         // let's apply it!
         LONG offsetIn = 0;
         LONG offsetOut = 0;
         for(LONG i=0;i<counts.GetCount();i++)
         {
            for(LONG j=counts[i]-1;j>=0;j--)
            {
               normalValues[offsetOut++] = meshNormals->get()[offsetIn+j].x;
               normalValues[offsetOut++] = meshNormals->get()[offsetIn+j].y;
               normalValues[offsetOut++] = meshNormals->get()[offsetIn+j].z;
            }
            offsetIn += counts[i];
         }

         // blend
         if(sampleInfo.alpha != 0.0 /*&& !isAlembicMeshTopology(&iObj)*/)
         {
            meshNormals = meshNormalsParam.getExpandedValue(sampleInfo.ceilIndex).getVals();
            if(meshNormals->size() == normalValues.GetCount() / 3)
            {
               offsetIn = 0;
               offsetOut = 0;

               for(LONG i=0;i<counts.GetCount();i++)
               {
                  for(LONG j=counts[i]-1;j>=0;j--)
                  {
                     CVector3 normal(normalValues[offsetOut],normalValues[offsetOut+1],normalValues[offsetOut+2]);
                     normal.LinearlyInterpolate(normal,CVector3(
                        meshNormals->get()[offsetIn+j].x,
                        meshNormals->get()[offsetIn+j].y,
                        meshNormals->get()[offsetIn+j].z),sampleInfo.alpha);
                     normal.NormalizeInPlace();
                     normalValues[offsetOut++] = normal.GetX();
                     normalValues[offsetOut++] = normal.GetY();
                     normalValues[offsetOut++] = normal.GetZ();
                  }
                  offsetIn += counts[i];
               }
            }
         }
      }
   }

   ClusterProperty(ctxt.GetOutputTarget()).GetElements().PutArray(normalValues);

   return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_normals_Term, CRef& )
   return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_uvs_Define, CRef& )
   return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_uvs_DefineLayout, CRef& )
   return alembicOp_DefineLayout(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_uvs_Init, CRef& )
   return alembicOp_Init( in_ctxt );
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_uvs_Update, CRef& )
   ESS_PROFILE_SCOPE("alembic_uvs_Update");
   OperatorContext ctxt( in_ctxt );

   CString path = ctxt.GetParameterValue(L"path");
   CStatus pathEditStat = alembicOp_PathEdit( in_ctxt, path );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString identifierAndIndex = ctxt.GetParameterValue(L"identifier");

   ULONG colonOffset = identifierAndIndex.ReverseFindString(L":");

   CString identifier = identifierAndIndex.GetSubString(0, colonOffset);
 
   LONG uvI = 0;
   if( colonOffset == identifierAndIndex.Length() ){
      uvI = (LONG)CValue(identifierAndIndex.GetSubString(colonOffset+1));
   }

   //ESS_LOG_WARNING("identifier: "<<identifier.GetAsciiString());
   //ESS_LOG_WARNING("uvI: "<<uvI);

  AbcG::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
  AbcG::IPolyMesh objMesh;
  AbcG::ISubD objSubD;
   if(AbcG::IPolyMesh::matches(iObj.getMetaData()))
      objMesh =AbcG::IPolyMesh(iObj,Abc::kWrapExisting);
   else
      objSubD =AbcG::ISubD(iObj,Abc::kWrapExisting);
   if(!objMesh.valid() && !objSubD.valid())
      return CStatus::OK;

   CDoubleArray uvValues = ClusterProperty((CRef)ctxt.GetInputValue(0)).GetElements().GetArray();
   PolygonMesh mesh = Primitive((CRef)ctxt.GetInputValue(1)).GetGeometry();
   CPolygonFaceRefArray faces = mesh.GetPolygons();
   CGeometryAccessor accessor = mesh.GetGeometryAccessor(siConstructionModeModeling);
   CLongArray counts;
   accessor.GetPolygonVerticesCount(counts);

  AbcG::IV2fGeomParam meshUvParam;
   if(objMesh.valid())
   {
      if(uvI == 0)
         meshUvParam = objMesh.getSchema().getUVsParam();
      else
      {
         CString storedUVName = CString(L"uv")+CString(uvI);
         if(objMesh.getSchema().getPropertyHeader( storedUVName.GetAsciiString() ) == NULL)
            return CStatus::OK;
         meshUvParam =AbcG::IV2fGeomParam( objMesh.getSchema(), storedUVName.GetAsciiString());
      }
   }
   else
   {
      if(uvI == 0)
         meshUvParam = objSubD.getSchema().getUVsParam();
      else
      {
         CString storedUVName = CString(L"uv")+CString(uvI);
         if(objSubD.getSchema().getPropertyHeader( storedUVName.GetAsciiString() ) == NULL)
            return CStatus::OK;
         meshUvParam =AbcG::IV2fGeomParam( objSubD.getSchema(), storedUVName.GetAsciiString());
      }
   }

   if(meshUvParam.valid())
   {
      SampleInfo sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         meshUvParam.getTimeSampling(),
         meshUvParam.getNumSamples()
      );

      Abc::V2fArraySamplePtr meshUVs = meshUvParam.getExpandedValue(sampleInfo.floorIndex).getVals();

	  Operator op(ctxt.GetSource());
	  updateOperatorInfo( op, sampleInfo, meshUvParam.getTimeSampling(),
						  (int) meshUVs->size() * 3, (int) uvValues.GetCount());

      if(meshUVs->size() * 3 == uvValues.GetCount())
      {
         // create a sample look table
         LONG offset = 0;
         CLongArray sampleLookup(accessor.GetNodeCount());
         for(LONG i=0;i<faces.GetCount();i++)
         {
            PolygonFace face(faces[i]);
            CLongArray samples = face.GetSamples().GetIndexArray();
            for(LONG j=samples.GetCount()-1;j>=0;j--){
               //ESS_LOG_WARNING("sampleLookup["<<samples[j]<<"]="<<offset);
               sampleLookup[samples[j]] = offset++;
               //sampleLookup[offset++] = samples[j];
            }
         }

         // let's apply it!
         offset = 0;
         for(LONG i=0;i<sampleLookup.GetCount();i++)
         {
            uvValues[offset++] = meshUVs->get()[sampleLookup[i]].x;
            uvValues[offset++] = meshUVs->get()[sampleLookup[i]].y;
            uvValues[offset++] = 0.0;
         }

         if(sampleInfo.alpha != 0.0)
         {
            meshUVs = meshUvParam.getExpandedValue(sampleInfo.ceilIndex).getVals();
            double ialpha = 1.0 - sampleInfo.alpha;

            offset = 0;
            for(LONG i=0;i<sampleLookup.GetCount();i++)
            {
               uvValues[offset] = uvValues[offset] * ialpha + meshUVs->get()[sampleLookup[i]].x * sampleInfo.alpha;
               offset++;
               uvValues[offset] = uvValues[offset] * ialpha + meshUVs->get()[sampleLookup[i]].y * sampleInfo.alpha;
               offset++;
               uvValues[offset] = 0.0;
               offset++;
            }
         }
      }
   }

   ClusterProperty(ctxt.GetOutputTarget()).GetElements().PutArray(uvValues);

   return CStatus::OK;
ESS_CALLBACK_END


ESS_CALLBACK_START( alembic_uvs_Term, CRef& )
   return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_polymesh_topo_Define, CRef& )
   return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_polymesh_topo_DefineLayout, CRef& )
   return alembicOp_DefineLayout(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_polymesh_topo_Init, CRef& )
   return alembicOp_Init( in_ctxt );
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_polymesh_topo_Update, CRef& )
   ESS_PROFILE_SCOPE("alembic_polymesh_topo_Update");
   OperatorContext ctxt( in_ctxt );

   CString path = ctxt.GetParameterValue(L"path");
   CStatus pathEditStat = alembicOp_PathEdit( in_ctxt, path );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString identifier = ctxt.GetParameterValue(L"identifier");

  AbcG::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
  AbcG::IPolyMesh objMesh;
  AbcG::ISubD objSubD;
   {
	ESS_PROFILE_SCOPE("alembic_polymesh_topo_Update type matching");
	if(AbcG::IPolyMesh::matches(iObj.getMetaData()))
      objMesh =AbcG::IPolyMesh(iObj,Abc::kWrapExisting);
   else
      objSubD =AbcG::ISubD(iObj,Abc::kWrapExisting);
   }

   {
		ESS_PROFILE_SCOPE("alembic_polymesh_topo_Update checking validity");
	   if(!objMesh.valid() && !objSubD.valid())
		  return CStatus::OK;
   }

   if( ! isAlembicMeshTopology( & iObj ) ) {
	   return CStatus::OK;
   }

   AbcA::TimeSamplingPtr timeSampling;
   int nSamples = 0;
   if(objMesh.valid())
   {
       timeSampling = objMesh.getSchema().getTimeSampling();
	  nSamples = (int) objMesh.getSchema().getNumSamples();
  }
   else
   {
        timeSampling = objSubD.getSchema().getTimeSampling();
	  nSamples = (int) objSubD.getSchema().getNumSamples();
   }

   SampleInfo sampleInfo;
   
   {
		ESS_PROFILE_SCOPE("alembic_polymesh_topo_Update getSampleInfo");
  
	   sampleInfo = getSampleInfo(
		 ctxt.GetParameterValue(L"time"),
		 timeSampling,
		 nSamples
	   );
   }

   Abc::P3fArraySamplePtr meshPos;
   Abc::V3fArraySamplePtr meshVel;
   Abc::Int32ArraySamplePtr meshFaceCount;
   Abc::Int32ArraySamplePtr meshFaceIndices;

   bool hasDynamicTopo = isAlembicMeshTopoDynamic( & objMesh );
   if(objMesh.valid())
   {
      ESS_PROFILE_SCOPE("alembic_polymesh_topo_Update load abc data arrays");

     AbcG::IPolyMeshSchema::Sample sample;
      objMesh.getSchema().get(sample,sampleInfo.floorIndex);
      meshPos = sample.getPositions();
      meshVel = sample.getVelocities();
      meshFaceCount = sample.getFaceCounts();
      meshFaceIndices = sample.getFaceIndices();
   }
   else
   {
      ESS_PROFILE_SCOPE("alembic_polymesh_topo_Update load abc data arrays");
     AbcG::ISubDSchema::Sample sample;
      objSubD.getSchema().get(sample,sampleInfo.floorIndex);
      meshPos = sample.getPositions();
      meshVel = sample.getVelocities();
      meshFaceCount = sample.getFaceCounts();
      meshFaceIndices = sample.getFaceIndices();
   }

   Operator op(ctxt.GetSource());
   updateOperatorInfo( op, sampleInfo, timeSampling, (int) meshPos->size(), (int) meshPos->size());

   CVector3Array pos((LONG)meshPos->size());
   CLongArray polies((LONG)(meshFaceCount->size() + meshFaceIndices->size()));

   {
       ESS_PROFILE_SCOPE("alembic_polymesh_topo_Update set positions");
	   for(size_t j=0;j<meshPos->size();j++) {
		  pos[(LONG)j].Set(meshPos->get()[j].x,meshPos->get()[j].y,meshPos->get()[j].z);
	   }
   }

   // check if this is an empty topo object
   if(meshFaceCount->size() > 0)
   {
        ESS_PROFILE_SCOPE("alembic_polymesh_topo_Update setup topology");
		if(meshFaceCount->get()[0] == 0)
      {
         if(!meshVel)
            return CStatus::OK;
         if(meshVel->size() != meshPos->size())
            return CStatus::OK;

         // dummy topo
         polies.Resize(4);
         polies[0] = 3;
         polies[1] = 0;
         polies[2] = 0;
         polies[3] = 0;
      }
      else
      {

         LONG offset1 = 0;
         Abc::int32_t offset2 = 0;

         //ESS_LOG_INFO("face count: " << (unsigned int)meshFaceCount->size());

         for(size_t j=0;j<meshFaceCount->size();j++)
         {
            Abc::int32_t singleFaceCount = meshFaceCount->get()[j];
            polies[offset1++] = singleFaceCount;
            offset2 += singleFaceCount;

            //ESS_LOG_INFO("singleFaceCount: " << (unsigned int)singleFaceCount);
            //ESS_LOG_INFO("offset2: " << (unsigned int)offset2);
            //ESS_LOG_INFO("meshFaceIndices->size(): " << (unsigned int)meshFaceIndices->size());

            unsigned int meshFIndxSz = (unsigned int)meshFaceIndices->size();

            for(size_t k=0;k<singleFaceCount;k++)
            {
               //ESS_LOG_INFO("index: " << (unsigned int)((size_t)offset2 - 1 - k));
               polies[offset1++] = meshFaceIndices->get()[(size_t)offset2 - 1 - k];
            }
         }
      }
   }

   // do the positional interpolation if necessary
   if(sampleInfo.alpha != 0.0)
   {
	  ESS_PROFILE_SCOPE("alembic_polymesh_topo_Update positional interpolation");
      double alpha = sampleInfo.alpha;
      double ialpha = 1.0 - alpha;

      // first check if the next frame has the same point count
      if(objMesh.valid())
      {
        AbcG::IPolyMeshSchema::Sample sample;
         objMesh.getSchema().get(sample,sampleInfo.ceilIndex);
         meshPos = sample.getPositions();
      }
      else
      {
        AbcG::ISubDSchema::Sample sample;
         objSubD.getSchema().get(sample,sampleInfo.floorIndex);
         meshPos = sample.getPositions();
      }

      if( !hasDynamicTopo)
      {
		  assert( meshPos->size() == (size_t)pos.GetCount() );

         for(LONG i=0;i<(LONG)meshPos->size();i++)
         {
            pos[i].PutX(ialpha * pos[i].GetX() + alpha * meshPos->get()[i].x);
            pos[i].PutY(ialpha * pos[i].GetY() + alpha * meshPos->get()[i].y);
            pos[i].PutZ(ialpha * pos[i].GetZ() + alpha * meshPos->get()[i].z);
         }
      }
      else if(meshVel)
      {
         float timeAlpha = getTimeOffsetFromObject( iObj, sampleInfo );
         if(meshVel->size() == (size_t)pos.GetCount())
         {
            for(LONG i=0;i<(LONG)meshVel->size();i++)
            {
               pos[i].PutX(pos[i].GetX() + timeAlpha * meshVel->get()[i].x);
               pos[i].PutY(pos[i].GetY() + timeAlpha * meshVel->get()[i].y);
               pos[i].PutZ(pos[i].GetZ() + timeAlpha * meshVel->get()[i].z);
            }
         }
      }
   }

   {
	PolygonMesh outMesh;
	{
	ESS_PROFILE_SCOPE("alembic_polymesh_topo_Update GetGeometry");
	outMesh = Primitive(ctxt.GetOutputTarget()).GetGeometry();
	}
	{
	ESS_PROFILE_SCOPE("alembic_polymesh_topo_Update PolygonMesh set");
	outMesh.Set(pos,polies);
	}
   }

   //ESS_LOG_INFO("EXIT alembic_polymesh_topo_Update");
   return CStatus::OK;
ESS_CALLBACK_END


ESS_CALLBACK_START( alembic_polymesh_topo_Term, CRef& )
   return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END


ESS_CALLBACK_START( alembic_bbox_Define, CRef& )
   alembicOp_Define(in_ctxt);

   Context ctxt( in_ctxt );
   CustomOperator oCustomOperator;

   Parameter oParam;
   CRef oPDef;

   Factory oFactory = Application().GetFactory();
   oCustomOperator = ctxt.GetSource();

   oPDef = oFactory.CreateParamDef(L"extend",CValue::siFloat,siAnimatable| siPersistable,L"extend",L"extend",0.0f,-10000.0f,10000.0f,0.0f,10.0f);
   oCustomOperator.AddParameter(oPDef,oParam);
   return CStatus::OK;
ESS_CALLBACK_END


ESS_CALLBACK_START( alembic_bbox_DefineLayout, CRef& )
   alembicOp_DefineLayout(in_ctxt);

   Context ctxt( in_ctxt );
   PPGLayout oLayout;
   PPGItem oItem;
   oLayout = ctxt.GetSource();
   oLayout.AddItem(L"extend",L"Extend Box");
   return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_bbox_Init, CRef& )
   return alembicOp_Init( in_ctxt );
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_bbox_Update, CRef& )
   OperatorContext ctxt( in_ctxt );

   CString path = ctxt.GetParameterValue(L"path");
   CStatus pathEditStat = alembicOp_PathEdit( in_ctxt, path );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString identifier = ctxt.GetParameterValue(L"identifier");
   float extend = ctxt.GetParameterValue(L"extend");

  AbcG::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;

   Abc::Box3d box;
   SampleInfo sampleInfo;
   AbcA::TimeSamplingPtr timeSampling;
   int nSamples = 0;
   
   // check what kind of object we have
   const Abc::MetaData &md = iObj.getMetaData();
   if(AbcG::IPolyMesh::matches(md)) {
     AbcG::IPolyMesh obj(iObj,Abc::kWrapExisting);
      if(!obj.valid())
         return CStatus::OK;

      timeSampling = obj.getSchema().getTimeSampling();
	  nSamples = (int) obj.getSchema().getNumSamples();
      sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         timeSampling,
         nSamples
      );

     AbcG::IPolyMeshSchema::Sample sample;
      obj.getSchema().get(sample,sampleInfo.floorIndex);
      box = sample.getSelfBounds();

      if(sampleInfo.alpha > 0.0)
      {
         obj.getSchema().get(sample,sampleInfo.ceilIndex);
         Abc::Box3d box2 = sample.getSelfBounds();

         box.min = (1.0 - sampleInfo.alpha) * box.min + sampleInfo.alpha * box2.min;
         box.max = (1.0 - sampleInfo.alpha) * box.max + sampleInfo.alpha * box2.max;
      }
   } else if(AbcG::ICurves::matches(md)) {
     AbcG::ICurves obj(iObj,Abc::kWrapExisting);
      if(!obj.valid())
         return CStatus::OK;

      timeSampling = obj.getSchema().getTimeSampling();
	  nSamples = (int) obj.getSchema().getNumSamples();
      sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         timeSampling,
         nSamples
      );

     AbcG::ICurvesSchema::Sample sample;
      obj.getSchema().get(sample,sampleInfo.floorIndex);
      box = sample.getSelfBounds();

      if(sampleInfo.alpha > 0.0)
      {
         obj.getSchema().get(sample,sampleInfo.ceilIndex);
         Abc::Box3d box2 = sample.getSelfBounds();

         box.min = (1.0 - sampleInfo.alpha) * box.min + sampleInfo.alpha * box2.min;
         box.max = (1.0 - sampleInfo.alpha) * box.max + sampleInfo.alpha * box2.max;
      }
   } else if(AbcG::IPoints::matches(md)) {
     AbcG::IPoints obj(iObj,Abc::kWrapExisting);
      if(!obj.valid())
         return CStatus::OK;

      timeSampling = obj.getSchema().getTimeSampling();
	  nSamples = (int) obj.getSchema().getNumSamples();
      sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         timeSampling,
         nSamples
      );

     AbcG::IPointsSchema::Sample sample;
      obj.getSchema().get(sample,sampleInfo.floorIndex);
      box = sample.getSelfBounds();

      if(sampleInfo.alpha > 0.0)
      {
         obj.getSchema().get(sample,sampleInfo.ceilIndex);
         Abc::Box3d box2 = sample.getSelfBounds();

         box.min = (1.0 - sampleInfo.alpha) * box.min + sampleInfo.alpha * box2.min;
         box.max = (1.0 - sampleInfo.alpha) * box.max + sampleInfo.alpha * box2.max;
      }
   } else if(AbcG::ISubD::matches(md)) {
     AbcG::ISubD obj(iObj,Abc::kWrapExisting);
      if(!obj.valid())
         return CStatus::OK;

      timeSampling = obj.getSchema().getTimeSampling();
	  nSamples = (int) obj.getSchema().getNumSamples();
      sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         timeSampling,
         nSamples
      );

     AbcG::ISubDSchema::Sample sample;
      obj.getSchema().get(sample,sampleInfo.floorIndex);
      box = sample.getSelfBounds();

      if(sampleInfo.alpha > 0.0)
      {
         obj.getSchema().get(sample,sampleInfo.ceilIndex);
         Abc::Box3d box2 = sample.getSelfBounds();

         box.min = (1.0 - sampleInfo.alpha) * box.min + sampleInfo.alpha * box2.min;
         box.max = (1.0 - sampleInfo.alpha) * box.max + sampleInfo.alpha * box2.max;
      }
   }

   Primitive inPrim((CRef)ctxt.GetInputValue(0));
   CVector3Array pos = inPrim.GetGeometry().GetPoints().GetPositionArray();

   Operator op(ctxt.GetSource());
   updateOperatorInfo( op, sampleInfo, timeSampling, pos.GetCount(), pos.GetCount());

   box.min.x -= extend;
   box.min.y -= extend;
   box.min.z -= extend;
   box.max.x += extend;
   box.max.y += extend;
   box.max.z += extend;

   // apply the bbox
   for(LONG i=0;i<pos.GetCount();i++)
   {
      pos[i].PutX( pos[i].GetX() < 0 ? box.min.x : box.max.x );
      pos[i].PutY( pos[i].GetY() < 0 ? box.min.y : box.max.y );
      pos[i].PutZ( pos[i].GetZ() < 0 ? box.min.z : box.max.z );
   }

   Primitive outPrim(ctxt.GetOutputTarget());
   outPrim.GetGeometry().GetPoints().PutPositionArray(pos);

   return CStatus::OK;
ESS_CALLBACK_END


ESS_CALLBACK_START( alembic_bbox_Term, CRef& )
   return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END


/// ICE NODE!
enum IDs
{
	ID_IN_path = 10,
	ID_IN_identifier = 11,
	ID_IN_renderpath = 12,
	ID_IN_renderidentifier = 13,
	ID_IN_time = 14,
	ID_IN_usevel = 15,
	ID_G_100 = 1005,
	ID_OUT_position = 12771,
	ID_OUT_velocity = 12772,
	ID_OUT_faceCounts = 12773,
	ID_OUT_faceIndices = 12774,

	ID_TYPE_CNS = 1400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
};

#define ID_UNDEF ((ULONG)-1)

using namespace XSI;

ESS_CALLBACK_START( alembic_polyMesh2_Init, CRef& )
   return alembicOp_Init( in_ctxt );
ESS_CALLBACK_END

XSIPLUGINCALLBACK CStatus alembic_polyMesh2_Evaluate(ICENodeContext& in_ctxt)
{
	//Application().LogMessage( "alembic_polyMesh2_Evaluate" );
	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	CDataArrayString pathData( in_ctxt, ID_IN_path );
	CString path = pathData[0];
	CDataArrayString identifierData( in_ctxt, ID_IN_identifier );
	CString identifier = identifierData[0];

    CStatus pathEditStat = alembicOp_PathEdit( in_ctxt, path );

	AbcG::IObject iObj = getObjectFromArchive(path,identifier);
	if(!iObj.valid())
		return CStatus::OK;

	CDataArrayFloat timeData( in_ctxt, ID_IN_time);
	const double time = timeData[0];

	CDataArrayBool usevelData( in_ctxt, ID_IN_usevel);
	const double usevel = usevelData[0];

	AbcG::IPolyMesh objMesh;
	AbcG::ISubD objSubD;
	const bool useMesh = AbcG::IPolyMesh::matches(iObj.getMetaData());
	if(useMesh)
		objMesh = AbcG::IPolyMesh(iObj,Abc::kWrapExisting);
	else
		objSubD = AbcG::ISubD(iObj,Abc::kWrapExisting);
	if(!objMesh.valid() && !objSubD.valid())
		return CStatus::OK;

	AbcA::TimeSamplingPtr timeSampling;
	int nSamples = 0;
	if(useMesh)
	{
		timeSampling = objMesh.getSchema().getTimeSampling();
		nSamples = (int) objMesh.getSchema().getNumSamples();
	}
	else
	{
		timeSampling = objSubD.getSchema().getTimeSampling();
		nSamples = (int) objSubD.getSchema().getNumSamples();
	}

	SampleInfo sampleInfo = getSampleInfo( time, timeSampling, nSamples );

	switch( out_portID )
	{
	case ID_OUT_position:
		{
			CDataArray2DVector3f outData( in_ctxt );
			CDataArray2DVector3f::Accessor acc;

			Abc::P3fArraySamplePtr ptr;
			if(useMesh)
			{
				AbcG::IPolyMeshSchema::Sample sample;
				objMesh.getSchema().get(sample,sampleInfo.floorIndex);
				ptr = sample.getPositions();
			}
			else
			{
				AbcG::ISubDSchema::Sample sample;
				objSubD.getSchema().get(sample,sampleInfo.floorIndex);
				ptr = sample.getPositions();
			}

			if(ptr == NULL || ptr->size() == 0 || (ptr->size() == 1 && ptr->get()[0].x == FLT_MAX) )
				acc = outData.Resize(0,0);
			else
			{
				acc = outData.Resize(0,(ULONG)ptr->size());
				bool done = false;
				if(sampleInfo.alpha != 0.0 && usevel)
				{
					const float alpha = (float)sampleInfo.alpha;//shouldn't we be using a time alpha here?

					Abc::V3fArraySamplePtr velPtr;
					if (useMesh)
					{
						AbcG::IPolyMeshSchema::Sample sample;
						objMesh.getSchema().get(sample, sampleInfo.floorIndex);
						velPtr = sample.getVelocities();
					}
					else
					{
						AbcG::ISubDSchema::Sample sample;
						objSubD.getSchema().get(sample, sampleInfo.floorIndex);
						velPtr = sample.getVelocities();
					}

					if(velPtr == NULL || velPtr->size() == 0)
						done = false;
					else
					{
						for(ULONG i=0; i<acc.GetCount(); ++i)
						{
							const Abc::V3f &pt = ptr->get()[i];
							const Abc::V3f &vl = velPtr->get()[i >= velPtr->size() ? 0 : i];
							acc[i].Set
							(
								pt.x + alpha * vl.x,
								pt.y + alpha * vl.y,
								pt.z + alpha * vl.z
							);
						}
						done = true;
					}
				}

				if(!done)
				{
					for(ULONG i=0;i<acc.GetCount();i++)
					{
						const Abc::V3f &pt = ptr->get()[i];
						acc[i].Set(pt.x, pt.y, pt.z);
					}
				}
			}
		}
		break;

	case ID_OUT_velocity:
		{
			CDataArray2DVector3f outData( in_ctxt );
			CDataArray2DVector3f::Accessor acc;

			Abc::V3fArraySamplePtr ptr;
			if(useMesh)
			{
				AbcG::IPolyMeshSchema::Sample sample;
				objMesh.getSchema().get(sample,sampleInfo.floorIndex);
				ptr = sample.getVelocities();
			}
			else
			{
				AbcG::ISubDSchema::Sample sample;
				objSubD.getSchema().get(sample,sampleInfo.floorIndex);
				ptr = sample.getVelocities();
			}

			if(ptr == NULL || ptr->size() == 0)
				acc = outData.Resize( 0, 0 );
			else
			{
				acc = outData.Resize(0, (ULONG)ptr->size());
				for(ULONG i=0; i<acc.GetCount(); ++i)
				{
					const Abc::V3f &vl = ptr->get()[i];
					acc[i].Set(vl.x, vl.y, vl.z);
				}
			}
		}
		break;

	case ID_OUT_faceCounts:
	case ID_OUT_faceIndices:
		{
			CDataArray2DLong outData( in_ctxt );
			CDataArray2DLong::Accessor acc;

			Abc::Int32ArraySamplePtr ptr;
			if (useMesh)
			{
				AbcG::IPolyMeshSchema::Sample sample;
				objMesh.getSchema().get(sample, sampleInfo.floorIndex);
				ptr = (out_portID == ID_OUT_faceCounts)?
								sample.getFaceCounts() :
								sample.getFaceIndices();
			}
			else
			{
				AbcG::ISubDSchema::Sample sample;
				objSubD.getSchema().get(sample, sampleInfo.floorIndex);
				ptr = (out_portID == ID_OUT_faceCounts)?
								sample.getFaceCounts() :
								sample.getFaceIndices();
			}

			if (ptr == NULL || ptr->size() == 0)
				acc = outData.Resize(0, 0);
			else
			{
				acc = outData.Resize(0, (ULONG)ptr->size() );
				for(ULONG i=0; i<acc.GetCount(); ++i)
					acc[i] = ptr->get()[i];
			}
		}
		break;
	}

	return CStatus::OK;
}


XSIPLUGINCALLBACK CStatus alembic_polyMesh2_Term(CRef& in_ctxt)
{
	return alembicOp_Term(in_ctxt);
}

XSI::CStatus Register_alembic_polyMesh( XSI::PluginRegistrar& in_reg )
{
	//Application().LogMessage( "Register_alembic_polyMesh" );
	ICENodeDef nodeDef = Application().GetFactory().CreateICENodeDef(L"alembic_polyMesh2", L"alembic_polyMesh2");

	CStatus st = nodeDef.PutColor(255, 188, 102);
	st.AssertSucceeded( ) ;

	st = nodeDef.PutThreadingModel(XSI::siICENodeSingleThreading);
	st.AssertSucceeded( ) ;

	// Add input ports and groups.
	st = nodeDef.AddPortGroup(ID_G_100);
	st.AssertSucceeded( ) ;

	st = nodeDef.AddInputPort(ID_IN_path,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"path",L"path",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
	st = nodeDef.AddInputPort(ID_IN_identifier,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"identifier",L"identifier",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
	st = nodeDef.AddInputPort(ID_IN_renderpath,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"renderpath",L"renderpath",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
	st = nodeDef.AddInputPort(ID_IN_renderidentifier,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"renderidentifier",L"renderidentifier",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
	st = nodeDef.AddInputPort(ID_IN_time,ID_G_100,siICENodeDataFloat,siICENodeStructureSingle,siICENodeContextSingleton,L"time",L"time",0.0f,ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
	st = nodeDef.AddInputPort(ID_IN_usevel,ID_G_100,siICENodeDataBool,siICENodeStructureSingle,siICENodeContextSingleton,L"usevel",L"usevel",false,ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;

	// Add output ports.
	st = nodeDef.AddOutputPort(ID_OUT_position,		siICENodeDataVector3,	siICENodeStructureArray, siICENodeContextSingleton, L"position",	L"position");
	st.AssertSucceeded( ) ;
	st = nodeDef.AddOutputPort(ID_OUT_velocity,		siICENodeDataVector3,	siICENodeStructureArray, siICENodeContextSingleton, L"velocity",	L"velocity");
	st.AssertSucceeded( ) ;
	st = nodeDef.AddOutputPort(ID_OUT_faceCounts,	siICENodeDataLong,		siICENodeStructureArray, siICENodeContextSingleton, L"faceCounts",	L"faceCounts");
	st.AssertSucceeded( ) ;
	st = nodeDef.AddOutputPort(ID_OUT_faceIndices,	siICENodeDataLong,		siICENodeStructureArray, siICENodeContextSingleton, L"faceIndices",	L"faceIndices");
	st.AssertSucceeded( ) ;

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(L"Alembic");

	return CStatus::OK;
}


