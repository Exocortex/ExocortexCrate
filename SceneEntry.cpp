#include "alembic.h"
#include "SceneEntry.h"
#include <inode.h>

SceneEntry::SceneEntry(INode *n, Object *o, int t) 
{ 
	node = n; 
    obj = o; 
    type = t; 
    next = NULL; 
	tnode = n->GetTarget();
}

void SceneEntry::SetID(int id) 
{ 
    this->id = id; 
}