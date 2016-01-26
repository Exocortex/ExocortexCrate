#include "stdafx.h"

#include "AlembicXform.h"
#include "MetaData.h"

#include <maya/MAnimUtil.h>
#include <maya/MDistance.h>

void AlembicXform::testAnimatedVisibility(AlembicObject *aobj, bool animTS,
                                          bool flatHierarchy)
{
  MStatus stat;

  MFnDagNode dagNode(GetRef());
  do {
    MDagPath path;
    stat = dagNode.getPath(path);
    if (stat != MS::kSuccess) {
      break;
    }

    MObject mObj = path.node(&stat);
    if (stat != MS::kSuccess) {
      continue;
    }

    MFnDependencyNode dep(mObj, &stat);
    if (stat != MS::kSuccess) {
      continue;
    }

    MPlug vis = dep.findPlug("visibility", true, &stat);
    if (stat != MS::kSuccess) {
      continue;
    }

    if (MAnimUtil::isAnimated(vis)) {
      if (visInfo.visibility == VISIBLE) {
        visInfo.visibility = ANIMATED_VISIBLE;
        visInfo.mOVisibility = CreateVisibilityProperty(mObject, animTS);
      }
      visInfo.visibilityPlugs.append(vis);  // just add animated ones!
    }
    else if (!vis.asBool()) {  // if it's never visible!
      if (visInfo.visibility == VISIBLE) {
        visInfo.mOVisibility = CreateVisibilityProperty(mObject, animTS);
      }
      visInfo.visibility = NOT_VISIBLE;
      visInfo.visibilityPlugs.clear();
      break;  // break out because no need to analyse this further!
    }

    if (dagNode.parentCount() == 0) {
      break;
    }
    dagNode.setObject(dagNode.parent(0));
  } while (flatHierarchy);
}

AlembicXform::AlembicXform(SceneNodePtr eNode, AlembicWriteJob *in_Job,
                           Abc::OObject oParent)
    : AlembicObject(eNode, in_Job, oParent), visInfo()
{
  const bool animTS = (GetJob()->GetAnimatedTs() > 0);
  mObject = AbcG::OXform(GetMyParent(), eNode->name, animTS);
  mSchema = mObject.getSchema();

  testAnimatedVisibility(this, animTS,
                         GetJob()->GetOption(L"flattenHierarchy").asInt() > 0);
}

AlembicXform::~AlembicXform()
{
  mObject.reset();
  mSchema.reset();
}

