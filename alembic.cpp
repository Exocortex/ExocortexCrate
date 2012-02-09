#include "Foundation.h"
#include "AlembicWriteJob.h"
#include "AlembicXform.h"

#include <maya/MFnPlugin.h>

const MTypeId mTimeControlNodeId(0x01084680);
const MTypeId mXformNodeId(0x01084681);

MStatus initializePlugin(MObject obj)
{
   const char * pluginVersion = "1.0";
   MFnPlugin plugin(obj, "ExocortexAlembicMaya", pluginVersion, "Any");

   MStatus status;

   // commands
   status = plugin.registerCommand("exocortexalembic_export",
      AlembicExportCommand::creator,
      AlembicExportCommand::createSyntax);

   // nodes
   /*
   status = plugin.registerNode("ExocortexAlembicTimeControl",
      mTimeControlNodeId,
      &AlembicTimeControlNode::creator,
      &AlembicTimeControlNode::initialize);
   */
   status = plugin.registerNode("ExocortexAlembicXform",
      mXformNodeId,
      &AlembicXformNode::creator,
      &AlembicXformNode::initialize);
   return status;
}

MStatus uninitializePlugin(MObject obj)
{
   MFnPlugin plugin(obj);

   MStatus status;

   status = plugin.deregisterCommand("exocortexalembic_export");

   //status = plugin.deregisterNode(mTimeControlNodeId);
   status = plugin.deregisterNode(mXformNodeId);

   return status;
}
