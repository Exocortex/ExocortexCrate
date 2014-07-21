#include "stdafx.h"
#include "AlembicFileNode.h"
#include <cstdio>
#include <sstream>
#include <fstream>

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
#ifndef MAYA_2011_UNSUPPORTED
   nAttr.setNiceNameOverride("Use Multiple Files");
#endif
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

static std::string createFileTemplate(const std::string &filename, size_t num, size_t dot, bool usePadding)
{
	std::stringstream ss;
	ss << filename.substr(0, num) << "%";
	if (usePadding)
	{
		const size_t diff = dot-num;
		if (diff)
		   ss << "0" << diff;
	}
	ss << "d" << filename.substr(dot);
	return ss.str();
}

MStatus AlembicFileNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   ESS_PROFILE_SCOPE("AlembicFileNode::compute");
   MString inFileName = dataBlock.inputValue(mFileNameAttr).asString();

   if (lastFileName != inFileName)
   {
	   lastFileName = inFileName;
	   multiFileTemplate.clear();
	   multiFileTemplateNoPadding.clear();
	   lastMultiFileName = inFileName;
   }

   if ( dataBlock.inputValue(mMultiFileAttr).asBool() )
   {
	   if (multiFileTemplate.size() == 0)
	   {
		   const std::string filename = inFileName.asChar();
		   size_t dot = filename.rfind(".");
		   size_t num = dot-1;
		   if (dot != std::string::npos)
		   {
			   while (filename[num] >= '0' && filename[num] <= '9')
				   --num;
			   ++num;
		   }

		   // The Padding format <filename>#####.abc --> <filename>00010.abc, <filename>08921.abc
		   multiFileTemplate = createFileTemplate(filename, num, dot, true);

		   // The Non-Padding format <filename>#.abc --> <filename>10.abc, <filename>8921.abc
		   multiFileTemplateNoPadding = createFileTemplate(filename, num, dot, false);
	   }

	   const int curFrame = dataBlock.inputValue(mTimeAttr).asTime().value();
	   if (lastCurFrame != curFrame)
	   {
		   std::vector<char> vecFormattedName(inFileName.length() + 30, '\0');
		   char *tmpFormattedName = &( vecFormattedName[0] );

		   sprintf(tmpFormattedName, multiFileTemplate.c_str(), curFrame);
		   std::ifstream in(tmpFormattedName);
		   if (in.is_open())	// try the padded version
		   {
			   in.close();
			   lastMultiFileName = tmpFormattedName;
			   lastCurFrame = curFrame;
		   }
		   else
		   {
			   sprintf(tmpFormattedName,  multiFileTemplateNoPadding.c_str(), curFrame);
			   in.open(tmpFormattedName);
			   if (in.is_open())	// try the non-padded version!
			   {
				   in.close();
				   lastMultiFileName = tmpFormattedName;
				   lastCurFrame = curFrame;
			   }
		   }
	   }
	   inFileName = lastMultiFileName;
   }

   dataBlock.outputValue(mOutFileNameAttr).set(resolvePath(inFileName));
   dataBlock.outputValue(mOutFileNameAttr).setClean();
   return MStatus::kSuccess;
}
