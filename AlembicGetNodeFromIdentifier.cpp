#include "stdafx.h"
#include "AlembicGetNodeFromIdentifier.h"

/*
AlembicGetNodeFromIdentifierCommand::AlembicGetNodeFromIdentifierCommand()
{
}

AlembicGetNodeFromIdentifierCommand::~AlembicGetNodeFromIdentifierCommand()
{
}

MSyntax AlembicGetNodeFromIdentifierCommand::createSyntax()
{
   MSyntax syntax;
   syntax.addFlag("-h", "-help");
   syntax.addFlag("-i", "-identifierArg", MSyntax::kString);
   syntax.enableQuery(false);
   syntax.enableEdit(false);

   return syntax;
}

void* AlembicGetNodeFromIdentifierCommand::creator()
{
   return new AlembicGetNodeFromIdentifierCommand();
}

MStatus AlembicGetNodeFromIdentifierCommand::doIt(const MArgList & args)
{
   ESS_PROFILE_SCOPE("AlembicGetNodeFromIdentifierCommand::doIt");

   MStatus status = MS::kSuccess;
   MArgParser argData(syntax(), args, &status);

   if (argData.isFlagSet("help"))
   {
      MGlobal::displayInfo("[ExocortexAlembic]: ExocortexAlembic_getNodeFromIdentifier command:");
      MGlobal::displayInfo("                    -i : provide an identifier(string)");
      return MS::kFailure;
   }

   if(!argData.isFlagSet("identifierArg"))
   {
      // TODO: display dialog
      MGlobal::displayError("[ExocortexAlembic] No identifier specified.");
      return MS::kFailure;
   }

   // get the filename arg
   MString identifier = argData.flagArgumentString("identifierArg",0);
   MString fullName = getFullNameFromIdentifier(identifier.asChar());

   // set the return value
   setResult(fullName);

   return status;
}
//*/

