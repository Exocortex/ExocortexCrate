#include "stdafx.h"
#include "AlembicArchiveStorage.h"
#include "AlembicVisibilityController.h"
#include "AlembicLicensing.h"
#include "AlembicNames.h"
#include "Utility.h"
#include "AlembicNurbsUtilities.h"
#include "AlembicMAXScript.h"
#include "AlembicMetadataUtils.h"
#include <surf_api.h> 


bool isAlembicNurbsTopoDynamic( AbcG::IObject *pIObj ) {
	AbcG::ICurves objCurves;

	if(AbcG::ICurves::matches((*pIObj).getMetaData())) {
		objCurves = AbcG::ICurves(*pIObj,Abc::kWrapExisting);
		return ! objCurves.getSchema().getNumVerticesProperty().isConstant();
	}
	return false;
}

bool LoadNurbs(NURBSSet& nset, Abc::P3fArraySamplePtr pCurvePos, Abc::Int32ArraySamplePtr pCurveNbVertices, Abc::FloatArraySamplePtr pKnotVec, AbcG::CurveType type, AbcG::CurvePeriodicity wrap, TimeValue time )
{
   const int nOrder = 4;
   const int numCurves = (int)pCurveNbVertices->size();
   const int numControl = (int)pCurvePos->size();

   const int abcTotalKnots = numControl + numCurves * (nOrder - 2) ;
   const int maxTotalKnots = numControl + numCurves * nOrder;

   //ESS_LOG_WARNING("abcTotalKnots: "<<abcTotalKnots);
   //ESS_LOG_WARNING("maxTotalKnots: "<<maxTotalKnots);
   //ESS_LOG_WARNING("knotVecSize: "<<pKnotVec->size());

   bool bDefaultKnot = false;
   if(pKnotVec){
      if( abcTotalKnots == pKnotVec->size() ){
         
      }
      else if( maxTotalKnots == pKnotVec->size() ){
         ESS_LOG_WARNING("3DS Max knot vector format not supported. Using Default knot vector.");
         bDefaultKnot = true;
      }
      else{
         ESS_LOG_WARNING("Knot vector has the wrong size. Using Default knot vector.");
         bDefaultKnot = true;      
      }
   }
   else{
      ESS_LOG_WARNING("Knot vector not found. Using Default knot vector.");
      bDefaultKnot = true;
   }

   //bDefaultKnot = true;

   size_t offset = 0;
   size_t abcKnotOffset = 0;
   for(size_t j=0;j<pCurveNbVertices->size();j++)
   {
      LONG nbVertices = (LONG)pCurveNbVertices->get()[j];
	  if( nbVertices == 0 ) {
		  continue;
	  }

      NURBSCVCurve *c = new NURBSCVCurve();
      c->SetNumCVs(nbVertices);
      c->SetOrder(nOrder);

      const int nNumMaxKnots = nbVertices + nOrder;
      const int nNumAbcKnots = nbVertices + nOrder - 2;

      c->SetNumKnots(nNumMaxKnots);
      c->SetName("");

      if(bDefaultKnot){
         
         // based on curve type, we do this for linear or closed cubic curves
         if(type == AbcG::kLinear)
         { 
            c->SetNumKnots(nbVertices+2);
            c->SetOrder(2);
            
            c->SetKnot(0, 0.0);
		    for(LONG k=0; k<nbVertices; k++) {
               c->SetKnot(k+1, k);
		    }
            c->SetKnot(nbVertices+1, nbVertices-1);
            //if(wrap == AbcG::kPeriodic){
            //   c->SetKnot(nNumMaxKnots, nNumMaxKnots);
            //}
         }
         else // cubic open
         {
            c->SetKnot(0, 0.0);
            c->SetKnot(1, 0.0);
            c->SetKnot(2, 0.0);
            for(int i=0; i<nbVertices-2; i++){
               c->SetKnot(3+i, i);
            }
            c->SetKnot(nbVertices+1, nbVertices-3);
            c->SetKnot(nbVertices+2, nbVertices-3);
            c->SetKnot(nbVertices+3, nbVertices-3);
         }
      }
      else{
         c->SetKnot(0, pKnotVec->get()[ abcKnotOffset ]);
         c->SetKnot(nNumMaxKnots-1, pKnotVec->get()[abcKnotOffset + nNumAbcKnots - 1]);
         for(int i=0; i<nNumAbcKnots; i++){
            c->SetKnot(i+1, pKnotVec->get()[abcKnotOffset + i]);
         }
      }

      //ESS_LOG_WARNING("knotVecValues: ");
      //for(int i=0; i<c->GetNumKnots(); i++){
      //   ESS_LOG_WARNING(i<<": "<<c->GetKnot(i));
      //}

      NURBSControlVertex cv;

      for(int i=0; i<nbVertices; i++){
         Point3 pt = ConvertAlembicPointToMaxPoint(pCurvePos->get()[offset + i]);
         cv.SetPosition(time, pt);
         c->SetCV(i, cv);
      }

      offset += nbVertices;
      abcKnotOffset += nNumAbcKnots;

      nset.AppendObject(c);
   }

   return true;
}

