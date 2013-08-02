#include "stdafx.h"
#include "AlembicWriteJob.h"
#include "AlembicImport.h"
//#include "AlembicGetInfo.h"
//#include "AlembicGetNodeFromIdentifier.h"
#include "AlembicTimeControl.h"
#include "AlembicFileNode.h"
#include "AlembicXform.h"
#include "AlembicCamera.h"
#include "AlembicPolyMesh.h"
#include "AlembicSubD.h"
#include "AlembicPoints.h"
#include "AlembicCurves.h"
//#include "AlembicNurbs.h"
#include "MetaData.h"
#include "AlembicValidateNameCmd.h"
#include <maya/MFnPlugin.h>
#include <maya/MDGMessage.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MFnStringData.h>

// IDs issues for this plugin are: 
// 0x0011A100 - 0x0011A1FF
const MTypeId mTimeControlNodeId(0x0011A100);
const MTypeId mFileNodeId(0x0011A101);
const MTypeId mXformNodeId(0x0011A102);
const MTypeId mCameraNodeId(0x0011A103); 
const MTypeId mPolyMeshNodeId(0x0011A104);
const MTypeId mSubDNodeId(0x0011A105);
const MTypeId mPolyMeshDeformNodeId(0x0011A106);
const MTypeId mSubDDeformNodeId(0x0011A107);
const MTypeId mPointsNodeId(0x0011A108);
const MTypeId mCurvesNodeId(0x0011A109);
const MTypeId mCurvesDeformNodeId(0x0011A110);
const MTypeId mCurvesLocatorNodeId(0x0011A111);

static MCallbackId deleteAllArchivesCallbackOnNewId = 0;
static MCallbackId deleteAllArchivesCallbackOnOpenId = 0;
static MCallbackId deleteAllArchivesCallbackOnExitId = 0;
static void deleteAllArchivesCallback( void* clientData )
{
   preDestructAllNodes(); 
   deleteAllArchives();
}

#ifdef _MSC_VER
  #define EC_EXPORT
#else
  #define EC_EXPORT extern "C"
#endif

void removeExocortexAlembicNode(MObject &node, void *clientData)
{
	MFnDependencyNode nodeFn(node);
	MObject fname = nodeFn.attribute("fileName");
	if (!fname.isNull())
	{
		MPlug attrFileName(nodeFn.object(), fname);
		if (attrFileName.getValue(fname))
			delRefArchive(MFnStringData(fname).string());
	}

	fname = nodeFn.attribute("uv_fileName");
	if (!fname.isNull())
	{
		MPlug attrFileName(nodeFn.object(), fname);
		if (attrFileName.getValue(fname))
			delRefArchive(MFnStringData(fname).string());
	}
}

