#include "AlembicTimeControl.h"

MObject AlembicTimeControlNode::mTimeAttr;
MObject AlembicTimeControlNode::mFactorAttr;
MObject AlembicTimeControlNode::mOffsetAttr;
MObject AlembicTimeControlNode::mOutTimeAttr;

MStatus AlembicTimeControlNode::initialize()
{
   MStatus status;

   MFnUnitAttribute uAttr;
   MFnTypedAttribute tAttr;
   MFnNumericAttribute nAttr;
   MFnGenericAttribute gAttr;
   MFnStringData emptyStringData;

   // input time
   mTimeAttr = nAttr.create("time", "tm", MFnNumericData::kDouble, 0.0, &status);
   status = nAttr.setStorable(true);
   status = addAttribute(mTimeAttr);

   // input factor
   mFactorAttr = nAttr.create("factor", "mul", MFnNumericData::kDouble, 1.0, &status);
   status = nAttr.setStorable(true);
   status = addAttribute(mFactorAttr);

   // input offset
   mOffsetAttr = nAttr.create("offset", "add", MFnNumericData::kDouble, 0.0, &status);
   status = nAttr.setStorable(true);
   status = addAttribute(mOffsetAttr);

   // output transform
   mOutTimeAttr = uAttr.create("outTime", "ot", MFnUnitAttribute::kTime, 0.0);
   status = tAttr.setStorable(false);
   status = tAttr.setWritable(false);
   status = tAttr.setKeyable(false);
   status = addAttribute(mOutTimeAttr);

   // create a mapping
   status = attributeAffects(mTimeAttr, mOutTimeAttr);
   status = attributeAffects(mFactorAttr, mOutTimeAttr);
   status = attributeAffects(mOffsetAttr, mOutTimeAttr);

   return status;
}

MStatus AlembicTimeControlNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   MStatus status;

   MDataHandle timeHandle = dataBlock.inputValue(mTimeAttr, &status);
   MDataHandle factorHandle = dataBlock.inputValue(mFactorAttr);
   MDataHandle offsetHandle = dataBlock.inputValue(mOffsetAttr);

   MTime t;
   t.setUnit(MTime::kSeconds);
   t.setValue(timeHandle.asDouble() * factorHandle.asDouble() + offsetHandle.asDouble());

   dataBlock.outputValue(mOutTimeAttr).setMTime(t);

   return status;
}
