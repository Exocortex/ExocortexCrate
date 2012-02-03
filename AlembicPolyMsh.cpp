#include "AlembicPolyMsh.h"
#include "Foundation.h"
#include "AlembicPolyMsh.h"
#include "AlembicXForm.h"
#include "SceneEntry.h"
#include <Object.h>
#include <triobj.h>
#include <IMetaData.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

AlembicPolyMesh::AlembicPolyMesh(const SceneEntry &in_Ref, AlembicWriteJob *in_Job)
: AlembicObject(in_Ref, in_Job)
{
    std::string meshName = in_Ref.node->GetName();
    std::string xformName = meshName + "Xfo";

    Alembic::AbcGeom::OXform xform(GetOParent(), xformName.c_str(), GetCurrentJob()->GetAnimatedTs());
    Alembic::AbcGeom::OPolyMesh mesh(xform, meshName.c_str(), GetCurrentJob()->GetAnimatedTs());

    // JSS - I'm not sure if this is require under 3DSMAx
    // AddRef(prim.GetParent3DObject().GetKinematics().GetGlobal().GetRef());

    // create the generic properties
    mOVisibility = CreateVisibilityProperty(mesh,GetCurrentJob()->GetAnimatedTs());

    mXformSchema = xform.getSchema();
    mMeshSchema = mesh.getSchema();
}

AlembicPolyMesh::~AlembicPolyMesh()
{
    // we have to clear this prior to destruction this is a workaround for issue-171
    mOVisibility.reset();
}

Alembic::Abc::OCompoundProperty AlembicPolyMesh::GetCompound()
{
    return mMeshSchema;
}

bool AlembicPolyMesh::Save(double time)
{
    // Store the transformation
    SaveXformSample(GetRef(), mXformSchema, mXformSample, time);
    
    // store the metadata
    // IMetaDataManager mng;
    // mng.GetMetaData(GetRef().node, 0);
    // SaveMetaData(prim.GetParent3DObject().GetRef(),this);

    // set the visibility
    if(GetRef().node->IsAnimated() || mNumSamples == 0)
    {
        mOVisibility.set(GetRef().node->GetPrimaryVisibility() ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);
    }

    // check if the mesh is animated (Otherwise, no need to export)
    if(mNumSamples > 0) 
    {
        if(!GetRef().node->IsAnimated())
        {
            return true;
        }
    }

    // check if we just have a pure pointcache (no surface)
    bool purePointCache = static_cast<bool>(GetCurrentJob()->GetOption("exportPurePointCache"));

    // define additional vectors, necessary for this task
    std::vector<Alembic::Abc::V3f> posVec;
    std::vector<Alembic::Abc::N3f> normalVec;
    std::vector<uint32_t> normalIndexVec;

    // access the mesh
    Object *currentObject = GetRef().obj;
	TriObject *triObj = (TriObject *)currentObject->ConvertToType(GetCurrentJob()->GetAnimatedTs(), triObjectClassID);
	Mesh &objectMesh = triObj->GetMesh();
    LONG vertCount = objectMesh.getNumVerts();

    // prepare the bounding box
    Alembic::Abc::Box3d bbox;

    // allocate the points and normals
    posVec.resize(vertCount);
    for(LONG i=0;i<vertCount;i++)
    {        
        posVec[i].x = static_cast<float>(objectMesh.getVert(i).x);
        posVec[i].y = static_cast<float>(objectMesh.getVert(i).y);
        posVec[i].z = static_cast<float>(objectMesh.getVert(i).z);
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
        return true;
    }

	// check if we support changing topology
	bool dynamicTopology = static_cast<bool>(GetCurrentJob()->GetOption("exportDynamicTopology"));

	// Get the entire face count and index Count for the mesh
    LONG faceCount = objectMesh.getNumFaces();
	LONG sampleCount = faceCount * 3;
	
	// create an index lookup table
	LONG offset = 0;
	std::vector<uint32_t> sampleLookup;
	sampleLookup.reserve(sampleCount);
	for(int f = 0; f < objectMesh.numFaces; f += 1)
	{
		for (int i = 0; i < 3; i += 1)
		{
			sampleLookup.push_back(objectMesh.faces[f].v[i]);
			offset += 1;
		}
	}

   // let's check if we have user normals
   size_t normalCount = 0;
   size_t normalIndexCount = 0;
   /*if((bool)GetJob()->GetOption(L"exportNormals"))
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
   */

	////////////////////////////////////////////////////////////////////////////////////////////////
	 // if we are the first frame!
   if(mNumSamples == 0 || (dynamicTopology))
   {
      // we also need to store the face counts as well as face indices
      if(mFaceIndicesVec.size() != sampleCount || sampleCount == 0)
      {
         mFaceCountVec.resize(faceCount);
         mFaceIndicesVec.resize(sampleCount);

         offset = 0;
         for(LONG f=0;f<faceCount;f++)
         {
            mFaceCountVec[f] = 3;
			for (int i = 0; i < 3; i += 1)
				mFaceIndicesVec[offset++] = objectMesh.faces[f].v[i];
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
      /*CRefArray clusters = mesh.GetClusters();
      if((bool)GetJob()->GetOption(L"exportUVs"))
      {
         CRef uvPropRef;
         if(GetRefCount() < 3)
         {
            for(LONG i=0;i<clusters.GetCount();i++)
            {
               Cluster cluster(clusters[i]);
               if(!cluster.GetType().IsEqualNoCase(L"sample"))
                  continue;
               CRefArray props(cluster.GetLocalProperties());
               for(LONG k=0;k<props.GetCount();k++)
               {
                  ClusterProperty prop(props[k]);
                  if(prop.GetType().IsEqualNoCase(L"uvspace"))
                  {
                     uvPropRef = props[k];
                     break;
                  }
               }
               if(uvPropRef.IsValid())
                  break;
            }
            AddRef(uvPropRef);
         }
         else
            uvPropRef = GetRef(2);

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
	  */

      // sweet, now let's have a look at face sets (really only for first sample)
      /*
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
	  */

      // save the sample
      mMeshSchema.set(mMeshSample);

      // check if we need to export the bindpose (also only for first frame)
	  /*
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
	  */
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

   // check if we should export the velocities
   /*
   if(dynamicTopology)
   {
      ICEAttribute velocitiesAttr = mesh.GetICEAttributeFromName(L"PointVelocity");
      if(velocitiesAttr.IsDefined() && velocitiesAttr.IsValid())
      {
         CICEAttributeDataArrayVector3f velocitiesData;
         velocitiesAttr.GetDataArray(velocitiesData);

         if(!mVelocityProperty.valid())
            mVelocityProperty = OV3fArrayProperty(mMeshSchema, ".velocities", mMeshSchema.getMetaData(), GetJob()->GetAnimatedTs());

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
         mVelocityProperty.set(sample);
      }
   }
   */

   mNumSamples++;




	///////////////////////////////////////////////////////////////////////////////////////////////

    

    return true;
}