#include "stdafx.h"

#include "AlembicCamera.h"
#include "MetaData.h"

AlembicCamera::AlembicCamera(SceneNodePtr eNode, AlembicWriteJob* in_Job,
                             Abc::OObject oParent)
    : AlembicObject(eNode, in_Job, oParent)
{
  mObject =
      AbcG::OCamera(GetMyParent(), eNode->name, GetJob()->GetAnimatedTs());
  mSchema = mObject.getSchema();
}

AlembicCamera::~AlembicCamera()
{
  mObject.reset();
  mSchema.reset();
}

MStatus AlembicCamera::Save(double time)
{
  ESS_PROFILE_SCOPE("AlembicCamera::Save");
  // access the camera
  MFnCamera node(GetRef());

  // save the metadata
  SaveMetaData(this);

  // bake in the device aspect ratio since other programs do not support it (see
  // the article "Maya to Softimage: Camera Interoperability")

  MCommonRenderSettingsData data;
  MRenderUtil::getCommonRenderSettings(data);

  double dar = (double)data.deviceAspectRatio;
  double cam = node.aspectRatio();
  MFnCamera::FilmFit filmFit = node.filmFit();

  if (filmFit == MFnCamera::kFillFilmFit) {  // Fits the resolution gate within
    // the film gate.
    if (cam < dar) {
      filmFit = MFnCamera::kHorizontalFilmFit;
    }
    else {
      filmFit = MFnCamera::kVerticalFilmFit;
    }
  }
  else if (filmFit == MFnCamera::kOverscanFilmFit) {  // Fits the film gate
    // within the resolution
    // gate.
    if (cam < dar) {
      filmFit = MFnCamera::kVerticalFilmFit;
    }
    else {
      filmFit = MFnCamera::kHorizontalFilmFit;
    }
  }

  double fHorizontalAperture = node.horizontalFilmAperture();
  double fVerticalAperture = node.verticalFilmAperture();

  if (filmFit == MFnCamera::kVerticalFilmFit) {  // Fits the resolution gate
    // horizontally within the film
    // gate.
    const double fFitFactor = (fVerticalAperture * dar) / fHorizontalAperture;
    fHorizontalAperture *= fFitFactor;
  }
  else if (filmFit == MFnCamera::kHorizontalFilmFit) {  // Fits the resolution
    // gate vertically
    // within the film gate.
    const double fFitFactor = fHorizontalAperture / (fVerticalAperture * dar);
    fVerticalAperture *= fFitFactor;
  }

  mSample.setFocalLength(node.focalLength());
  mSample.setFocusDistance(node.focusDistance());
  mSample.setLensSqueezeRatio(node.lensSqueezeRatio());
  mSample.setHorizontalAperture(fHorizontalAperture * 2.54);
  mSample.setVerticalAperture(fVerticalAperture * 2.54);
  mSample.setHorizontalFilmOffset(node.horizontalFilmOffset() * 2.54);
  mSample.setVerticalFilmOffset(node.verticalFilmOffset() * 2.54);
  mSample.setNearClippingPlane(node.nearClippingPlane());
  mSample.setFarClippingPlane(node.farClippingPlane());
  mSample.setFStop(node.fStop());
  mSample.setShutterOpen(0.0);

  // special case shutter angle
  mSample.setShutterClose(MTime(1.0, MTime::kSeconds).as(MTime::uiUnit()) *
                          AbcG::RadiansToDegrees(node.shutterAngle()) / 360.0);

  // save the sample
  mSchema.set(mSample);
  mNumSamples++;

  return MStatus::kSuccess;
}

void AlembicCameraNode::PreDestruction()
{
  mSchema.reset();
  delRefArchive(mFileName);
  mFileName.clear();
}

