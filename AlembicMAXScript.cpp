#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicMeshUtilities.h"
#include "AlembicXformController.h"
#include "AlembicCameraBaseModifier.h"
#include "AlembicSimpleParticle.h"
#include "MeshMtlList.h"
#include "SceneEnumProc.h"
#include "AlembicDefinitions.h"
#include "AlembicWriteJob.h"
#include "Utility.h"
#include "AlembicSimpleSpline.h"
#include "AlembicXformUtilities.h"
#include <maxscript\maxscript.h>

// Dummy function for progress bar
DWORD WINAPI DummyProgressFunction(LPVOID arg)
{
	return 0;
}

class ExocortexAlembicStaticInterface : public FPInterfaceDesc
{
public:

    static const Interface_ID id;
    
    enum FunctionIDs
    {
        exocortexAlembicImport,
		exocortexAlembicExport,
	    exocortexAlembicImportMesh,	
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
            11,                     //* Number  of arguments * /
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
        BOOL bExportUV, BOOL bExportMaterialIds, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology, BOOL bExportSelected );

    BEGIN_FUNCTION_MAP
        FN_6(exocortexAlembicImport, TYPE_INT, ExocortexAlembicImport, TYPE_FILENAME, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_INT)
        FN_9(exocortexAlembicImportMesh, TYPE_MESH, ExocortexAlembicImportMesh, TYPE_MESH, TYPE_FILENAME, TYPE_STRING, TYPE_FLOAT, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL)
		FN_11(exocortexAlembicExport, TYPE_INT, ExocortexAlembicExport, TYPE_FILENAME, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL)
    END_FUNCTION_MAP
};

const Interface_ID ExocortexAlembicStaticInterface::id = Interface_ID(0x4bd4297f, 0x4e8403d7);
static ExocortexAlembicStaticInterface exocortexAlembic;

void AlembicImport_TimeControl( alembic_importoptions &options ) {

	// Create the xform modifier
	HelperObject *pHelper = static_cast<HelperObject*>
		(GetCOREInterface()->CreateInstance(HELPER_CLASS_ID, ALEMBIC_TIME_CONTROL_HELPER_CLASSID));

	TimeValue zero( 0 );

	// Set the alembic id
	pHelper->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pHelper, 0, "current" ), zero, 0.0f );
	pHelper->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pHelper, 0, "offset" ), zero, 0.0f );
	pHelper->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pHelper, 0, "factor" ), zero, 1.0f );
		
	// Create the object node
	INode *node = GetCOREInterface12()->CreateObjectNode(pHelper, pHelper->GetObjectName() );

    // Add the new inode to our current scene list
	SceneEntry *pEntry = options.sceneEnumProc.Append(node, pHelper, OBTYPE_CURVES, &std::string( node->GetName() ) ); 
    options.currentSceneList.Append(pEntry);

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

