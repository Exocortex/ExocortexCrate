#ifndef _OBJECT_LIST_H_
#define _OBJECT_LIST_H_

#include "SceneEnumProc.h"

class INode;
class Object;

// We need to maintain a list of the unique objects in the scene
class ObjectEntry {
 public:
  SceneEntry entry;
  ObjectEntry(SceneEntry *e);
};

class ObjectList {
 public:
  std::vector<ObjectEntry> objectEntries;
  ObjectList();
  ObjectList(SceneEnumProc &scene);
  ~ObjectList();
  int Count() { return (int)objectEntries.size(); }
  void Append(SceneEntry *e);
  // ObjectEntry *Contains(Object *obj);
  ObjectEntry *Contains(INode *node);
  INode *FindLookatNode(INode *node);
  // INode       *FindNodeWithFullName(std::string &identifier);
  // INode       *FindNodeWithName(std::string &identifier, bool
  // removeXfo=true);
  void FillList(SceneEnumProc &scene);
  void ClearList();
};

#endif  //_OBJECT_LIST_H_