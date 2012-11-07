#include "stdafx.h"
#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicMeshUtilities.h"
#include "AlembicSplineUtilities.h"
#include "AlembicXformUtilities.h"
#include "AlembicXformController.h"
#include "AlembicCameraUtilities.h"
#include "AlembicParticles.h"
#include "SceneEnumProc.h"
#include "AlembicDefinitions.h"
#include "AlembicWriteJob.h"
#include "AlembicRecursiveImporter.h"
#include "Utility.h"
#include "CommonProfiler.h"

// Dummy function for progress bar
DWORD WINAPI DummyProgressFunction(LPVOID arg)
{
	return 0;
}

//MarshallH: ifnpub.h only has defines for up to FN_11
#define FN_12(_fid, _rtype, _f, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9, _p10, _p11, _p12)	\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(		\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3]),		\
					   FP_FIELD(_p5, p->params[4]),		\
					   FP_FIELD(_p6, p->params[5]),		\
					   FP_FIELD(_p7, p->params[6]),		\
					   FP_FIELD(_p8, p->params[7]),		\
					   FP_FIELD(_p9, p->params[8]),		\
					   FP_FIELD(_p10, p->params[9]),		\
					   FP_FIELD(_p11, p->params[10]), \
					   FP_FIELD(_p12, p->params[11]))));	\
		break;	

#define FN_13(_fid, _rtype, _f, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9, _p10, _p11, _p12, _p13)	\
	case _fid:											\
		result.LoadPtr(_rtype,	_rtype##_RSLT(		\
					_f(FP_FIELD(_p1, p->params[0]),		\
					   FP_FIELD(_p2, p->params[1]),		\
					   FP_FIELD(_p3, p->params[2]),		\
					   FP_FIELD(_p4, p->params[3]),		\
					   FP_FIELD(_p5, p->params[4]),		\
					   FP_FIELD(_p6, p->params[5]),		\
					   FP_FIELD(_p7, p->params[6]),		\
					   FP_FIELD(_p8, p->params[7]),		\
					   FP_FIELD(_p9, p->params[8]),		\
					   FP_FIELD(_p10, p->params[9]),		\
					   FP_FIELD(_p11, p->params[10]), \
					   FP_FIELD(_p12, p->params[11]), \
					   FP_FIELD(_p13, p->params[12]))));	\
		break;	



class ExocortexAlembicStaticInterface : public FPInterfaceDesc
{
public:

	static const Interface_ID id;

	enum FunctionIDs
	{
		exocortexAlembicImport,
		exocortexAlembicExport,
        exocortexAlembicInit,
		exocortexGetBinVersion,
		exocortexMemoryDiagnostics,
		exocortexGetLicenseStatus,
		exocortexAlembicImportJobs,
		exocortexAlembicExportJobs,
		exocortexProfileStats
	};

