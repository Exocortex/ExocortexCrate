#include "stdafx.h"

#include "AlembicXform.h"
#include "CommonProfiler.h"
#include "CommonUtilities.h"

using namespace XSI;
using namespace MATH;

// TODO: should only refer to our scene graph nodes, and rely on global
// transforms only
void SaveXformSample(SceneNodePtr node, AbcG::OXformSchema& schema,
                     AbcG::XformSample& sample, double time, bool xformCache,
                     bool globalSpace, bool flattenHierarchy)
{
  // check if we are exporting in global space
  if (globalSpace) {
    if (schema.getNumSamples() > 0) {
      return;
    }

    // store identity matrix
    sample.setTranslation(Imath::V3d(0.0, 0.0, 0.0));
    sample.setRotation(Imath::V3d(1.0, 0.0, 0.0), 0.0);
    sample.setScale(Imath::V3d(1.0, 1.0, 1.0));

    // save the sample
    schema.set(sample);
    return;
  }

  // check if the transform is animated
  // if(schema.getNumSamples() > 0)
  //{
  //   X3DObject parent(kineState.GetParent3DObject());
  //   if(!isRefAnimated(kineState.GetParent3DObject().GetRef(),xformCache))
  //      return;
  //}

  if (flattenHierarchy) {  // a hack needed to handle namespace xforms
    sample.setMatrix(node->getGlobalTransDouble(time));
  }
  else {
    Imath::M44d parentGlobalTransInv =
        node->parent->getGlobalTransDouble(time).invert();
    Imath::M44d transform =
        node->getGlobalTransDouble(time) * parentGlobalTransInv;
    sample.setMatrix(transform);
  }

  // KinematicState kineState(kineStateRef);
  // CTransformation globalTransform = kineState.GetTransform(time);
  // CTransformation transform;

  // if(flattenHierarchy){
  //   transform = globalTransform;
  //}
  // else{
  //   KinematicState parentKineState(parentKineStateRef);
  //   CMatrix4 parentGlobalTransform4 =
  //   parentKineState.GetTransform(time).GetMatrix4();
  //   CMatrix4 transform4 = globalTransform.GetMatrix4();
  //   parentGlobalTransform4.InvertInPlace();
  //   transform4.MulInPlace(parentGlobalTransform4);
  //   transform.SetMatrix4(transform4);
  //}

  //// store the transform
  // CVector3 trans = transform.GetTranslation();
  // CVector3 axis;
  // double angle = transform.GetRotationAxisAngle(axis);
  // CVector3 scale = transform.GetScaling();
  // sample.setTranslation(Imath::V3d(trans.GetX(),trans.GetY(),trans.GetZ()));
  // sample.setRotation(Imath::V3d(axis.GetX(),axis.GetY(),axis.GetZ()),RadiansToDegrees(angle));
  // sample.setScale(Imath::V3d(scale.GetX(),scale.GetY(),scale.GetZ()));

  // ESS_LOG_WARNING("time: "<<time<<" trans: ("<<trans.GetX()<<",
  // "<<trans.GetY()<<", "<<trans.GetZ()<<") angle: "<<angle<<" axis:
  // ("<<axis.GetX()<<", "<<axis.GetY()<<", "<<axis.GetZ());

  // save the sample
  schema.set(sample);
}

ESS_CALLBACK_START(alembic_xform_Define, CRef&)
return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_xform_DefineLayout, CRef&)
return alembicOp_DefineLayout(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_xform_Init, CRef&)
return alembicOp_Init(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_xform_Update, CRef&)
ESS_PROFILE_SCOPE("alembic_xform_Update");
OperatorContext ctxt(in_ctxt);

CString path = ctxt.GetParameterValue(L"path");

alembicOp_Multifile(in_ctxt, ctxt.GetParameterValue(L"multifile"),
                    ctxt.GetParameterValue(L"time"), path);
CStatus pathEditStat = alembicOp_PathEdit(in_ctxt, path);

if ((bool)ctxt.GetParameterValue(L"muted")) {
  return CStatus::OK;
}

CString identifier = ctxt.GetParameterValue(L"identifier");

AbcObjectCache* pObjectCache = getObjectCacheFromArchive(
    path.GetAsciiString(), identifier.GetAsciiString());

