#include "stdafx.h"
#include "AlembicCurves.h"
#include "AlembicXform.h"
#include "SceneEnumProc.h"
#include "utility.h"
#include "ExocortexCoreServicesAPI.h"
#include "AlembicMetadataUtils.h"
#include <surf_api.h> 

enum SplineExportType
{
    SplineExport_Simple,
    SplineExport_Hair,
    SplineExport_PointCloud,
    SplineExport_None
};  

AlembicCurves::AlembicCurves(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent)
: AlembicObject(eNode, in_Job, oParent)
{
    std::string xformName = EC_MCHAR_to_UTF8( mMaxNode->GetName() );
	std::string curveName = xformName + "Shape";

   AbcG::OCurves curves(GetOParent(),curveName,GetCurrentJob()->GetAnimatedTs());
   mCurvesSchema = curves.getSchema();

   // create all properties
   //mInTangentProperty = OV3fArrayProperty(mCurvesSchema.getArbGeomParams(), ".inTangent", mCurvesSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
   //mOutTangentProperty = OV3fArrayProperty(mCurvesSchema.getArbGeomParams(), ".outTangent", mCurvesSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
   //mRadiusProperty = OFloatArrayProperty(mCurvesSchema.getArbGeomParams(), ".radius", mCurvesSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
   //mColorProperty = OC4fArrayProperty(mCurvesSchema.getArbGeomParams(), ".color", mCurvesSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
}

AlembicCurves::~AlembicCurves()
{
}

Abc::OCompoundProperty AlembicCurves::GetCompound()
{
   return mCurvesSchema;
}

