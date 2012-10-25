#ifndef __COMMON_SCENE_GRAPH_H
#define __COMMON_SCENE_GRAPH_H


#include <boost/smart_ptr.hpp>
#include <list>
#include <string>
#include <map>

class SceneNode;
typedef boost::shared_ptr<SceneNode> exoNodePtr;

class SceneNode
{
public:
   typedef std::map<std::string, bool> SelectionMap;

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
      UNKNOWN,
      NUM_NODE_TYPES
   };

   exoNodePtr parent;
   std::list<exoNodePtr> children;

   nodeTypeE type;
   std::string name;
   std::string dccIdentifier;
   bool selected;

   SceneNode():type(NUM_NODE_TYPES), selected(false)
   {}

   SceneNode(nodeTypeE type, std::string name, std::string identifier):type(type), name(name), dccIdentifier(identifier)
   {}


};

void printSceneGraph(exoNodePtr root);

void selectNodes(exoNodePtr root, SceneNode::SelectionMap selectionMap, bool bParents, bool bChildren);

void filterNodeSelection(exoNodePtr root, bool bExcludeITransforms, bool bExcludeNonTransforms);


#endif