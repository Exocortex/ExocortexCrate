#ifndef _PYTHON_ALEMBIC_OOBJECT_H_
#define _PYTHON_ALEMBIC_OOBJECT_H_

#include "foundation.h"

enum oObjectType
{
   oObjectType_Xform,
   oObjectType_Camera,
   oObjectType_PolyMesh,
   oObjectType_Curves,
   oObjectType_Points,
   oObjectType_SubD,
   oObjectType_FaceSet,    // new
   oObjectType_NuPatch     // new
};

typedef struct
{
   oObjectType mType; 
   union
   {
     Alembic::AbcGeom::OXform * mXform;
     Alembic::AbcGeom::OCamera * mCamera;
     Alembic::AbcGeom::OPolyMesh * mPolyMesh;
     Alembic::AbcGeom::OCurves * mCurves;
     Alembic::AbcGeom::OPoints * mPoints;
     Alembic::AbcGeom::OSubD * mSubD;
     Alembic::AbcGeom::OFaceSet * mFaceSet;  // new
     Alembic::AbcGeom::ONuPatch * mNuPatch;  // new
   };
} oObjectPtr;

Alembic::Abc::OCompoundProperty getCompoundFromOObject(oObjectPtr in_Casted);

typedef struct {
  PyObject_HEAD
  Alembic::Abc::OObject * mObject;
  oObjectPtr mCasted;
  void * mArchive;
} oObject;

PyObject * oObject_new(Alembic::Abc::OObject in_Object, oObjectPtr in_Casted, void * in_Archive);
void oObject_deletePointers(oObject * object);

bool register_object_oObject(PyObject *module);

#endif