#include <maya/MEulerRotation.h>
#include <maya/MFnTransform.h>
MStatus AlembicXform::Save(double time, unsigned int timeIndex,
    bool isFirstFrame)
{
  ESS_PROFILE_SCOPE("AlembicXform::Save");

  // save the metadata
  SaveMetaData(this);

  // iterate all dagpaths
  MDagPath path;
  {
    ESS_PROFILE_SCOPE("AlembicXform::Save dagNode");
    MFnDagNode dagNode(GetRef());
    MDagPathArray dagPaths;
    dagNode.getAllPaths(dagPaths);
    path = dagPaths[0];
  }

  MFnTransform xform(path.node());
  MEulerRotation euler;
  double scale[3];
  xform.getRotation(euler);
  xform.getScale(scale);
  // ESS_LOG_WARNING("Rotation info, axis x:" << euler.x);
  // ESS_LOG_WARNING("Scale info:" << scale[0] << ", " << scale[1] << ", " <<
  // scale[2]);

  if (isFirstFrame) {
    Abc::OCompoundProperty cp;
    Abc::OCompoundProperty up;
    if (AttributesWriter::hasAnyAttr(xform, *GetJob())) {
      cp = mSchema.getArbGeomParams();
      up = mSchema.getUserProperties();
    }

    mAttrs = AttributesWriterPtr(
        new AttributesWriter(cp, up, GetMyParent(), xform, timeIndex, *GetJob())
        );
  }
  else {
    mAttrs->write();
  }

  switch (visInfo.visibility) {
    case NOT_VISIBLE:
      if (mNumSamples == 0) {
        visInfo.mOVisibility.set(AbcG::kVisibilityHidden);
      }
      break;
    case ANIMATED_VISIBLE: {
      bool isVisible = true;
      for (int i = 0; i < (int)visInfo.visibilityPlugs.length(); ++i) {
        if (!visInfo.visibilityPlugs[i].asBool()) {
          isVisible = false;
          break;
        }
      }
      visInfo.mOVisibility.set(isVisible ? AbcG::kVisibilityVisible
                                         : AbcG::kVisibilityHidden);
    } break;
    default:
      break;
  }

  // check if we have the global cache option
  if (GetJob()->GetOption(L"exportInGlobalSpace").asInt() > 0) {
    if (mNumSamples > 0 &&
        visInfo.visibility == VISIBLE)  // if the sample number is greater than
                                        // zero and the visibility is not
                                        // animated, do not export more data!
    {
      return MStatus::kSuccess;
    }

    // store identity matrix
    mSample.setTranslation(Abc::V3d(0.0, 0.0, 0.0));
    mSample.setRotation(Abc::V3d(1.0, 0.0, 0.0), 0.0);
    mSample.setScale(Abc::V3d(1.0, 1.0, 1.0));
  }
  else {
    MMatrix matrix;
    matrix.setToIdentity();

    Abc::M44d abcMatrix;

    {
      ESS_PROFILE_SCOPE("AlembicXform::Save matrix");

      // decide if we need to project to local
      if (IsParentedToRoot() ||
          GetJob()->GetOption(L"flattenHierarchy").asInt() > 0) {
        matrix = path.inclusiveMatrix();
      }
      else {
        matrix.setToProduct(path.inclusiveMatrix(),
                            path.exclusiveMatrixInverse());
      }

      matrix.get(abcMatrix.x);
      mSample.setMatrix(abcMatrix);
      mSample.setInheritsXforms(true);
    }
  }

  // save the sample
  {
    ESS_PROFILE_SCOPE("AlembicXform::Save mSchema.set(mSample)");
    mSchema.set(mSample);
  }
  mNumSamples++;

  return MStatus::kSuccess;
}

void AlembicXformNode::PreDestruction()
{
  mSchema.reset();
  delRefArchive(mFileName);
  mFileName.clear();
}

AlembicXformNode::~AlembicXformNode() { PreDestruction(); }
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
MObject AlembicXformNode::mOutVisibilityAttr;

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
  mFileNameAttr =
      tAttr.create("fileName", "fn", MFnData::kString, emptyStringObject);
  status = tAttr.setStorable(true);
  status = tAttr.setUsedAsFilename(true);
  status = tAttr.setKeyable(false);
  status = addAttribute(mFileNameAttr);

  // input identifier
  mIdentifierAttr =
      tAttr.create("identifier", "if", MFnData::kString, emptyStringObject);
  status = tAttr.setStorable(true);
  status = tAttr.setKeyable(false);
  status = addAttribute(mIdentifierAttr);

  // output translateX
  mOutTranslateXAttr =
      nAttr.create("translateX", "tx", MFnNumericData::kDouble, 0.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);

  // output translateY
  mOutTranslateYAttr =
      nAttr.create("translateY", "ty", MFnNumericData::kDouble, 0.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);

  // output translateY
  mOutTranslateZAttr =
      nAttr.create("translateZ", "tz", MFnNumericData::kDouble, 0.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);

  // output translate compound
  mOutTranslateAttr = nAttr.create("translate", "t", mOutTranslateXAttr,
                                   mOutTranslateYAttr, mOutTranslateZAttr);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutTranslateAttr);

  // output rotatex
  mOutRotateXAttr =
      uAttr.create("rotateX", "rx", MFnUnitAttribute::kAngle, 0.0);
  status = uAttr.setStorable(false);
  status = uAttr.setWritable(false);
  status = uAttr.setKeyable(false);
  status = uAttr.setHidden(false);

  // output rotatexy
  mOutRotateYAttr =
      uAttr.create("rotateY", "ry", MFnUnitAttribute::kAngle, 0.0);
  status = uAttr.setStorable(false);
  status = uAttr.setWritable(false);
  status = uAttr.setKeyable(false);
  status = uAttr.setHidden(false);

  // output rotatez
  mOutRotateZAttr =
      uAttr.create("rotateZ", "rz", MFnUnitAttribute::kAngle, 0.0);
  status = uAttr.setStorable(false);
  status = uAttr.setWritable(false);
  status = uAttr.setKeyable(false);
  status = uAttr.setHidden(false);

  // output rotate compound
  mOutRotateAttr = nAttr.create("rotate", "r", mOutRotateXAttr, mOutRotateYAttr,
                                mOutRotateZAttr);
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

  // output scale compound
  mOutScaleAttr = nAttr.create("scale", "s", mOutScaleXAttr, mOutScaleYAttr,
                               mOutScaleZAttr);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutScaleAttr);

  // output visibility
  mOutVisibilityAttr =
      nAttr.create("outVisibility", "ov", MFnNumericData::kBoolean, true);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutVisibilityAttr);

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

  status = attributeAffects(mTimeAttr, mOutVisibilityAttr);
  status = attributeAffects(mFileNameAttr, mOutVisibilityAttr);
  status = attributeAffects(mIdentifierAttr, mOutVisibilityAttr);

  return status;
}