AlembicCameraNode::~AlembicCameraNode() { PreDestruction(); }
MObject AlembicCameraNode::mTimeAttr;
MObject AlembicCameraNode::mFileNameAttr;
MObject AlembicCameraNode::mIdentifierAttr;
MObject AlembicCameraNode::mOutFocalLengthAttr;
MObject AlembicCameraNode::mOutFocusDistanceAttr;
MObject AlembicCameraNode::mOutLensSqueezeRatioAttr;
MObject AlembicCameraNode::mOutHorizontalApertureAttr;
MObject AlembicCameraNode::mOutVerticalApertureAttr;
MObject AlembicCameraNode::mOutHorizontalOffsetAttr;
MObject AlembicCameraNode::mOutVerticalOffsetAttr;
MObject AlembicCameraNode::mOutNearClippingAttr;
MObject AlembicCameraNode::mOutFarClippingAttr;
MObject AlembicCameraNode::mOutFStopAttr;
MObject AlembicCameraNode::mOutShutterAngleAttr;

MStatus AlembicCameraNode::initialize()
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
  MObject identityMatrixObject =
      identityMatrixData.create(MTransformationMatrix::identity);

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

  // output focalLength
  mOutFocalLengthAttr =
      nAttr.create("focalLength", "fl", MFnNumericData::kDouble, 30.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutFocalLengthAttr);

  // output focusDistance
  mOutFocusDistanceAttr =
      nAttr.create("focusDistance", "fd", MFnNumericData::kDouble, 10.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutFocusDistanceAttr);

  // output focalLength
  mOutLensSqueezeRatioAttr =
      nAttr.create("lensSqueezeRatio", "lr", MFnNumericData::kDouble, 1.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutLensSqueezeRatioAttr);

  // horizonal aperture
  mOutHorizontalApertureAttr = nAttr.create("horizontalFilmAperture", "ha",
                                            MFnNumericData::kDouble, 10.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutHorizontalApertureAttr);

  // vertical aperture
  mOutVerticalApertureAttr =
      nAttr.create("verticalFilmAperture", "va", MFnNumericData::kDouble, 10.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutVerticalApertureAttr);

  // horizonal offset
  mOutHorizontalOffsetAttr =
      nAttr.create("horizontalFilmOffset", "ho", MFnNumericData::kDouble, 0.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutHorizontalOffsetAttr);

  // output vertical offset
  mOutVerticalOffsetAttr =
      nAttr.create("verticalFilmOffset", "vo", MFnNumericData::kDouble, 0.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutVerticalOffsetAttr);

  // output near clipping
  mOutNearClippingAttr =
      nAttr.create("nearClippingPlane", "nc", MFnNumericData::kDouble, 0.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutNearClippingAttr);

  // output far clipping
  mOutFarClippingAttr =
      nAttr.create("farClippingPlane", "fc", MFnNumericData::kDouble, 1000.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutFarClippingAttr);

  // output fstop
  mOutFStopAttr = nAttr.create("fStop", "fs", MFnNumericData::kDouble, 5.6);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutFStopAttr);

  // output far clipping
  mOutShutterAngleAttr =
      nAttr.create("shutterAngle", "sa", MFnNumericData::kDouble, 20.0);
  status = nAttr.setStorable(false);
  status = nAttr.setWritable(false);
  status = nAttr.setKeyable(false);
  status = nAttr.setHidden(false);
  status = addAttribute(mOutShutterAngleAttr);

  // create a mapping
  status = attributeAffects(mTimeAttr, mOutFocalLengthAttr);
  status = attributeAffects(mFileNameAttr, mOutFocalLengthAttr);
  status = attributeAffects(mIdentifierAttr, mOutFocalLengthAttr);
  status = attributeAffects(mTimeAttr, mOutFocusDistanceAttr);
  status = attributeAffects(mFileNameAttr, mOutFocusDistanceAttr);
  status = attributeAffects(mIdentifierAttr, mOutFocusDistanceAttr);
  status = attributeAffects(mTimeAttr, mOutLensSqueezeRatioAttr);
  status = attributeAffects(mFileNameAttr, mOutLensSqueezeRatioAttr);
  status = attributeAffects(mIdentifierAttr, mOutLensSqueezeRatioAttr);
  status = attributeAffects(mTimeAttr, mOutHorizontalApertureAttr);
  status = attributeAffects(mFileNameAttr, mOutHorizontalApertureAttr);
  status = attributeAffects(mIdentifierAttr, mOutHorizontalApertureAttr);
  status = attributeAffects(mTimeAttr, mOutVerticalApertureAttr);
  status = attributeAffects(mFileNameAttr, mOutVerticalApertureAttr);
  status = attributeAffects(mIdentifierAttr, mOutVerticalApertureAttr);
  status = attributeAffects(mTimeAttr, mOutHorizontalOffsetAttr);
  status = attributeAffects(mFileNameAttr, mOutHorizontalOffsetAttr);
  status = attributeAffects(mIdentifierAttr, mOutHorizontalOffsetAttr);
  status = attributeAffects(mTimeAttr, mOutVerticalOffsetAttr);
  status = attributeAffects(mFileNameAttr, mOutVerticalOffsetAttr);
  status = attributeAffects(mIdentifierAttr, mOutVerticalOffsetAttr);
  status = attributeAffects(mTimeAttr, mOutNearClippingAttr);
  status = attributeAffects(mFileNameAttr, mOutNearClippingAttr);
  status = attributeAffects(mIdentifierAttr, mOutNearClippingAttr);
  status = attributeAffects(mTimeAttr, mOutFarClippingAttr);
  status = attributeAffects(mFileNameAttr, mOutFarClippingAttr);
  status = attributeAffects(mIdentifierAttr, mOutFarClippingAttr);
  status = attributeAffects(mTimeAttr, mOutFStopAttr);
  status = attributeAffects(mFileNameAttr, mOutFStopAttr);
  status = attributeAffects(mIdentifierAttr, mOutFStopAttr);
  status = attributeAffects(mTimeAttr, mOutShutterAngleAttr);
  status = attributeAffects(mFileNameAttr, mOutShutterAngleAttr);
  status = attributeAffects(mIdentifierAttr, mOutShutterAngleAttr);

  return status;
}

