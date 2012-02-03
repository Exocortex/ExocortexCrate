#include "Foundation.h"
#include "SceneEntry.h"
#include <inode.h>

SceneEntry::SceneEntry(INode *n, Object *o, int t) 
{ 
	node = n; 
    obj = o; 
    type = t; 
    next = NULL; 
	tnode = n->GetTarget();

    // Build an 'Alembic' like name
    /*name = node->GetName();
    INode *pWalkNode = node->GetParentNode();;
    while (pWalkNode)
    {
        name.insert(0, '//');
        name.insert(0, pWalkNode->GetName());
        pWalkNode = node->GetParentNode();
    }
    */
}

void SceneEntry::SetID(int id) 
{ 
    this->id = id; 
}