	ExocortexAlembicStaticInterface()
		: FPInterfaceDesc(id, _M("ExocortexAlembic"), 0, NULL, FP_CORE, p_end)
	{

		AppendFunction(
			exocortexAlembicImport,	//* function ID * /
			_M("import"),           //* internal name * /
			0,                      //* function name string resource name * / 
			TYPE_INT,               //* Return type * /
			0,                      //* Flags  * /
			6,                      //* Number  of arguments * /
			_M("path"),     //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_FILENAME,      //* arg type * /
			_M("importNormals"),//* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("importUVs"),    //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("importMaterialIds"), //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("attachToExisting"), //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("visibilityOption"), //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_INT,           //* arg type * /
			p_end); 

		AppendFunction(
			exocortexAlembicExport,	//* function ID * /
			_M("export"),           //* internal name * /
			0,                      //* function name string resource name * / 
			TYPE_INT,               //* Return type * /
			0,                      //* Flags  * /
			13,                     //* Number  of arguments * /
			_M("path"),     //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_FILENAME,      //* arg type * /
			_M("frameIn"),      //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_INT,           //* arg type * /
			_M("frameOut"),     //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_INT,           //* arg type * /
			_M("frameSteps"),   //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_INT,           //* arg type * /
			_M("frameSubSteps"),//* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_INT,           //* arg type * /
			_M("topologyType"), //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_INT,           //* arg type * /
			_M("exportUV"),     //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("exportMaterialIds"), //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("exportEnvelopeBindPose"), //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("exportDynamicTopology"), //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("exportSelected"), //* argument internal name * /
			0,                    //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("flattenHierarchy"), //* argument internal name * /
			0,                    //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("exportAsSingleMesh"), //* argument internal name * /
			0,                    //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			p_end); 	 
       
        AppendFunction(
			exocortexAlembicInit,	//* function ID * /
			_M("init"),             //* internal name * /
            0,                      //* function name string resource name * / 
			TYPE_INT,               //* Return type * /
			0,                      //* Flags  * /
			0,                     //* Number  of arguments * /
            p_end);           
  
		AppendFunction(
			exocortexGetBinVersion,	//* function ID * /
			_M("getBinVersion"),           //* internal name * /
			0,                      //* function name string resource name * / 
			TYPE_INT,               //* Return type * /
			0,                      //* Flags  * /
			0,                      //* Number  of arguments * /
			p_end); 

		AppendFunction(
			exocortexMemoryDiagnostics,	//* function ID * /
			_M("memoryDiagnostics"),           //* internal name * /
			0,                      //* function name string resource name * / 
			TYPE_INT,               //* Return type * /
			0,                      //* Flags  * /
			0,                      //* Number  of arguments * /
			p_end); 

		AppendFunction(
			exocortexGetLicenseStatus,	//* function ID * /
			_M("getLicenseStatus"),           //* internal name * /
			0,                      //* function name string resource name * / 
			TYPE_INT,               //* Return type * /
			0,                      //* Flags  * /
			0,                      //* Number  of arguments * /
			p_end); 

		AppendFunction(
			exocortexAlembicImportJobs,	//* function ID * /
			_M("createImportJob"),           //* internal name * /
			0,                      //* function name string resource name * / 
			TYPE_INT,               //* Return type * /
			0,                      //* Flags  * /
			1,                      //* Number  of arguments * /
			_M("path"),     //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_STRING,      //* arg type * /
			p_end); 

		AppendFunction(
			exocortexAlembicExportJobs,	//* function ID * /
			_M("createExportJobs"),           //* internal name * /
			0,                      //* function name string resource name * / 
			TYPE_INT,               //* Return type * /
			0,                      //* Flags  * /
			1,                      //* Number  of arguments * /
			_M("path"),     //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_STRING,      //* arg type * /
			p_end); 

		AppendFunction(
			exocortexProfileStats,	//* function ID * /
			_M("profileStats"),           //* internal name * /
			0,                      //* function name string resource name * / 
			TYPE_INT,               //* Return type * /
			0,                      //* Flags  * /
			0,                      //* Number  of arguments * /
			p_end); 
  }

	static int ExocortexAlembicImport(
		CONST_2013 MCHAR * strPath, 
		BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds,
		BOOL bAttachToExisting,
		int iVisOption);

	static int ExocortexAlembicExport(
		CONST_2013 MCHAR * strPath,
		int iFrameIn, int iFrameOut, int iFrameSteps, int iFrameSubSteps,
		int iType,
		BOOL bExportUV, BOOL bExportMaterialIds, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology, 
		BOOL bExportSelected, BOOL bFlattenHierarchy, BOOL bExportAsSingleMesh );

    static int ExocortexAlembicInit();
     	
	static int ExocortexGetBinVersion();

	static int ExocortexProfileStats();
	
	static int ExocortexMemoryDiagnostics();

	static int ExocortexGetLicenseStatus();

	static int ExocortexAlembicImportJobs(CONST_2013 MCHAR* jobString);

	static int ExocortexAlembicExportJobs(CONST_2013 MCHAR* jobString);

	BEGIN_FUNCTION_MAP
		FN_6(exocortexAlembicImport, TYPE_INT, ExocortexAlembicImport, TYPE_FILENAME, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_INT)
		FN_13(exocortexAlembicExport, TYPE_INT, ExocortexAlembicExport, TYPE_FILENAME, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL)
        FN_0(exocortexAlembicInit, TYPE_INT, ExocortexAlembicInit);
		FN_0(exocortexGetBinVersion, TYPE_INT, ExocortexGetBinVersion)
		FN_0(exocortexMemoryDiagnostics, TYPE_INT, ExocortexMemoryDiagnostics)
		FN_0(exocortexGetLicenseStatus, TYPE_INT, ExocortexGetLicenseStatus)
		FN_1(exocortexAlembicImportJobs, TYPE_INT, ExocortexAlembicImportJobs, TYPE_STRING)
		FN_1(exocortexAlembicExportJobs, TYPE_INT, ExocortexAlembicExportJobs, TYPE_STRING)
		FN_0(exocortexProfileStats, TYPE_INT, ExocortexProfileStats)
	END_FUNCTION_MAP
};

const Interface_ID ExocortexAlembicStaticInterface::id = Interface_ID(0x4bd4297f, 0x4e8403d7);
static ExocortexAlembicStaticInterface exocortexAlembic;

void AlembicImport_TimeControl( alembic_importoptions &options ) {

	// check if an Alembic Time Control already exists in the scene.

	BOOL alreadyExists = ExecuteMAXScriptScript( _T( "select $Alembic_Time_Control" ), TRUE );
	if( alreadyExists != 0 ) {
		if( GetCOREInterface()->GetSelNodeCount() > 0 ) {
			INode *pSelectedNode = GetCOREInterface()->GetSelNode( 0 );
			HelperObject *pSelectedHelper = static_cast<HelperObject*>( pSelectedNode->GetObjectRef() );
			options.pTimeControl = pSelectedHelper;
			return;
		}
	}

	// Create the xform modifier
	HelperObject *pHelper = static_cast<HelperObject*>
		(GetCOREInterface()->CreateInstance(HELPER_CLASS_ID, ALEMBIC_TIME_CONTROL_HELPER_CLASSID));

	TimeValue zero( 0 );

	// Set the alembic id
	pHelper->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pHelper, 0, "current" ), zero, 0.0f );
	pHelper->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pHelper, 0, "offset" ), zero, 0.0f );
	pHelper->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pHelper, 0, "factor" ), zero, 1.0f );

	// Create the object node
	INode *node = GET_MAX_INTERFACE()->CreateObjectNode(pHelper, pHelper->GetObjectName() );

	// Add the new inode to our current scene list
	SceneEntry *pEntry = options.sceneEnumProc.Append(node, pHelper, OBTYPE_CURVES, &std::string( EC_MCHAR_to_UTF8( node->GetName() ) ) ); 
	options.currentSceneList.Append(pEntry);

	GET_MAX_INTERFACE()->SelectNode( node );

	std::string nodeName = EC_MCHAR_to_UTF8( node->GetName() );
	char szBuffer[10000];	
	sprintf_s( szBuffer, 10000,
		"$'%s'.current.controller = float_expression()\n"
		"$'%s'.current.controller.setExpression \"S\"\n"
		"$'%s'.offset.controller = bezier_float()\n"
		"$'%s'.factor.controller = bezier_float()\n"
		, nodeName.c_str(), nodeName.c_str(), nodeName.c_str(), nodeName.c_str() );
	ExecuteMAXScriptScript( EC_UTF8_to_TCHAR( szBuffer ) );

	options.pTimeControl = pHelper;
}

