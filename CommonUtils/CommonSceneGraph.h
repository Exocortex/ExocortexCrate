#ifndef __COMMON_SCENE_GRAPH_H
#define __COMMON_SCENE_GRAPH_H


#include <boost/smart_ptr.hpp>
#include <list>
#include <string>
#include <map>

class exoNode;
typedef boost::shared_ptr<exoNode> exoNodePtr;

class exoNode
{
public:
   typedef std::map<std::string, bool> SelectionMap;

   enum nodeTypeE{
      SCENE_ROOT,
      TRANSFORM,
      TRANSFORM_GEO,
      CAMERA,
      POLYMESH,
      SUBD,
      SURFACE,
      CURVES,
      PARTICLES,
      NUM_NODE_TYPES
   };

   exoNodePtr parent;
   std::list<exoNodePtr> children;

   nodeTypeE type;
   std::string name;
   std::string dccIdentifier;
   bool selected;

   exoNode():type(NUM_NODE_TYPES), selected(false)
   {}

   exoNode(nodeTypeE type, std::string name, std::string identifier):type(type), name(name), dccIdentifier(identifier)
   {}


};

void printSceneGraph(exoNodePtr root);

void selectParentsAndChildren(exoNodePtr root, exoNode::SelectionMap selectionMap);






#endif