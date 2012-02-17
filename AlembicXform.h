#ifndef _ALEMBIC_XFORM_H_
#define _ALEMBIC_XFORM_H_

#include "AlembicObject.h"

class AlembicXform: public AlembicObject
{
private:
   Alembic::AbcGeom::OXform mObject;
   Alembic::AbcGeom::OXformSchema mSchema;
   Alembic::AbcGeom::XformSample mSample;
public:

   AlembicXform(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicXform();

   virtual Alembic::Abc::OObject GetObject() { return mObject; }
   virtual Alembic::Abc::OCompoundProperty GetCompound() { return mSchema; }
   virtual MStatus Save(double time);
};

class AlembicXformNode : public AlembicObjectNode
{
public:
   AlembicXformNode() {}
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
   Alembic::AbcGeom::IXformSchema mSchema;

   // output attributes
   static MObject mOutTranslateXAttr;
   static MObject mOutTranslateYAttr;
   static MObject mOutTranslateZAttr;
   static MObject mOutRotateXAttr;
   static MObject mOutRotateYAttr;
   static MObject mOutRotateZAttr;
   static MObject mOutScaleXAttr;
   static MObject mOutScaleYAttr;
   static MObject mOutScaleZAttr;
};

#endif