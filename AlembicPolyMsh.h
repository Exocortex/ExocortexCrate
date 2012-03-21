#ifndef _ALEMBIC_POLYMSH_H_
#define _ALEMBIC_POLYMSH_H_

#include "AlembicObject.h"

typedef std::map<int, std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> > facesetmap;
typedef std::map<int, std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> >::iterator facesetmap_it;
typedef std::pair<int, std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> > facesetmap_insert_pair;
typedef std::pair<facesetmap_it, bool> facesetmap_ret_pair;

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

class AlembicPolyMesh: public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::OPolyMeshSchema mMeshSchema;
   Alembic::AbcGeom::XformSample mXformSample;
   Alembic::AbcGeom::OPolyMeshSchema::Sample mMeshSample;
   std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mFaceCountVec;
   std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> mFaceIndicesVec;
   std::vector<Alembic::Abc::V3f> mBindPoseVec;
   std::vector<Alembic::Abc::V3f> mVelocitiesVec;
   std::vector<Alembic::Abc::V2f> mUvVec;
   std::vector<Alembic::Abc::uint32_t> mUvIndexVec;
   std::vector<Alembic::Abc::uint32_t> mMatIdIndexVec;
   Alembic::Abc::ALEMBIC_VERSION_NS::OV3fArrayProperty mBindPoseProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OV3fArrayProperty mVelocityProperty;
   Alembic::Abc::ALEMBIC_VERSION_NS::OUInt32ArrayProperty mMatIdProperty;
   std::vector<float> mRadiusVec;
   std::vector<VNormal> m_MeshSmoothGroupNormals;
   facesetmap mFaceSetsMap;
private:
    void BuildMeshSmoothingGroupNormals(Mesh &mesh);
    void BuildMeshSmoothingGroupNormals(MNMesh &mesh);
    void ClearMeshSmoothingGroupNormals();
public:
    static Point3 GetVertexNormal(Mesh* mesh, int faceNo, int faceVertNo, std::vector<VNormal> &sgVertexNormals);
    static Point3 GetVertexNormal(MNMesh *mesh, int faceNo, int faceVertNo, std::vector<VNormal> &sgVertexNormals);
    static void make_face_uv(Face *f, Point3 *tv);
    static BOOL CheckForFaceMap(Mtl* mtl, Mesh* mesh);
public:

   AlembicPolyMesh(const SceneEntry &in_Ref, AlembicWriteJob *in_Job);
   ~AlembicPolyMesh();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual bool Save(double time);
};

class SortableV3f : public Alembic::Abc::V3f
{
public:  
   SortableV3f()
   {
      x = y = z = 0.0f;
   }

   SortableV3f(const Alembic::Abc::V3f & other)
   {
      x = other.x;
      y = other.y;
      z = other.z;
   }
   bool operator < ( const SortableV3f & other) const
   {
      if(other.x != x)
         return other.x > x;
      if(other.y != y)
         return other.y > y;
      return other.z > z;
   }
   bool operator > ( const SortableV3f & other) const
   {
      if(other.x != x)
         return other.x < x;
      if(other.y != y)
         return other.y < y;
      return other.z < z;
   }
   bool operator == ( const SortableV3f & other) const
   {
      if(other.x != x)
         return false;
      if(other.y != y)
         return false;
      return other.z == z;
   }
};

class SortableV2f : public Alembic::Abc::V2f
{
public:  
   SortableV2f()
   {
      x = y = 0.0f;
   }

   SortableV2f(const Alembic::Abc::V2f & other)
   {
      x = other.x;
      y = other.y;
   }
   bool operator < ( const SortableV2f & other) const
   {
      if(other.x != x)
         return other.x > x;
      return other.y > y;
   }
   bool operator > ( const SortableV2f & other) const
   {
      if(other.x != x)
         return other.x < x;
      return other.y < y;
   }
   bool operator == ( const SortableV2f & other) const
   {
      if(other.x != x)
         return false;
      return other.y == y;
   }
};

#endif
