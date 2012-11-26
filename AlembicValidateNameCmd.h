#ifndef _ALEMBIC_VALIDATE_NAME_H_
#define _ALEMBIC_VALIDATE_NAME_H_

  #include "AlembicObject.h"

	/*
  class AlembicValidateNameCommand : public MPxCommand
  {
  public:
    AlembicValidateNameCommand() {}
    virtual ~AlembicValidateNameCommand()  {}

    virtual bool isUndoable() const { return false; }
    MStatus doIt(const MArgList& args);

    static MSyntax createSyntax();
    static void* creator() { return new AlembicValidateNameCommand(); }
  };
	//*/

  class AlembicAssignFacesetCommand : public MPxCommand
  {
  public:
    AlembicAssignFacesetCommand() {}
    virtual ~AlembicAssignFacesetCommand()  {}

    virtual bool isUndoable() const { return false; }
    MStatus doIt(const MArgList& args);

    static MSyntax createSyntax();
    static void* creator() { return new AlembicAssignFacesetCommand(); }
  };

  class AlembicAssignInitialSGCommand : public MPxCommand
  {
  public:
    AlembicAssignInitialSGCommand() {}
    virtual ~AlembicAssignInitialSGCommand()  {}

    virtual bool isUndoable() const { return false; }
    MStatus doIt(const MArgList& args);

    static MSyntax createSyntax();
    static void* creator() { return new AlembicAssignInitialSGCommand(); }
  };

	/*
  class AlembicFileRefCountCommand : public MPxCommand
  {
  public:
    AlembicFileRefCountCommand() {}
    virtual ~AlembicFileRefCountCommand()  {}

    virtual bool isUndoable() const { return false; }
    MStatus doIt(const MArgList& args);

    static MSyntax createSyntax();
    static void* creator() { return new AlembicFileRefCountCommand(); }
  };
	//*/

#endif
