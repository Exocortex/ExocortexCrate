#ifndef _PYTHON_ALEMBIC_IPROPERTY_H_
#define _PYTHON_ALEMBIC_IPROPERTY_H_

#include "foundation.h"

enum propertyTP
{
   propertyTP_unknown,
   propertyTP_compound,
   propertyTP_boolean,
   propertyTP_uchar,
   propertyTP_char,
   propertyTP_uint16,
   propertyTP_int16,
   propertyTP_uint32,
   propertyTP_int32,
   propertyTP_uint64,
   propertyTP_int64,
   propertyTP_half,
   propertyTP_float,
   propertyTP_double,
   propertyTP_string,
   propertyTP_wstring,
   propertyTP_v2s,
   propertyTP_v2i,
   propertyTP_v2f,
   propertyTP_v2d,
   propertyTP_v3s,
   propertyTP_v3i,
   propertyTP_v3f,
   propertyTP_v3d,
   propertyTP_p2s,
   propertyTP_p2i,
   propertyTP_p2f,
   propertyTP_p2d,
   propertyTP_p3s,
   propertyTP_p3i,
   propertyTP_p3f,
   propertyTP_p3d,
   propertyTP_box2s,
   propertyTP_box2i,
   propertyTP_box2f,
   propertyTP_box2d,
   propertyTP_box3s,
   propertyTP_box3i,
   propertyTP_box3f,
   propertyTP_box3d,
   propertyTP_m33f,
   propertyTP_m33d,
   propertyTP_m44f,
   propertyTP_m44d,
   propertyTP_quatf,
   propertyTP_quatd,
   propertyTP_c3h,
   propertyTP_c3f,
   propertyTP_c3c,
   propertyTP_c4h,
   propertyTP_c4f,
   propertyTP_c4c,
   propertyTP_n2f,
   propertyTP_n2d,
   propertyTP_n3f,
   propertyTP_n3d,
   propertyTP_boolean_array,
   propertyTP_uchar_array,
   propertyTP_char_array,
   propertyTP_uint16_array,
   propertyTP_int16_array,
   propertyTP_uint32_array,
   propertyTP_int32_array,
   propertyTP_uint64_array,
   propertyTP_int64_array,
   propertyTP_half_array,
   propertyTP_float_array,
   propertyTP_double_array,
   propertyTP_string_array,
   propertyTP_wstring_array,
   propertyTP_v2s_array,
   propertyTP_v2i_array,
   propertyTP_v2f_array,
   propertyTP_v2d_array,
   propertyTP_v3s_array,
   propertyTP_v3i_array,
   propertyTP_v3f_array,
   propertyTP_v3d_array,
   propertyTP_p2s_array,
   propertyTP_p2i_array,
   propertyTP_p2f_array,
   propertyTP_p2d_array,
   propertyTP_p3s_array,
   propertyTP_p3i_array,
   propertyTP_p3f_array,
   propertyTP_p3d_array,
   propertyTP_box2s_array,
   propertyTP_box2i_array,
   propertyTP_box2f_array,
   propertyTP_box2d_array,
   propertyTP_box3s_array,
   propertyTP_box3i_array,
   propertyTP_box3f_array,
   propertyTP_box3d_array,
   propertyTP_m33f_array,
   propertyTP_m33d_array,
   propertyTP_m44f_array,
   propertyTP_m44d_array,
   propertyTP_quatf_array,
   propertyTP_quatd_array,
   propertyTP_c3h_array,
   propertyTP_c3f_array,
   propertyTP_c3c_array,
   propertyTP_c4h_array,
   propertyTP_c4f_array,
   propertyTP_c4c_array,
   propertyTP_n2f_array,
   propertyTP_n2d_array,
   propertyTP_n3f_array,
   propertyTP_n3d_array,
   propertyTP_NBELEMENTS
};

