#ifndef _ALEMBIC_POLYMSH_H_
#define _ALEMBIC_POLYMSH_H_

#include "AlembicObject.h"

//typedef std::map<int, std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> > facesetmap;
//typedef std::map<int, std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> >::iterator facesetmap_it;
//typedef std::pair<int, std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> > facesetmap_insert_pair;
//typedef std::pair<facesetmap_it, bool> facesetmap_ret_pair;

class AlembicPolyMesh: public AlembicObject
{
private:
   Alembic::AbcGeom::OXformSchema mXformSchema;
   Alembic::AbcGeom::OPolyMeshSchema mMeshSchema;
   Alembic::AbcGeom::XformSample mXformSample;
   Alembic::AbcGeom::OPolyMeshSchema::Sample mMeshSample;
   Alembic::Abc::ALEMBIC_VERSION_NS::OUInt32ArrayProperty mMatIdProperty;

public:

   AlembicPolyMesh(const SceneEntry &in_Ref, AlembicWriteJob *in_Job);
   ~AlembicPolyMesh();

   virtual Alembic::Abc::OCompoundProperty GetCompound();
   virtual bool Save(double time);
};

#endif
