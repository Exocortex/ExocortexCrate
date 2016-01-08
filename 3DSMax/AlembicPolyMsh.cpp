#include "Alembic.h"
#include "AlembicIntermediatePolyMesh3DSMax.h"
#include "AlembicMetadataUtils.h"
#include "AlembicPointsUtils.h"
#include "AlembicPolyMsh.h"
#include "AlembicXForm.h"
#include "CommonMeshUtilities.h"
#include "CommonSubtreeMerge.h"
#include "SceneEnumProc.h"
#include "Utility.h"
#include "stdafx.h"

// From the SDK
// How to calculate UV's for face mapped materials.
// static Point3 basic_tva[3] = {
//	Point3(0.0,0.0,0.0),Point3(1.0,0.0,0.0),Point3(1.0,1.0,0.0)
//};
// static Point3 basic_tvb[3] = {
//	Point3(1.0,1.0,0.0),Point3(0.0,1.0,0.0),Point3(0.0,0.0,0.0)
//};
// static int nextpt[3] = {1,2,0};
// static int prevpt[3] = {2,0,1};

AlembicPolyMesh::AlembicPolyMesh(SceneNodePtr eNode, AlembicWriteJob* in_Job,
                                 Abc::OObject oParent)
    : AlembicObject(eNode, in_Job, oParent),
      customAttributes("Shape User Properties")  // TODO...
{
  AbcG::OPolyMesh mesh(GetOParent(), eNode->name,
                       GetCurrentJob()->GetAnimatedTs());

  mMeshSchema = mesh.getSchema();
}

AlembicPolyMesh::~AlembicPolyMesh() {}
Abc::OCompoundProperty AlembicPolyMesh::GetCompound() { return mMeshSchema; }
void AlembicPolyMesh::SaveMaterialsProperty(bool bFirstFrame, bool bLastFrame)
{
  if (bLastFrame) {
    std::vector<std::string> materialNames;
    mergedMeshMaterialsMap& matMap = materialsMerge.groupMatMap;

    for (mergedMeshMaterialsMap_it it = matMap.begin(); it != matMap.end();
         it++) {
      meshMaterialsMap& map = it->second;
      for (meshMaterialsMap_it it2 = map.begin(); it2 != map.end(); it2++) {
        std::stringstream nameStream;
        int nMaterialId = it2->second.matId + 1;
        nameStream << it2->second.name << "_" << nMaterialId;
        materialNames.push_back(nameStream.str());
      }
    }

    if (materialNames.size() > 0) {
      if (!mMatNamesProperty.valid()) {
        mMatNamesProperty = Abc::OStringArrayProperty(
            mMeshSchema, ".materialnames", mMeshSchema.getMetaData(),
            GetCurrentJob()->GetAnimatedTs());
      }
      Abc::StringArraySample sample = Abc::StringArraySample(materialNames);
      mMatNamesProperty.set(sample);
    }
  }
}

