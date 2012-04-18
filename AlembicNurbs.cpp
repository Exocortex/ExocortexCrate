#include "AlembicNurbs.h"
#include "AlembicXform.h"

#include <xsi_application.h>
#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_geometry.h>
#include <xsi_nurbssurfacemesh.h>
#include <xsi_nurbssurface.h>
#include <xsi_point.h>
#include <xsi_math.h>
#include <xsi_context.h>
#include <xsi_operatorcontext.h>
#include <xsi_customoperator.h>
#include <xsi_factory.h>
#include <xsi_parameter.h>
#include <xsi_ppglayout.h>
#include <xsi_ppgitem.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>

using namespace XSI;
using namespace MATH;

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

AlembicNurbs::AlembicNurbs(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   Primitive prim(GetRef());
   CString nurbsName(prim.GetParent3DObject().GetName());
   CString xformName(nurbsName+L"Xfo");
   Alembic::AbcGeom::OXform xform(GetOParent(),xformName.GetAsciiString(),GetJob()->GetAnimatedTs());
   Alembic::AbcGeom::ONuPatch nurbs(xform,nurbsName.GetAsciiString(),GetJob()->GetAnimatedTs());
   AddRef(prim.GetParent3DObject().GetKinematics().GetGlobal().GetRef());

   mXformSchema = xform.getSchema();
   mNurbsSchema = nurbs.getSchema();
}

AlembicNurbs::~AlembicNurbs()
{
}

Alembic::Abc::OCompoundProperty AlembicNurbs::GetCompound()
{
   return mNurbsSchema;
}

XSI::CStatus AlembicNurbs::Save(double time)
{
   // store the transform
   Primitive prim(GetRef());
   SaveXformSample(GetRef(1),mXformSchema,mXformSample,time);

   // store the metadata
   SaveMetaData(prim.GetParent3DObject().GetRef(),this);

   // check if the nurbs is animated
   if(mNumSamples > 0) {
      if(!isRefAnimated(GetRef()))
         return CStatus::OK;
   }

   // define additional vectors, necessary for this task
   std::vector<Alembic::Abc::V3f> posVec;
   std::vector<Alembic::Abc::N3f> normalVec;
   std::vector<uint32_t> normalIndexVec;

   // access the mesh
   NurbsSurfaceMesh nurbs = prim.GetGeometry(time);
   CVector3Array pos = nurbs.GetPoints().GetPositionArray();
   LONG vertCount = pos.GetCount();

   // prepare the bounding box
   Alembic::Abc::Box3d bbox;

   // allocate the points and normals
   posVec.resize(vertCount);
   for(LONG i=0;i<vertCount;i++)
   {
      posVec[i].x = (float)pos[i].GetX();
      posVec[i].y = (float)pos[i].GetY();
      posVec[i].z = (float)pos[i].GetZ();
      bbox.extendBy(posVec[i]);
   }

   // allocate the sample for the points
   if(posVec.size() == 0)
   {
      bbox.extendBy(Alembic::Abc::V3f(0,0,0));
      posVec.push_back(Alembic::Abc::V3f(FLT_MAX,FLT_MAX,FLT_MAX));
   }
   Alembic::Abc::P3fArraySample posSample(&posVec.front(),posVec.size());

   // store the positions && bbox
   if(mNumSamples == 0)
   {
      std::vector<float> knots(1);
      knots[0] = 0.0f;

      mNurbsSample.setUKnot(Alembic::Abc::FloatArraySample(&knots.front(),knots.size()));
      mNurbsSample.setVKnot(Alembic::Abc::FloatArraySample(&knots.front(),knots.size()));
   }
   mNurbsSample.setPositions(posSample);
   mNurbsSample.setSelfBounds(bbox);

   // abort here if we are just storing points
   mNurbsSchema.set(mNurbsSample);
   mNumSamples++;
   return CStatus::OK;
}

ESS_CALLBACK_START( alembic_nurbs_Define, CRef& )
   return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_nurbs_DefineLayout, CRef& )
   return alembicOp_DefineLayout(in_ctxt);
ESS_CALLBACK_END


ESS_CALLBACK_START( alembic_nurbs_Update, CRef& )
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
   Alembic::AbcGeom::INuPatch objNurbs(iObj,Alembic::Abc::kWrapExisting);
   if(!objNurbs.valid())
      return CStatus::OK;

   SampleInfo sampleInfo = getSampleInfo(
      ctxt.GetParameterValue(L"time"),
      objNurbs.getSchema().getTimeSampling(),
      objNurbs.getSchema().getNumSamples()
   );

   Alembic::AbcGeom::INuPatchSchema::Sample sample;
   objNurbs.getSchema().get(sample,sampleInfo.floorIndex);
   Alembic::Abc::P3fArraySamplePtr nurbsPos = sample.getPositions();

   NurbsSurfaceMesh inNurbs = Primitive((CRef)ctxt.GetInputValue(0)).GetGeometry();
   CVector3Array pos = inNurbs.GetPoints().GetPositionArray();

   if(pos.GetCount() != nurbsPos->size())
      return CStatus::OK;

   for(size_t i=0;i<nurbsPos->size();i++)
      pos[(LONG)i].Set(nurbsPos->get()[i].x,nurbsPos->get()[i].y,nurbsPos->get()[i].z);

   // blend
   if(sampleInfo.alpha != 0.0)
   {
      objNurbs.getSchema().get(sample,sampleInfo.floorIndex);
      nurbsPos = sample.getPositions();
      for(size_t i=0;i<nurbsPos->size();i++)
         pos[(LONG)i].LinearlyInterpolate(pos[(LONG)i],CVector3(nurbsPos->get()[i].x,nurbsPos->get()[i].y,nurbsPos->get()[i].z),sampleInfo.alpha);
   }

   Primitive(ctxt.GetOutputTarget()).GetGeometry().GetPoints().PutPositionArray(pos);

   return CStatus::OK;
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_nurbs_Term, CRef& )
   return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END