MStatus AlembicCameraNode::compute(const MPlug& plug, MDataBlock& dataBlock)
{
  ESS_PROFILE_SCOPE("AlembicCameraNode::compute");
  MStatus status;

  // update the frame number to be imported
  double inputTime =
      dataBlock.inputValue(mTimeAttr).asTime().as(MTime::kSeconds);
  MString& fileName = dataBlock.inputValue(mFileNameAttr).asString();
  MString& identifier = dataBlock.inputValue(mIdentifierAttr).asString();

  // check if we have the file
  if (fileName != mFileName || identifier != mIdentifier) {
    mSchema.reset();
    if (fileName != mFileName) {
      delRefArchive(mFileName);
      mFileName = fileName;
      addRefArchive(mFileName);
    }
    mIdentifier = identifier;

    // get the object from the archive
    Abc::IObject iObj = getObjectFromArchive(mFileName, identifier);
    if (!iObj.valid()) {
      MGlobal::displayWarning("[ExocortexAlembic] Identifier '" + identifier +
                              "' not found in archive '" + mFileName + "'.");
      return MStatus::kFailure;
    }
    AbcG::ICamera obj(iObj, Abc::kWrapExisting);
    if (!obj.valid()) {
      MGlobal::displayWarning("[ExocortexAlembic] Identifier '" + identifier +
                              "' in archive '" + mFileName +
                              "' is not a Camera.");
      return MStatus::kFailure;
    }
    mSchema = obj.getSchema();
  }

  if (!mSchema.valid()) {
    return MStatus::kFailure;
  }

  // get the sample
  SampleInfo sampleInfo = getSampleInfo(inputTime, mSchema.getTimeSampling(),
                                        mSchema.getNumSamples());

  // access the camera values
  AbcG::CameraSample sample;
  mSchema.get(sample, sampleInfo.floorIndex);
  double focalLength = sample.getFocalLength();
  double focusDistance = sample.getFocusDistance();
  double lensSqueezeRatio = sample.getLensSqueezeRatio();
  double horizontalAperture = sample.getHorizontalAperture();
  double verticalAperture = sample.getVerticalAperture();
  double horizontalOffset = sample.getHorizontalFilmOffset();
  double verticalOffset = sample.getVerticalFilmOffset();
  double nearClipping = sample.getNearClippingPlane();
  double farClipping = sample.getFarClippingPlane();
  double fStop = sample.getFStop();
  double shutterOpen = sample.getShutterOpen();
  double shutterClose = sample.getShutterClose();

  // blend the matrix if we are between frames
  if (sampleInfo.alpha != 0.0) {
    mSchema.get(sample, sampleInfo.ceilIndex);
    const double blend = sampleInfo.alpha;
    const double iblend = 1.0 - blend;
    focalLength = iblend * focalLength + blend * sample.getFocalLength();
    focusDistance = iblend * focusDistance + blend * sample.getFocusDistance();
    lensSqueezeRatio =
        iblend * lensSqueezeRatio + blend * sample.getLensSqueezeRatio();
    horizontalAperture =
        iblend * horizontalAperture + blend * sample.getHorizontalAperture();
    verticalAperture =
        iblend * verticalAperture + blend * sample.getVerticalAperture();
    horizontalOffset =
        iblend * horizontalOffset + blend * sample.getHorizontalFilmOffset();
    verticalOffset =
        iblend * verticalOffset + blend * sample.getVerticalFilmOffset();
    nearClipping =
        iblend * nearClipping + blend * sample.getNearClippingPlane();
    farClipping = iblend * farClipping + blend * sample.getFarClippingPlane();
    fStop = iblend * fStop + blend * sample.getFStop();
    shutterOpen = iblend * shutterOpen + blend * sample.getShutterOpen();
    shutterClose = iblend * shutterClose + blend * sample.getShutterClose();
  }

  // output all channels
  dataBlock.outputValue(mOutFocalLengthAttr).set(focalLength);
  dataBlock.outputValue(mOutFocusDistanceAttr).set(focusDistance);
  dataBlock.outputValue(mOutLensSqueezeRatioAttr).set(lensSqueezeRatio);
  dataBlock.outputValue(mOutHorizontalApertureAttr)
      .set(horizontalAperture / 2.54);
  dataBlock.outputValue(mOutVerticalApertureAttr).set(verticalAperture / 2.54);
  dataBlock.outputValue(mOutHorizontalOffsetAttr).set(horizontalOffset / 2.54);
  dataBlock.outputValue(mOutHorizontalOffsetAttr).set(verticalOffset / 2.54);
  dataBlock.outputValue(mOutNearClippingAttr).set(nearClipping);
  dataBlock.outputValue(mOutFarClippingAttr).set(farClipping);
  dataBlock.outputValue(mOutFStopAttr).set(farClipping);
  dataBlock.outputValue(mOutShutterAngleAttr)
      .set(360.0 * (shutterClose - shutterOpen) /
           MTime(1.0, MTime::kSeconds).as(MTime::uiUnit()));

  // clean output channels
  dataBlock.outputValue(mOutFocalLengthAttr).setClean();
  dataBlock.outputValue(mOutFocusDistanceAttr).setClean();
  dataBlock.outputValue(mOutLensSqueezeRatioAttr).setClean();
  dataBlock.outputValue(mOutHorizontalApertureAttr).setClean();
  dataBlock.outputValue(mOutVerticalApertureAttr).setClean();
  dataBlock.outputValue(mOutHorizontalOffsetAttr).setClean();
  dataBlock.outputValue(mOutHorizontalOffsetAttr).setClean();
  dataBlock.outputValue(mOutNearClippingAttr).setClean();
  dataBlock.outputValue(mOutFarClippingAttr).setClean();
  dataBlock.outputValue(mOutFStopAttr).setClean();
  dataBlock.outputValue(mOutShutterAngleAttr).setClean();
  return status;
}
