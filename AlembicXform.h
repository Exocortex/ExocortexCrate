#ifndef _ALEMBIC_XFORM_H_
#define _ALEMBIC_XFORM_H_

#include "AlembicObject.h"

class AlembicXform: public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::XformSample mXformSample;
public:

   AlembicXform(const MObject & in_Ref, AlembicWriteJob * in_Job);
   ~AlembicXform();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual MStatus Save(double time);
};

class AlembicXformNode : public MPxNode
{
public:
   AlembicXformNode() {}
   virtual ~AlembicXformNode() {}

   // override virtual methods from MPxNode
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicXformNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;

   // output attributes
    static MObject mOutTransformAttr;
};

#endif