#ifndef _ALEMBIC_TIMECONTROL_H_
#define _ALEMBIC_TIMECONTROL_H_

#include "AlembicObject.h"

class AlembicTimeControlNode : public AlembicObjectNode {
 public:
  AlembicTimeControlNode() {}
  virtual ~AlembicTimeControlNode() {}
  // override virtual methods from MPxNode
  virtual void PreDestruction(){};
  virtual MStatus compute(const MPlug& plug, MDataBlock& dataBlock);
  static void* creator() { return (new AlembicTimeControlNode()); }
  static MStatus initialize();

 private:
  // input attributes
  static MObject mUnitAttr;

  static MObject mTimeAttr;
  static MObject mFactorAttr;
  static MObject mOffsetAttr;

  static MObject mLoopStartAttr;
  static MObject mLoopEndAttr;

  // output attributes
  static MObject mOutTimeAttr;
};

#endif