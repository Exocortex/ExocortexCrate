#include "CommonSubtreeMerge.h"





struct MergeChildrenStackElement
{
   SceneNodePtr eNode;
   MergeChildrenStackElement(SceneNodePtr enode):eNode(enode)
   {}
};

SceneNodePolyMeshSubtreePtr findPolyMeshChildren(SceneNodePtr root)
{
   SceneNodePolyMeshSubtreePtr mergedMeshNode(new SceneNodePolyMeshSubtree("MergedPolyMeshShape", ""));

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
      //clear selected flag, so that we can use to select nonpolymesh subtree
      eNode->selected = false;

      for( std::list<SceneNodePtr>::iterator it = eNode->children.begin(); it != eNode->children.end(); it++){
         sceneStack.push_back(MergeChildrenStackElement(*it));
      }
   }

   return mergedMeshNode;
}


