#ifndef _ALEMBIC_XFORM_H_
#define _ALEMBIC_XFORM_H_

#include "AlembicObject.h"

class AlembicXform: public AlembicObject
{
private:
   AbcG::OXform mObject;
   AbcG::OXformSchema mSchema;
   AbcG::XformSample mSample;
   AbcG::OVisibilityProperty mOVisibility;
public:

   AlembicXform(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
   ~AlembicXform();

   virtual Abc::OObject GetObject() { return mObject; }
   virtual Abc::OCompoundProperty GetCompound() { return mSchema; }
   virtual MStatus Save(double time);
};

class AlembicXformNode : public AlembicObjectNode
{
public:
  AlembicXformNode(): mLastMatrix() {}
   virtual ~AlembicXformNode();

   // override virtual methods from MPxNode
   virtual void PreDestruction();
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicXformNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   AbcG::IXformSchema mSchema;
   std::map<AbcA::index_t,Abc::M44d> mSampleIndicesToMatrices;
   Abc::M44d mLastMatrix;
   Abc::IObject iObj;

   // output attributes
   static MObject mOutTranslateXAttr;
   static MObject mOutTranslateYAttr;
   static MObject mOutTranslateZAttr;
   static MObject mOutTranslateAttr;
   static MObject mOutRotateXAttr;
   static MObject mOutRotateYAttr;
   static MObject mOutRotateZAttr;
   static MObject mOutRotateAttr;
   static MObject mOutScaleXAttr;
   static MObject mOutScaleYAttr;
   static MObject mOutScaleZAttr;
   static MObject mOutScaleAttr;
   static MObject mOutVisibilityAttr;
};

#endif