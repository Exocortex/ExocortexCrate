#ifndef _METADATA_H_
#define _METADATA_H_

#include "Foundation.h"
#include "AlembicObject.h"

bool SaveMetaData(AlembicObject * obj);

class AlembicCreateMetaDataCommand : public MPxCommand
{
  public:
    AlembicCreateMetaDataCommand() {}
    virtual ~AlembicCreateMetaDataCommand()  {}

    virtual bool isUndoable() const { return false; }
    MStatus doIt(const MArgList& args);

    static MSyntax createSyntax();
    static void* creator() { return new AlembicCreateMetaDataCommand(); }
};

#endif  // _FOUNDATION_H_