void AlembicImport_ConnectTimeControl( const char* szControllerName, alembic_importoptions &options ) 
{
	std::string objectNameName = EC_MCHAR_to_UTF8( options.pTimeControl->GetObjectName() );
	char szBuffer[10000];	
	sprintf_s( szBuffer, 10000, 
		"%s.controller = float_expression()\n"
		"%s.controller.AddScalarTarget \"current\" $'%s'.current.controller\n"
		"%s.controller.AddScalarTarget \"offset\" $'%s'.offset.controller\n"
		"%s.controller.AddScalarTarget \"factor\" $'%s'.factor.controller\n"
		"%s.controller.setExpression \"current * factor + offset\"\n",
		szControllerName,
		szControllerName, objectNameName.c_str(),
		szControllerName, objectNameName.c_str(),
		szControllerName, objectNameName.c_str(),
		szControllerName );

	ExecuteMAXScriptScript( EC_UTF8_to_TCHAR( szBuffer ) );
}

int ExocortexAlembicStaticInterface::ExocortexGetBinVersion()
{
	return PLUGIN_MAJOR_VERSION * 1000 + PLUGIN_MINOR_VERSION;
}


int ExocortexAlembicStaticInterface::ExocortexProfileStats()
{
	ESS_PROFILE_REPORT();
	return 0;
}


int ExocortexAlembicStaticInterface::ExocortexMemoryDiagnostics()
{
	Exocortex::essLogWarning( "Exocortex Memory Diagnostics -----------------------------------------------------" );
#ifdef _DEBUG
	//Exocortex::essLogActiveAllocations();
#endif
	return 0;
}

int ExocortexAlembicStaticInterface::ExocortexGetLicenseStatus()
{
	if( HasAlembicWriterLicense() ) {
		return 1;
	}
	if( HasAlembicReaderLicense() ) {
		return 2;
	}
	return 0;
}


bool parseBool(std::string value){
	//std::istringstream(valuePair[1]) >> bExportSelected;

	if( value.find("true") != std::string::npos || value.find("1") != std::string::npos ){
		return true;
	}
	else{
		return false;
	}
}

int ExocortexAlembicStaticInterface_ExocortexAlembicImportJobs( CONST_2013 MCHAR* jobString );
int ExocortexAlembicStaticInterface::ExocortexAlembicImportJobs( CONST_2013 MCHAR* jobString )
{
	ESS_STRUCTURED_EXCEPTION_REPORTING_START

		return ExocortexAlembicStaticInterface_ExocortexAlembicImportJobs( jobString );

	ESS_STRUCTURED_EXCEPTION_REPORTING_END

	return alembic_failure;
}

