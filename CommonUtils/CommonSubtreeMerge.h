#ifndef __ALEMBIC_COMMON_SUBTREE_MERGE_H__
#define __ALEMBIC_COMMON_SUBTREE_MERGE_H__


#include "CommonSceneGraph.h"
#include "CommonIntermediatePolyMesh.h"


class SceneNodePolyMeshSubtree : public SceneNode
{
public:

   std::vector<SceneNodePtr> polyMeshNodes;

   //SceneNodePolyMeshSubtree(): SceneNode(SceneNode::POLYMESH_SUBTREE, std::string(""), std::string("")) {}
   SceneNodePolyMeshSubtree(std::string name, std::string identifier): SceneNode(SceneNode::POLYMESH_SUBTREE, name, identifier) {}

   void print(){ ESS_LOG_WARNING("SceneNodePolyMeshSubtree"); }
};

typedef boost::shared_ptr<SceneNodePolyMeshSubtree> SceneNodePolyMeshSubtreePtr;

//
void replacePolyMeshSubtree(SceneNodePtr root);


template <class T> void mergePolyMeshSubtreeNode(SceneNodePolyMeshSubtreePtr node, T& mergedMesh, const CommonOptions& options, double time)
{
   for(int i=0; i<node->polyMeshNodes.size(); i++){
      T currentMesh;
      Imath::M44f currentGlobalTrans = node->polyMeshNodes[i]->getGlobalTrans(time);
      currentMesh.Save(node->polyMeshNodes[i], currentGlobalTrans, options, time);
      mergedMesh.mergeWith(currentMesh);
   }
}



#endif