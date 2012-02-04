#ifndef _SCENE_ENTRY_H_
#define _SCENE_ENTRY_H_
#include <string>

class INode;
class Object;

class SceneEntry 
{
public:
    std::string fullname;
	INode *node;
    INode *tnode;
	Object *obj;
	int type;		
	int id;
	SceneEntry *next;
    SceneEntry(INode *n, Object *o, int t, std::string *providedfullname);
	void SetID(int id);
};

#endif //_SCENE_ENTRY_H_