#include "CommonSceneGraph.h"
#include "CommonLog.h"

void printSceneGraph(exoNodePtr root)
{
   struct stackElement
   {
      exoNodePtr eNode;
      int level;
      stackElement(exoNodePtr enode, int l):eNode(enode), level(l)
      {}
   };

   ESS_LOG_WARNING("ExoSceneGraph Begin");

   std::list<stackElement> sceneStack;
   
   sceneStack.push_back(stackElement(root, 0));

   while( !sceneStack.empty() )
   {

      stackElement sElement = sceneStack.back();
      exoNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();

      ESS_LOG_WARNING("Level: "<<sElement.level<<" - Name: "<<eNode->name<<" - Selected: "<<(eNode->selected)?"true":"false");
         //<<" - identifer: "<<eNode->dccIdentifier);

      for( std::list<exoNodePtr>::iterator it = eNode->children.begin(); it != eNode->children.end(); it++){
         sceneStack.push_back(stackElement(*it, sElement.level+1));
      }
   }

   ESS_LOG_WARNING("ExoSceneGraph End");
}



void selectNodes(exoNodePtr root, exoNode::SelectionMap selectionMap, bool bParents, bool bChildren, bool bExcludeITransforms, bool bExcludeNonTransforms)
{
   struct stackElement
   {
      exoNodePtr eNode;
      bool bSelectChildren;
      stackElement(exoNodePtr enode, bool selectChildren):eNode(enode), bSelectChildren(selectChildren)
      {}
   };

   struct NodeSelector
   {
      bool bExcludeITransforms;
      bool bExcludeNonTransforms;

      NodeSelector(bool eit, bool nt):bExcludeITransforms(eit), bExcludeNonTransforms(nt)
      {}

      void operator()(exoNodePtr node)
      {
         node->selected = true;

         if(bExcludeITransforms && node->type == exoNode::ITRANSFORM){
            node->selected = false;
         }

         if(bExcludeNonTransforms && 
            (node->type != exoNode::ITRANSFORM || node->type != exoNode::ETRANSFORM || node->type != exoNode::UNKNOWN)
         ){
            node->selected = false;
         }
      }
   };

   NodeSelector selector(bExcludeITransforms, bExcludeNonTransforms);


   std::list<stackElement> sceneStack;
   
   sceneStack.push_back(stackElement(root, false));

   while( !sceneStack.empty() )
   {

      stackElement sElement = sceneStack.back();
      exoNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();

      bool bSelected = false;
      if(selectionMap.find(eNode->name) != selectionMap.end()){
         
         //this node's name matches one of the names from the selection map, so select it
         selector(eNode);

         if(bParents){// select the parent nodes
            exoNodePtr currNode = eNode->parent;
            while(currNode){
               selector(currNode);
               currNode = currNode->parent;
            }
         }

         if(bChildren){// select the children
            bSelected = true;
         }
      }
      if(sElement.bSelectChildren){
         bSelected = true;
         selector(eNode);
      }

      for( std::list<exoNodePtr>::iterator it = eNode->children.begin(); it != eNode->children.end(); it++){
         sceneStack.push_back(stackElement(*it, bSelected));
      }
   }



}