#include "stdafx.h"
#include "AlembicXformUtilities.h"
#include "AlembicMAXScript.h"
#include "AlembicMetadataUtils.h"


bool isAlembicXform( AbcG::IObject *pIObj, bool& isConstant ) {
	AbcG::IXform objXfrm;

	isConstant = true; 

	if(AbcG::IXform::matches((*pIObj).getMetaData())) {
		objXfrm = AbcG::IXform(*pIObj,Abc::kWrapExisting);
		if( objXfrm.valid() ) {
			isConstant = objXfrm.getSchema().isConstant();
		} 
	}

	return objXfrm.valid();
}

size_t getNumXformChildren( AbcG::IObject& iObj )
{
	size_t xFormChildCount = 0;
	// An XForm with xForm(s) with an xForm child will require a dummy node (and a single xform can be attached it child geometry node)
	for(size_t j=0; j<iObj.getNumChildren(); j++)
	{
		if(AbcG::IXform::matches(iObj.getChild(j).getMetaData()))
		{
			xFormChildCount++;
		}
	} 
	return xFormChildCount;
}

void AlembicImport_FillInXForm_Internal(alembic_fillxform_options &options);

void AlembicImport_FillInXForm(alembic_fillxform_options &options)
{
	ESS_STRUCTURED_EXCEPTION_REPORTING_START
		AlembicImport_FillInXForm_Internal( options );
	ESS_STRUCTURED_EXCEPTION_REPORTING_END
}

void AlembicImport_FillInXForm_Internal(alembic_fillxform_options &options)
{
  ESS_PROFILE_FUNC();

   if(!options.pIObj->valid())
        return;
    
    IXformPtr pObj;
    {
       ESS_PROFILE_SCOPE("AlembicImport_FillInXForm - IXform obj");
       pObj = options.pObjectCache->getXform();
       if(!pObj){
           return;
       }
       //obj = AbcG::IXform(*(options.pIObj), Abc::kWrapExisting);
    }


    //if(!pObj->valid())
    //    return;

	extern bool g_bVerboseLogging;

	double SampleTime = GetSecondsFromTimeValue(options.dTicks);

	 Abc::M44d matrix;	// constructor creates an identity matrix

	if(g_bVerboseLogging){
		ESS_LOG_INFO("dTicks: "<<options.dTicks<<" sampleTime: "<<SampleTime);
	}

   // if no time samples, default to identity matrix
  if( options.pObjectCache->numSamples > 0 ) {

         SampleInfo sampleInfo;

         {
            ESS_PROFILE_SCOPE("AlembicImport_FillInXForm - getSampleInfo");

		    sampleInfo = getSampleInfo(
			SampleTime,
			pObj->getSchema().getTimeSampling(),
			options.pObjectCache->numSamples
			);
         }

		if(g_bVerboseLogging){
			ESS_LOG_INFO("SampleInfo.alpha: "<<sampleInfo.alpha<< "SampleInfo(fi, ci): "<<sampleInfo.floorIndex<<", "<<sampleInfo.ceilIndex);
		}

         matrix = options.pObjectCache->getXformMatrix(sampleInfo.floorIndex);

		//const Abc::Box3d &box3d = obj.getSchema().getChildBoundsProperty().getValue( sampleInfo.floorIndex );

		//options.maxBoundingBox = Box3(Point3(box3d.min.x, box3d.min.y, box3d.min.z), 
		//	Point3(box3d.max.x, box3d.max.y, box3d.max.z));

		// blend 
		if(sampleInfo.alpha != 0.0)
		{
            ESS_PROFILE_SCOPE("AlembicImport_FillInXForm - blend");

			//pObj->getSchema().get(sample,sampleInfo.ceilIndex);
			Abc::M44d ceilMatrix = options.pObjectCache->getXformMatrix(sampleInfo.ceilIndex);
               //sample.getMatrix();
			matrix = (1.0 - sampleInfo.alpha) * matrix + sampleInfo.alpha * ceilMatrix;
		}
   }

  {
     ESS_PROFILE_SCOPE("AlembicImport_FillInXForm - convert matrix");

    Matrix3 objMatrix(
        Point3(matrix.getValue()[0], matrix.getValue()[1], matrix.getValue()[2]),
        Point3(matrix.getValue()[4], matrix.getValue()[5], matrix.getValue()[6]),
        Point3(matrix.getValue()[8], matrix.getValue()[9], matrix.getValue()[10]),
        Point3(matrix.getValue()[12], matrix.getValue()[13], matrix.getValue()[14]));

    ConvertAlembicMatrixToMaxMatrix(objMatrix, options.maxMatrix);

    if (options.bIsCameraTransform)
    {
        // Cameras in Max are already pointing down the negative z-axis (as is expected from Alembic).
        // So we rotate it by 90 degrees so that it is pointing down the positive y-axis.
        Matrix3 rotation(TRUE);
        rotation.RotateX(HALFPI);
        options.maxMatrix = rotation * options.maxMatrix;
    }

  }
}

