#ifndef __COMMON_SCENE_GRAPH_H
#define __COMMON_SCENE_GRAPH_H


#include <boost/smart_ptr.hpp>
#include <list>
#include <string>
#include <map>
#include "CommonAlembic.h"

class SceneNode;
typedef boost::shared_ptr<SceneNode> SceneNodePtr;
typedef SceneNode* WeakSceneNodePtr;

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

   WeakSceneNodePtr parent;
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
   virtual bool replaceData(SceneNodePtr node, const IJobStringParser& jobParams){ return false; }
   virtual bool attachChild(SceneNodePtr node, const IJobStringParser& jobParams){ return false; }

   //for alembic scene graph
   virtual Abc::IObject getObject(){ return Abc::IObject(); }
};




void printSceneGraph(SceneNodePtr root, bool bOnlyPrintSelected);

bool hasExtractableTransform( SceneNode::nodeTypeE type );

void selectNodes(SceneNodePtr root, SceneNode::SelectionT selectionMap, bool bSelectParents, bool bChildren, bool bSelectShapeNodes);

//void filterNodeSelection(SceneNodePtr root, bool bExcludeNonTransforms);


#endif