bool InitNurbs(NURBSSet& nset )
{
   const int nOrder = 4;
   const int nbVertices = 4;
   NURBSCVCurve *c = new NURBSCVCurve();
   c->SetNumCVs(nbVertices);
   c->SetOrder(nOrder);

   const int nNumKnots = nbVertices + nOrder;

   c->SetNumKnots(nNumKnots);
   c->SetName("");

   NURBSControlVertex cv;
   cv.SetPosition(0, Point3(0.0, 0.0, 0.0));
   c->SetCV(0, cv);
   c->SetCV(1, cv);
   c->SetCV(2, cv);
   c->SetCV(3, cv);

   c->SetKnot(0, 0.0);
   c->SetKnot(1, 0.0);
   c->SetKnot(2, 0.0);
   c->SetKnot(3, 0.0);
   c->SetKnot(4, 1.0);
   c->SetKnot(5, 1.0);
   c->SetKnot(6, 1.0);
   c->SetKnot(7, 1.0);

   nset.AppendObject(c);

   return true;
}

void AlembicImport_LoadNURBS_Internal(alembic_NURBSload_options &options)
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
   obj.getSchema().get(curveSample, sampleInfo.floorIndex);

   //TODO: interpolation
   if(options.pObject && options.pObject->ClassID() == EDITABLE_SURF_CLASS_ID)
   {
         NURBSSet nset;

         Abc::P3fArraySamplePtr pCurvePos = curveSample.getPositions();
         Abc::Int32ArraySamplePtr pCurveNbVertices = curveSample.getCurvesNumVertices();

         Abc::FloatArraySamplePtr pKnotVec;

         if ( obj.getSchema().getArbGeomParams().getPropertyHeader( ".knot_vector" ) != NULL ){
            Abc::IFloatArrayProperty knotProp = Abc::IFloatArrayProperty( obj.getSchema().getArbGeomParams(), ".knot_vector" );
            if(knotProp.valid() && knotProp.getNumSamples() != 0){
               pKnotVec = knotProp.getValue(0);
            }
         }
            
         LoadNurbs(nset, pCurvePos, pCurveNbVertices, pKnotVec, /*AbcG::kLinear, AbcG::kNonPeriodic*/ curveSample.getType(), curveSample.getWrap(), nTicks);

         Matrix3 mat(1);
         Object *newObject = CreateNURBSObject((IObjParam*)GetCOREInterface(), &nset, mat);

         nset.DeleteObjects();

         options.pObject = newObject;
         
   }
}




int AlembicImport_NURBS(const std::string &path, AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode)
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


   // AbcG::ICurvesSchema::Sample curveSample;
   // objCurves.getSchema().get(curveSample, 0);

   //// prepare the curves 
   //Abc::P3fArraySamplePtr pCurvePos = curveSample.getPositions();
   //Abc::Int32ArraySamplePtr pCurveNbVertices = curveSample.getCurvesNumVertices();

   //Abc::FloatArraySamplePtr pKnotVec;

   //if ( objCurves.getSchema().getArbGeomParams().getPropertyHeader( ".knot_vector" ) != NULL ){
   //   Abc::IFloatArrayProperty knotProp = Abc::IFloatArrayProperty( objCurves.getSchema().getArbGeomParams(), ".knot_vector" );
   //   if(knotProp.valid() && knotProp.getNumSamples() != 0){
   //      pKnotVec = knotProp.getValue(0);
   //   }
   //}

   ////It would seem that 3DS Max won't let add a modifier onto an empty NURBS set, so we load the first frame here
   NURBSSet nset;
   InitNurbs(nset);


   // Create the object pNode
	INode *pNode = *pMaxNode;
	bool bReplaceExistingModifiers = false;
	if(!pNode){
        Matrix3 mat(1);

        Object *newObject = CreateNURBSObject((IObjParam*)GetCOREInterface(), &nset, mat);

        nset.DeleteObjects();

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

	TimeValue zero( 0 );

	std::vector<Modifier*> modifiersToEnable;

	bool isDynamicTopo = isAlembicNurbsTopoDynamic( &iObj );

	{
		Modifier *pModifier = NULL;
		bool bCreatedModifier = false;
		if(bReplaceExistingModifiers){
			pModifier = FindModifier(pNode, ALEMBIC_NURBS_MODIFIER_CLASSID, identifier.c_str() );
		}
		if(!pModifier){
			pModifier = static_cast<Modifier*>
				(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_NURBS_MODIFIER_CLASSID));
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
			GET_MAX_INTERFACE()->SelectNode( pNode );
			char szControllerName[10000];
			sprintf_s( szControllerName, 10000, "$.modifiers[#Alembic_NURBS].time" );
			AlembicImport_ConnectTimeControl( szControllerName, options );
        }

		modifiersToEnable.push_back( pModifier );
	}

    // Set the visibility controller
    AlembicImport_SetupVisControl( path.c_str(), identifier.c_str(), iObj, pNode, options);

	for( int i = 0; i < modifiersToEnable.size(); i ++ ) {
		modifiersToEnable[i]->EnableMod();
	}

	if( !FindModifier(pNode, "Alembic Metadata") ){
		GET_MAX_INTERFACE()->SelectNode( pNode );
		importMetadata(iObj);
	}

	return 0;
}