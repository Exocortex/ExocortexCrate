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
	std::vector<std::string> materialNames;

	if(!mMatNamesProperty.valid())
	{
		mMatNamesProperty = OStringArrayProperty(mMeshSchema, ".materialnames", mMeshSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs());
	}

	if(bLastFrame){
	  
		mergedMeshMaterialsMap& matMap = materialsMerge.groupMatMap;

		for ( mergedMeshMaterialsMap_it it=matMap.begin(); it != matMap.end(); it++)
		{
			meshMaterialsMap& map = it->second;
			for( meshMaterialsMap_it it2 =map.begin(); it2 != map.end(); it2++)
			{
				std::stringstream nameStream;
				int nMaterialId = it2->second.matId;
				nameStream<<it2->second.name<<" : "<<nMaterialId;
				materialNames.push_back(nameStream.str());
			}
		}

		if(materialNames.size() > 0){
			Alembic::Abc::StringArraySample sample = Alembic::Abc::StringArraySample(&materialNames.front(), materialNames.size());
			mMatNamesProperty.set(sample);
		}
		else{
			materialNames.push_back("");
			Alembic::Abc::StringArraySample sample = Alembic::Abc::StringArraySample(&materialNames.front(), 0);
			mMatNamesProperty.set(sample);
		}
	}
	else if(bFirstFrame){//alembic requires every property to have a sample at time 0
		materialNames.push_back("");
		Alembic::Abc::StringArraySample sample = Alembic::Abc::StringArraySample(&materialNames.front(), 0);
		mMatNamesProperty.set(sample);
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

		//std::vector<particleMeshData> meshes;

		//getParticleSystemRenderMeshes(ticks, obj, GetRef().node, meshes);

		//if(meshes.size() <= 0){
		//	return false;
		//}

		////save the first mesh to the final result, and then merge the others with it
		//materialsMerge.currUniqueHandle = meshes[0].animHandle;
		//finalPolyMesh.Save(mJob, ticks, meshes[0].pMesh, NULL, meshes[0].meshTM, meshes[0].pMtl, mNumSamples, &materialsMerge);

		//for(int i=1; i<meshes.size(); i++){
		//	IntermediatePolyMesh3DSMax currPolyMesh;
		//	materialsMerge.currUniqueHandle = meshes[i].animHandle;
		//	currPolyMesh.Save(mJob, ticks, meshes[i].pMesh, NULL, meshes[i].meshTM, meshes[i].pMtl, mNumSamples, &materialsMerge);
		//	bool bSuccess = finalPolyMesh.mergeWith(currPolyMesh);
		//	if(!bSuccess){
		//		return false;
		//	}
		//}

		//for(int i=0; i<meshes.size(); i++){
		//	
		//	if(meshes[i].bNeedDelete){
		//		delete meshes[i].pMesh;
		//	}
		//}
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

	
	// Extend the archive bounding box

	if (mJob){
		Alembic::Abc::M44d wm;
		ConvertMaxMatrixToAlembicMatrix(GetRef().node->GetObjTMAfterWSM(ticks), wm);

		Alembic::Abc::Box3d& b = finalPolyMesh.bbox;

		//ESS_LOG_INFO( "Archive bbox: min("<<b.min.x<<", "<<b.min.y<<", "<<b.min.z<<") max("<<b.max.x<<", "<<b.max.y<<", "<<b.max.z<<")" );

		finalPolyMesh.bbox.min = finalPolyMesh.bbox.min * wm;
		finalPolyMesh.bbox.max = finalPolyMesh.bbox.max * wm;

		//ESS_LOG_INFO( "Archive bbox: min("<<b.min.x<<", "<<b.min.y<<", "<<b.min.z<<") max("<<b.max.x<<", "<<b.max.y<<", "<<b.max.z<<")" );

		mJob->GetArchiveBBox().extendBy(finalPolyMesh.bbox);

		//Alembic::Abc::Box3d box = mJob->GetArchiveBBox();
		//ESS_LOG_INFO( "Archive bbox: min("<<box.min.x<<", "<<box.min.y<<", "<<box.min.z<<") max("<<box.max.x<<", "<<box.max.y<<", "<<box.max.z<<")" );
	}

	{
		// allocate the sample for the points
		if(finalPolyMesh.posVec.size() == 0)
		{
			finalPolyMesh.bbox.extendBy(Alembic::Abc::V3f(0,0,0));
			finalPolyMesh.posVec.push_back(Alembic::Abc::V3f(FLT_MAX,FLT_MAX,FLT_MAX));
		}

		Alembic::Abc::P3fArraySample posSample(&finalPolyMesh.posVec.front(), finalPolyMesh.posVec.size());

		// store the positions && bbox
		mMeshSample.setPositions(posSample);
		mMeshSample.setSelfBounds(finalPolyMesh.bbox);
		mMeshSample.setChildBounds(finalPolyMesh.bbox);
	}
	
	bool purePointCache = static_cast<bool>(GetCurrentJob()->GetOption("exportPurePointCache"));
    // abort here if we are just storing points
    if(purePointCache)
    {
        if(mNumSamples == 0)
        {
            // store a dummy empty topology
            finalPolyMesh.mFaceCountVec.push_back(0);
            finalPolyMesh.mFaceIndicesVec.push_back(0);
            Alembic::Abc::Int32ArraySample faceCountSample(&finalPolyMesh.mFaceCountVec.front(), finalPolyMesh.mFaceCountVec.size());
            Alembic::Abc::Int32ArraySample faceIndicesSample(&finalPolyMesh.mFaceIndicesVec.front(), finalPolyMesh.mFaceIndicesVec.size());
            mMeshSample.setFaceCounts(faceCountSample);
            mMeshSample.setFaceIndices(faceIndicesSample);
        }

        mMeshSchema.set(mMeshSample);
        mNumSamples++;
        return true;
    }



	// check if we support changing topology
	bool dynamicTopology = static_cast<bool>(GetCurrentJob()->GetOption("exportDynamicTopology"));
	
   if(bFirstFrame || (dynamicTopology))//write out the Normal data
   {
	  //write out the normal data
      Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
	  normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
      if(finalPolyMesh.normalVec.size() > 0)
      {
         normalSample.setVals(Alembic::Abc::N3fArraySample(&finalPolyMesh.normalVec.front(), finalPolyMesh.normalVec.size()));
		 if(finalPolyMesh.normalIndexVec.size() > 0){
            normalSample.setIndices(Alembic::Abc::UInt32ArraySample(&finalPolyMesh.normalIndexVec.front(), finalPolyMesh.normalIndexVec.size()));
		 }
      }
      else if (mNumSamples == 0 && dynamicTopology)
      {
         // If we are exporting dynamic topology, then we may have normals that show up later in our scene.  The problem is that Alembic wants
         // your parameter to be defined at sample zero if you plan to use it even later on, so we create a dummy normal parameter here if the case
         // requires it
         finalPolyMesh.normalVec.push_back(Imath::V3f(0,0,0));
         finalPolyMesh.normalIndexVec.push_back(0);
         normalSample.setVals(Alembic::Abc::N3fArraySample(&finalPolyMesh.normalVec.front(), 0));
         normalSample.setIndices(Alembic::Abc::UInt32ArraySample(&finalPolyMesh.normalIndexVec.front(), 0));
      }
	  mMeshSample.setNormals(normalSample);
   }
   else
   {
      Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
      if(finalPolyMesh.normalVec.size() > 0)
      {
         normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
         normalSample.setVals(Alembic::Abc::N3fArraySample(&finalPolyMesh.normalVec.front(), finalPolyMesh.normalVec.size()));
		 if(finalPolyMesh.normalIndexVec.size() > 0){
            normalSample.setIndices(Alembic::Abc::UInt32ArraySample(&finalPolyMesh.normalIndexVec.front(), finalPolyMesh.normalIndexVec.size()));
		 }
         mMeshSample.setNormals(normalSample);
      }
      mMeshSchema.set(mMeshSample);
   }

	//write out the face counts and face indices
   if(bFirstFrame || (dynamicTopology))
   {
      // we also need to store the face counts as well as face indices
      //if(mFaceIndicesVec.size() != sampleCount || sampleCount == 0) //TODO: is this condition important? I get the impression it is useless from the original code
      {
         if(finalPolyMesh.mFaceIndicesVec.size() == 0)
         {
            finalPolyMesh.mFaceCountVec.push_back(0);
            finalPolyMesh.mFaceIndicesVec.push_back(0);
         }
         Alembic::Abc::Int32ArraySample faceCountSample(&finalPolyMesh.mFaceCountVec.front(), finalPolyMesh.mFaceCountVec.size());
         Alembic::Abc::Int32ArraySample faceIndicesSample(&finalPolyMesh.mFaceIndicesVec.front(), finalPolyMesh.mFaceIndicesVec.size());

         mMeshSample.setFaceCounts(faceCountSample);
         mMeshSample.setFaceIndices(faceIndicesSample);
      }
   }

	//write out the texture coordinates if necessary
	if((bool)GetCurrentJob()->GetOption("exportUVs") && (bFirstFrame || dynamicTopology))
	{
		if (finalPolyMesh.mUvVec.size() > 0 && finalPolyMesh.mUvVec.size() == finalPolyMesh.mFaceIndicesVec.size() )
		{
			Alembic::AbcGeom::OV2fGeomParam::Sample uvSample(
				Alembic::Abc::V2fArraySample(&finalPolyMesh.mUvVec.front(),finalPolyMesh.mUvVec.size()),
				Alembic::AbcGeom::kFacevaryingScope);

			if(finalPolyMesh.mUvIndexVec.size() > 0){
			  uvSample.setIndices(Alembic::Abc::UInt32ArraySample(&finalPolyMesh.mUvIndexVec.front(),finalPolyMesh.mUvIndexVec.size()));
			}
			mMeshSample.setUVs(uvSample);
		}
		else if (mNumSamples == 0 && dynamicTopology)
		{
			// If we are exporting dynamic topology, then we may have uvs that show up later in our scene.  The problem is that Alembic wants
			// your parameter to be defined at sample zero if you plan to use it even later on, so we create a dummy uv parameter here if the case
			// requires it
			finalPolyMesh.mUvVec.push_back(Imath::V2f(0,0));
			finalPolyMesh.mUvIndexVec.push_back(0);
			Alembic::AbcGeom::OV2fGeomParam::Sample uvSample(Alembic::Abc::V2fArraySample(&finalPolyMesh.mUvVec.front(), 0), Alembic::AbcGeom::kFacevaryingScope);
			uvSample.setIndices(Alembic::Abc::UInt32ArraySample(&finalPolyMesh.mUvIndexVec.front(), 0) );
			mMeshSample.setUVs(uvSample);
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
		  if(nMatIndexSize == 0){
              finalPolyMesh.mMatIdIndexVec.push_back(0);
		  }
          Alembic::Abc::UInt32ArraySample sample = Alembic::Abc::UInt32ArraySample(&finalPolyMesh.mMatIdIndexVec.front(), nMatIndexSize);
          mMatIdProperty.set(sample);

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
   if(dynamicTopology)
   {

      if(finalPolyMesh.mVelocitiesVec.size() > 0)
      {

		  if(finalPolyMesh.posVec.size() != finalPolyMesh.mVelocitiesVec.size()){

			  ESS_LOG_INFO("mVelocitiesVec has wrong size.");
		  }

         Alembic::Abc::V3fArraySample sample = Alembic::Abc::V3fArraySample(&finalPolyMesh.mVelocitiesVec.front(),finalPolyMesh.mVelocitiesVec.size());
		 mMeshSample.setVelocities( sample );
      }
   }
   


   // save the sample
   mMeshSchema.set(mMeshSample);

   mNumSamples++;


	///////////////////////////////////////////////////////////////////////////////////////////////

    return true;
}