EC_EXPORT MStatus initializePlugin(MObject obj)
{
   const char * pluginVersion = "1.0";
   MFnPlugin plugin(obj, "ExocortexAlembicMaya", pluginVersion, "Any");

   MStatus status;

   // callbacks
   if (deleteAllArchivesCallbackOnNewId == 0)
      deleteAllArchivesCallbackOnNewId = MSceneMessage::addCallback( MSceneMessage::kBeforeNew, deleteAllArchivesCallback );     
   if (deleteAllArchivesCallbackOnOpenId == 0)
      deleteAllArchivesCallbackOnOpenId = MSceneMessage::addCallback( MSceneMessage::kBeforeOpen, deleteAllArchivesCallback );     
   if (deleteAllArchivesCallbackOnExitId == 0)
      deleteAllArchivesCallbackOnExitId = MSceneMessage::addCallback( MSceneMessage::kMayaExiting, deleteAllArchivesCallback );     

   // commands
   status = plugin.registerCommand("ExocortexAlembic_export",
      AlembicExportCommand::creator,
      AlembicExportCommand::createSyntax);
   status = plugin.registerCommand("ExocortexAlembic_import",
      AlembicImportCommand::creator,
      AlembicImportCommand::createSyntax);
   status = plugin.registerCommand("ExocortexAlembic_resolvePath",
      AlembicResolvePathCommand::creator,
      AlembicResolvePathCommand::createSyntax);
   status = plugin.registerCommand("ExocortexAlembic_createMetaData",
      AlembicCreateMetaDataCommand::creator,
      AlembicCreateMetaDataCommand::createSyntax);
   status = plugin.registerCommand("ExocortexAlembic_createFaceSets",
      AlembicCreateFaceSetsCommand::creator,
      AlembicCreateFaceSetsCommand::createSyntax);
   status = plugin.registerCommand("ExocortexAlembic_assignFaceSets",
      AlembicAssignFacesetCommand::creator,
      AlembicAssignFacesetCommand::createSyntax);

   status = plugin.registerCommand("ExocortexAlembic_postImportPoints",
      AlembicPostImportPointsCommand::creator,
      AlembicPostImportPointsCommand::createSyntax);
   status = plugin.registerCommand("ExocortexAlembic_profileBegin",
	  AlembicProfileBeginCommand::creator,
      AlembicProfileBeginCommand::createSyntax);
   status = plugin.registerCommand("ExocortexAlembic_profileEnd",
	  AlembicProfileEndCommand::creator,
      AlembicProfileEndCommand::createSyntax);
   status = plugin.registerCommand("ExocortexAlembic_profileStats",
	    AlembicProfileStatsCommand::creator,
      AlembicProfileStatsCommand::createSyntax);
   status = plugin.registerCommand("ExocortexAlembic_profileReset",
	    AlembicProfileResetCommand::creator,
      AlembicProfileResetCommand::createSyntax);
	status = plugin.registerCommand("ExocortexAlembic_meshToSubdiv", AlembicPolyMeshToSubdivCommand::creator, AlembicPolyMeshToSubdivCommand::createSyntax);

   // nodes
   status = plugin.registerNode("ExocortexAlembicTimeControl",
      mTimeControlNodeId,
      &AlembicTimeControlNode::creator,
      &AlembicTimeControlNode::initialize);
   status = plugin.registerNode("ExocortexAlembicFile",
      mFileNodeId,
      &AlembicFileNode::creator,
      &AlembicFileNode::initialize);
   status = plugin.registerNode("ExocortexAlembicXform",
      mXformNodeId,
      &AlembicXformNode::creator,
      &AlembicXformNode::initialize);
   status = plugin.registerNode("ExocortexAlembicCamera",
      mCameraNodeId,
      &AlembicCameraNode::creator,
      &AlembicCameraNode::initialize);
   status = plugin.registerNode("ExocortexAlembicPolyMesh",
      mPolyMeshNodeId,
      &AlembicPolyMeshNode::creator,
      &AlembicPolyMeshNode::initialize);
   status = plugin.registerNode("ExocortexAlembicSubD",
      mSubDNodeId,
      &AlembicSubDNode::creator,
      &AlembicSubDNode::initialize);
   status = plugin.registerNode("ExocortexAlembicPolyMeshDeform",
      mPolyMeshDeformNodeId,
      &AlembicPolyMeshDeformNode::creator,
      &AlembicPolyMeshDeformNode::initialize,
      MPxNode::kDeformerNode);
   status = plugin.registerNode("ExocortexAlembicSubDDeform",
      mSubDDeformNodeId,
      &AlembicSubDDeformNode::creator,
      &AlembicSubDDeformNode::initialize,
      MPxNode::kDeformerNode);
   status = plugin.registerNode("ExocortexAlembicPoints",
      mPointsNodeId,
      &AlembicPointsNode::creator,
      &AlembicPointsNode::initialize,
      MPxNode::kEmitterNode);
   status = plugin.registerNode("ExocortexAlembicCurves",
      mCurvesNodeId,
      &AlembicCurvesNode::creator,
      &AlembicCurvesNode::initialize);
   status = plugin.registerNode("ExocortexAlembicCurvesDeform",
      mCurvesDeformNodeId,
      &AlembicCurvesDeformNode::creator,
      &AlembicCurvesDeformNode::initialize,
      MPxNode::kDeformerNode);
   status = plugin.registerNode("ExocortexAlembicCurvesLocator",
      mCurvesLocatorNodeId,
      &AlembicCurvesLocatorNode::creator,
      &AlembicCurvesLocatorNode::initialize,
      MPxNode::kLocatorNode);

   MDGMessage::addNodeRemovedCallback(removeExocortexAlembicNode, "ExocortexAlembicXform", NULL, &status);
   MDGMessage::addNodeRemovedCallback(removeExocortexAlembicNode, "ExocortexAlembicCamera", NULL, &status);
   MDGMessage::addNodeRemovedCallback(removeExocortexAlembicNode, "ExocortexAlembicPolyMesh", NULL, &status);
   MDGMessage::addNodeRemovedCallback(removeExocortexAlembicNode, "ExocortexAlembicSubD", NULL, &status);
   MDGMessage::addNodeRemovedCallback(removeExocortexAlembicNode, "ExocortexAlembicPolyMeshDeform", NULL, &status);
   MDGMessage::addNodeRemovedCallback(removeExocortexAlembicNode, "ExocortexAlembicSubDDeform", NULL, &status);
   MDGMessage::addNodeRemovedCallback(removeExocortexAlembicNode, "ExocortexAlembicPoints", NULL, &status);
   MDGMessage::addNodeRemovedCallback(removeExocortexAlembicNode, "ExocortexAlembicCurves", NULL, &status);
   MDGMessage::addNodeRemovedCallback(removeExocortexAlembicNode, "ExocortexAlembicCurvesDeform", NULL, &status);

   // Load the menu!
   MString cmd = "source \"menu.mel\"; exocortexAlembicLoadMenu(\"" + plugin.name() + "\");";
   MStatus commandStatus = MGlobal::executeCommand(cmd, true, false);
   if (commandStatus != MStatus::kSuccess)
   {
	  //EC_LOG_ERROR("FAILED TO SOURCE ../scripts/menu.mel: " << commandStatus.errorString());
   }

   commandStatus = MGlobal::executePythonCommand("import ExocortexAlembic as ExoAlembic\n");
   if (commandStatus != MStatus::kSuccess)
	   MGlobal::displayError("Unable to import ExocortexAlembic");
   MGlobal::executePythonCommand("import maya.cmds as __cmds__\n");

   EC_LOG_INFO( "Exocortex Crate "<<crate_MAJOR_VERSION<<"."<<crate_MINOR_VERSION<<"."<<crate_BUILD_VERSION );
   return status;
}

