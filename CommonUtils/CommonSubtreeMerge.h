#ifndef __ALEMBIC_COMMON_SUBTREE_MERGE_H__
#define __ALEMBIC_COMMON_SUBTREE_MERGE_H__


#include "CommonSceneGraph.h"
#include "CommonIntermediatePolyMesh.h"


class SceneNodePolyMeshSubtree : public SceneNode
{
public:

   std::vector<SceneNodePtr> polyMeshNodes;

   //need to store reference to this. This must not be destroyed before we have a change retrieve global transform when exporting each frame
   SceneNodePtr commonRoot;

   //SceneNodePolyMeshSubtree(): SceneNode(SceneNode::POLYMESH_SUBTREE, std::string(""), std::string("")) {}
   SceneNodePolyMeshSubtree(std::string name, std::string identifier): SceneNode(SceneNode::POLYMESH_SUBTREE, name, identifier) {}

   void print(){ ESS_LOG_WARNING("SceneNodePolyMeshSubtree"); }
};

typedef boost::shared_ptr<SceneNodePolyMeshSubtree> SceneNodePolyMeshSubtreePtr;

//adjusts tree node links, but DOES NOT update the transforms, this will be done later in mergePolyMeshSubtreeNode method
void replacePolyMeshSubtree(SceneNodePtr root);


template <class T> void mergePolyMeshSubtreeNode(SceneNodePolyMeshSubtreePtr node, T& mergedMesh, const CommonOptions& options, double time)
{
   Imath::M44f subtreeRootGlobalTransInv = node->commonRoot->getGlobalTrans(time).invert();

   for(int i=0; i<node->polyMeshNodes.size(); i++){
      T currentMesh;
      Imath::M44f currentGlobalTrans = subtreeRootGlobalTransInv * node->polyMeshNodes[i]->getGlobalTrans(time);
       //Put the merged mesh in the space of the common parent. Position the merged Mesh Shape node at the origin for now. 

      currentMesh.Save(node->polyMeshNodes[i], currentGlobalTrans, options, time);
      mergedMesh.mergeWith(currentMesh);
   }
}



#endif