#include "AlembicXform.h"
#include <xsi_application.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_x3dobject.h>
#include <xsi_math.h>
#include <xsi_context.h>
#include <xsi_operatorcontext.h>
#include <xsi_customoperator.h>
#include <xsi_factory.h>
#include <xsi_parameter.h>
#include <xsi_ppglayout.h>
#include <xsi_ppgitem.h>
#include <xsi_model.h>

using namespace XSI;
using namespace MATH;

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

void SaveXformSample(XSI::CRef kinestateRef, Alembic::AbcGeom::OXformSchema & schema, Alembic::AbcGeom::XformSample & sample, double time)
{
   KinematicState kineState(kinestateRef);

   // check if the transform is animated
   if(schema.getNumSamples() > 0)
   {
      X3DObject parent(kineState.GetParent3DObject());
      if(!isRefAnimated(kineState.GetParent3DObject().GetRef()))
         return;
   }

   CTransformation global = kineState.GetTransform(time);
   CTransformation model = kineState.GetParent3DObject().GetModel().GetKinematics().GetGlobal().GetTransform(time);
   global = MapWorldPoseToObjectSpace(model,global);

   // store the transform
   CVector3 trans = global.GetTranslation();
   CVector3 axis;
   double angle = global.GetRotationAxisAngle(axis);
   CVector3 scale = global.GetScaling();
   sample.setTranslation(Imath::V3d(trans.GetX(),trans.GetY(),trans.GetZ()));
   sample.setRotation(Imath::V3d(axis.GetX(),axis.GetY(),axis.GetZ()),RadiansToDegrees(angle));
   sample.setScale(Imath::V3d(scale.GetX(),scale.GetY(),scale.GetZ()));

   // save the sample
   schema.set(sample);
}

XSIPLUGINCALLBACK CStatus alembic_xform_Define( CRef& in_ctxt )
{
   return alembicOp_Define(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_xform_DefineLayout( CRef& in_ctxt )
{
   return alembicOp_DefineLayout(in_ctxt);
}


XSIPLUGINCALLBACK CStatus alembic_xform_Update( CRef& in_ctxt )
{
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
   Alembic::AbcGeom::IXform obj(iObj,Alembic::Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

   SampleInfo sampleInfo = getSampleInfo(
      ctxt.GetParameterValue(L"time"),
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );

   Alembic::AbcGeom::XformSample sample;
   obj.getSchema().get(sample,sampleInfo.floorIndex);
   Alembic::Abc::M44d matrix = sample.getMatrix();

   // blend
   if(sampleInfo.alpha != 0.0)
   {
      obj.getSchema().get(sample,sampleInfo.ceilIndex);
      Alembic::Abc::M44d ceilMatrix = sample.getMatrix();
      matrix = (1.0 - sampleInfo.alpha) * matrix + sampleInfo.alpha * ceilMatrix;
   }

   CMatrix4 xsiMatrix;
   xsiMatrix.Set(
      matrix.getValue()[0],matrix.getValue()[1],matrix.getValue()[2],matrix.getValue()[3],
      matrix.getValue()[4],matrix.getValue()[5],matrix.getValue()[6],matrix.getValue()[7],
      matrix.getValue()[8],matrix.getValue()[9],matrix.getValue()[10],matrix.getValue()[11],
      matrix.getValue()[12],matrix.getValue()[13],matrix.getValue()[14],matrix.getValue()[15]);
   CTransformation xsiTransform;
   xsiTransform.SetMatrix4(xsiMatrix);

   KinematicState state(ctxt.GetOutputTarget());
   state.PutTransform(xsiTransform);

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_xform_Term(CRef & in_ctxt)
{
   Context ctxt( in_ctxt );
   CustomOperator op(ctxt.GetSource());
   delRefArchive(op.GetParameterValue(L"path").GetAsText());
   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_visibility_Define( CRef& in_ctxt )
{
   return alembicOp_Define(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_visibility_DefineLayout( CRef& in_ctxt )
{
   return alembicOp_DefineLayout(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_visibility_Update( CRef& in_ctxt )
{
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

   Alembic::AbcGeom::IObject obj = getObjectFromArchive(path,identifier);
   if(!obj.valid())
      return CStatus::OK;

   Alembic::AbcGeom::IVisibilityProperty visibilityProperty = 
      Alembic::AbcGeom::GetVisibilityProperty(obj);
   if(!visibilityProperty.valid())
      return CStatus::OK;

   SampleInfo sampleInfo = getSampleInfo(
      ctxt.GetParameterValue(L"time"),
      getTimeSamplingFromObject(obj),
      visibilityProperty.getNumSamples()
   );

   int8_t rawVisibilityValue = visibilityProperty.getValue ( sampleInfo.floorIndex );
   Alembic::AbcGeom::ObjectVisibility visibilityValue = Alembic::AbcGeom::ObjectVisibility ( rawVisibilityValue );

   Property prop(ctxt.GetOutputTarget());
   switch(visibilityValue)
   {
      case Alembic::AbcGeom::kVisibilityVisible:
      {
         prop.PutParameterValue(L"viewvis",true);
         prop.PutParameterValue(L"rendvis",true);
         break;
      }
      case Alembic::AbcGeom::kVisibilityHidden:
      {
         prop.PutParameterValue(L"viewvis",false);
         prop.PutParameterValue(L"rendvis",false);
         break;
      }
      default:
      {
         break;
      }
   }

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_visibility_Term(CRef & in_ctxt)
{
   return alembicOp_Term(in_ctxt);
}
