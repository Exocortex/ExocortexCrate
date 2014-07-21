#include "stdafx.h"
#include "Alembic.h"
#include "AlembicCamera.h"
#include "AlembicXForm.h"
#include "SceneEnumProc.h"
#include "Utility.h"
#include "AlembicMetadataUtils.h"


AlembicCamera::AlembicCamera(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent)
: AlembicObject(eNode, in_Job, oParent)
{
    std::string xformName = EC_MCHAR_to_UTF8( mMaxNode->GetName() );
	std::string cameraName = xformName + "Shape";

    AbcG::OCamera camera(GetOParent(), cameraName.c_str(), GetCurrentJob()->GetAnimatedTs());
    mCameraSchema = camera.getSchema();
}

AlembicCamera::~AlembicCamera()
{
}

Abc::OCompoundProperty AlembicCamera::GetCompound()
{
    return mCameraSchema;
}

bool AlembicCamera::Save(double time, bool bLastFrame)
{
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
	bForever = false;

	SaveMetaData(mMaxNode, this);

    // Set the xform sample
    Matrix3 wm = mMaxNode->GetObjTMAfterWSM(ticks);
    if (mJob)
    {
        Point3 worldMaxPoint = wm.GetTrans();
        Abc::V3f alembicWorldPoint = ConvertMaxPointToAlembicPoint(worldMaxPoint);
        mJob->GetArchiveBBox().extendBy(alembicWorldPoint);
    }

    // check if the camera is animated
    if(mNumSamples > 0) 
    {
        if(bForever)
        {
            return true;
        }
    }

    // Return a pointer to a Camera given an INode or return false if the node cannot be converted to a Camera
 
    CameraObject *cam = NULL;

    if (obj->CanConvertToType(Class_ID(SIMPLE_CAM_CLASS_ID, 0)))
    {
        cam = reinterpret_cast<CameraObject *>(obj->ConvertToType(ticks, Class_ID(SIMPLE_CAM_CLASS_ID, 0)));
    }
    else if (obj->CanConvertToType(Class_ID(LOOKAT_CAM_CLASS_ID, 0)))
    {
        cam = reinterpret_cast<CameraObject *>(obj->ConvertToType(ticks, Class_ID(LOOKAT_CAM_CLASS_ID, 0)));
    }
    else
    {
        return false;
    }

    CameraState cs;     
    Interval valid = FOREVER; 
    cam->EvalCameraState(ticks, valid, &cs); 
    float tDist = cam->GetTDist(ticks);
    float ratio = GetCOREInterface()->GetRendImageAspect();
	float aperatureWidth = GetCOREInterface()->GetRendApertureWidth();//this may differ from the imported value unfortunately
	float focalLength = (float)( (aperatureWidth/2.0) / tan(cs.fov/2.0) );//alembic wants this one in millimeters
	aperatureWidth/=10.0f; //convert to centimeters


	IMultiPassCameraEffect* pCameraEffect = cam->GetIMultiPassCameraEffect();

	Interval interval = FOREVER;
	
	BOOL bUseTargetDistance = FALSE;
	const int TARGET_DISTANCE = 0;
	pCameraEffect->GetParamBlockByID(0)->GetValue( TARGET_DISTANCE, ticks, bUseTargetDistance, interval );
	float fFocalDepth = 0.0f;
	const int FOCAL_DEPTH = 1;
	pCameraEffect->GetParamBlockByID(0)->GetValue( FOCAL_DEPTH, ticks, fFocalDepth, interval );


    // store the camera data    
    mCameraSample.setNearClippingPlane(cs.hither);
    mCameraSample.setFarClippingPlane(cs.yon);
    //mCameraSample.setLensSqueezeRatio(ratio);
    
    //should set to 1.0 according the article "Maya to Softimage: Camera Interoperability"
    mCameraSample.setLensSqueezeRatio(1.0);

    mCameraSample.setFocalLength(focalLength);
    mCameraSample.setHorizontalAperture(aperatureWidth);
    mCameraSample.setVerticalAperture(aperatureWidth / ratio);
	if(bUseTargetDistance){
		mCameraSample.setFocusDistance(tDist);
	}
	else{
		mCameraSample.setFocusDistance(fFocalDepth);
	}
	

    // save the samples
    mCameraSchema.set(mCameraSample);

    mNumSamples++;

    // Note that the CamObject should only be deleted if the pointer to it is not
    // equal to the object pointer that called ConvertToType()
    if (cam != NULL && obj != cam)
    {
        delete cam;
        cam = NULL;
        return false;
    }

    return true;
}