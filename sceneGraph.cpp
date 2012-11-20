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
	static const MString format("^1s = ExoAlembic._functions.alembicTimeAndFileNode(r\"^2s\")\n");

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

bool SceneNodeMaya::replaceData(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAlembicPtr& nextFileNode)
{
	if(!jobParams.attachToExisting)
		return false;


	return true;
}

static MString __PythonBool[] = { MString("False"), MString("True") };
static inline const MString &PythonBool(bool b)
{
	return __PythonBool[b?1:0];
}

bool SceneNodeMaya::addXformChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode)
{
	static const MString format("ExoAlembic._functions.importXform(r\"^1s\", r\"^2s\", ^3s, ^4s, ^5s)");

	MString parent;
	switch(type)
	{
	case ETRANSFORM:
	case ITRANSFORM:
		parent.format("\"^1s\"", dccIdentifier.c_str());
		break;
	default:
		parent = "None";
		break;
	}

	MString cmd;
	cmd.format(format, fileNode->name.c_str(), fileNode->dccIdentifier.c_str(), fileAndTime->variable(), parent, PythonBool(fileNode->pObjCache->isConstant));

	MGlobal::executePythonCommand(cmd, parent);
	newAppNode->dccIdentifier = parent.asChar();
	newAppNode->name = newAppNode->dccIdentifier;
	return true;
}

bool SceneNodeMaya::addPolyMeshChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode)
{
	static const MString extraParam("^1s, ^2s, ^3s, ^4s, ^5s, ^6s");
	static const MString format("ExoAlembic._functions.importPolyMesh(r\"^1s\", r\"^2s\", ^3s, ^4s)");

	MString parent;
	switch(type)
	{
	case ETRANSFORM:
	case ITRANSFORM:
		parent.format("\"^1s\"", dccIdentifier.c_str());
		break;
	default:
		parent = "None";
		break;
	}

	MString eParam;
	eParam.format
	(
		extraParam,	parent,									PythonBool(fileNode->pObjCache->isConstant),	PythonBool(fileNode->pObjCache->isMeshTopoDynamic),
					PythonBool(jobParams.importFacesets),	PythonBool(jobParams.importNormals),			PythonBool(jobParams.importUVs)
	);

	MString cmd;
	cmd.format(format, fileNode->name.c_str(), fileNode->dccIdentifier.c_str(), fileAndTime->variable(), eParam);

	MGlobal::executePythonCommand(cmd, parent);
	newAppNode->dccIdentifier = parent.asChar();
	newAppNode->name = newAppNode->dccIdentifier;
	return true;
}

bool SceneNodeMaya::addChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode)
{
	if(jobParams.attachToExisting)
		return false;

	newAppNode.reset(new SceneNodeMaya(fileAndTime));
	switch(newAppNode->type = fileNode->type)
	{
	case ETRANSFORM:
	case ITRANSFORM:
		return addXformChild(fileNode, jobParams, newAppNode);
	case CAMERA:
		break;
	case POLYMESH:
		return addPolyMeshChild(fileNode, jobParams, newAppNode);
	case SUBD:
		break;
	case CURVES:
		break;
	case PARTICLES:
		break;
	case HAIR:
		break;
	//case SURFACE:	// handle as default for now
		//break;
	//case LIGHT:	// handle as default for now
		//break;
	default:
		break;
	}
	return true;
}

void SceneNodeMaya::print(void)
{
	ESS_LOG_WARNING("Maya Scene Node: " << dccIdentifier);
}

static bool visitChild(const MObject &mObj, SceneNodeAppPtr &parent, const AlembicFileAndTimeControlPtr &alembicFileAndTimeControl)
{
	// check if it's a valid type of node first!
	SceneNodeAppPtr exoChild(new SceneNodeMaya(alembicFileAndTimeControl));
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

SceneNodeAppPtr buildMayaSceneGraph(const MDagPath &dagPath, const AlembicFileAndTimeControlPtr alembicFileAndTimeControl)
{
	ESS_PROFILE_SCOPE("buildMayaSceneGraph");
	SceneNodeAppPtr exoRoot(new SceneNodeMaya(alembicFileAndTimeControl));
	exoRoot->type = SceneNode::SCENE_ROOT;
	exoRoot->dccIdentifier = dagPath.fullPathName().asChar();
	exoRoot->name = dagPath.partialPathName().asChar();
	for (unsigned int i = 0; i < dagPath.childCount(); ++i)
		visitChild(dagPath.child(i), exoRoot, alembicFileAndTimeControl);
	return exoRoot;
}

