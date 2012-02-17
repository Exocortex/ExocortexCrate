#include "Foundation.h"
#include "AlembicWriteJob.h"
#include "AlembicGetInfo.h"
#include "AlembicTimeControl.h"
#include "AlembicFileNode.h"
#include "AlembicXform.h"
#include "AlembicCamera.h"
#include "AlembicPolyMesh.h"

#include <maya/MFnPlugin.h>
#include <maya/MSceneMessage.h>

// IDs issues for this plugin are:
// 0x0011A100 - 0x0011A1FF
const MTypeId mTimeControlNodeId(0x0011A100);
const MTypeId mFileNodeId(0x0011A101);
const MTypeId mXformNodeId(0x0011A102);
const MTypeId mCameraNodeId(0x0011A103);
const MTypeId mPolyMeshNodeId(0x0011A104);

static MCallbackId deleteAllArchivesCallbackOnNewId = 0;
static MCallbackId deleteAllArchivesCallbackOnOpenId = 0;
static MCallbackId deleteAllArchivesCallbackOnExitId = 0;
static void deleteAllArchivesCallback( void* clientData )
{
   preDestructAllNodes();
   deleteAllArchives();
}

MStatus initializePlugin(MObject obj)
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
   status = plugin.registerCommand("ExocortexAlembic_getInfo",
      AlembicGetInfoCommand::creator,
      AlembicGetInfoCommand::createSyntax);

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
   return status;
}

MStatus uninitializePlugin(MObject obj)
{
   MFnPlugin plugin(obj);

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

   status = plugin.deregisterNode(mTimeControlNodeId);
   status = plugin.deregisterNode(mFileNodeId);
   status = plugin.deregisterNode(mXformNodeId);
   status = plugin.deregisterNode(mCameraNodeId);
   status = plugin.deregisterNode(mPolyMeshNodeId);

   return status;
}
