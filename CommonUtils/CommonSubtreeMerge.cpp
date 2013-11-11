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

   SceneNodePolyMeshSubtreePtr mergedMeshNode(new SceneNodePolyMeshSubtree("MergedPolyMesh", ""));

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

   //if only one polymesh node is in the list, do nothing (no merging is required)
   if(mergedMeshNode->polyMeshNodes.size() <= 1){
      return;
   }

   //Walk up from each mesh node, and increment the counter. The first node to get a count equal to the number of mesh nodes is the common parent.

   SceneNodePtr commonRoot = root;

   int nNumMeshes = (int)mergedMeshNode->polyMeshNodes.size();
   for(int i=0; i<mergedMeshNode->polyMeshNodes.size(); i++){

      SceneNode* currNode = mergedMeshNode->polyMeshNodes[i].get();
      while(currNode){

         currNode->nCommonParentCounter++;

         if(currNode->nCommonParentCounter == nNumMeshes){
            //looks ugly, but we don't to delete twice by creating a new shared pointer that unaware of the old ones that refernece the object
            
            SceneNode* parent = currNode->parent;
            if(parent){
               for( SceneChildIterator it=parent->children.begin(); it != parent->children.end(); it++){
                  if( it->get() == currNode ){
                     commonRoot = *it;
                     goto done;
                  }
               }
            }
            else{// commonRoot is the scene root
               goto done;
            }
         }

         currNode = currNode->parent;
      }

   }
done:

   //remove the polymesh subtree
   selectNonPolyMeshLeafNodes(commonRoot);
   removeUnselectedNodes(commonRoot);


   //TODO: probably should add a parent model/null
   commonRoot->children.push_back(mergedMeshNode);

   //replace subtree with merging node
   for(int i=0; i<mergedMeshNode->polyMeshNodes.size(); i++){
      mergedMeshNode->polyMeshNodes[i]->parent = NULL;
   }

   mergedMeshNode->commonRoot = commonRoot; 
}


