#include "CommonSubtreeMerge.h"





struct MergeChildrenStackElement
{
   SceneNodePtr eNode;
   MergeChildrenStackElement(SceneNodePtr enode):eNode(enode)
   {}
};


void replacePolyMeshSubtree(SceneNodePtr root)
{
   ESS_PROFILE_FUNC();

   SceneNodePolyMeshSubtreePtr mergedMeshNode(new SceneNodePolyMeshSubtree());

   std::list<MergeChildrenStackElement> sceneStack;
   
   sceneStack.push_back(MergeChildrenStackElement(root));

   while( !sceneStack.empty() )
   {
      MergeChildrenStackElement sElement = sceneStack.back();
      SceneNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();

      if(eNode->type == SceneNode::POLYMESH){   
         mergedMeshNode->polyMeshNodes.push_back(eNode);
         
      }

      for( std::list<SceneNodePtr>::iterator it = eNode->children.begin(); it != eNode->children.end(); it++){
         sceneStack.push_back(MergeChildrenStackElement(*it));
      }
   }

   //TODO: modify tree to have counting integer. 
   //Walk up from each mesh node, and increment the counter. The first node to get a count equal to the number of mesh nodes is the common parent.

   //Put the merged mesh in the space of the common parent. Position the merged Mesh Shape node at the origin for now. 


   SceneNodePtr commonRoot;

   commonRoot = root; //assume root is the commonRoot for now

   //replace subtree with merging node
   for(int i=0; i<mergedMeshNode->polyMeshNodes.size(); i++){
      mergedMeshNode->polyMeshNodes[i]->parent = NULL;
   }

   commonRoot->children.clear();
   commonRoot->children.push_back(mergedMeshNode);
}