int ExocortexAlembicStaticInterface_ExocortexAlembicImportJobs( CONST_2013 MCHAR* jobString )
{
	try {

      //CONST_2013 MCHAR* strPath, 
      //BOOL bImportNormals, 
      //BOOL bImportUVs, 
      //BOOL bImportMaterialIds, 
      //BOOL bAttachToExisting, 
      //int iVisOption

		//ESS_LOG_INFO( "ExocortexAlembicImport( strPath=" << strPath <<
		//	", bImportNormals=" << bImportNormals << ", bImportUVs=" << bImportUVs <<
		//	", bImportMaterialIds=" << bImportMaterialIds << ", bAttachToExisting=" << bAttachToExisting <<
		//	", iVisOption=" << iVisOption << " )" );

      ESS_LOG_WARNING( "Processing import job: "<<jobString);

		alembic_importoptions options;

		std::string file;// = EC_MCHAR_to_UTF8( strPath );
	
      std::vector<std::string> nodesToImport;

      bool bIncludeChildren = false;

		std::vector<std::string> tokens;
		boost::split(tokens, jobString, boost::is_any_of(";"));
		for(int j=0; j<tokens.size(); j++){

			std::vector<std::string> valuePair;
			boost::split(valuePair, tokens[j], boost::is_any_of("="));
			if(valuePair.size() != 2){
				ESS_LOG_WARNING("Skipping invalid token: "<<tokens[j]);
				continue;
			}

			if(boost::iequals(valuePair[0], "filename")){
				file = valuePair[1];
			}
			else if(boost::iequals(valuePair[0], "normals")){
				options.importNormals = parseBool(valuePair[1]);
			}
			else if(boost::iequals(valuePair[0], "uvs")){
				options.importUVs = parseBool(valuePair[1]);
			}
			else if(boost::iequals(valuePair[0], "materialIds")){
            options.importMaterialIds = parseBool(valuePair[1]);
			}
         else if(boost::iequals(valuePair[0], "attachToExisting")){
            options.attachToExisting = parseBool(valuePair[1]);
			}
		  else if(boost::iequals(valuePair[0], "failOnUnsupported")){
            options.failOnUnsupported = parseBool(valuePair[1]);
			}
         else if(boost::iequals(valuePair[0], "filters")){  
		      boost::split(nodesToImport, valuePair[1], boost::is_any_of(","));
			}
		   else if(boost::iequals(valuePair[0], "includeChildren")){
            bIncludeChildren = parseBool(valuePair[1]);
			}
			else
			{
				ESS_LOG_INFO("Skipping invalid token: "<<tokens[j]);
				continue;
			}
		}

	// If no filename, then return an error code
		if(file.size() == 0) {
			ESS_LOG_ERROR( "No filename specified." );
			return alembic_invalidarg;
		}

		if( ! fs::exists( file.c_str() ) ) {
			ESS_LOG_ERROR( "Can't find Alembic file.  Path: " << file );
			return alembic_invalidarg;
		}

		//std::string file(szPath);

		// Try opening up the archive
		Abc::IArchive *pArchive = getArchiveFromID(file);
		if (!pArchive) {
			ESS_LOG_ERROR( "Unable to open Alembic file: " << file  );
			return alembic_failure;
		}

        // Since the archive is valid, add a reference to it
        addRefArchive(file);
   AbcArchiveCache *pArchiveCache = getArchiveCache( file );

		// Get a list of the current objects in the scene
		MAXInterface *i = GET_MAX_INTERFACE();

		options.sceneEnumProc.Init(i->GetScene(), i->GetTime(), i);
		options.currentSceneList.FillList(options.sceneEnumProc);

		ESS_LOG_INFO( "AlembicImport_TimeControl." );
				
		AlembicImport_TimeControl( options );

      AbcG::IObject root = pArchive->getTop();

      

      //nodesToImport.push_back(std::string("null1"));

      std::map<std::string, bool> nodeFullPaths;
		int totalAlembicItems = prescanAlembicHierarchy( pArchiveCache, &( pArchiveCache->find( "/" )->second ), nodesToImport, nodeFullPaths, bIncludeChildren);

		char szBuffer[1000];
		sprintf_s( szBuffer, 1000, "Importing %i Alembic Streams", totalAlembicItems );
		i->ProgressStart( EC_UTF8_to_TCHAR( szBuffer ), TRUE, DummyProgressFunction, NULL);

		int progressUpdateInterval = 0;
		int lastUpdateProcess = 0;

		progressUpdate progress(totalAlembicItems);

      if( importAlembicScene(pArchiveCache, &( pArchiveCache->find( "/" )->second ), options, file, progress, nodeFullPaths) != 0 ){
         return alembic_failure;
      }

		i->ProgressEnd();


		delRefArchive(file);

	} catch( boost::exception& e ) { 								
		ESS_LOG_ERROR(__FILE__ << "(line " << __LINE__ << "). A boost::exception occurred: " << boost::diagnostic_information(e) );					
		Exocortex::essLogStackTrace(); 
		Exocortex::essSendErrorReport( "boost::exception" ); 	
		GET_MAX_INTERFACE()->ProgressEnd(); 
		return alembic_failure; 
	} catch ( std::exception& e ) { 										
		ESS_LOG_ERROR(__FILE__ << "(line " << __LINE__ << "). An std::exception occurred: " << e.what()); 	 				
		Exocortex::essLogStackTrace(); 	
		char szBuffer[1024*4]; 	
		sprintf_s( szBuffer, 1024*4, "std::exception: %s", e.what() ); 
		Exocortex::essSendErrorReport( szBuffer ); 
		GET_MAX_INTERFACE()->ProgressEnd(); 
		return alembic_failure; 
	} catch( std::string& str ) { 									
		ESS_LOG_ERROR(__FILE__ << "(line " << __LINE__ << "). A string exception was thrown: " << str);					
		Exocortex::essLogStackTrace(); 										
		char szBuffer[1024*4]; 
		sprintf_s( szBuffer, 1024*4, "C++ String Exception: %s", str.c_str() ); 
		Exocortex::essSendErrorReport( szBuffer ); 
		GET_MAX_INTERFACE()->ProgressEnd(); 
		return alembic_failure; 
	} catch (...) {														
		ESS_LOG_ERROR(__FILE__ << "(line " << __LINE__ << "). An unknown error occurred"); 						
		Exocortex::essLogStackTrace(); 
		Exocortex::essSendErrorReport( "unknown C++ exception" ); 
		GET_MAX_INTERFACE()->ProgressEnd(); 
		return alembic_failure; 
	}

	return alembic_success;
}


int ExocortexAlembicStaticInterface_ExocortexAlembicImport( CONST_2013 MCHAR* strPath, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds, BOOL bAttachToExisting, int iVisOption);