void AlembicImport_ConnectTimeControl( char* szControllerName, alembic_importoptions &options ) {

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

int ExocortexAlembicStaticInterface::ExocortexAlembicImport(MCHAR* strPath, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds, BOOL bAttachToExisting, int iVisOption)
{
	ESS_CPP_EXCEPTION_REPORTING_START

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
	Interface12 *i = GetCOREInterface12();
    i->ProgressStart("Importing Alembic File", TRUE, DummyProgressFunction, NULL);

   MeshMtlList allMtls;
  
   options.sceneEnumProc.Init(i->GetScene(), i->GetTime(), i, &allMtls);
   options.currentSceneList.FillList(options.sceneEnumProc);
   Object *currentObject = NULL;

   AlembicImport_TimeControl( options );

      
   // Create the max objects as needed, we loop through the list in reverse to create
   // the children node first and then hook them up to their parents
   int progressUpdateInterval = 0;
   for(int j=(int)objects.size()-1; j>=0 ; j -= 1)
   {
	   progressUpdateInterval ++;
	   if( ( progressUpdateInterval % 10 ) == 0 ) {
			double dbProgress = ((double)objects.size() - 1 - j) / objects.size();
	        i->ProgressUpdate(static_cast<int>(100 * dbProgress));
	   }

		// XForm
       if(Alembic::AbcGeom::IXform::matches(objects[j].getMetaData()))
       {
		   //ESS_LOG_INFO( "AlembicImport_XForm: " << objects[j].getFullName() );
           int ret = AlembicImport_XForm(file, objects[j].getFullName(), options);
       }

       // PolyMesh
       else if (Alembic::AbcGeom::IPolyMesh::matches(objects[j].getMetaData()))
       {
		   //ESS_LOG_INFO( "AlembicImport_PolyMesh: " << objects[j].getFullName() );
           int ret = AlembicImport_PolyMesh(file, objects[j].getFullName(), options); 
       }

       // Camera
       else if (Alembic::AbcGeom::ICamera::matches(objects[j].getMetaData()))
       {
		  // ESS_LOG_INFO( "AlembicImport_Camera: " << objects[j].getFullName() );
           int ret = AlembicImport_Camera(file, objects[j].getFullName(), options);
       }

       // Points
       else if (Alembic::AbcGeom::IPoints::matches(objects[j].getMetaData()))
       {
		   //ESS_LOG_INFO( "AlembicImport_Points: " << objects[j].getFullName() );
           int ret = AlembicImport_Points(file, objects[j].getFullName(), options);
       }

       // Curves
       else if (Alembic::AbcGeom::ICurves::matches(objects[j].getMetaData()))
       {
		   //ESS_LOG_INFO( "AlembicImport_Shape: " << objects[j].getFullName() );
           int ret = AlembicImport_Shape(file, objects[j].getFullName(), options);
       }
   }

   i->ProgressEnd();
   delRefArchive(file);

   ESS_CPP_EXCEPTION_REPORTING_END

   return alembic_success;
}


Mesh* ExocortexAlembicStaticInterface::ExocortexAlembicImportMesh(Mesh* pMesh, MCHAR* strPath, MCHAR* strIdentifier, float time, BOOL bImportFaceList, BOOL bImportVertices, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportMaterialIds )
{
	if( pMesh == NULL ) {
		pMesh = CreateNewMesh();
	}

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
		options.nDataFillFlags |= ALEMBIC_DATAFILL_FACESETS;
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
   /*
	std::vector<Point3> points;
	points.push_back( Point3( 0, 0, 0 ) );
	points.push_back( Point3( 10, 0, 0 ) );
	points.push_back( Point3( 0, 10, 0 ) );
	std::vector<Point3> normals;
	normals.push_back( Point3( 0, 0, 1 ) );
	normals.push_back( Point3( 0, 0, 1 ) );
	normals.push_back( Point3( 0, 0, 1 ) );
    std::vector<Point2> uvs;
	uvs.push_back( Point2( 0, 0 ) );
	uvs.push_back( Point2( 1, 0 ) );
	uvs.push_back( Point2( 0, 1 ) );
    std::vector<int> triangleVertIndices;
	triangleVertIndices.push_back( 0 );
	triangleVertIndices.push_back( 1 );
	triangleVertIndices.push_back( 2 );
	triangleVertIndices.push_back( 0 );
	triangleVertIndices.push_back( 2 );
	triangleVertIndices.push_back( 1 );

 	ESS_CPP_EXCEPTION_REPORTING_START

	assert(points.size() == normals.size() && normals.size() == uvs.size());
    assert(triangleVertIndices.size() % 3 == 0);
 
    int numVertices = (int) points.size();
    int numTriangles = (int) triangleVertIndices.size() / 3;
	pMesh = CreateNewMesh();
    Mesh &mesh = *pMesh;
 
    // set vertex positions
    mesh.setNumVerts(numVertices);
    for (int i = 0; i < numVertices; i++)
        mesh.setVert(i, points[i]);
    
    // set vertex normals
    mesh.SpecifyNormals();
    MeshNormalSpec *normalSpec = mesh.GetSpecifiedNormals();
    normalSpec->ClearNormals();
    normalSpec->SetNumNormals(numVertices);
    for (int i = 0; i < numVertices; i++)
    {
        normalSpec->Normal(i) = normals[i].Normalize();
        normalSpec->SetNormalExplicit(i, true);
    }
 
    // set UVs
    // TODO: multiple map channels?
    // channel 0 is reserved for vertex color, channel 1 is the default texture mapping
    mesh.setNumMaps(2);
    mesh.setMapSupport(1, TRUE);  // enable map channel
    MeshMap &map = mesh.Map(1);
    map.setNumVerts(numVertices);
    for (int i = 0; i < numVertices; i++)
    {
        UVVert &texVert = map.tv[i];
        texVert.x = uvs[i].x;
        texVert.y = uvs[i].y;
        texVert.z = 0.0f;
    }
 
    // set triangles
    mesh.setNumFaces(numTriangles);
    normalSpec->SetNumFaces(numTriangles);
    map.setNumFaces(numTriangles);
    for (int i = 0, j = 0; i < numTriangles; i++, j += 3)
    {
        // three vertex indices of a triangle
        int v0 = triangleVertIndices[j];
        int v1 = triangleVertIndices[j+1];
        int v2 = triangleVertIndices[j+2];
 
        // vertex positions
        Face &face = mesh.faces[i];
        face.setMatID(1);
        face.setEdgeVisFlags(1, 1, 1);
        face.setVerts(v0, v1, v2);
 
        // vertex normals
        MeshNormalFace &normalFace = normalSpec->Face(i);
        normalFace.SpecifyAll();
        normalFace.SetNormalID(0, v0);
        normalFace.SetNormalID(1, v1);
        normalFace.SetNormalID(2, v2);
 
        // vertex UVs
        TVFace &texFace = map.tf[i];
        texFace.setTVerts(v0, v1, v2);
    }
 
    mesh.InvalidateGeomCache();
    mesh.InvalidateTopologyCache();*/
 
   ESS_CPP_EXCEPTION_REPORTING_END

	return pMesh;
}

int ExocortexAlembicStaticInterface::ExocortexAlembicExport(MCHAR * strPath, int iFrameIn, int iFrameOut, int iFrameSteps, int iFrameSubSteps, int iType,
                                                            BOOL bExportUV, BOOL bExportMaterialIds, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology,
                                                            BOOL bExportSelected)
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

	Interface12 *i = GetCOREInterface12();
    i->ProgressStart("Exporting Alembic File", TRUE, DummyProgressFunction, NULL);

    MeshTopologyType eTopologyType = static_cast<MeshTopologyType>(iType);

    MeshMtlList allMtls;
    SceneEnumProc currentScene(i->GetScene(), i->GetTime(), i, &allMtls);
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
