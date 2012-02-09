#ifndef _ALEMBIC_TIMECONTROL_H_
#define _ALEMBIC_TIMECONTROL_H_

#include "Foundation.h"

class AlembicTimeControlNode: public MPxNode
{
public:
   AlembicTimeControlNode() {}
   virtual ~AlembicTimeControlNode() {}

   // override virtual methods from MPxNode
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicTimeControlNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFactorAttr;
   static MObject mOffsetAttr;

   // output attributes
    static MObject mOutTimeAttr;
};

#endif