int ExocortexAlembicStaticInterface::ExocortexAlembicImport( CONST_2013 MCHAR* strPath, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds, BOOL bAttachToExisting, int iVisOption)
{
	ESS_STRUCTURED_EXCEPTION_REPORTING_START

		return ExocortexAlembicStaticInterface_ExocortexAlembicImport( strPath, bImportNormals, bImportUVs, bImportMaterialIds, bAttachToExisting, iVisOption);

	ESS_STRUCTURED_EXCEPTION_REPORTING_END

	return alembic_failure;
}

int ExocortexAlembicStaticInterface_ExocortexAlembicImport( CONST_2013 MCHAR* strPath, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds, BOOL bAttachToExisting, int iVisOption)
{
	std::stringstream jobStream;
	jobStream<<"filename="<<strPath<<";normals="<<bImportNormals<<";uvs="<<bImportUVs<<";materialIds="<<bImportMaterialIds<<";attachToExisting="<<bAttachToExisting;
	
	TSTR tStr = EC_UTF8_to_TSTR( jobStream.str().c_str() );
	return ExocortexAlembicStaticInterface_ExocortexAlembicImportJobs( tStr.data() );

	return 0;
}


void addNodeChildren(INode* pNode, ObjectList& allSceneObjects, TimeValue time, std::string path)
{
	for(int i=0; i<pNode->NumberOfChildren(); i++){

		INode* pChildNode = pNode->GetChildNode(i);

		path += "/";
		path += EC_MCHAR_to_UTF8( pChildNode->GetName() );

		SceneEntry sEntry = createSceneEntry(pChildNode, time, &path);
		allSceneObjects.Append(&sEntry);//The append will make a copy sEntry, so this is safe

		addNodeChildren(pChildNode, allSceneObjects, time, path);
	}
}

int parseObjectsParameter(const std::string& objectsString, ObjectList& allSceneObjects, TimeValue time)
{
	std::vector<std::string> objects;
	boost::split(objects, objectsString, boost::is_any_of(","));

	for(int i=0; i<objects.size(); i++){
		std::string path("");
		if(objects[i][0] != '/'){
			path += "/";
		}
		path += objects[i];

		INode* pNode = GetNodeFromHierarchyPath(path);

		if(!pNode){
			ESS_LOG_ERROR("Could not find "<<objects[i]<<". Please ensure you have provide the full path.");
			return alembic_failure;
		}

		SceneEntry sEntry = createSceneEntry(pNode, time, &path);
		allSceneObjects.Append(&sEntry);//The append will make a copy sEntry, so this is safe

		addNodeChildren(pNode, allSceneObjects, time, path);
	}

	return alembic_success;
}

int ExocortexAlembicStaticInterface_ExocortexAlembicExportJobs( CONST_2013 MCHAR* jobString );
int ExocortexAlembicStaticInterface::ExocortexAlembicExportJobs( CONST_2013 MCHAR* jobString )
{
	ESS_STRUCTURED_EXCEPTION_REPORTING_START

		return ExocortexAlembicStaticInterface_ExocortexAlembicExportJobs( jobString );

	ESS_STRUCTURED_EXCEPTION_REPORTING_END

	return alembic_failure;
}

