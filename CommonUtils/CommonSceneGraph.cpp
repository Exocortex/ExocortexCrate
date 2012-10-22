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


void selectParentsAndChildren(exoNodePtr root, exoNode::SelectionMap selectionMap)
{
   struct stackElement
   {
      exoNodePtr eNode;
      bool bSelectChildren;
      stackElement(exoNodePtr enode, bool selectChildren):eNode(enode), bSelectChildren(selectChildren)
      {}
   };


   std::list<stackElement> sceneStack;
   
   sceneStack.push_back(stackElement(root, false));

   while( !sceneStack.empty() )
   {

      stackElement sElement = sceneStack.back();
      exoNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();

      bool bSelected = false;
      if(selectionMap.find(eNode->name) != selectionMap.end()){
         bSelected = true;
         exoNodePtr currNode = eNode;
         while(currNode){
            currNode->selected = true;
            currNode = currNode->parent;
         }
      }
      if(sElement.bSelectChildren){
         bSelected = true;
         eNode->selected = true;
      }

      for( std::list<exoNodePtr>::iterator it = eNode->children.begin(); it != eNode->children.end(); it++){
         sceneStack.push_back(stackElement(*it, bSelected));
      }
   }



}