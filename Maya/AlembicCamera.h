#ifndef _ALEMBIC_CAMERA_H_
#define _ALEMBIC_CAMERA_H_

#include "AlembicObject.h"

class AlembicCamera : public AlembicObject {
 private:
  AbcG::OCamera mObject;
  AbcG::OCameraSchema mSchema;
  AbcG::CameraSample mSample;

 public:
  AlembicCamera(SceneNodePtr eNode, AlembicWriteJob* in_Job,
                Abc::OObject oParent);
  ~AlembicCamera();

  virtual Abc::OObject GetObject() { return mObject; }
  virtual Abc::OCompoundProperty GetCompound() { return mSchema; }
  virtual MStatus Save(double time);
};

class AlembicCameraNode : public AlembicObjectNode {
 public:
  AlembicCameraNode() {}
  virtual ~AlembicCameraNode();

  // override virtual methods from MPxNode
  virtual void PreDestruction();
  virtual MStatus compute(const MPlug& plug, MDataBlock& dataBlock);
  static void* creator() { return (new AlembicCameraNode()); }
  static MStatus initialize();

 private:
  // input attributes
  static MObject mTimeAttr;
  static MObject mFileNameAttr;
  static MObject mIdentifierAttr;
  MString mFileName;
  MString mIdentifier;
  AbcG::ICameraSchema mSchema;

  // output attributes
  static MObject mOutFocalLengthAttr;
  static MObject mOutFocusDistanceAttr;
  static MObject mOutLensSqueezeRatioAttr;
  static MObject mOutHorizontalApertureAttr;
  static MObject mOutVerticalApertureAttr;
  static MObject mOutHorizontalOffsetAttr;
  static MObject mOutVerticalOffsetAttr;
  static MObject mOutNearClippingAttr;
  static MObject mOutFarClippingAttr;
  static MObject mOutFStopAttr;
  static MObject mOutShutterAngleAttr;
};

#endif