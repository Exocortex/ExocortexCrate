#include "AlembicXform.h"
#include "MetaData.h"

#include <maya/MFnTransform.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

AlembicXform::AlembicXform(const MObject & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   MFnDependencyNode node(in_Ref);
   MString name = truncateName(node.name())+"Xfo";
   mObject = Alembic::AbcGeom::OXform(GetParentObject(),name.asChar(),GetJob()->GetAnimatedTs());

   mSchema = mObject.getSchema();
}

AlembicXform::~AlembicXform()
{
   mObject.reset();
   mSchema.reset();
}

MStatus AlembicXform::Save(double time)
{
   // access the xform
   MFnTransform node(GetRef());

   // save the metadata
   SaveMetaData(this);

   MTransformationMatrix xf = node.transformation();
   MMatrix matrix = xf.asMatrix();
   Alembic::Abc::M44d abcMatrix;
   matrix.get(abcMatrix.x);
   mSample.setMatrix(abcMatrix);
   mSample.setInheritsXforms(true);

   // save the sample
   mSchema.set(mSample);
   mNumSamples++;

   return MStatus::kSuccess;
}

void AlembicXformNode::PreDestruction()
{
   mSchema.reset();
   delRefArchive(mFileName);
   mFileName.clear();
}

AlembicXformNode::~AlembicXformNode()
{
   PreDestruction();
}

MObject AlembicXformNode::mTimeAttr;
MObject AlembicXformNode::mFileNameAttr;
MObject AlembicXformNode::mIdentifierAttr;
MObject AlembicXformNode::mOutTranslateXAttr;
MObject AlembicXformNode::mOutTranslateYAttr;
MObject AlembicXformNode::mOutTranslateZAttr;
MObject AlembicXformNode::mOutTranslateAttr;
MObject AlembicXformNode::mOutRotateXAttr;
MObject AlembicXformNode::mOutRotateYAttr;
MObject AlembicXformNode::mOutRotateZAttr;
MObject AlembicXformNode::mOutRotateAttr;
MObject AlembicXformNode::mOutScaleXAttr;
MObject AlembicXformNode::mOutScaleYAttr;
MObject AlembicXformNode::mOutScaleZAttr;
MObject AlembicXformNode::mOutScaleAttr;

