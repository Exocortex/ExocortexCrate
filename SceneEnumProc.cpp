#include "Alembic.h"
#include "AlembicMax.h"
#include "SceneEnumProc.h"
#include "Utility.h"

SceneEntry::SceneEntry(INode *n, Object *o, int t, std::string *providedfullname) 
{ 
	node = n; 
    obj = o; 
    type = t; 
	tnode = n->GetTarget();

    if (providedfullname == 0)
        fullname = buildIdentifierFromRef(*this);
    else
        fullname = *providedfullname;
}

void SceneEntry::SetID(int id) 
{ 
    this->id = id; 
}

SceneEntry createSceneEntry(INode* node, TimeValue time, std::string* pFullname)
{
	ESS_CPP_EXCEPTION_REPORTING_START

	Object *obj = node->EvalWorldState(time).obj;
    SClass_ID superClassID = obj->SuperClassID();
    Class_ID classID = obj->ClassID();

    if (obj->IsParticleSystem()){
        return SceneEntry(node, obj, OBTYPE_POINTS, pFullname);
    }

	if (obj->IsShapeObject() == FALSE &&
        (obj->CanConvertToType(polyObjectClassID) || 
         obj->CanConvertToType(triObjectClassID))){
		return SceneEntry(node, obj, OBTYPE_MESH, pFullname);
	}

	if (node->IsTarget()) 
    {
		INode* ln = node->GetLookatNode();
		if (ln) 
        {
			Object *lobj = ln->EvalWorldState(time).obj;
			switch(lobj->SuperClassID()) 
            {
				case LIGHT_CLASS_ID:  return SceneEntry(node, obj, OBTYPE_LTARGET, pFullname); break;
				case CAMERA_CLASS_ID: return SceneEntry(node, obj, OBTYPE_CTARGET, pFullname); break;
			}
		}
	}

	switch (superClassID) 
    { 
		case HELPER_CLASS_ID:
			if (classID == Class_ID(DUMMY_CLASS_ID, 0))
            {
				return SceneEntry(node, obj, OBTYPE_DUMMY, pFullname);
            }
			break;
		case LIGHT_CLASS_ID: 
            {
                /* LIGHT */
                break;
            }
		case CAMERA_CLASS_ID:
			if (classID == Class_ID(LOOKAT_CAM_CLASS_ID, 0) ||
                classID == Class_ID(SIMPLE_CAM_CLASS_ID, 0))
            {
				return SceneEntry(node, obj, OBTYPE_CAMERA, pFullname);
            }
			break;
        case SHAPE_CLASS_ID:
            if (obj->IsShapeObject() == TRUE)
            {
                return SceneEntry(node, obj, OBTYPE_CURVES, pFullname);
            }
            break;
	}

	ESS_CPP_EXCEPTION_REPORTING_END

	return SceneEntry();
}


SceneEnumProc::SceneEnumProc()
{
    time = 0;
	theScene = 0;
	i = 0;
}

SceneEnumProc::SceneEnumProc(IScene *scene, TimeValue t, Interface *i ) 
{
	Init(scene, t, i);
}

SceneEnumProc::~SceneEnumProc() 
{	
}

int SceneEnumProc::callback(INode *node) 
{
	SceneEntry sEntry = createSceneEntry(node, time, 0);
	
	if(sEntry.node){
		this->sceneEntries.push_back(sEntry);
	}
	
	return TREE_CONTINUE;
}


SceneEntry *SceneEnumProc::Append(INode *node, Object *obj, int type, std::string *providedfullname) 
{
	this->sceneEntries.push_back( SceneEntry(node, obj, type, providedfullname) );
    return &( this->sceneEntries[ this->sceneEntries.size() - 1 ] );
}

Box3 SceneEnumProc::Bound() 
{
	Box3 bound;
	bound.Init();
	ViewExp *vpt = i->GetViewport(NULL);
	for( int i = 0; i < this->sceneEntries.size(); i ++ ) {
		SceneEntry *e = &( this->sceneEntries[ i ] );
		Box3 bb;
		e->obj->GetWorldBoundBox(time, e->node, vpt, bb);
		bound += bb;
	}
	return bound;
}

SceneEntry *SceneEnumProc::Find(INode *node) 
{
	for( int i = 0; i < this->sceneEntries.size(); i ++ ) {
		SceneEntry *e = &( this->sceneEntries[ i ] );

		if(e->node == node)
        {
			return e;
        }
	}

	return NULL;
}

void SceneEnumProc::Init(IScene *scene, TimeValue t, Interface *i )
{
    time = t;
	theScene = scene;
	this->sceneEntries.clear();
	this->i = i;
	theScene->EnumTree(this);
}
