#ifndef _ALEMBIC_WRITE_JOB_H_
#define _ALEMBIC_WRITE_JOB_H_
#include "Foundation.h"
#include "ObjectList.h" 
#include "AlembicObject.h"

class Object;
class Interface;

class AlembicWriteJob
{
private:
    std::string mFileName;
    ObjectList mSelection;
    std::vector<double> mFrames;
    Alembic::Abc::OArchive mArchive;
    unsigned int mTs;
    std::map<std::string, bool> mOptions;
    std::vector<AlembicObjectPtr> mObjects;
    float mFrameRate;
    Interface *mApplication;
    Alembic::Abc::OBox3dProperty m_ArchiveBoxProp;
    Alembic::Abc::Box3d m_Archivebbox;

    void AddObject(AlembicObjectPtr obj);
public:
   AlembicWriteJob(const std::string &in_FileName, const ObjectList &in_Selection, const std::vector<double> &in_Frames, Interface *i);
   ~AlembicWriteJob();

   Alembic::Abc::OArchive GetArchive() { return mArchive; }
   const std::vector<double> & GetFrames() { return mFrames; }
   const std::string & GetFileName() { return mFileName; }
   unsigned int GetAnimatedTs() { return mTs; }
   void SetOption(const std::string & in_Name, const bool & in_Value);
   bool HasOption(const std::string & in_Name);
   bool GetOption(const std::string & in_Name);
   size_t GetNbObjects() { return mObjects.size(); }
   Alembic::Abc::Box3d &GetArchiveBBox() { return m_Archivebbox; }

 
   bool PreProcess();
   bool Process(double frame);
};

#endif