#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicCameraUtilities.h"
#include "AlembicDefinitions.h"
#include "AlembicLightUtilities.h"
#include "AlembicMeshUtilities.h"
#include "AlembicNurbsUtilities.h"
#include "AlembicParticles.h"
#include "AlembicRecursiveImporter.h"
#include "AlembicSplineUtilities.h"
#include "AlembicWriteJob.h"
#include "AlembicXformController.h"
#include "AlembicXformUtilities.h"
#include "SceneEnumProc.h"
#include "Utility.h"
#include "stdafx.h"

#include "AlembicRecursiveImporter.h"

int createAlembicObject(AbcG::IObject &iObj, INode **pMaxNode,
                        alembic_importoptions &options, std::string &file)
{
  AbcA::MetaData mdata = iObj.getMetaData();

  int ret = alembic_success;
  // if(AbcG::IXform::matches(iObj.getMetaData())) //Transform
  //{
  //	ESS_LOG_INFO( "AlembicImport_XForm: " << objects[j].getFullName() );
  //	int ret = AlembicImport_PolyMesh(file, iObj, options, pMaxNode);
  //}
  if (AbcG::IPolyMesh::matches(mdata) ||
      AbcG::ISubD::matches(mdata))  // PolyMesh / SubD
  {
    ESS_LOG_INFO("AlembicImport_PolyMesh: " << iObj.getFullName());
    ret = AlembicImport_PolyMesh(file, iObj, options, pMaxNode);
  }
  else if (AbcG::ICamera::matches(mdata))  // Camera
  {
    ESS_LOG_INFO("AlembicImport_Camera: " << iObj.getFullName());
    ret = AlembicImport_Camera(file, iObj, options, pMaxNode);
  }
  else if (AbcG::IPoints::matches(mdata))  // Points
  {
    ESS_LOG_INFO("AlembicImport_Points: " << iObj.getFullName());
    ret = AlembicImport_Points(file, iObj, options, pMaxNode);
  }
  else if (AbcG::ICurves::matches(mdata))  // Curves
  {
    if (options.loadCurvesAsNurbs) {
      ESS_LOG_INFO("AlembicImport_Nurbs: " << iObj.getFullName());
      ret = AlembicImport_NURBS(file, iObj, options, pMaxNode);
    }
    else {
      ESS_LOG_INFO("AlembicImport_Shape: " << iObj.getFullName());
      ret = AlembicImport_Shape(file, iObj, options, pMaxNode);
    }
  }
  else if (AbcG::ILight::matches(mdata))  // Light
  {
    ESS_LOG_INFO("AlembicImport_Light: " << iObj.getFullName());
    ret = AlembicImport_Light(file, iObj, options, pMaxNode);
  }
  else if (AbcM::IMaterial::matches(mdata)) {
    ESS_LOG_WARNING(
        "Alembic IMaterial not yet supported: " << iObj.getFullName());
  }
  else  // NURBS
  {
    if (options.failOnUnsupported) {
      ESS_LOG_ERROR("Alembic data type not supported: " << iObj.getFullName());
      return alembic_failure;
    }
    else {
      ESS_LOG_WARNING(
          "Alembic data type not supported: " << iObj.getFullName());
    }
  }
  return ret;
}

struct stackElement {
  AbcObjectCache *pObjectCache;
  INode *pParentMaxNode;

  stackElement(AbcObjectCache *pMyObjectCache)
      : pObjectCache(pMyObjectCache), pParentMaxNode(NULL)
  {
  }
  stackElement(AbcObjectCache *pMyObjectCache, INode *parent)
      : pObjectCache(pMyObjectCache), pParentMaxNode(parent)
  {
  }
};

