#include "AlembicTimeControl.h"
#include "stdafx.h"

#include <maya/MFnEnumAttribute.h>

MObject AlembicTimeControlNode::mUnitAttr;
MObject AlembicTimeControlNode::mTimeAttr;
MObject AlembicTimeControlNode::mFactorAttr;
MObject AlembicTimeControlNode::mOffsetAttr;
MObject AlembicTimeControlNode::mOutTimeAttr;
MObject AlembicTimeControlNode::mLoopStartAttr;
MObject AlembicTimeControlNode::mLoopEndAttr;

MStatus AlembicTimeControlNode::initialize()
{
  MStatus status;

  MFnUnitAttribute uAttr;
  MFnTypedAttribute tAttr;
  MFnNumericAttribute nAttr;
  MFnGenericAttribute gAttr;
  MFnEnumAttribute eAttr;
  MFnStringData emptyStringData;

  // input time unit
  mUnitAttr = eAttr.create("timeUnit", "tu", 0, &status);
  status = eAttr.addField("Seconds", 0);
  status = eAttr.addField("Working Unit", 1);
  status = eAttr.setStorable(true);
  addAttribute(mUnitAttr);

  // input time
  mTimeAttr =
      uAttr.create("inTime", "tm", MFnUnitAttribute::kTime, 0.0, &status);
  status = nAttr.setStorable(true);
  status = addAttribute(mTimeAttr);

  // input factor
  mFactorAttr =
      nAttr.create("factor", "mul", MFnNumericData::kDouble, 1.0, &status);
  status = nAttr.setStorable(true);
  status = addAttribute(mFactorAttr);

  // input offset
  mOffsetAttr =
      nAttr.create("offset", "add", MFnNumericData::kDouble, 0.0, &status);
  status = nAttr.setStorable(true);
  status = addAttribute(mOffsetAttr);

  // input loop
  // start
  mLoopStartAttr =
      nAttr.create("loopStart", "lst", MFnNumericData::kDouble, 0.0, &status);
  status = nAttr.setStorable(true);
  status = addAttribute(mLoopStartAttr);
  // end
  mLoopEndAttr =
      nAttr.create("loopEnd", "led", MFnNumericData::kDouble, 0.0, &status);
  status = nAttr.setStorable(true);
  status = addAttribute(mLoopEndAttr);

  // output time
  mOutTimeAttr =
      uAttr.create("outTime", "ot", MFnUnitAttribute::kTime, 0.0, &status);
  status = uAttr.setStorable(false);
  status = uAttr.setWritable(false);
  status = uAttr.setKeyable(false);
  status = uAttr.setHidden(false);
  status = addAttribute(mOutTimeAttr);

  // create a mapping
  status = attributeAffects(mUnitAttr, mOutTimeAttr);
  status = attributeAffects(mTimeAttr, mOutTimeAttr);
  status = attributeAffects(mFactorAttr, mOutTimeAttr);
  status = attributeAffects(mOffsetAttr, mOutTimeAttr);
  status = attributeAffects(mLoopStartAttr, mOutTimeAttr);
  status = attributeAffects(mLoopEndAttr, mOutTimeAttr);

  return status;
}

MStatus AlembicTimeControlNode::compute(const MPlug& plug,
                                        MDataBlock& dataBlock)
{
  ESS_PROFILE_SCOPE("AlembicTimeControlNode::compute");
  MStatus status;

  // read the wanted unit system
  MTime t;
  MTime::Unit tUnit = (dataBlock.inputValue(mUnitAttr).asInt() == 1)
                          ? t.unit()
                          : MTime::kSeconds;

  double inputTime = dataBlock.inputValue(mTimeAttr).asTime().as(tUnit);
  MDataHandle factorHandle = dataBlock.inputValue(mFactorAttr);
  MDataHandle offsetHandle = dataBlock.inputValue(mOffsetAttr);

  MDataHandle loopStartHandle = dataBlock.inputValue(mLoopStartAttr);
  MDataHandle loopEndHandle = dataBlock.inputValue(mLoopEndAttr);

  inputTime = inputTime * factorHandle.asDouble() + offsetHandle.asDouble();
  const double start = loopStartHandle.asDouble();
  const double delta = loopEndHandle.asDouble() - start;
  if (delta > 0.0) {
    inputTime = fmod(inputTime - start, delta);
    if (inputTime < 0.0) {
      inputTime += delta;
    }
    inputTime += start;
  }

  t.setUnit(tUnit);
  t.setValue(inputTime);

  dataBlock.outputValue(mOutTimeAttr).setMTime(t);
  dataBlock.outputValue(mOutTimeAttr).setClean();

  return status;
}
