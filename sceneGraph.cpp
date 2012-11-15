#include "stdafx.h"
#include "sceneGraph.h"

#include "CommonLog.h"

#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>

static void __file_and_time_control_kill(const MString &var)
{
	MGlobal::executePythonCommand(var + " = None\n");	// deallocate this variable in Python!
}

AlembicFileAndTimeControl::~AlembicFileAndTimeControl(void)
{
	__file_and_time_control_kill(var);
}

AlembicFileAndTimeControlPtr AlembicFileAndTimeControl::createControl(const std::string &filename)
{
	static unsigned int numberOfControlCreated = 0;
	static MString format("^1s = ExoAlembic._functions.alembicTimeAndFileNode(r\"^2s\")\n");

	MString var("__alembic_file_and_time_control_tuple_");
	var += (++numberOfControlCreated);

	MString cmd;
	cmd.format(format, var, filename.c_str());
	MStatus status = MGlobal::executePythonCommand(cmd);
	if (status != MS::kSuccess)
	{
		__file_and_time_control_kill(var);
		return AlembicFileAndTimeControlPtr();
	}
	return AlembicFileAndTimeControlPtr(new AlembicFileAndTimeControl(var));
}

bool SceneNodeMaya::replaceData(SceneNodePtr fileNode, const IJobStringParser& jobParams)
{
	return true;
}

bool SceneNodeMaya::addChild(SceneNodePtr fileNode, const IJobStringParser& jobParams, SceneNodePtr newAppNode)
{
	return true;
}

static bool visitChild(const MObject &mObj, SceneNodePtr &parent, const AlembicFileAndTimeControlPtr &alembicFileAndTimeControl)
{
	// check if it's a valid type of node first!
	SceneNodePtr exoChild(new SceneNodeMaya(alembicFileAndTimeControl));
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
	for (unsigned int i = 0; i < dagNode.childCount(); ++i)
		visitChild(dagNode.child(i), exoChild, alembicFileAndTimeControl);
	return true;
}

SceneNodePtr buildMayaSceneGraph(const MDagPath &dagPath, const AlembicFileAndTimeControlPtr alembicFileAndTimeControl)
{
	ESS_PROFILE_SCOPE("buildMayaSceneGraph");
	SceneNodePtr exoRoot(new SceneNodeMaya(alembicFileAndTimeControl));
	exoRoot->type = SceneNode::SCENE_ROOT;
	exoRoot->dccIdentifier = dagPath.fullPathName().asChar();
	exoRoot->name = dagPath.partialPathName().asChar();
	for (unsigned int i = 0; i < dagPath.childCount(); ++i)
		visitChild(dagPath.child(i), exoRoot, alembicFileAndTimeControl);
	return exoRoot;
}

