#include "stdafx.h"

#include "ObjectList.h"
#include "Alembic.h"
#include "SceneEnumProc.h"
#include "Utility.h"

ObjectEntry::ObjectEntry(SceneEntry *e) { entry = *e; }
ObjectList::ObjectList() {}
ObjectList::ObjectList(SceneEnumProc &scene) { FillList(scene); }
ObjectList::~ObjectList() {}
ObjectEntry *ObjectList::Contains(INode *node)
{
  for (int i = 0; i < this->objectEntries.size(); i++) {
    ObjectEntry *e = &(this->objectEntries[i]);
    if (e->entry.node == node) {
      return e;
    }
  }
  return NULL;
}

void ObjectList::Append(SceneEntry *e)
{
  if (e->type == OBTYPE_MESH && Contains(e->node)) {
    return;
  }

  ObjectEntry oe(e);
  this->objectEntries.push_back(oe);
}

void ObjectList::FillList(SceneEnumProc &scene)
{
  ClearList();
  for (int i = 0; i < scene.sceneEntries.size(); i++) {
    SceneEntry *e = &(scene.sceneEntries[i]);
    Append(e);
  }
}

void ObjectList::ClearList() { this->objectEntries.clear(); }
