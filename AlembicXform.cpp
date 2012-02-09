#include "AlembicXform.h"

#include <maya/MFnTransform.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

AlembicXform::AlembicXform(const MObject & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   MFnDependencyNode node(in_Ref);
   MString xformName = node.name()+"Xfo";
   Alembic::AbcGeom::OXform xform(GetOParent(),xformName.asChar(),GetJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();
}

AlembicXform::~AlembicXform()
{
}

Alembic::Abc::OCompoundProperty AlembicXform::GetCompound()
{
   return mXformSchema;
}

MStatus AlembicXform::Save(double time)
{
   // access the xform
   MFnTransform node(GetRef());

   // TODO: implement storage of metadata
   // SaveMetaData(prim.GetParent3DObject().GetRef(),this);

   Alembic::AbcGeom::XformSample sample;

   MTransformationMatrix xf = node.transformation();
   MMatrix matrix = xf.asMatrix();
   Alembic::Abc::M44d abcMatrix;
   matrix.get(abcMatrix.x);
   sample.setMatrix(abcMatrix);

   // save the sample
   mXformSchema.set(sample);

   return MStatus::kSuccess;
}

MObject AlembicXformNode::mTimeAttr;
MObject AlembicXformNode::mFileNameAttr;
MObject AlembicXformNode::mIdentifierAttr;
MObject AlembicXformNode::mOutTransformAttr;

MStatus AlembicXformNode::initialize()
{
    MStatus status;

    MFnUnitAttribute uAttr;
    MFnTypedAttribute tAttr;
    MFnNumericAttribute nAttr;
    MFnGenericAttribute gAttr;
    MFnStringData emptyStringData;
    MObject emptyStringObject = emptyStringData.create("");
    MFnMatrixData identityMatrixData;
    MTransformationMatrix identityMatrix;
    MObject identityMatrixObject = identityMatrixData.create(MTransformationMatrix::identity);

    // input time
    mTimeAttr = uAttr.create("time", "tm", MFnUnitAttribute::kTime, 0.0);
    status = uAttr.setStorable(true);
    status = addAttribute(mTimeAttr);

    // input file name
    mFileNameAttr = tAttr.create("fileName", "fn", MFnData::kString, emptyStringObject);
    status = tAttr.setStorable(true);
    status = tAttr.setUsedAsFilename(true);
    status = addAttribute(mFileNameAttr);

    // input identifier
    mIdentifierAttr = tAttr.create("identifier", "it", MFnData::kString, emptyStringObject);
    status = tAttr.setStorable(true);
    status = addAttribute(mIdentifierAttr);

    // output transform
    mOutTransformAttr = tAttr.create("transform", "xf", MFnData::kMatrix, identityMatrixObject);
    status = tAttr.setStorable(false);
    status = tAttr.setWritable(false);
    status = tAttr.setKeyable(false);
    status = addAttribute(mOutTransformAttr);

    // create a mapping
    status = attributeAffects(mTimeAttr, mOutTransformAttr);
    status = attributeAffects(mFileNameAttr, mOutTransformAttr);
    status = attributeAffects(mIdentifierAttr, mOutTransformAttr);

    return status;
}

MStatus AlembicXformNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
    MStatus status;

    // update the frame number to be imported
    MDataHandle timeHandle = dataBlock.inputValue(mTimeAttr, &status);
    MTime t = timeHandle.asTime();
    double inputTime = t.as(MTime::kSeconds);

    // TODO: implement the actual alembic pull

    return status;
}
