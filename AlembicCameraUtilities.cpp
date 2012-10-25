#include "stdafx.h"
#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicCameraUtilities.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include "AlembicVisibilityController.h"
#include "AlembicNames.h"
#include "AlembicFloatController.h"
#include "AlembicMAXScript.h"
#include "AlembicMetadataUtils.h"
 
//////////////////////////////////////////////////////////////////////////////////////////
// Import options struct containing the information necessary to fill the camera object
typedef struct _alembic_fillcamera_options
{
public:
    _alembic_fillcamera_options();

    AbcG::IObject  *pIObj;
    GenCamera				*pCameraObj;
    TimeValue                   dTicks;
}
alembic_fillcamera_options;

_alembic_fillcamera_options::_alembic_fillcamera_options()
{
    pIObj = NULL;
    pCameraObj = NULL;
    dTicks = 0;
}

 
//////////////////////////////////////////////////////////////////////////////////////////
// Functions

bool getCameraSampleVal(AbcG::ICamera& objCamera, SampleInfo& sampleInfo, AbcG::CameraSample sample, const char* name, double& sampleVal)
{
    objCamera.getSchema().get(sample, sampleInfo.floorIndex);

	if(boost::iequals(name, "horizontalFOV")){
		double aperature = sample.getHorizontalAperture()*10.0; //convert from cm to mm
		double focalLength = sample.getFocalLength();
		if(focalLength < 0.000001){
			focalLength = 1.0;
		}
		sampleVal = 2.0 * atan(aperature/(2.0*focalLength));
	}
	else if(boost::iequals(name, "verticalFOV")){
		double aperature = sample.getVerticalAperture()*10.0; //convert from cm to mm	
		double focalLength = sample.getFocalLength();
		if(focalLength < 0.000001){
			focalLength = 1.0;
		}
		sampleVal = 2.0 * atan(aperature/(2.0*focalLength));
	}
	else if(boost::iequals(name, "FocalLength")){
		sampleVal = sample.getFocalLength();
	}
	else if(boost::iequals(name, "HorizontalAperture")){
		sampleVal = sample.getHorizontalAperture();
	}
	else if(boost::iequals(name, "HorizontalFilmOffset")){
		sampleVal = sample.getHorizontalFilmOffset();
	}
	else if(boost::iequals(name, "VerticalAperture")){
		sampleVal = sample.getVerticalAperture();
	}
	else if(boost::iequals(name, "VerticalFilmOffset")){
		sampleVal = sample.getVerticalFilmOffset();
	}
	else if(boost::iequals(name, "LensSqueezeRatio")){
		sampleVal = sample.getLensSqueezeRatio();
	}
	else if(boost::iequals(name, "OverScanLeft")){
		sampleVal = sample.getOverScanLeft();
	}
	else if(boost::iequals(name, "OverScanRight")){
		sampleVal = sample.getOverScanRight();
	}
	else if(boost::iequals(name, "OverScanTop")){
		sampleVal = sample.getOverScanTop(); 
	}
	else if(boost::iequals(name, "OverScanBottom")){
		sampleVal = sample.getOverScanBottom();
	}
	else if(boost::iequals(name, "FStop")){
		sampleVal = sample.getFStop();
	}
	else if(boost::iequals(name, "FocusDistance")){
		sampleVal = sample.getFocusDistance();
	}
	else if(boost::iequals(name, "ShutterOpen")){
		sampleVal = sample.getShutterOpen();
	}
	else if(boost::iequals(name, "ShutterClose")){
		sampleVal = sample.getShutterClose();
	}
	else if(boost::iequals(name, "NearClippingPlane")){
		sampleVal = sample.getNearClippingPlane();
	}
	else if(boost::iequals(name, "FarClippingPlane")){
		sampleVal = sample.getFarClippingPlane();
	}
	else{
		return false;
	}

	return true;
}

