#include "Alembic.h"
#include <inode.h>
#include "SceneEntry.h"
#include "AlembicXform.h"
#include "Dummy.h"
#include "ImathVec.h"
#include "Utility.h"

// namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
// using namespace AbcA;

void GetObjectMatrix(TimeValue ticks, INode *node, Matrix3 &out)
{
    out = node->GetObjTMAfterWSM(ticks);
    INode *pModelTransformParent = GetParentModelTransformNode(node);

    if (pModelTransformParent)
    {
        Matrix3 modelParentTM = pModelTransformParent->GetObjTMAfterWSM(ticks);
        out = out * Inverse(modelParentTM);
    }
}

void SaveXformSample(const SceneEntry &in_Ref, Alembic::AbcGeom::OXformSchema &schema, Alembic::AbcGeom::XformSample &sample, double time)
{
    float masterScaleUnitMeters = (float)GetMasterScale(UNITS_METERS);

	// check if the transform is animated
    if(schema.getNumSamples() > 0)
    {
        if (!CheckIfNodeIsAnimated(in_Ref.node))
        {
            // No need to save transform after first frame for non-animated objects. 
            return;
        }
    }

    // JSS : To validate, I am currently assuming that the ObjectTM is what we are seeking. This may be wrong. 
    // Model transform
    TimeValue ticks = GetTimeValueFromFrame(time);

    Matrix3 transformation;
    GetObjectMatrix(ticks, in_Ref.node, transformation);

    // Convert the max transform to alembic
    Matrix3 alembicMatrix;
    ConvertMaxMatrixToAlembicMatrix(transformation, masterScaleUnitMeters, alembicMatrix);
    Alembic::Abc::M44d iMatrix( alembicMatrix.GetRow(0).x,  alembicMatrix.GetRow(0).y,  alembicMatrix.GetRow(0).z,  0,
                                alembicMatrix.GetRow(1).x,  alembicMatrix.GetRow(1).y,  alembicMatrix.GetRow(1).z,  0,
                                alembicMatrix.GetRow(2).x,  alembicMatrix.GetRow(2).y,  alembicMatrix.GetRow(2).z,  0,
                                alembicMatrix.GetRow(3).x,  alembicMatrix.GetRow(3).y,  alembicMatrix.GetRow(3).z,  1);

    // save the sample
    sample.setMatrix(iMatrix);
    schema.set(sample);
}

void SaveCameraXformSample(const SceneEntry &in_Ref, Alembic::AbcGeom::OXformSchema &schema, Alembic::AbcGeom::XformSample &sample, double time)
{
   // check if the transform is animated
    if(schema.getNumSamples() > 0)
    {
        if (!CheckIfNodeIsAnimated(in_Ref.node))
        {
            // No need to save transform after first frame for non-animated objects. 
            return;
        }
    }

   float masterScaleUnitMeters = (float)GetMasterScale(UNITS_METERS);

   // Model transform
    TimeValue ticks = GetTimeValueFromFrame(time);
    
    Matrix3 transformation;
    GetObjectMatrix(ticks, in_Ref.node, transformation);

    // Cameras in Max are already pointing down the negative z-axis (as is expected from Alembic).
    // So we rotate it by 90 degrees so that it is pointing down the positive y-axis.
    Matrix3 rotation(TRUE);
    rotation.RotateX(-HALFPI);
    transformation = rotation * transformation;

    // Convert the max transform to alembic
    Matrix3 alembicMatrix;
    ConvertMaxMatrixToAlembicMatrix(transformation, masterScaleUnitMeters, alembicMatrix);
    Alembic::Abc::M44d iMatrix( alembicMatrix.GetRow(0).x,  alembicMatrix.GetRow(0).y,  alembicMatrix.GetRow(0).z,  0,
                                alembicMatrix.GetRow(1).x,  alembicMatrix.GetRow(1).y,  alembicMatrix.GetRow(1).z,  0,
                                alembicMatrix.GetRow(2).x,  alembicMatrix.GetRow(2).y,  alembicMatrix.GetRow(2).z,  0,
                                alembicMatrix.GetRow(3).x,  alembicMatrix.GetRow(3).y,  alembicMatrix.GetRow(3).z,  1);

    // save the sample
    sample.setMatrix(iMatrix);
    schema.set(sample);
}

AlembicXForm::AlembicXForm(const SceneEntry &in_Ref, AlembicWriteJob *in_Job) : AlembicObject(in_Ref, in_Job)
{
    std::string xformName = std::string(in_Ref.node->GetName()) + "Xfo";

    Alembic::AbcGeom::OXform xform(GetOParent(), xformName.c_str(), GetCurrentJob()->GetAnimatedTs());

    mXformSchema = xform.getSchema();
}

AlembicXForm::~AlembicXForm()
{
}

bool AlembicXForm::Save(double time)
{
    // Set the bounding box to be used to draw the dummy object on import
    DummyObject *pDummyObject = static_cast<DummyObject*>(GetRef().obj);
    Box3 maxBox = pDummyObject->GetBox();
    Alembic::Abc::V3d minpoint(maxBox.pmin.x, maxBox.pmin.y, maxBox.pmin.z);
    Alembic::Abc::V3d maxpoint(maxBox.pmax.x, maxBox.pmax.y, maxBox.pmax.z);
    mXformSample.setChildBounds(Alembic::Abc::Box3d(minpoint, maxpoint));

    // Store the transformation
    SaveXformSample(GetRef(), mXformSchema, mXformSample, time);

    return true;
}

Alembic::Abc::OCompoundProperty AlembicXForm::GetCompound()
{
    return mXformSchema;
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
