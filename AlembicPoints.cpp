#include "AlembicPoints.h"
#include "AlembicXform.h"

#include <xsi_application.h>
#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_geometry.h>
#include <xsi_polygonmesh.h>
#include <xsi_vertex.h>
#include <xsi_polygonface.h>
#include <xsi_sample.h>
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
#include <xsi_iceattribute.h>
#include <xsi_iceattributedataarray.h>
#include <xsi_shape.h>
#include <xsi_icenode.h>
#include <xsi_icenodeinputport.h>
#include <xsi_icenodedef.h>
#include <xsi_dataarray.h>
#include <xsi_dataarray2D.h>

using namespace XSI;
using namespace MATH;

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

AlembicPoints::AlembicPoints(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   Primitive prim(GetRef());
   CString iceName(prim.GetParent3DObject().GetName());
   CString xformName(iceName+L"Xfo");
   Alembic::AbcGeom::OXform xform(GetOParent(),xformName.GetAsciiString(),GetJob()->GetAnimatedTs());
   Alembic::AbcGeom::OPoints points(xform,iceName.GetAsciiString(),GetJob()->GetAnimatedTs());
   AddRef(prim.GetParent3DObject().GetKinematics().GetGlobal().GetRef());

   // create the generic properties
   mOVisibility = CreateVisibilityProperty(points,GetJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();
   mPointsSchema = points.getSchema();

   // create all properties
   mInstancenamesProperty = OStringArrayProperty(mPointsSchema, ".instancenames", mPointsSchema.getMetaData(), GetJob()->GetAnimatedTs() );

   // particle attributes
   mScaleProperty = OV3fArrayProperty(mPointsSchema, ".scale", mPointsSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mOrientationProperty = OQuatfArrayProperty(mPointsSchema, ".orientation", mPointsSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mAngularVelocityProperty = OQuatfArrayProperty(mPointsSchema, ".angularvelocity", mPointsSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mAgeProperty = OFloatArrayProperty(mPointsSchema, ".age", mPointsSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mMassProperty = OFloatArrayProperty(mPointsSchema, ".mass", mPointsSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mShapeTypeProperty = OUInt16ArrayProperty(mPointsSchema, ".shapetype", mPointsSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mShapeTimeProperty = OFloatArrayProperty(mPointsSchema, ".shapetime", mPointsSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mShapeInstanceIDProperty = OUInt16ArrayProperty(mPointsSchema, ".shapeinstanceid", mPointsSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mColorProperty = OC4fArrayProperty(mPointsSchema, ".color", mPointsSchema.getMetaData(), GetJob()->GetAnimatedTs() );
}

AlembicPoints::~AlembicPoints()
{
   // we have to clear this prior to destruction
   // this is a workaround for issue-171
   mOVisibility.reset();
}

Alembic::Abc::OCompoundProperty AlembicPoints::GetCompound()
{
   return mPointsSchema;
}

XSI::CStatus AlembicPoints::Save(double time)
{
   // store the transform
   Primitive prim(GetRef());
   SaveXformSample(GetRef(1),mXformSchema,mXformSample,time);

   // store the metadata
   SaveMetaData(prim.GetParent3DObject().GetRef(),this);

   // set the visibility
   Property visProp;
   prim.GetParent3DObject().GetPropertyFromName(L"Visibility",visProp);
   if(isRefAnimated(visProp.GetRef()) || mNumSamples == 0)
   {
      bool visibility = visProp.GetParameterValue(L"rendvis",time);
      mOVisibility.set(visibility ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);
   }

   // check if the pointcloud is animated
   if(mNumSamples > 0) {
      if(!isRefAnimated(GetRef()))
         return CStatus::OK;
   }

   // access the geometry
   Geometry geo = prim.GetGeometry(time);

   // deal with each attribute. we scope here do free memory instantly
   {
      // position == PointPosition
      std::vector<Alembic::Abc::V3f> positionVec;
      {
         // prepare the bounding box
         Alembic::Abc::Box3d bbox;

         ICEAttribute attr = geo.GetICEAttributeFromName(L"PointPosition");
         if(attr.IsDefined() && attr.IsValid())
         {
            CICEAttributeDataArrayVector3f data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            positionVec.resize((size_t)count);
            for(ULONG i=0;i<count;i++)
            {
               positionVec[i].setValue(data[i].GetX(),data[i].GetY(),data[i].GetZ());
               bbox.extendBy(positionVec[i]);
            }
         }

         // store the bbox
         mPointsSample.setSelfBounds(bbox);
      }
      if(positionVec.size() == 0)
         positionVec.push_back(Alembic::Abc::V3f(FLT_MAX,FLT_MAX,FLT_MAX));
      Alembic::Abc::P3fArraySample positionSample = Alembic::Abc::P3fArraySample(&positionVec.front(),positionVec.size());

      // velocity == PointVelocity
      std::vector<Alembic::Abc::V3f> velocityVec;
      {
         float fps = (float)CTime().GetFrameRate();
         ICEAttribute attr = geo.GetICEAttributeFromName(L"PointVelocity");
         if(attr.IsDefined() && attr.IsValid())
         {
            CICEAttributeDataArrayVector3f data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            velocityVec.resize((size_t)count);
            if(count > 0)
            {
               bool isConstant = true;
               CVector3f firstVal = data[0];
               for(ULONG i=0;i<count;i++)
               {
                  velocityVec[i].setValue(data[i].GetX() / fps,data[i].GetY() / fps,data[i].GetZ() / fps);
                  if(isConstant)
                     isConstant = firstVal == data[i];
               }
               if(isConstant)
                  velocityVec.resize(1);
            }
         }
      }
      if(velocityVec.size() == 0)
         velocityVec.push_back(Alembic::Abc::V3f(0.0f,0.0f,0.0f));
      Alembic::Abc::V3fArraySample velocitySample = Alembic::Abc::V3fArraySample(&velocityVec.front(),velocityVec.size());

      // width == Size
      std::vector<float> widthVec;
      {
         ICEAttribute attr = geo.GetICEAttributeFromName(L"Size");
         if(attr.IsDefined() && attr.IsValid())
         {
            CICEAttributeDataArrayFloat data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            widthVec.resize((size_t)count);
            if(count > 0)
            {
               bool isConstant = true;
               float firstVal = data[0];
               for(ULONG i=0;i<count;i++)
               {
                  widthVec[i] = data[i];
                  if(isConstant)
                     isConstant = firstVal == data[i];
               }
               if(isConstant)
                  widthVec.resize(1);
            }
         }
      }
      if(widthVec.size() == 0)
         widthVec.push_back(0.0f);
      Alembic::Abc::FloatArraySample widthSample = Alembic::Abc::FloatArraySample(&widthVec.front(),widthVec.size());

      // id == ID
      std::vector<uint64_t> idVec;
      {
         ICEAttribute attr = geo.GetICEAttributeFromName(L"ID");
         if(attr.IsDefined() && attr.IsValid())
         {
            CICEAttributeDataArrayLong data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            idVec.resize((size_t)count);
            if(count > 0)
            {
               bool isConstant = true;
               LONG firstVal = data[0];
               for(ULONG i=0;i<count;i++)
               {
                  idVec[i] = (uint64_t)data[i];
                  if(isConstant)
                     isConstant = firstVal == data[i];
               }
               if(isConstant)
                  idVec.resize(1);
            }
         }
      }
      if(idVec.size() == 0)
         idVec.push_back((uint64_t)-1);
      Alembic::Abc::UInt64ArraySample idSample(&idVec.front(),idVec.size());

      // store the Points sample
      mPointsSample.setPositions(positionSample);
      mPointsSample.setVelocities(velocitySample);
      mPointsSample.setWidths(Alembic::AbcGeom::OFloatGeomParam::Sample(widthSample,Alembic::AbcGeom::kVertexScope));
      mPointsSample.setIds(idSample);
      mPointsSchema.set(mPointsSample);
   }

   // scale
   {
      ICEAttribute attr = geo.GetICEAttributeFromName(L"Scale");
      std::vector<Alembic::Abc::V3f> vec;
      if(attr.IsDefined() && attr.IsValid() && attr.GetElementCount() > 0)
      {
         {
            CICEAttributeDataArrayVector3f data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            vec.resize((size_t)count);
            if(count > 0)
            {
               bool isConstant = true;
               CVector3f firstVal = data[0];
               for(ULONG i=0;i<count;i++)
               {
                  vec[i].setValue(data[i].GetX(),data[i].GetY(),data[i].GetZ());
                  if(isConstant)
                     isConstant = firstVal == data[i];
               }
               if(isConstant)
                  vec.resize(1);
            }
         }
         Alembic::Abc::V3fArraySample sample = Alembic::Abc::V3fArraySample(&vec.front(),vec.size());
         mScaleProperty.set(sample);
      }
      else
      {
         vec.resize(1,Alembic::Abc::V3f(1.0f,1.0f,1.0f));
         Alembic::Abc::V3fArraySample sample(&vec.front(),vec.size());
         mScaleProperty.set(sample);
      }
   }

   // orientation + angular vel
   for(int attrIndex = 0;attrIndex < 2; attrIndex++)
   {
      ICEAttribute attr = geo.GetICEAttributeFromName(attrIndex == 0 ? L"Orientation" : L"AngularVelocity");
      std::vector<Alembic::Abc::Quatf> vec;
      if(attr.IsDefined() && attr.IsValid() && attr.GetElementCount() > 0)
      {
         {
            CICEAttributeDataArrayRotationf data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            vec.resize((size_t)count);
            if(count > 0)
            {
               bool isConstant = true;
               Alembic::Abc::Quatf firstVal;
               for(ULONG i=0;i<count;i++)
               {
                  if(data[i].GetRepresentation() == CRotationf::siAxisAngleRot)
                  {
                     float angle;
                     CVector3f axis = data[i].GetAxisAngle(angle);
                     CRotation rot;
                     rot.SetFromAxisAngle(CVector3(axis.GetX(),axis.GetY(),axis.GetZ()),angle);
                     CQuaternion quat = rot.GetQuaternion();
                     vec[i].v.x = (float)quat.GetX();
                     vec[i].v.y = (float)quat.GetY();
                     vec[i].v.z = (float)quat.GetZ();
                     vec[i].r = (float)quat.GetW();
                  }
                  else if(data[i].GetRepresentation() == CRotationf::siEulerRot)
                  {
                     CVector3f euler = data[i].GetXYZAngles();
                     CRotation rot;
                     rot.SetFromXYZAngles(CVector3(euler.GetX(),euler.GetY(),euler.GetZ()));
                     CQuaternion quat = rot.GetQuaternion();
                     vec[i].v.x = (float)quat.GetX();
                     vec[i].v.y = (float)quat.GetY();
                     vec[i].v.z = (float)quat.GetZ();
                     vec[i].r = (float)quat.GetW();
                  }
                  else // quaternion
                  {
                     CQuaternionf quat = data[i].GetQuaternion();
                     vec[i].v.x = quat.GetX();
                     vec[i].v.y = quat.GetY();
                     vec[i].v.z = quat.GetZ();
                     vec[i].r = quat.GetW();
                  }
                  if(i==0)
                     firstVal = vec[i];
                  else if(isConstant)
                     isConstant = firstVal == vec[i];
               }
               if(isConstant)
                  vec.resize(1);
            }
         }
         Alembic::Abc::QuatfArraySample sample;
         if(vec.size() > 0)
            sample = Alembic::Abc::QuatfArraySample(&vec.front(),vec.size());
         if(attrIndex == 0)
            mOrientationProperty.set(sample);
         else
            mAngularVelocityProperty.set(sample);
      }
      else
      {
         vec.resize(1,Alembic::Abc::Quatf(0.0f,1.0f,0.0f,0.0f));
         Alembic::Abc::QuatfArraySample sample(&vec.front(),vec.size());
         if(attrIndex == 0)
            mOrientationProperty.set(sample);
         else
            mAngularVelocityProperty.set(sample);
      }
   }

   // age
   {
      ICEAttribute attr = geo.GetICEAttributeFromName(L"Age");
      std::vector<float> vec;
      if(attr.IsDefined() && attr.IsValid() && attr.GetElementCount() > 0)
      {
         {
            CICEAttributeDataArrayFloat data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            vec.resize((size_t)count);
            if(count > 0)
            {
               bool isConstant = true;
               float firstVal = data[0];
               for(ULONG i=0;i<count;i++)
               {
                  vec[i] = data[i];
                  if(isConstant)
                     isConstant = firstVal == data[i];
               }
               if(isConstant)
                  vec.resize(1);
            }
         }
         Alembic::Abc::FloatArraySample sample;
         if(vec.size() > 0)
            sample = Alembic::Abc::FloatArraySample(&vec.front(),vec.size());
         mAgeProperty.set(sample);
      }
      else
      {
         vec.resize(1,0.0f);
         Alembic::Abc::FloatArraySample sample(&vec.front(),vec.size());
         mAgeProperty.set(sample);
      }
   }

   // mass
   {
      ICEAttribute attr = geo.GetICEAttributeFromName(L"Mass");
      std::vector<float> vec;
      if(attr.IsDefined() && attr.IsValid() && attr.GetElementCount() > 0)
      {
         {
            CICEAttributeDataArrayFloat data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            vec.resize((size_t)count);
            if(count > 0)
            {
               bool isConstant = true;
               float firstVal = data[0];
               for(ULONG i=0;i<count;i++)
               {
                  vec[i] = data[i];
                  if(isConstant)
                     isConstant = firstVal == data[i];
               }
               if(isConstant)
                  vec.resize(1);
            }
         }
         Alembic::Abc::FloatArraySample sample;
         if(vec.size() > 0)
            sample = Alembic::Abc::FloatArraySample(&vec.front(),vec.size());
         mMassProperty.set(sample);
      }
      else
      {
         vec.resize(1,1.0f);
         Alembic::Abc::FloatArraySample sample(&vec.front(),vec.size());
         mMassProperty.set(sample);
      }
   }

   // shapetype
   bool usesInstances = false;
   {
      ICEAttribute attr = geo.GetICEAttributeFromName(L"Shape");
      std::vector<uint16_t> vec;
      if(attr.IsDefined() && attr.IsValid() && attr.GetElementCount() > 0)
      {
         {
            CICEAttributeDataArrayShape data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            vec.resize((size_t)count);
            if(count > 0)
            {
               bool isConstant = true;
               CShape firstVal = data[0];
               for(ULONG i=0;i<count;i++)
               {
                  CShape shape = data[i];
                  switch(shape.GetType())
                  {
                     case siICEShapeBox:
                     {
                        vec[i] = ShapeType_Box;
                        break;
                     }
                     case siICEShapeCylinder:
                     {
                        vec[i] = ShapeType_Cylinder;
                        break;
                     }
                     case siICEShapeCone:
                     {
                        vec[i] = ShapeType_Cone;
                        break;
                     }
                     case siICEShapeDisc:
                     {
                        vec[i] = ShapeType_Disc;
                        break;
                     }
                     case siICEShapeRectangle:
                     {
                        vec[i] = ShapeType_Rectangle;
                        break;
                     }
                     case siICEShapeSphere:
                     {
                        vec[i] = ShapeType_Sphere;
                        break;
                     }
                     case siICEShapeInstance:
                     case siICEShapeReference:
                     {
                        vec[i] = ShapeType_Instance;
                        usesInstances = true;
                        break;
                     }
                     default:
                     {
                        vec[i] = ShapeType_Point;
                        break;
                     }
                  }
                  if(isConstant)
                  {
                     isConstant = firstVal.GetType() == shape.GetType();
                     if(isConstant && (firstVal.GetType() == siICEShapeInstance || firstVal.GetType() == siICEShapeReference))
                        isConstant = firstVal.GetReferenceID() == shape.GetReferenceID();
                  }
               }
               if(isConstant)
                  vec.resize(1);
            }
         }
         Alembic::Abc::UInt16ArraySample sample;
         if(vec.size() > 0)
            sample = Alembic::Abc::UInt16ArraySample(&vec.front(),vec.size());
         mShapeTypeProperty.set(sample);
      }
      else
      {
         vec.resize(1,ShapeType_Point);
         Alembic::Abc::UInt16ArraySample sample(&vec.front(),vec.size());
         mShapeTypeProperty.set(sample);
      }
   }

   // shapeInstanceID
   {
      ICEAttribute attr = geo.GetICEAttributeFromName(L"Shape");
      std::vector<uint16_t> vec;
      if(attr.IsDefined() && attr.IsValid() && attr.GetElementCount() > 0)
      {
         {
            std::map<unsigned long,size_t>::iterator it;

            CICEAttributeDataArrayShape data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            vec.resize((size_t)count);
            if(count > 0)
            {
               bool isConstant = true;
               uint16_t firstVal;
               for(ULONG i=0;i<count;i++)
               {
                  CShape shape = data[i];
                  switch(shape.GetType())
                  {
                     case siICEShapeInstance:
                     case siICEShapeReference:
                     {
                        // check if we know this instance
                        it = mInstanceMap.find(shape.GetReferenceID());
                        if(it == mInstanceMap.end())
                        {
                           // insert it
                           CRef ref = Application().GetObjectFromID(shape.GetReferenceID());
                           mInstanceMap.insert(std::pair<unsigned long,size_t>(shape.GetReferenceID(),mInstanceNames.size()));
                           vec[i] = (unsigned short)mInstanceNames.size();
                           mInstanceNames.push_back(getIdentifierFromRef(ref));
                        }
                        else
                           vec[i] = (unsigned short)it->second;
                        break;
                     }
                     default:
                     {
                        vec[i] = 0;
                        break;
                     }
                  }
                  if(i==0)
                     firstVal = vec[i];
                  else if(isConstant)
                     isConstant = firstVal == vec[i];
               }
               if(isConstant)
                  vec.resize(1);
            }
         }
         if(vec.size() == 0)
            vec.push_back(0 );
         Alembic::Abc::UInt16ArraySample sample(&vec.front(),vec.size());
         mShapeInstanceIDProperty.set(sample);

         Alembic::Abc::StringArraySample namesSample;
         std::vector<std::string> mTempName;
         if(mInstanceNames.size() > 0)
            namesSample = Alembic::Abc::StringArraySample(&mInstanceNames.front(),mInstanceNames.size());
         else
         {
            mTempName.push_back("");
            namesSample = Alembic::Abc::StringArraySample(&mTempName.front(),mTempName.size());
         }
         mInstancenamesProperty.set(namesSample);
      }
      else
      {
         vec.push_back(0);
         Alembic::Abc::UInt16ArraySample sample(&vec.front(),vec.size());
         mShapeInstanceIDProperty.set(sample);

         std::vector<std::string> mTempName;
         mTempName.push_back("");
         Alembic::Abc::StringArraySample namesSample(&mTempName.front(),mTempName.size());
         mInstancenamesProperty.set(namesSample);
      }
   }

   // shapetime
   {
      ICEAttribute attr = geo.GetICEAttributeFromName(L"ShapeInstancetime");
      std::vector<float> vec;
      if(attr.IsDefined() && attr.IsValid() && attr.GetElementCount() > 0)
      {
         {
            CICEAttributeDataArrayFloat data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            vec.resize((size_t)count);
            if(count > 0)
            {
               bool isConstant = true;
               float firstVal = data[0];
               for(ULONG i=0;i<count;i++)
               {
                  vec[i] = data[i];
                  if(isConstant)
                     isConstant = firstVal == data[i];
               }
               if(isConstant)
                  vec.resize(1);
            }
         }
         Alembic::Abc::FloatArraySample sample;
         if(vec.size() > 0)
            sample = Alembic::Abc::FloatArraySample(&vec.front(),vec.size());
         mShapeTimeProperty.set(sample);
      }
      else
      {
         vec.resize(1,1.0f);
         Alembic::Abc::FloatArraySample sample(&vec.front(),vec.size());
         mMassProperty.set(sample);
      }
   }

   // color
   {
      ICEAttribute attr = geo.GetICEAttributeFromName(L"Color");
      std::vector<Alembic::Abc::C4f> vec;
      if(attr.IsDefined() && attr.IsValid() && attr.GetElementCount() > 0)
      {
         {
            CICEAttributeDataArrayColor4f data;
            attr.GetDataArray(data);
            ULONG count = (data.IsConstant() || attr.IsConstant()) ? 1 : data.GetCount();
            vec.resize((size_t)count);
            if(count > 0)
            {
               bool isConstant = true;
               Alembic::Abc::C4f firstVal;
               for(ULONG i=0;i<count;i++)
               {
                  vec[i].r = data[i].GetR();
                  vec[i].g = data[i].GetG();
                  vec[i].b = data[i].GetB();
                  vec[i].a = data[i].GetA();
                  if(i==0)
                     firstVal = vec[i];
                  else if(isConstant)
                     isConstant = firstVal == vec[i];
               }
               if(isConstant)
                  vec.resize(1);
            }
         }
         Alembic::Abc::C4fArraySample sample;
         if(vec.size() > 0)
            sample = Alembic::Abc::C4fArraySample(&vec.front(),vec.size());
         mColorProperty.set(sample);
      }
      else
      {
         vec.resize(1,Alembic::Abc::C4f(0.0f,0.0f,0.0f,1.0f));
         Alembic::Abc::C4fArraySample sample(&vec.front(),vec.size());
         mColorProperty.set(sample);
      }
   }

   mNumSamples++;

   return CStatus::OK;
}

enum IDs
{
	ID_IN_path = 0,
	ID_IN_identifier = 1,
	ID_IN_renderpath = 2,
	ID_IN_renderidentifier = 3,
	ID_IN_time = 4,
	ID_IN_usevel = 5,
	ID_G_100 = 100,
	ID_OUT_position = 201,
	ID_OUT_velocity = 202,
	ID_OUT_id = 204,
	ID_OUT_size = 203,
	ID_OUT_scale = 205,
	ID_OUT_orientation = 206,
	ID_OUT_angularvelocity = 207,
	ID_OUT_age = 208,
	ID_OUT_mass = 209,
	ID_OUT_shape = 210,
	ID_OUT_shapeid = 211,
	ID_OUT_color = 212,
	ID_OUT_shapetime = 213,
	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
};

#define ID_UNDEF ((ULONG)-1)

using namespace XSI;
using namespace MATH;

CStatus Register_alembic_points( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	nodeDef = Application().GetFactory().CreateICENodeDef(L"alembic_points",L"alembic_points");

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
   st = nodeDef.AddInputPort(ID_IN_usevel,ID_G_100,siICENodeDataBool,siICENodeStructureSingle,siICENodeContextSingleton,L"usevel",L"usevel",false,ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
	
   // Add output ports.
   st = nodeDef.AddOutputPort(ID_OUT_position,siICENodeDataVector3,siICENodeStructureArray,siICENodeContextSingleton,L"position",L"position",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_velocity,siICENodeDataVector3,siICENodeStructureArray,siICENodeContextSingleton,L"velocity",L"velocity",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_id,siICENodeDataLong,siICENodeStructureArray,siICENodeContextSingleton,L"id",L"id",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_size,siICENodeDataFloat,siICENodeStructureArray,siICENodeContextSingleton,L"size",L"size",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_scale,siICENodeDataVector3,siICENodeStructureArray,siICENodeContextSingleton,L"scale",L"scale",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_orientation,siICENodeDataRotation,siICENodeStructureArray,siICENodeContextSingleton,L"orientation",L"orientation",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_angularvelocity,siICENodeDataRotation,siICENodeStructureArray,siICENodeContextSingleton,L"angularvelocity",L"angularvelocity",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_age,siICENodeDataFloat,siICENodeStructureArray,siICENodeContextSingleton,L"age",L"age",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_mass,siICENodeDataFloat,siICENodeStructureArray,siICENodeContextSingleton,L"mass",L"mass",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_shape,siICENodeDataShape,siICENodeStructureArray,siICENodeContextSingleton,L"shape",L"shape",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_shapeid,siICENodeDataLong,siICENodeStructureArray,siICENodeContextSingleton,L"shapeid",L"shapeid",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_shapetime,siICENodeDataFloat,siICENodeStructureArray,siICENodeContextSingleton,L"shapetime",L"shapetime",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_color,siICENodeDataColor4,siICENodeStructureArray,siICENodeContextSingleton,L"color",L"color",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(L"Custom ICENode");

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_points_Evaluate(ICENodeContext& in_ctxt)
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
   Alembic::AbcGeom::IPoints obj(iObj,Alembic::Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

	CDataArrayFloat timeData( in_ctxt, ID_IN_time);
   double time = timeData[0];

	CDataArrayBool usevelData( in_ctxt, ID_IN_usevel);
   double usevel = usevelData[0];

   SampleInfo sampleInfo = getSampleInfo(
      time,
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );
   Alembic::AbcGeom::IPointsSchema::Sample sample;
   obj.getSchema().get(sample,sampleInfo.floorIndex);

	switch( out_portID )
	{
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
            if(sampleInfo.alpha != 0.0 && usevel)
            {
               float alpha = (float)sampleInfo.alpha;

               Alembic::Abc::V3fArraySamplePtr velPtr = sample.getVelocities();
               if(velPtr == NULL)
                  done = false;
               else if(velPtr->size() == 0)
                  done = false;
               else
               {
                  for(ULONG i=0;i<acc.GetCount();i++)
                     acc[i].Set(
                        ptr->get()[i].x + alpha * velPtr->get()[i >= velPtr->size() ? 0 : i].x,
                        ptr->get()[i].y + alpha * velPtr->get()[i >= velPtr->size() ? 0 : i].y,
                        ptr->get()[i].z + alpha * velPtr->get()[i >= velPtr->size() ? 0 : i].z);
                  done = true;
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

         Alembic::Abc::V3fArraySamplePtr ptr = sample.getVelocities();
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            float fps = (float)CTime().GetFrameRate();
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
               acc[i].Set(ptr->get()[i].x * fps,ptr->get()[i].y * fps,ptr->get()[i].z * fps);
         }
   		break;
		}
      case ID_OUT_id:
		{
			// Get the output port array ...
         CDataArray2DLong outData( in_ctxt );
         CDataArray2DLong::Accessor acc;

         Alembic::Abc::UInt64ArraySamplePtr ptr = sample.getIds();
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
               acc[i] = (LONG)ptr->get()[i];
         }
   		break;
		}
      case ID_OUT_size:
		{
			// Get the output port array ...
         CDataArray2DFloat outData( in_ctxt );
         CDataArray2DFloat::Accessor acc;

         Alembic::AbcGeom::IFloatGeomParam widthParam = obj.getSchema().getWidthsParam();
         if(!widthParam)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         if(widthParam.getNumSamples() == 0)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }

         Alembic::Abc::FloatArraySamplePtr ptr = widthParam.getExpandedValue(sampleInfo.floorIndex).getVals();
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
               acc[i] = ptr->get()[i];
         }
   		break;
		}
      case ID_OUT_scale:
		{
			// Get the output port array ...
         CDataArray2DVector3f outData( in_ctxt );
         CDataArray2DVector3f::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".scale" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         IV3fArrayProperty prop = Alembic::Abc::IV3fArrayProperty( obj.getSchema(), ".scale" );
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
            for(ULONG i=0;i<acc.GetCount();i++)
               acc[i].Set(ptr->get()[i].x,ptr->get()[i].y,ptr->get()[i].z);
         }
   		break;
		}
      case ID_OUT_orientation:
		{
			// Get the output port array ...
         CDataArray2DRotationf outData( in_ctxt );
         CDataArray2DRotationf::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".orientation" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         IQuatfArrayProperty prop = Alembic::Abc::IQuatfArrayProperty( obj.getSchema(), ".orientation" );
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
         Alembic::Abc::QuatfArraySamplePtr ptr = prop.getValue(sampleInfo.floorIndex);
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            bool done = false;
            if(sampleInfo.alpha != 0.0 && usevel)
            {
               float alpha = (float)sampleInfo.alpha;
               if ( obj.getSchema().getPropertyHeader( ".angularvelocity" ) == NULL )
               {
                  acc = outData.Resize(0,0);
                  return CStatus::OK;
               }
               IQuatfArrayProperty velProp = Alembic::Abc::IQuatfArrayProperty( obj.getSchema(), ".angularvelocity" );
               if(!velProp.valid())
               {
                  acc = outData.Resize(0,0);
                  return CStatus::OK;
               }
               if(velProp.getNumSamples() == 0)
               {
                  acc = outData.Resize(0,0);
                  return CStatus::OK;
               }
               Alembic::Abc::QuatfArraySamplePtr velPtr = velProp.getValue(sampleInfo.floorIndex);
               if(velPtr == NULL)
                  acc = outData.Resize(0,0);
               else if(velPtr->size() == 0)
                  acc = outData.Resize(0,0);
               else
               {
                  CQuaternionf quat,vel;
                  for(ULONG i=0;i<acc.GetCount();i++)
                  {
                     quat.Set(ptr->get()[i].r,ptr->get()[i].v.x,ptr->get()[i].v.y,ptr->get()[i].v.z);
                     vel.Set(
                        velPtr->get()[i >= velPtr->size() ? 0 : i].r,
                        velPtr->get()[i >= velPtr->size() ? 0 : i].v.x,
                        velPtr->get()[i >= velPtr->size() ? 0 : i].v.y,
                        velPtr->get()[i >= velPtr->size() ? 0 : i].v.z);
                     vel.PutW(vel.GetW() * alpha);
                     vel.PutX(vel.GetX() * alpha);
                     vel.PutY(vel.GetY() * alpha);
                     vel.PutZ(vel.GetZ() * alpha);
                     if(vel.GetW() != 0.0f)
                        quat.Mul(vel,quat);
                     quat.NormalizeInPlace();
                     acc[i].Set(quat);
                  }
                  done = true;
               }
            }

            if(!done)
            {
               CQuaternionf quat;
               for(ULONG i=0;i<acc.GetCount();i++)
               {
                  quat.Set(ptr->get()[i].r,ptr->get()[i].v.x,ptr->get()[i].v.y,ptr->get()[i].v.z);
                  acc[i].Set(quat);
               }
            }
         }
   		break;
		}
      case ID_OUT_angularvelocity:
		{
			// Get the output port array ...
         CDataArray2DRotationf outData( in_ctxt );
         CDataArray2DRotationf::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".angularvelocity" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         IQuatfArrayProperty prop = Alembic::Abc::IQuatfArrayProperty( obj.getSchema(), ".angularvelocity" );
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
         Alembic::Abc::QuatfArraySamplePtr ptr = prop.getValue(sampleInfo.floorIndex);
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            CQuaternionf quat;
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               quat.Set(ptr->get()[i].r,ptr->get()[i].v.z,ptr->get()[i].v.y,ptr->get()[i].v.z);
               acc[i].Set(quat);
            }
         }
   		break;
		}
      case ID_OUT_age:
		{
			// Get the output port array ...
         CDataArray2DFloat outData( in_ctxt );
         CDataArray2DFloat ::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".age" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         IFloatArrayProperty prop = Alembic::Abc::IFloatArrayProperty( obj.getSchema(), ".age" );
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
      case ID_OUT_mass:
		{
			// Get the output port array ...
         CDataArray2DFloat outData( in_ctxt );
         CDataArray2DFloat ::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".mass" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         IFloatArrayProperty prop = Alembic::Abc::IFloatArrayProperty( obj.getSchema(), ".mass" );
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
      case ID_OUT_shape:
		{
			// Get the output port array ...
         CDataArray2DShape outData( in_ctxt );
         CDataArray2DShape::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".shapetype" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         IUInt16ArrayProperty shapeTypeProp = Alembic::Abc::IUInt16ArrayProperty( obj.getSchema(), ".shapetype" );
         /*
         IUInt16ArrayProperty shapeInstanceIDProp = Alembic::Abc::IUInt16ArrayProperty( obj.getSchema(), ".shapeinstanceid" );
         IStringArrayProperty shapeInstanceNamesProp = Alembic::Abc::IStringArrayProperty( obj.getSchema(), ".instancenames" );
         Alembic::Abc::UInt16ArraySamplePtr shapeInstanceIDPtr = shapeInstanceIDProp.getValue(sampleInfo.floorIndex);
         Alembic::Abc::StringArraySamplePtr shapeInstanceNamesPtr = shapeInstanceNamesProp.getValue(sampleInfo.floorIndex);
         */
         if(!shapeTypeProp.valid())
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         if(shapeTypeProp.getNumSamples() == 0)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Alembic::Abc::UInt16ArraySamplePtr shapeTypePtr = shapeTypeProp.getValue(sampleInfo.floorIndex);
         if(shapeTypePtr == NULL)
            acc = outData.Resize(0,0);
         else if(shapeTypePtr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)shapeTypePtr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               switch(shapeTypePtr->get()[i])
               {
                  case AlembicPoints::ShapeType_Point:
                  {
                     acc[i] = CShape(siICEShapePoint);
                     break;
                  }
                  case AlembicPoints::ShapeType_Box:
                  {
                     acc[i] = CShape(siICEShapeBox);
                     break;
                  }
                  case AlembicPoints::ShapeType_Sphere:
                  {
                     acc[i] = CShape(siICEShapeSphere);
                     break;
                  }
                  case AlembicPoints::ShapeType_Cylinder:
                  {
                     acc[i] = CShape(siICEShapeCylinder);
                     break;
                  }
                  case AlembicPoints::ShapeType_Cone:
                  {
                     acc[i] = CShape(siICEShapeCone);
                     break;
                  }
                  case AlembicPoints::ShapeType_Disc:
                  {
                     acc[i] = CShape(siICEShapeDisc);
                     break;
                  }
                  case AlembicPoints::ShapeType_Rectangle:
                  {
                     acc[i] = CShape(siICEShapeRectangle);
                     break;
                  }
                  case AlembicPoints::ShapeType_Instance:
                  {
                     acc[i] = CShape(siICEShapePoint);
                     break;
                  }
               }
            }
         }
   		break;
		}
      case ID_OUT_shapeid:
		{
			// Get the output port array ...
         CDataArray2DLong outData( in_ctxt );
         CDataArray2DLong::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".shapetype" ) == NULL || obj.getSchema().getPropertyHeader( ".shapeinstanceid" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         IUInt16ArrayProperty shapeTypeProp = Alembic::Abc::IUInt16ArrayProperty( obj.getSchema(), ".shapetype" );
         IUInt16ArrayProperty shapeInstanceIDProp = Alembic::Abc::IUInt16ArrayProperty( obj.getSchema(), ".shapeinstanceid" );

         if(!shapeTypeProp.valid() || !shapeInstanceIDProp.valid())
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         if(shapeTypeProp.getNumSamples() == 0 || shapeInstanceIDProp.getNumSamples() == 0)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Alembic::Abc::UInt16ArraySamplePtr shapeTypePtr = shapeTypeProp.getValue(sampleInfo.floorIndex);
         Alembic::Abc::UInt16ArraySamplePtr shapeInstanceIDPtr = shapeInstanceIDProp.getValue(sampleInfo.floorIndex);
         if(shapeTypePtr == NULL || shapeInstanceIDPtr == NULL)
            acc = outData.Resize(0,0);
         else if(shapeTypePtr->size() == 0 || shapeInstanceIDPtr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)shapeTypePtr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               switch(shapeTypePtr->get()[i])
               {
                  case AlembicPoints::ShapeType_Instance:
                  {
                     acc[i] = (LONG)shapeInstanceIDPtr->get()[i];
                     break;
                  }
                  default:
                  {
                     // for all other shapes
                     acc[i] = -1l;
                     break;
                  }
               }
            }
         }
   		break;
		}
      case ID_OUT_shapetime:
		{
			// Get the output port array ...
         CDataArray2DFloat outData( in_ctxt );
         CDataArray2DFloat ::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".shapetime" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         IFloatArrayProperty prop = Alembic::Abc::IFloatArrayProperty( obj.getSchema(), ".shapetime" );
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
	};

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_points_Term(CRef& in_ctxt)
{
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
