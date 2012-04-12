#include "AlembicMax.h"
#include "AlembicArchiveStorage.h"
#include "AlembicVisibilityController.h"
#include "AlembicLicensing.h"
#include "AlembicNames.h"
#include "Utility.h"
#include "AlembicSplineUtilities.h"
#include "EmptySplineObject.h"
#include "EmptyPolyLineObject.h"
#include "AlembicMAXScript.h"



bool isAlembicSplinePositions( Alembic::AbcGeom::IObject *pIObj, bool& isConstant ) {
	Alembic::AbcGeom::ICurves objCurves;

	if(Alembic::AbcGeom::ICurves::matches((*pIObj).getMetaData())) {
		objCurves = Alembic::AbcGeom::ICurves(*pIObj,Alembic::Abc::kWrapExisting);
		isConstant = objCurves.getSchema().getPositionsProperty().isConstant();
		return true;
	}

	isConstant = true;
	return false;
}

bool isAlembicSplineTopoDynamic( Alembic::AbcGeom::IObject *pIObj ) {
	Alembic::AbcGeom::ICurves objCurves;

	if(Alembic::AbcGeom::ICurves::matches((*pIObj).getMetaData())) {
		objCurves = Alembic::AbcGeom::ICurves(*pIObj,Alembic::Abc::kWrapExisting);
		return ! objCurves.getSchema().getNumVerticesProperty().isConstant();
	}
	return false;
}


void AlembicImport_FillInShape_Internal(alembic_fillshape_options &options);

void AlembicImport_FillInShape(alembic_fillshape_options &options)
{
	ESS_STRUCTURED_EXCEPTION_REPORTING_START
		AlembicImport_FillInShape_Internal( options );
	ESS_STRUCTURED_EXCEPTION_REPORTING_END
}

void AlembicImport_FillInShape_Internal(alembic_fillshape_options &options)
{
   float masterScaleUnitMeters = (float)GetMasterScale(UNITS_METERS);

   Alembic::AbcGeom::ICurves obj(*options.pIObj,Alembic::Abc::kWrapExisting);

   if(!obj.valid())
   {
      return;
   }

   double sampleTime = GetSecondsFromTimeValue(options.dTicks);

   SampleInfo sampleInfo = getSampleInfo(
      sampleTime,
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );

   // Compute the time interval this fill is good for
   if (sampleInfo.alpha != 0)
   {
       options.validInterval = Interval(options.dTicks, options.dTicks);
   }
   else
   {
       double startSeconds = obj.getSchema().getTimeSampling()->getSampleTime(sampleInfo.floorIndex);
       double endSeconds = obj.getSchema().getTimeSampling()->getSampleTime(sampleInfo.ceilIndex);
       TimeValue start = GetTimeValueFromSeconds(startSeconds);
       TimeValue end = GetTimeValueFromSeconds(endSeconds);
       if (start == 0  && end == 0)
       {
           options.validInterval = FOREVER;
       }
       else
       {
            options.validInterval.Set(start, end);
       }
   }

   Alembic::AbcGeom::ICurvesSchema::Sample curveSample;
   obj.getSchema().get(curveSample,sampleInfo.floorIndex);

   // check for valid curve types...!
   if(curveSample.getType() != Alembic::AbcGeom::ALEMBIC_VERSION_NS::kLinear &&
      curveSample.getType() != Alembic::AbcGeom::ALEMBIC_VERSION_NS::kCubic)
   {
      // Application().LogMessage(L"[ExocortexAlembic] Skipping curve '"+identifier+L"', invalid curve type.",siWarningMsg);
      return;
   }

   if (curveSample.getType() == Alembic::AbcGeom::ALEMBIC_VERSION_NS::kCubic && !options.pBezierShape)
   {
       return;
   }

   if (curveSample.getType() == Alembic::AbcGeom::ALEMBIC_VERSION_NS::kLinear && !options.pPolyShape)
   {
       return;
   }

   Alembic::Abc::Int32ArraySamplePtr curveNbVertices = curveSample.getCurvesNumVertices();
   Alembic::Abc::P3fArraySamplePtr curvePos = curveSample.getPositions();

   // Prepare the knots
   if (options.nDataFillFlags & ALEMBIC_DATAFILL_SPLINE_KNOTS)
   {
       Alembic::Abc::Int32ArraySamplePtr curveNbVertices = curveSample.getCurvesNumVertices();

       if (options.pBezierShape)
       {
           options.pBezierShape->NewShape();

           for (int i = 0; i < curveNbVertices->size(); i += 1)
           {
               Spline3D *pSpline = options.pBezierShape->NewSpline();
               
               if (curveSample.getWrap() == Alembic::AbcGeom::ALEMBIC_VERSION_NS::kPeriodic)
                   pSpline->SetClosed();
			
               SplineKnot knot;
               int nNumKnots = (curveNbVertices->get()[i]+3-1)/3;
               for (int j = 0; j < nNumKnots; j += 1)
                   pSpline->AddKnot(knot);
           }
       }
       else if (options.pPolyShape)
       {
           options.pPolyShape->NewShape();

           for (int i = 0; i < curveNbVertices->size(); i += 1)
           {
               PolyLine *pLine = options.pPolyShape->NewLine();
               int nNumPoints = curveNbVertices->get()[i];
               pLine->SetNumPts(nNumPoints);
           }
       }
   }

   // Set the control points
   if (options.nDataFillFlags & ALEMBIC_DATAFILL_VERTEX)
   {
       if (options.pBezierShape)
       {
           int nVertexOffset = 0;
           Point3 in, p, out;

           for (int i = 0; i < options.pBezierShape->SplineCount(); i +=1)
           {
			   Spline3D *pSpline = options.pBezierShape->GetSpline(i);
               int knots = pSpline->KnotCount();
               int kType;
               for(int ix = 0; ix < knots; ++ix) 
               {
                   if (ix == 0 && !pSpline->Closed())
                   {
                       p = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       out = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       in = p;
                       kType = KTYPE_BEZIER_CORNER;
                   }
                   else if ( ix == knots-1 && !pSpline->Closed())
                   {
                       in = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       p = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       out = p;
                       kType = KTYPE_BEZIER_CORNER;
                   }
                   else
                   {
                       in = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       p = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       out = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       kType = KTYPE_BEZIER;
                   }

                   pSpline->SetKnot(ix, SplineKnot(kType, LTYPE_CURVE, p, in, out)); 
               }

			  pSpline->ComputeBezPoints();
           }
       }
       else if (options.pPolyShape)
       {
           int nVertexOffset = 0;
           Point3 p;

           for (int i = 0; i < options.pPolyShape->numLines; i += 1)
           {
               PolyLine &pLine = options.pPolyShape->lines[i];
               for (int j = 0; j < pLine.numPts; j += 1)
               {
                   p = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters);
                   nVertexOffset += 1;
                   pLine[j].p = p;
               }
           }
       }
   }

   if (options.pBezierShape)
   {
       options.pBezierShape->UpdateSels();
       options.pBezierShape->InvalidateGeomCache();
   }
   else if (options.pPolyShape)
   {
       options.pPolyShape->UpdateSels();
       options.pPolyShape->InvalidateGeomCache(0);
       options.pPolyShape->InvalidateCapCache();
   }

	options.pShapeObject->InvalidateGeomCache();
}

