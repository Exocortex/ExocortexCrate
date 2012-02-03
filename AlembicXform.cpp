#include "Foundation.h"
#include <inode.h>
#include "SceneEntry.h"
#include "AlembicXform.h"

// namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
// using namespace AbcA;

void SaveXformSample(const SceneEntry &in_Ref, Alembic::AbcGeom::OXformSchema &schema, Alembic::AbcGeom::XformSample &sample, double time)
{
    // check if the transform is animated
    if(schema.getNumSamples() > 0)
    {
        if (!in_Ref.node->IsAnimated())
        {
            // No need to save transform after first frame for non-animated objects. 
            return;
        }
    }

    // Is there a global transform?

    // JSS : To validate, I am currently assuming that the ObjectTM is what we are seeking. This may be wrong. 
    // Model transform
    Matrix3 transformation = in_Ref.node->GetObjectTM(static_cast<TimeValue>(time));

    // store the transform
    sample.setTranslation(Imath::V3d(transformation.GetTrans().x,transformation.GetTrans().y,transformation.GetTrans().z));

    float rotX = 0.0f;
    float rotY = 0.0f;
    float rotZ = 0.0f;
    transformation.GetYawPitchRoll(&rotY, &rotX, &rotZ);
    sample.setRotation(Imath::V3d(1,0,0), rotX * 180.0 / M_PI);
    sample.setRotation(Imath::V3d(0,1,0), rotY * 180.0 / M_PI);
    sample.setRotation(Imath::V3d(0,0,1), rotZ * 180.0 / M_PI);

    // JSS: TODO Get Rotation and Scale
    sample.setScale(Imath::V3d(1,1,1));

    // save the sample
    schema.set(sample);
}

/*
JSS - This is for importer
bool alembic_xform_Define( CRef& in_ctxt )
{
    return alembicOp_Define(in_ctxt);
}

bool alembic_xform_DefineLayout( CRef& in_ctxt )
{
    return alembicOp_DefineLayout(in_ctxt);
}


bool alembic_xform_Update( CRef& in_ctxt )
{
    OperatorContext ctxt( in_ctxt );

    if((bool)ctxt.GetParameterValue(L"muted"))
        return CStatus::OK;

    CString path = ctxt.GetParameterValue(L"path");
    CString identifier = ctxt.GetParameterValue(L"identifier");

    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
    if(!iObj.valid())
        return CStatus::OK;
    Alembic::AbcGeom::IXform obj(iObj,Alembic::Abc::kWrapExisting);
    if(!obj.valid())
        return CStatus::OK;

    SampleInfo sampleInfo = getSampleInfo(
        ctxt.GetParameterValue(L"time"),
        obj.getSchema().getTimeSampling(),
        obj.getSchema().getNumSamples()
        );

    Alembic::AbcGeom::XformSample sample;
    obj.getSchema().get(sample,sampleInfo.floorIndex);
    Alembic::Abc::M44d matrix = sample.getMatrix();

    // blend
    if(sampleInfo.alpha != 0.0)
    {
        obj.getSchema().get(sample,sampleInfo.ceilIndex);
        Alembic::Abc::M44d ceilMatrix = sample.getMatrix();
        matrix = (1.0 - sampleInfo.alpha) * matrix + sampleInfo.alpha * ceilMatrix;
    }

    CMatrix4 xsiMatrix;
    xsiMatrix.Set(
        matrix.getValue()[0],matrix.getValue()[1],matrix.getValue()[2],matrix.getValue()[3],
        matrix.getValue()[4],matrix.getValue()[5],matrix.getValue()[6],matrix.getValue()[7],
        matrix.getValue()[8],matrix.getValue()[9],matrix.getValue()[10],matrix.getValue()[11],
        matrix.getValue()[12],matrix.getValue()[13],matrix.getValue()[14],matrix.getValue()[15]);
    CTransformation xsiTransform;
    xsiTransform.SetMatrix4(xsiMatrix);

    KinematicState state(ctxt.GetOutputTarget());
    state.PutTransform(xsiTransform);

    return CStatus::OK;
}

bool alembic_xform_Term(CRef & in_ctxt)
{
    Context ctxt( in_ctxt );
    CustomOperator op(ctxt.GetSource());
    delRefArchive(op.GetParameterValue(L"path").GetAsText());
    return CStatus::OK;
}

bool alembic_visibility_Define( CRef& in_ctxt )
{
    return alembicOp_Define(in_ctxt);
}

bool alembic_visibility_DefineLayout( CRef& in_ctxt )
{
    return alembicOp_DefineLayout(in_ctxt);
}

bool alembic_visibility_Update( CRef& in_ctxt )
{
    OperatorContext ctxt( in_ctxt );

    if((bool)ctxt.GetParameterValue(L"muted"))
        return CStatus::OK;

    CString path = ctxt.GetParameterValue(L"path");
    CString identifier = ctxt.GetParameterValue(L"identifier");

    Alembic::AbcGeom::IObject obj = getObjectFromArchive(path,identifier);
    if(!obj.valid())
        return CStatus::OK;

    Alembic::AbcGeom::IVisibilityProperty visibilityProperty = 
        Alembic::AbcGeom::GetVisibilityProperty(obj);
    if(!visibilityProperty.valid())
        return CStatus::OK;

    SampleInfo sampleInfo = getSampleInfo(
        ctxt.GetParameterValue(L"time"),
        getTimeSamplingFromObject(obj),
        visibilityProperty.getNumSamples()
        );

    int8_t rawVisibilityValue = visibilityProperty.getValue ( sampleInfo.floorIndex );
    Alembic::AbcGeom::ObjectVisibility visibilityValue = Alembic::AbcGeom::ObjectVisibility ( rawVisibilityValue );

    Property prop(ctxt.GetOutputTarget());
    switch(visibilityValue)
    {
    case Alembic::AbcGeom::kVisibilityVisible:
        {
            prop.PutParameterValue(L"viewvis",true);
            prop.PutParameterValue(L"rendvis",true);
            break;
        }
    case Alembic::AbcGeom::kVisibilityHidden:
        {
            prop.PutParameterValue(L"viewvis",false);
            prop.PutParameterValue(L"rendvis",false);
            break;
        }
    default:
        {
            break;
        }
    }

    return CStatus::OK;
}

bool alembic_visibility_Term(CRef & in_ctxt)
{
    return alembicOp_Term(in_ctxt);
}
*/
