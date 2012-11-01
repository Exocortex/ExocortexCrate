#include "stdafx.h"
#include "AlembicValidateNameCmd.h"

#include <maya/MFnSet.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>

/// AlembicValidateNameCommand
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
  ESS_PROFILE_SCOPE("AlembicValidateNameCommand::doIt");

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
  identifier = removeInvalidCharacter(identifier, true);
  setResult(identifier.c_str());
  return MS::kSuccess;
}

/// AlembicAssignFacesetCommand
MSyntax AlembicAssignFacesetCommand::createSyntax()
{
   MSyntax syntax;
   syntax.addFlag("-h", "-help");
   syntax.addFlag("-a", "-attribute", MSyntax::kString);
   syntax.addFlag("-m", "-mesh", MSyntax::kString);
   //syntax.addFlag("-x", "-xset", MSyntax::kString);
   syntax.enableQuery(false);
   syntax.enableEdit(false);

   return syntax;
}

MStatus AlembicAssignFacesetCommand::doIt(const MArgList& args)
{
  ESS_PROFILE_SCOPE("AlembicAssignFacesetCommand::doIt");

  MStatus status;
  MArgParser argData(syntax(), args, &status);

  if (argData.isFlagSet("help"))
  {
    MGlobal::displayInfo("[ExocortexAlembic]: ExocortexAlembic_assignFaceset command:");
    MGlobal::displayInfo("                    -a : attribute of the shape with the faceset indices");
    MGlobal::displayInfo("                    -m : mesh to assign the faceset on");
    //MGlobal::displayInfo("                    -s : set to assign the faceset on");
    return MS::kSuccess;
  }

  /*
  if (!argData.isFlagSet("xset"))
  {
    MGlobal::displayError("[ExocortexAlembic]: ExocortexAlembic_assignFaceSets command missing set");
    return MS::kFailure;
  }
  //*/

  if (!argData.isFlagSet("attribute"))
  {
    MGlobal::displayError("[ExocortexAlembic]: ExocortexAlembic_assignFaceSets command missing attribute");
    return MS::kFailure;
  }

  if (!argData.isFlagSet("mesh"))
  {
    MGlobal::displayError("[ExocortexAlembic]: ExocortexAlembic_assignFaceSets command missing mesh");
    return MS::kFailure;
  }

  // load the mesh!
  MString argstr;
  MSelectionList sl;
  argstr = argData.flagArgumentString("mesh", 0);
  sl.add(argstr);
  MDagPath dagp;
  sl.getDagPath(0, dagp);
  MFnMesh fnMesh(dagp, &status);
  if (status != MS::kSuccess)
  {
    MGlobal::displayError("invalid mesh shape");
    return MS::kFailure;
  }

  // check if the attribute is valid
  argstr = argData.flagArgumentString("attribute", 0);
  MObject attr = fnMesh.attribute(argstr);
  MFnAttribute mfnAttr(attr);
  MPlug plug = fnMesh.findPlug(attr, true);

  if (!mfnAttr.isReadable() || plug.isNull())
  {
    MGlobal::displayError("invalid attribute");
    return MS::kFailure;
  }

  MFnIntArrayData arr(plug.asMObject(), &status);   // read the attribute
  if (status != MS::kSuccess || arr.length() == 0)
  {
    MGlobal::displayError("invalid attribute");
    return MS::kFailure;
  }

  {
    MObject oMesh = fnMesh.object();
    MSelectionList faces;
    int pol_idx = 0, facesetIdx = 0;
    for (MItMeshPolygon iter(oMesh); !iter.isDone(); iter.next(), ++pol_idx)
    {
      if (pol_idx == arr[facesetIdx])
      {
        faces.add(iter.currentItem());
        ++facesetIdx;

        if (facesetIdx == arr.length())
          break;
      }
    }

    MFnSet fnSet;
    MObject oSet = fnSet.create(sl, MFnSet::kFacetsOnly, &status);
    if (!status)
    {
      MGlobal::displayError("invalid set: " + status.errorString());
      return MS::kFailure;
    }
    setResult(fnSet.name());
  }
  return MS::kSuccess;
}

