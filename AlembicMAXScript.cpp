#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicArchiveStorage.h"
#include "AlembicMeshUtilities.h"
#include "AlembicSplineUtilities.h"
#include "AlembicXformUtilities.h"
#include "AlembicXformController.h"
#include "AlembicCameraBaseModifier.h"
#include "AlembicSimpleParticle.h"
#include "SceneEnumProc.h"
#include "AlembicDefinitions.h"
#include "AlembicWriteJob.h"
#include "Utility.h"

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

class ExocortexAlembicStaticInterface : public FPInterfaceDesc
{
public:

	static const Interface_ID id;

	enum FunctionIDs
	{
		exocortexAlembicImport,
		exocortexAlembicExport,
		exocortexAlembicImportMesh,	
        exocortexAlembicInit
	};

	ExocortexAlembicStaticInterface()
		: FPInterfaceDesc(id, _M("ExocortexAlembic"), 0, NULL, FP_CORE, end)
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
			end); 

		AppendFunction(
			exocortexAlembicImportMesh,	//* function ID * /
			_M("importMesh"),           //* internal name * /
			0,                      //* function name string resource name * / 
			TYPE_MESH,               //* Return type * /
			0,                      //* Flags  * /
			9,                     //* Number  of arguments * /
			_M("mesh"),     //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_MESH,      //* arg type * /
			_M("path"),     //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_FILENAME,      //* arg type * /
			_M("identifier"),     //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_STRING,      //* arg type * /
			_M("time"),     //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_FLOAT,      //* arg type * /        	  
			_M("importFaceList"),//* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("importVerticse"),//* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("importNormals"),//* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("importUVs"),    //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /
			_M("importMaterialIds"), //* argument internal name * /
			0,                  //* argument localizable name string resource id * /
			TYPE_BOOL,          //* arg type * /              
			end); 

		AppendFunction(
			exocortexAlembicExport,	//* function ID * /
			_M("export"),           //* internal name * /
			0,                      //* function name string resource name * / 
			TYPE_INT,               //* Return type * /
			0,                      //* Flags  * /
			12,                     //* Number  of arguments * /
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
			end); 	
       
        AppendFunction(
			exocortexAlembicInit,	//* function ID * /
			_M("init"),             //* internal name * /
            0,                      //* function name string resource name * / 
			TYPE_INT,               //* Return type * /
			0,                      //* Flags  * /
			0,                     //* Number  of arguments * /
            end);           
    }

	static int ExocortexAlembicImport(
		MCHAR * strPath, 
		BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds,
		BOOL bAttachToExisting,
		int iVisOption);

	static Mesh* ExocortexAlembicImportMesh(
		Mesh* pMesh,
		MCHAR * strPath, MCHAR * strIdentifier,
		float time, 
		BOOL bImportFaceList, BOOL bImportVertices, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds
		);
	static int ExocortexAlembicExport(
		MCHAR * strPath,
		int iFrameIn, int iFrameOut, int iFrameSteps, int iFrameSubSteps,
		int iType,
		BOOL bExportUV, BOOL bExportMaterialIds, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology, BOOL bExportSelected, BOOL bFlattenHierarchy );

    static int ExocortexAlembicInit();
     	
    BEGIN_FUNCTION_MAP
		FN_6(exocortexAlembicImport, TYPE_INT, ExocortexAlembicImport, TYPE_FILENAME, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_INT)
		FN_9(exocortexAlembicImportMesh, TYPE_MESH, ExocortexAlembicImportMesh, TYPE_MESH, TYPE_FILENAME, TYPE_STRING, TYPE_FLOAT, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL)
		FN_12(exocortexAlembicExport, TYPE_INT, ExocortexAlembicExport, TYPE_FILENAME, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL)
        FN_0(exocortexAlembicInit, TYPE_INT, ExocortexAlembicInit);
	END_FUNCTION_MAP
};

const Interface_ID ExocortexAlembicStaticInterface::id = Interface_ID(0x4bd4297f, 0x4e8403d7);
static ExocortexAlembicStaticInterface exocortexAlembic;

