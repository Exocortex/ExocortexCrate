#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MObject.h>
#include <maya/MGlobal.h>

//const MTypeId AlembicNode::mMayaNodeId(0x00082697);

MStatus initializePlugin(MObject obj)
{
    const char * pluginVersion = "1.0";
    MFnPlugin plugin(obj, "ExocortexAlembicMaya", pluginVersion, "Any");

    MStatus status;
    //status = plugin.registerCommand("alembic_import",
    //                            AbcImport::creator,
    //                            AbcImport::createSyntax);

    //status = plugin.registerNode("AlembicNode",
    //                            AlembicNode::mMayaNodeId,
    //                            &AlembicNode::creator,
    //                            &AlembicNode::initialize);

    MString info = "Hello World.";
    MGlobal::displayInfo(info);
    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);

    MStatus status;

    //status = plugin.deregisterCommand("alembic_import");
    //status = plugin.deregisterNode(AlembicNode::mMayaNodeId);

    return status;
}