/*
int AlembicImport_Shape(const std::string &file, const std::string &identifier, alembic_importoptions &options)
{
    // Find the object in the archive
	Alembic::AbcGeom::IObject iObj = getObjectFromArchive(file,identifier);
	if(!iObj.valid())
		return alembic_failure;

    Object *newObject = NULL;
    AlembicSimpleSpline *pAlembicSpline = NULL;
    LinearShape *pAlembicShape = NULL;

    if (!Alembic::AbcGeom::ICurves::matches(iObj.getMetaData()))
    {
        return alembic_failure;
    }

    Alembic::AbcGeom::ICurves objCurves = Alembic::AbcGeom::ICurves(iObj, Alembic::Abc::kWrapExisting);
    if (!objCurves.valid())
    {
        return alembic_failure;
    }

    Alembic::AbcGeom::ICurvesSchema::Sample curveSample;
    objCurves.getSchema().get(curveSample, 0);

    if (curveSample.getType() == Alembic::AbcGeom::ALEMBIC_VERSION_NS::kCubic)
    {
        pAlembicSpline = static_cast<AlembicSimpleSpline*>(GET_MAX_INTERFACE()->CreateInstance(SHAPE_CLASS_ID, ALEMBIC_SIMPLE_SPLINE_CLASSID));
	    newObject = pAlembicSpline;
    }
    else
    {
        // PeterM : ToDo: Fill in an alembic linear shape class
        pAlembicShape = 0;
    }

    if (newObject == NULL)
    {
        return alembic_failure;
    }

    // Set the update data fill flags
    unsigned int nDataFillFlags = ALEMBIC_DATAFILL_VERTEX|ALEMBIC_DATAFILL_SPLINE_KNOTS;
    nDataFillFlags |= options.importBboxes ? ALEMBIC_DATAFILL_BOUNDINGBOX : 0;

    // Fill in the alembic object
    if (pAlembicSpline)
    {
        pAlembicSpline->SetAlembicId(file, identifier);
	    pAlembicSpline->SetAlembicUpdateDataFillFlags(nDataFillFlags);
    }
    else if (pAlembicShape)
    {

    }

	// Create the object node
	INode *node = GET_MAX_INTERFACE()->CreateObjectNode(newObject, iObj.getName().c_str());

	if (!node)
    {
        ALEMBIC_SAFE_DELETE(pAlembicSpline);
        ALEMBIC_SAFE_DELETE(pAlembicShape);
		return alembic_failure;
    }

    // Add the new inode to our current scene list
    SceneEntry *pEntry = options.sceneEnumProc.Append(node, newObject, OBTYPE_CURVES, &std::string(iObj.getFullName())); 
    options.currentSceneList.Append(pEntry);

    // Set the visibility controller
    AlembicImport_SetupVisControl(file.c_str(), identifier.c_str(), iObj, node, options);

	return 0;
}*/


