#ifndef __COMMON_SCENE_GRAPH_H
#define __COMMON_SCENE_GRAPH_H


#include <boost/smart_ptr.hpp>
#include <list>
#include <string>
#include <map>
#include "CommonAlembic.h"

class SceneNode;
typedef boost::shared_ptr<SceneNode> SceneNodePtr;

typedef std::list<SceneNodePtr>::iterator SceneChildIterator;

class IJobStringParser;

class SceneNode
{
public:
   typedef std::map<std::string, bool> SelectionT;

   enum nodeTypeE{
      SCENE_ROOT,
      ETRANSFORM,// external transform (a parent of a geometry node)
      ITRANSFORM,// internal transform (all other transforms)
      CAMERA,
      POLYMESH,
      SUBD,
      SURFACE,
      CURVES,
      PARTICLES,
      LIGHT,
      UNKNOWN,
      NUM_NODE_TYPES
   };

   SceneNode* parent;
   std::list<SceneNodePtr> children;

   nodeTypeE type;
   std::string name;
   std::string dccIdentifier;
   bool selected;

   SceneNode():type(NUM_NODE_TYPES), selected(false)
   {}

   SceneNode(nodeTypeE type, std::string name, std::string identifier):type(type), name(name), dccIdentifier(identifier)
   {}
   //~SceneNode();

   //for application scene graph
   virtual bool replaceData(SceneNodePtr fileNode, const IJobStringParser& jobParams){ return false; }
   virtual bool addChild(SceneNodePtr fileNode, const IJobStringParser& jobParams, SceneNodePtr newAppNode){ return false; }

   //for alembic scene graph
   virtual Abc::IObject getObject(){ return Abc::IObject(); }
   virtual bool isMerged(){ return false; }
   virtual void setMerged(bool bMerged=true){ }
   virtual bool isAttached(){ return false; }
   virtual void setAttached(bool bAttached=true){ }
};


class IJobStringParser;
class SceneNodeAlembic;
class SceneNodeApp : public SceneNode
{
public:
   virtual bool replaceData(SceneNodePtr fileNode, const IJobStringParser& jobParams){ return false; }
   virtual bool addChild(SceneNodePtr fileNode, const IJobStringParser& jobParams, SceneNodePtr newAppNode){ return false; }
};

class SceneNodeAlembic : public SceneNode
{
public:
   Abc::IObject iObj;
   bool bWasMerged;
   bool bWasAttached;

   SceneNodeAlembic(Abc::IObject& obj):iObj(obj), bWasMerged(false)
   {}

   virtual Abc::IObject getObject();

   virtual bool isMerged();
   virtual void setMerged(bool bMerged=true);

   virtual bool isAttached();
   virtual void setAttached(bool bAttached=true);
};

void printSceneGraph(SceneNodePtr root, bool bOnlyPrintSelected);

bool hasExtractableTransform( SceneNode::nodeTypeE type );

void selectNodes(SceneNodePtr root, SceneNode::SelectionT selectionMap, bool bSelectParents, bool bChildren, bool bSelectShapeNodes);

//void filterNodeSelection(SceneNodePtr root, bool bExcludeNonTransforms);


#endif