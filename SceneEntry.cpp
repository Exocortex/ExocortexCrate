#include "Alembic.h"
#include "SceneEntry.h"
#include <inode.h>
#include "Utility.h"

SceneEntry::SceneEntry(INode *n, Object *o, int t, std::string *providedfullname) 
{ 
	node = n; 
    obj = o; 
    type = t; 
    next = NULL; 
	tnode = n->GetTarget();

    if (providedfullname == 0)
        fullname = buildIdentifierFromRef(*this);
    else
        fullname = *providedfullname;
}

void SceneEntry::SetID(int id) 
{ 
    this->id = id; 
}