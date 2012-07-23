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


void AlembicPolyMesh::SaveMaterialsProperty(bool bFirstFrame, bool bLastFrame)
{
	if(bLastFrame){
		std::vector<std::string> materialNames;
		mergedMeshMaterialsMap& matMap = materialsMerge.groupMatMap;

		for ( mergedMeshMaterialsMap_it it=matMap.begin(); it != matMap.end(); it++)
		{
			meshMaterialsMap& map = it->second;
			for( meshMaterialsMap_it it2 =map.begin(); it2 != map.end(); it2++)
			{
				std::stringstream nameStream;
				int nMaterialId = it2->second.matId+1;
				nameStream<<it2->second.name<<" : "<<nMaterialId;
				materialNames.push_back(nameStream.str());
			}
		}

		if(materialNames.size() > 0){
			if(!mMatNamesProperty.valid())
			{
				mMatNamesProperty = OStringArrayProperty(mMeshSchema, ".materialnames", mMeshSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs());
			}
			Alembic::Abc::StringArraySample sample = Alembic::Abc::StringArraySample(materialNames);
			mMatNamesProperty.set(sample);
		}
	}
}


bool AlembicPolyMesh::Save(double time, bool bLastFrame)
{   
	const bool bFirstFrame = mNumSamples == 0;

	TimeValue ticks = GetTimeValueFromFrame(time);

	Object *obj = GetRef().node->EvalWorldState(ticks).obj;
	const bool bIsParticleSystem = obj->IsParticleSystem() == 1;

	if(bIsParticleSystem){
		bForever = false;
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

	IntermediatePolyMesh3DSMax finalPolyMesh;

	if(bIsParticleSystem){

		bool bSuccess = getParticleSystemMesh(ticks, obj, GetRef().node, &finalPolyMesh, &materialsMerge, mJob, mNumSamples);
		if(!bSuccess){
			ESS_LOG_INFO( "Error. Could not get particle system mesh. Time: "<<time );
			return false;
		}
	}
	else
	{
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

		//keep the orignal material IDs, since we are not saving out a single nonmerged mesh
		materialsMerge.bPreserveIds = true;

		Matrix3 worldTrans;
		worldTrans.IdentityMatrix();
		finalPolyMesh.Save(mJob, ticks, triMesh, polyMesh, worldTrans, GetRef().node->GetMtl(), mNumSamples, &materialsMerge);

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
	}

	bool dynamicTopology = static_cast<bool>(GetCurrentJob()->GetOption("exportDynamicTopology"));
	
	// Extend the archive bounding box

	if (mJob){
		Alembic::Abc::M44d wm;
		ConvertMaxMatrixToAlembicMatrix(GetRef().node->GetObjTMAfterWSM(ticks), wm);

		Alembic::Abc::Box3d bbox = finalPolyMesh.bbox;

		//ESS_LOG_INFO( "Archive bbox: min("<<bbox.min.x<<", "<<bbox.min.y<<", "<<bbox.min.z<<") max("<<bbox.max.x<<", "<<bbox.max.y<<", "<<bbox.max.z<<")" );

		bbox.min = finalPolyMesh.bbox.min * wm;
		bbox.max = finalPolyMesh.bbox.max * wm;

		//ESS_LOG_INFO( "Archive bbox: min("<<bbox.min.x<<", "<<bbox.min.y<<", "<<bbox.min.z<<") max("<<bbox.max.x<<", "<<bbox.max.y<<", "<<bbox.max.z<<")" );

		mJob->GetArchiveBBox().extendBy(bbox);

		Alembic::Abc::Box3d box = mJob->GetArchiveBBox();
		//ESS_LOG_INFO( "Archive bbox: min("<<box.min.x<<", "<<box.min.y<<", "<<box.min.z<<") max("<<box.max.x<<", "<<box.max.y<<", "<<box.max.z<<")" );
	}

	if((mNumSamples == 0 || dynamicTopology) && finalPolyMesh.posVec.size() > 0){
		mMeshSample.setPositions(Alembic::Abc::P3fArraySample(finalPolyMesh.posVec));
	}
	else if(mNumSamples == 0 && dynamicTopology){
		mMeshSample.setPositions(Alembic::Abc::P3fArraySample(NULL, 0));
	}

	mMeshSample.setSelfBounds(finalPolyMesh.bbox);
	mMeshSample.setChildBounds(finalPolyMesh.bbox);
	
	bool purePointCache = static_cast<bool>(GetCurrentJob()->GetOption("exportPurePointCache"));
    // abort here if we are just storing points
    if(purePointCache)
    {
        if(mNumSamples == 0)
        {
            // store a dummy empty topology
			mMeshSample.setFaceCounts(Alembic::Abc::Int32ArraySample(NULL, 0));
			mMeshSample.setFaceIndices(Alembic::Abc::Int32ArraySample(NULL, 0));
        }

        mMeshSchema.set(mMeshSample);
        mNumSamples++;
        return true;
    }



	if((mNumSamples == 0 || dynamicTopology) && finalPolyMesh.normalVec.size() > 0){
		Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
		normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
		normalSample.setVals(Alembic::Abc::N3fArraySample(finalPolyMesh.normalVec));
		if(finalPolyMesh.normalIndexVec.size() > 0){
			normalSample.setIndices(Alembic::Abc::UInt32ArraySample(finalPolyMesh.normalIndexVec));
		}
		mMeshSample.setNormals(normalSample);
	}
	else if(mNumSamples == 0 && dynamicTopology){
		Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
		normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
		normalSample.setVals(Alembic::Abc::N3fArraySample(NULL, 0));
		normalSample.setIndices(Alembic::Abc::UInt32ArraySample(NULL, 0));
		mMeshSample.setNormals(normalSample);
	}

	if((mNumSamples == 0 || dynamicTopology) && finalPolyMesh.mFaceCountVec.size() > 0 && finalPolyMesh.mFaceIndicesVec.size() > 0){
		Alembic::Abc::Int32ArraySample faceCountSample(finalPolyMesh.mFaceCountVec);
		Alembic::Abc::Int32ArraySample faceIndicesSample(finalPolyMesh.mFaceIndicesVec);
		mMeshSample.setFaceCounts(faceCountSample);
		mMeshSample.setFaceIndices(faceIndicesSample);
	}
	else if(mNumSamples == 0 && dynamicTopology){
		mMeshSample.setFaceCounts(Alembic::Abc::Int32ArraySample(NULL, 0));
		mMeshSample.setFaceIndices(Alembic::Abc::Int32ArraySample(NULL, 0));
	}

	//write out the texture coordinates if necessary
	if((bool)GetCurrentJob()->GetOption("exportUVs") && (bFirstFrame || dynamicTopology))
	{

		if(mNumSamples == 0 && finalPolyMesh.mUvSetNames.size() > 0){
			Alembic::Abc::OStringArrayProperty uvSetNamesProperty = Alembic::Abc::OStringArrayProperty(
				mMeshSchema, ".uvSetNames", mMeshSchema.getMetaData(), mJob->GetAnimatedTs() );
			Alembic::Abc::StringArraySample uvSetNamesSample(&finalPolyMesh.mUvSetNames.front(), finalPolyMesh.mUvSetNames.size());
			uvSetNamesProperty.set(uvSetNamesSample);
		}

		for(int i=0; i<finalPolyMesh.mUvVec.size(); i++){
			std::vector<Alembic::Abc::V2f>& uvVec = finalPolyMesh.mUvVec[i];
			std::vector<Alembic::Abc::uint32_t>& uvIndexVec = finalPolyMesh.mUvIndexVec[i];

			int uvIndexSize = 0;
			Alembic::AbcGeom::OV2fGeomParam::Sample uvSample;
			if((mNumSamples == 0 || dynamicTopology) && uvVec.size() > 0){
				uvSample = Alembic::AbcGeom::OV2fGeomParam::Sample(Alembic::Abc::V2fArraySample(uvVec), Alembic::AbcGeom::kFacevaryingScope);
				if(uvIndexVec.size() > 0){
					uvIndexSize = (int)uvIndexVec.size();
					uvSample.setIndices(Alembic::Abc::UInt32ArraySample(uvIndexVec));
				}
			}
			else if(mNumSamples == 0 && dynamicTopology){
				uvSample = Alembic::AbcGeom::OV2fGeomParam::Sample(Alembic::Abc::V2fArraySample(NULL, 0), Alembic::AbcGeom::kFacevaryingScope);
				uvSample.setIndices(Alembic::Abc::UInt32ArraySample(NULL, 0));
			}
			else{
				continue;
			}

			if(i == 0){
				mMeshSample.setUVs(uvSample);
			}
			else{
				// create the uv param if required
				if(mNumSamples == 0)
				{
					std::stringstream storedUVSetNameStream;
					storedUVSetNameStream<<"uv"<<i;
					mUvParams.push_back(Alembic::AbcGeom::OV2fGeomParam( mMeshSchema, storedUVSetNameStream.str().c_str(), uvIndexSize > 0,
									 Alembic::AbcGeom::kFacevaryingScope, 1, mJob->GetAnimatedTs()));
				}
				mUvParams[i-1].set(uvSample);
			}
		}
	}


      // sweet, now let's have a look at face sets (really only for first sample)
      // for 3DS Max, we are mapping this to the material ids
	  if(GetCurrentJob()->GetOption("exportMaterialIds") && (bFirstFrame || dynamicTopology || bLastFrame))
      {

          if(!mMatIdProperty.valid())
          {
              mMatIdProperty = OUInt32ArrayProperty(mMeshSchema, ".materialids", mMeshSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs());
          }

          size_t nMatIndexSize = finalPolyMesh.mMatIdIndexVec.size();
		  if(nMatIndexSize != 0){
				Alembic::Abc::UInt32ArraySample sample = Alembic::Abc::UInt32ArraySample(&finalPolyMesh.mMatIdIndexVec.front(), nMatIndexSize);
				mMatIdProperty.set(sample);
		  }

		  SaveMaterialsProperty(bFirstFrame, bLastFrame || bForever);

		  size_t numMatId = finalPolyMesh.mFaceSetsMap.size();
          // For sample zero, export the material ids as face sets
		  if (bFirstFrame && numMatId > 1)
          {
			  for ( facesetmap_it it=finalPolyMesh.mFaceSetsMap.begin(); it != finalPolyMesh.mFaceSetsMap.end(); it++)
              {
				  std::stringstream nameStream;
				  int nMaterialId = it->first+1;
				  nameStream<<it->second.name<<" : "<<nMaterialId;

                  std::vector<int32_t> & faceSetVec = it->second.faceIds;

                  Alembic::AbcGeom::OFaceSet faceSet = mMeshSchema.createFaceSet(nameStream.str());
                  Alembic::AbcGeom::OFaceSetSchema::Sample faceSetSample(Alembic::Abc::Int32ArraySample(&faceSetVec.front(),faceSetVec.size()));
                  faceSet.getSchema().set(faceSetSample);
              }
          }

       }

   if(bFirstFrame || (dynamicTopology))
   {
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


   // check if we should export the velocities
   // TODO: support velocity property for nonparticle system meshes if possible
	if(dynamicTopology && bIsParticleSystem)
	{
		if(finalPolyMesh.mVelocitiesVec.size() > 0)
		{
			if(finalPolyMesh.posVec.size() != finalPolyMesh.mVelocitiesVec.size()){
				ESS_LOG_INFO("mVelocitiesVec has wrong size.");
			}
			mMeshSample.setVelocities(Alembic::Abc::V3fArraySample(finalPolyMesh.mVelocitiesVec));
		}
		else if(mNumSamples == 0 && dynamicTopology){
			mMeshSample.setVelocities(Alembic::Abc::V3fArraySample(NULL, 0));
		}
	}


   // save the sample
   mMeshSchema.set(mMeshSample);

   mNumSamples++;


	///////////////////////////////////////////////////////////////////////////////////////////////

    return true;
}
