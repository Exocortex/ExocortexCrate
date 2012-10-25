#ifndef _PYTHON_ALEMBIC_OOBJECT_H_
#define _PYTHON_ALEMBIC_OOBJECT_H_

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
     AbcG::OXform * mXform;
     AbcG::OCamera * mCamera;
     AbcG::OPolyMesh * mPolyMesh;
     AbcG::OCurves * mCurves;
     AbcG::OPoints * mPoints;
     AbcG::OSubD * mSubD;
     AbcG::OFaceSet * mFaceSet;  // new
     AbcG::ONuPatch * mNuPatch;  // new
   };
} oObjectPtr;

Abc::OCompoundProperty getCompoundFromOObject(oObjectPtr in_Casted);

typedef struct {
  PyObject_HEAD
  Abc::OObject * mObject;
  oObjectPtr mCasted;
  void * mArchive;
   int tsIndex;   // new, for quick access!
} oObject;

PyObject * oObject_new(Abc::OObject in_Object, oObjectPtr in_Casted, void * in_Archive, int tsIndex);
void oObject_deletePointers(oObject * object);

bool register_object_oObject(PyObject *module);

#endif


