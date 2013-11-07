#ifndef __ALEMBIC_COMMON_SUBTREE_MERGE_H__
#define __ALEMBIC_COMMON_SUBTREE_MERGE_H__


#include "CommonSceneGraph.h"
#include "CommonIntermediatePolyMesh.h"


class SceneNodePolyMeshSubtree : public SceneNode
{
public:

   std::vector<SceneNodePtr> polyMeshNodes;

	SceneNodePolyMeshSubtree(void): SceneNode() {}

   void print(){ ESS_LOG_WARNING("SceneNodePolyMeshSubtree"); }
};

typedef boost::shared_ptr<SceneNodePolyMeshSubtree> SceneNodePolyMeshSubtreePtr;

//
void replacePolyMeshSubtree(SceneNodePtr root);


template <class T> void mergePolyMeshSubtreeNode(SceneNodePolyMeshSubtree node, T& mergedMesh, const CommonOptions& options, double time)
{
   for(int i=0; i<node.polyMeshNodes.size(); i++){
      T currentMesh;
      math::M44f currentGlobalTrans = node.polyMeshNodes[i]->getGlobalTrans();
      currentMesh.Save(node.polyMeshNodes[i], currentGlobalTrans, options, time);
      mergedMesh.mergeWith(currentMesh);
   }
}



#endif