int AlembicImport_DummyNode(AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode, const std::string& importName)
{
 ESS_PROFILE_FUNC();
   Object* dObj = static_cast<Object*>(CreateInstance(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID,0)));
	if (!dObj){
		return alembic_failure;
	}

    DummyObject *pDummy = static_cast<DummyObject*>(dObj);

    // Hard code these values for now
    pDummy->SetColor(Point3(0.6f, 0.8f, 1.0f));
    

    double SampleTime = GetSecondsFromTimeValue( GET_MAX_INTERFACE()->GetTime() );

    AbcG::IXform obj(iObj, Abc::kWrapExisting);

	if(!obj.valid()){
		return alembic_failure;
	}

    SampleInfo sampleInfo = getSampleInfo(
        SampleTime,
        obj.getSchema().getTimeSampling(),
        obj.getSchema().getNumSamples()
        );

    AbcG::XformSample sample;
    obj.getSchema().get(sample,sampleInfo.floorIndex);
    Abc::M44d matrix = sample.getMatrix();

	if( obj.getSchema().getChildBoundsProperty().valid() ) {
		const Abc::Box3d &box3d = obj.getSchema().getChildBoundsProperty().getValue( sampleInfo.floorIndex );
      pDummy->SetBox( Box3(  ConvertAlembicPointToMaxPoint(box3d.min), ConvertAlembicPointToMaxPoint(box3d.max)) );
		//pDummy->SetBox( Box3(Point3(box3d.min.x, box3d.min.y, box3d.min.z), Point3(box3d.max.x, box3d.max.y, box3d.max.z)) );
	}

    pDummy->EnableDisplay();

    *pMaxNode = GET_MAX_INTERFACE()->CreateObjectNode(dObj, EC_UTF8_to_TCHAR( importName.c_str() ) );

    SceneEntry *pEntry = options.sceneEnumProc.Append(*pMaxNode, dObj, OBTYPE_DUMMY, &std::string(iObj.getFullName())); 
    options.currentSceneList.Append(pEntry);

	importMetadata(*pMaxNode, iObj);
	
	return alembic_success;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicImport_XForm
