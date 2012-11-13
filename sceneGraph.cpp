#include "stdafx.h"
#include "sceneGraph.h"

#include "CommonLog.h"

#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>

static bool visitChild(const MObject &mObj, SceneNodePtr &parent)
{
	// check if it's a valid type of node first!
	SceneNodePtr exoChild(new SceneNode());
	switch(mObj.apiType())
	{
	case MFn::kCamera:
		exoChild->type = SceneNode::CAMERA;
		break;
	case MFn::kMesh:
		exoChild->type = SceneNode::POLYMESH;
		break;
	case MFn::kSubdiv:
		exoChild->type = SceneNode::SUBD;
		break;
	case MFn::kNurbsCurve:
		exoChild->type = SceneNode::CURVES;
		break;
	case MFn::kParticle:
		exoChild->type = SceneNode::PARTICLES;
		break;
	case MFn::kPfxHair:
		exoChild->type = SceneNode::HAIR;
		break;
	case MFn::kTransform:
		exoChild->type = SceneNode::ETRANSFORM;
		break;
	default:
		return false;
	}

	parent->children.push_back(exoChild);
	exoChild->parent = parent.get();

	MFnDagNode dagNode(mObj);
	exoChild->dccIdentifier = dagNode.fullPathName().asChar();
	exoChild->name = dagNode.partialPathName().asChar();
	for (int i = 0; i < dagNode.childCount(); ++i)
		visitChild(dagNode.child(i), exoChild);
	return true;
}

SceneNodePtr buildCommonSceneGraph(const MDagPath &dagPath)
{
	SceneNodePtr exoRoot(new SceneNode());
	exoRoot->type = SceneNode::SCENE_ROOT;
	exoRoot->dccIdentifier = dagPath.fullPathName().asChar();
	exoRoot->name = dagPath.partialPathName().asChar();
	for (int i = 0; i < dagPath.childCount(); ++i)
		visitChild(dagPath.child(i), exoRoot);
	return exoRoot;
}

