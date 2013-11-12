#include "CommonAlembic.h"
#include "CommonSceneGraph.h"
#include "CommonAbcCache.h"
#include "CommonUtilities.h"

//SceneNode::~SceneNode()
//{
//   ESS_LOG_WARNING("deleting node");
//}




void SceneNodeFile::setMerged(bool bMerged)
{
   isMergedIntoAppNode = bMerged;
}

bool SceneNodeFile::isAttached()
{
   return isAttachedToAppNode;
}

void SceneNodeFile::setAttached(bool bAttached)
{
   isAttachedToAppNode = bAttached;
}

bool SceneNodeFile::isMerged()
{
   return isMergedIntoAppNode;
}

bool SceneNodeAlembic::isSupported()
{
    return NodeCategory::get(pObjCache->obj) != NodeCategory::UNSUPPORTED;
}

Abc::IObject SceneNodeAlembic::getObject()
{
   return pObjCache->obj;
}

void SceneNodeAlembic::print()
{
   ESS_LOG_WARNING("AlembicNodeObjectFullName: "<<pObjCache->obj.getFullName());
}






struct PrintStackElement
{
   SceneNodePtr eNode;
   int level;
   PrintStackElement(SceneNodePtr enode, int l):eNode(enode), level(l)
   {}
};

void printSceneGraph(SceneNodePtr root, bool bOnlyPrintSelected)
{
   const char* classType[]={
      "FILE",
      "FILE_ALEMBIC",
      "APP",
      "APP_MAX",
      "APP_MAYA",
      "APP_XSI"
   };
 
   const char* table[]={
      "SCENE_ROOT",
      "NAMESPACE_TRANSFORM",//for export of XSI models
      "ETRANSFORM",// external transform (a parent of a geometry node)
      "ITRANSFORM",// internal transform (all other transforms)
      "CAMERA",
      "POLYMESH",
      "SUBD",
      "SURFACE",
      "CURVES",
      "PARTICLES",
	  "HAIR",
      "LIGHT",
      "UNKNOWN",
      "NUM_NODE_TYPES"
   };

   //ESS_LOG_WARNING("ExoSceneGraph Begin - ClassType: "<<classType[root->getClass()]);

   std::list<PrintStackElement> sceneStack;
   
   sceneStack.push_back(PrintStackElement(root, 0));

   while( !sceneStack.empty() )
   {

      PrintStackElement sElement = sceneStack.back();
      SceneNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();

      if(!bOnlyPrintSelected || (bOnlyPrintSelected && eNode->selected)){
         const char* name = eNode->name.c_str();

         ESS_LOG_WARNING("Level: "<<sElement.level<<" - Name: "<<eNode->name.c_str()<<" - Type: "<<table[eNode->type]<<" - ddcID: "<<eNode->dccIdentifier.c_str()<<" - Selected: "<<(eNode->selected?"true":"false"));
         //if(eNode->parent){
         //   ESS_LOG_WARNING("Parent: "<<eNode->parent->name);
         //}
         eNode->print();
      }

    

      for( std::list<SceneNodePtr>::iterator it = eNode->children.begin(); it != eNode->children.end(); it++){
         sceneStack.push_back(PrintStackElement(*it, sElement.level+1));
      }
   }

   ESS_LOG_WARNING("ExoSceneGraph End");
}


struct SelectChildrenStackElement
{
   SceneNodePtr eNode;
   bool bSelectChildren;
   SelectChildrenStackElement(SceneNodePtr enode, bool selectChildren):eNode(enode), bSelectChildren(selectChildren)
   {}
};

