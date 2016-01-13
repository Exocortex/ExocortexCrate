#include "stdafx.h"

#include "AlembicValidateNameCmd.h"

#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MItMeshPolygon.h>

/// AlembicAssignFacesetCommand
MSyntax AlembicAssignFacesetCommand::createSyntax()
{
  MSyntax syntax;
  syntax.addFlag("-h", "-help");
  syntax.addFlag("-a", "-attribute", MSyntax::kString);
  syntax.addFlag("-m", "-mesh", MSyntax::kString);
  // syntax.addFlag("-x", "-xset", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);

  return syntax;
}

MStatus AlembicAssignFacesetCommand::doIt(const MArgList& args)
{
  ESS_PROFILE_SCOPE("AlembicAssignFacesetCommand::doIt");

  MStatus status;
  MArgParser argData(syntax(), args, &status);

  if (argData.isFlagSet("help")) {
    MGlobal::displayInfo(
        "[ExocortexAlembic]: ExocortexAlembic_assignFaceset command:");
    MGlobal::displayInfo(
        "                    -a : attribute of the shape with the faceset "
        "indices");
    MGlobal::displayInfo(
        "                    -m : mesh to assign the faceset on");
    // MGlobal::displayInfo("                    -s : set to assign the faceset
    // on");
    return MS::kSuccess;
  }

  /*
  if (!argData.isFlagSet("xset"))
  {
    MGlobal::displayError("[ExocortexAlembic]: ExocortexAlembic_assignFaceSets
  command missing set");
    return MS::kFailure;
  }
  //*/

  if (!argData.isFlagSet("attribute")) {
    MGlobal::displayError(
        "[ExocortexAlembic]: ExocortexAlembic_assignFaceSets command missing "
        "attribute");
    return MS::kFailure;
  }

  if (!argData.isFlagSet("mesh")) {
    MGlobal::displayError(
        "[ExocortexAlembic]: ExocortexAlembic_assignFaceSets command missing "
        "mesh");
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
  if (status != MS::kSuccess) {
    MGlobal::displayError("invalid mesh shape");
    return MS::kFailure;
  }

  // check if the attribute is valid
  argstr = argData.flagArgumentString("attribute", 0);
  MObject attr = fnMesh.attribute(argstr);
  MFnAttribute mfnAttr(attr);
  MPlug plug = fnMesh.findPlug(attr, true);

  if (!mfnAttr.isReadable() || plug.isNull()) {
    MGlobal::displayError("invalid attribute");
    return MS::kFailure;
  }

  MFnIntArrayData arr(plug.asMObject(), &status);  // read the attribute
  if (status != MS::kSuccess || arr.length() == 0) {
    MGlobal::displayError("invalid attribute");
    return MS::kFailure;
  }

  {
    MObject oMesh = fnMesh.object();
    MSelectionList faces;
    int pol_idx = 0, facesetIdx = 0;
    for (MItMeshPolygon iter(oMesh); !iter.isDone(); iter.next(), ++pol_idx) {
      if (pol_idx == arr[facesetIdx]) {
        faces.add(iter.currentItem());
        ++facesetIdx;

        if (facesetIdx == arr.length()) {
          break;
        }
      }
    }

    MFnSet fnSet;
    MObject oSet = fnSet.create(sl, MFnSet::kFacetsOnly, &status);
    if (!status) {
      MGlobal::displayError("invalid set: " + status.errorString());
      return MS::kFailure;
    }
    setResult(fnSet.name());
  }
  return MS::kSuccess;
}

/// AlembicAssignFacesetCommand

static MStatus getObjectByName(const MString& name, MObject& object)
{
  object = MObject::kNullObj;

  MSelectionList sList;
  MStatus status = sList.add(name);
  if (status == MS::kSuccess) {
    status = sList.getDependNode(0, object);
  }
  return status;
}

static MStatus getDagPathByName(const MString& name, MDagPath& dagPath)
{
  MSelectionList sList;
  MStatus status = sList.add(name);
  if (status == MS::kSuccess) {
    status = sList.getDagPath(0, dagPath);
  }
  return status;
}

MSyntax AlembicAssignInitialSGCommand::createSyntax()
{
  MSyntax syntax;
  syntax.addFlag("-h", "-help");
  syntax.addFlag("-m", "-mesh", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);

  return syntax;
}

MStatus AlembicAssignInitialSGCommand::doIt(const MArgList& args)
{
  MStatus status;
  MArgParser argData(syntax(), args, &status);

  if (argData.isFlagSet("help")) {
    MGlobal::displayInfo(
        "[ExocortexAlembic]: ExocortexAlembic_assignFaceset command:");
    MGlobal::displayInfo(
        "                    -m : mesh to assign the initialShadingGroup on");
    return MS::kSuccess;
  }

  if (!argData.isFlagSet("mesh")) {
    MGlobal::displayError("No mesh/subdiv specified!");
    return MS::kFailure;
  }

  MObject initShader;
  MDagPath dagPath;

  if (getObjectByName("initialShadingGroup", initShader) == MS::kSuccess &&
      getDagPathByName(argData.flagArgumentString("mesh", 0), dagPath) ==
          MS::kSuccess) {
    ESS_PROFILE_SCOPE("AlembicAssignInitialSGCommand::doIt::MFnSet");
    MFnSet set(initShader);
    set.addMember(dagPath);
  }
  else {
    MString theError("Error getting adding ");
    theError += argData.flagArgumentString("mesh", 0);
    theError += MString(" to initalShadingGroup.");
    MGlobal::displayError(theError);
    return MS::kFailure;
  }
  return MS::kSuccess;
}

MSyntax AlembicPolyMeshToSubdivCommand::createSyntax()
{
  MSyntax syntax;
  syntax.addFlag("-h", "-help");
  syntax.addFlag("-m", "-mesh", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);

  return syntax;
}

MStatus AlembicPolyMeshToSubdivCommand::doIt(const MArgList& args)
{
  MStatus status;
  MArgParser argData(syntax(), args, &status);

  if (argData.isFlagSet("help")) {
    MGlobal::displayInfo(
        "[ExocortexAlembic]: ExocortexAlembic_meshToSubdiv command:");
    MGlobal::displayInfo(
        "                    -m : mesh to assign the initialShadingGroup on");
    return MS::kSuccess;
  }

  const MString mesh =
      argData.isFlagSet("mesh")
          ? ("\"" + argData.flagArgumentString("mesh", 0) + "\"")
          : "";

  MString result;
  MGlobal::executePythonCommand(
      ("ExoAlembic._functions.alembicPolyMeshToSubdiv(" + mesh) + ")", result);
  if (result.length()) {
    MPxCommand::setResult(result);
    return MS::kFailure;
  }
  return MS::kSuccess;
}
