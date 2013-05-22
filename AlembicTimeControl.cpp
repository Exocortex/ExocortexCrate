#include "stdafx.h"
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
   mTimeAttr = uAttr.create("inTime", "tm", MFnUnitAttribute::kTime, 0.0, &status);
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

   // output time
   mOutTimeAttr = uAttr.create("outTime", "ot", MFnUnitAttribute::kTime, 0.0, &status);
   status = uAttr.setStorable(false);
   status = uAttr.setWritable(false);
   status = uAttr.setKeyable(false);
   status = uAttr.setHidden(false);
   status = addAttribute(mOutTimeAttr);

   // create a mapping
   status = attributeAffects(mTimeAttr, mOutTimeAttr);
   status = attributeAffects(mFactorAttr, mOutTimeAttr);
   status = attributeAffects(mOffsetAttr, mOutTimeAttr);

   return status;
}

MStatus AlembicTimeControlNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   ESS_PROFILE_SCOPE("AlembicTimeControlNode::compute");
   MStatus status;

   double inputTime = dataBlock.inputValue(mTimeAttr).asTime().as(MTime::kSeconds);
   MDataHandle factorHandle = dataBlock.inputValue(mFactorAttr);
   MDataHandle offsetHandle = dataBlock.inputValue(mOffsetAttr);


   MTime t;
   t.setUnit(MTime::kSeconds);
   t.setValue(inputTime * factorHandle.asDouble() + offsetHandle.asDouble());

   dataBlock.outputValue(mOutTimeAttr).setMTime(t);
   dataBlock.outputValue(mOutTimeAttr).setClean();

   return status;
}
