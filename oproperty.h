#ifndef _PYTHON_ALEMBIC_OPROPERTY_H_
#define _PYTHON_ALEMBIC_OPROPERTY_H_

#include "foundation.h"
#include "iproperty.h"
#include "oobject.h"

typedef struct {
   PyObject_HEAD
   bool mIsCompound;    // will be able to remove this after debugging
   bool mIsArray;
   void * mArchive;

   propertyTP mPropType;
   union {
      Alembic::Abc::OScalarProperty * mBaseScalarProperty;
      Alembic::Abc::OArrayProperty * mBaseArrayProperty;
      Alembic::Abc::OCompoundProperty * mBaseCompoundProperty;
   };
   union {
      Alembic::Abc::OBoolProperty * mBoolProperty;
      Alembic::Abc::OUcharProperty * mUcharProperty;
      Alembic::Abc::OCharProperty * mCharProperty;
      Alembic::Abc::OUInt16Property * mUInt16Property;
      Alembic::Abc::OInt16Property * mInt16Property;
      Alembic::Abc::OUInt32Property * mUInt32Property;
      Alembic::Abc::OInt32Property * mInt32Property;
      Alembic::Abc::OUInt64Property * mUInt64Property;
      Alembic::Abc::OInt64Property * mInt64Property;
      Alembic::Abc::OHalfProperty * mHalfProperty;
      Alembic::Abc::OFloatProperty * mFloatProperty;
      Alembic::Abc::ODoubleProperty * mDoubleProperty;
      Alembic::Abc::OStringProperty * mStringProperty;
      Alembic::Abc::OWstringProperty * mWstringProperty;
      Alembic::Abc::OV2sProperty * mV2sProperty;
      Alembic::Abc::OV2iProperty * mV2iProperty;
      Alembic::Abc::OV2fProperty * mV2fProperty;
      Alembic::Abc::OV2dProperty * mV2dProperty;
      Alembic::Abc::OV3sProperty * mV3sProperty;
      Alembic::Abc::OV3iProperty * mV3iProperty;
      Alembic::Abc::OV3fProperty * mV3fProperty;
      Alembic::Abc::OV3dProperty * mV3dProperty;
      Alembic::Abc::OP2sProperty * mP2sProperty;
      Alembic::Abc::OP2iProperty * mP2iProperty;
      Alembic::Abc::OP2fProperty * mP2fProperty;
      Alembic::Abc::OP2dProperty * mP2dProperty;
      Alembic::Abc::OP3sProperty * mP3sProperty;
      Alembic::Abc::OP3iProperty * mP3iProperty;
      Alembic::Abc::OP3fProperty * mP3fProperty;
      Alembic::Abc::OP3dProperty * mP3dProperty;
      Alembic::Abc::OBox2sProperty * mBox2sProperty;
      Alembic::Abc::OBox2iProperty * mBox2iProperty;
      Alembic::Abc::OBox2fProperty * mBox2fProperty;
      Alembic::Abc::OBox2dProperty * mBox2dProperty;
      Alembic::Abc::OBox3sProperty * mBox3sProperty;
      Alembic::Abc::OBox3iProperty * mBox3iProperty;
      Alembic::Abc::OBox3fProperty * mBox3fProperty;
      Alembic::Abc::OBox3dProperty * mBox3dProperty;
      Alembic::Abc::OM33fProperty * mM33fProperty;
      Alembic::Abc::OM33dProperty * mM33dProperty;
      Alembic::Abc::OM44fProperty * mM44fProperty;
      Alembic::Abc::OM44dProperty * mM44dProperty;
      Alembic::Abc::OQuatfProperty * mQuatfProperty;
      Alembic::Abc::OQuatdProperty * mQuatdProperty;
      Alembic::Abc::OC3hProperty * mC3hProperty;
      Alembic::Abc::OC3fProperty * mC3fProperty;
      Alembic::Abc::OC3cProperty * mC3cProperty;
      Alembic::Abc::OC4hProperty * mC4hProperty;
      Alembic::Abc::OC4fProperty * mC4fProperty;
      Alembic::Abc::OC4cProperty * mC4cProperty;
      Alembic::Abc::ON2fProperty * mN2fProperty;
      Alembic::Abc::ON2dProperty * mN2dProperty;
      Alembic::Abc::ON3fProperty * mN3fProperty;
      Alembic::Abc::ON3dProperty * mN3dProperty;
      Alembic::Abc::OBoolArrayProperty * mBoolArrayProperty;
      Alembic::Abc::OUcharArrayProperty * mUcharArrayProperty;
      Alembic::Abc::OCharArrayProperty * mCharArrayProperty;
      Alembic::Abc::OUInt16ArrayProperty * mUInt16ArrayProperty;
      Alembic::Abc::OInt16ArrayProperty * mInt16ArrayProperty;
      Alembic::Abc::OUInt32ArrayProperty * mUInt32ArrayProperty;
      Alembic::Abc::OInt32ArrayProperty * mInt32ArrayProperty;
      Alembic::Abc::OUInt64ArrayProperty * mUInt64ArrayProperty;
      Alembic::Abc::OInt64ArrayProperty * mInt64ArrayProperty;
      Alembic::Abc::OHalfArrayProperty * mHalfArrayProperty;
      Alembic::Abc::OFloatArrayProperty * mFloatArrayProperty;
      Alembic::Abc::ODoubleArrayProperty * mDoubleArrayProperty;
      Alembic::Abc::OStringArrayProperty * mStringArrayProperty;
      Alembic::Abc::OWstringArrayProperty * mWstringArrayProperty;
      Alembic::Abc::OV2sArrayProperty * mV2sArrayProperty;
      Alembic::Abc::OV2iArrayProperty * mV2iArrayProperty;
      Alembic::Abc::OV2fArrayProperty * mV2fArrayProperty;
      Alembic::Abc::OV2dArrayProperty * mV2dArrayProperty;
      Alembic::Abc::OV3sArrayProperty * mV3sArrayProperty;
      Alembic::Abc::OV3iArrayProperty * mV3iArrayProperty;
      Alembic::Abc::OV3fArrayProperty * mV3fArrayProperty;
      Alembic::Abc::OV3dArrayProperty * mV3dArrayProperty;
      Alembic::Abc::OP2sArrayProperty * mP2sArrayProperty;
      Alembic::Abc::OP2iArrayProperty * mP2iArrayProperty;
      Alembic::Abc::OP2fArrayProperty * mP2fArrayProperty;
      Alembic::Abc::OP2dArrayProperty * mP2dArrayProperty;
      Alembic::Abc::OP3sArrayProperty * mP3sArrayProperty;
      Alembic::Abc::OP3iArrayProperty * mP3iArrayProperty;
      Alembic::Abc::OP3fArrayProperty * mP3fArrayProperty;
      Alembic::Abc::OP3dArrayProperty * mP3dArrayProperty;
      Alembic::Abc::OBox2sArrayProperty * mBox2sArrayProperty;
      Alembic::Abc::OBox2iArrayProperty * mBox2iArrayProperty;
      Alembic::Abc::OBox2fArrayProperty * mBox2fArrayProperty;
      Alembic::Abc::OBox2dArrayProperty * mBox2dArrayProperty;
      Alembic::Abc::OBox3sArrayProperty * mBox3sArrayProperty;
      Alembic::Abc::OBox3iArrayProperty * mBox3iArrayProperty;
      Alembic::Abc::OBox3fArrayProperty * mBox3fArrayProperty;
      Alembic::Abc::OBox3dArrayProperty * mBox3dArrayProperty;
      Alembic::Abc::OM33fArrayProperty * mM33fArrayProperty;
      Alembic::Abc::OM33dArrayProperty * mM33dArrayProperty;
      Alembic::Abc::OM44fArrayProperty * mM44fArrayProperty;
      Alembic::Abc::OM44dArrayProperty * mM44dArrayProperty;
      Alembic::Abc::OQuatfArrayProperty * mQuatfArrayProperty;
      Alembic::Abc::OQuatdArrayProperty * mQuatdArrayProperty;
      Alembic::Abc::OC3hArrayProperty * mC3hArrayProperty;
      Alembic::Abc::OC3fArrayProperty * mC3fArrayProperty;
      Alembic::Abc::OC3cArrayProperty * mC3cArrayProperty;
      Alembic::Abc::OC4hArrayProperty * mC4hArrayProperty;
      Alembic::Abc::OC4fArrayProperty * mC4fArrayProperty;
      Alembic::Abc::OC4cArrayProperty * mC4cArrayProperty;
      Alembic::Abc::ON2fArrayProperty * mN2fArrayProperty;
      Alembic::Abc::ON2dArrayProperty * mN2dArrayProperty;
      Alembic::Abc::ON3fArrayProperty * mN3fArrayProperty;
      Alembic::Abc::ON3dArrayProperty * mN3dArrayProperty;
   };
} oProperty;

PyObject * oProperty_new(Alembic::Abc::OCompoundProperty compound, char * in_propName, char * in_propType, int tsIndex, void * in_Archive);
void oProperty_deletePointers(oProperty * prop);

bool register_object_oProperty(PyObject *module);

#endif
