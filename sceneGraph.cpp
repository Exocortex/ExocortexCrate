#include "stdafx.h"
#include "sceneGraph.h"

#include "CommonLog.h"

#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>

static MString __PythonBool[] = { MString("False"), MString("True") };
static inline const MString &PythonBool(bool b)
{
	return __PythonBool[b?1:0];
}

static void __file_and_time_control_kill(const MString &var)
{
	MGlobal::executePythonCommand(var + " = None\n");	// deallocate this variable in Python!
}

AlembicFileAndTimeControl::~AlembicFileAndTimeControl(void)
{
	__file_and_time_control_kill(var);
}

AlembicFileAndTimeControlPtr AlembicFileAndTimeControl::createControl(const IJobStringParser& jobParams)
{
	static unsigned int numberOfControlCreated = 0;
	static const MString format("^1s = ExoAlembic._functions.IJobInfo(r\"^2s\", ^3s, ^4s, ^5s)\n");

	ESS_PROFILE_SCOPE("AlembicFileAndTimeControl::createControl");
	MString var("__alembic_file_and_time_control_tuple_");
	var += (++numberOfControlCreated);

	MString cmd;
	cmd.format(format, var, jobParams.filename.c_str(), PythonBool(jobParams.importNormals), PythonBool(jobParams.importUVs), PythonBool(jobParams.importFacesets));
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
	ESS_PROFILE_SCOPE("SceneNodeMaya::replaceData");
	if(!jobParams.attachToExisting)
		return false;

	return true;
}

bool SceneNodeMaya::addXformChild(SceneNodeAlembicPtr fileNode, SceneNodeAppPtr& newAppNode)
{
	static const MString format("ExoAlembic._functions.importXform(r\"^1s\", r\"^2s\", ^3s, ^4s, ^5s)");

	ESS_PROFILE_SCOPE("SceneNodeMaya::addXformChild");
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

bool SceneNodeMaya::addCameraChild(SceneNodeAlembicPtr fileNode, SceneNodeAppPtr& newAppNode)
{
	static const MString format("ExoAlembic._functions.importCamera(r\"^1s\", r\"^2s\", ^3s, r\"^4s\", ^5s)");

	ESS_PROFILE_SCOPE("SceneNodeMaya::addCameraChild");
	MString cmd;
	cmd.format(format, fileNode->name.c_str(), fileNode->dccIdentifier.c_str(), fileAndTime->variable(), dccIdentifier.c_str(), PythonBool(fileNode->pObjCache->isConstant));

	MString result;
	MGlobal::executePythonCommand(cmd, result);
	newAppNode->dccIdentifier = result.asChar();
	newAppNode->name = newAppNode->dccIdentifier;
	return true;
}

bool SceneNodeMaya::addPolyMeshChild(SceneNodeAlembicPtr fileNode, SceneNodeAppPtr& newAppNode)
{
	static const MString format("ExoAlembic._functions.importPolyMesh(r\"^1s\", r\"^2s\", ^3s, \"^4s\", ^5s, ^6s)");

	ESS_PROFILE_SCOPE("SceneNodeMaya::addPolyMeshChild");
	MString cmd;
	cmd.format(format,	fileNode->name.c_str(),	fileNode->dccIdentifier.c_str(),				fileAndTime->variable(),
						dccIdentifier.c_str(),	PythonBool(fileNode->pObjCache->isConstant),	PythonBool(fileNode->pObjCache->isMeshTopoDynamic));

	MString result;
	MGlobal::executePythonCommand(cmd, result);
	newAppNode->dccIdentifier = result.asChar();
	newAppNode->name = newAppNode->dccIdentifier;
	return true;
}

bool SceneNodeMaya::addPointsChild(SceneNodeAlembicPtr fileNode, SceneNodeAppPtr& newAppNode)
{
	static const MString format("ExoAlembic._functions.importPoints(r\"^1s\", r\"^2s\", ^3s, r\"^4s\", ^5s)");

	ESS_PROFILE_SCOPE("SceneNodeMaya::addPointsChild");
	MString cmd;
	cmd.format(format, fileNode->name.c_str(), fileNode->dccIdentifier.c_str(), fileAndTime->variable(), dccIdentifier.c_str(), PythonBool(fileNode->pObjCache->isConstant));

	MString result;
	MGlobal::executePythonCommand(cmd, result);
	newAppNode->dccIdentifier = result.asChar();
	newAppNode->name = newAppNode->dccIdentifier;
	return true;
}

bool SceneNodeMaya::addChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode)
{
	ESS_PROFILE_SCOPE("SceneNodeMaya::addChild");
	if(jobParams.attachToExisting)
		return false;

	newAppNode.reset(new SceneNodeMaya(fileAndTime));
	switch(newAppNode->type = fileNode->type)
	{
	case ETRANSFORM:
	case ITRANSFORM:
		return addXformChild(fileNode, newAppNode);
	case CAMERA:
		return addCameraChild(fileNode, newAppNode);
	case POLYMESH:
	case SUBD:
		return addPolyMeshChild(fileNode, newAppNode);
	case CURVES:
		break;
	case PARTICLES:
		return addPointsChild(fileNode, newAppNode);
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