bool AlembicCurves::Save(double time, bool bLastFrame)
{
    ESS_PROFILE_FUNC();

    //TimeValue ticks = GET_MAX_INTERFACE()->GetTime();
    TimeValue ticks = GetTimeValueFromFrame(time);
	Object *obj = mMaxNode->EvalWorldState(ticks).obj;
	if(mNumSamples == 0){
		bForever = CheckIfObjIsValidForever(obj, ticks);
	}
	else{
		bool bNewForever = CheckIfObjIsValidForever(obj, ticks);
		if(bForever && bNewForever != bForever){
			ESS_LOG_INFO( "bForever has changed" );
		}
	}

	SaveMetaData(mMaxNode, this);

    // check if the spline is animated
    if(mNumSamples > 0) 
    {
        if(bForever)
        {
            return true;
        }
    }

    AbcG::OCurvesSchema::Sample curvesSample;

	std::vector<AbcA::int32_t> nbVertices;
    std::vector<Point3> vertices;
    std::vector<float> knotVector;
    std::vector<Abc::uint16_t> orders;

    if(obj->ClassID() == EDITABLE_SURF_CLASS_ID){

       NURBSSet nurbsSet;   
       BOOL success = GetNURBSSet(obj, ticks, nurbsSet, TRUE);   

       AbcG::CurvePeriodicity cPeriod = AbcG::kNonPeriodic;
       AbcG::CurveType cType = AbcG::kCubic;
       AbcG::BasisType cBasis = AbcG::kNoBasis;

       int n = nurbsSet.GetNumObjects();
       for(int i=0; i<n; i++){
          NURBSObject* pObject = nurbsSet.GetNURBSObject((int)i);

          //NURBSType type = pObject->GetType();
          if(!pObject){
             continue;
          }

          if( pObject->GetKind() == kNURBSCurve ){
             NURBSCurve* pNurbsCurve = (NURBSCurve*)pObject;

             int degree;
             int numCVs;
             NURBSCVTab cvs;
			 int numKnots;
		     NURBSKnotTab knots;
             pNurbsCurve->GetNURBSData(ticks, degree, numCVs, cvs, numKnots, knots);

             orders.push_back(degree+1);

             const int cvsCount = cvs.Count();
             const int knotCount = knots.Count();

             for(int j=0; j<cvs.Count(); j++){
                NURBSControlVertex cv = cvs[j];
                double x, y, z;
                cv.GetPosition(ticks, x, y, z);
                vertices.push_back( Point3((float)x, (float)y, (float)z) );
             }

             nbVertices.push_back(cvsCount);

             //skip the first and last entry because Maya and XSI use this format
             for(int j=1; j<knots.Count()-1; j++){
                knotVector.push_back((float)knots[j]);
             }

             if(i == 0){
                if(pNurbsCurve->IsClosed()){
                   cPeriod = AbcG::kPeriodic;
                }  
             }
             else{
                if(pNurbsCurve->IsClosed()){
                   if(cPeriod != AbcG::kPeriodic){
                      ESS_LOG_WARNING("Mixed curve wrap types not supported.");
                   }
                }
                else{
                   if(cPeriod != AbcG::kNonPeriodic){
                      ESS_LOG_WARNING("Mixed curve wrap types not supported.");
                   }
                }
             }

          }
          
       }
       

       curvesSample.setType(cType);
       curvesSample.setWrap(cPeriod);
       curvesSample.setBasis(cBasis);
    }
    else
    {
          BezierShape beziershape;
          PolyShape polyShape;
          bool bBezier = false;

          // Get a pointer to the spline shpae
          ShapeObject *pShapeObject = NULL;
          if (obj->IsShapeObject())
          {
              pShapeObject = reinterpret_cast<ShapeObject *>(obj);
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

          //std::vector<Point3> inTangents;
	      //std::vector<Point3> outTangents;
          if (bBezier)
          {
              int oldVerticesCount = (int)vertices.size();
              for (int i = 0; i < beziershape.SplineCount(); i += 1)
              {
                  Spline3D *pSpline = beziershape.GetSpline(i);
                  int knots = pSpline->KnotCount();
                  for(int ix = 0; ix < knots; ++ix) 
                  {
                      Point3 in = pSpline->GetInVec(ix);
                      Point3 p = pSpline->GetKnotPoint(ix);
                      Point3 out = pSpline->GetOutVec(ix);

                      vertices.push_back( p );
				      //inTangents.push_back( in );
				      //outTangents.push_back( out );
                  }

                  int nNumVerticesAdded = (int)vertices.size() - oldVerticesCount;
                  nbVertices.push_back( nNumVerticesAdded );
                  oldVerticesCount = (int)vertices.size();
              }
          }
          else
          {
              for (int i = 0; i < polyShape.numLines; i += 1)
              {
                  PolyLine &refLine = polyShape.lines[i];
                  nbVertices.push_back(refLine.numPts);
                  for (int j = 0; j < refLine.numPts; j += 1)
                  {
                      Point3 p = refLine.pts[j].p;
                      vertices.push_back(p);
                  }
              }
          }

          // set the type + wrapping
	      curvesSample.setType(bBezier ? AbcG::kCubic : AbcG::kLinear);
          curvesSample.setWrap(pShapeObject->CurveClosed(ticks, 0) ? AbcG::kPeriodic : AbcG::kNonPeriodic);
          curvesSample.setBasis(AbcG::kNoBasis);
    }

    if(nbVertices.size() == 0 || vertices.size() == 0){
       ESS_LOG_WARNING("No curve data to export.");
       return false;
    }
 

    const int vertCount = (int)vertices.size();

    // prepare the bounding box
    Abc::Box3d bbox;

    // allocate the points and normals
    std::vector<Abc::V3f> posVec(vertCount);
    Matrix3 wm = mMaxNode->GetObjTMAfterWSM(ticks);

    for(int i=0;i<vertCount;i++)
    {
        posVec[i] = ConvertMaxPointToAlembicPoint(vertices[i] );
        bbox.extendBy(posVec[i]);

        // Set the archive bounding box
        if (mJob)
        {
            Point3 worldMaxPoint = wm * vertices[i];
            Abc::V3f alembicWorldPoint = ConvertMaxPointToAlembicPoint(worldMaxPoint);
            mJob->GetArchiveBBox().extendBy(alembicWorldPoint);
        }
    }

    if(knotVector.size() > 0 && orders.size() > 0){
       if(!mKnotVectorProperty.valid()){
          mKnotVectorProperty = Abc::OFloatArrayProperty(mCurvesSchema.getArbGeomParams(), ".knot_vector", mCurvesSchema.getMetaData(), mJob->GetAnimatedTs() );
       }
       mKnotVectorProperty.set(Abc::FloatArraySample(knotVector));

       if(!mOrdersProperty.valid()){
          mOrdersProperty = Abc::OUInt16ArrayProperty(mCurvesSchema.getArbGeomParams(), ".orders", mCurvesSchema.getMetaData(), mJob->GetAnimatedTs() );
       }
       mOrdersProperty.set(Abc::UInt16ArraySample(orders));
    }

    // store the bbox
    curvesSample.setSelfBounds(bbox);
	mCurvesSchema.getChildBoundsProperty().set(bbox);

 
    Abc::Int32ArraySample nbVerticesSample(&nbVertices.front(),nbVertices.size());
    curvesSample.setCurvesNumVertices(nbVerticesSample);


 
    // allocate for the points and normals
    Abc::P3fArraySample posSample(&posVec.front(),posVec.size());
	curvesSample.setPositions(posSample);


    mCurvesSchema.set(curvesSample);

   mNumSamples++;

   return true;
}
