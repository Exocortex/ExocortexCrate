#ifndef _ALEMBIC_GETINFO_H_
#define _ALEMBIC_GETINFO_H_

#include "Foundation.h"

class AlembicGetInfoCommand : public MPxCommand
{
  public:
    AlembicGetInfoCommand();
    virtual ~AlembicGetInfoCommand();

    virtual bool isUndoable() const { return false; }
    MStatus doIt(const MArgList& args);

    static MSyntax createSyntax();
    static void* creator();
};

#endif