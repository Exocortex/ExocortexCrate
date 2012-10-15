#include "AlembicValidateNameCmd.h"

MSyntax AlembicValidateNameCommand::createSyntax()
{
   MSyntax syntax;
   syntax.addFlag("-h", "-help");
   syntax.addFlag("-i", "-identifier", MSyntax::kString);
   syntax.enableQuery(false);
   syntax.enableEdit(false);

   return syntax;
}

MStatus AlembicValidateNameCommand::doIt(const MArgList& args)
{
  MStatus status;
  MArgParser argData(syntax(), args, &status);

  if (argData.isFlagSet("help"))
  {
    MGlobal::displayInfo("[ExocortexAlembic]: ExocortexAlembic_createValidName command:");
    MGlobal::displayInfo("                    -i : create a new valid maya name for this identifier");
    return MS::kSuccess;
  }

  if (!argData.isFlagSet("identifier"))
  {
    MGlobal::displayError("[ExocortexAlembic]: ExocortexAlembic_createValidName command missing identifier name");
    return MS::kFailure;
  }

  MString id = argData.flagArgumentString("identifier",0);
  std::string identifier(id.asChar());
  identifier = removeInvalidCharacter(identifier);
  setResult(identifier.c_str());
  return MS::kSuccess;
}