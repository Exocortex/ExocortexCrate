#include "Foundation.h"
#include "ObjectEntry.h"

ObjectEntry::ObjectEntry(SceneEntry *e) 
{ 
    entry = e; 
    next = NULL;  
    tri = NULL; 
}