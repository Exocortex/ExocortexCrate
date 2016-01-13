#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "CommonProfiler.h"
#include "CommonUtilities.h"

std::string getIdentifierFromRef(const MObject& in_Ref);
std::string removeInvalidCharacter(const std::string& str,
                                   bool keepSemi = false);
MString removeTrailFromName(MString& name);
MString truncateName(const MString& in_Name);
MString injectShapeToName(const MString& in_Name);
MString getFullNameFromRef(const MObject& in_Ref);
MString getFullNameFromIdentifier(std::string in_Identifier);
MObject getRefFromFullName(const MString& in_Path);
MObject getRefFromIdentifier(std::string in_Identifier);
bool isRefAnimated(const MObject& in_Ref);
bool returnIsRefAnimated(const MObject& in_Ref, bool animated);
void clearIsRefAnimatedCache();

// remapping imported names
void nameMapAdd(MString identifier, MString name);
MString nameMapGet(MString identifier);
void nameMapClear();

// utility mappings
enum ALEMBIC_TYPE {
  AT_Xform,
  AT_PolyMesh,
  AT_Curves,
  AT_NuPatch,
  AT_Points,
  AT_SubD,
  AT_Camera,
  AT_Group,
  AT_UNKNOWN,

  AT_NB_ALEMBIC_TYPES
};
ALEMBIC_TYPE getAlembicTypeFromObject(Abc::IObject object);
std::string alembicTypeToString(ALEMBIC_TYPE at);
MString getTypeFromObject(Abc::IObject object);

// transform wranglers
Abc::M44f GetGlobalMatrix(const MObject& in_Ref);

// metadata related
class AlembicObject;

class AlembicResolvePathCommand : public MPxCommand {
 public:
  AlembicResolvePathCommand() {}
  virtual ~AlembicResolvePathCommand() {}
  virtual bool isUndoable() const { return false; }
  MStatus doIt(const MArgList& args);

  static MSyntax createSyntax();
  static void* creator() { return new AlembicResolvePathCommand(); }
};

class AlembicProfileBeginCommand : public MPxCommand {
 public:
  AlembicProfileBeginCommand() {}
  virtual ~AlembicProfileBeginCommand() {}
  virtual bool isUndoable() const { return false; }
  MStatus doIt(const MArgList& args);

  static MSyntax createSyntax();
  static void* creator() { return new AlembicProfileBeginCommand(); }
};

class AlembicProfileEndCommand : public MPxCommand {
 public:
  AlembicProfileEndCommand() {}
  virtual ~AlembicProfileEndCommand() {}
  virtual bool isUndoable() const { return false; }
  MStatus doIt(const MArgList& args);

  static MSyntax createSyntax();
  static void* creator() { return new AlembicProfileEndCommand(); }
};

class AlembicProfileStatsCommand : public MPxCommand {
 public:
  AlembicProfileStatsCommand() {}
  virtual ~AlembicProfileStatsCommand() {}
  virtual bool isUndoable() const { return false; }
  MStatus doIt(const MArgList& args);

  static MSyntax createSyntax();
  static void* creator() { return new AlembicProfileStatsCommand(); }
};

class AlembicProfileResetCommand : public MPxCommand {
 public:
  AlembicProfileResetCommand() {}
  virtual ~AlembicProfileResetCommand() {}
  virtual bool isUndoable() const { return false; }
  MStatus doIt(const MArgList& args);

  static MSyntax createSyntax();
  static void* creator() { return new AlembicProfileResetCommand(); }
};

#endif  // _FOUNDATION_H_
