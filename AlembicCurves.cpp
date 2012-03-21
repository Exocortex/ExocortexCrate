#include "AlembicCurves.h"
#include "AlembicXform.h"
#include "SceneEntry.h"
#include <object.h>
#include "utility.h"

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
namespace AbcC = ::Alembic::AbcGeom::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;
using namespace AbcC;

enum SplineExportType
{
    SplineExport_Simple,
    SplineExport_Hair,
    SplineExport_PointCloud,
    SplineExport_None
};  

AlembicCurves::AlembicCurves(const SceneEntry &in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   std::string curveName = in_Ref.node->GetName();
   std::string xformName = curveName + "Xfo";

   Alembic::AbcGeom::OXform xform(GetOParent(),xformName,GetCurrentJob()->GetAnimatedTs());
   Alembic::AbcGeom::OCurves curves(xform,curveName,GetCurrentJob()->GetAnimatedTs());

   // create the generic properties
   mOVisibility = CreateVisibilityProperty(curves,GetCurrentJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();
   mCurvesSchema = curves.getSchema();

   // create all properties
   mRadiusProperty = OFloatArrayProperty(mCurvesSchema, ".radius", mCurvesSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
   mColorProperty = OC4fArrayProperty(mCurvesSchema, ".color", mCurvesSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
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

bool AlembicCurves::Save(double time)
{
    // Store the transformation
    SaveXformSample(GetRef(), mXformSchema, mXformSample, time);

    TimeValue ticks = GET_MAX_INTERFACE()->GetTime();

    // store the metadata
    // IMetaDataManager mng;
    // mng.GetMetaData(GetRef().node, 0);
    // SaveMetaData(prim.GetParent3DObject().GetRef(),this);

    // set the visibility
    if(GetRef().node->IsAnimated() || mNumSamples == 0)
    {
        float flVisibility = GetRef().node->GetLocalVisibility(ticks);
        mOVisibility.set(flVisibility > 0 ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);
    }

    // check if the spline is animated
    if(mNumSamples > 0) 
    {
        if(!GetRef().node->IsAnimated())
        {
            return true;
        }
    }

    SplineShape *pSplineShape = NULL;
    SplineExportType nExportType = SplineExport_None;
    BezierShape beziershape;
    PolyShape polyShape;
    bool bBezier = false;

    // Get a pointer to the spline shpae
    Object *obj = GetRef().node->EvalWorldState(ticks).obj;
    ShapeObject *pShapeObject = NULL;
    if (obj->IsShapeObject())
    {
        pShapeObject = reinterpret_cast<ShapeObject *>(obj);
        nExportType = SplineExport_Simple;
    }
    else
    {
        return false;
    }

    // Determine if we are a bezier shape
    if (pShapeObject->CanMakeBezier())
    {
        pShapeObject->MakeBezier(ticks, beziershape);
        bBezier = true;
    }
    else
    {
        pShapeObject->MakePolyShape(ticks, polyShape);
        bBezier = false;
    }

    // Get the control points
    std::vector<Point3> controlPoints;
    if (bBezier)
    {
        mNbVertices.reserve(beziershape.SplineCount());
        int oldControlPointCount = (int)controlPoints.size();
        for (int i = 0; i < beziershape.SplineCount(); i += 1)
        {
            Spline3D *pSpline = beziershape.GetSpline(i);
            int knots = pSpline->KnotCount();
            for(int ix = 0; ix < knots; ++ix) 
            {
                Point3 in = pSpline->GetInVec(ix);
                Point3 p = pSpline->GetKnotPoint(ix);
                Point3 out = pSpline->GetOutVec(ix);

                if (ix == 0 && !pSpline->Closed())
                {
                    controlPoints.push_back(p);
                    controlPoints.push_back(out);
                }
                else if ( ix == knots-1 && !pSpline->Closed())
                {
                    controlPoints.push_back(in);
                    controlPoints.push_back(p);
                }
                else
                {
                    controlPoints.push_back(in);
                    controlPoints.push_back(p);
                    controlPoints.push_back(out);
                }
            }

            int nNumPointsAdded = (int)controlPoints.size() - oldControlPointCount;
            mNbVertices.push_back(nNumPointsAdded);
            oldControlPointCount = (int)controlPoints.size();
        }
    }
    else
    {
        mNbVertices.resize(polyShape.numLines);
        for (int i = 0; i < polyShape.numLines; i += 1)
        {
            PolyLine &refLine = polyShape.lines[i];
            mNbVertices.push_back(refLine.numPts);
            for (int j = 0; j < refLine.numPts; j += 1)
            {
                Point3 p = refLine.pts[j].p;
                controlPoints.push_back(p);
            }
        }
    }

    int vertCount = (int)controlPoints.size();

    // prepare the bounding box
    Alembic::Abc::Box3d bbox;

	float masterScaleUnitMeters = (float)GetMasterScale(UNITS_METERS);

    // allocate the points and normals
    std::vector<Alembic::Abc::V3f> posVec(vertCount);
    for(int i=0;i<vertCount;i++)
    {
        posVec[i] = ConvertMaxPointToAlembicPoint(controlPoints[i], masterScaleUnitMeters );
        bbox.extendBy(posVec[i]);
    }

    // store the bbox
    mCurvesSample.setSelfBounds(bbox);

    // allocate for the points and normals
    Alembic::Abc::P3fArraySample posSample(&posVec.front(),posVec.size());

    // if we are the first frame!
    if(mNumSamples == 0)
    {
        Alembic::Abc::Int32ArraySample nbVerticesSample(&mNbVertices.front(),mNbVertices.size());
        mCurvesSample.setPositions(posSample);
        mCurvesSample.setCurvesNumVertices(nbVerticesSample);

        // set the type + wrapping
        mCurvesSample.setType(bBezier ? kCubic : kLinear);
        mCurvesSample.setWrap(pShapeObject->CurveClosed(ticks, 0) ? kPeriodic : kNonPeriodic);
        mCurvesSample.setBasis(kNoBasis);

        // save the sample
        mCurvesSchema.set(mCurvesSample);
    }
    else
    {
        mCurvesSample.setPositions(posSample);
        mCurvesSchema.set(mCurvesSample);
    }

   /*else if(prim.GetType().IsEqualNoCase(L"hair"))
   {
      HairPrimitive hairPrim(GetRef());
      LONG totalHairs = prim.GetParameterValue(L"TotalHairs");
      CRenderHairAccessor accessor = hairPrim.GetRenderHairAccessor(totalHairs,totalHairs,time);
      accessor.Next();

      CFloatArray hairPos;
      CStatus result = accessor.GetVertexPositions(hairPos);

      // prepare the bounding box
      Alembic::Abc::Box3d bbox;

      ULONG vertCount = hairPos.GetCount();
      vertCount /= 3;
      std::vector<Alembic::Abc::V3f> posVec(vertCount);
      ULONG offset = 0;
      for(ULONG i=0;i<vertCount;i++)
      {
         posVec[i].x = hairPos[offset++];
         posVec[i].y = hairPos[offset++];
         posVec[i].z = hairPos[offset++];
         bbox.extendBy(posVec[i]);
      }
      mCurvesSample.setPositions(Alembic::Abc::P3fArraySample(&posVec.front(),posVec.size()));
      hairPos.Clear();

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
               posVec[offset].x = (float)sub[j].GetX();
               posVec[offset].y = (float)sub[j].GetY();
               posVec[offset].z = (float)sub[j].GetZ();
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
                  mVelVec[offset].x = sub[j].GetX() / fps;
                  mVelVec[offset].y = sub[j].GetY() / fps;
                  mVelVec[offset].z = sub[j].GetZ() / fps;
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
                  mVelVec[i].x = data[i].GetX() / fps;
                  mVelVec[i].y = data[i].GetY() / fps;
                  mVelVec[i].z = data[i].GetZ() / fps;
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
   */
   mNumSamples++;

   return true;
}
