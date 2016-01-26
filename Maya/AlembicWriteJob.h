#ifndef _ALEMBIC_WRITE_JOB_H_
#define _ALEMBIC_WRITE_JOB_H_

#include <set>

#include "AlembicObject.h"

#include "CommonRegex.h"

typedef std::pair<std::string, AlembicObjectPtr> pairStrAbcObj;
typedef std::multimap<std::string, AlembicObjectPtr> multiMapStrAbcObj;

class AlembicWriteJob {
 private:
  MString mFileName;
  MObjectArray mSelection;
  std::vector<double> mFrames;
  Abc::OArchive mArchive;
  Abc::OObject mTop;
  unsigned int mTs;
  std::map<std::string, std::string> mOptions;
  bool useOgawa;
  std::vector<std::string> mPrefixFilters;
  std::set<std::string> mAttributes;
  std::vector<std::string> mUserPrefixFilters;
  std::set<std::string> mUserAttributes;

  multiMapStrAbcObj mapObjects;
  double mFrameRate;

  void createArchive(
      const char* sceneFileName);  // initialize mArchive with HDF5 or Ogawa!

 public:
  AlembicWriteJob(const MString& in_FileName, const MObjectArray& in_Selection,
                  const MDoubleArray& in_Frames, bool useOgawa,
                  const std::vector<std::string>& in_prefixFilters,
                  const std::set<std::string>& in_attributes,
                  const std::vector<std::string>& in_userPrefixFilters,
                  const std::set<std::string>& in_userAttributes);
  ~AlembicWriteJob();

  // to change the name of objects while exporting!
  SearchReplace::ReplacePtr replacer;

  Abc::OArchive GetArchive() { return mArchive; }
  Abc::OObject GetTop() { return mTop; }
  const std::vector<double>& GetFrames() { return mFrames; }
  const MString& GetFileName() { return mFileName; }
  unsigned int GetAnimatedTs() { return mTs; }
  const std::vector<std::string>& GetPrefixFilters() const { return mPrefixFilters; }
  const std::set<std::string>& GetAttributes() const { return mAttributes; }
  const std::vector<std::string>& GetUserPrefixFilters() const { return mUserPrefixFilters; }
  const std::set<std::string>& GetUserAttributes() const { return mUserAttributes; }
  void SetOption(const MString& in_Name, const MString& in_Value);
  bool HasOption(const MString& in_Name);
  MString GetOption(const MString& in_Name);

  bool ObjectExists(const MObject& in_Ref);
  bool AddObject(AlembicObjectPtr in_Obj);
  size_t GetNbObjects() { return mapObjects.size(); }
  MStatus PreProcess();
  MStatus Process(double frame);
  bool forceCloseArchive(void);
};

class AlembicExportCommand : public MPxCommand {
 public:
  AlembicExportCommand();
  virtual ~AlembicExportCommand();

  virtual bool isUndoable() const { return false; }
  MStatus doIt(const MArgList& args);

  static MSyntax createSyntax();
  static void* creator();
};

#endif