void AlembicImport_TimeControl( alembic_importoptions &options ) {

	// check if an Alembic Time Control already exists in the scene.

	BOOL alreadyExists = ExecuteMAXScriptScript( "select $Alembic_Time_Control", TRUE );
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
	SceneEntry *pEntry = options.sceneEnumProc.Append(node, pHelper, OBTYPE_CURVES, &std::string( node->GetName() ) ); 
	options.currentSceneList.Append(pEntry);

	GET_MAX_INTERFACE()->SelectNode( node );

	char szBuffer[10000];	
	sprintf_s( szBuffer, 10000,
		"$'%s'.current.controller = float_expression()\n"
		"$'%s'.current.controller.setExpression \"S\"\n"
		"$'%s'.offset.controller = bezier_float()\n"
		"$'%s'.factor.controller = bezier_float()\n"
		, node->GetName(), node->GetName(), node->GetName(), node->GetName() );
	ExecuteMAXScriptScript( szBuffer );

	options.pTimeControl = pHelper;
}

void AlembicImport_ConnectTimeControl( char* szControllerName, alembic_importoptions &options ) 
{
	char szBuffer[10000];	
	sprintf_s( szBuffer, 10000, 
		"%s.controller = float_expression()\n"
		"%s.controller.AddScalarTarget \"current\" $'%s'.current.controller\n"
		"%s.controller.AddScalarTarget \"offset\" $'%s'.offset.controller\n"
		"%s.controller.AddScalarTarget \"factor\" $'%s'.factor.controller\n"
		"%s.controller.setExpression \"current * factor + offset\"\n",
		szControllerName,
		szControllerName, options.pTimeControl->GetObjectName(),
		szControllerName, options.pTimeControl->GetObjectName(),
		szControllerName, options.pTimeControl->GetObjectName(),
		szControllerName );

	ExecuteMAXScriptScript( szBuffer );
}

int ExocortexAlembicStaticInterface_ExocortexAlembicImport(MCHAR* strPath, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds, BOOL bAttachToExisting, int iVisOption);

int ExocortexAlembicStaticInterface::ExocortexAlembicImport(MCHAR* strPath, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds, BOOL bAttachToExisting, int iVisOption)
{
	ESS_STRUCTURED_EXCEPTION_REPORTING_START

		return ExocortexAlembicStaticInterface_ExocortexAlembicImport( strPath, bImportNormals, bImportUVs, bImportMaterialIds, bAttachToExisting, iVisOption);

	ESS_STRUCTURED_EXCEPTION_REPORTING_END

	return alembic_failure;
}

