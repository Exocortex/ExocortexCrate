#include "sceneGraph.h"
#include "stdafx.h"

#include "CommonLog.h"

#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>

static MString __PythonBool[] = {MString("False"), MString("True")};
static inline const MString &PythonBool(bool b)
{
  return __PythonBool[b ? 1 : 0];
}

static void __file_and_time_control_kill(const MString &var)
{
  MGlobal::executePythonCommand(
      var + " = None\n");  // deallocate this variable in Python!
}

AlembicFileAndTimeControl::~AlembicFileAndTimeControl(void)
{
  __file_and_time_control_kill(var);
}

AlembicFileAndTimeControlPtr AlembicFileAndTimeControl::createControl(
    const IJobStringParser &jobParams)
{
  static unsigned int numberOfControlCreated = 0;
  static const MString format(
      "^1s = ExoAlembic._import.IJobInfo(r\"^2s\", ^3s, ^4s, ^5s, ^6s)\n");

  ESS_PROFILE_SCOPE("AlembicFileAndTimeControl::createControl");
  MString var("__alembic_file_and_time_control_tuple_");
  var += (++numberOfControlCreated);

  MString cmd;
  cmd.format(
      format, var, jobParams.filename.c_str(),
      PythonBool(jobParams.importNormals), PythonBool(jobParams.importUVs),
      PythonBool(jobParams.importFacesets), PythonBool(jobParams.useMultiFile));
  MStatus status = MGlobal::executePythonCommand(cmd);
  if (status != MS::kSuccess) {
    __file_and_time_control_kill(var);
    return AlembicFileAndTimeControlPtr();
  }
  return AlembicFileAndTimeControlPtr(new AlembicFileAndTimeControl(var));
}

bool SceneNodeMaya::replaceSimilarData(const char *functionName,
                                       SceneNodeAlembicPtr fileNode)
{
  static const MString format(
      "ExoAlembic._attach.attach^1s(r\"^2s\", r\"^3s\", ^4s, ^5s)");

  ESS_PROFILE_SCOPE("SceneNodeMaya::replaceSimilarData");

  MString cmd;
  cmd.format(format, functionName, this->dccIdentifier.c_str(),
             fileNode->dccIdentifier.c_str(), fileAndTime->variable(),
             PythonBool(fileNode->pObjCache->isConstant));
  MStatus result = MGlobal::executePythonCommand(cmd);
  if (result.error()) {
    ESS_LOG_WARNING("Attached failed for " << functionName << ", reason: "
                                           << result.errorString().asChar());
    return false;
  }

  fileNode->setAttached(true);
  return true;
}

bool SceneNodeMaya::replaceData(SceneNodeAlembicPtr fileNode,
                                const IJobStringParser &jobParams,
                                SceneNodeAlembicPtr &nextFileNode)
{
  ESS_PROFILE_SCOPE("SceneNodeMaya::replaceData");
  nextFileNode = fileNode;
  switch (nextFileNode->type) {
    case ETRANSFORM:
    case ITRANSFORM:
      return replaceSimilarData("Xform", fileNode);
    case CAMERA:
      return replaceSimilarData("Camera", fileNode);
    case POLYMESH:
    case SUBD:
      return replaceSimilarData("PolyMesh", fileNode);
    case CURVES:
    case HAIR:
      return replaceSimilarData("Curves", fileNode);
    case PARTICLES:
      return replaceSimilarData("Points", fileNode);
    // case SURFACE:	// handle as default for now
    // break;
    // case LIGHT:	// handle as default for now
    // break;
    default:
      break;
  }
  return false;
}

bool SceneNodeMaya::executeAddChild(const MString &cmd,
                                    SceneNodeAppPtr &newAppNode)
{
  MString result;
  MGlobal::executePythonCommand(cmd, result);
  if (result.length() == 0)
    return false;
  else if (result.asChar()[0] == '?') {
#ifdef _DEBUG
    MGlobal::displayError(result);
    MGlobal::displayError(cmd);
#endif
    return false;
  }

  newAppNode->dccIdentifier = result.asChar();
  newAppNode->name = newAppNode->dccIdentifier;
  return true;
}

bool SceneNodeMaya::addSimilarChild(const char *functionName,
                                    SceneNodeAlembicPtr fileNode,
                                    SceneNodeAppPtr &newAppNode)
{
  static const MString format(
      "ExoAlembic._import.import^1s(r\"^2s\", r\"^3s\", ^4s, r\"^5s\", ^6s)");

  ESS_PROFILE_SCOPE("SceneNodeMaya::addSimilarChild");
  MString cmd;
  cmd.format(
      format, functionName, fileNode->name.c_str(),
      fileNode->dccIdentifier.c_str(), fileAndTime->variable(),
      dccIdentifier.c_str(),
      PythonBool(this->useMultiFile ? false : fileNode->pObjCache->isConstant));
  return executeAddChild(cmd, newAppNode);
}

bool SceneNodeMaya::addXformChild(SceneNodeAlembicPtr fileNode,
                                  SceneNodeAppPtr &newAppNode)
{
  static const MString format(
      "ExoAlembic._import.importXform(r\"^1s\", r\"^2s\", ^3s, ^4s, ^5s)");

  ESS_PROFILE_SCOPE("SceneNodeMaya::addXformChild");
  MString parent;
  switch (type) {
    case ETRANSFORM:
    case ITRANSFORM:
      parent.format("\"^1s\"", dccIdentifier.c_str());
      break;
    default:
      parent = "None";
      break;
  }

  MString cmd;
  cmd.format(format, fileNode->name.c_str(), fileNode->dccIdentifier.c_str(),
             fileAndTime->variable(), parent,
             PythonBool(fileNode->pObjCache->isConstant));
  return executeAddChild(cmd, newAppNode);
}

