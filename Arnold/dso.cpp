#include "stdafx.h"

#include "CommonRegex.h"
#include "common.h"
#include "curves.h"
#include "instance.h"
#include "nurbs.h"
#include "points.h"
#include "polyMesh.h"

boost::mutex gGlobalLock;

#define GLOBAL_LOCK boost::mutex::scoped_lock writeLock(gGlobalLock);

std::map<std::string, std::string> gUsedArchives;

static int Init(AtNode *mynode, void **user_ptr)
{
  GLOBAL_LOCK;

  userData *ud = new userData();
  *user_ptr = ud;
  ud->gProcShaders = NULL;
  ud->gProcDispMap = NULL;

  const std::string strDataString =
      EnvVariables::replace(AiNodeGetStr(mynode, "data"));
  ud->gDataString = (char *)strDataString.c_str();
  ud->gProcShaders = AiArrayCopy(AiNodeGetArray(mynode, "shader"));

  ud->has_subdiv_settings =
      AiNodeLookUpUserParameter(mynode, "subdiv_type") != NULL;
  if (ud->has_subdiv_settings) {
    ud->subdiv_type = AiNodeGetStr(mynode, "subdiv_type");
    ud->subdiv_iterations = AiNodeGetInt(mynode, "subdiv_iterations");
    ud->subdiv_pixel_error = AiNodeGetFlt(mynode, "subdiv_pixel_error");
    if (AiNodeGetStr(mynode, "subdiv_dicing_camera")) {
      ud->subdiv_dicing_camera = AiNodeGetStr(mynode, "subdiv_dicing_camera");
    }
    else {
      ud->subdiv_dicing_camera.clear();
    }
  }

  ud->has_disp_settings =
      AiNodeLookUpUserParameter(mynode, "disp_zero_value") != NULL;
  if (ud->has_disp_settings) {
    ud->gProcDispMap = AiNodeGetArray(mynode, "disp_map");
    ud->disp_zero_value = AiNodeGetFlt(mynode, "disp_zero_value");
    ud->disp_height = AiNodeGetFlt(mynode, "disp_height");
    ud->disp_autobump = AiNodeGetBool(mynode, "disp_autobump");
    ud->disp_padding = AiNodeGetFlt(mynode, "disp_padding");
  }

  // set defaults for options
  ud->gCurvesMode = "ribbon";
  ud->gPointsMode = "";
  ud->gMbKeys.clear();

  // check the data string
  std::string completeStr(ud->gDataString);
  if (completeStr.length() == 0) {
    AiMsgError("[ExocortexAlembicArnold] No data string specified.");
    return NULL;
  }

  // split the string using boost
  std::vector<std::string> nameValuePairs;
  boost::split(nameValuePairs, completeStr, boost::is_any_of("&"));

  std::vector<std::string> paths(2);
  std::string identifier;
  ud->gTime = FLT_MAX;
  ud->gCurrTime = FLT_MAX;
  for (size_t i = 0; i < nameValuePairs.size(); i++) {
    std::vector<std::string> token;
    boost::split(token, nameValuePairs[i], boost::is_any_of("="));
    if (token.size() != 2) {
      AiMsgError(
          "[ExocortexAlembicArnold] Invalid dsodata token pair '%s' specified!",
          nameValuePairs[i].c_str());
      return NULL;
    }
    if (token[0] == "path") {
      paths[0] = token[1];
      if (paths[1].empty()) {
        paths[1] = paths[0];
      }
    }
    else if (token[0] == "instancespath") {
      paths[1] = token[1];
    }
    else if (token[0] == "identifier") {
      identifier = token[1];
    }
    else if (token[0] == "time") {
      ud->gTime = (float)atof(token[1].c_str());
    }
    else if (token[0] == "currtime") {
      ud->gCurrTime = (float)atof(token[1].c_str());
    }
    else if (token[0] == "curvesmode") {
      ud->gCurvesMode = token[1];
    }
    else if (token[0] == "pointsmode") {
      ud->gPointsMode = token[1];
    }
    else if (token[0] == "mbkeys") {
      std::vector<std::string> sampleTimes;
      boost::split(sampleTimes, token[1], boost::is_any_of(";"));
      ud->gMbKeys.resize(sampleTimes.size(), ud->gTime);
      for (size_t j = 0; j < sampleTimes.size(); j++) {
        ud->gMbKeys[j] = (float)atof(sampleTimes[j].c_str());
      }
    }
    else {
      AiMsgError(
          "[ExocortexAlembicArnold] Invalid dsodata token name '%s' specified!",
          token[0].c_str());
      return NULL;
    }
  }

  // compute the central time
  ud->gCentroidTime = 0.0f;
  for (size_t j = 0; j < ud->gMbKeys.size(); j++) {
    ud->gCentroidTime += ud->gMbKeys[j];
  }
  if (ud->gMbKeys.size() > 0) {
    ud->gCentroidTime /= float(ud->gMbKeys.size());
  }
  ud->gCentroidTime = roundCentroid(ud->gCentroidTime);

  // check if we have all important values
  if (paths[0].length() == 0) {
    AiMsgError("[ExocortexAlembicArnold] path token not specified in '%s'.",
               ud->gDataString);
    return NULL;
  }
  if (identifier.length() == 0) {
    AiMsgError(
        "[ExocortexAlembicArnold] identifier token not specified in '%s'.",
        ud->gDataString);
    return NULL;
  }
  if (ud->gTime == FLT_MAX) {
    AiMsgError("[ExocortexAlembicArnold] time token not specified in '%s'.",
               ud->gDataString);
    return NULL;
  }
  if (ud->gCurrTime == FLT_MAX) {
    AiMsgError("[ExocortexAlembicArnold] currtime token not specified in '%s'.",
               ud->gDataString);
    return NULL;
  }
  if (ud->gMbKeys.size() == 0) {
    ud->gMbKeys.push_back(ud->gTime);
  }

  // fix all paths
  for (size_t pathIndex = 0; pathIndex < paths.size(); pathIndex++) {
#ifdef _WIN32
    // #54: UNC paths don't work
    // add another backslash if we start with a backslash
    if (paths[pathIndex].at(0) == '\\' && paths[pathIndex].at(1) != '\\') {
      paths[pathIndex] = "\\" + paths[pathIndex];
    }
#endif

    // resolve tokens
    int insideToken = 0;
    std::string result;
    std::string tokenName;
    std::string tokenValue;

    for (size_t i = 0; i < paths[pathIndex].size(); i++) {
      if (insideToken == 0) {
        if (paths[pathIndex][i] == '{') {
          insideToken = 1;
        }
        else {
          result += paths[pathIndex][i];
        }
      }
      else if (insideToken == 1) {
        if (paths[pathIndex][i] == '}') {
          AiMsgError("[ExocortexAlembicArnold] Invalid tokens in '%s'.",
                     paths[pathIndex].c_str());
          return NULL;
        }
        else if (paths[pathIndex][i] == ' ') {
          insideToken = 2;
        }
        else if (paths[pathIndex][i] == '}') {
          insideToken = 0;
          std::transform(tokenName.begin(), tokenName.end(), tokenName.begin(),
                         ::tolower);
          // todo: eventually deal with unary tokens here
          // if(tokenName == "mytoken")
          //{
          //}
          // else
          {
            AiMsgError("[ExocortexAlembicArnold] Unknown unary token '%s'.",
                       tokenName.c_str());
            return NULL;
          }
        }
        else {
          tokenName += paths[pathIndex][i];
        }
      }
      else if (insideToken == 2) {
        if (paths[pathIndex][i] == '{') {
          AiMsgError("[ExocortexAlembicArnold] Invalid tokens in '%s'.",
                     paths[pathIndex].c_str());
          return NULL;
        }
        else if (paths[pathIndex][i] == '}') {
          // binary tokens
          insideToken = 0;
          std::transform(tokenName.begin(), tokenName.end(), tokenName.begin(),
                         ::tolower);
          if (tokenName == "env") {
            bool found = false;
            if (getenv(tokenValue.c_str()) != NULL) {
              std::string envValue = getenv(tokenValue.c_str());
              if (!envValue.empty()) {
                result += envValue;
                found = true;
              }
            }
            if (!found) {
              AiMsgError(
                  "[ExocortexAlembicArnold] Environment variable '%s' is not "
                  "defined.",
                  tokenValue.c_str());
              return NULL;
            }
          }
          else {
            AiMsgError("[ExocortexAlembicArnold] Unknown binary token '%s'.",
                       tokenName.c_str());
            return NULL;
          }
        }
        else {
          tokenValue += paths[pathIndex][i];
        }
      }
    }
    paths[pathIndex] = result;
  }

  AiMsgDebug("[ExocortexAlembicArnold] path used: %s", paths[0].c_str());
  AiMsgDebug("[ExocortexAlembicArnold] identifier used: %s",
             identifier.c_str());
  AiMsgDebug("[ExocortexAlembicArnold] time used: %f", time);

  // now let's test if the archive exists
  FILE *file = fopen(paths[0].c_str(), "rb");
  if (file == NULL) {
    AiMsgError("[ExocortexAlembicArnold] File '%s' does not exist.",
               paths[0].c_str());
    return NULL;
  }
  fclose(file);

  // also check the instancesPath if it is different
  if (paths[1] != paths[0]) {
    FILE *file = fopen(paths[1].c_str(), "rb");
    if (file == NULL) {
      AiMsgError("[ExocortexAlembicArnold] File '%s' does not exist.",
                 paths[1].c_str());
      return NULL;
    }
    fclose(file);
  }

  // open the archive
  // ESS_LOG_INFO(paths[0].c_str());
  Alembic::Abc::IArchive archive(Alembic::AbcCoreHDF5::ReadArchive(), paths[0]);
  if (!archive.getTop().valid()) {
    AiMsgError(
        "[ExocortexAlembicArnold] Not a valid Alembic data stream.  Path: %s",
        paths[0].c_str());
    return NULL;
  }
  // ESS_LOG_INFO(paths[1].c_str());
  Alembic::Abc::IArchive instancesArchive(Alembic::AbcCoreHDF5::ReadArchive(),
                                          paths[1]);
  if (!instancesArchive.getTop().valid()) {
    AiMsgError(
        "[ExocortexAlembicArnold] Not a valid Alembic data stream.  Path: %s",
        paths[1].c_str());
    return NULL;
  }

  std::vector<std::string> parts;
  boost::split(parts, identifier, boost::is_any_of("/\\"));
  ud->proceduralDepth = (int)(parts.size() - 2);

  // recurse to find the object
  Alembic::Abc::IObject object = archive.getTop();
  for (size_t i = 1; i < parts.size(); i++) {
    Alembic::Abc::IObject child(object, parts[i]);
    object = child;
    if (!object) {
      AiMsgError("[ExocortexAlembicArnold] Cannot find object '%s'.",
                 identifier.c_str());
      return NULL;
    }
  }

  // push all objects to process into the static list
  std::vector<Alembic::Abc::IObject> objects;
  objects.push_back(object);
  for (size_t i = 0; i < objects.size(); i++) {
    if (Alembic::AbcGeom::IXform::matches(objects[i].getMetaData())) {
      for (size_t j = 0; j < objects[i].getNumChildren(); j++) {
        objects.push_back(objects[i].getChild(j));
      }
    }
    else if (Alembic::AbcGeom::IPoints::matches(objects[i].getMetaData())) {
      // cast to curves
      Alembic::AbcGeom::IPoints typedObject(objects[i],
                                            Alembic::Abc::kWrapExisting);

      // first thing to check is if this is an instancing cloud
      if (typedObject.getSchema().getPropertyHeader(".shapetype") != NULL &&
          typedObject.getSchema().getPropertyHeader(".shapeinstanceid") !=
              NULL &&
          typedObject.getSchema().getPropertyHeader(".instancenames") != NULL) {
        size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1
                                   ? typedObject.getSchema().getNumSamples()
                                   : ud->gMbKeys.size();
        Alembic::Abc::IUInt16ArrayProperty shapeTypeProp =
            Alembic::Abc::IUInt16ArrayProperty(typedObject.getSchema(),
                                               ".shapetype");
        Alembic::Abc::IUInt16ArrayProperty shapeInstanceIDProp =
            Alembic::Abc::IUInt16ArrayProperty(typedObject.getSchema(),
                                               ".shapeinstanceid");
        Alembic::Abc::IStringArrayProperty shapeInstanceNamesProp =
            Alembic::Abc::IStringArrayProperty(typedObject.getSchema(),
                                               ".instancenames");
        if (shapeTypeProp.getNumSamples() > 0 &&
            shapeInstanceIDProp.getNumSamples() > 0 &&
            shapeInstanceNamesProp.getNumSamples() > 0) {
          // ok, we are for sure an instancing cloud...
          // let's skip this, we'll do it the second time around
          continue;
        }
      }

      objectInfo info(ud->gCentroidTime);
      info.abc = objects[i];
      ud->gIObjects.push_back(info);
    }
    else {
      objectInfo info(ud->gCentroidTime);
      info.abc = objects[i];
      ud->gIObjects.push_back(info);
    }
  }

  // loop a second time, only for the instancing clouds
  for (size_t i = 0; i < objects.size(); i++) {
    if (Alembic::AbcGeom::IPoints::matches(objects[i].getMetaData())) {
      // cast to curves
      Alembic::AbcGeom::IPoints typedObject(objects[i],
                                            Alembic::Abc::kWrapExisting);

      // first thing to check is if this is an instancing cloud
      Alembic::Abc::IUInt16ArrayProperty
          shapeTypeProp;  // Alembic::Abc::IUInt16ArrayProperty(
      // typedObject.getSchema(), ".shapetype" );
      Alembic::Abc::IUInt16ArrayProperty
          shapeInstanceIDProp;  // Alembic::Abc::IUInt16ArrayProperty(
      // typedObject.getSchema(), ".shapeinstanceid"
      // );
      Alembic::Abc::IStringArrayProperty
          shapeInstanceNamesProp;  // Alembic::Abc::IStringArrayProperty(
      // typedObject.getSchema(), ".instancenames"
      // );
      if (getArbGeomParamPropertyAlembic(typedObject, "shapetype",
                                         shapeTypeProp) &&
          getArbGeomParamPropertyAlembic(typedObject, "shapeinstanceid",
                                         shapeInstanceIDProp) &&
          getArbGeomParamPropertyAlembic(typedObject, "instancenames",
                                         shapeInstanceNamesProp)) {
        size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1
                                   ? typedObject.getSchema().getNumSamples()
                                   : ud->gMbKeys.size();
        if (shapeTypeProp.getNumSamples() > 0 &&
            shapeInstanceIDProp.getNumSamples() > 0 &&
            shapeInstanceNamesProp.getNumSamples() > 0) {
          // check all of the nodes to be instanced
          // for this we will use the very last sample
          instanceCloudInfo cloudInfo;
          Alembic::Abc::StringArraySamplePtr lastSampleInstanceNames =
              shapeInstanceNamesProp.getValue(
                  shapeInstanceNamesProp.getNumSamples() - 1);
          for (size_t j = 0; j < lastSampleInstanceNames->size(); j++) {
            instanceGroupInfo groupInfo;
            std::string identifier = lastSampleInstanceNames->get()[j];

            // first, we need to figure out if this is a transform
            bool found = false;
            for (size_t k = 0; k < ud->gIObjects.size(); k++) {
              if (ud->gIObjects[k].abc.getFullName() == identifier) {
                // if we find the object in our export list, it is not a
                // transform!
                // we don't push matrices, so that the transform is used without
                // an offset.
                // furthermore we push
                groupInfo.identifiers.push_back(identifier);
                groupInfo.objects.push_back(ud->gIObjects[k].abc);
                groupInfo.nodes.push_back(std::map<float, AtNode *>());
                groupInfo.nodes[groupInfo.nodes.size() - 1].insert(
                    std::pair<float, AtNode *>(ud->gCentroidTime, NULL));
                found = true;
                break;
              }
            }
            // only do this search if we don't require time shifting
            AtNode *masterNode = NULL;
            if (!found) {
              // this means we have not exported this yes, neither did we find
              // it in Arnold
              // as an exported node. so we will search in the alembic file for
              // it
              std::vector<std::string> parts;
              boost::split(parts, identifier, boost::is_any_of("/\\"));

              // recurse to find the object
              objectInfo info(ud->gCentroidTime);
              info.hide = true;
              info.abc = instancesArchive.getTop();
              info.suffix = "_INSTANCE";
              found = true;
              for (size_t k = 1; k < parts.size(); k++) {
                Alembic::Abc::IObject child(info.abc, parts[k]);
                info.abc = child;
                if (!info.abc) {
                  found = false;
                  break;
                }
              }
              if (found) {
                // if this is an alembic transform, then we need to build
                // exports for every
                // shape below it
                if (Alembic::AbcGeom::IXform::matches(info.abc.getMetaData())) {
                  // collect all of the children
                  Alembic::Abc::IObject parent = info.abc;
                  std::vector<Alembic::Abc::IObject> children;
                  children.push_back(info.abc);
                  for (size_t k = 0; k < children.size(); k++) {
                    Alembic::Abc::IObject child = children[k];
                    if (Alembic::AbcGeom::IXform::matches(
                            child.getMetaData())) {
                      for (size_t l = 0; l < child.getNumChildren(); l++) {
                        children.push_back(child.getChild(l));
                      }
                    }
                    else {
                      // check if we already exported this object
                      // and push it to the export list if we didn't
                      bool nodeFound = false;
                      masterNode = AiNodeLookUpByName(
                          (getNameFromIdentifier(child.getFullName()) + "_DSO")
                              .c_str());
                      if (!masterNode)
                        masterNode = AiNodeLookUpByName(
                            getNameFromIdentifier(child.getFullName()).c_str());
                      for (size_t l = 0; l < ud->gIObjects.size(); l++) {
                        if (ud->gIObjects[l].abc.getFullName() ==
                            child.getFullName()) {
                          nodeFound = true;
                          break;
                        }
                      }
                      if (!nodeFound && masterNode == NULL) {
                        objectInfo childInfo(ud->gCentroidTime);
                        childInfo.hide = true;
                        childInfo.abc = child;
                        ud->gIObjects.push_back(childInfo);
                      }

                      // push this to our group info
                      groupInfo.identifiers.push_back(child.getFullName());
                      groupInfo.objects.push_back(child);
                      groupInfo.nodes.push_back(std::map<float, AtNode *>());
                      groupInfo.nodes[groupInfo.nodes.size() - 1].insert(
                          std::pair<float, AtNode *>(ud->gCentroidTime,
                                                     masterNode));

                      // we also store the parent
                      // this will enforce to compute actual offset matrices for
                      // each given time
                      groupInfo.parents.push_back(parent);
                      groupInfo.matrices.push_back(
                          std::map<float, std::vector<Alembic::Abc::M44f> >());
                    }
                  }
                }
                else {
                  // just push it for the export
                  if (masterNode == NULL) {
                    ud->gIObjects.push_back(info);
                  }

                  // also update our groupInfo
                  groupInfo.identifiers.push_back(identifier);
                  groupInfo.objects.push_back(info.abc);
                  groupInfo.nodes.push_back(std::map<float, AtNode *>());
                  groupInfo.nodes[groupInfo.nodes.size() - 1].insert(
                      std::pair<float, AtNode *>(ud->gCentroidTime,
                                                 masterNode));
                }
              }
            }

            if (!found) {
              // let's try to find the node inside Arnold directly.
              masterNode = AiNodeLookUpByName(
                  (getNameFromIdentifier(identifier) + "_DSO").c_str());
              if (!masterNode)
                masterNode = AiNodeLookUpByName(
                    getNameFromIdentifier(identifier).c_str());

              // if we now found a node, we can use it straight
              if (masterNode != NULL && cloudInfo.time.size() == 0) {
                groupInfo.identifiers.push_back(identifier);
                groupInfo.objects.push_back(Alembic::Abc::IObject());
                groupInfo.nodes.push_back(std::map<float, AtNode *>());
                groupInfo.nodes[groupInfo.nodes.size() - 1].insert(
                    std::pair<float, AtNode *>(ud->gCentroidTime, masterNode));
                found = true;
              }
            }

            // if we found this, let's push it
            if (found) {
              cloudInfo.groupInfos.push_back(groupInfo);
            }
            else {
              AiMsgError(
                  "[ExocortexAlembicArnold] Identifier '%s' is not part of "
                  "file '%s'. Aborting.",
                  identifier.c_str(), paths[1].c_str());
              return FALSE;
            }
          }

          Alembic::AbcGeom::IPointsSchema::Sample sampleFloor;
          Alembic::AbcGeom::IPointsSchema::Sample sampleCeil;

          // now let's get all of the positions etc
          for (size_t j = 0; j < minNumSamples; j++) {
            SampleInfo sampleInfo = getSampleInfo(
                ud->gMbKeys[j], typedObject.getSchema().getTimeSampling(),
                typedObject.getSchema().getNumSamples());
            typedObject.getSchema().get(sampleFloor, sampleInfo.floorIndex);
            typedObject.getSchema().get(sampleCeil, sampleInfo.ceilIndex);

            cloudInfo.pos.push_back(sampleFloor.getPositions());
            cloudInfo.vel.push_back(sampleFloor.getVelocities());
            cloudInfo.id.push_back(sampleFloor.getIds());
            cloudInfo.pos.push_back(sampleCeil.getPositions());
            cloudInfo.vel.push_back(sampleCeil.getVelocities());
            cloudInfo.id.push_back(sampleCeil.getIds());
          }

          // store the widths
          Alembic::AbcGeom::IFloatGeomParam widthParam =
              typedObject.getSchema().getWidthsParam();
          if (widthParam.valid()) {
            for (size_t j = 0; j < minNumSamples; j++) {
              SampleInfo sampleInfo =
                  getSampleInfo(ud->gMbKeys[j], widthParam.getTimeSampling(),
                                widthParam.getNumSamples());
              cloudInfo.width.push_back(
                  widthParam.getExpandedValue(sampleInfo.floorIndex).getVals());
              cloudInfo.width.push_back(
                  widthParam.getExpandedValue(sampleInfo.ceilIndex).getVals());
            }
          }

          // store the scale
          Alembic::Abc::IV3fArrayProperty propScale;
          if (getArbGeomParamPropertyAlembic(typedObject, "scale", propScale)) {
            for (size_t j = 0; j < minNumSamples; j++) {
              SampleInfo sampleInfo =
                  getSampleInfo(ud->gMbKeys[j], propScale.getTimeSampling(),
                                propScale.getNumSamples());
              cloudInfo.scale.push_back(
                  propScale.getValue(sampleInfo.floorIndex));
              cloudInfo.scale.push_back(
                  propScale.getValue(sampleInfo.ceilIndex));
            }
          }

          // store the orientation
          Alembic::Abc::IQuatfArrayProperty propOrientation;
          if (getArbGeomParamPropertyAlembic(typedObject, "orientation",
                                             propOrientation)) {
            for (size_t j = 0; j < minNumSamples; j++) {
              SampleInfo sampleInfo = getSampleInfo(
                  ud->gMbKeys[j], propOrientation.getTimeSampling(),
                  propOrientation.getNumSamples());
              cloudInfo.rot.push_back(
                  propOrientation.getValue(sampleInfo.floorIndex));
            }
          }

          // store the angular velocity
          Alembic::Abc::IQuatfArrayProperty propAngularvelocity;
          if (getArbGeomParamPropertyAlembic(typedObject, "angularvelocity",
                                             propAngularvelocity)) {
            for (size_t j = 0; j < minNumSamples; j++) {
              SampleInfo sampleInfo = getSampleInfo(
                  ud->gMbKeys[j], propAngularvelocity.getTimeSampling(),
                  propAngularvelocity.getNumSamples());
              cloudInfo.ang.push_back(
                  propAngularvelocity.getValue(sampleInfo.floorIndex));
            }
          }

          // store the age
          Alembic::Abc::IFloatArrayProperty propAge;
          if (getArbGeomParamPropertyAlembic(typedObject, "age", propAge)) {
            for (size_t j = 0; j < minNumSamples; j++) {
              SampleInfo sampleInfo =
                  getSampleInfo(ud->gMbKeys[j], propAge.getTimeSampling(),
                                propAge.getNumSamples());
              cloudInfo.age.push_back(propAge.getValue(sampleInfo.floorIndex));
            }
          }

          // store the mass
          Alembic::Abc::IFloatArrayProperty propMass;
          if (getArbGeomParamPropertyAlembic(typedObject, "mass", propMass)) {
            for (size_t j = 0; j < minNumSamples; j++) {
              SampleInfo sampleInfo =
                  getSampleInfo(ud->gMbKeys[j], propMass.getTimeSampling(),
                                propMass.getNumSamples());
              cloudInfo.mass.push_back(
                  propMass.getValue(sampleInfo.floorIndex));
            }
          }

          // store the shape id
          Alembic::Abc::IUInt16ArrayProperty propShapeInstanceID;
          if (getArbGeomParamPropertyAlembic(typedObject, "shapeinstanceid",
                                             propShapeInstanceID)) {
            for (size_t j = 0; j < minNumSamples; j++) {
              SampleInfo sampleInfo = getSampleInfo(
                  ud->gMbKeys[j], propShapeInstanceID.getTimeSampling(),
                  propShapeInstanceID.getNumSamples());
              cloudInfo.shape.push_back(
                  propShapeInstanceID.getValue(sampleInfo.floorIndex));
            }
          }

          // store the shape time
          Alembic::Abc::IFloatArrayProperty propShapeTime;
          if (getArbGeomParamPropertyAlembic(typedObject, "shapetime",
                                             propShapeTime)) {
            SampleInfo sampleInfo = getSampleInfo(
                ud->gCentroidTime, propShapeTime.getTimeSampling(),
                propShapeTime.getNumSamples());
            cloudInfo.time.push_back(
                propShapeTime.getValue(sampleInfo.floorIndex));
            cloudInfo.timeAlpha = (float)sampleInfo.alpha;
            if (sampleInfo.alpha != 0.0)
              cloudInfo.time.push_back(
                  propShapeTime.getValue(sampleInfo.ceilIndex));
          }

          // store the color
          Alembic::Abc::IC4fArrayProperty propColor;
          if (getArbGeomParamPropertyAlembic(typedObject, "color", propColor)) {
            for (size_t j = 0; j < minNumSamples; j++) {
              SampleInfo sampleInfo =
                  getSampleInfo(ud->gMbKeys[j], propColor.getTimeSampling(),
                                propColor.getNumSamples());
              cloudInfo.color.push_back(
                  propColor.getValue(sampleInfo.floorIndex));
            }
          }

          // now check if we have the time offsets, and if so let's export all
          // of these master nodes as well
          if (cloudInfo.time.size() > 0 && cloudInfo.shape.size() > 0) {
            Alembic::Abc::FloatArraySamplePtr times = cloudInfo.time[0];
            Alembic::Abc::UInt16ArraySamplePtr shapes =
                cloudInfo.shape[cloudInfo.shape.size() - 1];

            // for all of the particles
            size_t numParticles =
                times->size() > shapes->size() ? times->size() : shapes->size();
            for (size_t j = 0; j < numParticles; j++) {
              float centroidTime =
                  times->get()[j < times->size() ? j : times->size() - 1];
              if (cloudInfo.time.size() > 1) {
                // interpolate the time if necessary
                centroidTime =
                    (1.0f - cloudInfo.timeAlpha) * centroidTime +
                    cloudInfo.timeAlpha *
                        cloudInfo.time[1]
                            ->get()[j < cloudInfo.time[1]->size()
                                        ? j
                                        : cloudInfo.time[1]->size() - 1];
              }
              centroidTime = roundCentroid(centroidTime);

              // get the id of the shape
              size_t shapeID =
                  (size_t)shapes
                      ->get()[j < shapes->size() ? j : shapes->size() - 1];

              // check if we have this shapeID already
              if (shapeID >= cloudInfo.groupInfos.size()) {
                AiMsgError(
                    "[ExocortexAlembicArnold] shapeID '%d' is not valid. "
                    "Aborting.",
                    (int)shapeID);
                return NULL;
              }

              instanceGroupInfo *groupInfo = &cloudInfo.groupInfos[shapeID];
              for (size_t g = 0; g < groupInfo->identifiers.size(); g++) {
                std::map<float, AtNode *>::iterator it =
                    groupInfo->nodes[g].find(centroidTime);
                if (it == groupInfo->nodes[g].end()) {
                  // check if we have this gObject somewhere...
                  Alembic::Abc::IObject abcMasterObject;
                  objectInfo objInfo(ud->gCentroidTime);
                  objInfo.hide = true;
                  for (size_t k = 0; k < ud->gIObjects.size(); k++) {
                    if (ud->gIObjects[k].abc.getFullName() ==
                        groupInfo->identifiers[g]) {
                      objInfo.abc = ud->gIObjects[k].abc;
                      break;
                    }
                  }
                  if (!objInfo.abc.valid() && groupInfo->objects[g].valid()) {
                    objInfo.abc = groupInfo->objects[g];
                  }
                  if (!objInfo.abc.valid()) {
                    AiMsgError(
                        "[ExocortexAlembicArnold] Identifier '%s' is not part "
                        "of file '%s'. Aborting.",
                        groupInfo->identifiers[g].c_str(), paths[1].c_str());
                    return NULL;
                  }

                  // push it to the map. This way we can ensure to export it!
                  objInfo.centroidTime = centroidTime;
                  ud->gIObjects.push_back(objInfo);

                  groupInfo->nodes[g].insert(
                      std::pair<float, AtNode *>(centroidTime, NULL));
                }
              }
            }
          }

          ud->gInstances.push_back(cloudInfo);

          // now let's get the number of position of the first sample
          // and create an instance export for that one
          for (size_t j = 0; j < cloudInfo.pos[0]->size(); j++) {
            objectInfo objInfo(ud->gCentroidTime);
            objInfo.abc = objects[i];
            objInfo.instanceCloud = &ud->gInstances[ud->gInstances.size() - 1];
            objInfo.instanceID = (long)(cloudInfo.shape[0]->size() > j
                                            ? cloudInfo.shape[0]->get()[j]
                                            : cloudInfo.shape[0]->get()[0]);
            objInfo.ID = (long)j;
            for (size_t k = 0;
                 k < cloudInfo.groupInfos[objInfo.instanceID].nodes.size();
                 k++) {
              objInfo.instanceGroupID = (long)k;
              ud->gIObjects.push_back(objInfo);
            }
          }
        }
      }
    }
  }
  return TRUE;
}

