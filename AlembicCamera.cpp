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
   SaveXformSample(GetRef(1),mXformSchema,mXformSample,time);

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

   // check if the camera is animated
   if(mCameraSchema.getNumSamples() > 0)
   {
      if(!isRefAnimated(GetRef()))
         return CStatus::OK;
   }

   // store the camera data
   mCameraSample.setLensSqueezeRatio(prim.GetParameterValue(L"aspect",time));
   mCameraSample.setFocalLength(prim.GetParameterValue(L"projplanedist",time));
   mCameraSample.setVerticalAperture(float(prim.GetParameterValue(L"projplaneheight",time)) * 2.54f);
   mCameraSample.setHorizontalAperture(float(prim.GetParameterValue(L"projplanewidth",time)) * 2.54f);
   mCameraSample.setNearClippingPlane(prim.GetParameterValue(L"near",time));
   mCameraSample.setFarClippingPlane(prim.GetParameterValue(L"far",time));

   // save the samples
   mCameraSchema.set(mCameraSample);

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_camera_Define( CRef& in_ctxt )
{
   return alembicOp_Define(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_camera_DefineLayout( CRef& in_ctxt )
{
   return alembicOp_DefineLayout(in_ctxt);
}


XSIPLUGINCALLBACK CStatus alembic_camera_Update( CRef& in_ctxt )
{
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

   Alembic::AbcGeom::CameraSample sample;
   obj.getSchema().get(sample,sampleInfo.floorIndex);

   // values
   double lensSqueezeRatio = sample.getLensSqueezeRatio();
   double focalLength = sample.getFocalLength();
   double aperture = sample.getVerticalAperture() / 2.54f;
   double nearClipping = sample.getNearClippingPlane();
   double farClipping = sample.getFarClippingPlane();

   // blend
   if(sampleInfo.alpha != 0.0)
   {
      obj.getSchema().get(sample,sampleInfo.ceilIndex);
      lensSqueezeRatio = (1.0 - sampleInfo.alpha) * lensSqueezeRatio + sampleInfo.alpha * sample.getLensSqueezeRatio();
      focalLength = (1.0 - sampleInfo.alpha) * focalLength + sampleInfo.alpha * sample.getFocalLength();
      aperture = (1.0 - sampleInfo.alpha) * aperture + sampleInfo.alpha * sample.getVerticalAperture() / 2.54f;
      nearClipping = (1.0 - sampleInfo.alpha) * nearClipping + sampleInfo.alpha * sample.getNearClippingPlane();
      farClipping = (1.0 - sampleInfo.alpha) * farClipping + sampleInfo.alpha * sample.getFarClippingPlane();
   }

   // output
   Primitive prim(ctxt.GetOutputTarget());
   prim.PutParameterValue(L"std",(LONG)0l);
   prim.PutParameterValue(L"aspect",lensSqueezeRatio);
   prim.PutParameterValue(L"projplane",true);
   prim.PutParameterValue(L"projplanedist",focalLength);
   prim.PutParameterValue(L"projplaneheight",aperture);
   prim.PutParameterValue(L"near",nearClipping);
   prim.PutParameterValue(L"far",farClipping);

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_camera_Term(CRef & in_ctxt)
{
   return alembicOp_Term(in_ctxt);
}