int ExocortexAlembicStaticInterface_ExocortexAlembicExportJobs( CONST_2013 MCHAR* jobString )
{
	try {

		ESS_LOG_WARNING( "Processing export jobs: "<<jobString);

		MAXInterface *pMaxInterface = GET_MAX_INTERFACE();
		pMaxInterface->ProgressStart( _T( "Exporting Alembic File" ), TRUE, DummyProgressFunction, NULL);


		TimeValue currentTime = pMaxInterface->GetTime();

		SceneEnumProc currentScene;

		std::vector<std::string> jobs;
		boost::split(jobs, jobString, boost::is_any_of("|"));

		std::vector<AlembicWriteJob*> jobPtrs;

		double dbMinFrame = 1000000.0;
		double dbMaxFrame = -1000000.0;
		double dbMaxSteps = 1;
		double dbMaxSubsteps = 1;

		for(int i=0; i<jobs.size(); i++){

			double dbFrameIn = 1.0;
			double dbFrameOut = 1.0;
			double dbFrameSteps = 1.0;
			double dbFrameSubSteps = 1.0;
			std::string filename;
			bool bTransformCache = false;
			bool bPurePointCache = false;
			bool bNormals = true;
			bool bVelocities = false;
			bool bUVs = true;
			bool bFacesets = true;
			bool bMaterialIds = true;
			bool bBindPose = true;
			bool bDynamicTopology = false;
			bool bGlobalSpace = false;
			bool bGuideCurves = false;
			bool bExportSelected = false;
			bool bFlattenHierarchy = false;
			bool bExportAsSingleMesh = false;
			bool bObjectsParameterExists = false;
			bool bUiExport = false;
			bool bAutomaticInstancing = false;
			bool bValidateMeshTopology = false;

			ObjectList allSceneObjects;
			
			std::vector<std::string> tokens;
			boost::split(tokens, jobs[i], boost::is_any_of(";"));
			for(int j=0; j<tokens.size(); j++){

				std::vector<std::string> valuePair;
				boost::split(valuePair, tokens[j], boost::is_any_of("="));
				if(valuePair.size() != 2){
					ESS_LOG_WARNING("Skipping invalid token: "<<tokens[j]);
					continue;
				}

				if(boost::iequals(valuePair[0], "in")){
					std::istringstream(valuePair[1]) >> dbFrameIn;
				}
				else if(boost::iequals(valuePair[0], "out")){
					std::istringstream(valuePair[1]) >> dbFrameOut;
				}
				else if(boost::iequals(valuePair[0], "step")){
					std::istringstream(valuePair[1]) >> dbFrameSteps;
				}
				else if(boost::iequals(valuePair[0], "substep")){
					std::istringstream(valuePair[1]) >> dbFrameSubSteps;
				}
				else if(boost::iequals(valuePair[0], "normals")){
					bNormals = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "velocities")){
					bVelocities = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "uvs")){
					bUVs = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "facesets")){
					bFacesets = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "materialids")){
					bMaterialIds = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "bindpose")){
					bBindPose = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "transformcache")){
					bTransformCache = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "validateMeshTopology")){
					bValidateMeshTopology = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "purepointcache")){
					bPurePointCache = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "dynamictopology")){
					bDynamicTopology = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "globalspace")){
					bGlobalSpace = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "guidecurves")){
					bGuideCurves = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "filename")){
					filename = valuePair[1];
				}
				else if(boost::iequals(valuePair[0], "flattenhierarchy")){
					bFlattenHierarchy = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "particlesystemtomeshconversion")){
					bExportAsSingleMesh = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "automaticinstancing")){
					bAutomaticInstancing = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "exportselected")){
					bExportSelected = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "uiExport")){
					bUiExport = parseBool(valuePair[1]);
				}
				else if(boost::iequals(valuePair[0], "objects")){
					bObjectsParameterExists = true;
					int res = parseObjectsParameter(valuePair[1], allSceneObjects, currentTime);
					if(res != alembic_success){
						pMaxInterface->ProgressEnd();
						return res;
					}
					if(allSceneObjects.Count() == 0){
						ESS_LOG_ERROR("0 export objects were specified via object parameter.");
						pMaxInterface->ProgressEnd();
						return alembic_invalidarg;
					}
				}
				else
				{
					ESS_LOG_INFO("Skipping invalid token: "<<tokens[j]);
					continue;
				}
			}

			if(filename.size() == 0){
				ESS_LOG_ERROR("No filename specified.");
				pMaxInterface->ProgressEnd();
				return alembic_invalidarg;
			}

			if(archiveExists(filename)){
				ESS_LOG_ERROR(""<<filename<<" is already open.");
				pMaxInterface->ProgressEnd();
				return alembic_invalidarg;
			}

			// check if we have incompatible subframes
			if(dbMaxSubsteps > 1.0 && dbFrameSubSteps > 1.0)
			{
				if(dbMaxSubsteps > dbFrameSubSteps)
				{
					double part = dbMaxSubsteps / dbFrameSubSteps;
					if(abs(part - floor(part)) > 0.001)
					{
						ESS_LOG_INFO("You cannot combine substeps "<<dbFrameSubSteps<<" and "<<dbMaxSubsteps<<" in one export. Aborting.");
						pMaxInterface->ProgressEnd();
						return alembic_invalidarg;
					}
				}
				else if(dbFrameSubSteps > dbMaxSubsteps )
				{
					double part = dbFrameSubSteps / dbMaxSubsteps;
					if(abs(part - floor(part)) > 0.001)
					{
						ESS_LOG_INFO("You cannot combine substeps "<<dbMaxSubsteps<<" and "<<dbFrameSubSteps<<" in one export. Aborting.");
						pMaxInterface->ProgressEnd();
						return alembic_invalidarg;
					}
				}
			}

			// remember the min and max values for the frames
			if(dbFrameIn < dbMinFrame) dbMinFrame = dbFrameIn;
			if(dbFrameOut > dbMaxFrame) dbMaxFrame = dbFrameOut;
			if(dbFrameSteps > dbMaxSteps) dbMaxSteps = dbFrameSteps;
			if(dbFrameSteps > 1.0) dbFrameSubSteps = 1.0;
			if(dbFrameSubSteps > dbMaxSubsteps) dbMaxSubsteps = dbFrameSubSteps;

			std::vector<double> frames;
			for (double frame = dbFrameIn; frame <= dbFrameOut; frame += dbFrameSteps / dbFrameSubSteps){
				frames.push_back(frame);
			}

			if(bObjectsParameterExists){
				bExportSelected = false;
			}
			else{
				if(!bUiExport){
					if(bExportSelected){
						ESS_LOG_WARNING("Objects parameter not specified. Exporting all selected objects.");
					}
					else{
						ESS_LOG_WARNING("Objects parameter not specified. Exporting all objects.");
					}
				}

				if(currentScene.Count() == 0){
					currentScene.Init(pMaxInterface->GetScene(), currentTime, pMaxInterface);
				}
				allSceneObjects.FillList(currentScene);
			}


			AlembicWriteJob * job = new AlembicWriteJob(filename, allSceneObjects, frames, pMaxInterface);
			//job->SetOption(L"exportFaceSets",facesets);
			//job->SetOption(L"globalSpace",globalspace);
			//job->SetOption(L"guideCurves",guidecurves);
			job->SetOption("exportNormals", bNormals);
			job->SetOption("exportUVs", bUVs);
			job->SetOption("exportPurePointCache", bPurePointCache);
			job->SetOption("exportBindPose", bBindPose);
			job->SetOption("exportMaterialIds", bMaterialIds);
			job->SetOption("exportDynamicTopology", bDynamicTopology);
			job->SetOption("exportSelected", bExportSelected);
			job->SetOption("flattenHierarchy", bFlattenHierarchy);
			job->SetOption("exportParticlesAsMesh", bExportAsSingleMesh);
			job->SetOption("transformCache", bTransformCache);
			job->SetOption("automaticInstancing", bAutomaticInstancing);
			job->SetOption("validateMeshTopology", bValidateMeshTopology);

			if (job->PreProcess() != true)
			{
				ESS_LOG_ERROR( "Job skipped. Not satisfied.");
				delete(job);
				continue;
			}

			// push the job to our registry
			ESS_LOG_INFO("[ExocortexAlembic] Using WriteJob:"<<jobs[i]);
			jobPtrs.push_back(job);
		}

		//ProgressBar prog;
		//prog = Application().GetUIToolkit().GetProgressBar();
		//prog.PutCaption(L"Exporting "+CString(jobCount)+L" frames from " + CString(objectCount) + " objects...");
		//prog.PutMinimum(0);
		//prog.PutMaximum(jobCount);
		//prog.PutValue(0);
		//prog.PutCancelEnabled(true);
		//prog.PutVisible(true);

		// now, let's run through all frames, and process the jobs
		bool bSuccess = true;
		double dbErrorFrame = 0.0;
		size_t nErrorJob = 0;
		for(double frame = dbMinFrame; frame<=dbMaxFrame; frame += dbMaxSteps / dbMaxSubsteps)
		{
			for(size_t i=0; i<jobPtrs.size(); i++)
			{
				bool bStatusOK = jobPtrs[i]->Process(frame);
				if( bStatusOK && ( pMaxInterface->GetCancel() == FALSE ) ){
					//TODO: should probably move this block after the loop
					double dbProgress = (100.0 * frame - dbMinFrame) / (dbMaxFrame - dbMinFrame);
					pMaxInterface->ProgressUpdate(static_cast<int>(dbProgress));
				}
				else{
					bSuccess = false;
					dbErrorFrame = frame;
					nErrorJob = i;
					goto EXIT_ON_ERROR;
				}

			}
		} EXIT_ON_ERROR:

		//prog.PutVisible(false);

		int meshErrors = 0;
		// delete all jobs
		for(size_t k=0;k<jobPtrs.size();k++){
			meshErrors += jobPtrs[k]->mMeshErrors;
			deleteArchive(jobPtrs[k]->GetFileName());
			delete(jobPtrs[k]);
		}

		// remove all known archives
		//deleteAllArchives(); Why do this?

		pMaxInterface->ProgressEnd();

		if( meshErrors > 0 ) {
			ESS_LOG_ERROR("Mesh validation failed, " << meshErrors << " mesh errors detected." );
			return alembic_failure;
		}

		if(!bSuccess){
			ESS_LOG_ERROR("Encountered error on frame "<<dbErrorFrame<<" of job "<<nErrorJob);
			return alembic_failure;
		}

	} catch( boost::exception& e ) { 								
		ESS_LOG_ERROR(__FILE__ << "(line " << __LINE__ << "). A boost::exception occurred: " << boost::diagnostic_information(e) );					
		Exocortex::essLogStackTrace(); 
		Exocortex::essSendErrorReport( "boost::exception" ); 	
		GET_MAX_INTERFACE()->ProgressEnd(); 
		return alembic_failure; 
	} catch ( std::exception& e ) { 										
		ESS_LOG_ERROR(__FILE__ << "(line " << __LINE__ << "). An std::exception occurred: " << e.what()); 	 				
		Exocortex::essLogStackTrace(); 	
		char szBuffer[1024*4]; 	
		sprintf_s( szBuffer, 1024*4, "std::exception: %s", e.what() ); 
		Exocortex::essSendErrorReport( szBuffer ); 
		GET_MAX_INTERFACE()->ProgressEnd(); 
		return alembic_failure; 
	} catch( std::string& str ) { 									
		ESS_LOG_ERROR(__FILE__ << "(line " << __LINE__ << "). A string exception was thrown: " << str);					
		Exocortex::essLogStackTrace(); 										
		char szBuffer[1024*4]; 
		sprintf_s( szBuffer, 1024*4, "C++ String Exception: %s", str.c_str() ); 
		Exocortex::essSendErrorReport( szBuffer ); 
		GET_MAX_INTERFACE()->ProgressEnd(); 
		return alembic_failure; 
	} catch (...) {														
		ESS_LOG_ERROR(__FILE__ << "(line " << __LINE__ << "). An unknown error occurred"); 						
		Exocortex::essLogStackTrace(); 
		Exocortex::essSendErrorReport( "unknown C++ exception" ); 
		GET_MAX_INTERFACE()->ProgressEnd(); 
		return alembic_failure; 
	}

	return alembic_success;
}

