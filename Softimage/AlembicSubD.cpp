#include "AlembicSubD.h"
#include "AlembicPolyMsh.h"
#include "AlembicXform.h"
#include "stdafx.h"

using namespace XSI;
using namespace MATH;

AlembicSubD::AlembicSubD(SceneNodePtr eNode, AlembicWriteJob* in_Job,
                         Abc::OObject oParent)
    : AlembicObject(eNode, in_Job, oParent)
{
  AbcG::OSubD subD(GetMyParent(), eNode->name, GetJob()->GetAnimatedTs());

  mSubDSchema = subD.getSchema();

  Primitive prim(GetRef(REF_PRIMITIVE));
  Abc::OCompoundProperty argGeomParamsProp = mSubDSchema.getArbGeomParams();
  customAttributes.defineCustomAttributes(prim.GetGeometry(), argGeomParamsProp,
                                          mSubDSchema.getMetaData(),
                                          GetJob()->GetAnimatedTs());
}

AlembicSubD::~AlembicSubD() {}
Abc::OCompoundProperty AlembicSubD::GetCompound() { return mSubDSchema; }
XSI::CStatus AlembicSubD::Save(double time)
{
  // store the transform
  Primitive prim(GetRef(REF_PRIMITIVE));
  bool globalSpace = GetJob()->GetOption(L"globalSpace");

  // query the global space
  CTransformation globalXfo;
  if (globalSpace) {
    globalXfo = KinematicState(GetRef(REF_GLOBAL_TRANS)).GetTransform(time);
  }
  CTransformation globalRotation;
  globalRotation.SetRotation(globalXfo.GetRotation());

  // store the metadata
  SaveMetaData(GetRef(REF_NODE), this);

  // check if the mesh is animated
  if (mNumSamples > 0) {
    if (!isRefAnimated(GetRef(REF_PRIMITIVE), false, globalSpace)) {
      return CStatus::OK;
    }
  }

  // determine if we are a pure point cache
  bool purePointCache = (bool)GetJob()->GetOption(L"exportPurePointCache");

  // access the mesh
  PolygonMesh mesh = prim.GetGeometry(time);
  CVector3Array pos = mesh.GetVertices().GetPositionArray();
  LONG vertCount = pos.GetCount();

  // prepare the bounding box
  Abc::Box3d bbox;

  // allocate the points and normals
  std::vector<Abc::V3f> posVec(vertCount);
  for (LONG i = 0; i < vertCount; i++) {
    if (globalSpace) {
      pos[i] = MapObjectPositionToWorldSpace(globalXfo, pos[i]);
    }
    posVec[i].x = (float)pos[i].GetX();
    posVec[i].y = (float)pos[i].GetY();
    posVec[i].z = (float)pos[i].GetZ();
    bbox.extendBy(posVec[i]);
  }

  // allocate the sample for the points
  if (posVec.size() == 0) {
    bbox.extendBy(Abc::V3f(0, 0, 0));
    posVec.push_back(Abc::V3f(FLT_MAX, FLT_MAX, FLT_MAX));
  }
  Abc::P3fArraySample posSample(&posVec.front(), posVec.size());

  // store the positions && bbox
  mSubDSample.setPositions(posSample);
  mSubDSample.setSelfBounds(bbox);

  customAttributes.exportCustomAttributes(mesh);

  // abort here if we are just storing points
  if (purePointCache) {
    if (mNumSamples == 0) {
      // store a dummy empty topology
      mFaceCountVec.push_back(0);
      mFaceIndicesVec.push_back(0);
      Abc::Int32ArraySample faceCountSample(&mFaceCountVec.front(),
                                            mFaceCountVec.size());
      Abc::Int32ArraySample faceIndicesSample(&mFaceIndicesVec.front(),
                                              mFaceIndicesVec.size());
      mSubDSample.setFaceCounts(faceCountSample);
      mSubDSample.setFaceIndices(faceIndicesSample);
    }

    mSubDSchema.set(mSubDSample);
    mNumSamples++;
    return CStatus::OK;
  }

  // check if we support changing topology
  bool dynamicTopology = (bool)GetJob()->GetOption(L"exportDynamicTopology");

  CPolygonFaceRefArray faces = mesh.GetPolygons();
  LONG faceCount = faces.GetCount();
  LONG sampleCount = mesh.GetSamples().GetCount();

  // create a sample look table
  LONG offset = 0;
  CLongArray sampleLookup(sampleCount);
  for (LONG i = 0; i < faces.GetCount(); i++) {
    PolygonFace face(faces[i]);
    CLongArray samples = face.GetSamples().GetIndexArray();
    for (LONG j = samples.GetCount() - 1; j >= 0; j--) {
      sampleLookup[offset++] = samples[j];
    }
  }

  // check if we should export the velocities
  if (dynamicTopology) {
    ICEAttribute velocitiesAttr =
        mesh.GetICEAttributeFromName(L"PointVelocity");
    if (velocitiesAttr.IsDefined() && velocitiesAttr.IsValid()) {
      CICEAttributeDataArrayVector3f velocitiesData;
      velocitiesAttr.GetDataArray(velocitiesData);

      mVelocitiesVec.resize(vertCount);
      for (LONG i = 0; i < vertCount; i++) {
        CVector3 vel;
        vel.PutX(velocitiesData[i].GetX());
        vel.PutY(velocitiesData[i].GetY());
        vel.PutZ(velocitiesData[i].GetZ());
        if (globalSpace) {
          vel = MapObjectPositionToWorldSpace(globalRotation, vel);
        }
        mVelocitiesVec[i].x = (float)vel.GetX();
        mVelocitiesVec[i].y = (float)vel.GetY();
        mVelocitiesVec[i].z = (float)vel.GetZ();
      }

      if (mVelocitiesVec.size() == 0) {
        mVelocitiesVec.push_back(Abc::V3f(0, 0, 0));
      }
      Abc::V3fArraySample sample =
          Abc::V3fArraySample(&mVelocitiesVec.front(), mVelocitiesVec.size());
      mSubDSample.setVelocities(sample);
    }
  }

  // if we are the first frame!
  if (mNumSamples == 0 || dynamicTopology) {
    // we also need to store the face counts as well as face indices
    if (mFaceIndicesVec.size() != sampleCount || sampleCount == 0) {
      mFaceCountVec.resize(faceCount);
      mFaceIndicesVec.resize(sampleCount);

      offset = 0;
      for (LONG i = 0; i < faceCount; i++) {
        PolygonFace face(faces[i]);
        CLongArray indices = face.GetVertices().GetIndexArray();
        mFaceCountVec[i] = indices.GetCount();
        for (LONG j = indices.GetCount() - 1; j >= 0; j--) {
          mFaceIndicesVec[offset++] = indices[j];
        }
      }

      if (mFaceIndicesVec.size() == 0) {
        mFaceCountVec.push_back(0);
        mFaceIndicesVec.push_back(0);
      }
      Abc::Int32ArraySample faceCountSample(&mFaceCountVec.front(),
                                            mFaceCountVec.size());
      Abc::Int32ArraySample faceIndicesSample(&mFaceIndicesVec.front(),
                                              mFaceIndicesVec.size());

      mSubDSample.setFaceCounts(faceCountSample);
      mSubDSample.setFaceIndices(faceIndicesSample);
    }

    // set the subd level
    Property geomApproxProp;
    prim.GetParent3DObject().GetPropertyFromName(L"geomapprox", geomApproxProp);
    mSubDSample.setFaceVaryingInterpolateBoundary(
        geomApproxProp.GetParameterValue(L"gapproxmordrsl"));

    // also check if we need to store UV
    CRefArray clusters = mesh.GetClusters();
    if ((bool)GetJob()->GetOption(L"exportUVs")) {
      CGeometryAccessor accessor =
          mesh.GetGeometryAccessor(siConstructionModeSecondaryShape);
      CRefArray uvPropRefs = accessor.GetUVs();

      // if we now finally found a valid uvprop
      if (uvPropRefs.GetCount() > 0) {
        // ok, great, we found UVs, let's set them up
        if (mNumSamples == 0) {
          mUvVec.resize(uvPropRefs.GetCount());
          if ((bool)GetJob()->GetOption(L"indexedUVs")) {
            mUvIndexVec.resize(uvPropRefs.GetCount());
          }

          // query the names of all uv properties
          std::vector<std::string> uvSetNames;
          for (LONG i = 0; i < uvPropRefs.GetCount(); i++)
            uvSetNames.push_back(
                ClusterProperty(uvPropRefs[i]).GetName().GetAsciiString());

          Abc::OStringArrayProperty uvSetNamesProperty =
              Abc::OStringArrayProperty(mSubDSchema, ".uvSetNames",
                                        mSubDSchema.getMetaData(),
                                        GetJob()->GetAnimatedTs());
          Abc::StringArraySample uvSetNamesSample(&uvSetNames.front(),
                                                  uvSetNames.size());
          uvSetNamesProperty.set(uvSetNamesSample);
        }

        // loop over all uvsets
        for (LONG uvI = 0; uvI < uvPropRefs.GetCount(); uvI++) {
          mUvVec[uvI].resize(sampleCount);
          CDoubleArray uvValues =
              ClusterProperty(uvPropRefs[uvI]).GetElements().GetArray();

          for (LONG i = 0; i < sampleCount; i++) {
            mUvVec[uvI][i].x = (float)uvValues[sampleLookup[i] * 3 + 0];
            mUvVec[uvI][i].y = (float)uvValues[sampleLookup[i] * 3 + 1];
          }

          // now let's sort the normals
          size_t uvCount = mUvVec[uvI].size();
          size_t uvIndexCount = 0;
          if ((bool)GetJob()->GetOption(L"indexedUVs")) {
            std::map<SortableV2f, size_t> uvMap;
            std::map<SortableV2f, size_t>::const_iterator it;
            size_t sortedUVCount = 0;
            std::vector<Abc::V2f> sortedUVVec;
            mUvIndexVec[uvI].resize(mUvVec[uvI].size());
            sortedUVVec.resize(mUvVec[uvI].size());

            // loop over all uvs
            for (size_t i = 0; i < mUvVec[uvI].size(); i++) {
              it = uvMap.find(mUvVec[uvI][i]);
              if (it != uvMap.end()) {
                mUvIndexVec[uvI][uvIndexCount++] = (Abc::uint32_t)it->second;
              }
              else {
                mUvIndexVec[uvI][uvIndexCount++] = (Abc::uint32_t)sortedUVCount;
                uvMap.insert(std::pair<Abc::V2f, size_t>(
                    mUvVec[uvI][i], (Abc::uint32_t)sortedUVCount));
                sortedUVVec[sortedUVCount++] = mUvVec[uvI][i];
              }
            }

            // use indexed uvs if they use less space
            mUvVec[uvI] = sortedUVVec;
            uvCount = sortedUVCount;

            sortedUVCount = 0;
            sortedUVVec.clear();
          }

          AbcG::OV2fGeomParam::Sample uvSample(
              Abc::V2fArraySample(&mUvVec[uvI].front(), uvCount),
              AbcG::kFacevaryingScope);
          if (mUvIndexVec.size() > 0 && uvIndexCount > 0)
            uvSample.setIndices(Abc::UInt32ArraySample(
                &mUvIndexVec[uvI].front(), uvIndexCount));

          if (uvI == 0) {
            mSubDSample.setUVs(uvSample);
          }
          else {
            // create the uv param if required
            if (mNumSamples == 0) {
              CString storedUvSetName = CString(L"uv") + CString(uvI);
              mUvParams.push_back(AbcG::OV2fGeomParam(
                  mSubDSchema, storedUvSetName.GetAsciiString(),
                  uvIndexCount > 0, AbcG::kFacevaryingScope, 1,
                  GetJob()->GetAnimatedTs()));
            }
            mUvParams[uvI - 1].set(uvSample);
          }
        }

        // create the uv options
        if (mUvOptionsVec.size() == 0) {
          mUvOptionsProperty = Abc::OFloatArrayProperty(
              mSubDSchema, ".uvOptions", mSubDSchema.getMetaData(),
              GetJob()->GetAnimatedTs());

          for (LONG uvI = 0; uvI < uvPropRefs.GetCount(); uvI++) {
            // ESS_LOG_ERROR( "Cluster Property child name: " <<
            // ClusterProperty(uvPropRefs[uvI]).GetFullName().GetAsciiString()
            // );
            // ESS_LOG_ERROR( "Cluster Property child type: " <<
            // ClusterProperty(uvPropRefs[uvI]).GetType().GetAsciiString() );

            ClusterProperty clusterProperty = (ClusterProperty)uvPropRefs[uvI];
            bool subdsmooth = false;
            if (clusterProperty.GetType() == L"uvspace") {
              subdsmooth =
                  (bool)clusterProperty.GetParameter(L"subdsmooth").GetValue();
              // ESS_LOG_ERROR( "subdsmooth: " << subdsmooth );
            }

            CRefArray children = clusterProperty.GetNestedObjects();
            bool uWrap = false;
            bool vWrap = false;
            for (LONG i = 0; i < children.GetCount(); i++) {
              ProjectItem child(children.GetItem(i));
              CString type = child.GetType();
              // ESS_LOG_ERROR( "  Cluster Property child type: " <<
              // type.GetAsciiString() );
              if (type == L"uvprojdef") {
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
          mUvOptionsProperty.set(Abc::FloatArraySample(&mUvOptionsVec.front(),
                                                       mUvOptionsVec.size()));
        }
      }
    }

    // sweet, now let's have a look at face sets
    if (GetJob()->GetOption(L"exportFaceSets") && mNumSamples == 0) {
      for (LONG i = 0; i < clusters.GetCount(); i++) {
        Cluster cluster(clusters[i]);
        if (!cluster.GetType().IsEqualNoCase(L"poly")) {
          continue;
        }

        CLongArray elements = cluster.GetElements().GetArray();
        if (elements.GetCount() == 0) {
          continue;
        }

        std::string name(cluster.GetName().GetAsciiString());

        mFaceSetsVec.push_back(std::vector<Abc::int32_t>());
        std::vector<Abc::int32_t>& faceSetVec = mFaceSetsVec.back();
        for (LONG j = 0; j < elements.GetCount(); j++) {
          faceSetVec.push_back(elements[j]);
        }

        if (faceSetVec.size() > 0) {
          AbcG::OFaceSet faceSet = mSubDSchema.createFaceSet(name);
          AbcG::OFaceSetSchema::Sample faceSetSample(
              Abc::Int32ArraySample(&faceSetVec.front(), faceSetVec.size()));
          faceSet.getSchema().set(faceSetSample);
        }
      }
    }

    // save the sample
    mSubDSchema.set(mSubDSample);

    // check if we need to export the bindpose
    if (GetJob()->GetOption(L"exportBindPose") &&
        prim.GetParent3DObject().GetEnvelopes().GetCount() > 0 &&
        mNumSamples == 0) {
      mBindPoseProperty = Abc::OV3fArrayProperty(mSubDSchema, ".bindpose",
                                                 mSubDSchema.getMetaData(),
                                                 GetJob()->GetAnimatedTs());

      // store the positions of the modeling stack into here
      PolygonMesh bindPoseGeo =
          prim.GetGeometry(time, siConstructionModeModeling);
      CVector3Array bindPosePos = bindPoseGeo.GetPoints().GetPositionArray();
      mBindPoseVec.resize((size_t)bindPosePos.GetCount());
      for (LONG i = 0; i < bindPosePos.GetCount(); i++) {
        mBindPoseVec[i].x = (float)bindPosePos[i].GetX();
        mBindPoseVec[i].y = (float)bindPosePos[i].GetY();
        mBindPoseVec[i].z = (float)bindPosePos[i].GetZ();
      }

      Abc::V3fArraySample sample;
      if (mBindPoseVec.size() > 0)
        sample =
            Abc::V3fArraySample(&mBindPoseVec.front(), mBindPoseVec.size());
      mBindPoseProperty.set(sample);
    }
  }
  else {
    mSubDSchema.set(mSubDSample);
  }

  mNumSamples++;

  return CStatus::OK;
}

ESS_CALLBACK_START(alembic_geomapprox_Define, CRef&)
return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_geomapprox_DefineLayout, CRef&)
return alembicOp_DefineLayout(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_geomapprox_Init, CRef&)
return alembicOp_Init(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_geomapprox_Update, CRef&)
OperatorContext ctxt(in_ctxt);

CString path = ctxt.GetParameterValue(L"path");

alembicOp_Multifile(in_ctxt, ctxt.GetParameterValue(L"multifile"),
                    ctxt.GetParameterValue(L"time"), path);
CStatus pathEditStat = alembicOp_PathEdit(in_ctxt, path);

if ((bool)ctxt.GetParameterValue(L"muted")) {
  return CStatus::OK;
}

CString identifier = ctxt.GetParameterValue(L"identifier");

AbcG::IObject iObj = getObjectFromArchive(path, identifier);
if (!iObj.valid()) {
  return CStatus::OK;
}

Property prop(ctxt.GetOutputTarget());

if (AbcG::IPolyMesh::matches(iObj.getMetaData())) {
  AbcG::IPolyMesh objMesh(iObj, Abc::kWrapExisting);
  if (objMesh.valid() &&
      objMesh.getSchema().getPropertyHeader(
          ".faceVaryingInterpolateBoundary") != NULL) {
    Abc::IInt32Property faceVaryingInterpolateBoundary = Abc::IInt32Property(
        objMesh.getSchema(), ".faceVaryingInterpolateBoundary");
    if (faceVaryingInterpolateBoundary.getNumSamples() > 0) {
      Abc::int32_t subDLevel;
      faceVaryingInterpolateBoundary.get(subDLevel, 0);
      prop.PutParameterValue(L"gapproxmosl", (LONG)subDLevel);
      prop.PutParameterValue(L"gapproxmordrsl", (LONG)subDLevel);
    }
  }
}
else {
  AbcG::ISubD objSubD(iObj, Abc::kWrapExisting);
  if (objSubD.valid()) {
    AbcG::ISubDSchema::Sample sample;
    objSubD.getSchema().get(sample, 0);

    LONG subDLevel = sample.getFaceVaryingInterpolateBoundary();
    prop.PutParameterValue(L"gapproxmosl", subDLevel);
    prop.PutParameterValue(L"gapproxmordrsl", subDLevel);
  }
}

return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_geomapprox_Term, CRef&)
return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END
