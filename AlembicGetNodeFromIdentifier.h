#ifndef _ALEMBIC_GETNODEFROMIDENTIFIER_H_
#define _ALEMBIC_GETNODEFROMIDENTIFIER_H_

#include "Foundation.h"

class AlembicGetNodeFromIdentifierCommand : public MPxCommand
{
  public:
    AlembicGetNodeFromIdentifierCommand();
    virtual ~AlembicGetNodeFromIdentifierCommand();

    virtual bool isUndoable() const { return false; }
    MStatus doIt(const MArgList& args);

    static MSyntax createSyntax();
    static void* creator();
};

#endif