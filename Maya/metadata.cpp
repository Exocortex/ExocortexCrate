#include "stdafx.h"

#include "MetaData.h"

bool SaveMetaData(AlembicObject* obj)
{
  ESS_PROFILE_SCOPE("SaveMetaData");
  if (obj->GetNumSamples() > 0) {
    return false;
  }

  MFnDependencyNode node(obj->GetRef());

  std::vector<std::string> metaData;
  for (int i = 0; i < 10; i++) {
    MString index;
    index.set((double)i, 0);
    MObject labelAttribute = node.attribute("md_label" + index);
    if (labelAttribute.isNull()) {
      return false;
    }
    MPlug labelPlug(obj->GetRef(), labelAttribute);
    MString labelString;
    labelPlug.getValue(labelString);
    metaData.push_back(labelString.asChar());
    MObject valueAttribute = node.attribute("md_value" + index);
    if (valueAttribute.isNull()) {
      return false;
    }
    MPlug valuePlug(obj->GetRef(), valueAttribute);
    MString valueString;
    valuePlug.getValue(valueString);
    metaData.push_back(valueString.asChar());
  }

  // let's create the metadata property
  Abc::OStringArrayProperty metaDataProperty = Abc::OStringArrayProperty(
      obj->GetCompound(), ".metadata", obj->GetCompound().getMetaData(),
      obj->GetJob()->GetAnimatedTs());

  Abc::StringArraySample metaDataSample(&metaData.front(), metaData.size());
  metaDataProperty.set(metaDataSample);

  return true;
}

MSyntax AlembicCreateMetaDataCommand::createSyntax()
{
  MSyntax syntax;
  syntax.addFlag("-h", "-help");
  syntax.addFlag("-f", "-fileNameArg", MSyntax::kString);
  syntax.addFlag("-i", "-identifierArg", MSyntax::kString);
  syntax.addFlag("-o", "-objectArg", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);

  return syntax;
}

MStatus AlembicCreateMetaDataCommand::doIt(const MArgList& args)
{
  ESS_PROFILE_SCOPE("AlembicCreateMetaDataCommand::doIt");
  MStatus status = MS::kSuccess;
  MArgParser argData(syntax(), args, &status);

  if (argData.isFlagSet("help")) {
    MGlobal::displayInfo(
        "[ExocortexAlembic]: ExocortexAlembic_createMetaData command:");
    MGlobal::displayInfo(
        "                    -f : provide an unresolved fileName (string)");
    MGlobal::displayInfo(
        "                    -i : provide an identifier inside the file");
    MGlobal::displayInfo(
        "                    -o : provide an object to create the meta data "
        "on");
    return MS::kSuccess;
  }

  if (!argData.isFlagSet("objectArg")) {
    MGlobal::displayError("[ExocortexAlembic] No objectArg specified.");
    return MStatus::kFailure;
  }
  MString objectPath = argData.flagArgumentString("objectArg", 0);
  MObject nodeObject = getRefFromFullName(objectPath);
  if (nodeObject.isNull()) {
    MGlobal::displayError("[ExocortexAlembic] Invalid objectArg specified.");
    return MStatus::kFailure;
  }
  MFnDagNode node(nodeObject);

  MString fileName;
  if (argData.isFlagSet("fileNameArg")) {
    fileName = argData.flagArgumentString("fileNameArg", 0);
  }
  MString identifier;
  if (argData.isFlagSet("identifierArg")) {
    identifier = argData.flagArgumentString("identifierArg", 0);
  }

  if (fileName.length() > 0) {
    fileName = resolvePath(fileName);
  }

  MStringArray metadata;
  if (fileName.length() > 0 && identifier.length() > 0) {
    addRefArchive(fileName);
    Abc::IObject object = getObjectFromArchive(fileName, identifier);
    if (!object.valid()) {
      MGlobal::displayError(
          "[ExocortexAlembic] Invalid fileName or identifier specified.");
      delRefArchive(fileName);
      return MStatus::kFailure;
    }

    // check the metadata
    if (getCompoundFromObject(object).getPropertyHeader(".metadata") != NULL) {
      Abc::IStringArrayProperty metaDataProp =
          Abc::IStringArrayProperty(getCompoundFromObject(object), ".metadata");
      Abc::StringArraySamplePtr ptr = metaDataProp.getValue(0);
      metadata.setLength((unsigned int)ptr->size());
      for (unsigned int i = 0; i < metadata.length(); i++) {
        metadata[i] = ptr->get()[i].c_str();
      }
    }
    else {
      // if we don't have metadata, let's just roll
      delRefArchive(fileName);
      return status;
    }
    delRefArchive(fileName);
  }

  MFnTypedAttribute tAttr;

  // check each parameter for existance
  for (int i = 0; i < 10; i++) {
    MString index;
    index.set((double)i, 0);
    MObject labelAttribute = node.attribute("md_label" + index);
    if (labelAttribute.isNull()) {
      labelAttribute = tAttr.create(
          "md_label" + index, "MetaData Label " + index, MFnData::kString);
      tAttr.setStorable(true);
      tAttr.setKeyable(false);
      node.addAttribute(labelAttribute, MFnDependencyNode::kLocalDynamicAttr);
    }
    MPlug labelPlug(nodeObject, labelAttribute);
    MObject valueAttribute = node.attribute("md_value" + index);
    if (valueAttribute.isNull()) {
      valueAttribute = tAttr.create(
          "md_value" + index, "MetaData Value " + index, MFnData::kString);
      tAttr.setStorable(true);
      tAttr.setKeyable(false);
      node.addAttribute(valueAttribute, MFnDependencyNode::kLocalDynamicAttr);
    }
    MPlug valuePlug(nodeObject, valueAttribute);

    // if we have metadata
    if (metadata.length() > 0) {
      // set the values
      labelPlug.setString(metadata[i * 2 + 0]);
      valuePlug.setString(metadata[i * 2 + 1]);
    }
  }

  return status;
}
