#ifndef _PYTHON_ALEMBIC_OPROPERTY_H_
#define _PYTHON_ALEMBIC_OPROPERTY_H_

#include "iproperty.h"
#include "oobject.h"

typedef struct {
  PyObject_HEAD bool mIsArray;
  int intent;
  void *mArchive;

  propertyTP mPropType;
  union {
    Abc::OScalarProperty *mBaseScalarProperty;
    Abc::OArrayProperty *mBaseArrayProperty;
    Abc::OCompoundProperty *mBaseCompoundProperty;
  };
  union {
    Abc::OBoolProperty *mBoolProperty;
    Abc::OUcharProperty *mUcharProperty;
    Abc::OCharProperty *mCharProperty;
    Abc::OUInt16Property *mUInt16Property;
    Abc::OInt16Property *mInt16Property;
    Abc::OUInt32Property *mUInt32Property;
    Abc::OInt32Property *mInt32Property;
    Abc::OUInt64Property *mUInt64Property;
    Abc::OInt64Property *mInt64Property;
    Abc::OHalfProperty *mHalfProperty;
    Abc::OFloatProperty *mFloatProperty;
    Abc::ODoubleProperty *mDoubleProperty;
    Abc::OStringProperty *mStringProperty;
    Abc::OWstringProperty *mWstringProperty;
    Abc::OV2sProperty *mV2sProperty;
    Abc::OV2iProperty *mV2iProperty;
    Abc::OV2fProperty *mV2fProperty;
    Abc::OV2dProperty *mV2dProperty;
    Abc::OV3sProperty *mV3sProperty;
    Abc::OV3iProperty *mV3iProperty;
    Abc::OV3fProperty *mV3fProperty;
    Abc::OV3dProperty *mV3dProperty;
    Abc::OP2sProperty *mP2sProperty;
    Abc::OP2iProperty *mP2iProperty;
    Abc::OP2fProperty *mP2fProperty;
    Abc::OP2dProperty *mP2dProperty;
    Abc::OP3sProperty *mP3sProperty;
    Abc::OP3iProperty *mP3iProperty;
    Abc::OP3fProperty *mP3fProperty;
    Abc::OP3dProperty *mP3dProperty;
    Abc::OBox2sProperty *mBox2sProperty;
    Abc::OBox2iProperty *mBox2iProperty;
    Abc::OBox2fProperty *mBox2fProperty;
    Abc::OBox2dProperty *mBox2dProperty;
    Abc::OBox3sProperty *mBox3sProperty;
    Abc::OBox3iProperty *mBox3iProperty;
    Abc::OBox3fProperty *mBox3fProperty;
    Abc::OBox3dProperty *mBox3dProperty;
    Abc::OM33fProperty *mM33fProperty;
    Abc::OM33dProperty *mM33dProperty;
    Abc::OM44fProperty *mM44fProperty;
    Abc::OM44dProperty *mM44dProperty;
    Abc::OQuatfProperty *mQuatfProperty;
    Abc::OQuatdProperty *mQuatdProperty;
    Abc::OC3hProperty *mC3hProperty;
    Abc::OC3fProperty *mC3fProperty;
    Abc::OC3cProperty *mC3cProperty;
    Abc::OC4hProperty *mC4hProperty;
    Abc::OC4fProperty *mC4fProperty;
    Abc::OC4cProperty *mC4cProperty;
    Abc::ON2fProperty *mN2fProperty;
    Abc::ON2dProperty *mN2dProperty;
    Abc::ON3fProperty *mN3fProperty;
    Abc::ON3dProperty *mN3dProperty;
    Abc::OBoolArrayProperty *mBoolArrayProperty;
    Abc::OUcharArrayProperty *mUcharArrayProperty;
    Abc::OCharArrayProperty *mCharArrayProperty;
    Abc::OUInt16ArrayProperty *mUInt16ArrayProperty;
    Abc::OInt16ArrayProperty *mInt16ArrayProperty;
    Abc::OUInt32ArrayProperty *mUInt32ArrayProperty;
    Abc::OInt32ArrayProperty *mInt32ArrayProperty;
    Abc::OUInt64ArrayProperty *mUInt64ArrayProperty;
    Abc::OInt64ArrayProperty *mInt64ArrayProperty;
    Abc::OHalfArrayProperty *mHalfArrayProperty;
    Abc::OFloatArrayProperty *mFloatArrayProperty;
    Abc::ODoubleArrayProperty *mDoubleArrayProperty;
    Abc::OStringArrayProperty *mStringArrayProperty;
    Abc::OWstringArrayProperty *mWstringArrayProperty;
    Abc::OV2sArrayProperty *mV2sArrayProperty;
    Abc::OV2iArrayProperty *mV2iArrayProperty;
    Abc::OV2fArrayProperty *mV2fArrayProperty;
    Abc::OV2dArrayProperty *mV2dArrayProperty;
    Abc::OV3sArrayProperty *mV3sArrayProperty;
    Abc::OV3iArrayProperty *mV3iArrayProperty;
    Abc::OV3fArrayProperty *mV3fArrayProperty;
    Abc::OV3dArrayProperty *mV3dArrayProperty;
    Abc::OP2sArrayProperty *mP2sArrayProperty;
    Abc::OP2iArrayProperty *mP2iArrayProperty;
    Abc::OP2fArrayProperty *mP2fArrayProperty;
    Abc::OP2dArrayProperty *mP2dArrayProperty;
    Abc::OP3sArrayProperty *mP3sArrayProperty;
    Abc::OP3iArrayProperty *mP3iArrayProperty;
    Abc::OP3fArrayProperty *mP3fArrayProperty;
    Abc::OP3dArrayProperty *mP3dArrayProperty;
    Abc::OBox2sArrayProperty *mBox2sArrayProperty;
    Abc::OBox2iArrayProperty *mBox2iArrayProperty;
    Abc::OBox2fArrayProperty *mBox2fArrayProperty;
    Abc::OBox2dArrayProperty *mBox2dArrayProperty;
    Abc::OBox3sArrayProperty *mBox3sArrayProperty;
    Abc::OBox3iArrayProperty *mBox3iArrayProperty;
    Abc::OBox3fArrayProperty *mBox3fArrayProperty;
    Abc::OBox3dArrayProperty *mBox3dArrayProperty;
    Abc::OM33fArrayProperty *mM33fArrayProperty;
    Abc::OM33dArrayProperty *mM33dArrayProperty;
    Abc::OM44fArrayProperty *mM44fArrayProperty;
    Abc::OM44dArrayProperty *mM44dArrayProperty;
    Abc::OQuatfArrayProperty *mQuatfArrayProperty;
    Abc::OQuatdArrayProperty *mQuatdArrayProperty;
    Abc::OC3hArrayProperty *mC3hArrayProperty;
    Abc::OC3fArrayProperty *mC3fArrayProperty;
    Abc::OC3cArrayProperty *mC3cArrayProperty;
    Abc::OC4hArrayProperty *mC4hArrayProperty;
    Abc::OC4fArrayProperty *mC4fArrayProperty;
    Abc::OC4cArrayProperty *mC4cArrayProperty;
    Abc::ON2fArrayProperty *mN2fArrayProperty;
    Abc::ON2dArrayProperty *mN2dArrayProperty;
    Abc::ON3fArrayProperty *mN3fArrayProperty;
    Abc::ON3dArrayProperty *mN3dArrayProperty;
  };
} oProperty;

PyObject *oProperty_new(Abc::OCompoundProperty compound,
                        std::string compoundFullName, char *in_propName,
                        char *in_propType, int tsIndex, void *in_Archive);
void oProperty_deletePointers(oProperty *prop);

bool register_object_oProperty(PyObject *module);

#endif