int AlembicImport_Shape(const std::string &path, const std::string &identifier, alembic_importoptions &options)
{
   
 	// Find the object in the archive
	Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
	if(!iObj.valid())
	{
		return alembic_failure;
	}

    if (!Alembic::AbcGeom::ICurves::matches(iObj.getMetaData()))
    {
        return alembic_failure;
    }

    Alembic::AbcGeom::ICurves objCurves = Alembic::AbcGeom::ICurves(iObj, Alembic::Abc::kWrapExisting);
    if (!objCurves.valid())
    {
        return alembic_failure;
    }

	if( objCurves.getSchema().getNumSamples() == 0 ) {
        ESS_LOG_WARNING( "Alembic Curve set has 0 samples, ignoring." );
        return alembic_failure;
	}

    Alembic::AbcGeom::ICurvesSchema::Sample curveSample;
    objCurves.getSchema().get(curveSample, 0);

    Object *newObject = NULL;
    if (curveSample.getType() == Alembic::AbcGeom::ALEMBIC_VERSION_NS::kCubic)
    {
		EmptySplineObject *pEmptySplineObject = static_cast<EmptySplineObject*>(GET_MAX_INTERFACE()->CreateInstance(SHAPE_CLASS_ID, EMPTY_SPLINE_OBJECT_CLASSID));
	    newObject = pEmptySplineObject;
    }
    else
    {
		EmptyPolyLineObject *pEmptyPolyLineObject = static_cast<EmptyPolyLineObject*>(GET_MAX_INTERFACE()->CreateInstance(SHAPE_CLASS_ID, EMPTY_POLYLINE_OBJECT_CLASSID));
	    newObject = pEmptyPolyLineObject;
    }

    if (newObject == NULL)
    {
        return alembic_failure;
    }

   // Create the object pNode
	INode *pNode = GET_MAX_INTERFACE()->CreateObjectNode(newObject, iObj.getName().c_str());
	if (pNode == NULL)
    {
		return alembic_failure;
    }

	TimeValue zero( 0 );

	std::vector<Modifier*> modifiersToEnable;

	bool isDynamicTopo = isAlembicSplineTopoDynamic( &iObj );

	{
		// Create the polymesh modifier
		Modifier *pModifier = static_cast<Modifier*>
			(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_SPLINE_TOPO_MODIFIER_CLASSID));

		pModifier->DisableMod();

		// Set the alembic id
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, path.c_str());
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, identifier.c_str() );
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "time" ), zero, 0.0f );
		if( isDynamicTopo ) {
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "geometry" ), zero, TRUE );
		}
		else {
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "geometry" ), zero, FALSE );
		}

		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );
	
		// Add the modifier to the pNode
		GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);

		if( isDynamicTopo ) {
			GET_MAX_INTERFACE()->SelectNode( pNode );
			char szControllerName[10000];
			sprintf_s( szControllerName, 10000, "$.modifiers[#Alembic_Spline_Topology].time" );
			AlembicImport_ConnectTimeControl( szControllerName, options );
		}

		modifiersToEnable.push_back( pModifier );
	}
	bool isGeomContant = true;
	if( ( ! isDynamicTopo ) && isAlembicSplinePositions( &iObj, isGeomContant ) ) {
		//ESS_LOG_INFO( "isGeomContant: " << isGeomContant );
		// Create the polymesh modifier
		Modifier *pModifier = static_cast<Modifier*>
			(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_SPLINE_GEOM_MODIFIER_CLASSID));

		pModifier->DisableMod();

		// Set the alembic id
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, path.c_str());
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, identifier.c_str() );
		//pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "geoAlpha" ), zero, 1.0f );
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "time" ), zero, 0.0f );
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );
	
		// Add the modifier to the pNode
		GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);

		if( ! isGeomContant ) {
			GET_MAX_INTERFACE()->SelectNode( pNode );
			char szControllerName[10000];
			sprintf_s( szControllerName, 10000, "$.modifiers[#Alembic_Spline_Geometry].time" );
			AlembicImport_ConnectTimeControl( szControllerName, options );
		}

		modifiersToEnable.push_back( pModifier );
	}	

    // Add the new inode to our current scene list
    SceneEntry *pEntry = options.sceneEnumProc.Append(pNode, newObject, OBTYPE_MESH, &std::string(iObj.getFullName())); 
    options.currentSceneList.Append(pEntry);

    // Set the visibility controller
    AlembicImport_SetupVisControl( path.c_str(), identifier.c_str(), iObj, pNode, options);

	for( int i = 0; i < modifiersToEnable.size(); i ++ ) {
		modifiersToEnable[i]->EnableMod();
	}

	return 0;
}