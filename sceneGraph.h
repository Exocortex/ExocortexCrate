#ifndef _SCENEGRAPH_H_
#define _SCENEGRAPH_H_

#include "CommonSceneGraph.h"
#include "CommonImport.h"

class Object;
class PolyObject;
class TriObject;
class Mesh;
class MNMesh; 
class MeshData
{
public:
   Object *obj;
	PolyObject *polyObj;
	TriObject *triObj;
   Mesh* triMesh;
   MNMesh* polyMesh; 

   MeshData():polyObj(NULL), triObj(NULL), triMesh(NULL), polyMesh(NULL)
   {}

   void free(){
	   // Note that the TriObject should only be deleted
	   // if the pointer to it is not equal to the object
	   // pointer that called ConvertToType()
	   if (polyObj != NULL && polyObj != obj)
	   {
		   delete polyObj;
		   polyObj = NULL;
	   }

	   if (triObj != NULL && triObj != obj)
	   {
		   delete triObj;
		   triObj = NULL;
	   }
   }
};

class INode;
class SceneNodeMax : public SceneNodeApp
{
public:

   INode *node;
   bool bMergedSubtreeNodeParent;
   bool bIsCameraTransform;

   SceneNodeMax(INode *n):node(n), bMergedSubtreeNodeParent(false), bIsCameraTransform(false)
   {}

   SceneNodeMax(const SceneNodeMax& n, bool mergedSubtreeNodeParent):
      node(n.node), bMergedSubtreeNodeParent(mergedSubtreeNodeParent), bIsCameraTransform(false)
   {}

	MeshData getMeshData(double time);

   //Import methods, we won't need these until we update the importer
   virtual bool replaceData(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAlembicPtr& nextFileNode);
   virtual bool addChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode);
   virtual void print();
   
   virtual Imath::M44f getGlobalTransFloat(double time);
   virtual Imath::M44d getGlobalTransDouble(double time);
   virtual bool getVisibility(double time);
};

typedef boost::shared_ptr<SceneNodeMax> SceneNodeMaxPtr;

SceneNodeMaxPtr buildCommonSceneGraph(int& nNumNodes, bool bUnmergeNodes, bool bSelectAll);





#endif