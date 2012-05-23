#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicPolyMsh.h"
#include "AlembicXForm.h"
#include "SceneEnumProc.h"
#include "Utility.h"
#include "AlembicMetadataUtils.h"
#include "AlembicPointsUtils.h"
#include "AlembicIntermediatePolyMesh3DSMax.h"

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

// From the SDK
// How to calculate UV's for face mapped materials.
//static Point3 basic_tva[3] = { 
//	Point3(0.0,0.0,0.0),Point3(1.0,0.0,0.0),Point3(1.0,1.0,0.0)
//};
//static Point3 basic_tvb[3] = { 
//	Point3(1.0,1.0,0.0),Point3(0.0,1.0,0.0),Point3(0.0,0.0,0.0)
//};
//static int nextpt[3] = {1,2,0};
//static int prevpt[3] = {2,0,1};


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
	const bool bIsFirstFrame = mNumSamples == 0;

	TimeValue ticks = GetTimeValueFromFrame(time);

	Object *obj = GetRef().node->EvalWorldState(ticks).obj;
	const bool bIsParticleSystem = obj->IsParticleSystem() == 1;

	if(bIsParticleSystem){
		bForever = false;//TODO...
	}
	else{
		if(mNumSamples == 0){
			bForever = CheckIfObjIsValidForever(obj, ticks);
		}
		else{
			bool bNewForever = CheckIfObjIsValidForever(obj, ticks);
			if(bForever && bNewForever != bForever){
				ESS_LOG_INFO( "bForever has changed" );
			}
		}
	}

	const bool bFlatten = GetCurrentJob()->GetOption("flattenHierarchy");

    // Store the transformation
    SaveXformSample(GetRef(), mXformSchema, mXformSample, time, bFlatten);

	SaveMetaData(GetRef().node, this);
   
    // Clear our data
    mFaceCountVec.clear();
    mFaceIndicesVec.clear(); 
    mBindPoseVec.clear();
    mVelocitiesVec.clear();
    mUvVec.clear();
    mUvIndexVec.clear();
    mMatIdIndexVec.clear();
    mFaceSetsMap.clear();

    // store the metadata
    //IMetaDataManager mng;
    // mng.GetMetaData(GetRef().node, 0);
    // SaveMetaData(prim.GetParent3DObject().GetRef(),this);

    // set the visibility
    if(!bForever || mNumSamples == 0)
    {
        float flVisibility = GetRef().node->GetLocalVisibility(ticks);
        mOVisibility.set(flVisibility > 0 ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);
    }

    // check if the mesh is animated (Otherwise, no need to export)
    if(mNumSamples > 0) 
    {
        if(bForever)
        {
			ESS_LOG_INFO( "Node is not animated, not saving topology on subsequent frames." );
            return true;
        }
    }


	PolyObject *polyObj = NULL;
	TriObject *triObj = NULL;
	Mesh *triMesh = NULL;
	MNMesh* polyMesh = NULL;

	if (obj->CanConvertToType(Class_ID(POLYOBJ_CLASS_ID, 0)))
	{
		polyObj = reinterpret_cast<PolyObject *>(obj->ConvertToType(ticks, Class_ID(POLYOBJ_CLASS_ID, 0)));
		polyMesh = &polyObj->GetMesh();
	}
	else if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
	{
		triObj = reinterpret_cast<TriObject *>(obj->ConvertToType(ticks, Class_ID(TRIOBJ_CLASS_ID, 0)));
		triMesh = &triObj->GetMesh();
	}

	// Make sure we have a poly or a tri object
	if (polyMesh == NULL && triMesh == NULL)
	{
		return false;
	}

	IntermediatePolyMesh3DSMax currPolyMesh;
	currPolyMesh.Save(mJob, ticks, triMesh, polyMesh, GetRef().node->GetObjTMAfterWSM(ticks), GetRef().node->GetMtl(), mNumSamples);

	// Extend the archive bounding box

	if (mJob){
		mJob->GetArchiveBBox().extendBy(currPolyMesh.bbox);
	}

	{
		// allocate the sample for the points
		if(currPolyMesh.posVec.size() == 0)
		{
			currPolyMesh.bbox.extendBy(Alembic::Abc::V3f(0,0,0));
			currPolyMesh.posVec.push_back(Alembic::Abc::V3f(FLT_MAX,FLT_MAX,FLT_MAX));
		}

		Alembic::Abc::P3fArraySample posSample(&currPolyMesh.posVec.front(), currPolyMesh.posVec.size());

		// store the positions && bbox
		mMeshSample.setPositions(posSample);
		mMeshSample.setSelfBounds(currPolyMesh.bbox);
		mMeshSample.setChildBounds(currPolyMesh.bbox);
	}
	
	bool purePointCache = static_cast<bool>(GetCurrentJob()->GetOption("exportPurePointCache"));
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
	
   if(bIsFirstFrame || (dynamicTopology))//write out the Normal data
   {
	  //write out the normal data
      Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
	  normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
      if(currPolyMesh.normalVec.size() > 0)
      {
         normalSample.setVals(Alembic::Abc::N3fArraySample(&currPolyMesh.normalVec.front(), currPolyMesh.normalVec.size()));
		 if(currPolyMesh.normalIndexVec.size() > 0){
            normalSample.setIndices(Alembic::Abc::UInt32ArraySample(&currPolyMesh.normalIndexVec.front(), currPolyMesh.normalIndexVec.size()));
		 }
      }
      else if (mNumSamples == 0 && dynamicTopology)
      {
         // If we are exporting dynamic topology, then we may have normals that show up later in our scene.  The problem is that Alembic wants
         // your parameter to be defined at sample zero if you plan to use it even later on, so we create a dummy normal parameter here if the case
         // requires it
         currPolyMesh.normalVec.push_back(Imath::V3f(0,0,0));
         currPolyMesh.normalIndexVec.push_back(0);
         normalSample.setVals(Alembic::Abc::N3fArraySample(&currPolyMesh.normalVec.front(), 0));
         normalSample.setIndices(Alembic::Abc::UInt32ArraySample(&currPolyMesh.normalIndexVec.front(), 0));
      }
	  mMeshSample.setNormals(normalSample);
   }
   else
   {
      Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
      if(currPolyMesh.normalVec.size() > 0)
      {
         normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
         normalSample.setVals(Alembic::Abc::N3fArraySample(&currPolyMesh.normalVec.front(), currPolyMesh.normalVec.size()));
		 if(currPolyMesh.normalIndexVec.size() > 0){
            normalSample.setIndices(Alembic::Abc::UInt32ArraySample(&currPolyMesh.normalIndexVec.front(), currPolyMesh.normalIndexVec.size()));
		 }
         mMeshSample.setNormals(normalSample);
      }
      mMeshSchema.set(mMeshSample);
   }

	//write out the face counts and face indices
   if(bIsFirstFrame || (dynamicTopology))
   {
      // we also need to store the face counts as well as face indices
      //if(mFaceIndicesVec.size() != sampleCount || sampleCount == 0) //TODO: is this condition important? I get the impression it is useless from the original code
      {
         if(currPolyMesh.mFaceIndicesVec.size() == 0)
         {
            currPolyMesh.mFaceCountVec.push_back(0);
            currPolyMesh.mFaceIndicesVec.push_back(0);
         }
         Alembic::Abc::Int32ArraySample faceCountSample(&currPolyMesh.mFaceCountVec.front(), currPolyMesh.mFaceCountVec.size());
         Alembic::Abc::Int32ArraySample faceIndicesSample(&currPolyMesh.mFaceIndicesVec.front(), currPolyMesh.mFaceIndicesVec.size());

         mMeshSample.setFaceCounts(faceCountSample);
         mMeshSample.setFaceIndices(faceIndicesSample);
      }
   }

	//write out the texture coordinates if necessary
	if((bool)GetCurrentJob()->GetOption("exportUVs") && (bIsFirstFrame || dynamicTopology))
	{
		if (currPolyMesh.mUvVec.size() == currPolyMesh.sampleCount)
		{
			if (currPolyMesh.mUvVec.size() > 0)
			{
				Alembic::AbcGeom::OV2fGeomParam::Sample uvSample(
					Alembic::Abc::V2fArraySample(&currPolyMesh.mUvVec.front(),currPolyMesh.mUvVec.size()),
					Alembic::AbcGeom::kFacevaryingScope);

				if(mUvIndexVec.size() > 0){
				  uvSample.setIndices(Alembic::Abc::UInt32ArraySample(&currPolyMesh.mUvIndexVec.front(),currPolyMesh.mUvIndexVec.size()));
				}
				mMeshSample.setUVs(uvSample);
			}
			else if (mNumSamples == 0 && dynamicTopology)
			{
				// If we are exporting dynamic topology, then we may have uvs that show up later in our scene.  The problem is that Alembic wants
				// your parameter to be defined at sample zero if you plan to use it even later on, so we create a dummy uv parameter here if the case
				// requires it
				currPolyMesh.mUvVec.push_back(Imath::V2f(0,0));
				currPolyMesh.mUvIndexVec.push_back(0);
				Alembic::AbcGeom::OV2fGeomParam::Sample uvSample(Alembic::Abc::V2fArraySample(&mUvVec.front(), 0), Alembic::AbcGeom::kFacevaryingScope);
				uvSample.setIndices(Alembic::Abc::UInt32ArraySample(&mUvIndexVec.front(), 0) );
				mMeshSample.setUVs(uvSample);
			}
		}
	}

