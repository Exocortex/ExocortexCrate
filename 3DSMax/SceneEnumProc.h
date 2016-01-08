#ifndef _SCENEENUMPROC_H_
#define _SCENEENUMPROC_H_

class IScene;
class Object;

#define OBTYPE_MESH 0
#define OBTYPE_CAMERA 1
#define OBTYPE_OMNILIGHT 2
#define OBTYPE_SPOTLIGHT 3
#define OBTYPE_DUMMY 5
#define OBTYPE_CTARGET 6
#define OBTYPE_LTARGET 7
#define OBTYPE_POINTS 8
#define OBTYPE_CURVES 9
#define OBTYPE_POINTS_TP 10

class SceneEntry {
 public:
  std::string fullname;
  INode *node;
  INode *tnode;
  Object *obj;
  int type;
  int id;
  SceneEntry()
  {
    this->node = NULL;
    this->tnode = NULL;
    this->obj = NULL;
  }
  SceneEntry(INode *n, Object *o, int t, std::string *providedfullname);
  void SetID(int id);
};

SceneEntry createSceneEntry(INode *node, TimeValue time,
                            std::string *pFullname);

class SceneEnumProc : public ITreeEnumProc {
 public:
  Interface *i;
  std::vector<SceneEntry> sceneEntries;
  IScene *theScene;
  TimeValue time;

 public:
  SceneEnumProc();
  SceneEnumProc(IScene *scene, TimeValue t, Interface *i);
  ~SceneEnumProc();
  int Count() { return (int)sceneEntries.size(); }
  SceneEntry *Append(INode *node, Object *obj, int type,
                     std::string *providedfullname);
  int callback(INode *node);
  // Box3		Bound();
  SceneEntry *Find(INode *node);
  void Init(IScene *scene, TimeValue t, Interface *i);
};

#endif  //_SCENEENUMPROC_H_