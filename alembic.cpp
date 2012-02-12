#include "Foundation.h"
#include "AlembicWriteJob.h"
#include "AlembicGetInfo.h"
#include "AlembicTimeControl.h"
#include "AlembicFileNode.h"
#include "AlembicXform.h"
#include "AlembicCamera.h"

#include <maya/MFnPlugin.h>

// IDs issues for this plugin are:
// 0x0011A100 - 0x0011A1FF
const MTypeId mTimeControlNodeId(0x0011A100);
const MTypeId mFileNodeId(0x0011A101);
const MTypeId mXformNodeId(0x0011A102);
const MTypeId mCameraNodeId(0x0011A103);

MStatus initializePlugin(MObject obj)
{
   const char * pluginVersion = "1.0";
   MFnPlugin plugin(obj, "ExocortexAlembicMaya", pluginVersion, "Any");

   MStatus status;

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
   return status;
}

MStatus uninitializePlugin(MObject obj)
{
   MFnPlugin plugin(obj);

   MStatus status;

   status = plugin.deregisterCommand("ExocortexAlembic_export");
   status = plugin.deregisterCommand("ExocortexAlembic_getInfo");

   status = plugin.deregisterNode(mTimeControlNodeId);
   status = plugin.deregisterNode(mFileNodeId);
   status = plugin.deregisterNode(mXformNodeId);
   status = plugin.deregisterNode(mCameraNodeId);

   return status;
}
