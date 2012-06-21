#include "Alembic.h"
#include "AlembicMax.h"
#include "ObjectList.h"

#include "SceneEnumProc.h"
#include "Utility.h"

ObjectEntry::ObjectEntry(SceneEntry *e) 
{ 
    entry = *e; 
}


ObjectList::ObjectList() 
{
}

ObjectList::ObjectList(SceneEnumProc &scene) 
{
    FillList(scene);
}

ObjectList::~ObjectList() 
{
}

ObjectEntry *ObjectList::Contains(INode *node) 
{
	for( int i = 0; i < this->objectEntries.size(); i ++ ) {
		ObjectEntry *e = &( this->objectEntries[i] );
		if(e->entry.node == node)
        {
			return e;
        }
	}
	return NULL;
}

void ObjectList::Append(SceneEntry *e) 
{
    if (e->type == OBTYPE_MESH && Contains(e->node))
        return;

	ObjectEntry oe( e );
	this->objectEntries.push_back( oe );
}

void ObjectList::FillList(SceneEnumProc &scene)
{
    ClearList();
	for( int i = 0; i < scene.sceneEntries.size(); i ++ ) {
		SceneEntry *e = &( scene.sceneEntries[ i ] );
		Append( e );
	}
}

void ObjectList::ClearList()
{
	this->objectEntries.clear();
}

INode* ObjectList::FindNodeWithFullName(std::string &identifier)
{
    // Max Scene nodes are also identified by their transform nodes since an INode contains
    // both the transform and the shape.  So if we find an "xfo" at the end of the identifier
    // then we extract the model name from the identifier
    std::string modelName = getModelFullName(identifier);

	for( int i = 0; i < this->objectEntries.size(); i ++ ) {
		ObjectEntry *e = &( this->objectEntries[i] );
		if(e->entry.fullname == identifier)
        {
			return e->entry.node;
        }
	}

    return NULL;
}

INode* ObjectList::FindNodeWithName(std::string &identifier, bool removeXfo)
{
    // Max Scene nodes are also identified by their transform nodes since an INode contains
    // both the transform and the shape.  So if we find an "xfo" at the end of the identifier
    // then we extract the model name from the identifier
	std::string modelName = identifier;
    if(removeXfo) modelName = getModelName(identifier);
    
	// we get the last node to ensure that it is one of the new nodes we just imported rather than one with the same name that already existed.
	INode* pLastNode = NULL;
   	for( int i = 0; i < this->objectEntries.size(); i ++ ) {
		ObjectEntry *e = &( this->objectEntries[i] );
		if(e->entry.node->GetName() == modelName)
        {
			pLastNode = e->entry.node;
        }
	}

    return pLastNode;
}