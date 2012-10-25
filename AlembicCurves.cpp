#include "AlembicMax.h"
#include "AlembicCurves.h"
#include "AlembicXform.h"
#include "SceneEnumProc.h"
#include "utility.h"
#include "ExocortexCoreServicesAPI.h"
#include "AlembicMetadataUtils.h"


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
   std::string curveName = EC_MCHAR_to_UTF8( in_Ref.node->GetName() );
   std::string xformName = curveName + "Xfo";

   AbcG::OXform xform(GetOParent(),xformName,GetCurrentJob()->GetAnimatedTs());
   AbcG::OCurves curves(xform,curveName,GetCurrentJob()->GetAnimatedTs());

   // create the generic properties
   mOVisibility = CreateVisibilityProperty(curves,GetCurrentJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();
   mCurvesSchema = curves.getSchema();

   // create all properties
   //mInTangentProperty = OV3fArrayProperty(mCurvesSchema.getArbGeomParams(), ".inTangent", mCurvesSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
   //mOutTangentProperty = OV3fArrayProperty(mCurvesSchema.getArbGeomParams(), ".outTangent", mCurvesSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
   //mRadiusProperty = OFloatArrayProperty(mCurvesSchema.getArbGeomParams(), ".radius", mCurvesSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
   //mColorProperty = OC4fArrayProperty(mCurvesSchema.getArbGeomParams(), ".color", mCurvesSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
}

AlembicCurves::~AlembicCurves()
{
   // we have to clear this prior to destruction
   // this is a workaround for issue-171
   mOVisibility.reset();
}

Abc::OCompoundProperty AlembicCurves::GetCompound()
{
   return mCurvesSchema;
}

bool AlembicCurves::Save(double time, bool bLastFrame)
{
    TimeValue ticks = GET_MAX_INTERFACE()->GetTime();

	Object *obj = GetRef().node->EvalWorldState(ticks).obj;
	if(mNumSamples == 0){
		bForever = CheckIfObjIsValidForever(obj, ticks);
	}
	else{
		bool bNewForever = CheckIfObjIsValidForever(obj, ticks);
		if(bForever && bNewForever != bForever){
			ESS_LOG_INFO( "bForever has changed" );
		}
	}

	bool bFlatten = GetCurrentJob()->GetOption("flattenHierarchy");

    // Store the transformation
    SaveXformSample(GetRef(), mXformSchema, mXformSample, time, bFlatten);

	SaveMetaData(GetRef().node, this);

 
    // set the visibility
    if(!bForever || mNumSamples == 0)
    {
        float flVisibility = GetRef().node->GetLocalVisibility(ticks);
        mOVisibility.set(flVisibility > 0 ? AbcG::kVisibilityVisible : AbcG::kVisibilityHidden);
    }

    // check if the spline is animated
    if(mNumSamples > 0) 
    {
        if(bForever)
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
	std::vector<AbcA::int32_t> nbVertices;
    std::vector<Point3> vertices;
    std::vector<Point3> inTangents;
	std::vector<Point3> outTangents;
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
				inTangents.push_back( in );
				outTangents.push_back( out );
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

    int vertCount = (int)vertices.size();

    // prepare the bounding box
    Abc::Box3d bbox;

    // allocate the points and normals
    std::vector<Abc::V3f> posVec(vertCount);
   Matrix3 wm = GetRef().node->GetObjTMAfterWSM(ticks);

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

    // store the bbox
    mCurvesSample.setSelfBounds(bbox);
	mCurvesSchema.getChildBoundsProperty().set(bbox);

 
    // if we are the first frame!
    Abc::Int32ArraySample nbVerticesSample(&nbVertices.front(),nbVertices.size());
    mCurvesSample.setCurvesNumVertices(nbVerticesSample);

    // set the type + wrapping
	mCurvesSample.setType(bBezier ? AbcG::kCubic : AbcG::kLinear);
    mCurvesSample.setWrap(pShapeObject->CurveClosed(ticks, 0) ? AbcG::kPeriodic : AbcG::kNonPeriodic);
    mCurvesSample.setBasis(AbcG::kNoBasis);
 
    // allocate for the points and normals
    Abc::P3fArraySample posSample(&posVec.front(),posVec.size());
	mCurvesSample.setPositions(posSample);


    mCurvesSchema.set(mCurvesSample);

   mNumSamples++;

   return true;
}
