#ifndef _ALEMBIC_CAMERA_H_
#define _ALEMBIC_CAMERA_H_

#include "AlembicObject.h"

class AlembicCamera: public AlembicObject
{
private:
   Alembic::AbcGeom::OCamera mObject;
   Alembic::AbcGeom::OCameraSchema mSchema;
   Alembic::AbcGeom::CameraSample mSample;
public:

   AlembicCamera(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicCamera();

   virtual Alembic::Abc::OObject GetObject() { return mObject; }
   virtual Alembic::Abc::OCompoundProperty GetCompound() { return mSchema; }
   virtual MStatus Save(double time);
};

class AlembicCameraNode : public AlembicObjectNode
{
public:
   AlembicCameraNode() {}
   virtual ~AlembicCameraNode();

   // override virtual methods from MPxNode
   virtual void PreDestruction();
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicCameraNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   Alembic::AbcGeom::ICameraSchema mSchema;

   // output attributes
   static MObject mOutFocalLengthAttr;
   static MObject mOutFocusDistanceAttr;
   static MObject mOutAspectRatioAttr;
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