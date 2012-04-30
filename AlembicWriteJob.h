#ifndef _ALEMBIC_WRITE_JOB_H_
#define _ALEMBIC_WRITE_JOB_H_

#include "Foundation.h"
#include <xsi_ref.h>
#include <xsi_doublearray.h>
#include "AlembicObject.h"

class AlembicWriteJob
{
private:
    XSI::CString mFileName;
    XSI::CRefArray mSelection;
    std::vector<double> mFrames;
    Alembic::Abc::OArchive mArchive;
    unsigned int mTs;
    std::map<XSI::CString,XSI::CValue> mOptions;
    std::vector<AlembicObjectPtr> mObjects;
	std::map<std::string,AlembicObjectPtr> mObjectsNames;
    float mFrameRate;
public:
   AlembicWriteJob(
      const XSI::CString & in_FileName,
      const XSI::CRefArray & in_Selection,
      const XSI::CDoubleArray & in_Frames);
   ~AlembicWriteJob();

   Alembic::Abc::OArchive GetArchive() { return mArchive; }
   const std::vector<double> & GetFrames() { return mFrames; }
   const XSI::CString & GetFileName() { return mFileName; }
   unsigned int GetAnimatedTs() { return mTs; }
   void SetOption(const XSI::CString & in_Name, const XSI::CValue & in_Value);
   bool HasOption(const XSI::CString & in_Name);
   XSI::CValue GetOption(const XSI::CString & in_Name);
   AlembicObjectPtr GetObject(const XSI::CRef & in_Ref);
   bool AddObject(AlembicObjectPtr in_Obj);
   size_t GetNbObjects() { return mObjects.size(); }
 
   XSI::CStatus PreProcess();
   XSI::CStatus Process(double frame);
};

#endif