MStatus AlembicXformNode::initialize()
{
   MStatus status;

   MFnUnitAttribute uAttr;
   MFnTypedAttribute tAttr;
   MFnNumericAttribute nAttr;
   MFnGenericAttribute gAttr;
   MFnStringData emptyStringData;
   MObject emptyStringObject = emptyStringData.create("");

   // input time
   mTimeAttr = uAttr.create("inTime", "tm", MFnUnitAttribute::kTime, 0.0);
   status = uAttr.setStorable(true);
   status = uAttr.setKeyable(true);
   status = addAttribute(mTimeAttr);

   // input file name
   mFileNameAttr = tAttr.create("fileName", "fn", MFnData::kString, emptyStringObject);
   status = tAttr.setStorable(true);
   status = tAttr.setUsedAsFilename(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mFileNameAttr);

   // input identifier
   mIdentifierAttr = tAttr.create("identifier", "it", MFnData::kString, emptyStringObject);
   status = tAttr.setStorable(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mIdentifierAttr);

   // output translateX
   mOutTranslateXAttr = nAttr.create("translateX", "tx", MFnNumericData::kDouble, 0.0);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = nAttr.setHidden(false);

   // output translateY
   mOutTranslateYAttr = nAttr.create("translateY", "ty", MFnNumericData::kDouble, 0.0);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = nAttr.setHidden(false);

   // output translateY
   mOutTranslateZAttr = nAttr.create("translateZ", "tz", MFnNumericData::kDouble, 0.0);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = nAttr.setHidden(false);

   // output translate compound
   mOutTranslateAttr = nAttr.create( "translate", "t", mOutTranslateXAttr, mOutTranslateYAttr, mOutTranslateZAttr);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = nAttr.setHidden(false);
   status = addAttribute(mOutTranslateAttr);

   // output rotatex
   mOutRotateXAttr = uAttr.create("rotateX", "rx", MFnUnitAttribute::kAngle, 0.0);
   status = uAttr.setStorable(false);
   status = uAttr.setWritable(false);
   status = uAttr.setKeyable(false);
   status = uAttr.setHidden(false);

   // output rotatexy
   mOutRotateYAttr = uAttr.create("rotateY", "ry", MFnUnitAttribute::kAngle, 0.0);
   status = uAttr.setStorable(false);
   status = uAttr.setWritable(false);
   status = uAttr.setKeyable(false);
   status = uAttr.setHidden(false);

   // output rotatez
   mOutRotateZAttr = uAttr.create("rotateZ", "rz", MFnUnitAttribute::kAngle, 0.0);
   status = uAttr.setStorable(false);
   status = uAttr.setWritable(false);
   status = uAttr.setKeyable(false);
   status = uAttr.setHidden(false);

   // output translate compound
   mOutRotateAttr = nAttr.create( "rotate", "r", mOutRotateXAttr, mOutRotateYAttr, mOutRotateZAttr);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = nAttr.setHidden(false);
   status = addAttribute(mOutRotateAttr);

   // output scalex
   mOutScaleXAttr = nAttr.create("scaleX", "sx", MFnNumericData::kDouble, 1.0);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = nAttr.setHidden(false);

   // output scaley
   mOutScaleYAttr = nAttr.create("scaleY", "sy", MFnNumericData::kDouble, 1.0);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = nAttr.setHidden(false);

   // output scalez
   mOutScaleZAttr = nAttr.create("scaleZ", "sz", MFnNumericData::kDouble, 1.0);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = nAttr.setHidden(false);

   // output translate compound
   mOutScaleAttr = nAttr.create( "scale", "s", mOutScaleXAttr, mOutScaleYAttr, mOutScaleZAttr);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = nAttr.setHidden(false);
   status = addAttribute(mOutScaleAttr);

   // create a mapping
   status = attributeAffects(mTimeAttr, mOutTranslateXAttr);
   status = attributeAffects(mFileNameAttr, mOutTranslateXAttr);
   status = attributeAffects(mIdentifierAttr, mOutTranslateXAttr);
   status = attributeAffects(mTimeAttr, mOutTranslateYAttr);
   status = attributeAffects(mFileNameAttr, mOutTranslateYAttr);
   status = attributeAffects(mIdentifierAttr, mOutTranslateYAttr);
   status = attributeAffects(mTimeAttr, mOutTranslateZAttr);
   status = attributeAffects(mFileNameAttr, mOutTranslateZAttr);
   status = attributeAffects(mIdentifierAttr, mOutTranslateZAttr);
   status = attributeAffects(mTimeAttr, mOutRotateXAttr);
   status = attributeAffects(mFileNameAttr, mOutRotateXAttr);
   status = attributeAffects(mIdentifierAttr, mOutRotateXAttr);
   status = attributeAffects(mTimeAttr, mOutRotateYAttr);
   status = attributeAffects(mFileNameAttr, mOutRotateYAttr);
   status = attributeAffects(mIdentifierAttr, mOutRotateYAttr);
   status = attributeAffects(mTimeAttr, mOutRotateZAttr);
   status = attributeAffects(mFileNameAttr, mOutRotateZAttr);
   status = attributeAffects(mIdentifierAttr, mOutRotateZAttr);
   status = attributeAffects(mTimeAttr, mOutScaleXAttr);
   status = attributeAffects(mFileNameAttr, mOutScaleXAttr);
   status = attributeAffects(mIdentifierAttr, mOutScaleXAttr);
   status = attributeAffects(mTimeAttr, mOutScaleYAttr);
   status = attributeAffects(mFileNameAttr, mOutScaleYAttr);
   status = attributeAffects(mIdentifierAttr, mOutScaleYAttr);
   status = attributeAffects(mTimeAttr, mOutScaleZAttr);
   status = attributeAffects(mFileNameAttr, mOutScaleZAttr);
   status = attributeAffects(mIdentifierAttr, mOutScaleZAttr);

   return status;
}

