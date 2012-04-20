#include "AlembicCurves.h"
#include "AlembicXform.h"

#include <xsi_application.h>
#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_geometry.h>
#include <xsi_sample.h>
#include <xsi_knot.h>
#include <xsi_controlpoint.h>
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
#include <xsi_clusterproperty.h>
#include <xsi_cluster.h>
#include <xsi_geometryaccessor.h>
#include <xsi_material.h>
#include <xsi_materiallibrary.h>
#include <xsi_nurbscurvelist.h>
#include <xsi_nurbscurve.h>
#include <xsi_hairprimitive.h>
#include <xsi_renderhairaccessor.h>
#include <xsi_icenode.h>
#include <xsi_icenodeinputport.h>
#include <xsi_icenodedef.h>
#include <xsi_dataarray.h>
#include <xsi_dataarray2D.h>
#include <xsi_inputport.h>
#include <xsi_iceattribute.h>
#include <xsi_iceattributedataarray.h>
#include <xsi_iceattributedataarray2D.h>
 
using namespace XSI;
using namespace MATH;

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
namespace AbcC = ::Alembic::AbcGeom::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;
using namespace AbcC;

AlembicCurves::AlembicCurves(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   Primitive prim(GetRef());
   CString curveName(prim.GetParent3DObject().GetName());
   CString xformName(curveName+L"Xfo");
   Alembic::AbcGeom::OXform xform(GetOParent(),xformName.GetAsciiString(),GetJob()->GetAnimatedTs());
   Alembic::AbcGeom::OCurves curves(xform,curveName.GetAsciiString(),GetJob()->GetAnimatedTs());
   AddRef(prim.GetParent3DObject().GetKinematics().GetGlobal().GetRef());

   // create the generic properties
   mOVisibility = CreateVisibilityProperty(curves,GetJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();
   mCurvesSchema = curves.getSchema();

   // create all properties
   mRadiusProperty = OFloatArrayProperty(mCurvesSchema, ".radius", mCurvesSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mColorProperty = OC4fArrayProperty(mCurvesSchema, ".color", mCurvesSchema.getMetaData(), GetJob()->GetAnimatedTs() );
}

AlembicCurves::~AlembicCurves()
{
   // we have to clear this prior to destruction
   // this is a workaround for issue-171
   mOVisibility.reset();
}

Alembic::Abc::OCompoundProperty AlembicCurves::GetCompound()
{
   return mCurvesSchema;
}

XSI::CStatus AlembicCurves::Save(double time)
{
   // store the transform
   Primitive prim(GetRef());
   bool globalSpace = GetJob()->GetOption(L"globalSpace");
   SaveXformSample(GetRef(1),mXformSchema,mXformSample,time,false,globalSpace);

   // query the global space
   CTransformation globalXfo;
   if(globalSpace)
      globalXfo = Kinematics(KinematicState(GetRef(1)).GetParent()).GetGlobal().GetTransform(time);
   CTransformation globalRotation;
   globalRotation.SetRotation(globalXfo.GetRotation());

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

   // check if the crvlist is animated
   if(mNumSamples > 0) {
      if(!isRefAnimated(GetRef(),false,globalSpace))
         return CStatus::OK;
   }

   // access the crvlist
   if(prim.GetType().IsEqualNoCase(L"crvlist"))
   {
      NurbsCurveList crvlist = prim.GetGeometry(time);
      CVector3Array pos = crvlist.GetPoints().GetPositionArray();
      ULONG vertCount = pos.GetCount();

      // prepare the bounding box
      Alembic::Abc::Box3d bbox;

      // allocate the points and normals
      std::vector<Alembic::Abc::V3f> posVec(vertCount);
      for(ULONG i=0;i<vertCount;i++)
      {
         if(globalSpace)
            pos[i] = MapObjectPositionToWorldSpace(globalXfo,pos[i]);
         posVec[i].x = (float)pos[i].GetX();
         posVec[i].y = (float)pos[i].GetY();
         posVec[i].z = (float)pos[i].GetZ();
         bbox.extendBy(posVec[i]);
      }

      // store the bbox
      mCurvesSample.setSelfBounds(bbox);

      // allocate for the points and normals
      Alembic::Abc::P3fArraySample posSample(&posVec.front(),posVec.size());

      // if we are the first frame!
      if(mNumSamples == 0)
      {
         // we also need to store the face counts as well as face indices
         CNurbsCurveRefArray curves = crvlist.GetCurves();
         mNbVertices.resize((size_t)curves.GetCount());
         for(LONG i=0;i<curves.GetCount();i++)
            mNbVertices[i] = (int32_t)NurbsCurve(curves[i]).GetControlPoints().GetCount();

         Alembic::Abc::Int32ArraySample nbVerticesSample(&mNbVertices.front(),mNbVertices.size());

         mCurvesSample.setPositions(posSample);
         mCurvesSample.setCurvesNumVertices(nbVerticesSample);

         // set the type + wrapping
         CNurbsCurveData curveData;
         NurbsCurve(curves[0]).Get(siSINurbs,curveData);
         mCurvesSample.setType(curveData.m_lDegree == 3 ? kCubic : kLinear);
         mCurvesSample.setWrap(curveData.m_bClosed ? kPeriodic : kNonPeriodic);
         mCurvesSample.setBasis(kNoBasis);

         // save the sample
         mCurvesSchema.set(mCurvesSample);
      }
      else
      {
         mCurvesSample.setPositions(posSample);
         mCurvesSchema.set(mCurvesSample);
      }
   }
   else if(prim.GetType().IsEqualNoCase(L"hair"))
   {
      HairPrimitive hairPrim(GetRef());
      LONG totalHairs = prim.GetParameterValue(L"TotalHairs");
      totalHairs *= (LONG)prim.GetParameterValue(L"StrandMult");
      CRenderHairAccessor accessor = hairPrim.GetRenderHairAccessor(totalHairs,10000,time);

      // prepare the bounding box
      Alembic::Abc::Box3d bbox;
      std::vector<Alembic::Abc::V3f> posVec;

      while(accessor.Next())
      {
         CFloatArray hairPos;
         CStatus result = accessor.GetVertexPositions(hairPos);
         ULONG vertCount = hairPos.GetCount();
         vertCount /= 3;
         size_t posVecOffset = posVec.size();
         posVec.resize(posVec.size() + size_t(vertCount));
         ULONG offset = 0;
         for(ULONG i=0;i<vertCount;i++)
         {
            CVector3 pos;
            pos.PutX(hairPos[offset++]);
            pos.PutY(hairPos[offset++]);
            pos.PutZ(hairPos[offset++]);
            if(globalSpace)
               pos = MapObjectPositionToWorldSpace(globalXfo,pos);
            posVec[posVecOffset+i].x = (float)pos.GetX();
            posVec[posVecOffset+i].y = (float)pos.GetY();
            posVec[posVecOffset+i].z = (float)pos.GetZ();
            bbox.extendBy(posVec[posVecOffset+i]);
         }
      }
      mCurvesSample.setPositions(Alembic::Abc::P3fArraySample(&posVec.front(),posVec.size()));

      // store the bbox
      mCurvesSample.setSelfBounds(bbox);

      // if we are the first frame!
      if(mNumSamples == 0)
      {
         CLongArray hairCount;
         accessor.GetVerticesCount(hairCount);
         mNbVertices.resize((size_t)hairCount.GetCount());
         for(LONG i=0;i<hairCount.GetCount();i++)
            mNbVertices[i] = (int32_t)hairCount[i];
         mCurvesSample.setCurvesNumVertices(Alembic::Abc::Int32ArraySample(&mNbVertices.front(),mNbVertices.size()));
         hairCount.Clear();

         // set the type + wrapping
         mCurvesSample.setType(kLinear);
         mCurvesSample.setWrap(kNonPeriodic);
         mCurvesSample.setBasis(kNoBasis);

         // store the hair radius
         CFloatArray hairRadius;
         accessor.GetVertexRadiusValues(hairRadius);
         mRadiusVec.resize((size_t)hairRadius.GetCount());
         for(LONG i=0;i<hairRadius.GetCount();i++)
            mRadiusVec[i] = hairRadius[i];
         mRadiusProperty.set(Alembic::Abc::FloatArraySample(&mRadiusVec.front(),mRadiusVec.size()));
         hairRadius.Clear();

         // store the hair color (if any)
         if(accessor.GetVertexColorCount() > 0)
         {
            CFloatArray hairColor;
            accessor.GetVertexColorValues(0,hairColor);
            mColorVec.resize((size_t)hairColor.GetCount()/4);
            ULONG offset = 0;
            for(size_t i=0;i<mColorVec.size();i++)
            {
               mColorVec[i].r = hairColor[offset++];
               mColorVec[i].g = hairColor[offset++];
               mColorVec[i].b = hairColor[offset++];
               mColorVec[i].a = hairColor[offset++];
            }
            mColorProperty.set(Alembic::Abc::C4fArraySample(&mColorVec.front(),mColorVec.size()));
            hairColor.Clear();
         }

         // store the hair color (if any)
         if(accessor.GetUVCount() > 0)
         {
            CFloatArray hairUV;
            accessor.GetUVValues(0,hairUV);
            mUvVec.resize((size_t)hairUV.GetCount()/3);
            ULONG offset = 0;
            for(size_t i=0;i<mUvVec.size();i++)
            {
               mUvVec[i].x = hairUV[offset++];
               mUvVec[i].y = 1.0f - hairUV[offset++];
               offset++;
            }
            mCurvesSample.setUVs(Alembic::AbcGeom::OV2fGeomParam::Sample(Alembic::Abc::V2fArraySample(&mUvVec.front(),mUvVec.size()),Alembic::AbcGeom::kVertexScope));
            hairUV.Clear();
         }
      }
      mCurvesSchema.set(mCurvesSample);
   }
   else if(prim.GetType().IsEqualNoCase(L"pointcloud"))
   {
      // prepare the bounding box
      Alembic::Abc::Box3d bbox;

      Geometry geo = prim.GetGeometry(time);
      ICEAttribute attr = geo.GetICEAttributeFromName(L"StrandPosition");
      size_t vertexCount = 0;
      std::vector<Alembic::Abc::V3f> posVec;
      if(attr.IsDefined() && attr.IsValid())
      {
         CICEAttributeDataArray2DVector3f data;
         attr.GetDataArray2D(data);

         CICEAttributeDataArrayVector3f sub;
         mNbVertices.resize(data.GetCount());
         for(ULONG i=0;i<data.GetCount();i++)
         {
            data.GetSubArray(i,sub);
            vertexCount += sub.GetCount();
            mNbVertices[i] = (int32_t)sub.GetCount();
         }
         mCurvesSample.setCurvesNumVertices(Alembic::Abc::Int32ArraySample(&mNbVertices.front(),mNbVertices.size()));

         // set wrap parameters
         mCurvesSample.setType(kLinear);
         mCurvesSample.setWrap(kNonPeriodic);
         mCurvesSample.setBasis(kNoBasis);

         posVec.resize(vertexCount);
         size_t offset = 0;
         for(ULONG i=0;i<data.GetCount();i++)
         {
            data.GetSubArray(i,sub);
            for(ULONG j=0;j<sub.GetCount();j++)
            {
               CVector3 pos;
               pos.PutX(sub[j].GetX());
               pos.PutY(sub[j].GetY());
               pos.PutZ(sub[j].GetZ());
               if(globalSpace)
                  pos = MapObjectPositionToWorldSpace(globalXfo,pos);
               posVec[offset].x = (float)pos.GetX();
               posVec[offset].y = (float)pos.GetY();
               posVec[offset].z = (float)pos.GetZ();
               bbox.extendBy(posVec[offset]);
               offset++;
            }
         }

         if(vertexCount > 0)
            mCurvesSample.setPositions(Alembic::Abc::P3fArraySample(&posVec.front(),posVec.size()));
         else
            mCurvesSample.setPositions(Alembic::Abc::P3fArraySample());
      }

      // store the bbox
      mCurvesSample.setSelfBounds(bbox);

      if(vertexCount > 0)
      {
         ICEAttribute attr = geo.GetICEAttributeFromName(L"StrandSize");
         if(attr.IsDefined() && attr.IsValid())
         {
            CICEAttributeDataArray2DFloat data;
            attr.GetDataArray2D(data);

            CICEAttributeDataArrayFloat sub;

            mRadiusVec.resize(vertexCount);
            size_t offset = 0;
            for(ULONG i=0;i<data.GetCount();i++)
            {
               data.GetSubArray(data.IsConstant() ? 0 : i,sub);
               for(ULONG j=0;j<sub.GetCount();j++)
                  mRadiusVec[offset++] = (float)sub[j];
            }
            mRadiusProperty.set(Alembic::Abc::FloatArraySample(&mRadiusVec.front(),mRadiusVec.size()));
         }
         else
         {
            attr = geo.GetICEAttributeFromName(L"Size");
            if(attr.IsDefined() && attr.IsValid())
            {
               CICEAttributeDataArrayFloat data;
               attr.GetDataArray(data);

               mRadiusVec.resize(data.GetCount());
               for(ULONG i=0;i<data.GetCount();i++)
                  mRadiusVec[i] = (float)data[data.IsConstant() ? 0 : i];
               mRadiusProperty.set(Alembic::Abc::FloatArraySample(&mRadiusVec.front(),mRadiusVec.size()));
            }
         }

         attr = geo.GetICEAttributeFromName(L"StrandColor");
         if(attr.IsDefined() && attr.IsValid())
         {
            CICEAttributeDataArray2DColor4f data;
            attr.GetDataArray2D(data);

            CICEAttributeDataArrayColor4f sub;

            mColorVec.resize(vertexCount);
            size_t offset = 0;
            for(ULONG i=0;i<data.GetCount();i++)
            {
               data.GetSubArray(data.IsConstant() ? 0 : i,sub);
               for(ULONG j=0;j<sub.GetCount();j++)
               {
                  mColorVec[offset].r = (float)sub[j].GetR();
                  mColorVec[offset].g = (float)sub[j].GetG();
                  mColorVec[offset].b = (float)sub[j].GetB();
                  mColorVec[offset].a = (float)sub[j].GetA();
                  offset++;
               }
            }
            mColorProperty.set(Alembic::Abc::C4fArraySample(&mColorVec.front(),mColorVec.size()));
         }
         else
         {
            attr = geo.GetICEAttributeFromName(L"Color");
            if(attr.IsDefined() && attr.IsValid())
            {
               CICEAttributeDataArrayColor4f data;
               attr.GetDataArray(data);

               mColorVec.resize(data.GetCount());
               for(ULONG i=0;i<data.GetCount();i++)
               {
                  mColorVec[i].r = (float)data[i].GetR();
                  mColorVec[i].g = (float)data[i].GetG();
                  mColorVec[i].b = (float)data[i].GetB();
                  mColorVec[i].a = (float)data[i].GetA();
               }
               mColorProperty.set(Alembic::Abc::C4fArraySample(&mColorVec.front(),mColorVec.size()));
            }
         }

         attr = geo.GetICEAttributeFromName(L"StrandUVs");
         if(attr.IsDefined() && attr.IsValid())
         {
            CICEAttributeDataArray2DVector2f data;
            attr.GetDataArray2D(data);

            CICEAttributeDataArrayVector2f sub;

            mUvVec.resize(vertexCount);
            size_t offset = 0;
            for(ULONG i=0;i<data.GetCount();i++)
            {
               data.GetSubArray(data.IsConstant() ? 0 : i,sub);
               for(ULONG j=0;j<sub.GetCount();j++)
               {
                  mUvVec[offset].x = (float)sub[j].GetX();
                  mUvVec[offset].y = (float)sub[j].GetY();
                  offset++;
               }
            }
            mCurvesSample.setUVs(Alembic::AbcGeom::OV2fGeomParam::Sample(Alembic::Abc::V2fArraySample(&mUvVec.front(),mUvVec.size()),Alembic::AbcGeom::kVertexScope));
         }
         else
         {
            attr = geo.GetICEAttributeFromName(L"UVs");
            if(attr.IsDefined() && attr.IsValid())
            {
               CICEAttributeDataArrayVector2f data;
               attr.GetDataArray(data);

               mUvVec.resize(data.GetCount());
               for(ULONG i=0;i<data.GetCount();i++)
               {
                  mUvVec[i].x = (float)data[i].GetX();
                  mUvVec[i].y = (float)data[i].GetY();
               }
               mCurvesSample.setUVs(Alembic::AbcGeom::OV2fGeomParam::Sample(Alembic::Abc::V2fArraySample(&mUvVec.front(),mUvVec.size()),Alembic::AbcGeom::kVertexScope));
            }
         }

         attr = geo.GetICEAttributeFromName(L"StrandVelocity");
         if(attr.IsDefined() && attr.IsValid())
         {
            float fps = (float)CTime().GetFrameRate();
            CICEAttributeDataArray2DVector3f data;
            attr.GetDataArray2D(data);

            CICEAttributeDataArrayVector3f sub;

            mVelVec.resize(vertexCount);
            size_t offset = 0;
            for(ULONG i=0;i<data.GetCount();i++)
            {
               data.GetSubArray(data.IsConstant() ? 0 : i,sub);
               for(ULONG j=0;j<sub.GetCount();j++)
               {
                  CVector3 vel;
                  vel.PutX(sub[j].GetX() / fps);
                  vel.PutY(sub[j].GetY() / fps);
                  vel.PutZ(sub[j].GetZ() / fps);
                  if(globalSpace)
                     vel = MapObjectPositionToWorldSpace(globalRotation,vel);
                  mVelVec[offset].x = (float)vel.GetX();
                  mVelVec[offset].y = (float)vel.GetY();
                  mVelVec[offset].z = (float)vel.GetZ();
                  offset++;
               }
            }
            mCurvesSample.setVelocities(Alembic::Abc::V3fArraySample(&mVelVec.front(),mVelVec.size()));
         }
         else
         {
            attr = geo.GetICEAttributeFromName(L"PointVelocity");
            if(attr.IsDefined() && attr.IsValid())
            {
               float fps = (float)CTime().GetFrameRate();
               CICEAttributeDataArrayVector3f data;
               attr.GetDataArray(data);

               mVelVec.resize(data.GetCount());
               for(ULONG i=0;i<data.GetCount();i++)
               {
                  CVector3 vel;
                  vel.PutX(data[i].GetX() / fps);
                  vel.PutY(data[i].GetY() / fps);
                  vel.PutZ(data[i].GetZ() / fps);
                  if(globalSpace)
                     vel = MapObjectPositionToWorldSpace(globalRotation,vel);
                  mVelVec[i].x = (float)vel.GetX();
                  mVelVec[i].y = (float)vel.GetY();
                  mVelVec[i].z = (float)vel.GetZ();
               }
               mCurvesSample.setVelocities(Alembic::Abc::V3fArraySample(&mVelVec.front(),mVelVec.size()));
            }
         }
      }
      if(vertexCount > 0)
      {
         mCurvesSchema.set(mCurvesSample);
      }
   }
   mNumSamples++;

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_crvlist_Define( CRef& in_ctxt )
{
   return alembicOp_Define(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_crvlist_DefineLayout( CRef& in_ctxt )
{
   return alembicOp_DefineLayout(in_ctxt);
}


XSIPLUGINCALLBACK CStatus alembic_crvlist_Update( CRef& in_ctxt )
{
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
   Alembic::AbcGeom::ICurves obj(iObj,Alembic::Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

   SampleInfo sampleInfo = getSampleInfo(
      ctxt.GetParameterValue(L"time"),
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );

   Alembic::AbcGeom::ICurvesSchema::Sample sample;
   obj.getSchema().get(sample,sampleInfo.floorIndex);

   NurbsCurveList curves = Primitive((CRef)ctxt.GetInputValue(0)).GetGeometry();
   CVector3Array pos = curves.GetPoints().GetPositionArray();

   Alembic::Abc::P3fArraySamplePtr curvePos = sample.getPositions();

   if(pos.GetCount() != curvePos->size())
      return CStatus::OK;

   for(size_t i=0;i<curvePos->size();i++)
      pos[(LONG)i].Set(curvePos->get()[i].x,curvePos->get()[i].y,curvePos->get()[i].z);

   // blend
   if(sampleInfo.alpha != 0.0)
   {
      obj.getSchema().get(sample,sampleInfo.ceilIndex);
      curvePos = sample.getPositions();
      for(size_t i=0;i<curvePos->size();i++)
         pos[(LONG)i].LinearlyInterpolate(pos[(LONG)i],CVector3(curvePos->get()[i].x,curvePos->get()[i].y,curvePos->get()[i].z),sampleInfo.alpha);
   }

   Primitive(ctxt.GetOutputTarget()).GetGeometry().GetPoints().PutPositionArray(pos);

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_crvlist_Term(CRef & in_ctxt)
{
   return alembicOp_Term(in_ctxt);
}

enum IDs
{
	ID_IN_path = 0,
	ID_IN_identifier = 1,
	ID_IN_renderpath = 2,
	ID_IN_renderidentifier = 3,
	ID_IN_time = 4,
	ID_G_100 = 100,
	ID_OUT_count = 201,
	ID_OUT_vertices = 202,
	ID_OUT_offset = 203,
	ID_OUT_position = 204,
	ID_OUT_velocity = 205,
	ID_OUT_radius = 206,
	ID_OUT_color = 207,
	ID_OUT_uv = 208,
	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
};

#define ID_UNDEF ((ULONG)-1)

using namespace XSI;
using namespace MATH;

CStatus Register_alembic_curves( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	nodeDef = Application().GetFactory().CreateICENodeDef(L"alembic_curves",L"alembic_curves");

	CStatus st;
	st = nodeDef.PutColor(255,188,102);
	st.AssertSucceeded( ) ;

   st = nodeDef.PutThreadingModel(XSI::siICENodeSingleThreading);
	st.AssertSucceeded( ) ;

	// Add input ports and groups.
	st = nodeDef.AddPortGroup(ID_G_100);
	st.AssertSucceeded( ) ;

   st = nodeDef.AddInputPort(ID_IN_path,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"path",L"path",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddInputPort(ID_IN_identifier,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"identifier",L"identifier",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddInputPort(ID_IN_renderpath,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"renderpath",L"renderpath",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddInputPort(ID_IN_renderidentifier,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"renderidentifier",L"renderidentifier",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddInputPort(ID_IN_time,ID_G_100,siICENodeDataFloat,siICENodeStructureSingle,siICENodeContextSingleton,L"time",L"time",0.0f,ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
	
   // Add output ports.
   st = nodeDef.AddOutputPort(ID_OUT_count,siICENodeDataLong,siICENodeStructureSingle,siICENodeContextSingleton,L"count",L"count",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_vertices,siICENodeDataLong,siICENodeStructureArray,siICENodeContextSingleton,L"vertices",L"vertices",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_offset,siICENodeDataLong,siICENodeStructureArray,siICENodeContextSingleton,L"offset",L"offset",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_position,siICENodeDataVector3,siICENodeStructureArray,siICENodeContextSingleton,L"position",L"position",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_velocity,siICENodeDataVector3,siICENodeStructureArray,siICENodeContextSingleton,L"velocity",L"velocity",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_radius,siICENodeDataFloat,siICENodeStructureArray,siICENodeContextSingleton,L"radius",L"radius",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_color,siICENodeDataColor4,siICENodeStructureArray,siICENodeContextSingleton,L"color",L"color",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_uv,siICENodeDataVector2,siICENodeStructureArray,siICENodeContextSingleton,L"uv",L"uv",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(L"Custom ICENode");

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_curves_Evaluate(ICENodeContext& in_ctxt)
{
	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	CDataArrayString pathData( in_ctxt, ID_IN_path );
   CString path = pathData[0];
	CDataArrayString identifierData( in_ctxt, ID_IN_identifier );
   CString identifier = identifierData[0];

   // check if we need t addref the archive
   CValue udVal = in_ctxt.GetUserData();
   ArchiveInfo * p = (ArchiveInfo*)(CValue::siPtrType)udVal;
   if(p == NULL)
   {
      p = new ArchiveInfo;
      p->path = path.GetAsciiString();
      addRefArchive(path);
      CValue val = (CValue::siPtrType) p;
      in_ctxt.PutUserData( val ) ;
   }

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
   Alembic::AbcGeom::ICurves obj(iObj,Alembic::Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

	CDataArrayFloat timeData( in_ctxt, ID_IN_time);
   double time = timeData[0];

   SampleInfo sampleInfo = getSampleInfo(
      time,
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );
   Alembic::AbcGeom::ICurvesSchema::Sample sample;
   obj.getSchema().get(sample,sampleInfo.floorIndex);

	switch( out_portID )
	{
      case ID_OUT_count:
		{
			// Get the output port array ...
         CDataArrayLong outData( in_ctxt );
         outData[0] = (LONG)sample.getNumCurves();
   		break;
		}
      case ID_OUT_vertices:
		{
			// Get the output port array ...
         CDataArray2DLong outData( in_ctxt );
         CDataArray2DLong::Accessor acc;

         Alembic::Abc::Int32ArraySamplePtr ptr = sample.getCurvesNumVertices();
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               acc[i] = (LONG)ptr->get()[i];
            }
         }
   		break;
		}
      case ID_OUT_offset:
		{
			// Get the output port array ...
         CDataArray2DLong outData( in_ctxt );
         CDataArray2DLong::Accessor acc;

         Alembic::Abc::Int32ArraySamplePtr ptr = sample.getCurvesNumVertices();
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            LONG offset = 0;
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               acc[i] = offset;
               offset += (LONG)ptr->get()[i];
            }
         }
   		break;
		}
      case ID_OUT_position:
		{
			// Get the output port array ...
         CDataArray2DVector3f outData( in_ctxt );
         CDataArray2DVector3f::Accessor acc;

         Alembic::Abc::P3fArraySamplePtr ptr = sample.getPositions();
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 1 && ptr->get()[0].x == FLT_MAX)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            bool done = false;

            if(sampleInfo.alpha != 0.0)
            {
               Alembic::Abc::P3fArraySamplePtr ptr2 = obj.getSchema().getValue(sampleInfo.ceilIndex).getPositions();
               float alpha = (float)sampleInfo.alpha;
               float ialpha = 1.0f - alpha;
               if(ptr2->size() == ptr->size())
               {
                  for(ULONG i=0;i<acc.GetCount();i++)
                  {
                     acc[i].Set(
                        ialpha * ptr->get()[i].x + alpha * ptr2->get()[i].x,
                        ialpha * ptr->get()[i].y + alpha * ptr2->get()[i].y,
                        ialpha * ptr->get()[i].z + alpha * ptr2->get()[i].z);
                  }
                  done = true;
               }
               else
               {
                  Alembic::Abc::V3fArraySamplePtr vel = sample.getVelocities();
                  if(vel->size() == ptr->size())
                  {
                     for(ULONG i=0;i<acc.GetCount();i++)
                     {
                        acc[i].Set(
                           ptr->get()[i].x + alpha * vel->get()[i].x,
                           ptr->get()[i].y + alpha * vel->get()[i].y,
                           ptr->get()[i].z + alpha * vel->get()[i].z);
                     }
                     done = true;
                  }
               }
            }

            if(!done)
            {
               for(ULONG i=0;i<acc.GetCount();i++)
                  acc[i].Set(ptr->get()[i].x,ptr->get()[i].y,ptr->get()[i].z);
            }
         }
   		break;
		}
      case ID_OUT_velocity:
		{
			// Get the output port array ...
         CDataArray2DVector3f outData( in_ctxt );
         CDataArray2DVector3f::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".velocity" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         IV3fArrayProperty prop = Alembic::Abc::IV3fArrayProperty( obj.getSchema(), ".velocity" );
         if(!prop.valid())
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         if(prop.getNumSamples() == 0)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Alembic::Abc::V3fArraySamplePtr ptr = prop.getValue(sampleInfo.floorIndex);
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            if(sampleInfo.alpha != 0.0)
            {
               Alembic::Abc::V3fArraySamplePtr ptr2 = prop.getValue(sampleInfo.ceilIndex);
               float alpha = (float)sampleInfo.alpha;
               float ialpha = 1.0f - alpha;
               for(ULONG i=0;i<acc.GetCount();i++)
               {
                  acc[i].Set(
                     ialpha * ptr->get()[i].x + alpha * ptr2->get()[i].x,
                     ialpha * ptr->get()[i].y + alpha * ptr2->get()[i].y,
                     ialpha * ptr->get()[i].z + alpha * ptr2->get()[i].z);
               }
            }
            else
            {
               for(ULONG i=0;i<acc.GetCount();i++)
                  acc[i].Set(ptr->get()[i].x,ptr->get()[i].y,ptr->get()[i].z);
            }
         }
   		break;
		}
      case ID_OUT_radius:
		{
			// Get the output port array ...
         CDataArray2DFloat outData( in_ctxt );
         CDataArray2DFloat ::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".radius" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         IFloatArrayProperty prop = Alembic::Abc::IFloatArrayProperty( obj.getSchema(), ".radius" );
         if(!prop.valid())
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         if(prop.getNumSamples() == 0)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Alembic::Abc::FloatArraySamplePtr ptr = prop.getValue(sampleInfo.floorIndex);
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               acc[i] = ptr->get()[i];
            }
         }
   		break;
		}
      case ID_OUT_color:
		{
			// Get the output port array ...
         CDataArray2DColor4f outData( in_ctxt );
         CDataArray2DColor4f::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".color" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         IC4fArrayProperty prop = Alembic::Abc::IC4fArrayProperty( obj.getSchema(), ".color" );
         if(!prop.valid())
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         if(prop.getNumSamples() == 0)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Alembic::Abc::C4fArraySamplePtr ptr = prop.getValue(sampleInfo.floorIndex);
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               acc[i].Set(ptr->get()[i].r,ptr->get()[i].g,ptr->get()[i].b,ptr->get()[i].a);
            }
         }
   		break;
		}
      case ID_OUT_uv:
		{
			// Get the output port array ...
         CDataArray2DVector2f outData( in_ctxt );
         CDataArray2DVector2f::Accessor acc;

         Alembic::AbcGeom::IV2fGeomParam uvsParam = obj.getSchema().getUVsParam();
         if(!uvsParam)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         if(uvsParam.getNumSamples() == 0)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Alembic::Abc::V2fArraySamplePtr ptr = uvsParam.getExpandedValue(sampleInfo.floorIndex).getVals();
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               acc[i].PutX(ptr->get()[i].x);
               acc[i].PutY(1.0f - ptr->get()[i].y);
            }
         }
   		break;
		}
	};

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_curves_Term(CRef& in_ctxt)
{
   return alembicOp_Term(in_ctxt);
	Context ctxt( in_ctxt );
   CValue udVal = ctxt.GetUserData();
   ArchiveInfo * p = (ArchiveInfo*)(CValue::siPtrType)udVal;
   if(p != NULL)
   {
      delRefArchive(p->path.c_str());
      delete(p);
   }
   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_crvlist_topo_Define( CRef& in_ctxt )
{
   return alembicOp_Define(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_crvlist_topo_DefineLayout( CRef& in_ctxt )
{
   return alembicOp_DefineLayout(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_crvlist_topo_Update( CRef& in_ctxt )
{
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
   Alembic::AbcGeom::ICurves obj(iObj,Alembic::Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

   SampleInfo sampleInfo = getSampleInfo(
      ctxt.GetParameterValue(L"time"),
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );

   Alembic::AbcGeom::ICurvesSchema::Sample curveSample;
   obj.getSchema().get(curveSample,sampleInfo.floorIndex);

   // check for valid curve types...!
   if(curveSample.getType() != Alembic::AbcGeom::ALEMBIC_VERSION_NS::kLinear &&
      curveSample.getType() != Alembic::AbcGeom::ALEMBIC_VERSION_NS::kCubic)
   {
      Application().LogMessage(L"[ExocortexAlembic] Skipping curve '"+identifier+L"', invalid curve type.",siWarningMsg);
      return CStatus::Fail;
   }

   // prepare the curves
   Alembic::Abc::P3fArraySamplePtr curvePos = curveSample.getPositions();
   Alembic::Abc::Int32ArraySamplePtr curveNbVertices = curveSample.getCurvesNumVertices();

   CVector3Array pos((LONG)curvePos->size());

   CNurbsCurveDataArray curveDatas;
   size_t offset = 0;
   for(size_t j=0;j<curveNbVertices->size();j++)
   {
      CNurbsCurveData curveData;
      LONG nbVertices = (LONG)curveNbVertices->get()[j];
      curveData.m_aControlPoints.Resize(nbVertices);
      curveData.m_aKnots.Resize(nbVertices);
      curveData.m_siParameterization = siUniformParameterization;
      curveData.m_bClosed = curveSample.getWrap() == Alembic::AbcGeom::kPeriodic;
      curveData.m_lDegree = curveSample.getType() == Alembic::AbcGeom::ALEMBIC_VERSION_NS::kLinear ? 1 : 3;

      for(LONG k=0;k<nbVertices;k++)
      {
         curveData.m_aControlPoints[k].Set(
            curvePos->get()[offset].x,
            curvePos->get()[offset].y,
            curvePos->get()[offset].z,
            1.0);
         offset++;
      }

      // based on curve type, we do this for linear or closed cubic curves
      if(curveSample.getType() == Alembic::AbcGeom::ALEMBIC_VERSION_NS::kLinear || curveData.m_bClosed)
      {
         curveData.m_aKnots.Resize(nbVertices);
         for(LONG k=0;k<nbVertices;k++)
            curveData.m_aKnots[k] = k;
         if(curveData.m_bClosed)
            curveData.m_aKnots.Add(nbVertices);
      }
      else // cubic open
      {
         curveData.m_aKnots.Resize(nbVertices+2);
         curveData.m_aKnots[0] = 0;
         curveData.m_aKnots[1] = 0;
         for(LONG k=0;k<nbVertices-2;k++)
            curveData.m_aKnots[k+2] = k;
         curveData.m_aKnots[nbVertices] = nbVertices-3;
         curveData.m_aKnots[nbVertices+1] = nbVertices-3;
      }

      curveDatas.Add(curveData);
   }
   
   NurbsCurveList curves = Primitive(ctxt.GetOutputTarget()).GetGeometry();
   curves.Set(curveDatas);

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_crvlist_topo_Term(CRef & in_ctxt)
{
   return alembicOp_Term(in_ctxt);
}
