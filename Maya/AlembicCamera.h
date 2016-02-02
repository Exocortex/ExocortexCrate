#ifndef _ALEMBIC_CAMERA_H_
#define _ALEMBIC_CAMERA_H_

#include "AlembicObject.h"
#include "AttributesWriter.h"

class AlembicCamera : public AlembicObject {
 private:
  AbcG::OCamera mObject;
  AbcG::OCameraSchema mSchema;
  AbcG::CameraSample mSample;
  AttributesWriterPtr mAttrs;

 public:
  AlembicCamera(SceneNodePtr eNode, AlembicWriteJob* in_Job,
                Abc::OObject oParent);
  ~AlembicCamera();

  virtual Abc::OObject GetObject() { return mObject; }
  virtual Abc::OCompoundProperty GetCompound() { return mSchema; }
  virtual MStatus Save(double time, unsigned int timeIndex,
      bool isFirstFrame);
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

  bool setInternalValueInContext(const MPlug & plug,
      const MDataHandle & dataHandle,
      MDGContext & ctx);
  MStatus setDependentsDirty(const MPlug &plugBeingDirtied,
      MPlugArray &affectedPlugs);

 private:
  // input attributes
  static MObject mTimeAttr;
  static MObject mFileNameAttr;
  static MObject mIdentifierAttr;
  MString mFileName;
  MString mIdentifier;
  MPlugArray mGeomParamPlugs;
  MPlugArray mUserAttrPlugs;
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

  static MObject mGeomParamsList;
  static MObject mUserAttrsList;
};

#endif