MStatus AlembicXformNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   MStatus status;

   // update the frame number to be imported
   double inputTime = dataBlock.inputValue(mTimeAttr).asTime().as(MTime::kSeconds);
   MString & fileName = dataBlock.inputValue(mFileNameAttr).asString();
   MString & identifier = dataBlock.inputValue(mIdentifierAttr).asString();

   // check if we have the file
   if(fileName != mFileName || identifier != mIdentifier)
   {
      mSchema.reset();
      if(fileName != mFileName)
      {
         delRefArchive(mFileName);
         mFileName = fileName;
         addRefArchive(mFileName);
      }
      mIdentifier = identifier;

      // get the object from the archive
      Alembic::Abc::IObject iObj = getObjectFromArchive(mFileName,identifier);
      if(!iObj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' not found in archive '"+mFileName+"'.");
         return MStatus::kFailure;
      }
      Alembic::AbcGeom::IXform obj(iObj,Alembic::Abc::kWrapExisting);
      if(!obj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' in archive '"+mFileName+"' is not a Xform.");
         return MStatus::kFailure;
      }
      mSchema = obj.getSchema();

      if(!mSchema.valid())
         return MStatus::kFailure;

      Alembic::AbcGeom::XformSample sample;
      mLastFloor = 0;
      mTimes.clear();
      mMatrices.clear();
      for(size_t i=0;i<mSchema.getNumSamples();i++)
      {
         mTimes.push_back((double)mSchema.getTimeSampling()->getStoredTimes()[i]);
         mSchema.get(sample,i);
         mMatrices.push_back(sample.getMatrix());
      }
   }

   // find the index
   size_t index = mLastFloor;
   while(inputTime > mTimes[index] && index < mTimes.size()-1)
      index++;
   while(inputTime < mTimes[index] && index > 0)
      index--;

   Alembic::Abc::M44d matrix;
   if(fabs(time - mTimes[index]) < 0.001 || index == mTimes.size()-1)
   {
      matrix = mMatrices[index];
   }
   else
   {
      double blend = (time - mTimes[index]) / (mTimes[index+1] - mTimes[index]);
      matrix = (1.0f - blend) * mMatrices[index] + blend * mMatrices[index+1];
   }
   mLastFloor = index;

   // get the maya matrix
   MMatrix m(matrix.x);
   MTransformationMatrix transform(m);

   // decompose it
   MVector translation = transform.translation(MSpace::kTransform);
   double rotation[3];
   MTransformationMatrix::RotationOrder order;
   transform.getRotation(rotation,order);
   double scale[3];
   transform.getScale(scale,MSpace::kTransform);

   // output all channels
   dataBlock.outputValue(mOutTranslateXAttr).setDouble(translation.x);
   dataBlock.outputValue(mOutTranslateYAttr).setDouble(translation.y);
   dataBlock.outputValue(mOutTranslateZAttr).setDouble(translation.z);
   dataBlock.outputValue(mOutRotateXAttr).setMAngle(MAngle(rotation[0],MAngle::kRadians));
   dataBlock.outputValue(mOutRotateYAttr).setMAngle(MAngle(rotation[1],MAngle::kRadians));
   dataBlock.outputValue(mOutRotateZAttr).setMAngle(MAngle(rotation[2],MAngle::kRadians));
   dataBlock.outputValue(mOutScaleXAttr).setDouble(scale[0]);
   dataBlock.outputValue(mOutScaleYAttr).setDouble(scale[1]);
   dataBlock.outputValue(mOutScaleZAttr).setDouble(scale[2]);

   return status;
}
