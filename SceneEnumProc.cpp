#include "alembic.h"
#include "SceneEnumProc.h"
#include <sceneapi.h>
#include <object.h>
#include <triobj.h>
#include "SceneEntry.h"
#include "MeshMtlList.h"

SceneEnumProc::SceneEnumProc(IScene *scene, TimeValue t, Interface *i, MeshMtlList *ml) 
{
	time = t;
	theScene = scene;
	count = 0;
	head = tail = NULL;
	this->i = i;
	mtlList = ml;
	theScene->EnumTree(this);
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
	Object *obj = node->EvalWorldState(time).obj;
	if (obj->CanConvertToType(triObjectClassID)) 
    {
		Append(node, obj, OBTYPE_MESH);
		mtlList->AddMtl(node->GetMtl());
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
				case LIGHT_CLASS_ID:  Append(node, obj, OBTYPE_LTARGET); break;
				case CAMERA_CLASS_ID: Append(node, obj, OBTYPE_CTARGET); break;
			}
		}
		return TREE_CONTINUE;
	}

	switch (obj->SuperClassID()) 
    { 
		case HELPER_CLASS_ID:
			if ( obj->ClassID()==Class_ID(DUMMY_CLASS_ID,0)) 
            {
				Append(node, obj, OBTYPE_DUMMY);
            }
			break;
		case LIGHT_CLASS_ID: 
            {
                /* LIGHT */
                break;
            }
		case CAMERA_CLASS_ID:
			if (obj->ClassID()==Class_ID(LOOKAT_CAM_CLASS_ID,0))
            {
				Append(node, obj, OBTYPE_CAMERA);
            }
			break;
	}

	return TREE_CONTINUE;
}


void SceneEnumProc::Append(INode *node, Object *obj, int type) 
{
	SceneEntry *entry = new SceneEntry(node, obj, type);

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