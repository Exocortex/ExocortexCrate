#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicPolyMeshModifier.h"
#include "AlembicXFormCtrl.h"
#include "AlembicCameraModifier.h"
#include "MeshMtlList.h"
#include "SceneEnumProc.h"
#include "AlembicDefinitions.h"
#include "AlembicWriteJob.h"
#include "Utility.h"

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
                _M("fileName"),     //* argument internal name * /
                0,                  //* argument localizable name string resource id * /
                TYPE_FILENAME,      //* arg type * /
                _M("importNormals"),//* argument internal name * /
                0,                  //* argument localizable name string resource id * /
                TYPE_BOOL,          //* arg type * /
                _M("importUVs"),    //* argument internal name * /
                0,                  //* argument localizable name string resource id * /
                TYPE_BOOL,          //* arg type * /
                _M("importClusters"), //* argument internal name * /
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
            exocortexAlembicExport,	//* function ID * /
            _M("export"),           //* internal name * /
            0,                      //* function name string resource name * / 
            TYPE_INT,               //* Return type * /
            0,                      //* Flags  * /
            11,                     //* Number  of arguments * /
                _M("fileName"),     //* argument internal name * /
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
                _M("exportClusters"), //* argument internal name * /
                0,                  //* argument localizable name string resource id * /
                TYPE_BOOL,          //* arg type * /
                _M("exportEnvelopeBindPose"), //* argument internal name * /
                0,                  //* argument localizable name string resource id * /
                TYPE_BOOL,          //* arg type * /
                _M("exportDynamicTopology"), //* argument internal name * /
                0,                  //* argument localizable name string resource id * /
                TYPE_BOOL,          //* arg type * /
                _M("exportSelected"), //* argument internal name * /
                0,                  //* argument localizable name string resource id * /
                TYPE_BOOL,          //* arg type * /
            end); 
    }

    static int ExocortexAlembicImport(MCHAR * strFileName, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportClusters, BOOL bAttachToExisting, int iVisOption);
	static int ExocortexAlembicExport(MCHAR * strFileName, int iFrameIn, int iFrameOut, int iFrameSteps, int iFrameSubSteps, int iType,
                                      BOOL bExportUV, BOOL bExportClusters, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology, BOOL bExportSelected);

    BEGIN_FUNCTION_MAP
        FN_6(exocortexAlembicImport, TYPE_INT, ExocortexAlembicImport, TYPE_FILENAME, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_INT)
		FN_11(exocortexAlembicExport, TYPE_INT, ExocortexAlembicExport, TYPE_FILENAME, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL)
    END_FUNCTION_MAP
};

const Interface_ID ExocortexAlembicStaticInterface::id = Interface_ID(0x4bd4297f, 0x4e8403d7);
static ExocortexAlembicStaticInterface exocortexAlembic;

/*TriObject *createTriangleMesh(const std::vector<Point3> &points,
                              const std::vector<Point3> &normals,
                              const std::vector<Point2> &uvs,
                              const std::vector<int> &triangleVertIndices)
{
    TriObject *triobj = CreateNewTriObject();
    if (triobj == NULL)
        return NULL;
 
    assert(points.size() == normals.size() && normals.size() == uvs.size());
    assert(triangleVertIndices.size() % 3 == 0);
 
    int numVertices = (int) points.size();
    int numTriangles = (int) triangleVertIndices.size() / 3;
    Mesh &mesh = triobj->GetMesh();
 
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
    mesh.InvalidateTopologyCache();
 
    return triobj;
}
*/

