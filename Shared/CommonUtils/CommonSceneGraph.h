#ifndef __COMMON_SCENE_GRAPH_H
#define __COMMON_SCENE_GRAPH_H


#include <boost/smart_ptr.hpp>
#include <list>
#include <string>
#include <map>
#include "CommonAlembic.h"

class SceneNode;
class SceneNodeApp;
class SceneNodeFile;
class SceneNodeAlembic;

typedef boost::shared_ptr<SceneNode> SceneNodePtr;
typedef boost::shared_ptr<SceneNodeApp> SceneNodeAppPtr;
typedef boost::shared_ptr<SceneNodeFile> SceneNodeFilePtr;
typedef boost::shared_ptr<SceneNodeAlembic> SceneNodeAlembicPtr;


template<class T, class U> 
boost::shared_ptr<U> reinterpret( boost::shared_ptr<T> original ) {
   return boost::dynamic_pointer_cast<U, T>(original);
   //return *((boost::shared_ptr<U>*)((void*)&original));
}

typedef std::list<SceneNodePtr>::iterator SceneChildIterator;

class IJobStringParser;



class SceneNode
{
public:
   typedef std::map<std::string, bool> SelectionT;

   enum nodeTypeE{
      SCENE_ROOT,
      NAMESPACE_TRANSFORM,//for export of XSI models
      ETRANSFORM,// external transform (a parent of a geometry node)
      ITRANSFORM,// internal transform (all other transforms)
      CAMERA,
      POLYMESH,
      POLYMESH_SUBTREE,
      SUBD,
      SURFACE,
      CURVES,
      PARTICLES,
      PARTICLES_TP,
	   HAIR,
      LIGHT,
      UNKNOWN,
      NUM_NODE_TYPES
   };

   SceneNode* parent;

   std::list<SceneNodePtr> children;
   
   nodeTypeE type;
   std::string name;
   std::string dccIdentifier;
   bool dccSelected;
   bool selected;

   int nCommonParentCounter;//used by common parent algorithm

   SceneNode():
      parent(NULL), type(NUM_NODE_TYPES), dccSelected(false), selected(false), nCommonParentCounter(0)
   {}

   SceneNode(nodeTypeE type, std::string name, std::string identifier):
      parent(NULL), type(type), dccSelected(false), selected(false), nCommonParentCounter(0), name(name), dccIdentifier(identifier)
   {}
   //~SceneNode();

   virtual Imath::M44f getGlobalTransFloat(double time);
   virtual Imath::M44d getGlobalTransDouble(double time);
   virtual bool getVisibility(double time);
   //It is better to work with global transformations if possible (I believe most DCC apps precompute them anyways), because this way we not have update our own copy the transform every frame
   //and we can still skip nodes and every will work, because it easy to compute a local transform given two global transforms

   virtual void print() = 0;
};


inline bool isShapeNode( SceneNode::nodeTypeE type )
{
   return 
      type == SceneNode::CAMERA ||
      type == SceneNode::POLYMESH ||
      type == SceneNode::POLYMESH_SUBTREE ||
      type == SceneNode::SUBD ||
      type == SceneNode::SURFACE ||
      type == SceneNode::CURVES ||
      type == SceneNode::PARTICLES ||
      type == SceneNode::PARTICLES_TP;
}

inline bool isParticleSystem( SceneNode::nodeTypeE type )
{
   return type == SceneNode::PARTICLES || type == SceneNode::PARTICLES_TP; 
}

class IJobStringParser;
class SceneNodeAlembic;
class SceneNodeApp : public SceneNode
{
public:
	SceneNodeApp(void): SceneNode() {}
	SceneNodeApp(nodeTypeE type, std::string name, std::string identifier): SceneNode(type, name, identifier) {}

   virtual bool replaceData(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAlembicPtr& nextFileNode){ return false; }
   virtual bool addChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode){ return false; }
   virtual void print() = 0;
};

class SceneNodeFile : public SceneNode
{
public:
   bool isMergedIntoAppNode;
   bool isAttachedToAppNode;

   SceneNodeFile(): isMergedIntoAppNode(false), isAttachedToAppNode(false)
   {}

   virtual bool isMerged();
   virtual void setMerged(bool bMerged);
   virtual bool isAttached();
   virtual void setAttached(bool bAttached);
   virtual bool isSupported() = 0;
   virtual void print() = 0;
};

class AbcObjectCache;
class SceneNodeAlembic : public SceneNodeFile
{
public:

   AbcObjectCache *pObjCache;
   bool bIsDirectChild;

   SceneNodeAlembic(AbcObjectCache *pObjectCache):pObjCache(pObjectCache), bIsDirectChild(false)
   {}

   virtual bool isSupported();
   virtual Abc::IObject getObject();
   virtual void print();
};


void printSceneGraph(SceneNodePtr root, bool bOnlyPrintSelected);



int selectNodes(SceneNodePtr root, SceneNode::SelectionT& selectionMap, bool bSelectParents, bool bChildren, bool bSelectShapeNodes, bool isMaya = false);
int refineSelection(SceneNodePtr root, bool bSelectParents, bool bChildren, bool bSelectShapeNodes);
int selectTransformNodes(SceneNodePtr root);
int selectPolyMeshShapeNodes(SceneNodePtr root);
int renameConflictingNodes(SceneNodePtr root, bool bValidate);
void flattenSceneGraph(SceneNodePtr root, int nNumNodes);
int removeUnselectedNodes(SceneNodePtr root);

//void filterNodeSelection(SceneNodePtr root, bool bExcludeNonTransforms);


#endif