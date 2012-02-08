#include "Foundation.h"
#include "AlembicWriteJob.h"
#include <maya/MFnPlugin.h>

//const MTypeId AlembicNode::mMayaNodeId(0x00082697);

MStatus initializePlugin(MObject obj)
{
    const char * pluginVersion = "1.0";
    MFnPlugin plugin(obj, "ExocortexAlembicMaya", pluginVersion, "Any");

    MStatus status;
    status = plugin.registerCommand("exocortexalembic_export",
                                AlembicExportCommand::creator,
                                AlembicExportCommand::createSyntax);

    //status = plugin.registerNode("AlembicNode",
    //                            AlembicNode::mMayaNodeId,
    //                            &AlembicNode::creator,
    //                            &AlembicNode::initialize);
    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);

    MStatus status;

    status = plugin.deregisterCommand("exocortexalembic_export");
    //status = plugin.deregisterNode(AlembicNode::mMayaNodeId);

    return status;
}