void AlembicImport_FillInCamera(alembic_fillcamera_options &options)
{
   	ESS_CPP_EXCEPTION_REPORTING_START

	ESS_LOG_INFO( "AlembicImport_FillInCamera" );

	if (options.pCameraObj == NULL ||
        !AbcG::ICamera::matches((*options.pIObj).getMetaData()))
    {
        return;
    }

    AbcG::ICamera objCamera = AbcG::ICamera(*options.pIObj, Abc::kWrapExisting);
    if (!objCamera.valid())
    {
        return;
    }

    double sampleTime = GetSecondsFromTimeValue(options.dTicks);
    SampleInfo sampleInfo = getSampleInfo(sampleTime,
                                          objCamera.getSchema().getTimeSampling(),
                                          objCamera.getSchema().getNumSamples());
    AbcG::CameraSample sample;
    objCamera.getSchema().get(sample, sampleInfo.floorIndex);

    // Extract the camera values from the sample
    //double focalLength = sample.getFocalLength();
    //double fov = sample.getHorizontalAperture();
    //double nearClipping = sample.getNearClippingPlane();
    //double farClipping = sample.getFarClippingPlane();

    // Blend the camera values, if necessary
    //if (sampleInfo.alpha != 0.0)
    //{
    //    objCamera.getSchema().get(sample, sampleInfo.ceilIndex);
    //    focalLength = (1.0 - sampleInfo.alpha) * focalLength + sampleInfo.alpha * sample.getFocalLength();
    //    fov = (1.0 - sampleInfo.alpha) * fov + sampleInfo.alpha * sample.getHorizontalAperture();
    //    nearClipping = (1.0 - sampleInfo.alpha) * nearClipping + sampleInfo.alpha * sample.getNearClippingPlane();
    //    farClipping = (1.0 - sampleInfo.alpha) * farClipping + sampleInfo.alpha * sample.getFarClippingPlane();
    //}

    //options.pCameraObj->SetTDist(options.dTicks, static_cast<float>(focalLength));
   // options.pCameraObj->SetFOVType(0);  // Width FoV = 0
    //options.pCameraObj->SetFOV(options.dTicks, static_cast<float>(fov));
    //options.pCameraObj->SetClipDist(options.dTicks, CAM_HITHER_CLIP, static_cast<float>(nearClipping));
    //options.pCameraObj->SetClipDist(options.dTicks, CAM_YON_CLIP, static_cast<float>(farClipping));

    options.pCameraObj->SetManualClip(TRUE);

	ESS_CPP_EXCEPTION_REPORTING_END
}



AlembicFloatController* createFloatController(const std::string &path, const std::string &identifier, const std::string& prop)
{
    AlembicFloatController *pControl = static_cast<AlembicFloatController*>
        (GetCOREInterface()->CreateInstance(CTRL_FLOAT_CLASS_ID, ALEMBIC_FLOAT_CONTROLLER_CLASSID));

	if(!pControl){
		return NULL;
	}

	TimeValue zero( 0 );

	// Set the alembic id
    pControl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pControl, 0, "path" ), zero, EC_UTF8_to_TCHAR( path.c_str() ) );
	pControl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pControl, 0, "identifier" ), zero, EC_UTF8_to_TCHAR( identifier.c_str() ) );
	pControl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pControl, 0, "property" ), zero, EC_UTF8_to_TCHAR( prop.c_str() ) );
	pControl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pControl, 0, "time" ), zero, 0.0f );
    pControl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pControl, 0, "muted" ), zero, FALSE );

	return pControl;
}

AlembicFloatController* getController(Animatable* pObj, const std::string &identifier, const std::string &camProperty, int i, int j, int k=-1 )
{
	if( i >= pObj->NumSubs() ){
		return NULL;
	}

	Animatable* an = pObj->SubAnim(i);

	if( !an || j >= an->NumSubs() ){
		return NULL;
	}

	an = an->SubAnim(j);

	if( k != -1 ){
		if(!an || k >= an->NumSubs()){
			return NULL;
		}
		an = an->SubAnim(k);
	}

	Class_ID cid = an->ClassID();
	if( !an || an->ClassID() != ALEMBIC_FLOAT_CONTROLLER_CLASSID){
		return NULL;
	}

	AlembicFloatController* pControl = static_cast<AlembicFloatController*>(an);	
	TimeValue zero(0);
	std::string contIdentifier = EC_MCHAR_to_UTF8( pControl->GetParamBlockByID(0)->GetStr(GetParamIdByName(pControl, 0, "identifier"), zero) );
	if(strcmp(contIdentifier.c_str(), identifier.c_str()) != 0){
		return NULL;
	}

	std::string contCamProperty = EC_MCHAR_to_UTF8( pControl->GetParamBlockByID(0)->GetStr(GetParamIdByName(pControl, 0, "property"), zero) );
	if(strcmp(contCamProperty.c_str(), camProperty.c_str()) != 0){
		return NULL;
	}
	
	return pControl;
}