bool SceneNodeMaya::addPolyMeshChild(SceneNodeAlembicPtr fileNode,
                                     SceneNodeAppPtr &newAppNode)
{
  static const MString format(
      "ExoAlembic._import.importPolyMesh(r\"^1s\", r\"^2s\", ^3s, \"^4s\", "
      "^5s, ^6s)");

  ESS_PROFILE_SCOPE("SceneNodeMaya::addPolyMeshChild");
  MString cmd;
  cmd.format(format, fileNode->name.c_str(), fileNode->dccIdentifier.c_str(),
             fileAndTime->variable(), dccIdentifier.c_str(),
             PythonBool(fileNode->pObjCache->isConstant),
             PythonBool(fileNode->pObjCache->isMeshTopoDynamic));
  return executeAddChild(cmd, newAppNode);
}

bool SceneNodeMaya::addCurveChild(SceneNodeAlembicPtr fileNode,
                                  SceneNodeAppPtr &newAppNode)
{
  static const MString format(
      "ExoAlembic._import.importCurves(r\"^1s\", r\"^2s\", ^3s, r\"^4s\", ^5s, "
      "^6s)");

  MString strNb;
  {
    AbcG::ICurvesSchema::Sample sample;
    AbcG::ICurves obj(fileNode->getObject(), Abc::kWrapExisting);
    obj.getSchema().get(sample, 0);

    strNb += (unsigned int)sample.getCurvesNumVertices()->size();
  }

  MString cmd;
  cmd.format(format, fileNode->name.c_str(), fileNode->dccIdentifier.c_str(),
             fileAndTime->variable(), dccIdentifier.c_str(),
             PythonBool(fileNode->pObjCache->isConstant), strNb);
  return executeAddChild(cmd, newAppNode);
}

bool SceneNodeMaya::addChild(SceneNodeAlembicPtr fileNode,
                             const IJobStringParser &jobParams,
                             SceneNodeAppPtr &newAppNode)
{
  ESS_PROFILE_SCOPE("SceneNodeMaya::addChild");
  this->useMultiFile = jobParams.useMultiFile;
  newAppNode.reset(new SceneNodeMaya(fileAndTime));
  switch (newAppNode->type = fileNode->type) {
    case ETRANSFORM:
    case ITRANSFORM:
      return addXformChild(fileNode, newAppNode);
    case CAMERA:
      return addSimilarChild("Camera", fileNode, newAppNode);
    case POLYMESH:
    case SUBD:
      return addPolyMeshChild(fileNode, newAppNode);
    case CURVES:
    case HAIR:
      return addCurveChild(fileNode, newAppNode);
    case PARTICLES:
      return addSimilarChild("Points", fileNode, newAppNode);
    // case SURFACE:	// handle as default for now
    // break;
    // case LIGHT:	// handle as default for now
    // break;
    default:
      break;
  }
  return false;
}

void SceneNodeMaya::print(void)
{
  ESS_LOG_WARNING("Maya Scene Node: " << dccIdentifier);
}

static bool visitChild(
    const MObject &mObj, SceneNodeAppPtr &parent,
    const AlembicFileAndTimeControlPtr &alembicFileAndTimeControl,
    const SearchReplace::ReplacePtr &replacer)
{
  MFnDagNode dagNode(mObj);
  if (dagNode.isIntermediateObject())  // skip intermediate object!
    return true;

  const std::string dccId = dagNode.fullPathName().asChar();
  if (dccId.length() == 0) return true;

  // check if it's a valid type of node first!
  SceneNodeAppPtr exoChild(new SceneNodeMaya(alembicFileAndTimeControl));
  switch (mObj.apiType()) {
    case MFn::kCamera:
      exoChild->type = SceneNode::CAMERA;
      break;
    case MFn::kMesh:
      exoChild->type = SceneNode::POLYMESH;
      break;
    case MFn::kSubdiv:
      exoChild->type = SceneNode::SUBD;
      break;
    case MFn::kNurbsCurve:
      exoChild->type = SceneNode::CURVES;
      break;
    case MFn::kParticle:
      exoChild->type = SceneNode::PARTICLES;
      break;
    case MFn::kPfxHair:
      exoChild->type = SceneNode::HAIR;
      break;
    case MFn::kTransform:
    default:
      exoChild->type = SceneNode::ETRANSFORM;
      break;
  }

  parent->children.push_back(exoChild);
  exoChild->parent = parent.get();

  exoChild->dccIdentifier = dccId;
  {
    const std::string rname =
        replacer->replace(dagNode.fullPathName().asChar());
    const size_t pos = rname.rfind("|");
    exoChild->name = (pos != std::string::npos) ? rname.substr(pos + 1) : rname;
  }
  for (unsigned int i = 0; i < dagNode.childCount(); ++i)
    visitChild(dagNode.child(i), exoChild, alembicFileAndTimeControl, replacer);
  return true;
}

SceneNodeAppPtr buildMayaSceneGraph(
    const MDagPath &dagPath, const SearchReplace::ReplacePtr &replacer,
    const AlembicFileAndTimeControlPtr alembicFileAndTimeControl)
{
  ESS_PROFILE_SCOPE("buildMayaSceneGraph");
  SceneNodeAppPtr exoRoot(new SceneNodeMaya(alembicFileAndTimeControl));
  exoRoot->type = SceneNode::SCENE_ROOT;
  exoRoot->dccIdentifier = dagPath.fullPathName().asChar();
  exoRoot->name = dagPath.partialPathName().asChar();
  for (unsigned int i = 0; i < dagPath.childCount(); ++i)
    visitChild(dagPath.child(i), exoRoot, alembicFileAndTimeControl, replacer);
  return exoRoot;
}
