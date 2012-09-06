#ifndef __ALEMBIC_INTERMEDIATE_POLYMESH_3DSMax_H__
#define __ALEMBIC_INTERMEDIATE_POLYMESH_3DSMax_H__


#include "AlembicMax.h"
#include "AlembicIntermediatePolyMesh.h"

class VNormal
{   
public:     
    Point3 norm;     
    DWORD smooth;     
    VNormal *next;     
    BOOL init;      
    VNormal() {smooth=0;next=NULL;init=FALSE;norm=Point3(0,0,0);}     
    VNormal(Point3 &n,DWORD s) {next=NULL;init=TRUE;norm=n;smooth=s;}     
    ~VNormal() {smooth=0;next=NULL;init=FALSE;norm=Point3(0,0,0);}     
    void AddNormal(Point3 &n,DWORD s);     
    Point3 &GetNormal(DWORD s);     
    void Normalize();
};


struct materialStr{
	
	std::string name;
	int matId;

};

typedef std::map<int, materialStr> meshMaterialsMap;
typedef std::map<int, materialStr>::iterator meshMaterialsMap_it;
typedef std::map<int, materialStr>::const_iterator meshMaterialsMap_cit;

typedef std::map<AnimHandle, meshMaterialsMap> mergedMeshMaterialsMap;
typedef std::map<AnimHandle, meshMaterialsMap>::iterator mergedMeshMaterialsMap_it;
typedef std::map<AnimHandle, meshMaterialsMap>::const_iterator mergedMaterialsMap_cit;

struct materialsMergeStr
{
	mergedMeshMaterialsMap groupMatMap;
	AnimHandle currUniqueHandle;
	int nNextMatId;
	bool bPreserveIds;

	materialsMergeStr():currUniqueHandle(NULL), nNextMatId(0), bPreserveIds(false)
	{}

	int getUniqueMatId(int matId);
	materialStr& getMatEntry(AnimHandle uniqueHandle, int matId);

	void setMatName(int matId, const std::string& name);
};

class AlembicWriteJob;

class IntermediatePolyMesh3DSMax : public AlembicIntermediatePolyMesh
{
	std::vector<VNormal> m_MeshSmoothGroupNormals;


private:
    void BuildMeshSmoothingGroupNormals(Mesh &mesh);
    void BuildMeshSmoothingGroupNormals(MNMesh &mesh);
    void ClearMeshSmoothingGroupNormals();
public:
    static Point3 GetVertexNormal(Mesh* mesh, int faceNo, int faceVertNo, std::vector<VNormal> &sgVertexNormals);
    static Point3 GetVertexNormal(MNMesh *mesh, int faceNo, int faceVertNo, std::vector<VNormal> &sgVertexNormals);
    static void make_face_uv(Face *f, Point3 *tv);
    static BOOL CheckForFaceMap(Mtl* mtl, Mesh* mesh);

	void Save(std::map<std::string, bool>& mOptions, Mesh *triMesh, MNMesh* polyMesh, Matrix3& meshTM, Mtl* pMtl, int nMatId, bool bFirstFrame, materialsMergeStr* pMatMerge);
};


#endif