bool assignController(Animatable* controller, Animatable* pObj, int i, int j, int k=-1)
{
	if( i >= pObj->NumSubs() ){
		return false;
	}

	Animatable* an = pObj->SubAnim(i);

	if( !an || j >= an->NumSubs() ){
		return false;
	}
	int aIndex = j;

	if( k != -1 ){
		an = an->SubAnim(j);
		if( k >= an->NumSubs() ){
			return false;
		}
		aIndex = k;
	}

	return an->AssignController(controller, aIndex) == TRUE;
}



int AlembicImport_Camera(const std::string &path, AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode)
{
	const std::string &identifier = iObj.getFullName();

	if (!AbcG::ICamera::matches(iObj.getMetaData())){
        return alembic_failure;
    }
    AbcG::ICamera objCamera = AbcG::ICamera(iObj, Abc::kWrapExisting);
    if (!objCamera.valid()){
        return alembic_failure;
    }
	bool isConstant = objCamera.getSchema().isConstant();

	TimeValue zero(0);

	INode* pNode = *pMaxNode;
	CameraObject* pCameraObj = NULL;
	if(!pNode){
		// Create the camera object and place it in the scene
		GenCamera* pGenCameraObj = GET_MAX_INTERFACE()->CreateCameraObject(FREE_CAMERA);
		if (pGenCameraObj == NULL){
			return alembic_failure;
		}
		pGenCameraObj->Enable(TRUE);
		pGenCameraObj->SetConeState(TRUE);
		pGenCameraObj->SetManualClip(TRUE);

		IMultiPassCameraEffect* pCameraEffect = pGenCameraObj->GetIMultiPassCameraEffect();
		const int TARGET_DISTANCE = 0;
		pCameraEffect->GetParamBlockByID(0)->SetValue( TARGET_DISTANCE, zero, FALSE );

		pCameraObj = pGenCameraObj;

		pNode = GET_MAX_INTERFACE()->CreateObjectNode(pGenCameraObj, EC_UTF8_to_TCHAR( iObj.getName().c_str() ) );
		if (pNode == NULL){
			return alembic_failure;
		}
		*pMaxNode = pNode;
	}
	else{
		Object *obj = pNode->EvalWorldState(zero).obj;

		if (obj->CanConvertToType(Class_ID(SIMPLE_CAM_CLASS_ID, 0))){
			pCameraObj = reinterpret_cast<CameraObject *>(obj->ConvertToType(zero, Class_ID(SIMPLE_CAM_CLASS_ID, 0)));
		}
		else if (obj->CanConvertToType(Class_ID(LOOKAT_CAM_CLASS_ID, 0))){
			pCameraObj = reinterpret_cast<CameraObject *>(obj->ConvertToType(zero, Class_ID(LOOKAT_CAM_CLASS_ID, 0)));
		}
		else{
			return alembic_failure;
		}
	}

     //Fill in the mesh
 //   alembic_fillcamera_options dataFillOptions;
 //   dataFillOptions.pIObj = &iObj;
 //   dataFillOptions.pCameraObj = pCameraObj;
 //   dataFillOptions.dTicks =  GET_MAX_INTERFACE()->GetTime();
	//AlembicImport_FillInCamera(dataFillOptions);

	//printAnimatables(pCameraObj);
	
	Interval interval = FOREVER;
	
	GET_MAX_INTERFACE()->SelectNode( pNode );

	AlembicFloatController* pControl = NULL;
	{
		std::string prop("horizontalFOV");
		if(options.attachToExisting){
			pControl = getController(pCameraObj, identifier, prop, 0, 0);
		}
		if(pControl){
			pControl->GetParamBlockByID(0)->SetValue( GetParamIdByName( pControl, 0, "path" ), zero, EC_UTF8_to_TCHAR( path.c_str() ) );
		}
		else if(assignController(createFloatController(path, identifier, prop), pCameraObj, 0, 0) && !isConstant){
			AlembicImport_ConnectTimeControl("$.FOV.controller.time", options );
		}
	}
	{
		std::string prop("FocusDistance");
		if(options.attachToExisting){
			pControl = getController(pCameraObj, identifier, prop, 1, 0, 1);
		}
		if(pControl){
			pControl->GetParamBlockByID(0)->SetValue( GetParamIdByName( pControl, 0, "path" ), zero, EC_UTF8_to_TCHAR( path.c_str() ) );
		}
		else if(assignController(createFloatController(path, identifier, prop), pCameraObj, 1, 0, 1) && !isConstant){
			AlembicImport_ConnectTimeControl("$.MultiPass_Effect.focalDepth.controller.time", options );
		}
	}
	{
		std::string prop("NearClippingPlane");
		if(options.attachToExisting){
			pControl = getController(pCameraObj, identifier, prop, 0, 2);
		}
		if(pControl){
			pControl->GetParamBlockByID(0)->SetValue( GetParamIdByName( pControl, 0, "path" ), zero, EC_UTF8_to_TCHAR( path.c_str() ) );
		}
		else if(assignController(createFloatController(path, identifier, prop), pCameraObj, 0, 2) && !isConstant){
			AlembicImport_ConnectTimeControl("$.nearclip.controller.time", options );
		}
	}
	{
		std::string prop("FarClippingPlane");
		if(options.attachToExisting){
			pControl = getController(pCameraObj, identifier, prop, 0, 3);
		}
		if(pControl){
			pControl->GetParamBlockByID(0)->SetValue( GetParamIdByName( pControl, 0, "path" ), zero, EC_UTF8_to_TCHAR( path.c_str() ) );
		}
		else if(assignController(createFloatController(path, identifier, prop), pCameraObj, 0, 3) && !isConstant){
			AlembicImport_ConnectTimeControl("$.farclip.controller.time", options );
		}
	}

	//if(assignControllerToLevel1SubAnim(createFloatController(path, identifier, std::string("FocusDistance")), pCameraObj, 0, 1) && !isConstant){
	//	AlembicImport_ConnectTimeControl( "$.targetDistance.controller.time", options );
	//}	

	////Create the Camera modifier
	Modifier *pModifier = static_cast<Modifier*>
		(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_CAMERA_MODIFIER_CLASSID));

	TimeValue now =  GET_MAX_INTERFACE()->GetTime();

	//Set the alembic id
	pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), now, EC_UTF8_to_TCHAR( path.c_str() ) );
	pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), now, EC_UTF8_to_TCHAR( identifier.c_str() ) );
	
	//Set the alembic id
	//pModifier->SetCamera(pCameraObj);

	//Add the modifier to the node
	GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);

   //AlembicImport_ConnectTimeControl("$.modifiers[\"AlembicCameraProperties\"].time", options );

   //printControllers(pModifier->GetParamBlockByID(0));

   IParamBlock2* pBlock = pModifier->GetParamBlockByID(0);
   if(pBlock){
      pBlock->AssignController(createFloatController(path, identifier, "horizontalFOV"), 0);
      pBlock->AssignController(createFloatController(path, identifier, "verticalFOV"), 1);
      pBlock->AssignController(createFloatController(path, identifier, "FocalLength"), 2);
      pBlock->AssignController(createFloatController(path, identifier, "HorizontalAperture"), 3);
      pBlock->AssignController(createFloatController(path, identifier, "VerticalAperture"), 4);
      pBlock->AssignController(createFloatController(path, identifier, "HorizontalFilmOffset"), 5);
      pBlock->AssignController(createFloatController(path, identifier, "VerticalFilmOffset"), 6);
      pBlock->AssignController(createFloatController(path, identifier, "LensSqueezeRatio"), 7);
      pBlock->AssignController(createFloatController(path, identifier, "OverScanLeft"), 8);
      pBlock->AssignController(createFloatController(path, identifier, "OverScanRight"), 9);
      pBlock->AssignController(createFloatController(path, identifier, "OverScanTop"), 10);
      pBlock->AssignController(createFloatController(path, identifier, "OverScanBottom"), 11);
      pBlock->AssignController(createFloatController(path, identifier, "FStop"), 12);
      pBlock->AssignController(createFloatController(path, identifier, "FocusDistance"), 13);
      pBlock->AssignController(createFloatController(path, identifier, "ShutterOpen"), 14);
      pBlock->AssignController(createFloatController(path, identifier, "ShutterClose"), 15);
      pBlock->AssignController(createFloatController(path, identifier, "NearClippingPlane"), 16);
      pBlock->AssignController(createFloatController(path, identifier, "FarClippingPlane"), 17);
   }

    // Add the new inode to our current scene list
    SceneEntry *pEntry = options.sceneEnumProc.Append(pNode, pCameraObj, OBTYPE_CAMERA, &std::string(iObj.getFullName())); 
    options.currentSceneList.Append(pEntry);

    // Set the visibility controller
    AlembicImport_SetupVisControl( path, identifier, iObj, pNode, options);

	GET_MAX_INTERFACE()->SelectNode( pNode );
	importMetadata(iObj);

	return 0;
}
