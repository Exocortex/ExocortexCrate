#include "stdafx.h"
#include "AlembicArchiveStorage.h"
#include "AlembicVisibilityController.h"
#include "AlembicLicensing.h"
#include "AlembicNames.h"
#include "Utility.h"
#include "AlembicSplineUtilities.h"
#include "EmptySplineObject.h"
#include "EmptyPolyLineObject.h"
#include "AlembicMAXScript.h"
#include "AlembicMetadataUtils.h"
#include "AlembicPropertyUtils.h"


bool isAlembicSplinePositions( AbcG::IObject *pIObj, bool& isConstant ) {
	AbcG::ICurves objCurves;

	if(AbcG::ICurves::matches((*pIObj).getMetaData())) {
		objCurves = AbcG::ICurves(*pIObj,Abc::kWrapExisting);
		isConstant = objCurves.getSchema().getPositionsProperty().isConstant();
		return true;
	}

	isConstant = true;
	return false;
}

bool isAlembicSplineTopoDynamic( AbcG::IObject *pIObj ) {
	AbcG::ICurves objCurves;

	if(AbcG::ICurves::matches((*pIObj).getMetaData())) {
		objCurves = AbcG::ICurves(*pIObj,Abc::kWrapExisting);
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


class CurvePositionSampler{
public:

	Abc::P3fArraySamplePtr curvePos1;
	Abc::V3fArraySamplePtr curveVel1;
	Abc::P3fArraySamplePtr curvePos2;

	enum interpT{
		NONE,
		POSITION,
		VELOCITY
	};

	interpT interp;

	float timeAlpha;
	float sampleAlpha;

	CurvePositionSampler(AbcG::ICurves& obj, SampleInfo& sampleInfo, float tAlpha, float sAlpha){

		bool isDynamicTopo = isAlembicSplineTopoDynamic( &obj );

		AbcG::ICurvesSchema::Sample curveSample;
		obj.getSchema().get(curveSample, sampleInfo.floorIndex);
		curvePos1 = curveSample.getPositions();

		this->timeAlpha = tAlpha;
		this->sampleAlpha = sAlpha;

		if(isDynamicTopo){ //interpolate based on velocity
			curveVel1 = curveSample.getVelocities();
			interp = VELOCITY;  
		}
		else{
			AbcG::ICurvesSchema::Sample curveSample2;
			obj.getSchema().get(curveSample2, sampleInfo.ceilIndex);
			curvePos2 = curveSample2.getPositions();

			if(curvePos1->size() == curvePos2->size()){ // interpolate based on positions
				interp = POSITION;
			}
		}

	}

	Abc::V3f operator[](int index){

		Abc::V3f pos1 = curvePos1->get()[index];
		if(sampleAlpha != 0.0 && interp == POSITION){
			return pos1 + ((curvePos2->get()[index] - pos1) * sampleAlpha);
		}
		else if(timeAlpha != 0.0 && interp == VELOCITY){
			return pos1 + (curveVel1->get()[index] * timeAlpha);
		}
		else{
			return pos1;
		}
	}
};



void AlembicImport_FillInShape_Internal(alembic_fillshape_options &options)
{
  ESS_PROFILE_FUNC();
	AbcG::ICurves obj(*options.pIObj,Abc::kWrapExisting);

   if(!obj.valid())
   {
      return;
   }

   int nTicks = options.dTicks;
   float fTimeAlpha = 0.0f;
   if(options.nDataFillFlags & ALEMBIC_DATAFILL_IGNORE_SUBFRAME_SAMPLES){
      RoundTicksToNearestFrame(nTicks, fTimeAlpha);
   }
   double sampleTime = GetSecondsFromTimeValue(nTicks);

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
   /*else
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
   }*/

   AbcG::ICurvesSchema::Sample curveSample;
   obj.getSchema().get(curveSample,sampleInfo.floorIndex);

   // check for valid curve types...!
   if(curveSample.getType() != AbcG::kLinear &&
      curveSample.getType() != AbcG::kCubic)
   {
	   ESS_LOG_ERROR( "Skipping curve '" << options.pIObj->getFullName() << "', invalid curve type." );
      return;
   }

   int curveType = KTYPE_BEZIER;
   int cornerType = KTYPE_BEZIER_CORNER;
   int lineType = LTYPE_CURVE;

  /* if (curveSample.getType() == AbcG::kCubic && !options.pBezierShape)
   {
       return;
   }*/

   if (curveSample.getType() == AbcG::kLinear )
   {
	   curveType = KTYPE_CORNER;
	   cornerType = KTYPE_CORNER;
       lineType = LTYPE_LINE;
   }

   Abc::Int32ArraySamplePtr curveNbVertices = curveSample.getCurvesNumVertices();

   float fSamplerTimeAlpha;
   if(options.nDataFillFlags & ALEMBIC_DATAFILL_IGNORE_SUBFRAME_SAMPLES){
       fSamplerTimeAlpha = fTimeAlpha;
   }
   else{ 
       fSamplerTimeAlpha = getTimeOffsetFromObject( obj, sampleInfo );
   }
   CurvePositionSampler posSampler(obj, sampleInfo, fSamplerTimeAlpha, (float)sampleInfo.alpha);


   // Prepare the knots
   if (options.nDataFillFlags & ALEMBIC_DATAFILL_SPLINE_KNOTS)
   {
       Abc::Int32ArraySamplePtr curveNbVertices = curveSample.getCurvesNumVertices();

       if (options.pBezierShape)
       {
           options.pBezierShape->NewShape();

           for (int i = 0; i < curveNbVertices->size(); i += 1)
           {
               Spline3D *pSpline = options.pBezierShape->NewSpline();
               
               if (curveSample.getWrap() == AbcG::kPeriodic)
                   pSpline->SetClosed();
			
               SplineKnot knot( curveType, lineType, Point3(0,0,0), Point3(0,0,0), Point3(0,0,0) );
			   int nNumKnots = curveNbVertices->get()[i];
			   /*if (curveSample.getType() == AbcG::kCubic ) {
					assert( ( nNumKnots % 3 ) == 0 );
				   nNumKnots /= 3;
			   }			   
			   */

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
			   int startVertex = nVertexOffset;
			   int vertexCount = curveNbVertices->get()[i];

			   nVertexOffset += vertexCount;

			   Spline3D *pSpline = options.pBezierShape->GetSpline(i);
               int knots = pSpline->KnotCount();
			  /* if (curveSample.getType() == AbcG::kCubic ) {
				   assert( 3*knots == vertexCount );
			   }
			   else {
				   assert( knots == vertexCount );
			   }*/

               int kType = curveType;
               for(int j = 0; j < knots; j ++ ) 
               { 

                   Point3 in, p, out;

				  /*if (curveSample.getType() == AbcG::kCubic ) {
					   /*int k = j * 3;
					   in = ConvertAlembicPointToMaxPoint(posSampler[startVertex + k ]); 
					   p = ConvertAlembicPointToMaxPoint(posSampler[startVertex + k + 1 ]); 
					   out = ConvertAlembicPointToMaxPoint(posSampler[startVertex + k + 2 ]); 					
					   * /

					   in = ConvertAlembicPointToMaxPoint(posSampler[startVertex + max( j - 1, 0 )]); 
					   p = ConvertAlembicPointToMaxPoint(posSampler[startVertex + j]); 
					   out = ConvertAlembicPointToMaxPoint(posSampler[startVertex + min( j + 1, knots - 1 )]);                
				   }
				   else {*/
				   in = ConvertAlembicPointToMaxPoint(posSampler[startVertex + std::max( j - 1, 0 )]); 
					   p = ConvertAlembicPointToMaxPoint(posSampler[startVertex + j]); 
					   out = ConvertAlembicPointToMaxPoint(posSampler[startVertex + std::min( j + 1, knots - 1 )]);					
               
					   if( pSpline->Closed() ) {
						   if (j == 0 )
						   {
							   in = ConvertAlembicPointToMaxPoint(posSampler[startVertex + knots - 1]);
						   }
						   else if ( j == knots-1 )
						   {
							   out = ConvertAlembicPointToMaxPoint(posSampler[startVertex + 0]);
						   }                   
					   }
					   else {
						   if (j == 0 )
						   {
							   in = p;
						   }
						   else if ( j == knots-1 )
						   {
							   out = p;
						   }
					   }
				  // }

					   if( curveType == KTYPE_BEZIER ) {
						   Point3 inNew = ( in - out ) * 0.1f + p;
						   Point3 outNew = ( out - in ) * 0.1f + p;
						   in = inNew;
						   out = outNew;
					   }

					if( ! pSpline->Closed() ) {
					   if (j == 0 )
					   {
						   kType = cornerType;
					   }
					   else if ( j == knots-1 )
					   {
						   kType = cornerType;
					   }                   
					}
					pSpline->SetKnot(j, SplineKnot(kType, lineType, p, in, out)); 
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
                   p = ConvertAlembicPointToMaxPoint(posSampler[nVertexOffset]);
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
	AbcG::IObject iObj = getObjectFromArchive(file,identifier);
	if(!iObj.valid())
		return alembic_failure;

    Object *newObject = NULL;
    AlembicSimpleSpline *pAlembicSpline = NULL;
    LinearShape *pAlembicShape = NULL;

    if (!AbcG::ICurves::matches(iObj.getMetaData()))
    {
        return alembic_failure;
    }

    AbcG::ICurves objCurves = AbcG::ICurves(iObj, Abc::kWrapExisting);
    if (!objCurves.valid())
    {
        return alembic_failure;
    }

    AbcG::ICurvesSchema::Sample curveSample;
    objCurves.getSchema().get(curveSample, 0);

    if (curveSample.getType() == AbcG::kCubic)
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


int AlembicImport_Shape(const std::string &path, AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode)
{
   
	const std::string &identifier = iObj.getFullName();

    if (!AbcG::ICurves::matches(iObj.getMetaData()))
    {
        return alembic_failure;
    }

    AbcG::ICurves objCurves = AbcG::ICurves(iObj, Abc::kWrapExisting);
    if (!objCurves.valid())
    {
        return alembic_failure;
    }

	if( objCurves.getSchema().getNumSamples() == 0 ) {
        ESS_LOG_WARNING( "Alembic Curve set has 0 samples, ignoring." );
        return alembic_failure;
	}

    AbcG::ICurvesSchema::Sample curveSample;
    objCurves.getSchema().get(curveSample, 0);


   // Create the object pNode
	INode *pNode = *pMaxNode;
	bool bReplaceExistingModifiers = false;
	if(!pNode){

		Object *newObject = reinterpret_cast<Object*>(GET_MAX_INTERFACE()->CreateInstance(SHAPE_CLASS_ID, EMPTY_SPLINE_OBJECT_CLASSID));
		if (newObject == NULL){
			return alembic_failure;
		}

      Abc::IObject parent = iObj.getParent();
      std::string name = removeXfoSuffix(parent.getName().c_str());
		pNode = GET_MAX_INTERFACE()->CreateObjectNode(newObject, EC_UTF8_to_TCHAR( name.c_str() ) );
		if (pNode == NULL){
			return alembic_failure;
		}
		*pMaxNode = pNode;
	}
	else{
		bReplaceExistingModifiers = true;
	}

   setupPropertyModifiers(iObj, *pMaxNode, std::string("Shape"));

   AbcG::IObject parentXform = iObj.getParent();
   if(parentXform.valid()){
      setupPropertyModifiers(parentXform, *pMaxNode);
   }

	TimeValue zero( 0 );

	std::vector<Modifier*> modifiersToEnable;

	bool isDynamicTopo = isAlembicSplineTopoDynamic( &iObj );

	{
		Modifier *pModifier = NULL;
		bool bCreatedModifier = false;
		if(bReplaceExistingModifiers){
			pModifier = FindModifier(pNode, ALEMBIC_SPLINE_TOPO_MODIFIER_CLASSID, identifier.c_str() );
		}
		if(!pModifier){
			pModifier = static_cast<Modifier*>
				(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_SPLINE_TOPO_MODIFIER_CLASSID));
			bCreatedModifier = true;
		}

		pModifier->DisableMod();

		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, EC_UTF8_to_TCHAR( path.c_str() ) );

		if(bCreatedModifier){
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, EC_UTF8_to_TCHAR( identifier.c_str() ) );
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
		}

		if( isDynamicTopo ) {
            std::stringstream controllerName;
            controllerName<<GET_MAXSCRIPT_NODE(pNode);
            controllerName<<"mynode2113.modifiers[#Alembic_Spline_Topology].time";
			AlembicImport_ConnectTimeControl( controllerName.str().c_str(), options );
		}

		modifiersToEnable.push_back( pModifier );
	}
	bool isGeomContant = true;
	if( ( ! isDynamicTopo ) && isAlembicSplinePositions( &iObj, isGeomContant ) ) {
		//ESS_LOG_INFO( "isGeomContant: " << isGeomContant );

		Modifier *pModifier = NULL;
		bool bCreatedModifier = false;
		if(bReplaceExistingModifiers){
			pModifier = FindModifier(pNode, ALEMBIC_SPLINE_GEOM_MODIFIER_CLASSID, identifier.c_str() );
		}
		if(!pModifier){
			pModifier = static_cast<Modifier*>
				(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_SPLINE_GEOM_MODIFIER_CLASSID));
			bCreatedModifier = true;
		}

		pModifier->DisableMod();

		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, EC_UTF8_to_TCHAR( path.c_str() ) );

		if(bCreatedModifier){
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, EC_UTF8_to_TCHAR( identifier.c_str() ) );
			//pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "geoAlpha" ), zero, 1.0f );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "time" ), zero, 0.0f );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );
		
			// Add the modifier to the pNode
			GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);
		}

		if( ! isGeomContant ) {
            std::stringstream controllerName;
            controllerName<<GET_MAXSCRIPT_NODE(pNode);
            controllerName<<"mynode2113.modifiers[#Alembic_Spline_Geometry].time";
			AlembicImport_ConnectTimeControl( controllerName.str().c_str(), options );
		}

		modifiersToEnable.push_back( pModifier );
	}	

    // Add the new inode to our current scene list
    //SceneEntry *pEntry = options.sceneEnumProc.Append(pNode, newObject, OBTYPE_MESH, &std::string(iObj.getFullName())); 
    //options.currentSceneList.Append(pEntry);

    // Set the visibility controller
    AlembicImport_SetupVisControl( path.c_str(), identifier.c_str(), iObj, pNode, options);

	for( int i = 0; i < modifiersToEnable.size(); i ++ ) {
		modifiersToEnable[i]->EnableMod();
	}

	if( !FindModifier(pNode, "Alembic Metadata") ){
		importMetadata(pNode, iObj);
	}

	return 0;
}