int importAlembicScene(AbcArchiveCache *pArchiveCache,
                       AbcObjectCache *pRootObjectCache,
                       alembic_importoptions &options, std::string &file,
                       progressUpdate &progress,
                       std::map<std::string, bool> &nodeFullPaths)
{
  std::vector<stackElement> sceneStack;
  sceneStack.reserve(200);
  for (size_t j = 0; j < pRootObjectCache->childIdentifiers.size(); j++) {
    sceneStack.push_back(stackElement(
        &(pArchiveCache->find(pRootObjectCache->childIdentifiers[j])->second)));
  }

  while (!sceneStack.empty()) {
    stackElement sElement = sceneStack.back();
    sceneStack.pop_back();
    Abc::IObject &iObj = sElement.pObjectCache->obj;
    INode *pParentMaxNode = sElement.pParentMaxNode;

    if (!iObj.valid()) {
      return alembic_failure;
    }

    const std::string fullname = iObj.getFullName();
    const std::string pname = (pParentMaxNode)
                                  ? EC_MCHAR_to_UTF8(pParentMaxNode->GetName())
                                  : std::string("");
    const std::string name = iObj.getName();

    ESS_LOG_INFO("Importing " << fullname);

    bool bCreateDummyNode = false;
    int mergedGeomNodeIndex = -1;
    AbcObjectCache *pMergedObjectCache = NULL;
    getMergeInfo(pArchiveCache, sElement.pObjectCache, bCreateDummyNode,
                 mergedGeomNodeIndex, &pMergedObjectCache);

    INode *pMaxNode =
        NULL;  // the newly create node, which may be a merged node
    INode *pExistingNode = NULL;
    int keepTM = 1;  // I don't remember why this needed to be set in some
                     // cases.

    bool bCreateNode = true;

    if (!nodeFullPaths.empty()) {
      if (mergedGeomNodeIndex != -1) {
        AbcG::IObject mergedGeomChild = pMergedObjectCache->obj;
        bCreateNode = nodeFullPaths.find(mergedGeomChild.getFullName()) !=
                      nodeFullPaths.end();
      }
      else {
        bCreateNode = nodeFullPaths.find(fullname) != nodeFullPaths.end();
      }
    }

    if (bCreateNode) {
      // if we are about to merge a camera with its parent transform, force it
      // to create a dummy node instead if the camera's
      // transform also has children. This is done to prevent the camera
      // correction matrix from being applied to the other children
      if (!bCreateDummyNode && pMergedObjectCache &&
          sElement.pObjectCache->childIdentifiers.size() > 1 &&
          AbcG::ICamera::matches(pMergedObjectCache->obj.getMetaData())) {
        bCreateDummyNode = true;
        mergedGeomNodeIndex = -1;
      }

      if (bCreateDummyNode) {
        std::string importName = removeXfoSuffix(iObj.getName());

        pExistingNode = GetChildNodeFromName(importName, pParentMaxNode);
        if (options.attachToExisting && pExistingNode) {
          pMaxNode = pExistingNode;

          // see if a controller already exists, and then delete it

          int ret = AlembicImport_XForm(pParentMaxNode, pMaxNode, iObj, NULL,
                                        file, options);
          if (ret != 0) return ret;
        }  // only create node if either attachToExisting is false or it is true
           // and the object does not already exist
        else {
          int ret =
              AlembicImport_DummyNode(iObj, options, &pMaxNode, importName);
          if (ret != 0) return ret;

          ret = AlembicImport_XForm(pParentMaxNode, pMaxNode, iObj, NULL, file,
                                    options);
          if (ret != 0) return ret;
        }
      }
      else {
        if (mergedGeomNodeIndex !=
            -1) {  // we are merging, so look at the child geometry node
          AbcG::IObject mergedGeomChild = pMergedObjectCache->obj;
          std::string importName =
              removeXfoSuffix(iObj.getName());  // mergedGeomChild.getName());
          pExistingNode = GetChildNodeFromName(importName, pParentMaxNode);
          if (options.attachToExisting && pExistingNode) {
            pMaxNode = pExistingNode;
          }  // only create node if either attachToExisting is false or it is
             // true and the object does not already exist

          int ret =
              createAlembicObject(mergedGeomChild, &pMaxNode, options, file);
          if (ret != 0) return ret;
          if (pMaxNode != NULL) {
            ret = AlembicImport_XForm(pParentMaxNode, pMaxNode, iObj,
                                      &mergedGeomChild, file, options);
            if (ret != 0) return ret;
          }
        }
        else {  // geometry node(s) under a dummy node (in pParentMaxNode)
          pExistingNode = GetChildNodeFromName(iObj.getName(), pParentMaxNode);
          if (options.attachToExisting && pExistingNode) {
            pMaxNode = pExistingNode;
          }  // only create node if either attachToExisting is false or it is
             // true and the object does not already exist

          int ret = createAlembicObject(iObj, &pMaxNode, options, file);
          if (ret != 0) return ret;

          // since the transform is the identity, should position relative to
          // parent
          keepTM = 0;

          if (AbcG::ICamera::matches(iObj.getMetaData())) {
            // apply camera adjustment matrix to the identity
            Matrix3 rotation(TRUE);
            rotation.RotateX(HALFPI);
            TimeValue zero(0);
            pMaxNode->SetNodeTM(zero, rotation);
          }

          // import identity matrix, since more than goemetry node share the
          // same transform
          // Should we just list MAX put a default position/scale/rotation
          // controller on?

          //	int ret = AlembicImport_XForm(pMaxNode, *piParentObj, file,
          //options);
        }

        if (options.failOnUnsupported) {
          if (!pMaxNode) {
            return alembic_failure;
          }
        }
      }
    }

    if (pMaxNode && pParentMaxNode && !pExistingNode) {
      pParentMaxNode->AttachChild(pMaxNode, keepTM);
    }

    progress.increment();
    progress.update();

    if (pMaxNode) {
      for (size_t j = 0; j < sElement.pObjectCache->childIdentifiers.size();
           j++) {
        AbcObjectCache *pChildObjectCache =
            &(pArchiveCache->find(sElement.pObjectCache->childIdentifiers[j])
                  ->second);
        if (NodeCategory::get(pChildObjectCache->obj) ==
            NodeCategory::UNSUPPORTED)
          continue;  // skip over unsupported types

        // I assume that geometry nodes are always leaf nodes. Thus, if we
        // merged a geometry node will its parent transform, we don't
        // need to push it to the stack.
        // A geometry node can't be combined with its transform node, the
        // transform node has other tranform nodes as children. These
        // nodes must be pushed.
        if (mergedGeomNodeIndex != j) {
          sceneStack.push_back(stackElement(pChildObjectCache, pMaxNode));
        }
      }
    }
  }

  return alembic_success;
}