int selectNodes(SceneNodePtr root, SceneNode::SelectionT selectionMap, bool bSelectParents, bool bChildren, bool bSelectShapeNodes, bool isMaya)
{
   ESS_PROFILE_FUNC();

   SceneNode::SelectionT::iterator selectionEnd = selectionMap.end();

   std::list<SelectChildrenStackElement> sceneStack;

   sceneStack.push_back(SelectChildrenStackElement(root, false));

   int nSelectionCount = 0;

   while( !sceneStack.empty() )
   {
      SelectChildrenStackElement sElement = sceneStack.back();
      SceneNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();

      bool bSelected = false;
      //check if the node matches a full path

      if(eNode->type == SceneNode::ETRANSFORM || eNode->type == SceneNode::ITRANSFORM || eNode->type == SceneNode::NAMESPACE_TRANSFORM || eNode->type == SceneNode::HAIR)// removed to be able to export hair properly in Maya!
	  {   

         SceneNode::SelectionT::iterator selectionIt = selectionMap.find(eNode->dccIdentifier);

         //otherwise, check if the node names match
         if(selectionIt == selectionEnd){
            selectionIt = selectionMap.find(removeXfoSuffix(eNode->name));
         }

         if(selectionIt != selectionEnd){

            //this node's name matches one of the names from the selection map, so select it
            if(!eNode->selected) nSelectionCount++;
            eNode->selected = true;

            if(eNode->type == SceneNode::ETRANSFORM && bSelectShapeNodes)
			{
            //Select the shape nodes first
				if (isMaya)
				{
					for(std::list<SceneNodePtr>::reverse_iterator it=eNode->children.rbegin(); it != eNode->children.rend(); ++it)
					{
						if(::isShapeNode((*it)->type))//MergedPolyMesh node should work here...
						{
							if(!(*it)->selected) nSelectionCount++;
							(*it)->selected = true;
							break;
						}
					}	
				}
				else
				{
				   for(std::list<SceneNodePtr>::iterator it=eNode->children.begin(); it != eNode->children.end(); it++){
					  if(::isShapeNode((*it)->type)){
						 if(!(*it)->selected) nSelectionCount++;
						 (*it)->selected = true;
						 break;
					  }
				   }
				}
            }

            //then select the parents
            if(bSelectParents){// select all parent nodes
               SceneNode* currNode = eNode->parent;
               while(currNode){
                  if(!currNode->selected) nSelectionCount++;
                  currNode->selected = true;
                  currNode = currNode->parent;
               }
            }

            if(bChildren){// select the children
               bSelected = true;
            }
         }	// if(selectionIt != 
      }	// if(eNode->type == 
      if(sElement.bSelectChildren){
         bSelected = true;
         if(!eNode->selected) nSelectionCount++;
         eNode->selected = true;
      }

      for( std::list<SceneNodePtr>::iterator it = eNode->children.begin(); it != eNode->children.end(); it++){
         sceneStack.push_back(SelectChildrenStackElement(*it, bSelected));
      }
   }

   root->selected = true;

   return nSelectionCount;
}

int selectTransformNodes(SceneNodePtr root)
{
   ESS_PROFILE_FUNC();

   std::list<SelectChildrenStackElement> sceneStack;
   
   sceneStack.push_back(SelectChildrenStackElement(root, false));

   int nSelectionCount = 0;

   while( !sceneStack.empty() )
   {
      SelectChildrenStackElement sElement = sceneStack.back();
      SceneNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();

      if(eNode->type == SceneNode::NAMESPACE_TRANSFORM || eNode->type == SceneNode::ETRANSFORM || eNode->type == SceneNode::ITRANSFORM){   
         eNode->selected = true;
         nSelectionCount++;
      }

      for( std::list<SceneNodePtr>::iterator it = eNode->children.begin(); it != eNode->children.end(); it++){
         sceneStack.push_back(SelectChildrenStackElement(*it, false));
      }
   }

   root->selected = true;

   return nSelectionCount;
}


int selectNonPolyMeshLeafNodes(SceneNodePtr root)
{
   ESS_PROFILE_FUNC();

   std::list<SelectChildrenStackElement> sceneStack;
   
   sceneStack.push_back(SelectChildrenStackElement(root, false));

   int nSelectionCount = 0;

   while( !sceneStack.empty() )
   {
      SelectChildrenStackElement sElement = sceneStack.back();
      SceneNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();

      if(eNode->children.empty() && eNode->type != SceneNode::POLYMESH){   
         eNode->selected = true;
         nSelectionCount++;

         SceneNode* currNode = eNode->parent;
         while(currNode){
            if(!currNode->selected) nSelectionCount++;
            currNode->selected = true;
            currNode = currNode->parent;
         }
      }

      for( std::list<SceneNodePtr>::iterator it = eNode->children.begin(); it != eNode->children.end(); it++){
         sceneStack.push_back(SelectChildrenStackElement(*it, false));
      }
   }

   root->selected = true;

   return nSelectionCount;
}


struct FlattenStackElement
{
   SceneNodePtr currNode;
   SceneNodePtr currParentNode;

   FlattenStackElement(SceneNodePtr node, SceneNodePtr parentNode):currNode(node), currParentNode(parentNode)
   {}

};