bool AlembicPolyMesh::Save(double time, bool bLastFrame)
{
  ESS_PROFILE_FUNC();

  // this call is here to avoid reading pointers that are only valid on a single
  // frame
  mMeshSample.reset();

  const bool bFirstFrame = mNumSamples == 0;

  TimeValue ticks = GetTimeValueFromFrame(time);

  const bool bIsParticleSystem = isParticleSystem(mExoSceneNode->type);

  // mMaxNode (could be null if this is a merged polyMesh)

  if (bIsParticleSystem) {
    bForever = false;
  }
  else if (mMaxNode) {
    Object* obj = mMaxNode->EvalWorldState(ticks).obj;
    if (mNumSamples == 0) {
      bForever = CheckIfObjIsValidForever(obj, ticks);
    }
  }

  if (mMaxNode) {
    SaveMetaData(mMaxNode, this);
  }

  // check if the mesh is animated (Otherwise, no need to export)
  if (mNumSamples > 0) {
    if (bForever) {
      ESS_LOG_INFO(
          "Node is not animated, not saving topology on subsequent frames.");
      return true;
    }
  }

  IntermediatePolyMesh3DSMax finalMesh;

  if (bIsParticleSystem) {  // Merged Particle System Export

    const bool bEnableVelocityExport = true;
    bool bSuccess = mMaxNode != NULL;
    if (bSuccess) {
      bSuccess =
          getParticleSystemMesh(ticks, mMaxNode, &finalMesh, &materialsMerge,
                                mJob, mNumSamples, bEnableVelocityExport);
    }
    if (!bSuccess) {
      ESS_LOG_INFO("Error. Could not get particle system mesh. Time: " << time);
      return false;
    }
    velocityCalc.calcVelocities(finalMesh.posVec, finalMesh.mFaceIndicesVec,
                                finalMesh.mVelocitiesVec,
                                GetSecondsFromTimeValue(ticks));
  }
  else {
    CommonOptions options;
    options.Copy(GetCurrentJob()->mOptions);

    // if(options.GetBoolOption("exportFaceSets") && mNumSamples != 0){//turn
    // off faceset exports for all frames except the first
    //   options.SetOption("exportFaceSets", false);
    //}

    if (mExoSceneNode->type ==
        SceneNode::POLYMESH_SUBTREE) {  // Merged PolyMesh Subtree Export
      SceneNodePolyMeshSubtreePtr meshSubtreeNode =
          reinterpret<SceneNode, SceneNodePolyMeshSubtree>(mExoSceneNode);
      mergePolyMeshSubtreeNode<IntermediatePolyMesh3DSMax>(
          meshSubtreeNode, finalMesh, options, time);
    }
    else {  // Normal PolyMesh node export

      // for now, custom attribute will ignore if meshes are being merged
      // customAttributes.exportCustomAttributes(mesh);

      // keep the orignal material IDs, since we are not saving out a single
      // nonmerged mesh
      materialsMerge.bPreserveIds = true;
      Imath::M44f transform44f;
      transform44f.makeIdentity();
      finalMesh.Save(mExoSceneNode, transform44f, options,
                     mNumSamples == 0 ? 0.0 : time);
    }
  }

  bool dynamicTopology =
      static_cast<bool>(GetCurrentJob()->GetOption("exportDynamicTopology"));

  // Extend the archive bounding box
  if (mJob && mMaxNode) {
    // TODO: need to make this work for mergedPolyMesh somehow
    Abc::M44d wm = mExoSceneNode->getGlobalTransDouble(time);

    Abc::Box3d bbox = finalMesh.bbox;

    // ESS_LOG_INFO( "Archive bbox: min("<<bbox.min.x<<", "<<bbox.min.y<<",
    // "<<bbox.min.z<<") max("<<bbox.max.x<<", "<<bbox.max.y<<",
    // "<<bbox.max.z<<")" );

    bbox.min = finalMesh.bbox.min * wm;
    bbox.max = finalMesh.bbox.max * wm;

    // ESS_LOG_INFO( "Archive bbox: min("<<bbox.min.x<<", "<<bbox.min.y<<",
    // "<<bbox.min.z<<") max("<<bbox.max.x<<", "<<bbox.max.y<<",
    // "<<bbox.max.z<<")" );

    mJob->GetArchiveBBox().extendBy(bbox);

    Abc::Box3d box = mJob->GetArchiveBBox();
    // ESS_LOG_INFO( "Archive bbox: min("<<box.min.x<<", "<<box.min.y<<",
    // "<<box.min.z<<") max("<<box.max.x<<", "<<box.max.y<<", "<<box.max.z<<")"
    // );
  }

  mMeshSample.setPositions(Abc::P3fArraySample(finalMesh.posVec));

  mMeshSample.setSelfBounds(finalMesh.bbox);
  mMeshSchema.getChildBoundsProperty().set(finalMesh.bbox);

  // abort here if we are just storing points
  if (mJob->GetOption("exportPurePointCache")) {
    if (mNumSamples == 0) {
      // store a dummy empty topology
      mMeshSample.setFaceCounts(Abc::Int32ArraySample(NULL, 0));
      mMeshSample.setFaceIndices(Abc::Int32ArraySample(NULL, 0));
    }

    mMeshSchema.set(mMeshSample);
    mNumSamples++;
    return true;
  }

  if (mJob->GetOption("validateMeshTopology")) {
    mJob->mMeshErrors +=
        validateAlembicMeshTopo(finalMesh.mFaceCountVec,
                                finalMesh.mFaceIndicesVec, mExoSceneNode->name);
  }

  Abc::Int32ArraySample faceCountSample(finalMesh.mFaceCountVec);
  Abc::Int32ArraySample faceIndicesSample(finalMesh.mFaceIndicesVec);
  mMeshSample.setFaceCounts(faceCountSample);
  mMeshSample.setFaceIndices(faceIndicesSample);

  if (mJob->GetOption("exportNormals")) {
    AbcG::ON3fGeomParam::Sample normalSample;
    normalSample.setScope(AbcG::kFacevaryingScope);
    normalSample.setVals(Abc::N3fArraySample(finalMesh.mIndexedNormals.values));
    normalSample.setIndices(
        Abc::UInt32ArraySample(finalMesh.mIndexedNormals.indices));
    mMeshSample.setNormals(normalSample);
  }

  AbcG::OV2fGeomParam::Sample uvSample;

  // write out the texture coordinates if necessary
  if (mJob->GetOption("exportUVs")) {
    if (correctInvalidUVs(finalMesh.mIndexedUVSet)) {
      ESS_LOG_WARNING("Capped out of range uvs on object "
                      << mExoSceneNode->name << ", frame = " << time);
    }
    saveIndexedUVs(mMeshSchema, mMeshSample, uvSample, mUvParams,
                   mJob->GetAnimatedTs(), mNumSamples, finalMesh.mIndexedUVSet);
  }

  // sweet, now let's have a look at face sets (really only for first sample)
  // for 3DS Max, we are mapping this to the material ids
  if (GetCurrentJob()->GetOption("exportMaterialIds")) {
    if (!mMatIdProperty.valid()) {
      mMatIdProperty = Abc::OUInt32ArrayProperty(
          mMeshSchema, ".materialids", mMeshSchema.getMetaData(),
          GetCurrentJob()->GetAnimatedTs());
    }

    Abc::UInt32ArraySample sample =
        Abc::UInt32ArraySample(finalMesh.mMatIdIndexVec);
    mMatIdProperty.set(sample);

    SaveMaterialsProperty(bFirstFrame, bLastFrame || bForever);

    size_t numMatId = finalMesh.mFaceSets.size();
    bool bExportAllFaceset =
        GetCurrentJob()->GetOption("partitioningFacesetsOnly") == false;
    // For sample zero, export the material ids as face sets
    if (bFirstFrame && (numMatId > 1 || bExportAllFaceset)) {
      for (facesetmap_it it = finalMesh.mFaceSets.begin();
           it != finalMesh.mFaceSets.end(); it++) {
        std::stringstream nameStream;
        int nMaterialId = it->first + 1;
        nameStream << it->second.name << "_" << nMaterialId;

        std::vector<Abc::int32_t>& faceSetVec = it->second.faceIds;

        AbcG::OFaceSet faceSet = mMeshSchema.createFaceSet(nameStream.str());
        AbcG::OFaceSetSchema::Sample faceSetSample(
            Abc::Int32ArraySample(&faceSetVec.front(), faceSetVec.size()));
        faceSet.getSchema().set(faceSetSample);
      }
    }
  }

  // check if we should export the velocities
  // TODO: support velocity property for nonparticle system meshes if possible
  // TODO: add export velocities option
  if (bIsParticleSystem) {
    if (finalMesh.posVec.size() != finalMesh.mVelocitiesVec.size()) {
      ESS_LOG_INFO("mVelocitiesVec has wrong size.");
    }
    mMeshSample.setVelocities(Abc::V3fArraySample(finalMesh.mVelocitiesVec));
  }

  // save the sample
  mMeshSchema.set(mMeshSample);

  mNumSamples++;

  ///////////////////////////////////////////////////////////////////////////////////////////////

  return true;
}
