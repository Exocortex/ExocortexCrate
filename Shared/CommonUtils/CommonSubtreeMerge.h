#ifndef __ALEMBIC_COMMON_SUBTREE_MERGE_H__
#define __ALEMBIC_COMMON_SUBTREE_MERGE_H__


#include "CommonSceneGraph.h"
#include "CommonIntermediatePolyMesh.h"


class SceneNodePolyMeshSubtree : public SceneNode
{
public:

   std::vector<SceneNodePtr> polyMeshNodes;

   //TODO: probably do not need to store this anymore (because we will keep the scenegraph root around until export is complete)
   //need to store reference to this. This must not be destroyed before we have a change retrieve global transform when exporting each frame
   //SceneNodePtr commonRoot;

   //SceneNodePolyMeshSubtree(): SceneNode(SceneNode::POLYMESH_SUBTREE, std::string(""), std::string("")) {}
   SceneNodePolyMeshSubtree(std::string name, std::string identifier): SceneNode(SceneNode::POLYMESH_SUBTREE, name, identifier) {}

   void print(){ ESS_LOG_WARNING("SceneNodePolyMeshSubtree"); }
};

typedef boost::shared_ptr<SceneNodePolyMeshSubtree> SceneNodePolyMeshSubtreePtr;

SceneNodePolyMeshSubtreePtr findPolyMeshChildren(SceneNodePtr root);
//adjusts tree node links, but DOES NOT update the transforms, this will be done later in mergePolyMeshSubtreeNode method
template<class S, class T> void replacePolyMeshSubtree(SceneNodePtr root)
{
   ESS_PROFILE_FUNC();

   SceneNodePolyMeshSubtreePtr mergedMeshNode = findPolyMeshChildren(root);

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
  // selectPolyMeshShapeNodes(commonRoot);
   //removeUnselectedNodes(commonRoot);


   //here we make a copy of the common root. Each shape should have its own parent transform node.
   //We want it to have the same global transform as the common root.

   
   S commonNodeApp = reinterpret<SceneNode, T>(commonRoot);

   S sceneNode(new T(*commonNodeApp.get(), true));
   sceneNode->name = "MergedPolyMesh";
   sceneNode->type = SceneNode::ETRANSFORM;
   sceneNode->dccIdentifier = commonNodeApp->dccIdentifier;
   sceneNode->children.push_back(mergedMeshNode);
   commonRoot->children.clear();
   commonRoot->children.push_back(sceneNode);

   sceneNode->parent = commonRoot.get();
   mergedMeshNode->parent = sceneNode.get();


   //commonRoot->children.push_back(mergedMeshNode);
   //mergedMeshNode->parent = commonRoot.get();

   //replace subtree with merging node
   for(int i=0; i<mergedMeshNode->polyMeshNodes.size(); i++){
      mergedMeshNode->polyMeshNodes[i]->parent = NULL;
   }

   //mergedMeshNode->commonRoot = commonRoot; 
}



template <class T> void mergePolyMeshSubtreeNode(SceneNodePolyMeshSubtreePtr node, T& mergedMesh, const CommonOptions& options, double time)
{
   Imath::M44f subtreeRootGlobalTransInv = node->parent->getGlobalTransFloat(time).invert();

   for(int i=0; i<node->polyMeshNodes.size(); i++){
      T currentMesh;
      Imath::M44f currentGlobalTrans = node->polyMeshNodes[i]->getGlobalTransFloat(time) * subtreeRootGlobalTransInv;
       //Put the merged mesh in the space of the common parent. Position the merged Mesh Shape node at the origin for now. 

      currentMesh.Save(node->polyMeshNodes[i], currentGlobalTrans, options, time);
      mergedMesh.mergeWith(currentMesh);
   }
}



#endif