void flattenSceneGraph(SceneNodePtr root, int nNumNodes)
{
   ESS_PROFILE_FUNC();

   SceneNodePtr newRoot = root;

   std::list<FlattenStackElement> sceneStack;

   //push a reference to each child to the stack
   for(SceneChildIterator it = root->children.begin(); it != root->children.end(); it++){
      SceneNodePtr fileNode = *it;
      sceneStack.push_back(FlattenStackElement(fileNode, root));
   }
   //clear the children since we may be changing what is parented to it
   root->children.clear();
 

   while( !sceneStack.empty() )
   {
      FlattenStackElement sElement = sceneStack.back();
      SceneNodePtr fileNode = sElement.currNode;
      SceneNodePtr parentNode = sElement.currParentNode;//a node from the original tree, its childrens will have been cleared
      //we will add child nodes to it that meet the correct criteria
      sceneStack.pop_back();

      //a nodes that we want keep, so connect it to the parent
      if (fileNode->type == SceneNode::NAMESPACE_TRANSFORM ||  //namespace transform (XSI model)
          fileNode->type == SceneNode::ETRANSFORM ||           //shape node parent transform
          isShapeNode(fileNode->type)                          //shape node
         ) { //mergedpolymesh node would apply here
            parentNode->children.push_back(fileNode);
            fileNode->parent = parentNode.get();
            
            nNumNodes++;
      }

      //here we set what the parent is going to be for the file node we pushing to the stack
      //    - if parentNode is passed instead of fileNode, the subtree starting at fileNode is being skipped
      if(fileNode->type == SceneNode::NAMESPACE_TRANSFORM){
         for(SceneChildIterator it = fileNode->children.begin(); it != fileNode->children.end(); it++){
            sceneStack.push_back( FlattenStackElement( *it, fileNode ) );
         }
      }
      else if(fileNode->type == SceneNode::ETRANSFORM){
         for(SceneChildIterator it = fileNode->children.begin(); it != fileNode->children.end(); it++){
            if(isShapeNode((*it)->type)){
               sceneStack.push_back( FlattenStackElement( *it, fileNode ) );
            }
            else{
               sceneStack.push_back( FlattenStackElement( *it, parentNode ) );
            }
         }
      }
      else{
         for(SceneChildIterator it = fileNode->children.begin(); it != fileNode->children.end(); it++){
            sceneStack.push_back( FlattenStackElement( *it, parentNode ) );
         }
      }

      fileNode->children.clear();
   }
}

int removeUnselectedNodes(SceneNodePtr root)
{
   ESS_PROFILE_FUNC();

   int nNumNodes = 0;

   SceneNodePtr newRoot = root;

   std::list<FlattenStackElement> sceneStack;

   //push a reference to each child to the stack
   for(SceneChildIterator it = root->children.begin(); it != root->children.end(); it++){
      SceneNodePtr fileNode = *it;
      sceneStack.push_back(FlattenStackElement(fileNode, root));
   }
   //clear the children since we may be changing what is parented to it
   root->children.clear();
 

   while( !sceneStack.empty() )
   {
      FlattenStackElement sElement = sceneStack.back();
      SceneNodePtr fileNode = sElement.currNode;
      SceneNodePtr parentNode = sElement.currParentNode;//a node from the original tree, its childrens will have been cleared
      //we will add child nodes to it that meet the correct criteria
      sceneStack.pop_back();

      if (fileNode->selected){
            parentNode->children.push_back(fileNode);
            fileNode->parent = parentNode.get();
            
            nNumNodes++;

            for(SceneChildIterator it = fileNode->children.begin(); it != fileNode->children.end(); it++){
               sceneStack.push_back( FlattenStackElement( *it, fileNode ) );
            }
      }
      else{
         //if a shape node is not selected its parent transform should become an ITRANSFORM
         if(isShapeNode(fileNode->type) && fileNode->parent && fileNode->parent->selected && fileNode->parent->type == SceneNode::ETRANSFORM){
            fileNode->parent->type = SceneNode::ITRANSFORM;
         }

         for(SceneChildIterator it = fileNode->children.begin(); it != fileNode->children.end(); it++){
            sceneStack.push_back( FlattenStackElement( *it, parentNode ) );
         }
      }

      fileNode->children.clear();
   }

   return nNumNodes;
}


Imath::M44f SceneNode::getGlobalTransFloat(double time)
{
   Imath::M44f ret;
   ret.makeIdentity();
   return ret;
}

Imath::M44d SceneNode::getGlobalTransDouble(double time)
{
   Imath::M44d ret;
   ret.makeIdentity();
   return ret;
}
