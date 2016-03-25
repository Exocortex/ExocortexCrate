#ifndef __MAYA_SCENE_GRAPH_H
#define __MAYA_SCENE_GRAPH_H

#include "CommonImport.h"
#include "CommonRegex.h"
#include "CommonSceneGraph.h"
#include "AttributesReading.h"

static inline const MString &PythonBool(bool b);

class AlembicFileAndTimeControl;
typedef boost::shared_ptr<AlembicFileAndTimeControl>
    AlembicFileAndTimeControlPtr;

// Will also hold all the informations about the IJobString necessary!
class AlembicFileAndTimeControl {
 private:
  MString var;

  AlembicFileAndTimeControl(const MString& variable) : var(variable) {}
 public:
  ~AlembicFileAndTimeControl(void);

  const MString& variable(void) const { return var; }
  static AlembicFileAndTimeControlPtr createControl(
      const IJobStringParser& jobParams);
};

MObject findMObjectByName(const MString &obj);

class SceneNodeMaya : public SceneNodeApp {
 private:
  AlembicFileAndTimeControlPtr fileAndTime;
  bool useMultiFile;

  bool removeProps(const MString &dccReaderIdentifier);
  bool connectProps(MFnDependencyNode &depNode,
      MFnDependencyNode &readerDepNode);

  template<typename OBJECT_TYPE, typename SCHEMA_TYPE>
    bool addAndConnectProps(const MString &dccIdentifier,
        const MString &dccReaderIdentifier,
        bool addPropsToShape)
    {
      MStatus status;
      MObject node = findMObjectByName(dccIdentifier);
      MObject readerNode = findMObjectByName(dccReaderIdentifier);

      MFnDependencyNode depNode(node, &status);
      MFnDependencyNode readerDepNode(readerNode, &status);

      MPlug fileNamePlug = readerDepNode.findPlug("fileName", &status);
      MPlug identifierPlug = readerDepNode.findPlug("identifier", &status);
      MPlug geomParamsPlug = readerDepNode.findPlug("ExocortexAlembic_GeomParams",
          &status);
      MPlug userAttrsPlug = readerDepNode.findPlug("ExocortexAlembic_UserAttributes",
          &status);

      MString fileName;
      status = fileNamePlug.getValue(fileName);
      MString identifier;
      status = identifierPlug.getValue(identifier);

      if (status != MStatus::kSuccess) {
        return false;
      }

      // Don't show these warnings; let the reader node display them
      Alembic::Abc::IObject iObj = getObjectFromArchive(fileName, identifier);
      if (!iObj.valid()) {
        return false;
      }

      OBJECT_TYPE obj = OBJECT_TYPE(iObj, Alembic::Abc::kWrapExisting);
      if (!obj.valid()) {
        return false;
      }

      SCHEMA_TYPE schema = obj.getSchema();

      if (!schema.valid()) {
        return false;
      }

      Alembic::Abc::ICompoundProperty arbProp = schema.getArbGeomParams();
      Alembic::Abc::ICompoundProperty userProp = schema.getUserProperties();

      if (addPropsToShape) {
        addProps(arbProp, node, false, true);
        addProps(userProp, node, false, true);
      }
      std::string arbPropStr = addProps(arbProp, readerNode, false, false);
      std::string userPropStr = addProps(userProp, readerNode, false, false);

      geomParamsPlug.setValue(arbPropStr.c_str());
      userAttrsPlug.setValue(userPropStr.c_str());

      return connectProps(depNode, readerDepNode);
    }

  template<typename OBJECT_TYPE, typename SCHEMA_TYPE>
    inline bool addAndConnectProps(SceneNodeAppPtr &newAppNode)
    {
      return addAndConnectProps<OBJECT_TYPE, SCHEMA_TYPE>(
          newAppNode->dccIdentifier.c_str(),
          newAppNode->dccReaderIdentifier.c_str(),
          true
          );
    }


  // --- replace data
  template<typename OBJECT_TYPE, typename SCHEMA_TYPE>
    bool replaceSimilarData(const char *functionName,
        SceneNodeAlembicPtr fileNode)
    {
      static const MString format(
          "ExoAlembic._attach.attach^1s(r\"^2s\", r\"^3s\", ^4s, ^5s)");

      ESS_PROFILE_SCOPE("SceneNodeMaya::replaceSimilarData");

      const MString connectTo = this->connectTo.length() == 0
        ? this->dccIdentifier.c_str()
        : this->connectTo;

      MString cmd;
      cmd.format(format, functionName, connectTo,
          fileNode->dccIdentifier.c_str(), fileAndTime->variable(),
          PythonBool(fileNode->pObjCache->isConstant));
      MStringArray results;
      MStatus result = MGlobal::executePythonCommand(cmd, results);
      if (result.error() && results.length() != 2) {
        ESS_LOG_WARNING("Attached failed for " << functionName << ", reason: "
            << result.errorString().asChar());
        return false;
      }
      if (results.length() == 2) {
        ESS_LOG_WARNING("Attached failed for " << functionName << ", reason: "
            << results[1].asChar());
        return false;
      }

      if (!removeProps(results[0])) {
        return false;
      }

      if (!addAndConnectProps<OBJECT_TYPE, SCHEMA_TYPE>(
            connectTo, results[0], false)) {
        return false;
      }

      fileNode->setAttached(true);
      return true;
    }


  // --- add child
  bool executeAddChild(const MString& cmd, SceneNodeAppPtr& newAppNode);

  // because camera, curves and points work the same way!
  bool addSimilarChild(const char* functionName, SceneNodeAlembicPtr fileNode,
                       SceneNodeAppPtr& newAppNode);
  bool addXformChild(SceneNodeAlembicPtr fileNode, SceneNodeAppPtr& newAppNode);
  bool addPolyMeshChild(SceneNodeAlembicPtr fileNode,
                        SceneNodeAppPtr& newAppNode);
  bool addCurveChild(SceneNodeAlembicPtr fileNode, SceneNodeAppPtr& newAppNode);

 public:
  SceneNodeMaya(const AlembicFileAndTimeControlPtr alembicFileAndTimeControl =
                    AlembicFileAndTimeControlPtr())
      : fileAndTime(alembicFileAndTimeControl), useMultiFile(false), connectTo("")
  {
  }

  MString connectTo;

  virtual bool replaceData(SceneNodeAlembicPtr fileNode,
                           const IJobStringParser& jobParams,
                           SceneNodeAlembicPtr& nextFileNode);
  virtual bool addChild(SceneNodeAlembicPtr fileNode,
                        const IJobStringParser& jobParams,
                        SceneNodeAppPtr& newAppNode);
  virtual void print(void);
};

/*
 * Build a scene graph base on the selected objects in the scene!
 * @param dagPath - The dag node of the root
 * @param alembicFileAndTimeControl - an optional file and time controller!
 */
SceneNodeAppPtr buildMayaSceneGraph(
    const MDagPath& dagPath, const SearchReplace::ReplacePtr& replacer,
    const AlembicFileAndTimeControlPtr alembicFileAndTimeControl =
        AlembicFileAndTimeControlPtr(),
    bool allowDeformedByUs = false);

#endif
