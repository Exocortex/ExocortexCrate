#ifndef _ALEMBIC_FILENODE_H_
#define _ALEMBIC_FILENODE_H_

#include "AlembicObject.h"

class AlembicFileNode: public AlembicObjectNode
{
public:
	AlembicFileNode(): lastCurFrame(-50000) {}
   virtual ~AlembicFileNode() {}

   // override virtual methods from MPxNode
   virtual void PreDestruction() {};
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicFileNode()); }
   static MStatus initialize();

private:
	MString lastFileName;
	std::string multiFileTemplate;
	std::string multiFileTemplateNoPadding;
	int lastCurFrame;
	MString lastMultiFileName;

   // input attributes
   static MObject mTimeAttr;
   static MObject mMultiFileAttr;

   // output attributes
   static MObject mFileNameAttr;
   static MObject mOutFileNameAttr;
};

#endif