EC_EXPORT MStatus uninitializePlugin(MObject obj)
{
   MFnPlugin plugin(obj);

   // Unload the menu!
   MStatus commandStatus = MGlobal::executeCommand("source \"menu.mel\"; exocortexAlembicUnloadMenu;", true, false);
   if (commandStatus != MStatus::kSuccess)
   {
	  //EC_LOG_ERROR("FAILED TO SOURCE ../scripts/menu.mel: " << commandStatus.errorString());
   }

   MStatus status;

   if (deleteAllArchivesCallbackOnNewId)
   {
      MMessage::removeCallback( deleteAllArchivesCallbackOnNewId );
      deleteAllArchivesCallbackOnNewId = 0;
   }
   if (deleteAllArchivesCallbackOnOpenId)
   {
      MMessage::removeCallback( deleteAllArchivesCallbackOnOpenId );
      deleteAllArchivesCallbackOnOpenId = 0;
   }
   if (deleteAllArchivesCallbackOnExitId)
   {
      MMessage::removeCallback( deleteAllArchivesCallbackOnExitId );
      deleteAllArchivesCallbackOnExitId = 0;
   }

   status = plugin.deregisterCommand("ExocortexAlembic_export");
   status = plugin.deregisterCommand("ExocortexAlembic_getInfo");
   status = plugin.deregisterCommand("ExocortexAlembic_resolvePath");
   status = plugin.deregisterCommand("ExocortexAlembic_createMetaData");
   status = plugin.deregisterCommand("ExocortexAlembic_createFaceSets");

   status = plugin.deregisterNode(mTimeControlNodeId);
   status = plugin.deregisterNode(mFileNodeId);
   status = plugin.deregisterNode(mXformNodeId);
   status = plugin.deregisterNode(mCameraNodeId);
   status = plugin.deregisterNode(mPolyMeshNodeId);
   status = plugin.deregisterNode(mSubDNodeId);
   status = plugin.deregisterNode(mPolyMeshDeformNodeId);
   status = plugin.deregisterNode(mSubDDeformNodeId);
   status = plugin.deregisterNode(mPointsNodeId);
   status = plugin.deregisterNode(mCurvesNodeId);
   status = plugin.deregisterNode(mCurvesDeformNodeId);
   status = plugin.deregisterNode(mCurvesLocatorNodeId);

   return status;
}
