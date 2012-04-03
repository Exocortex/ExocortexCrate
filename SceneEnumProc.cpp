#include "Alembic.h"
#include "AlembicMax.h"
#include "SceneEnumProc.h"
#include "Utility.h"

SceneEntry::SceneEntry(INode *n, Object *o, int t, std::string *providedfullname) 
{ 
	node = n; 
    obj = o; 
    type = t; 
    next = NULL; 
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


SceneEnumProc::SceneEnumProc()
{
    time = 0;
	theScene = 0;
	count = 0;
	head = tail = NULL;
	i = 0;
}

SceneEnumProc::SceneEnumProc(IScene *scene, TimeValue t, Interface *i ) 
{
	Init(scene, t, i);
}

SceneEnumProc::~SceneEnumProc() 
{
	while(head) 
    {
		SceneEntry *next = head->next;
		delete head;
		head = next;
	}

	head = tail = NULL;
	count = 0;	
}

int SceneEnumProc::callback(INode *node) 
{
	ESS_CPP_EXCEPTION_REPORTING_START

	Object *obj = node->EvalWorldState(time).obj;
    SClass_ID superClassID = obj->SuperClassID();
    Class_ID classID = obj->ClassID();

    if (PFSystemInterface(obj) != NULL)
    {
        Append(node, obj, OBTYPE_POINTS, 0);
        return TREE_CONTINUE;
    }

	if (obj->IsShapeObject() == FALSE &&
        (obj->CanConvertToType(polyObjectClassID) || 
         obj->CanConvertToType(triObjectClassID))) 
    {
		Append(node, obj, OBTYPE_MESH, 0);
		return TREE_CONTINUE;
	}

	if (node->IsTarget()) 
    {
		INode* ln = node->GetLookatNode();
		if (ln) 
        {
			Object *lobj = ln->EvalWorldState(time).obj;
			switch(lobj->SuperClassID()) 
            {
				case LIGHT_CLASS_ID:  Append(node, obj, OBTYPE_LTARGET, 0); break;
				case CAMERA_CLASS_ID: Append(node, obj, OBTYPE_CTARGET, 0); break;
			}
		}
		return TREE_CONTINUE;
	}

	switch (superClassID) 
    { 
		case HELPER_CLASS_ID:
			if (classID == Class_ID(DUMMY_CLASS_ID, 0))
            {
				// Append(node, obj, OBTYPE_DUMMY, 0);
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
				Append(node, obj, OBTYPE_CAMERA, 0);
            }
			break;
        case SHAPE_CLASS_ID:
            if (obj->IsShapeObject() == TRUE)
            {
                Append(node, obj, OBTYPE_CURVES, 0);
            }
            break;
	}

	ESS_CPP_EXCEPTION_REPORTING_END


	return TREE_CONTINUE;
}


SceneEntry *SceneEnumProc::Append(INode *node, Object *obj, int type, std::string *providedfullname) 
{
	SceneEntry *entry = new SceneEntry(node, obj, type, providedfullname);

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

    return entry;
}

Box3 SceneEnumProc::Bound() 
{
	Box3 bound;
	bound.Init();
	SceneEntry *e = head;
	ViewExp *vpt = i->GetViewport(NULL);
	while(e) 
    {
		Box3 bb;
		e->obj->GetWorldBoundBox(time, e->node, vpt, bb);
		bound += bb;
		e = e->next;
	}
	return bound;
}

SceneEntry *SceneEnumProc::Find(INode *node) 
{
	SceneEntry *e = head;
	while(e) 
    {
		if(e->node == node)
        {
			return e;
        }
		e = e->next;
	}

	return NULL;
}

void SceneEnumProc::Init(IScene *scene, TimeValue t, Interface *i )
{
    time = t;
	theScene = scene;
	count = 0;
	head = tail = NULL;
	this->i = i;
	theScene->EnumTree(this);
}
