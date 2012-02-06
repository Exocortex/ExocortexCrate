#include "Alembic.h"
#include "ObjectEntry.h"

ObjectEntry::ObjectEntry(SceneEntry *e) 
{ 
    entry = e; 
    next = NULL;  
    tri = NULL; 
}