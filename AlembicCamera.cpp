#include "Alembic.h"
#include "AlembicCamera.h"
#include "AlembicXForm.h"
#include "SceneEntry.h"
#include <Object.h>
#include <IMetaData.h>
#include "Utility.h"

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

AlembicCamera::AlembicCamera(const SceneEntry &in_Ref, AlembicWriteJob *in_Job)
: AlembicObject(in_Ref, in_Job)
{
    std::string cameraName = in_Ref.node->GetName();
    std::string xformName = cameraName + "Xfo";

    Alembic::AbcGeom::OXform xform(GetOParent(), xformName.c_str(), GetCurrentJob()->GetAnimatedTs());
    Alembic::AbcGeom::OCamera camera(xform, cameraName.c_str(), GetCurrentJob()->GetAnimatedTs());

    // create the generic properties
    mOVisibility = CreateVisibilityProperty(camera,GetCurrentJob()->GetAnimatedTs());

    mXformSchema = xform.getSchema();
    mCameraSchema = camera.getSchema();
}

AlembicCamera::~AlembicCamera()
{
    // we have to clear this prior to destruction this is a workaround for issue-171
    mOVisibility.reset();
}

Alembic::Abc::OCompoundProperty AlembicCamera::GetCompound()
{
    return mCameraSchema;
}

bool AlembicCamera::Save(double time)
{
    // Store the transformation
    SaveXformSample(GetRef(), mXformSchema, mXformSample, time);

    // store the metadata
    // IMetaDataManager mng;
    // mng.GetMetaData(GetRef().node, 0);
    // SaveMetaData(prim.GetParent3DObject().GetRef(),this);

    // set the visibility
    if(GetRef().node->IsAnimated() || mNumSamples == 0)
    {
        mOVisibility.set(GetRef().node->GetPrimaryVisibility() ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);
    }

    // check if the camera is animated
    // check if the mesh is animated (Otherwise, no need to export)
    if(mNumSamples > 0) 
    {
        if(!GetRef().node->IsAnimated())
        {
            return true;
        }
    }


    // Return a pointer to a TriObject given an INode or return NULL if the node cannot be converted to a TriObject
    TimeValue ticks = GetTimeValueFromSeconds(time);
    Object *obj = GetRef().node->EvalWorldState(ticks).obj;
    CameraObject *cam = NULL;

    if (obj->CanConvertToType(Class_ID(CAMERA_CLASS_ID, 0)))
    {
        cam = (CameraObject *) obj->ConvertToType(ticks, Class_ID(CAMERA_CLASS_ID, 0));

        // Note that the TriObject should only be deleted
        // if the pointer to it is not equal to the object
        // pointer that called ConvertToType()
        if (obj != cam && cam)
        {
            delete cam;
            cam = NULL;
            return false;
        }
    }
    else
    {
        return false;
    }

    CameraState cs;     
    Interval valid = FOREVER; 
    cam->EvalCameraState(ticks, valid, &cs); 

    // store the camera data    
    mCameraSample.setNearClippingPlane(cs.hither);
    mCameraSample.setFarClippingPlane(cs.yon);
    
    Matrix3 camMatrix = GetRef().node->GetObjTMAfterWSM(ticks);
    ViewExp *_pViewport = GetCOREInterface()->GetActiveViewport(); 
    GraphicsWindow *_pGraphicWindow = _pViewport->getGW(); 
   
    // JSS : TODO Validate I am not sure that it is the correct way of getting Aspect Ratio and next parameters
    float ratio = static_cast<float>(_pGraphicWindow->getWinSizeX()) / static_cast<float>(_pGraphicWindow->getWinSizeY());
    mCameraSample.setLensSqueezeRatio(ratio);
    mCameraSample.setFocalLength(_pViewport->GetFocalDist());
    mCameraSample.setHorizontalAperture(cs.fov);
    mCameraSample.setVerticalAperture(cs.fov / ratio);

    // save the samples
    mCameraSchema.set(mCameraSample);

    return true;
}