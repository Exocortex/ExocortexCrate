#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "Foundation.h"
#include "CommonUtilities.h"

std::string getIdentifierFromRef(const MObject & in_Ref);
MString removeTrailFromName(MString & name);
MString truncateName(const MString & in_Name);
MString injectShapeToName(const MString & in_Name);
MString getFullNameFromRef(const MObject & in_Ref);
MString getFullNameFromIdentifier(std::string in_Identifier);
MObject getRefFromFullName(const MString & in_Path);
MObject getRefFromIdentifier(std::string in_Identifier);
bool isRefAnimated(const MObject & in_Ref);
bool returnIsRefAnimated(const MObject & in_Ref, bool animated);
void clearIsRefAnimatedCache();

// remapping imported names
void nameMapAdd(MString identifier, MString name);
MString nameMapGet(MString identifier);
void nameMapClear();

// utility mappings
MString getTypeFromObject(Alembic::Abc::IObject object);

// transform wranglers
Alembic::Abc::M44f GetGlobalMatrix(const MObject & in_Ref);

// metadata related
class AlembicObject;


class AlembicResolvePathCommand : public MPxCommand
{
  public:
    AlembicResolvePathCommand() {}
    virtual ~AlembicResolvePathCommand()  {}

    virtual bool isUndoable() const { return false; }
    MStatus doIt(const MArgList& args);

    static MSyntax createSyntax();
    static void* creator() { return new AlembicResolvePathCommand(); }
};


#endif  // _FOUNDATION_H_
