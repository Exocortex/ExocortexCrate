#include "Alembic.h"
#include "AlembicMax.h"
#include "ObjectList.h"
#include "ObjectEntry.h"
#include "SceneEnumProc.h"
#include "Utility.h"

ObjectList::ObjectList() 
{
	head = tail = NULL;
	count = 0;
}

ObjectList::ObjectList(SceneEnumProc &scene) 
{
    head = tail = NULL;
	count = 0;

    FillList(scene);
}

ObjectList::~ObjectList() 
{
    // ClearList();
}

ObjectEntry *ObjectList::Contains(Object *obj) 
{
	ObjectEntry *e;
	for (e=head; e!=NULL; e = e->next) 
    {
		if(e->entry->obj == obj)
        {
			return e;
        }
	}
	return NULL;
}

void ObjectList::Append(SceneEntry *e) 
{
    if (e->type == OBTYPE_MESH && Contains(e->obj))
        return;

    ObjectEntry *entry = new ObjectEntry(e);
    if(tail)
    {
        tail->next = entry;
    }
    tail = entry;
    if(!head)
    {
        head = entry;
    }
    count++;	
}

void ObjectList::FillList(SceneEnumProc &scene)
{
    ClearList();
	int scount = scene.Count();
	for(SceneEntry *se = scene.head; se!=NULL; se = se->next) 
			Append(se);
}

void ObjectList::ClearList()
{
    // JSS - This lead to crash on exporter end, will debug later
	while(head) 
    {
		ObjectEntry *next = head->next;
		delete head;
		head = next;
	}

	head = tail = NULL;
	count = 0;	
}

INode* ObjectList::FindNodeWithFullName(std::string &identifier)
{
    // Max Scene nodes are also identified by their transform nodes since an INode contains
    // both the transform and the shape.  So if we find an "xfo" at the end of the identifier
    // then we extract the model name from the identifier
    std::string modelName = getModelFullName(identifier);

    ObjectEntry *e;
	for (e=head; e!=NULL; e = e->next) 
    {
		if(e->entry->fullname == identifier)
        {
			return e->entry->node;
        }
	}

    return 0;
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
    ObjectEntry *e;
	for (e=head; e!=NULL; e = e->next) 
    {
		if(e->entry->node->GetName() == modelName)
        {
			pLastNode = e->entry->node;
        }
	}

    return pLastNode;
}