int ExocortexAlembicStaticInterface_ExocortexAlembicImport(MCHAR* strPath, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds, BOOL bAttachToExisting, int iVisOption)
{
	ESS_CPP_EXCEPTION_REPORTING_START

		//TestMasterUnit();
		
		if( ! HasFullLicense() ) {
			ESS_LOG_ERROR( "No valid license found for Exocortex Alembic." );
			return alembic_failure;
		}

		ESS_LOG_INFO( "ExocortexAlembicImport( strPath=" << strPath <<
			", bImportNormals=" << bImportNormals << ", bImportUVs=" << bImportUVs <<
			", bImportMaterialIds=" << bImportMaterialIds << ", bAttachToExisting=" << bAttachToExisting <<
			", iVisOption=" << iVisOption << " )" );

		alembic_importoptions options;
		options.importNormals = (bImportNormals != FALSE);
		options.importUVs = (bImportUVs != FALSE);
		options.importMaterialIds = (bImportMaterialIds != FALSE);
		options.attachToExisting = (bAttachToExisting != FALSE);
		options.importVisibility = static_cast<VisImportOption>(iVisOption);

		// If no filename, then return an error code
		if(strPath[0] == 0) {
			ESS_LOG_ERROR( "No filename specified." );
			return alembic_invalidarg;
		}

		if( ! fs::exists( strPath ) ) {
			ESS_LOG_ERROR( "Can't file Alembic file.  Path: " << strPath );
			return alembic_invalidarg;
		}

		std::string file(strPath);

		// Try opening up the archive
		Alembic::Abc::IArchive *pArchive = getArchiveFromID(file);
		if (!pArchive) {
			ESS_LOG_ERROR( "Unable to open Alembic file: " << file  );
			return alembic_failure;
		}

        // Since the archive is valid, add a reference to it
        addRefArchive(file);

		// let's figure out which objects we have
		std::vector<Alembic::Abc::IObject> objects;
		objects.push_back(pArchive->getTop());
		for(size_t i=0;i<objects.size();i++)
		{
			// first, let's recurse
			for(size_t j=0;j<objects[i].getNumChildren();j++)
				objects.push_back(objects[i].getChild(j));
		}

		// Get a list of the current objects in the scene
		MAXInterface *i = GET_MAX_INTERFACE();

		options.sceneEnumProc.Init(i->GetScene(), i->GetTime(), i);
		options.currentSceneList.FillList(options.sceneEnumProc);
		Object *currentObject = NULL;

		ESS_LOG_INFO( "AlembicImport_TimeControl." );
				
		AlembicImport_TimeControl( options );


		// Create the max objects as needed, we loop through the list in reverse to create
		// the children node first and then hook them up to their parents
		int totalAlemicItems = 0;
		ESS_LOG_INFO( "Alembic file contents:" );
		for(int j=(int)objects.size()-1; j>=0 ; j -= 1)
		{
			ESS_LOG_INFO( objects[j].getFullName() );
			// XForm
			if(Alembic::AbcGeom::IXform::matches(objects[j].getMetaData()))
			{
				totalAlemicItems++;
			}

			// PolyMesh
			else if (Alembic::AbcGeom::IPolyMesh::matches(objects[j].getMetaData()))
			{
				totalAlemicItems++;
			}

			// Camera
			else if (Alembic::AbcGeom::ICamera::matches(objects[j].getMetaData()))
			{
				totalAlemicItems++;
			}

			// Points
			else if (Alembic::AbcGeom::IPoints::matches(objects[j].getMetaData()))
			{
				totalAlemicItems++;
			}

			// Curves
			else if (Alembic::AbcGeom::ICurves::matches(objects[j].getMetaData()))
			{
				totalAlemicItems++;
			}
		}
		char szBuffer[1000];
		sprintf_s( szBuffer, 1000, "Importing %i Alembic Streams", totalAlemicItems );
		i->ProgressStart(szBuffer, TRUE, DummyProgressFunction, NULL);

		int progressUpdateInterval = 0;
		int lastUpdateProcess = 0;
		for(int j=(int)objects.size()-1; j>=0 ; j -= 1)
		{
			if( ( progressUpdateInterval % 10 ) == 0 ) {
				if( lastUpdateProcess != progressUpdateInterval ) {
					double dbProgress = ((double)progressUpdateInterval) / totalAlemicItems;
					i->ProgressUpdate(static_cast<int>(100 * dbProgress));
					lastUpdateProcess = progressUpdateInterval;
				}
			}
			
			// XForm
			if(Alembic::AbcGeom::IXform::matches(objects[j].getMetaData()))
			{
				ESS_LOG_INFO( "AlembicImport_XForm: " << objects[j].getFullName() );
				int ret = AlembicImport_XForm(file, objects[j].getFullName(), options);
				progressUpdateInterval ++;
			} 

			// PolyMesh
			else if (Alembic::AbcGeom::IPolyMesh::matches(objects[j].getMetaData()))
			{
				ESS_LOG_INFO( "AlembicImport_PolyMesh: " << objects[j].getFullName() );
				int ret = AlembicImport_PolyMesh(file, objects[j].getFullName(), options); 
				progressUpdateInterval ++;
			}

			// Camera
			else if (Alembic::AbcGeom::ICamera::matches(objects[j].getMetaData()))
			{
				ESS_LOG_INFO( "AlembicImport_Camera: " << objects[j].getFullName() );
				int ret = AlembicImport_Camera(file, objects[j].getFullName(), options);
				progressUpdateInterval ++;
			}

			// Points
			else if (Alembic::AbcGeom::IPoints::matches(objects[j].getMetaData()))
			{
				ESS_LOG_INFO( "AlembicImport_Points: " << objects[j].getFullName() );
				int ret = AlembicImport_Points(file, objects[j].getFullName(), options);
				progressUpdateInterval ++;
			}

			// Curves
			else if (Alembic::AbcGeom::ICurves::matches(objects[j].getMetaData()))
			{
				ESS_LOG_INFO( "AlembicImport_Shape: " << objects[j].getFullName() );
				int ret = AlembicImport_Shape(file, objects[j].getFullName(), options);
				progressUpdateInterval ++;
			}
			else if (Alembic::AbcGeom::ISubD::matches(objects[j].getMetaData())) {
				ESS_LOG_WARNING( "Exocortex Alembic for 3DS Max does not yet support SubD primitives: " << objects[j].getFullName() );
			}
			else {
				std::string schemaObjTitle = objects[j].getMetaData().get( "schemaObjTitle" );
				std::string schema = objects[j].getMetaData().get( "schema" );
				ESS_LOG_INFO( "Diagnostics, primitive not supported: " << objects[j].getFullName() << "( " << schemaObjTitle << " " << schema << " )" );
			}
		}

		i->ProgressEnd();
		delRefArchive(file);

	ESS_CPP_EXCEPTION_REPORTING_END

	return alembic_success;
}

