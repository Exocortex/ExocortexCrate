#include "stdafx.h"
#include "AlembicFileNode.h"
#include <cstdio>
#include <sstream>

MObject AlembicFileNode::mTimeAttr;
MObject AlembicFileNode::mMultiFileAttr;

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

   // inputs
   mMultiFileAttr = nAttr.create("multiFiles", "mf", MFnNumericData::kBoolean, true);
   nAttr.setNiceNameOverride("Use Multiple Files");
   status = nAttr.setStorable(true);
   status = nAttr.setDefault(false);
   status = nAttr.setKeyable(false);
   status = addAttribute(mMultiFileAttr);

   mTimeAttr = uAttr.create("inTime", "tm", MFnUnitAttribute::kTime, 0.0);
   status = uAttr.setStorable(true);
   status = uAttr.setKeyable(true);
   status = addAttribute(mTimeAttr);

   mFileNameAttr = tAttr.create("fileName", "fn", MFnData::kString, emptyStringObject);
   status = tAttr.setStorable(true);
   status = tAttr.setUsedAsFilename(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mFileNameAttr);

   // output file name
   mOutFileNameAttr = tAttr.create("outFileName", "of", MFnData::kString);
   status = tAttr.setStorable(false);
   status = tAttr.setWritable(false);
   status = tAttr.setKeyable(false);
   status = tAttr.setHidden(false);
   status = addAttribute(mOutFileNameAttr);

   // create a mapping
   status = attributeAffects(mMultiFileAttr, mOutFileNameAttr);
   status = attributeAffects(mTimeAttr, mOutFileNameAttr);
   status = attributeAffects(mFileNameAttr, mOutFileNameAttr);

   return status;
}

MStatus AlembicFileNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   ESS_PROFILE_SCOPE("AlembicFileNode::compute");
   MString inFileName = dataBlock.inputValue(mFileNameAttr).asString();

   if (lastFileName != inFileName)
   {
	   lastFileName = inFileName;
	   multiFileTemplate.clear();
   }

   if ( dataBlock.inputValue(mMultiFileAttr).asBool() )
   {
	   if (multiFileTemplate.size() == 0)
	   {
		   std::string filename = inFileName.asChar();
		   size_t dot = filename.rfind(".");
		   size_t num = dot-1;
		   if (dot != std::string::npos)
		   {
			   while (filename[num] >= '0' && filename[num] <= '9')
				   --num;
			   ++num;
		   }

		   std::stringstream ss;
		   ss << filename.substr(0, num) << "%";
		   {
			   const size_t diff = dot-num;
			   if (diff)
				   ss << "0" << diff;
		   }
		   ss << "d" << filename.substr(dot);
		   multiFileTemplate = ss.str();
	   }

	   const int curFrame = dataBlock.inputValue(mTimeAttr).asTime().value();
	   if (lastCurFrame != curFrame)
	   {
		   lastCurFrame = curFrame;
		   char *tmpFormattedName = new char[inFileName.length() + 30];
		   sprintf(tmpFormattedName, multiFileTemplate.c_str(), curFrame);
		   lastMultiFileName = tmpFormattedName;
		   delete [] tmpFormattedName;
	   }
	   inFileName = lastMultiFileName;
   }

   dataBlock.outputValue(mOutFileNameAttr).set(resolvePath(inFileName));
   dataBlock.outputValue(mOutFileNameAttr).setClean();
   return MStatus::kSuccess;
}