if (!pObjectCache) {
  // ESS_LOG_ERROR("Failure in alembic_xform operator connected to
  // "<<ctxt.GetOutputTarget().GetAsText().GetAsciiString()<<" because object
  // with path \""<<path.GetAsciiString()<<"\" and identifier
  // \""<<identifier.GetAsciiString()<<"\" could not be retrieved.");
  return CStatus::OK;
}

AbcG::IObject iObj = pObjectCache->obj;

IXformPtr pObj = pObjectCache->getXform();
if (!pObj) {
  return CStatus::OK;
}

AbcG::IXform& obj = *pObj;
if (!obj.valid()) {
  return CStatus::OK;
}

double time = ctxt.GetParameterValue(L"time");
Abc::M44d matrix;  // constructor creates an identity matrix

// if no time samples, default to identity matrix
if (obj.getSchema().getNumSamples() > 0) {
  SampleInfo sampleInfo = getSampleInfo(time, obj.getSchema().getTimeSampling(),
                                        obj.getSchema().getNumSamples());
  Abc::M44d floorMatrix =
      pObjectCache->getXformMatrix((int)sampleInfo.floorIndex);

  if (sampleInfo.alpha == 0.0f || sampleInfo.alpha == 1.0f) {
    matrix = floorMatrix;
  }
  else {
    Abc::M44d ceilMatrix =
        pObjectCache->getXformMatrix((int)sampleInfo.ceilIndex);
    matrix =
        (1.0 - sampleInfo.alpha) * floorMatrix + sampleInfo.alpha * ceilMatrix;
  }
}

CMatrix4 xsiMatrix;
xsiMatrix.Set(matrix.getValue()[0], matrix.getValue()[1], matrix.getValue()[2],
              matrix.getValue()[3], matrix.getValue()[4], matrix.getValue()[5],
              matrix.getValue()[6], matrix.getValue()[7], matrix.getValue()[8],
              matrix.getValue()[9], matrix.getValue()[10],
              matrix.getValue()[11], matrix.getValue()[12],
              matrix.getValue()[13], matrix.getValue()[14],
              matrix.getValue()[15]);
CTransformation xsiTransform;
xsiTransform.SetMatrix4(xsiMatrix);

KinematicState state(ctxt.GetOutputTarget());
state.PutTransform(xsiTransform);

return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_xform_Term, CRef&)
return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_visibility_Define, CRef&)
return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_visibility_DefineLayout, CRef&)
return alembicOp_DefineLayout(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_visibility_Init, CRef&)
return alembicOp_Init(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_visibility_Update, CRef&)
ESS_PROFILE_SCOPE("alembic_visibility_Update");
OperatorContext ctxt(in_ctxt);

CString path = ctxt.GetParameterValue(L"path");

alembicOp_Multifile(in_ctxt, ctxt.GetParameterValue(L"multifile"),
                    ctxt.GetParameterValue(L"time"), path);
CStatus pathEditStat = alembicOp_PathEdit(in_ctxt, path);

if ((bool)ctxt.GetParameterValue(L"muted")) {
  return CStatus::OK;
}

CString identifier = ctxt.GetParameterValue(L"identifier");

AbcG::IObject obj = getObjectFromArchive(path, identifier);
if (!obj.valid()) {
  return CStatus::OK;
}

AbcG::IVisibilityProperty visibilityProperty = getAbcVisibilityProperty(obj);
if (!visibilityProperty.valid()) {
  return CStatus::OK;
}

SampleInfo sampleInfo = getSampleInfo(ctxt.GetParameterValue(L"time"),
                                      getTimeSamplingFromObject(obj),
                                      visibilityProperty.getNumSamples());

AbcA::int8_t rawVisibilityValue =
    visibilityProperty.getValue(sampleInfo.floorIndex);
AbcG::ObjectVisibility visibilityValue =
    AbcG::ObjectVisibility(rawVisibilityValue);

Property prop(ctxt.GetOutputTarget());
switch (visibilityValue) {
  case AbcG::kVisibilityVisible: {
    prop.PutParameterValue(L"viewvis", true);
    prop.PutParameterValue(L"rendvis", true);
    break;
  }
  case AbcG::kVisibilityHidden: {
    prop.PutParameterValue(L"viewvis", false);
    prop.PutParameterValue(L"rendvis", false);
    break;
  }
  default: {
    break;
  }
}

return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START(alembic_visibility_Term, CRef&)
return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END
