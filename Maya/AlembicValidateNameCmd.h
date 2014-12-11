#ifndef _ALEMBIC_VALIDATE_NAME_H_
#define _ALEMBIC_VALIDATE_NAME_H_

  #include "AlembicObject.h"

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

	class AlembicPolyMeshToSubdivCommand: public MPxCommand
	{
	public:
		AlembicPolyMeshToSubdivCommand(void) {}
		virtual ~AlembicPolyMeshToSubdivCommand(void) {}

		virtual bool isUndoable(void) const { return false; }
		MStatus doIt(const MArgList &args);

		static MSyntax createSyntax();
		static void* creator() { return new AlembicPolyMeshToSubdivCommand(); }
	};

#endif
