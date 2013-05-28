#include "stdafx.h"
#include "AlembicCamera.h"
#include "AlembicXform.h"

using namespace XSI;
using namespace MATH;

AlembicCamera::AlembicCamera(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent)
: AlembicObject(eNode, in_Job, oParent)
{
   AbcG::OCamera camera(GetMyParent(), eNode->name, GetJob()->GetAnimatedTs());

   mCameraSchema = camera.getSchema();
}

AlembicCamera::~AlembicCamera()
{
}

Abc::OCompoundProperty AlembicCamera::GetCompound()
{
   return mCameraSchema;
}

XSI::CStatus AlembicCamera::Save(double time)
{
   // access the camera
   Primitive prim(GetRef(REF_PRIMITIVE));

   // store the metadata
   SaveMetaData(GetRef(REF_NODE),this);

   // store the camera data
   mCameraSample.setFocusDistance(prim.GetParameterValue(L"interestdist",time));

   //should set to 1.0 according the article "Maya to Softimage: Camera Interoperability"
   mCameraSample.setLensSqueezeRatio(1.0);

   //const float fAspectR = (float)prim.GetParameterValue(L"aspect",time);
   //const float fAspect = (float)prim.GetParameterValue(L"projplanewidth",time) / (float)prim.GetParameterValue(L"projplaneheight",time);

   mCameraSample.setFocalLength(prim.GetParameterValue(L"projplanedist",time));
   mCameraSample.setVerticalAperture(float(prim.GetParameterValue(L"projplaneheight",time)) * 2.54f);
   mCameraSample.setHorizontalAperture(float(prim.GetParameterValue(L"projplanewidth",time)) * 2.54f);
   mCameraSample.setNearClippingPlane(prim.GetParameterValue(L"near",time));
   mCameraSample.setFarClippingPlane(prim.GetParameterValue(L"far",time));

   // save the samples
   mCameraSchema.set(mCameraSample);
   mNumSamples++;

   return CStatus::OK;
}

ESS_CALLBACK_START( alembic_camera_Define, CRef& )
   return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_camera_DefineLayout, CRef& )
   return alembicOp_DefineLayout(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_camera_Init, CRef& )
   return alembicOp_Init( in_ctxt );
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_camera_Update, CRef& )
   OperatorContext ctxt( in_ctxt );

   CString path = ctxt.GetParameterValue(L"path");
   CStatus pathEditStat = alembicOp_PathEdit( in_ctxt, path );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString identifier = ctxt.GetParameterValue(L"identifier");

  AbcG::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
  AbcG::ICamera obj(iObj,Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

   SampleInfo sampleInfo = getSampleInfo(
      ctxt.GetParameterValue(L"time"),
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );
   
   Operator op(ctxt.GetSource());
   updateOperatorInfo( op, sampleInfo, obj.getSchema().getTimeSampling(), 0, 0);

  AbcG::CameraSample sample;
   obj.getSchema().get(sample,sampleInfo.floorIndex);

   // values
   double lensSqueezeRatio = sample.getLensSqueezeRatio();
   double focalLength = sample.getFocalLength();
   double xaperture = sample.getHorizontalAperture() / 2.54f;
   double yaperture = sample.getVerticalAperture() / 2.54f;
   double nearClipping = sample.getNearClippingPlane();
   double farClipping = sample.getFarClippingPlane();
   double interestDist = sample.getFocusDistance();

   // blend
   if(sampleInfo.alpha != 0.0)
   {
      obj.getSchema().get(sample,sampleInfo.ceilIndex);
      lensSqueezeRatio = (1.0 - sampleInfo.alpha) * lensSqueezeRatio + sampleInfo.alpha * sample.getLensSqueezeRatio();
      focalLength = (1.0 - sampleInfo.alpha) * focalLength + sampleInfo.alpha * sample.getFocalLength();
      xaperture = (1.0 - sampleInfo.alpha) * xaperture + sampleInfo.alpha * sample.getHorizontalAperture() / 2.54f;
      yaperture = (1.0 - sampleInfo.alpha) * yaperture + sampleInfo.alpha * sample.getVerticalAperture() / 2.54f;
      nearClipping = (1.0 - sampleInfo.alpha) * nearClipping + sampleInfo.alpha * sample.getNearClippingPlane();
      farClipping = (1.0 - sampleInfo.alpha) * farClipping + sampleInfo.alpha * sample.getFarClippingPlane();
      interestDist = (1.0 - sampleInfo.alpha) * interestDist + sampleInfo.alpha * sample.getFocusDistance();
   }

   // output
   Primitive prim(ctxt.GetOutputTarget());
   prim.PutParameterValue(L"std",(LONG)0l);
   //prim.PutParameterValue(L"aspect",lensSqueezeRatio);
   prim.PutParameterValue(L"pixelratio",1.0);
   prim.PutParameterValue(L"projplane",true);
   prim.PutParameterValue(L"projplanelockaspect",false);
   prim.PutParameterValue(L"projplanedist",focalLength);

   //The correct way to handle lens squeeze ratio according to the article "Maya to Softimage: Camera Interoperability"
   prim.PutParameterValue(L"projplanewidth",xaperture * lensSqueezeRatio);

   prim.PutParameterValue(L"projplaneheight",yaperture);
   prim.PutParameterValue(L"near",nearClipping);
   prim.PutParameterValue(L"far",farClipping);
   prim.PutParameterValue(L"interestdist",interestDist);

   return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_camera_Term, CRef& )
   return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END