void AlembicXformNode::cleanDataHandles(MDataBlock &dataBlock)
{
  dataBlock.outputValue(mOutTranslateXAttr).setClean();
  dataBlock.outputValue(mOutTranslateYAttr).setClean();
  dataBlock.outputValue(mOutTranslateZAttr).setClean();

  dataBlock.outputValue(mOutRotateXAttr).setClean();
  dataBlock.outputValue(mOutRotateYAttr).setClean();
  dataBlock.outputValue(mOutRotateZAttr).setClean();

  dataBlock.outputValue(mOutScaleXAttr).setClean();
  dataBlock.outputValue(mOutScaleYAttr).setClean();
  dataBlock.outputValue(mOutScaleZAttr).setClean();

  dataBlock.outputValue(mOutVisibilityAttr).setClean();
}

MStatus AlembicXformNode::compute(const MPlug &plug, MDataBlock &dataBlock)
{
  ESS_PROFILE_SCOPE("AlembicXformNode::compute");
  MStatus status;

  // update the frame number to be imported
  double inputTime =
      dataBlock.inputValue(mTimeAttr).asTime().as(MTime::kSeconds);
  MString &fileName = dataBlock.inputValue(mFileNameAttr).asString();
  MString &identifier = dataBlock.inputValue(mIdentifierAttr).asString();

  // check if we have the file
  if (fileName != mFileName || identifier != mIdentifier) {
    ESS_PROFILE_SCOPE("AlembicXformNode::compute load ABC file");
    mSchema.reset();
    if (fileName != mFileName) {
      delRefArchive(mFileName);
      mFileName = fileName;
      addRefArchive(mFileName);
    }
    mIdentifier = identifier;

    // get the object from the archive
    iObj = getObjectFromArchive(mFileName, identifier);
    if (!iObj.valid()) {
      MGlobal::displayWarning("[ExocortexAlembic] Identifier '" + identifier +
                              "' not found in archive '" + mFileName + "'.");
      return MStatus::kFailure;
    }
    AbcG::IXform obj;
    {
      ESS_PROFILE_SCOPE("AlembicXformNode::compute AbcG::IXform()");
      obj = AbcG::IXform(iObj, Abc::kWrapExisting);
    }
    if (!obj.valid()) {
      MGlobal::displayWarning("[ExocortexAlembic] Identifier '" + identifier +
                              "' in archive '" + mFileName +
                              "' is not a Xform.");
      return MStatus::kFailure;
    }
    {
      ESS_PROFILE_SCOPE("AlembicXformNode::compute obj.getSchema");
      mSchema = obj.getSchema();
    }

    if (!mSchema.valid()) {
      return MStatus::kFailure;
    }

    mSampleIndicesToMatrices.clear();
  }

  if (mSchema.getNumSamples() == 0) {
    return MStatus::kFailure;
  }

  SampleInfo sampleInfo = getSampleInfo(inputTime, mSchema.getTimeSampling(),
                                        mSchema.getNumSamples());

  Abc::M44d matrixAtI;
  Abc::M44d matrixAtIPlus1;

  if (mSampleIndicesToMatrices.find(sampleInfo.floorIndex) ==
      mSampleIndicesToMatrices.end()) {
    AbcG::XformSample sample;
    mSchema.get(sample, sampleInfo.floorIndex);
    matrixAtI = sample.getMatrix();
    mSampleIndicesToMatrices.insert(
        std::map<AbcA::index_t, Abc::M44d>::value_type(sampleInfo.floorIndex,
                                                       matrixAtI));
  }
  else {
    matrixAtI = mSampleIndicesToMatrices[sampleInfo.floorIndex];
  }
  if ((sampleInfo.ceilIndex < (int)mSchema.getNumSamples()) &&
      mSampleIndicesToMatrices.find(sampleInfo.ceilIndex) ==
          mSampleIndicesToMatrices.end()) {
    AbcG::XformSample sample;
    mSchema.get(sample, sampleInfo.ceilIndex);
    matrixAtIPlus1 = sample.getMatrix();
    mSampleIndicesToMatrices.insert(
        std::map<AbcA::index_t, Abc::M44d>::value_type(sampleInfo.ceilIndex,
                                                       matrixAtIPlus1));
  }
  else {
    matrixAtIPlus1 = mSampleIndicesToMatrices[sampleInfo.ceilIndex];
  }

  Abc::M44d matrix;
  if (sampleInfo.alpha == 0.0f) {
    matrix = matrixAtI;
  }
  else if (sampleInfo.alpha == 1.0f) {
    matrix = matrixAtIPlus1;
  }
  else {
    matrix = matrixAtI +
             sampleInfo.alpha *
                 (matrixAtIPlus1 - matrixAtI);  // saving one multiplication
  }

  // export visibility before comparing current and hold matrix!
  AbcG::IVisibilityProperty visibilityProperty =
      AbcG::GetVisibilityProperty(iObj);
  const bool curVisibility =
      visibilityProperty.valid()
          ? (visibilityProperty.getValue(sampleInfo.floorIndex) != 0)
          : true;

  if (mLastMatrix == matrix && curVisibility == mLastVisibility) {
    cleanDataHandles(dataBlock);
    return MS::kSuccess;  // if the current matrix and the previous matrix are
                          // identical!
  }
  mLastMatrix = matrix;
  mLastVisibility = curVisibility;

  // get the maya matrix
  MMatrix m(matrix.x);
  MTransformationMatrix transform(m);

  // decompose it
  MVector translation = transform.translation(MSpace::kTransform);
  double rotation[3];
  MTransformationMatrix::RotationOrder order;
  transform.getRotation(rotation, order);
  double scale[3];
  transform.getScale(scale, MSpace::kTransform);

  {
    ESS_PROFILE_SCOPE("AlembicXformNode::compute outputValues");

    // Convert alembic unit to maya unit
    MDistance distance_tx(translation.x);
    MDistance distance_ty(translation.y);
    MDistance distance_tz(translation.z);
    double translation_x = distance_tx.asUnits(MDistance::uiUnit());
    double translation_y = distance_ty.asUnits(MDistance::uiUnit());
    double translation_z = distance_tz.asUnits(MDistance::uiUnit());

    // output all channels
    dataBlock.outputValue(mOutTranslateXAttr).setDouble(translation_x);
    dataBlock.outputValue(mOutTranslateYAttr).setDouble(translation_y);
    dataBlock.outputValue(mOutTranslateZAttr).setDouble(translation_z);
    dataBlock.outputValue(mOutRotateXAttr)
        .setMAngle(MAngle(rotation[0], MAngle::kRadians));
    dataBlock.outputValue(mOutRotateYAttr)
        .setMAngle(MAngle(rotation[1], MAngle::kRadians));
    dataBlock.outputValue(mOutRotateZAttr)
        .setMAngle(MAngle(rotation[2], MAngle::kRadians));
    dataBlock.outputValue(mOutScaleXAttr).setDouble(scale[0]);
    dataBlock.outputValue(mOutScaleYAttr).setDouble(scale[1]);
    dataBlock.outputValue(mOutScaleZAttr).setDouble(scale[2]);
    dataBlock.outputValue(mOutVisibilityAttr).setBool(curVisibility);

    cleanDataHandles(dataBlock);
  }

  return status;
}
