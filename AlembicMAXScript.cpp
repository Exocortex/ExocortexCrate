#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicPolyMeshModifier.h"
#include "AlembicXFormModifier.h"
#include "MeshMtlList.h"
#include "SceneEnumProc.h"
#include "AlembicDefinitions.h"

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
            _M("import"),     //* internal name * /
            0,              //* function name string resource name * / 
            TYPE_INT,       //* Return type * /
            0,              //* Flags  * /
            1,              //* Number  of arguments * /
                _M("fileName"),    //* argument internal name * /
                0,          //* argument localizable name string resource id * /
                TYPE_FILENAME,   //* arg type * /
            end); 

		 AppendFunction(
            exocortexAlembicExport,	//* function ID * /
            _M("export"),   //* internal name * /
            0,              //* function name string resource name * / 
            TYPE_INT,       //* Return type * /
            0,              //* Flags  * /
            1,              //* Number  of arguments * /
                _M("fileName"),    //* argument internal name * /
                0,          //* argument localizable name string resource id * /
                TYPE_FILENAME,   //* arg type * /
            end); 
    }

    static int ExocortexAlembicImport(MCHAR * strFileName);
	static int ExocortexAlembicExport(MCHAR * strFileName);

    BEGIN_FUNCTION_MAP
        FN_1(exocortexAlembicImport, TYPE_INT, ExocortexAlembicImport, TYPE_FILENAME)
		FN_1(exocortexAlembicExport, TYPE_INT, ExocortexAlembicExport, TYPE_FILENAME)
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



int ExocortexAlembicStaticInterface::ExocortexAlembicImport(MCHAR* strFileName)
{
	alembic_importoptions importOptions;

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
   Interface7 *i = GetCOREInterface7();
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
	}

   delRefArchive(file);

   return 0;
}

int ExocortexAlembicStaticInterface::ExocortexAlembicExport(MCHAR * strFileName)
{
	return 0;
}