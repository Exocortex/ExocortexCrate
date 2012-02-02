#include "alembic.h"
#include "ObjectList.h"
#include <inode.h>
#include "ObjectEntry.h"
#include "SceneEntry.h"
#include "SceneEnumProc.h"

ObjectList::ObjectList() 
{
	head = tail = NULL;
	count = 0;
}

ObjectList::ObjectList(SceneEnumProc &scene) 
{
	head = tail = NULL;
	count = 0;
	int scount = scene.Count();
	for(SceneEntry *se = scene.head; se!=NULL; se = se->next) 
    {
		if ( (se->type!=OBTYPE_MESH)|| !Contains(se->obj))
        {
			Append(se);
        }
	}
}

ObjectList::~ObjectList() 
{
    /* JSS - This lead to crash on exporter end, will debug later
	while(head) 
    {
		ObjectEntry *next = head->next;
		delete head;
		head = next;
	}
    */

	head = tail = NULL;
	count = 0;	
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