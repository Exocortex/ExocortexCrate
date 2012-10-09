#include "AlembicCamera.h"
#include "AlembicXform.h"

#include <xsi_application.h>
#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_context.h>
#include <xsi_operatorcontext.h>
#include <xsi_customoperator.h>
#include <xsi_factory.h>
#include <xsi_parameter.h>
#include <xsi_ppglayout.h>
#include <xsi_ppgitem.h>
#include <xsi_math.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>

using namespace XSI;
using namespace MATH;

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

AlembicCamera::AlembicCamera(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   Primitive prim(GetRef());
   CString cameraName(prim.GetParent3DObject().GetName());
   CString xformName(cameraName+L"Xfo");
   Alembic::AbcGeom::OXform xform(GetOParent(),xformName.GetAsciiString(),GetJob()->GetAnimatedTs());
   Alembic::AbcGeom::OCamera camera(xform,cameraName.GetAsciiString(),GetJob()->GetAnimatedTs());
   AddRef(prim.GetParent3DObject().GetKinematics().GetGlobal().GetRef());

   // create the generic properties
   mOVisibility = CreateVisibilityProperty(camera,GetJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();
   mCameraSchema = camera.getSchema();
}

AlembicCamera::~AlembicCamera()
{
   // we have to clear this prior to destruction
   // this is a workaround for issue-171
   mOVisibility.reset();
}

Alembic::Abc::OCompoundProperty AlembicCamera::GetCompound()
{
   return mCameraSchema;
}

XSI::CStatus AlembicCamera::Save(double time)
{
   // access the camera
   Primitive prim(GetRef());

   // store the transform
   bool flattenHierarchy = GetJob()->GetOption(L"flattenHierarchy");
   SaveXformSample(GetRef(1),mXformSchema,mXformSample,time,false,false,flattenHierarchy);

   // set the visibility
   Property visProp;
   prim.GetParent3DObject().GetPropertyFromName(L"Visibility",visProp);
   if(isRefAnimated(visProp.GetRef()) || mNumSamples == 0)
   {
      bool visibility = visProp.GetParameterValue(L"rendvis",time);
      mOVisibility.set(visibility ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);
   }

   // store the metadata
   SaveMetaData(prim.GetParent3DObject().GetRef(),this);

   // store the camera data
   mCameraSample.setFocusDistance(prim.GetParameterValue(L"interestdist",time));
   mCameraSample.setLensSqueezeRatio(prim.GetParameterValue(L"aspect",time));
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


ESS_CALLBACK_START( alembic_camera_Update, CRef& )
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
   Alembic::AbcGeom::ICamera obj(iObj,Alembic::Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

   SampleInfo sampleInfo = getSampleInfo(
      ctxt.GetParameterValue(L"time"),
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );
   
   Operator op(ctxt.GetSource());
   updateOperatorInfo( op, sampleInfo, obj.getSchema().getTimeSampling(), 0, 0);

   Alembic::AbcGeom::CameraSample sample;
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
   prim.PutParameterValue(L"aspect",lensSqueezeRatio);
   prim.PutParameterValue(L"pixelratio",1.0);
   prim.PutParameterValue(L"projplane",true);
   prim.PutParameterValue(L"projplanelockaspect",false);
   prim.PutParameterValue(L"projplanedist",focalLength);
   prim.PutParameterValue(L"projplanewidth",xaperture);
   prim.PutParameterValue(L"projplaneheight",yaperture);
   prim.PutParameterValue(L"near",nearClipping);
   prim.PutParameterValue(L"far",farClipping);
   prim.PutParameterValue(L"interestdist",interestDist);

   return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_camera_Term, CRef& )
   return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END