int ExocortexAlembicStaticInterface_ExocortexAlembicExport(CONST_2013 MCHAR * strPath, int iFrameIn, int iFrameOut, int iFrameSteps, int iFrameSubSteps, int iType,
															BOOL bExportUV, BOOL bExportMaterialIds, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology,
															BOOL bExportSelected, BOOL bFlattenHierarchy, BOOL bExportAsSingleMesh);

int ExocortexAlembicStaticInterface::ExocortexAlembicExport(CONST_2013 MCHAR * strPath, int iFrameIn, int iFrameOut, int iFrameSteps, int iFrameSubSteps, int iType,
															BOOL bExportUV, BOOL bExportMaterialIds, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology,
															BOOL bExportSelected, BOOL bFlattenHierarchy, BOOL bExportAsSingleMesh)
{
	ESS_STRUCTURED_EXCEPTION_REPORTING_START
		return ExocortexAlembicStaticInterface_ExocortexAlembicExport( strPath, iFrameIn, iFrameOut, iFrameSteps, iFrameSubSteps, iType,
															bExportUV, bExportMaterialIds, bExportEnvelopeBindPose, bExportDynamicTopology,
															bExportSelected, bFlattenHierarchy, bExportAsSingleMesh );
	ESS_STRUCTURED_EXCEPTION_REPORTING_END
	return alembic_failure;
}

