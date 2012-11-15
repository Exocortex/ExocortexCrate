#include "stdafx.h"
#include "AlembicImport.h"
#include "AlembicLicensing.h"
#include "AlembicObject.h"
#include "AlembicXform.h"
#include "AlembicCamera.h"
#include "AlembicPolyMesh.h"
#include "AlembicSubD.h"
#include "AlembicPoints.h"
#include "AlembicCurves.h"
#include "AlembicHair.h"
#include "CommonLog.h"
#include "CommonUtilities.h"
#include "CommonImport.h"

#include "sceneGraph.h"
#include <maya/MItDag.h>

AlembicImportCommand::AlembicImportCommand(void)
{
}

AlembicImportCommand::~AlembicImportCommand(void)
{
}

MSyntax AlembicImportCommand::createSyntax(void)
{
	MSyntax syntax;
		syntax.addFlag("-h", "-help");
		syntax.addFlag("-j", "-job", MSyntax::kString);
		syntax.makeFlagMultiUse("-j");
		syntax.enableQuery(false);
		syntax.enableEdit(false);
	return syntax;
}

void* AlembicImportCommand::creator(void)
{
	return new AlembicImportCommand();
}

MStatus AlembicImportCommand::importSingleJob(MString job, int jobNumber)
{
	ESS_PROFILE_SCOPE("AlembicImportCommand::importSingleJob");
	MStatus status;
	IJobStringParser jobParser;
	if (!jobParser.parse(job.asChar()))
	{
		ESS_LOG_ERROR("[ExocortexAlembic] Error in job #" << jobNumber << " job string invalid.");
		return MS::kUnknownParameter;
	}

	if(jobParser.filename.empty())
	{
		ESS_LOG_ERROR("[ExocortexAlembic] Error in job #" << jobNumber << " No filename specified.");
		return MS::kInvalidParameter;
	}

	// let's try to read this
	Abc::IArchive* archive = NULL;
	try
	{
		archive = getArchiveFromID( jobParser.filename );
	}
	catch(Alembic::Util::Exception& e)
	{
		ESS_LOG_ERROR("[ExocortexAlembic] Error reading file: "<<e.what());
		return MS::kFailure;
	}
	catch(...)
	{
		ESS_LOG_ERROR("[ExocortexAlembic] Error reading file.");
		return MS::kFailure;
	}
	addRefArchive( jobParser.filename );
	AbcArchiveCache *pArchiveCache = getArchiveCache( jobParser.filename );

	int nNumNodes = 0;
	SceneNodePtr fileRoot = buildCommonSceneGraph(pArchiveCache, &( pArchiveCache->find( "/" )->second ), nNumNodes);

	// create time control
	AlembicFileAndTimeControlPtr fileTimeCtrl = AlembicFileAndTimeControl::createControl(jobParser.filename);
	if(jobParser.attachToExisting)
	{
		MDagPath dagPath;
		{ MItDag().getPath(dagPath); }
		SceneNodePtr appRoot = buildMayaSceneGraph(dagPath, fileTimeCtrl);
		if (!ImportSceneFile(jobParser, fileRoot, appRoot))
			return MS::kFailure;
	}
	else
	{
		SceneNodePtr appRoot(new SceneNodeMaya(fileTimeCtrl));
		if (!ImportSceneFile(jobParser, fileRoot, appRoot))
			return MS::kFailure;
	}
	return status;
}

MStatus AlembicImportCommand::doIt(const MArgList & args)
{
	ESS_PROFILE_SCOPE("AlembicImportCommand::doIt");
	MStatus status;
	MArgParser argData(syntax(), args, &status);
	if (argData.isFlagSet("help"))
	{
		MGlobal::displayInfo("[ExocortexAlembic]: ExocortexAlembic_import command:");
		MGlobal::displayInfo("                    -j : import jobs (multiple flag can be used)");
		return MS::kSuccess;
	}

	const unsigned int jobCount = argData.numberOfFlagUses("job");
	if (jobCount == 0)
	{
		MGlobal::displayError("[ExocortexAlembic] No jobs specified.");
		return MS::kSuccess;
	}

	for(unsigned int i=0; i<jobCount;)	// 'i' is increment below!
	{
		MArgList jobArgList;
		argData.getFlagArgumentList("job", i, jobArgList);
		status = importSingleJob(jobArgList.asString(0), ++i);
		if (status != MS::kSuccess)
			break;
	}
	return status;
}

