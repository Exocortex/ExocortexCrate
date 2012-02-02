#include "AlembicPolyMsh.h"
#include "AlembicXform.h"

#include <xsi_application.h>
#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_geometry.h>
#include <xsi_polygonmesh.h>
#include <xsi_vertex.h>
#include <xsi_polygonface.h>
#include <xsi_sample.h>
#include <xsi_math.h>
#include <xsi_context.h>
#include <xsi_operatorcontext.h>
#include <xsi_customoperator.h>
#include <xsi_factory.h>
#include <xsi_parameter.h>
#include <xsi_ppglayout.h>
#include <xsi_ppgitem.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_clusterproperty.h>
#include <xsi_cluster.h>
#include <xsi_geometryaccessor.h>
#include <xsi_material.h>
#include <xsi_materiallibrary.h>
#include <xsi_iceattribute.h>
#include <xsi_iceattributedataarray.h>

using namespace XSI;
using namespace MATH;

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

AlembicPolyMesh::AlembicPolyMesh(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   Primitive prim(GetRef());
   CString meshName(prim.GetParent3DObject().GetName());
   CString xformName(meshName+L"Xfo");
   Alembic::AbcGeom::OXform xform(GetOParent(),xformName.GetAsciiString(),GetJob()->GetAnimatedTs());
   Alembic::AbcGeom::OPolyMesh mesh(xform,meshName.GetAsciiString(),GetJob()->GetAnimatedTs());
   AddRef(prim.GetParent3DObject().GetKinematics().GetGlobal().GetRef());

   // create the generic properties
   mOVisibility = CreateVisibilityProperty(mesh,GetJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();
   mMeshSchema = mesh.getSchema();
}

AlembicPolyMesh::~AlembicPolyMesh()
{
   // we have to clear this prior to destruction
   // this is a workaround for issue-171
   mOVisibility.reset();
}

Alembic::Abc::OCompoundProperty AlembicPolyMesh::GetCompound()
{
   return mMeshSchema;
}

XSI::CStatus AlembicPolyMesh::Save(double time)
{
   // store the transform
   Primitive prim(GetRef());
   SaveXformSample(GetRef(1),mXformSchema,mXformSample,time);

   // store the metadata
   SaveMetaData(prim.GetParent3DObject().GetRef(),this);

   // set the visibility
   Property visProp;
   prim.GetParent3DObject().GetPropertyFromName(L"Visibility",visProp);
   if(isRefAnimated(visProp.GetRef()) || mNumSamples == 0)
   {
      bool visibility = visProp.GetParameterValue(L"rendvis",time);
      mOVisibility.set(visibility ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);
   }

   // check if the mesh is animated
   if(mNumSamples > 0) {
      if(!isRefAnimated(GetRef()))
         return CStatus::OK;
   }

   // check if we just have a pure pointcache (no surface)
   bool purePointCache = (bool)GetJob()->GetOption(L"exportPurePointCache");

   // define additional vectors, necessary for this task
   std::vector<Alembic::Abc::V3f> posVec;
   std::vector<Alembic::Abc::N3f> normalVec;
   std::vector<uint32_t> normalIndexVec;

   // access the mesh
   PolygonMesh mesh = prim.GetGeometry(time);
   CVector3Array pos = mesh.GetVertices().GetPositionArray();
   LONG vertCount = pos.GetCount();

   // prepare the bounding box
   Alembic::Abc::Box3d bbox;

   // allocate the points and normals
   posVec.resize(vertCount);
   for(LONG i=0;i<vertCount;i++)
   {
      posVec[i].x = (float)pos[i].GetX();
      posVec[i].y = (float)pos[i].GetY();
      posVec[i].z = (float)pos[i].GetZ();
      bbox.extendBy(posVec[i]);
   }

   // allocate the sample for the points
   if(posVec.size() == 0)
   {
      bbox.extendBy(Alembic::Abc::V3f(0,0,0));
      posVec.push_back(Alembic::Abc::V3f(FLT_MAX,FLT_MAX,FLT_MAX));
   }
   Alembic::Abc::P3fArraySample posSample(&posVec.front(),posVec.size());

   // store the positions && bbox
   mMeshSample.setPositions(posSample);
   mMeshSample.setSelfBounds(bbox);

   // abort here if we are just storing points
   if(purePointCache)
   {
      if(mNumSamples == 0)
      {
         // store a dummy empty topology
         mFaceCountVec.push_back(0);
         mFaceIndicesVec.push_back(0);
         Alembic::Abc::Int32ArraySample faceCountSample(&mFaceCountVec.front(),mFaceCountVec.size());
         Alembic::Abc::Int32ArraySample faceIndicesSample(&mFaceIndicesVec.front(),mFaceIndicesVec.size());
         mMeshSample.setFaceCounts(faceCountSample);
         mMeshSample.setFaceIndices(faceIndicesSample);
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

   // let's check if we have user normals
   size_t normalCount = 0;
   size_t normalIndexCount = 0;
   if((bool)GetJob()->GetOption(L"exportNormals"))
   {
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
      normalCount = normalVec.size();

      for(LONG i=0;i<sampleCount;i++)
      {
         LONG lookedup = sampleLookup[i];
         normalVec[i].x = shadingNormals[lookedup * 3 + 0];
         normalVec[i].y = shadingNormals[lookedup * 3 + 1];
         normalVec[i].z = shadingNormals[lookedup * 3 + 2];
      }

      // now let's sort the normals 
      if((bool)GetJob()->GetOption(L"indexedNormals")) {
         std::map<SortableV3f,size_t> normalMap;
         std::map<SortableV3f,size_t>::const_iterator it;
         size_t sortedNormalCount = 0;
         std::vector<Alembic::Abc::V3f> sortedNormalVec;
         normalIndexVec.resize(normalVec.size());
         sortedNormalVec.resize(normalVec.size());

         // loop over all normals
         for(size_t i=0;i<normalVec.size();i++)
         {
            it = normalMap.find(normalVec[i]);
            if(it != normalMap.end())
               normalIndexVec[normalIndexCount++] = (uint32_t)it->second;
            else
            {
               normalIndexVec[normalIndexCount++] = (uint32_t)sortedNormalCount;
               normalMap.insert(std::pair<Alembic::Abc::V3f,size_t>(normalVec[i],(uint32_t)sortedNormalCount));
               sortedNormalVec[sortedNormalCount++] = normalVec[i];
            }
         }

         // use indexed normals if they use less space
         if(sortedNormalCount * sizeof(Alembic::Abc::V3f) + 
            normalIndexCount * sizeof(uint32_t) < 
            sizeof(Alembic::Abc::V3f) * normalVec.size())
         {
            normalVec = sortedNormalVec;
            normalCount = sortedNormalCount;
         }
         else
         {
            normalIndexCount = 0;
            normalIndexVec.clear();
         }
         sortedNormalCount = 0;
         sortedNormalVec.clear();
      }
   }

   // check if we should export the velocities
   if(dynamicTopology)
   {
      ICEAttribute velocitiesAttr = mesh.GetICEAttributeFromName(L"PointVelocity");
      if(velocitiesAttr.IsDefined() && velocitiesAttr.IsValid())
      {
         CICEAttributeDataArrayVector3f velocitiesData;
         velocitiesAttr.GetDataArray(velocitiesData);

         mVelocitiesVec.resize(vertCount);
         for(LONG i=0;i<vertCount;i++)
         {
            mVelocitiesVec[i].x = velocitiesData[i].GetX();
            mVelocitiesVec[i].y = velocitiesData[i].GetY();
            mVelocitiesVec[i].z = velocitiesData[i].GetZ();
         }

         if(mVelocitiesVec.size() == 0)
            mVelocitiesVec.push_back(Alembic::Abc::V3f(0,0,0));


         Alembic::Abc::V3fArraySample sample = Alembic::Abc::V3fArraySample(&mVelocitiesVec.front(),mVelocitiesVec.size());
         mMeshSample.setVelocities(sample);
      }
   }

   // if we are the first frame!
   if(mNumSamples == 0 || (dynamicTopology))
   {
      // we also need to store the face counts as well as face indices
      if(mFaceIndicesVec.size() != sampleCount || sampleCount == 0)
      {
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

         if(mFaceIndicesVec.size() == 0)
         {
            mFaceCountVec.push_back(0);
            mFaceIndicesVec.push_back(0);
         }
         Alembic::Abc::Int32ArraySample faceCountSample(&mFaceCountVec.front(),mFaceCountVec.size());
         Alembic::Abc::Int32ArraySample faceIndicesSample(&mFaceIndicesVec.front(),mFaceIndicesVec.size());

         mMeshSample.setFaceCounts(faceCountSample);
         mMeshSample.setFaceIndices(faceIndicesSample);
      }

      Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
      if(normalVec.size() > 0 && normalCount > 0)
      {
         normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
         normalSample.setVals(Alembic::Abc::N3fArraySample(&normalVec.front(),normalCount));
         if(normalIndexCount > 0)
            normalSample.setIndices(Alembic::Abc::UInt32ArraySample(&normalIndexVec.front(),normalIndexCount));
         mMeshSample.setNormals(normalSample);
      }

      // also check if we need to store UV
      CRefArray clusters = mesh.GetClusters();
      if((bool)GetJob()->GetOption(L"exportUVs"))
      {
         CGeometryAccessor accessor = mesh.GetGeometryAccessor(siConstructionModeSecondaryShape);
         CRef uvPropRef;
         if(accessor.GetUVs().GetCount()>0)
            uvPropRef = accessor.GetUVs()[0];

         // if we now finally found a valid uvprop
         if(uvPropRef.IsValid())
         {
            // ok, great, we found UVs, let's set them up
            mUvVec.resize(sampleCount);
            CDoubleArray uvValues = ClusterProperty(uvPropRef).GetElements().GetArray();

            for(LONG i=0;i<sampleCount;i++)
            {
               mUvVec[i].x = (float)uvValues[sampleLookup[i] * 3 + 0];
               mUvVec[i].y = (float)uvValues[sampleLookup[i] * 3 + 1];
            }

            // now let's sort the normals 
            size_t uvCount = mUvVec.size();
            size_t uvIndexCount = 0;
            if((bool)GetJob()->GetOption(L"indexedUVs")) {
               std::map<SortableV2f,size_t> uvMap;
               std::map<SortableV2f,size_t>::const_iterator it;
               size_t sortedUVCount = 0;
               std::vector<Alembic::Abc::V2f> sortedUVVec;
               mUvIndexVec.resize(mUvVec.size());
               sortedUVVec.resize(mUvVec.size());

               // loop over all uvs
               for(size_t i=0;i<mUvVec.size();i++)
               {
                  it = uvMap.find(mUvVec[i]);
                  if(it != uvMap.end())
                     mUvIndexVec[uvIndexCount++] = (uint32_t)it->second;
                  else
                  {
                     mUvIndexVec[uvIndexCount++] = (uint32_t)sortedUVCount;
                     uvMap.insert(std::pair<Alembic::Abc::V2f,size_t>(mUvVec[i],(uint32_t)sortedUVCount));
                     sortedUVVec[sortedUVCount++] = mUvVec[i];
                  }
               }

               // use indexed uvs if they use less space
               if(sortedUVCount * sizeof(Alembic::Abc::V2f) + 
                  uvIndexCount * sizeof(uint32_t) < 
                  sizeof(Alembic::Abc::V2f) * mUvVec.size())
               {
                  mUvVec = sortedUVVec;
                  uvCount = sortedUVCount;
               }
               else
               {
                  uvIndexCount = 0;
                  mUvIndexVec.clear();
               }
               sortedUVCount = 0;
               sortedUVVec.clear();
            }


            Alembic::AbcGeom::OV2fGeomParam::Sample uvSample(Alembic::Abc::V2fArraySample(&mUvVec.front(),uvCount),Alembic::AbcGeom::kFacevaryingScope);
            if(mUvIndexVec.size() > 0 && uvIndexCount > 0)
               uvSample.setIndices(Alembic::Abc::UInt32ArraySample(&mUvIndexVec.front(),uvIndexCount));
            mMeshSample.setUVs(uvSample);
         }
      }

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

            mFaceSetsVec.push_back(std::vector<int32_t>());
            std::vector<int32_t> & faceSetVec = mFaceSetsVec.back();
            for(LONG j=0;j<elements.GetCount();j++)
               faceSetVec.push_back(elements[j]);

            if(faceSetVec.size() > 0)
            {
               Alembic::AbcGeom::OFaceSet faceSet = mMeshSchema.createFaceSet(name);
               Alembic::AbcGeom::OFaceSetSchema::Sample faceSetSample(Alembic::Abc::Int32ArraySample(&faceSetVec.front(),faceSetVec.size()));
               faceSet.getSchema().set(faceSetSample);
            }
         }
      }

      // save the sample
      mMeshSchema.set(mMeshSample);

      // check if we need to export the bindpose (also only for first frame)
      if(GetJob()->GetOption(L"exportBindPose") && prim.GetParent3DObject().GetEnvelopes().GetCount() > 0 && mNumSamples == 0)
      {
         mBindPoseProperty = OV3fArrayProperty(mMeshSchema, ".bindpose", mMeshSchema.getMetaData(), GetJob()->GetAnimatedTs());

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

         Alembic::Abc::V3fArraySample sample;
         if(mBindPoseVec.size() > 0)
            sample = Alembic::Abc::V3fArraySample(&mBindPoseVec.front(),mBindPoseVec.size());
         mBindPoseProperty.set(sample);
      }   
   }
   else
   {
      Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
      if(normalVec.size() > 0 && normalCount > 0)
      {
         normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
         normalSample.setVals(Alembic::Abc::N3fArraySample(&normalVec.front(),normalCount));
         if(normalIndexCount > 0)
            normalSample.setIndices(Alembic::Abc::UInt32ArraySample(&normalIndexVec.front(),normalIndexCount));
         mMeshSample.setNormals(normalSample);
      }
      mMeshSchema.set(mMeshSample);
   }

   mNumSamples++;

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_polymesh_Define( CRef& in_ctxt )
{
   return alembicOp_Define(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_polymesh_DefineLayout( CRef& in_ctxt )
{
   return alembicOp_DefineLayout(in_ctxt);
}


XSIPLUGINCALLBACK CStatus alembic_polymesh_Update( CRef& in_ctxt )
{
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
   Alembic::AbcGeom::IPolyMesh objMesh;
   Alembic::AbcGeom::ISubD objSubD;
   if(Alembic::AbcGeom::IPolyMesh::matches(iObj.getMetaData()))
      objMesh = Alembic::AbcGeom::IPolyMesh(iObj,Alembic::Abc::kWrapExisting);
   else
      objSubD = Alembic::AbcGeom::ISubD(iObj,Alembic::Abc::kWrapExisting);
   if(!objMesh.valid() && !objSubD.valid())
      return CStatus::OK;

   SampleInfo sampleInfo;
   if(objMesh.valid())
      sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         objMesh.getSchema().getTimeSampling(),
         objMesh.getSchema().getNumSamples()
      );
   else
      sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         objSubD.getSchema().getTimeSampling(),
         objSubD.getSchema().getNumSamples()
      );

   Alembic::Abc::P3fArraySamplePtr meshPos;
   if(objMesh.valid())
   {
      Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
      objMesh.getSchema().get(sample,sampleInfo.floorIndex);
      meshPos = sample.getPositions();
   }
   else
   {
      Alembic::AbcGeom::ISubDSchema::Sample sample;
      objSubD.getSchema().get(sample,sampleInfo.floorIndex);
      meshPos = sample.getPositions();
   }

   PolygonMesh inMesh = Primitive((CRef)ctxt.GetInputValue(0)).GetGeometry();
   CVector3Array pos = inMesh.GetPoints().GetPositionArray();

   if(pos.GetCount() != meshPos->size())
      return CStatus::OK;

   for(size_t i=0;i<meshPos->size();i++)
      pos[(LONG)i].Set(meshPos->get()[i].x,meshPos->get()[i].y,meshPos->get()[i].z);

   // blend
   if(sampleInfo.alpha != 0.0)
   {
      if(objMesh.valid())
      {
         Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
         objMesh.getSchema().get(sample,sampleInfo.ceilIndex);
         meshPos = sample.getPositions();
      }
      else
      {
         Alembic::AbcGeom::ISubDSchema::Sample sample;
         objSubD.getSchema().get(sample,sampleInfo.ceilIndex);
         meshPos = sample.getPositions();
      }
      for(size_t i=0;i<meshPos->size();i++)
         pos[(LONG)i].LinearlyInterpolate(pos[(LONG)i],CVector3(meshPos->get()[i].x,meshPos->get()[i].y,meshPos->get()[i].z),sampleInfo.alpha);
   }

   Primitive(ctxt.GetOutputTarget()).GetGeometry().GetPoints().PutPositionArray(pos);

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_polymesh_Term(CRef & in_ctxt)
{
   return alembicOp_Term(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_normals_Define( CRef& in_ctxt )
{
   return alembicOp_Define(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_normals_DefineLayout( CRef& in_ctxt )
{
   return alembicOp_DefineLayout(in_ctxt);
}


XSIPLUGINCALLBACK CStatus alembic_normals_Update( CRef& in_ctxt )
{
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
   Alembic::AbcGeom::IPolyMesh obj(iObj,Alembic::Abc::kWrapExisting);
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

   Alembic::AbcGeom::IN3fGeomParam meshNormalsParam = obj.getSchema().getNormalsParam();
   if(meshNormalsParam.valid())
   {
      Alembic::Abc::N3fArraySamplePtr meshNormals = meshNormalsParam.getExpandedValue(sampleInfo.floorIndex).getVals();
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
         if(sampleInfo.alpha != 0.0)
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
}

XSIPLUGINCALLBACK CStatus alembic_normals_Term(CRef & in_ctxt)
{
   Context ctxt( in_ctxt );
   CustomOperator op(ctxt.GetSource());
   delRefArchive(op.GetParameterValue(L"path").GetAsText());
   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_uvs_Define( CRef& in_ctxt )
{
   return alembicOp_Define(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_uvs_DefineLayout( CRef& in_ctxt )
{
   return alembicOp_DefineLayout(in_ctxt);
}


XSIPLUGINCALLBACK CStatus alembic_uvs_Update( CRef& in_ctxt )
{
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
   Alembic::AbcGeom::IPolyMesh objMesh;
   Alembic::AbcGeom::ISubD objSubD;
   if(Alembic::AbcGeom::IPolyMesh::matches(iObj.getMetaData()))
      objMesh = Alembic::AbcGeom::IPolyMesh(iObj,Alembic::Abc::kWrapExisting);
   else
      objSubD = Alembic::AbcGeom::ISubD(iObj,Alembic::Abc::kWrapExisting);
   if(!objMesh.valid() && !objSubD.valid())
      return CStatus::OK;

   CDoubleArray uvValues = ClusterProperty((CRef)ctxt.GetInputValue(0)).GetElements().GetArray();
   PolygonMesh mesh = Primitive((CRef)ctxt.GetInputValue(1)).GetGeometry();
   CPolygonFaceRefArray faces = mesh.GetPolygons();
   CGeometryAccessor accessor = mesh.GetGeometryAccessor(siConstructionModeModeling);
   CLongArray counts;
   accessor.GetPolygonVerticesCount(counts);

   Alembic::AbcGeom::IV2fGeomParam meshUvParam;
   if(objMesh.valid())
      meshUvParam = objMesh.getSchema().getUVsParam();
   else
      meshUvParam = objSubD.getSchema().getUVsParam();

   if(meshUvParam.valid())
   {
      SampleInfo sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         meshUvParam.getTimeSampling(),
         meshUvParam.getNumSamples()
      );

      Alembic::Abc::V2fArraySamplePtr meshUVs = meshUvParam.getExpandedValue(sampleInfo.floorIndex).getVals();
      if(meshUVs->size() * 3 == uvValues.GetCount())
      {
         // create a sample look table
         LONG offset = 0;
         CLongArray sampleLookup(accessor.GetNodeCount());
         for(LONG i=0;i<faces.GetCount();i++)
         {
            PolygonFace face(faces[i]);
            CLongArray samples = face.GetSamples().GetIndexArray();
            for(LONG j=samples.GetCount()-1;j>=0;j--)
               sampleLookup[samples[j]] = offset++;
               //sampleLookup[offset++] = samples[j];
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
               uvValues[offset++] = uvValues[offset] * ialpha + meshUVs->get()[sampleLookup[i]].x * sampleInfo.alpha;;
               uvValues[offset++] = uvValues[offset] * ialpha + meshUVs->get()[sampleLookup[i]].y * sampleInfo.alpha;;
               uvValues[offset++] = 0.0;
            }
         }
      }
   }

   ClusterProperty(ctxt.GetOutputTarget()).GetElements().PutArray(uvValues);

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_uvs_Term(CRef & in_ctxt)
{
   return alembicOp_Term(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_polymesh_topo_Define( CRef& in_ctxt )
{
   return alembicOp_Define(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_polymesh_topo_DefineLayout( CRef& in_ctxt )
{
   return alembicOp_DefineLayout(in_ctxt);
}


XSIPLUGINCALLBACK CStatus alembic_polymesh_topo_Update( CRef& in_ctxt )
{
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
   Alembic::AbcGeom::IPolyMesh objMesh;
   Alembic::AbcGeom::ISubD objSubD;
   if(Alembic::AbcGeom::IPolyMesh::matches(iObj.getMetaData()))
      objMesh = Alembic::AbcGeom::IPolyMesh(iObj,Alembic::Abc::kWrapExisting);
   else
      objSubD = Alembic::AbcGeom::ISubD(iObj,Alembic::Abc::kWrapExisting);
   if(!objMesh.valid() && !objSubD.valid())
      return CStatus::OK;

   SampleInfo sampleInfo;
   if(objMesh.valid())
   {
      sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         objMesh.getSchema().getTimeSampling(),
         objMesh.getSchema().getNumSamples()
      );
   }
   else
   {
      sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         objSubD.getSchema().getTimeSampling(),
         objSubD.getSchema().getNumSamples()
      );
   }

   Alembic::Abc::P3fArraySamplePtr meshPos;
   Alembic::Abc::V3fArraySamplePtr meshVel;
   Alembic::Abc::Int32ArraySamplePtr meshFaceCount;
   Alembic::Abc::Int32ArraySamplePtr meshFaceIndices;

   if(objMesh.valid())
   {
      Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
      objMesh.getSchema().get(sample,sampleInfo.floorIndex);
      meshPos = sample.getPositions();
      meshVel = sample.getVelocities();
      meshFaceCount = sample.getFaceCounts();
      meshFaceIndices = sample.getFaceIndices();
   }
   else
   {
      Alembic::AbcGeom::ISubDSchema::Sample sample;
      objSubD.getSchema().get(sample,sampleInfo.floorIndex);
      meshPos = sample.getPositions();
      meshVel = sample.getVelocities();
      meshFaceCount = sample.getFaceCounts();
      meshFaceIndices = sample.getFaceIndices();
   }

   CVector3Array pos((LONG)meshPos->size());
   CLongArray polies((LONG)(meshFaceCount->size() + meshFaceIndices->size()));

   for(size_t j=0;j<meshPos->size();j++)
      pos[(LONG)j].Set(meshPos->get()[j].x,meshPos->get()[j].y,meshPos->get()[j].z);

   // check if this is an empty topo object
   if(meshFaceCount->size() > 0)
   {
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
         Alembic::Abc::int32_t offset2 = 0;
         for(size_t j=0;j<meshFaceCount->size();j++)
         {
            Alembic::Abc::int32_t singleFaceCount = meshFaceCount->get()[j];
            polies[offset1++] = singleFaceCount;
            offset2 += singleFaceCount;
            for(size_t k=0;k<singleFaceCount;k++)
            {
               polies[offset1++] = meshFaceIndices->get()[(size_t)offset2 - 1 - k];
            }
         }
      }
   }

   // do the positional interpolation if necessary
   if(objMesh.valid() && sampleInfo.alpha != 0.0)
   {
      double alpha = sampleInfo.alpha;
      double ialpha = 1.0 - alpha;

      // first check if the next frame has the same point count
      if(objMesh.valid())
      {
         Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
         objMesh.getSchema().get(sample,sampleInfo.ceilIndex);
         meshPos = sample.getPositions();
      }

      if(meshPos->size() == (size_t)pos.GetCount())
      {
         for(LONG i=0;i<(LONG)meshPos->size();i++)
         {
            pos[i].PutX(ialpha * pos[i].GetX() + alpha * meshPos->get()[i].x);
            pos[i].PutY(ialpha * pos[i].GetY() + alpha * meshPos->get()[i].y);
            pos[i].PutZ(ialpha * pos[i].GetZ() + alpha * meshPos->get()[i].z);
         }
      }
      else if(meshVel)
      {
         if(meshVel->size() == meshPos->size())
         {
            for(LONG i=0;i<(LONG)meshVel->size();i++)
            {
               pos[i].PutX(pos[i].GetX() + alpha * meshVel->get()[i].x);
               pos[i].PutY(pos[i].GetY() + alpha * meshVel->get()[i].y);
               pos[i].PutZ(pos[i].GetZ() + alpha * meshVel->get()[i].z);
            }
         }
      }
   }

   PolygonMesh outMesh = Primitive(ctxt.GetOutputTarget()).GetGeometry();
   outMesh.Set(pos,polies);

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_polymesh_topo_Term(CRef & in_ctxt)
{
   return alembicOp_Term(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_bbox_Define( CRef& in_ctxt )
{
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
}

XSIPLUGINCALLBACK CStatus alembic_bbox_DefineLayout( CRef& in_ctxt )
{
   alembicOp_DefineLayout(in_ctxt);

   Context ctxt( in_ctxt );
   PPGLayout oLayout;
   PPGItem oItem;
   oLayout = ctxt.GetSource();
   oLayout.AddItem(L"extend",L"Extend Box");
   return CStatus::OK;
}


XSIPLUGINCALLBACK CStatus alembic_bbox_Update( CRef& in_ctxt )
{
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");
   float extend = ctxt.GetParameterValue(L"extend");

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;

   Alembic::Abc::Box3d box;

   
   // check what kind of object we have
   const Alembic::Abc::MetaData &md = iObj.getMetaData();
   if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      Alembic::AbcGeom::IPolyMesh obj(iObj,Alembic::Abc::kWrapExisting);
      if(!obj.valid())
         return CStatus::OK;

      SampleInfo sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         obj.getSchema().getTimeSampling(),
         obj.getSchema().getNumSamples()
      );

      Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
      obj.getSchema().get(sample,sampleInfo.floorIndex);
      box = sample.getSelfBounds();

      if(sampleInfo.alpha > 0.0)
      {
         obj.getSchema().get(sample,sampleInfo.ceilIndex);
         Alembic::Abc::Box3d box2 = sample.getSelfBounds();

         box.min = (1.0 - sampleInfo.alpha) * box.min + sampleInfo.alpha * box2.min;
         box.max = (1.0 - sampleInfo.alpha) * box.max + sampleInfo.alpha * box2.max;
      }
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      Alembic::AbcGeom::ICurves obj(iObj,Alembic::Abc::kWrapExisting);
      if(!obj.valid())
         return CStatus::OK;

      SampleInfo sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         obj.getSchema().getTimeSampling(),
         obj.getSchema().getNumSamples()
      );

      Alembic::AbcGeom::ICurvesSchema::Sample sample;
      obj.getSchema().get(sample,sampleInfo.floorIndex);
      box = sample.getSelfBounds();

      if(sampleInfo.alpha > 0.0)
      {
         obj.getSchema().get(sample,sampleInfo.ceilIndex);
         Alembic::Abc::Box3d box2 = sample.getSelfBounds();

         box.min = (1.0 - sampleInfo.alpha) * box.min + sampleInfo.alpha * box2.min;
         box.max = (1.0 - sampleInfo.alpha) * box.max + sampleInfo.alpha * box2.max;
      }
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      Alembic::AbcGeom::IPoints obj(iObj,Alembic::Abc::kWrapExisting);
      if(!obj.valid())
         return CStatus::OK;

      SampleInfo sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         obj.getSchema().getTimeSampling(),
         obj.getSchema().getNumSamples()
      );

      Alembic::AbcGeom::IPointsSchema::Sample sample;
      obj.getSchema().get(sample,sampleInfo.floorIndex);
      box = sample.getSelfBounds();

      if(sampleInfo.alpha > 0.0)
      {
         obj.getSchema().get(sample,sampleInfo.ceilIndex);
         Alembic::Abc::Box3d box2 = sample.getSelfBounds();

         box.min = (1.0 - sampleInfo.alpha) * box.min + sampleInfo.alpha * box2.min;
         box.max = (1.0 - sampleInfo.alpha) * box.max + sampleInfo.alpha * box2.max;
      }
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      Alembic::AbcGeom::ISubD obj(iObj,Alembic::Abc::kWrapExisting);
      if(!obj.valid())
         return CStatus::OK;

      SampleInfo sampleInfo = getSampleInfo(
         ctxt.GetParameterValue(L"time"),
         obj.getSchema().getTimeSampling(),
         obj.getSchema().getNumSamples()
      );

      Alembic::AbcGeom::ISubDSchema::Sample sample;
      obj.getSchema().get(sample,sampleInfo.floorIndex);
      box = sample.getSelfBounds();

      if(sampleInfo.alpha > 0.0)
      {
         obj.getSchema().get(sample,sampleInfo.ceilIndex);
         Alembic::Abc::Box3d box2 = sample.getSelfBounds();

         box.min = (1.0 - sampleInfo.alpha) * box.min + sampleInfo.alpha * box2.min;
         box.max = (1.0 - sampleInfo.alpha) * box.max + sampleInfo.alpha * box2.max;
      }
   }

   Primitive inPrim((CRef)ctxt.GetInputValue(0));
   CVector3Array pos = inPrim.GetGeometry().GetPoints().GetPositionArray();

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
}

XSIPLUGINCALLBACK CStatus alembic_bbox_Term(CRef & in_ctxt)
{
   return alembicOp_Term(in_ctxt);
}
