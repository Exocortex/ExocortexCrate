#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicCamera.h"
#include "AlembicXForm.h"
#include "SceneEntry.h"
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
    TimeValue ticks = GetTimeValueFromFrame(time);

    // Store the transformation
    SaveCameraXformSample(GetRef(), mXformSchema, mXformSample, time);

    // store the metadata
    // IMetaDataManager mng;
    // mng.GetMetaData(GetRef().node, 0);
    // SaveMetaData(prim.GetParent3DObject().GetRef(),this);

    // set the visibility
    if(GetRef().node->IsAnimated() || mNumSamples == 0)
    {
        mOVisibility.set(GetRef().node->GetLocalVisibility(ticks) > 0 ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);
    }

    // check if the camera is animated
    if(mNumSamples > 0) 
    {
        if(!GetRef().node->IsAnimated())
        {
            return true;
        }
    }


    // Return a pointer to a Camera given an INode or return false if the node cannot be converted to a Camera
    Object *obj = GetRef().node->EvalWorldState(ticks).obj;
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

    // store the camera data    
    mCameraSample.setNearClippingPlane(cs.nearRange);
    mCameraSample.setFarClippingPlane(cs.farRange);
    mCameraSample.setLensSqueezeRatio(ratio);
    mCameraSample.setFocalLength(tDist);
    mCameraSample.setHorizontalAperture(cs.fov);
    mCameraSample.setVerticalAperture(cs.fov / ratio);

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