Mesh* ExocortexAlembicStaticInterface_ExocortexAlembicImportMesh(Mesh* pMesh, MCHAR* strPath, MCHAR* strIdentifier, float time, BOOL bImportFaceList, BOOL bImportVertices, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds );

Mesh* ExocortexAlembicStaticInterface::ExocortexAlembicImportMesh(Mesh* pMesh, MCHAR* strPath, MCHAR* strIdentifier, float time, BOOL bImportFaceList, BOOL bImportVertices, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds )
{
	if( pMesh == NULL ) {
		pMesh = CreateNewMesh();
	}

	ESS_STRUCTURED_EXCEPTION_REPORTING_START
		return ExocortexAlembicStaticInterface_ExocortexAlembicImportMesh( pMesh, strPath, strIdentifier, time, bImportFaceList, bImportVertices, bImportNormals, bImportUVs, bImportMaterialIds );
	ESS_STRUCTURED_EXCEPTION_REPORTING_END

	return pMesh;
}

Mesh* ExocortexAlembicStaticInterface_ExocortexAlembicImportMesh(Mesh* pMesh, MCHAR* strPath, MCHAR* strIdentifier, float time, BOOL bImportFaceList, BOOL bImportVertices, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds )
{
	ESS_CPP_EXCEPTION_REPORTING_START

		if( ! HasFullLicense() ) {
			ESS_LOG_ERROR( "No valid license found for Exocortex Alembic." );
			return pMesh;
		}

		ESS_LOG_INFO( "ExocortexAlembicImportMesh( strPath=" << strPath <<
			", strIdentifier=" << strIdentifier << 
			", bImportFaceList=" << bImportFaceList << ", bImportVertices=" << bImportVertices <<
			", bImportNormals=" << bImportNormals << ", bImportUVs=" << bImportUVs <<
			", bImportMaterialIds=" << bImportMaterialIds << " )" );

		if( strlen( strPath ) == 0 ) {
			ESS_LOG_ERROR( "No filename specified." );
			return pMesh;
		}
		if( strlen( strIdentifier ) == 0 ) {
			ESS_LOG_ERROR( "No path specified." );
			return pMesh;
		}
		if( ! fs::exists( strPath ) ) {
			ESS_LOG_ERROR( "Can't file Alembic file.  Path: " << strPath );
			return pMesh;
		}

		Alembic::AbcGeom::IObject iObj;
		try {
			iObj = getObjectFromArchive(strPath, strIdentifier);
		} catch( std::exception exp ) {
			ESS_LOG_ERROR( "Can not open Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier << " reason: " << exp.what() );
			return pMesh;
		}

		if(!iObj.valid()) {
			ESS_LOG_ERROR( "Not a valid Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier );
			return pMesh;
		}

		alembic_fillmesh_options options;
		options.pIObj = &iObj;
		options.dTicks = GetTimeValueFromSeconds( time );
		options.nDataFillFlags = 0;
		if( bImportFaceList ) {
			options.nDataFillFlags |= ALEMBIC_DATAFILL_FACELIST;
		}
		if( bImportVertices ) {
			options.nDataFillFlags |= ALEMBIC_DATAFILL_VERTEX;
		}
		if( bImportNormals ) {
			options.nDataFillFlags |= ALEMBIC_DATAFILL_NORMALS;
		}
		if( bImportUVs ) {
			options.nDataFillFlags |= ALEMBIC_DATAFILL_UVS;
		}
		if( bImportMaterialIds ) {
			options.nDataFillFlags |= ALEMBIC_DATAFILL_MATERIALIDS;
		}
		options.pMesh = pMesh;

		try {
			AlembicImport_FillInPolyMesh(options);
			pMesh = options.pMesh;
		}
		catch(std::exception exp ) {
			ESS_LOG_ERROR( "Error creating mesh from Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier << " reason: " << exp.what() );
			return pMesh;
		}

		ESS_LOG_INFO( "NumFaces: " << pMesh->getNumFaces() << "  NumVerts: " << pMesh->getNumVerts() );
	
		ESS_CPP_EXCEPTION_REPORTING_END

	return pMesh;
}

int ExocortexAlembicStaticInterface_ExocortexAlembicExport(MCHAR * strPath, int iFrameIn, int iFrameOut, int iFrameSteps, int iFrameSubSteps, int iType,
															BOOL bExportUV, BOOL bExportMaterialIds, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology,
															BOOL bExportSelected, BOOL bFlattenHierarchy);

int ExocortexAlembicStaticInterface::ExocortexAlembicExport(MCHAR * strPath, int iFrameIn, int iFrameOut, int iFrameSteps, int iFrameSubSteps, int iType,
															BOOL bExportUV, BOOL bExportMaterialIds, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology,
															BOOL bExportSelected, BOOL bFlattenHierarchy)
{
	ESS_STRUCTURED_EXCEPTION_REPORTING_START
		return ExocortexAlembicStaticInterface_ExocortexAlembicExport( strPath, iFrameIn, iFrameOut, iFrameSteps, iFrameSubSteps, iType,
															bExportUV, bExportMaterialIds, bExportEnvelopeBindPose, bExportDynamicTopology,
															bExportSelected, bFlattenHierarchy );
	ESS_STRUCTURED_EXCEPTION_REPORTING_END
	return alembic_failure;
}

int ExocortexAlembicStaticInterface_ExocortexAlembicExport(MCHAR * strPath, int iFrameIn, int iFrameOut, int iFrameSteps, int iFrameSubSteps, int iType,
															BOOL bExportUV, BOOL bExportMaterialIds, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology,
															BOOL bExportSelected, BOOL bFlattenHierarchy)
{
	ESS_CPP_EXCEPTION_REPORTING_START

		if( ! HasFullLicense() ) {
			ESS_LOG_ERROR( "No valid license found for Exocortex Alembic." );
			return alembic_failure;
		}

		ESS_LOG_INFO( "ExocortexAlembicExport( strPath=" << strPath <<
			", iFrameIn=" << iFrameIn << ", iFrameOut=" << iFrameOut <<
			", iFrameSteps=" << iFrameSteps << ", iFrameSubSteps=" << iFrameSubSteps <<
			", iType=" << iType  << ", bExportUV=" << bExportUV <<
			", bExportMaterialIds=" << bExportMaterialIds << ", bExportEnvelopeBindPose=" << bExportEnvelopeBindPose <<
			", bExportDynamicTopology=" << bExportDynamicTopology << ", bExportSelected=" << bExportSelected <<
			" )" );

		MAXInterface *i = GET_MAX_INTERFACE();
		i->ProgressStart("Exporting Alembic File", TRUE, DummyProgressFunction, NULL);

		MeshTopologyType eTopologyType = static_cast<MeshTopologyType>(iType);

		SceneEnumProc currentScene(i->GetScene(), i->GetTime(), i);
		ObjectList allSceneObjects(currentScene);
		Object *currentObject = NULL;

		if (strlen(strPath) <= 0)
		{
			ESS_LOG_ERROR( "No filename specified." );
			i->ProgressEnd();
			return alembic_invalidarg;
		}

		// Delete this archive if we have already imported it and are currently using it
		deleteArchive(strPath);

		// Keep track of the min/max values when processing multiple jobs
		double dbFrameMinIn = 1000000.0;
		double dbFrameMaxOut = -1000000.0;
		double dbFrameMaxSteps = 1.0;
		double dbFrameMaxSubSteps = 1.0;

		double dbFrameIn = static_cast<double>(iFrameIn);
		double dbFrameOut = static_cast<double>(iFrameOut);
		double dbFrameSteps = static_cast<double>(iFrameSteps);
		double dbFrameSubSteps = (iFrameSteps > 1) ? 1.0 : static_cast<double>(iFrameSubSteps);

		// check if we have incompatible subframes
		if (dbFrameMaxSubSteps > 1.0 && dbFrameSubSteps > 1.0)
		{
			if (dbFrameMaxSubSteps > dbFrameSubSteps)
			{
				double part = dbFrameMaxSubSteps / dbFrameSubSteps;
				if (abs(part - floor(part)) > 0.001)
				{
					ESS_LOG_ERROR( "Invalid combination of substeps in the same export. Aborting." );
					i->ProgressEnd();
					return alembic_invalidarg;
				}
			}
			else if (dbFrameSubSteps > dbFrameMaxSubSteps)
			{
				double part = dbFrameSubSteps / dbFrameMaxSubSteps;
				if (abs(part - floor(part)) > 0.001)
				{
					ESS_LOG_ERROR( "Invalid combination of substeps in the same export. Aborting." );
					i->ProgressEnd();
					return alembic_invalidarg;
				}
			}
		}

		std::vector<double> frames;
		for (double dbFrame = dbFrameIn; dbFrame <= dbFrameOut; dbFrame += dbFrameSteps / dbFrameSubSteps)
		{
			// Adding frames
			frames.push_back(dbFrame);
		}

		AlembicWriteJob *job = new AlembicWriteJob(strPath, allSceneObjects, frames, i);
		job->SetOption("exportNormals", eTopologyType == SURFACE_NORMAL);
		job->SetOption("exportUVs", (bExportUV != FALSE));
		job->SetOption("exportPurePointCache", eTopologyType == POINTCACHE);
		job->SetOption("exportBindPose", (bExportEnvelopeBindPose != FALSE));
		job->SetOption("exportMaterialIds", (bExportMaterialIds != FALSE));
		job->SetOption("exportDynamicTopology", (bExportDynamicTopology != FALSE));
		job->SetOption("indexedNormals", true);
		job->SetOption("indexedUVs", true);
		job->SetOption("exportSelected", (bExportSelected != FALSE));
		job->SetOption("flattenHierarchy",(bFlattenHierarchy != FALSE));

		// check if the job is satisfied
		if (job->PreProcess() != true)
		{
			ESS_LOG_ERROR( "Job skipped. Not satisfied.");
			delete(job);
			i->ProgressEnd();
			return alembic_failure;
		}

		dbFrameMinIn = min(dbFrameMinIn, dbFrameIn);
		dbFrameMaxOut = max(dbFrameMaxOut, dbFrameOut);
		dbFrameMaxSteps = max(dbFrameMaxSteps, dbFrameSteps);   // TODO: Shouldn't this be a min?
		dbFrameMaxSubSteps = max(dbFrameMaxSubSteps, dbFrameSubSteps);

		// now, let's run through all frames, and process the jobs
		for (double dbFrame = dbFrameMinIn; dbFrame <= dbFrameMaxOut; dbFrame += dbFrameMaxSteps / dbFrameMaxSubSteps)
		{
			double dbProgress = (100.0 * dbFrame - dbFrameMinIn) / (dbFrameMaxOut - dbFrameMinIn);
			i->ProgressUpdate(static_cast<int>(dbProgress));
			job->Process(dbFrame);
		}

		delete(job);

		i->ProgressEnd();

	ESS_CPP_EXCEPTION_REPORTING_END

	return alembic_success;
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