typedef struct {
   PyObject_HEAD
   bool mIsArray;
   propertyTP mPropType;
   union {
      Alembic::Abc::IScalarProperty * mBaseScalarProperty;
      Alembic::Abc::IArrayProperty * mBaseArrayProperty;
      Alembic::Abc::ICompoundProperty * mBaseCompoundProperty;
   };
   union {
      Alembic::Abc::IBoolProperty * mBoolProperty;
      Alembic::Abc::IUcharProperty * mUcharProperty;
      Alembic::Abc::ICharProperty * mCharProperty;
      Alembic::Abc::IUInt16Property * mUInt16Property;
      Alembic::Abc::IInt16Property * mInt16Property;
      Alembic::Abc::IUInt32Property * mUInt32Property;
      Alembic::Abc::IInt32Property * mInt32Property;
      Alembic::Abc::IUInt64Property * mUInt64Property;
      Alembic::Abc::IInt64Property * mInt64Property;
      Alembic::Abc::IHalfProperty * mHalfProperty;
      Alembic::Abc::IFloatProperty * mFloatProperty;
      Alembic::Abc::IDoubleProperty * mDoubleProperty;
      Alembic::Abc::IStringProperty * mStringProperty;
      Alembic::Abc::IWstringProperty * mWstringProperty;
      Alembic::Abc::IV2sProperty * mV2sProperty;
      Alembic::Abc::IV2iProperty * mV2iProperty;
      Alembic::Abc::IV2fProperty * mV2fProperty;
      Alembic::Abc::IV2dProperty * mV2dProperty;
      Alembic::Abc::IV3sProperty * mV3sProperty;
      Alembic::Abc::IV3iProperty * mV3iProperty;
      Alembic::Abc::IV3fProperty * mV3fProperty;
      Alembic::Abc::IV3dProperty * mV3dProperty;
      Alembic::Abc::IP2sProperty * mP2sProperty;
      Alembic::Abc::IP2iProperty * mP2iProperty;
      Alembic::Abc::IP2fProperty * mP2fProperty;
      Alembic::Abc::IP2dProperty * mP2dProperty;
      Alembic::Abc::IP3sProperty * mP3sProperty;
      Alembic::Abc::IP3iProperty * mP3iProperty;
      Alembic::Abc::IP3fProperty * mP3fProperty;
      Alembic::Abc::IP3dProperty * mP3dProperty;
      Alembic::Abc::IBox2sProperty * mBox2sProperty;
      Alembic::Abc::IBox2iProperty * mBox2iProperty;
      Alembic::Abc::IBox2fProperty * mBox2fProperty;
      Alembic::Abc::IBox2dProperty * mBox2dProperty;
      Alembic::Abc::IBox3sProperty * mBox3sProperty;
      Alembic::Abc::IBox3iProperty * mBox3iProperty;
      Alembic::Abc::IBox3fProperty * mBox3fProperty;
      Alembic::Abc::IBox3dProperty * mBox3dProperty;
      Alembic::Abc::IM33fProperty * mM33fProperty;
      Alembic::Abc::IM33dProperty * mM33dProperty;
      Alembic::Abc::IM44fProperty * mM44fProperty;
      Alembic::Abc::IM44dProperty * mM44dProperty;
      Alembic::Abc::IQuatfProperty * mQuatfProperty;
      Alembic::Abc::IQuatdProperty * mQuatdProperty;
      Alembic::Abc::IC3hProperty * mC3hProperty;
      Alembic::Abc::IC3fProperty * mC3fProperty;
      Alembic::Abc::IC3cProperty * mC3cProperty;
      Alembic::Abc::IC4hProperty * mC4hProperty;
      Alembic::Abc::IC4fProperty * mC4fProperty;
      Alembic::Abc::IC4cProperty * mC4cProperty;
      Alembic::Abc::IN2fProperty * mN2fProperty;
      Alembic::Abc::IN2dProperty * mN2dProperty;
      Alembic::Abc::IN3fProperty * mN3fProperty;
      Alembic::Abc::IN3dProperty * mN3dProperty;
      Alembic::Abc::IBoolArrayProperty * mBoolArrayProperty;
      Alembic::Abc::IUcharArrayProperty * mUcharArrayProperty;
      Alembic::Abc::ICharArrayProperty * mCharArrayProperty;
      Alembic::Abc::IUInt16ArrayProperty * mUInt16ArrayProperty;
      Alembic::Abc::IInt16ArrayProperty * mInt16ArrayProperty;
      Alembic::Abc::IUInt32ArrayProperty * mUInt32ArrayProperty;
      Alembic::Abc::IInt32ArrayProperty * mInt32ArrayProperty;
      Alembic::Abc::IUInt64ArrayProperty * mUInt64ArrayProperty;
      Alembic::Abc::IInt64ArrayProperty * mInt64ArrayProperty;
      Alembic::Abc::IHalfArrayProperty * mHalfArrayProperty;
      Alembic::Abc::IFloatArrayProperty * mFloatArrayProperty;
      Alembic::Abc::IDoubleArrayProperty * mDoubleArrayProperty;
      Alembic::Abc::IStringArrayProperty * mStringArrayProperty;
      Alembic::Abc::IWstringArrayProperty * mWstringArrayProperty;
      Alembic::Abc::IV2sArrayProperty * mV2sArrayProperty;
      Alembic::Abc::IV2iArrayProperty * mV2iArrayProperty;
      Alembic::Abc::IV2fArrayProperty * mV2fArrayProperty;
      Alembic::Abc::IV2dArrayProperty * mV2dArrayProperty;
      Alembic::Abc::IV3sArrayProperty * mV3sArrayProperty;
      Alembic::Abc::IV3iArrayProperty * mV3iArrayProperty;
      Alembic::Abc::IV3fArrayProperty * mV3fArrayProperty;
      Alembic::Abc::IV3dArrayProperty * mV3dArrayProperty;
      Alembic::Abc::IP2sArrayProperty * mP2sArrayProperty;
      Alembic::Abc::IP2iArrayProperty * mP2iArrayProperty;
      Alembic::Abc::IP2fArrayProperty * mP2fArrayProperty;
      Alembic::Abc::IP2dArrayProperty * mP2dArrayProperty;
      Alembic::Abc::IP3sArrayProperty * mP3sArrayProperty;
      Alembic::Abc::IP3iArrayProperty * mP3iArrayProperty;
      Alembic::Abc::IP3fArrayProperty * mP3fArrayProperty;
      Alembic::Abc::IP3dArrayProperty * mP3dArrayProperty;
      Alembic::Abc::IBox2sArrayProperty * mBox2sArrayProperty;
      Alembic::Abc::IBox2iArrayProperty * mBox2iArrayProperty;
      Alembic::Abc::IBox2fArrayProperty * mBox2fArrayProperty;
      Alembic::Abc::IBox2dArrayProperty * mBox2dArrayProperty;
      Alembic::Abc::IBox3sArrayProperty * mBox3sArrayProperty;
      Alembic::Abc::IBox3iArrayProperty * mBox3iArrayProperty;
      Alembic::Abc::IBox3fArrayProperty * mBox3fArrayProperty;
      Alembic::Abc::IBox3dArrayProperty * mBox3dArrayProperty;
      Alembic::Abc::IM33fArrayProperty * mM33fArrayProperty;
      Alembic::Abc::IM33dArrayProperty * mM33dArrayProperty;
      Alembic::Abc::IM44fArrayProperty * mM44fArrayProperty;
      Alembic::Abc::IM44dArrayProperty * mM44dArrayProperty;
      Alembic::Abc::IQuatfArrayProperty * mQuatfArrayProperty;
      Alembic::Abc::IQuatdArrayProperty * mQuatdArrayProperty;
      Alembic::Abc::IC3hArrayProperty * mC3hArrayProperty;
      Alembic::Abc::IC3fArrayProperty * mC3fArrayProperty;
      Alembic::Abc::IC3cArrayProperty * mC3cArrayProperty;
      Alembic::Abc::IC4hArrayProperty * mC4hArrayProperty;
      Alembic::Abc::IC4fArrayProperty * mC4fArrayProperty;
      Alembic::Abc::IC4cArrayProperty * mC4cArrayProperty;
      Alembic::Abc::IN2fArrayProperty * mN2fArrayProperty;
      Alembic::Abc::IN2dArrayProperty * mN2dArrayProperty;
      Alembic::Abc::IN3fArrayProperty * mN3fArrayProperty;
      Alembic::Abc::IN3dArrayProperty * mN3dArrayProperty;
   };
} iProperty;

// Two version of iProperty_new the first one is used by iCompoundProperty to create iProperty and also by the second version of the function
PyObject * iProperty_new(Alembic::Abc::ICompoundProperty &in_compound, char * in_propName);
PyObject * iProperty_new(Alembic::Abc::IObject in_Object, char * in_propName);

bool register_object_iProperty(PyObject *module);

#endif