int ExocortexAlembicStaticInterface::ExocortexAlembicImport(MCHAR* strFileName, BOOL bImportNormals, BOOL bImportUVs, BOOL bImportClusters, BOOL bAttachToExisting, int iVisOption)
{
	alembic_importoptions importOptions;
    importOptions.importNormals = (bImportNormals != FALSE);
    importOptions.importUVs = (bImportUVs != FALSE);
    importOptions.importClusters = (bImportClusters != FALSE);
    importOptions.attachToExisting = (bAttachToExisting != FALSE);
    importOptions.importVisibility = static_cast<VisImportOption>(iVisOption);

	// If no filename, then return an error code
	if(strFileName[0] == 0)
	   return alembic_invalidarg;

	std::string file(strFileName);

   // Try opening up the archive
   Alembic::Abc::IArchive *pArchive = getArchiveFromID(file);
   if (!pArchive)
	   return alembic_failure;

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
   MeshMtlList allMtls;
   Interface12 *i = GetCOREInterface12();
   importOptions.sceneEnumProc.Init(i->GetScene(), i->GetTime(), i, &allMtls);
   importOptions.currentSceneList.FillList(importOptions.sceneEnumProc);
   Object *currentObject = NULL;

   // Create the max objects as needed, we loop through the list in reverse to create
   // the children node first and then hook them up to their parents
   for(int i=(int)objects.size()-1; i>=0 ; i -= 1)
   {
		// XForm
		if(Alembic::AbcGeom::IXform::matches(objects[i].getMetaData()))
		{
            int ret = AlembicImport_XForm(file, objects[i].getFullName(), importOptions);
		}

		// PolyMesh
		else if (Alembic::AbcGeom::IPolyMesh::matches(objects[i].getMetaData()))
		{
			int ret = AlembicImport_PolyMesh(file, objects[i].getFullName(), importOptions); 
        }

        // Camera
        else if (Alembic::AbcGeom::OCamera::matches(objects[i].getMetaData()))
        {
            int ret = AlembicImport_Camera(file, objects[i].getFullName(), importOptions);
        }
	}

   delRefArchive(file);

   return 0;
}

int ExocortexAlembicStaticInterface::ExocortexAlembicExport(MCHAR * strFileName, int iFrameIn, int iFrameOut, int iFrameSteps, int iFrameSubSteps, int iType,
                                                            BOOL bExportUV, BOOL bExportClusters, BOOL bExportEnvelopeBindPose, BOOL bExportDynamicTopology,
                                                            BOOL bExportSelected)
{
    Interface12 *i = GetCOREInterface12();
    i->ProgressStart("Exporting Alembic File", TRUE, DummyProgressFunction, NULL);

    MeshTopologyType eTopologyType = static_cast<MeshTopologyType>(iType);

    MeshMtlList allMtls;
    SceneEnumProc currentScene(i->GetScene(), i->GetTime(), i, &allMtls);
    ObjectList allSceneObjects(currentScene);
    Object *currentObject = NULL;

    if (strlen(strFileName) <= 0)
    {
        MessageBox(GetActiveWindow(), "[alembic] No filename specified.", "Error", MB_OK);
        return 1;
    }

    // Delete this archive if we have already imported it and are currently using it
    deleteArchive(strFileName);

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
                MessageBox(GetActiveWindow(), "[ExocortexAlembic] Invalid combination of substeps in the same export. Aborting.", "Error", MB_OK);
                return 1;
            }
        }
        else if (dbFrameSubSteps > dbFrameMaxSubSteps)
        {
            double part = dbFrameSubSteps / dbFrameMaxSubSteps;
            if (abs(part - floor(part)) > 0.001)
            {
                MessageBox(GetActiveWindow(), "[ExocortexAlembic] Invalid combination of substeps in the same export. Aborting.", "Error", MB_OK);
                return 1;
            }
        }
    }

    std::vector<double> frames;
    for (double dbFrame = dbFrameIn; dbFrame <= dbFrameOut; dbFrame += dbFrameSteps / dbFrameSubSteps)
    {
        // Adding frames
        frames.push_back(dbFrame);
    }

    AlembicWriteJob *job = new AlembicWriteJob(strFileName, allSceneObjects, frames, i);
    job->SetOption("exportNormals", eTopologyType != SURFACE);
    job->SetOption("exportUVs", (bExportUV != FALSE));
    job->SetOption("exportFaceSets", eTopologyType != NORMAL);
    job->SetOption("exportBindPose", (bExportEnvelopeBindPose != FALSE));
    job->SetOption("exportPurePointCache", (bExportClusters != FALSE));
    job->SetOption("exportDynamicTopology", (bExportDynamicTopology != FALSE));
    job->SetOption("indexedNormals", true);
    job->SetOption("indexedUVs", true);
    job->SetOption("exportSelected", (bExportSelected != FALSE));

    // check if the job is satisfied
    if (job->PreProcess() != true)
    {
        MessageBox(GetActiveWindow(), "[alembic] Job skipped. Not satisfied.", "Error", MB_OK);
        delete(job);
        return 1;
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

	return 0;
}
