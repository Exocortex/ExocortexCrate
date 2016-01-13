#ifndef _SCENEGRAPH_H_
#define _SCENEGRAPH_H_

#include "CommonImport.h"
#include "CommonSceneGraph.h"

class Object;
class PolyObject;
class TriObject;
class Mesh;
class MNMesh;
class MeshData {
 public:
  Object* obj;
  PolyObject* polyObj;
  TriObject* triObj;
  Mesh* triMesh;
  MNMesh* polyMesh;

  MeshData(Mesh* mesh)
      : obj(NULL), polyObj(NULL), triObj(NULL), triMesh(mesh), polyMesh(NULL)
  {
  }

  MeshData()
      : obj(NULL), polyObj(NULL), triObj(NULL), triMesh(NULL), polyMesh(NULL)
  {
  }

  void free()
  {
    // Note that the TriObject should only be deleted
    // if the pointer to it is not equal to the object
    // pointer that called ConvertToType()
    if (polyObj != NULL && polyObj != obj) {
      delete polyObj;
      polyObj = NULL;
    }

    if (triObj != NULL && triObj != obj) {
      delete triObj;
      triObj = NULL;
    }
  }
};

class INode;
class SceneNodeMax : public SceneNodeApp {
 public:
  INode* node;
  bool bMergedSubtreeNodeParent;
  bool bIsCameraTransform;

  SceneNodeMax(INode* n)
      : node(n), bMergedSubtreeNodeParent(false), bIsCameraTransform(false)
  {
  }

  SceneNodeMax(const SceneNodeMax& n, bool mergedSubtreeNodeParent)
      : node(n.node),
        bMergedSubtreeNodeParent(mergedSubtreeNodeParent),
        bIsCameraTransform(false)
  {
  }

  virtual MeshData getMeshData(double time);
  virtual Mtl* getMtl();
  virtual int getMtlID();

  // Import methods, we won't need these until we update the importer
  virtual bool replaceData(SceneNodeAlembicPtr fileNode,
                           const IJobStringParser& jobParams,
                           SceneNodeAlembicPtr& nextFileNode);
  virtual bool addChild(SceneNodeAlembicPtr fileNode,
                        const IJobStringParser& jobParams,
                        SceneNodeAppPtr& newAppNode);
  virtual void print();

  virtual Imath::M44f getGlobalTransFloat(double time);
  virtual Imath::M44d getGlobalTransDouble(double time);
  virtual bool getVisibility(double time);
};

// Used for merged particles export so we can use
// IntermediatePolyMesh3DSMax::Save
class SceneNodeMaxParticles : public SceneNodeMax {
 public:
  MeshData mMeshData;
  Mtl* mtl;
  int mMatId;

  SceneNodeMaxParticles(Mesh* mesh, Mtl* m, int id = -1)
      : SceneNodeMax(NULL), mMeshData(mesh), mtl(m), mMatId(id)
  {
  }

  virtual MeshData getMeshData(double time) { return mMeshData; }
  Mtl* getMtl() { return mtl; }
  int getMtlID() { return mMatId; }
};

typedef boost::shared_ptr<SceneNodeMax> SceneNodeMaxPtr;
typedef boost::shared_ptr<SceneNodeMaxParticles> SceneNodeMaxParticlesPtr;

namespace bcsgSelection {
enum types { ALL, APP, NONE };
};
SceneNodeMaxPtr buildCommonSceneGraph(int& nNumNodes, bool bUnmergeNodes,
                                      bcsgSelection::types selectionOption);

#endif