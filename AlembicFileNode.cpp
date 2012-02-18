#include "AlembicFileNode.h"

MObject AlembicFileNode::mFileNameAttr;
MObject AlembicFileNode::mOutFileNameAttr;

MStatus AlembicFileNode::initialize()
{
   MStatus status;

   MFnUnitAttribute uAttr;
   MFnTypedAttribute tAttr;
   MFnNumericAttribute nAttr;
   MFnGenericAttribute gAttr;
   MFnStringData emptyStringData;
   MObject emptyStringObject = emptyStringData.create("");

   // input file name
   mFileNameAttr = tAttr.create("fileName", "fn", MFnData::kString, emptyStringObject);
   status = tAttr.setStorable(true);
   status = tAttr.setUsedAsFilename(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mFileNameAttr);

   mOutFileNameAttr = tAttr.create("outFileName", "of", MFnData::kString);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = addAttribute(mOutFileNameAttr);

   // create a mapping
   status = attributeAffects(mFileNameAttr, mOutFileNameAttr);

   return status;
}

MStatus AlembicFileNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   dataBlock.outputValue(mOutFileNameAttr).set(resolvePath(dataBlock.inputValue(mFileNameAttr).asString()));
   return MStatus::kSuccess;
}
