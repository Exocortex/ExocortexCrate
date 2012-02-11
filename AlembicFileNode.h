#ifndef _ALEMBIC_FILENODE_H_
#define _ALEMBIC_FILENODE_H_

#include "Foundation.h"

class AlembicFileNode: public MPxNode
{
public:
   AlembicFileNode() {}
   virtual ~AlembicFileNode() {}

   // override virtual methods from MPxNode
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