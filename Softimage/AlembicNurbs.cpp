#include "AlembicNurbs.h"
#include "AlembicXform.h"
#include "stdafx.h"

using namespace XSI;
using namespace MATH;

AlembicNurbs::AlembicNurbs(SceneNodePtr eNode, AlembicWriteJob* in_Job,
                           Abc::OObject oParent)
    : AlembicObject(eNode, in_Job, oParent)
{
  AbcG::ONuPatch nurbs(GetMyParent(), eNode->name, GetJob()->GetAnimatedTs());

  mNurbsSchema = nurbs.getSchema();
}

AlembicNurbs::~AlembicNurbs() {}
Abc::OCompoundProperty AlembicNurbs::GetCompound() { return mNurbsSchema; }
XSI::CStatus AlembicNurbs::Save(double time)
{
  // store the transform
  Primitive prim(GetRef(REF_PRIMITIVE));
  bool globalSpace = GetJob()->GetOption(L"globalSpace");

  // query the global space
  CTransformation globalXfo;
  if (globalSpace) {
    globalXfo = KinematicState(GetRef(REF_GLOBAL_TRANS)).GetTransform(time);
  }

  // store the metadata
  SaveMetaData(GetRef(REF_NODE), this);

  // check if the nurbs is animated
  if (mNumSamples > 0) {
    if (!isRefAnimated(GetRef(REF_PRIMITIVE))) {
      return CStatus::OK;
    }
  }

  // define additional vectors, necessary for this task
  std::vector<Abc::V3f> posVec;
  std::vector<Abc::N3f> normalVec;
  std::vector<AbcA::uint32_t> normalIndexVec;

  // access the mesh
  NurbsSurfaceMesh nurbs = prim.GetGeometry(time);
  CVector3Array pos = nurbs.GetPoints().GetPositionArray();
  LONG vertCount = pos.GetCount();

  // prepare the bounding box
  Abc::Box3d bbox;

  // allocate the points and normals
  posVec.resize(vertCount);
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
  if (mNumSamples == 0) {
    std::vector<float> knots(1);
    knots[0] = 0.0f;

    mNurbsSample.setUKnot(Abc::FloatArraySample(&knots.front(), knots.size()));
    mNurbsSample.setVKnot(Abc::FloatArraySample(&knots.front(), knots.size()));
  }
  mNurbsSample.setPositions(posSample);
  mNurbsSample.setSelfBounds(bbox);

  // abort here if we are just storing points
  mNurbsSchema.set(mNurbsSample);
  mNumSamples++;
  return CStatus::OK;
}

ESS_CALLBACK_START(alembic_nurbs_Define, CRef&)
return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_nurbs_DefineLayout, CRef&)
return alembicOp_DefineLayout(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_nurbs_Init, CRef&)
return alembicOp_Init(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_nurbs_Update, CRef&)
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
AbcG::INuPatch objNurbs(iObj, Abc::kWrapExisting);
if (!objNurbs.valid()) {
  return CStatus::OK;
}

SampleInfo sampleInfo = getSampleInfo(ctxt.GetParameterValue(L"time"),
                                      objNurbs.getSchema().getTimeSampling(),
                                      objNurbs.getSchema().getNumSamples());

AbcG::INuPatchSchema::Sample sample;
objNurbs.getSchema().get(sample, sampleInfo.floorIndex);
Abc::P3fArraySamplePtr nurbsPos = sample.getPositions();

NurbsSurfaceMesh inNurbs = Primitive((CRef)ctxt.GetInputValue(0)).GetGeometry();
CVector3Array pos = inNurbs.GetPoints().GetPositionArray();

if (pos.GetCount() != nurbsPos->size()) {
  return CStatus::OK;
}

Operator op(ctxt.GetSource());
updateOperatorInfo(op, sampleInfo, objNurbs.getSchema().getTimeSampling(),
                   pos.GetCount(), (int)nurbsPos->size());

for (size_t i = 0; i < nurbsPos->size(); i++)
  pos[(LONG)i].Set(nurbsPos->get()[i].x, nurbsPos->get()[i].y,
                   nurbsPos->get()[i].z);

// blend
if (sampleInfo.alpha != 0.0) {
  objNurbs.getSchema().get(sample, sampleInfo.floorIndex);
  nurbsPos = sample.getPositions();
  for (size_t i = 0; i < nurbsPos->size(); i++)
    pos[(LONG)i].LinearlyInterpolate(
        pos[(LONG)i], CVector3(nurbsPos->get()[i].x, nurbsPos->get()[i].y,
                               nurbsPos->get()[i].z),
        sampleInfo.alpha);
}

Primitive(ctxt.GetOutputTarget())
    .GetGeometry()
    .GetPoints()
    .PutPositionArray(pos);

return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_nurbs_Term, CRef&)
return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END