#if 0
   if(bIsFirstFrame || (dynamicTopology))
   {
      // sweet, now let's have a look at face sets (really only for first sample)
      // for 3DS Max, we are mapping this to the material ids
      std::vector<boost::int32_t> zeroFaceVector;
	  if(GetCurrentJob()->GetOption("exportMaterialIds") && (mNumSamples == 0 || dynamicTopology))
      {
          if (polyMesh != NULL || triMesh != NULL)
          {
              if(!mMatIdProperty.valid())
              {
                  mMatIdProperty = OUInt32ArrayProperty(mMeshSchema, ".materialids", mMeshSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs());
              }

			  int numMatId = 0;
              int numFaces = polyMesh ? polyMesh->numf : triMesh->getNumFaces();
              mMatIdIndexVec.resize(numFaces);
              for (int i = 0; i < numFaces; i += 1)
              {
                  int matId = polyMesh ? polyMesh->f[i].material : triMesh->faces[i].getMatID();
                  mMatIdIndexVec[i] = matId;

                  // Record the face set map if sample zero
                  if (mNumSamples == 0)
                  {
                      facesetmap_it it;
                      it = mFaceSetsMap.find(matId);

                      if (it == mFaceSetsMap.end())
                      {
                          facesetmap_ret_pair ret = mFaceSetsMap.insert(facesetmap_insert_pair(matId, std::vector<int32_t>()));
                          it = ret.first;
						  numMatId++;
                      }

                      it->second.push_back(i);
                  }
              }

              size_t nMatIndexSize = mMatIdIndexVec.size();
              if(nMatIndexSize == 0)
                  mMatIdIndexVec.push_back(0);
              Alembic::Abc::UInt32ArraySample sample = Alembic::Abc::UInt32ArraySample(&mMatIdIndexVec.front(), nMatIndexSize);
              mMatIdProperty.set(sample);

			  Mtl* pMat = GetRef().node->GetMtl();

              // For sample zero, export the material ids as face sets
			  if (mNumSamples == 0 && numMatId > 1 )
              {
				  int i = 0;
				  for ( facesetmap_it it=mFaceSetsMap.begin(); it != mFaceSetsMap.end(); it++)
                  {
                     

					  Mtl* pSubMat = NULL;
					  if(pMat && i<pMat->NumSubMtls()){
					      pSubMat = pMat->GetSubMtl(i);
					  }
					  std::stringstream nameStream;
					  int nMaterialId = it->first+1;
					  if(pSubMat){
                          nameStream<<pSubMat->GetName();
					  }
					  else if(pMat){
						  nameStream<<pMat->GetName();
					  }
					  else{
					      nameStream<<"Unnamed";
					  }
					  nameStream<<" : "<<nMaterialId;
					  std::string name = nameStream.str();

                      std::vector<int32_t> & faceSetVec = it->second;

                      Alembic::AbcGeom::OFaceSet faceSet = mMeshSchema.createFaceSet(name);
                      Alembic::AbcGeom::OFaceSetSchema::Sample faceSetSample(Alembic::Abc::Int32ArraySample(&faceSetVec.front(),faceSetVec.size()));
                      faceSet.getSchema().set(faceSetSample);

					  i++;
                  }
              }
          }
      }

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
#endif

   // check if we should export the velocities
   /*if(dynamicTopology)
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


   // save the sample
   mMeshSchema.set(mMeshSample);

   mNumSamples++;

   // Note that the TriObject should only be deleted
   // if the pointer to it is not equal to the object
   // pointer that called ConvertToType()
   if (polyObj != NULL && polyObj != obj)
   {
       delete polyObj;
       polyObj = NULL;
   }

   if (triObj != NULL && triObj != obj)
   {
       delete triObj;
       triObj = NULL;
   }

  // if(particleMesh != NULL && bParticleMeshNeedDelete)
  // {
		//delete particleMesh;
  // }

	///////////////////////////////////////////////////////////////////////////////////////////////

    return true;
}