int ExocortexAlembicStaticInterface_ExocortexAlembicExport(CONST_2013 MCHAR * strPath, int iFrameIn, int iFrameOut, int iFrameSteps, int iFrameSubSteps, int iType,
															BOOL bExportUV, BOOL bExportMaterialIds, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology,
															BOOL bExportSelected, BOOL bFlattenHierarchy, BOOL bExportAsSingleMesh)
{

	std::stringstream jobStream;
	jobStream<<"filename="<<strPath<<";in="<<iFrameIn<<";out="<<iFrameOut<<";step="<<iFrameSteps<<";substep="<<iFrameSubSteps;

	MeshTopologyType eTopologyType = static_cast<MeshTopologyType>(iType);
	if(eTopologyType == SURFACE){
	}
	else if(eTopologyType == POINTCACHE){
		jobStream<<";purepointcache=true";
	}
	else if(eTopologyType == SURFACE_NORMAL){
		jobStream<<";normals=true";
	}
	jobStream<<";uvs="<<bExportUV<<";materialids="<<bExportMaterialIds<<";bindpose="<<bExportEnvelopeBindPose<<";dynamictopology="<<bExportDynamicTopology;
	jobStream<<";exportselected="<<bExportSelected<<";flattenhierarchy="<<bFlattenHierarchy<<";particlesystemtomeshconversion="<<bExportAsSingleMesh;
	jobStream<<";uiExport=true";
	
	TSTR tStr = EC_UTF8_to_TSTR( jobStream.str().c_str() );
	return ExocortexAlembicStaticInterface_ExocortexAlembicExportJobs( tStr.data() );

	return 0;
}

//MarshallH: This function probably isn't necessary and it breaks the node undo mechanism
//void Alembic_NodeDeleteNotify(void *param, NotifyInfo *info)
//{
//    // Get the nodes being deleted  
//    INode *pNode = (INode*)info->callParam;
//
//    if (pNode)
//    {
//        // Check the xform controller
//        Control *pXfrmControl = pNode->GetTMController();
//        if (pXfrmControl && pXfrmControl->ClassID() == ALEMBIC_XFORM_CONTROLLER_CLASSID)
//        {
//            pXfrmControl->DeleteAllRefsFromMe();
//            pXfrmControl->DeleteAllRefsToMe();
//            pXfrmControl->DeleteThis();
//        }
//
//        // Check the visibility controller
//        Control *pVisControl = pNode->GetVisController();
//        if (pVisControl && pVisControl->ClassID() == ALEMBIC_VISIBILITY_CONTROLLER_CLASSID)
//        {
//            pVisControl->DeleteAllRefsFromMe();
//            pVisControl->DeleteAllRefsToMe();
//            pVisControl->DeleteThis();
//        }
//
//        // Run through the modifier stack
//        Object *pObject = pNode->GetObjectRef();
//        while (pObject && pObject->SuperClassID() == GEN_DERIVOB_CLASS_ID)
//        {
//            IDerivedObject *pDerivedObj = static_cast<IDerivedObject*>(pObject);
//
//            int ModStackIndex = 0;
//            while (ModStackIndex < pDerivedObj->NumModifiers())
//            {
//                Modifier *pModifier = pDerivedObj->GetModifier(ModStackIndex);
//
//                if (pModifier->ClassID() == ALEMBIC_MESH_GEOM_MODIFIER_CLASSID ||
//                    pModifier->ClassID() == ALEMBIC_MESH_NORMALS_MODIFIER_CLASSID ||
//                    pModifier->ClassID() == ALEMBIC_MESH_TOPO_MODIFIER_CLASSID ||
//                    pModifier->ClassID() == ALEMBIC_MESH_UVW_MODIFIER_CLASSID)
//                {
//                    pDerivedObj->DeleteModifier(ModStackIndex);
//                    pModifier->DeleteAllRefsFromMe();
//                    pModifier->DeleteAllRefsToMe();
//                    pModifier->DeleteThis();
//                }
//                else
//                {
//                    ModStackIndex += 1;
//                }
//            }
//
//            pObject = pDerivedObj->GetObjRef();
//        }
//    }
//}

int ExocortexAlembicStaticInterface::ExocortexAlembicInit()
{
    //int result = RegisterNotification(Alembic_NodeDeleteNotify, NULL, NOTIFY_SCENE_PRE_DELETED_NODE);
    //return result;
	return 1;
}