///////////////////////////////////////////////////////////////////////////////////////////////////
int AlembicImport_XForm(INode* pParentNode, INode* pMaxNode, AbcG::IObject& iObjXform, AbcG::IObject* p_iObjGeom, const std::string &file, alembic_importoptions &options)
{
  ESS_PROFILE_FUNC();
	const std::string &identifier = iObjXform.getFullName();

	bool isConstant = false;
	if( ! isAlembicXform( &iObjXform, isConstant ) ) {
		return alembic_failure;
	}

    //TODO: this method should take this bool a parameter instead
	bool bIsCamera = p_iObjGeom && p_iObjGeom->valid() && AbcG::ICamera::matches(p_iObjGeom->getMetaData());
	
	TimeValue zero(0);
	if(!isConstant) {

		AlembicXformController *pControl = NULL;		
		bool bCreatedController = false;

		if(options.attachToExisting){
			Animatable* pAnimatable = pMaxNode->SubAnim(2);

			if(pAnimatable && pAnimatable->ClassID() == ALEMBIC_XFORM_CONTROLLER_CLASSID){
				pControl = static_cast<AlembicXformController*>(pAnimatable);

				std::string modIdentifier = EC_MCHAR_to_UTF8( pControl->GetParamBlockByID(0)->GetStr(GetParamIdByName(pControl, 0, "identifier"), zero) );
				if(strcmp(modIdentifier.c_str(), identifier.c_str()) != 0){
					pControl = NULL;
				}
			}
		}
		if(!pControl){
			pControl = static_cast<AlembicXformController*>
				(GetCOREInterface()->CreateInstance(CTRL_MATRIX3_CLASS_ID, ALEMBIC_XFORM_CONTROLLER_CLASSID));
			bCreatedController = true;
		}

		pControl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pControl, 0, "identifier" ), zero, EC_UTF8_to_TCHAR( identifier.c_str() ) );

		if(bCreatedController){
      ESS_PROFILE_SCOPE("AlembicImport_XForm - Setting Control Values");

			pControl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pControl, 0, "path" ), zero, EC_UTF8_to_TCHAR( file.c_str() ) );
			pControl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pControl, 0, "time" ), zero, 0.0f );
			pControl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pControl, 0, "camera" ), zero, ( bIsCamera ? TRUE : FALSE ) );
    	pControl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pControl, 0, "muted" ), zero, TRUE );

   			// Add the modifier to the node
			pMaxNode->SetTMController(pControl);

            std::stringstream controllerName;
            controllerName<<GET_MAXSCRIPT_NODE(pMaxNode);
            controllerName<<"mynode2113.transform.controller.time";
			AlembicImport_ConnectTimeControl( controllerName.str().c_str(), options );

    	pControl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pControl, 0, "muted" ), zero, FALSE );
		}
    }
	else{//if the transform is not animated, do not use a controller. Thus, the user will be able to adjust the object position, orientation and so on.
      ESS_PROFILE_SCOPE("AlembicImport_XForm - Setting Non-Animated Value");
		
		//check if the xform controlller exists, and then delete it
		
		Control* pControl = pMaxNode->GetTMController();
		
		if(pControl){
			MSTR name;
			pControl->GetClassName(name);
			if(strcmp( EC_MSTR_to_UTF8( name ).c_str(), "Position/Rotation/Scale") != 0){
				//TODO: Do I need to be concerned with deleting controllers?
				pMaxNode->SetTMController(CreatePRSControl());
			}
		}

		//doesn't work for some reason
		//Animatable* pAnimatable = pMaxNode->SubAnim(2);
		//if(pAnimatable && pMaxNode->CanDeleteSubAnim(2) ){
		//	pMaxNode->DeleteSubAnim(2);
		//}
		
		alembic_fillxform_options xformOptions;
		xformOptions.pIObj = &iObjXform;
		xformOptions.dTicks = 0;
		xformOptions.bIsCameraTransform = bIsCamera;
    xformOptions.pObjectCache = getObjectCacheFromArchive( file, identifier );
  

		AlembicImport_FillInXForm(xformOptions);

		if(pParentNode != NULL){
			pMaxNode->SetNodeTM(zero, xformOptions.maxMatrix * pParentNode->GetObjectTM(zero));
		}
		else{
			pMaxNode->SetNodeTM(zero, xformOptions.maxMatrix);
		}
	}
    //pMaxNode->InvalidateTreeTM();
	
	if(!isConstant){
		// Lock the transform
		LockNodeTransform(pMaxNode, true);
	}

	return alembic_success;
}


