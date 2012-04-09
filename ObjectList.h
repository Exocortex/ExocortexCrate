#ifndef _OBJECT_LIST_H_
#define _OBJECT_LIST_H_

class INode;
class ObjectEntry;
class SceneEntry;
class SceneEnumProc;
class Object;

class ObjectList 
{
public:
	ObjectEntry *head;
	ObjectEntry *tail;
	int			count;
                ObjectList();
				ObjectList(SceneEnumProc &scene);
				~ObjectList();
	int			Count() { return count; }
	void		Append(SceneEntry *e);
	ObjectEntry *Contains(Object *obj);
	ObjectEntry *Contains(INode *node);
	INode		*FindLookatNode(INode *node);
    INode       *FindNodeWithFullName(std::string &identifier);
    INode       *FindNodeWithName(std::string &identifier, bool removeXfo=true);
    void        FillList(SceneEnumProc &scene);
    void        ClearList();
};

#endif //_OBJECT_LIST_H_