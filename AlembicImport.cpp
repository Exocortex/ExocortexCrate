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
#include <maya/MItDependencyNodes.h>

/// Maya Progress Bar
void MayaProgressBar::init(int _min, int _max, int incr)
{
	ESS_PROFILE_SCOPE("MayaProgressBar::init");
	MString cmd = "ExoAlembic._functions.progressBar_init(";
	cmd += ((_max -= _min) < 1) ? 1 : _max;
	cmd += ")";

	MGlobal::executePythonCommand(cmd);
}

void MayaProgressBar::start(void)
{
	ESS_PROFILE_SCOPE("MayaProgressBar::start");
	MGlobal::executePythonCommand("ExoAlembic._functions.progressBar_start()");
}

void MayaProgressBar::stop(void)
{
	ESS_PROFILE_SCOPE("MayaProgressBar::stop");
	MGlobal::executePythonCommand("ExoAlembic._functions.progressBar_stop()");
}

void MayaProgressBar::incr(int step)
{
	ESS_PROFILE_SCOPE("MayaProgressBar::incr");
	switch(step)
	{
	case 0:
		break;
	case 20:
		MGlobal::executePythonCommand("ExoAlembic._functions.progressBar_incr()");
		break;
	default:
		{
			MString cmd = "ExoAlembic._functions.progressBar_incr(";
			cmd += step;
			cmd += ")";
			MGlobal::executePythonCommand(cmd);
		}
		break;
	}
}

bool MayaProgressBar::isCancelled(void)
{
	ESS_PROFILE_SCOPE("MayaProgressBar::isCancelled");
	int result;
	MGlobal::executePythonCommand("ExoAlembic._functions.progressBar_isCancelled()", result);
	return result > 0;
}

/// Import command
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
		syntax.addFlag("-j", "-jobArg", MSyntax::kString);
		syntax.makeFlagMultiUse("-j");
		syntax.enableQuery(false);
		syntax.enableEdit(false);
	return syntax;
}

void* AlembicImportCommand::creator(void)
{
	return new AlembicImportCommand();
}

MStatus AlembicImportCommand::importSingleJob(const MString &job, int jobNumber)
{
	ESS_PROFILE_SCOPE("AlembicImportCommand::importSingleJob");
	MStatus status;
	IJobStringParser jobParser;
	if (!jobParser.parse(job.asChar()))
	{
		ESS_LOG_ERROR("[ExocortexAlembic] Error in job #" << jobNumber << " job string invalid.");
		MPxCommand::setResult("job string invalid");
		return MS::kUnknownParameter;
	}

	if(jobParser.filename.empty())
	{
		ESS_LOG_ERROR("[ExocortexAlembic] Error in job #" << jobNumber << " No filename specified.");
		MPxCommand::setResult("No filename specified");
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
		MPxCommand::setResult("Error reading file");
		return MS::kFailure;
	}
	catch(...)
	{
		ESS_LOG_ERROR("[ExocortexAlembic] Error reading file.");
		MPxCommand::setResult("Error reading file");
		return MS::kFailure;
	}
	addRefArchive( jobParser.filename );

	MayaProgressBar pBar;
	pBar.init(0, 100000000, 1);
	pBar.start();
	//pBar.setCaption(std::string("Caching"));
	AbcArchiveCache *pArchiveCache = getArchiveCache( jobParser.filename, &pBar );
	if (pArchiveCache == 0)
	{
		ESS_LOG_WARNING("[ExocortexAlembic] Import job cancelled by user");
		pBar.stop();
		return MS::kSuccess;
	}

	int nNumNodes = 0;
	//pBar.setCaption(std::string("Scene Graph"));
	AbcObjectCache *objCache = &( pArchiveCache->find( "/" )->second );
	SceneNodeAlembicPtr fileRoot = buildAlembicSceneGraph(pArchiveCache, objCache, nNumNodes, jobParser, true, &pBar);
	if (fileRoot.get() == 0)
	{
		pArchiveCache->clear();
		ESS_LOG_WARNING("[ExocortexAlembic] Import job cancelled by user");
		pBar.stop();
		return MS::kSuccess;
	}

	// create time control
	AlembicFileAndTimeControlPtr fileTimeCtrl = AlembicFileAndTimeControl::createControl(jobParser);
	if (!fileTimeCtrl.get())
	{
		pArchiveCache->clear();
		ESS_LOG_ERROR("[ExocortexAlembic] Unable to create file node and/or time control.");
		MPxCommand::setResult("Unable to create file node and/or time control");
		return MS::kFailure;
	}

	pBar.stop();
	pBar.init(0, nNumNodes, 1);
	
	if(jobParser.attachToExisting)
	{
		//pBar.setCaption(std::string("Attach"));
		MDagPath dagPath;
		MItDag().getPath(dagPath);
		SceneNodeAppPtr appRoot = buildMayaSceneGraph(dagPath, SearchReplace::createReplacer(), fileTimeCtrl);
		if (!AttachSceneFile(fileRoot, appRoot, jobParser, &pBar))
			return MS::kFailure;
	}
	else
	{
		//pBar.setCaption(std::string("Import"));
		SceneNodeAppPtr appRoot(new SceneNodeMaya(fileTimeCtrl));
		if (!ImportSceneFile(fileRoot, appRoot, jobParser, &pBar))
			return MS::kFailure;
		AlembicPostImportPoints();
	}

	// check if an error was set!
	{
		MString result;
		status = MPxCommand::getCurrentResult(result);
		if (status == MS::kSuccess && result.length() > 0)	// therefore, there's a return value and something went wrong somewhere!
			return MS::kFailure;
	}
	return MS::kSuccess;
}

MStatus AlembicImportCommand::doIt(const MArgList & args)
{
	ESS_PROFILE_SCOPE("AlembicImportCommand::doIt");
	MStatus status;
	MArgParser argData(syntax(), args, &status);
	if (status != MS::kSuccess)
	{
		MPxCommand::setResult("unable to parse arguments");
		return status;
	}

	if (argData.isFlagSet("help"))
	{
		MGlobal::displayInfo("[ExocortexAlembic]: ExocortexAlembic_import command:");
		MGlobal::displayInfo("                    -j : import jobs (multiple flag can be used)");
		return MS::kSuccess;
	}

	const unsigned int jobCount = argData.numberOfFlagUses("jobArg");
	if (jobCount == 0)
	{
		MGlobal::displayError("[ExocortexAlembic] No jobs specified.");
		return MS::kSuccess;
	}

	// number of nodes before importing!
	int nbBeforeNodes = 0;
	for (MItDependencyNodes nodeIt; !nodeIt.isDone(); nodeIt.next(), ++nbBeforeNodes);

	// import jobs!
	for(unsigned int i=0; i<jobCount;)	// 'i' is increment below!
	{
		MArgList jobArgList;
		argData.getFlagArgumentList("jobArg", i, jobArgList);
		status = importSingleJob(jobArgList.asString(0), ++i);
		if (status != MS::kSuccess)
			break;
	}

	// get the new nodes (listed after the old ones)!
	MSelectionList afterList;MItDependencyNodes nodeIt;
	for (; nbBeforeNodes; nodeIt.next(), --nbBeforeNodes);

	for (; !nodeIt.isDone(); nodeIt.next())
		afterList.add(nodeIt.item());

	MStringArray newNodes;
	afterList.getSelectionStrings(newNodes);
	afterList.clear();

	MPxCommand::setResult(newNodes);
	return status;
}

