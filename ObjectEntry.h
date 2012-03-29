#ifndef _OBJECT_ENTRY_H_
#define _OBJECT_ENTRY_H_
#include <triobj.h>
#include "SceneEnumProc.h"

// We need to maintain a list of the unique objects in the scene
class ObjectEntry 
{
public:
	TriObject *tri;			// 3D Max object - triangular mesh
	SceneEntry *entry;		
	ObjectEntry *next;
	ObjectEntry(SceneEntry *e);
};

#endif //_OBJECT_ENTRY_H_