// All done, deallocate stuff
static int Cleanup(void *user_ptr)
{
  GLOBAL_LOCK;
  userData *ud = (userData *)user_ptr;

  ud->gIObjects.clear();
  ud->gInstances.clear();
  ud->gMbKeys.clear();

  delete (ud);
  return TRUE;
}

// Get number of nodes
static int NumNodes(void *user_ptr)
{
  GLOBAL_LOCK;

  userData *ud = (userData *)user_ptr;
  int size = (int)ud->gIObjects.size();
  return size;
}

// Get the i_th node
static AtNode *GetNode(void *user_ptr, int i)
{
  GLOBAL_LOCK;
  userData *ud = (userData *)user_ptr;
  // check if this is a known object
  if (i >= (int)ud->gIObjects.size()) {
    return NULL;
  }

  nodeData nodata;  // contain basic information common in all types of data!

  // construct the timesamples
  size_t nbSamples = ud->gMbKeys.size();

  nodata.samples = ud->gMbKeys;
  for (size_t j = 0; j < nbSamples; ++j) {
    nodata.samples[j] += ud->gIObjects[i].centroidTime - ud->gCentroidTime;
  }
  nodata.shifted =
      fabsf(ud->gIObjects[i].centroidTime - ud->gCentroidTime) > 0.001f;
  nodata.createdShifted = nodata.shifted;

  // create the resulting node
  AtNode *shapeNode = NULL;

  // now check if this is supposed to be an instance
  if (ud->gIObjects[i].instanceID > -1) {
    return createInstanceNode(nodata, ud, i);
  }

  nodata.shaders = NULL;
  nodata.shaderIndices = NULL;
  nodata.isPolyMeshNode = false;

  nodata.object = ud->gIObjects[i].abc;
  if (!nodata.object.valid()) {
    AiMsgError("[ExocortexAlembicArnold] Not a valid Alembic data stream.");
    return NULL;
  }

  const Alembic::Abc::MetaData &md = nodata.object.getMetaData();
  if (Alembic::AbcGeom::IPolyMesh::matches(md)) {
    shapeNode = createPolyMeshNode(nodata, ud, nodata.samples, i);
  }
  else if (Alembic::AbcGeom::ISubD::matches(md)) {
    shapeNode = createSubDNode(nodata, ud, nodata.samples, i);
  }
  else if (Alembic::AbcGeom::ICurves::matches(md)) {
    shapeNode = createCurvesNode(nodata, ud, nodata.samples, i);
  }
  else if (Alembic::AbcGeom::INuPatch::matches(md)) {
    shapeNode = createNurbsNode(nodata, ud, nodata.samples, i);
  }
  else if (Alembic::AbcGeom::IPoints::matches(md)) {
    shapeNode = createPointsNode(nodata, ud, nodata.samples, i);
  }
  else if (Alembic::AbcGeom::ICamera::matches(md)) {
    AiMsgWarning("[ExocortexAlembicArnold] Cameras are not supported.");
  }
  else
    AiMsgError(
        "[ExocortexAlembicArnold] This object type is not supported: '%s'.",
        md.get("schema").c_str());

  // if we have a shape
  if (shapeNode != NULL) {
    ud->constructedNodes.push_back(shapeNode);
    if (nodata.shaders != NULL) {
      ud->shadersToAssign.push_back(AiArrayCopy(nodata.shaders));
    }
    else if (ud->gProcShaders != NULL) {
      ud->shadersToAssign.push_back(AiArrayCopy(ud->gProcShaders));
    }
    else {
      ud->shadersToAssign.push_back(NULL);
    }

    if (nodata.isPolyMeshNode) {
      if (ud->gProcDispMap != NULL) {
        AiNodeSetArray(shapeNode, "disp_map", AiArrayCopy(ud->gProcDispMap));
        if (ud->has_disp_settings) {
          AiNodeSetFlt(shapeNode, "disp_zero_value", ud->disp_zero_value);
          AiNodeSetFlt(shapeNode, "disp_height", ud->disp_height);
          AiNodeSetBool(shapeNode, "disp_autobump", ud->disp_autobump);
          AiNodeSetFlt(shapeNode, "disp_padding", ud->disp_padding);
        }
      }
      if (nodata.shaderIndices != NULL) {
        AiNodeSetArray(shapeNode, "shidxs", nodata.shaderIndices);
      }

      // do we have subdivision settings
      if (ud->has_subdiv_settings) {
        AiNodeSetStr(shapeNode, "subdiv_type", ud->subdiv_type.c_str());
        AiNodeSetInt(shapeNode, "subdiv_iterations", ud->subdiv_iterations);
        AiNodeSetFlt(shapeNode, "subdiv_pixel_error", ud->subdiv_pixel_error);
        if (ud->subdiv_dicing_camera.length() > 0)
          AiNodeSetStr(shapeNode, "subdiv_dicing_camera",
                       ud->subdiv_dicing_camera.c_str());
      }
    }

    // allocate the matrices for arnold and initiate them with identities
    AtArray *matrices = AiArrayAllocate(1, (AtInt)nbSamples, AI_TYPE_MATRIX);
    for (AtInt i = 0; i < nbSamples; ++i) {
      AtMatrix matrix;
      AiM4Identity(matrix);
      AiArraySetMtx(matrices, i, matrix);
    }

    // check if we have a parent that is a transform
    Alembic::Abc::IObject parent = nodata.object.getParent();

    // count the depth
    int depth = 0;
    while (Alembic::AbcGeom::IXform::matches(parent.getMetaData())) {
      ++depth;
      parent = parent.getParent();
    }

    // loop until we hit the procedural's depth
    parent = nodata.object.getParent();
    while (Alembic::AbcGeom::IXform::matches(parent.getMetaData()) &&
           depth > ud->proceduralDepth) {
      --depth;

      // cast to a xform
      Alembic::AbcGeom::IXform parentXform(parent, Alembic::Abc::kWrapExisting);
      if (parentXform.getSchema().getNumSamples() == 0) {
        break;
      }

      // loop over all samples
      for (size_t sampleIndex = 0; sampleIndex < nbSamples; sampleIndex++) {
        SampleInfo sampleInfo =
            getSampleInfo(nodata.samples[sampleIndex],
                          parentXform.getSchema().getTimeSampling(),
                          parentXform.getSchema().getNumSamples());

        // get the data and blend it if necessary
        Alembic::AbcGeom::XformSample sample;
        parentXform.getSchema().get(sample, sampleInfo.floorIndex);
        Alembic::Abc::M44d abcMatrix = sample.getMatrix();
        if (sampleInfo.alpha >= sampleTolerance) {
          parentXform.getSchema().get(sample, sampleInfo.ceilIndex);
          Alembic::Abc::M44d ceilAbcMatrix = sample.getMatrix();
          abcMatrix = (1.0 - sampleInfo.alpha) * abcMatrix +
                      sampleInfo.alpha * ceilAbcMatrix;
        }

        // now convert to an arnold matrix
        AtMatrix parentMatrix, childMatrix, globalMatrix;
        size_t offset = 0;
        for (size_t row = 0; row < 4; ++row)
          for (size_t col = 0; col < 4; ++col, ++offset) {
            parentMatrix[row][col] = (AtFloat)abcMatrix.getValue()[offset];
          }

        // multiply the matrices since we want to know where we are in global
        // space
        AiArrayGetMtx(matrices, (AtULong)sampleIndex, childMatrix);
        AiM4Mult(globalMatrix, childMatrix, parentMatrix);
        AiArraySetMtx(matrices, (AtULong)sampleIndex, globalMatrix);
      }

      // go upwards
      parent = parent.getParent();
    }

    AiNodeSetArray(shapeNode, "matrix", matrices);
  }

  // set the name of the node
  std::string nameStr =
      getNameFromIdentifier(nodata.object.getFullName(),
                            ud->gIObjects[i].instanceID,
                            ud->gIObjects[i].instanceGroupID) +
      ud->gIObjects[i].suffix;
  if (nodata.shifted)
    nameStr +=
        ".time" + boost::lexical_cast<std::string>(
                      (int)(ud->gIObjects[i].centroidTime * 1000.0f + 0.5f));
  if (ud->gIObjects[i].hide) {
    AiNodeSetInt(shapeNode, "visibility", 0);
  }
  AiNodeSetStr(shapeNode, "name", nameStr.c_str());

  // set the pointer inside the map
  ud->gIObjects[i].node = shapeNode;

  // now update the instance maps
  const size_t gInstSize = ud->gInstances.size();
  const std::string &objFullName = nodata.object.getFullName();
  for (size_t j = 0; j < gInstSize; ++j) {
    std::vector<instanceGroupInfo> &gInstGrpInfo = ud->gInstances[j].groupInfos;
    const size_t grpInfoSize = gInstGrpInfo.size();
    for (size_t k = 0; k < grpInfoSize; ++k) {
      std::vector<std::string> gInstGrpIds = gInstGrpInfo[k].identifiers;
      const size_t idSize = gInstGrpIds.size();
      for (size_t l = 0; l < idSize; ++l) {
        if (gInstGrpIds[l] == objFullName) {
          std::map<float, AtNode *>::iterator it =
              gInstGrpInfo[k].nodes[l].find(ud->gIObjects[i].centroidTime);
          if (it != gInstGrpInfo[k].nodes[l].end()) {
            it->second = shapeNode;
          }
          break;
        }
      }
    }
  }
  return shapeNode;
}

// DSO hook
#ifdef __cplusplus
extern "C" {
#endif

AI_EXPORT_LIB int ProcLoader(AtProcVtable *vtable)
{
  vtable->Init = Init;
  vtable->Cleanup = Cleanup;
  vtable->NumNodes = NumNodes;
  vtable->GetNode = GetNode;

  sprintf(vtable->version, AI_VERSION);
  return 1;
}

#ifdef __cplusplus
}
#endif
