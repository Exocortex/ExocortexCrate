#ifndef __COMMON_SCENE_GRAPH_H
#define __COMMON_SCENE_GRAPH_H


#include <boost/smart_ptr.hpp>
#include <list>
#include <string>
#include <map>

class SceneNode;
typedef boost::shared_ptr<SceneNode> SceneNodePtr;

typedef std::list<SceneNodePtr>::iterator SceneChildIterator;

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
	  HAIR,
      LIGHT,
      UNKNOWN,
      NUM_NODE_TYPES
   };

   SceneNode *parent;
   std::list<SceneNodePtr> children;

   nodeTypeE type;
   std::string name;
   std::string dccIdentifier;
   bool selected;

   SceneNode():type(NUM_NODE_TYPES), selected(false), parent(0)
   {}

   SceneNode(nodeTypeE type, std::string name, std::string identifier):type(type), name(name), dccIdentifier(identifier), parent(0)
   {}


};

void printSceneGraph(SceneNodePtr root, bool bOnlyPrintSelected);

bool hasExtractableTransform( SceneNode::nodeTypeE type );

void selectNodes(SceneNodePtr root, SceneNode::SelectionT selectionMap, bool bSelectParents, bool bChildren, bool bSelectShapeNodes);

//void filterNodeSelection(SceneNodePtr root, bool bExcludeNonTransforms);


#endif