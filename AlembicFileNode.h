#ifndef _ALEMBIC_FILENODE_H_
#define _ALEMBIC_FILENODE_H_

#include "Foundation.h"
#include "AlembicObject.h"

class AlembicFileNode: public AlembicObjectNode
{
public:
   AlembicFileNode() {}
   virtual ~AlembicFileNode() {}

   // override virtual methods from MPxNode
   virtual void PreDestruction() {};
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicFileNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFactorAttr;
   static MObject mOffsetAttr;

   // output attributes
   static MObject mFileNameAttr;
   static MObject mOutFileNameAttr;
};

#endif