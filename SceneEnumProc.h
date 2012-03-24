#ifndef _SCENEENUMPROC_H_
#define _SCENEENUMPROC_H_
#include <inode.h>
#include <box3.h>

class SceneEntry;
class IScene;

#define OBTYPE_MESH 0
#define OBTYPE_CAMERA 1
#define OBTYPE_OMNILIGHT 2
#define OBTYPE_SPOTLIGHT 3
#define OBTYPE_DUMMY 5
#define OBTYPE_CTARGET 6
#define OBTYPE_LTARGET 7
#define OBTYPE_POINTS 8
#define OBTYPE_CURVES 9

class SceneEnumProc : public ITreeEnumProc 
{
public:
	Interface	*i;
	SceneEntry *head;
	SceneEntry *tail;
	IScene		*theScene;
	int			count;
	TimeValue	time;
public:
                SceneEnumProc();
	            SceneEnumProc(IScene *scene, TimeValue t, Interface *i);
                ~SceneEnumProc();
	int			Count() { return count; }
    SceneEntry*	Append(INode *node, Object *obj, int type, std::string *providedfullname);
	int			callback( INode *node );
	Box3		Bound();
	SceneEntry *Find(INode *node);
    void        Init(IScene *scene, TimeValue t, Interface *i);
};

#endif //_SCENEENUMPROC_H_