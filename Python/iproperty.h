#ifndef _PYTHON_ALEMBIC_IPROPERTY_H_
#define _PYTHON_ALEMBIC_IPROPERTY_H_

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
   int intent;       // NEW
   union {
      Abc::IScalarProperty * mBaseScalarProperty;
      Abc::IArrayProperty * mBaseArrayProperty;
      Abc::ICompoundProperty * mBaseCompoundProperty;
   };
   union {
      Abc::IBoolProperty * mBoolProperty;
      Abc::IUcharProperty * mUcharProperty;
      Abc::ICharProperty * mCharProperty;
      Abc::IUInt16Property * mUInt16Property;
      Abc::IInt16Property * mInt16Property;
      Abc::IUInt32Property * mUInt32Property;
      Abc::IInt32Property * mInt32Property;
      Abc::IUInt64Property * mUInt64Property;
      Abc::IInt64Property * mInt64Property;
      Abc::IHalfProperty * mHalfProperty;
      Abc::IFloatProperty * mFloatProperty;
      Abc::IDoubleProperty * mDoubleProperty;
      Abc::IStringProperty * mStringProperty;
      Abc::IWstringProperty * mWstringProperty;
      Abc::IV2sProperty * mV2sProperty;
      Abc::IV2iProperty * mV2iProperty;
      Abc::IV2fProperty * mV2fProperty;
      Abc::IV2dProperty * mV2dProperty;
      Abc::IV3sProperty * mV3sProperty;
      Abc::IV3iProperty * mV3iProperty;
      Abc::IV3fProperty * mV3fProperty;
      Abc::IV3dProperty * mV3dProperty;
      Abc::IP2sProperty * mP2sProperty;
      Abc::IP2iProperty * mP2iProperty;
      Abc::IP2fProperty * mP2fProperty;
      Abc::IP2dProperty * mP2dProperty;
      Abc::IP3sProperty * mP3sProperty;
      Abc::IP3iProperty * mP3iProperty;
      Abc::IP3fProperty * mP3fProperty;
      Abc::IP3dProperty * mP3dProperty;
      Abc::IBox2sProperty * mBox2sProperty;
      Abc::IBox2iProperty * mBox2iProperty;
      Abc::IBox2fProperty * mBox2fProperty;
      Abc::IBox2dProperty * mBox2dProperty;
      Abc::IBox3sProperty * mBox3sProperty;
      Abc::IBox3iProperty * mBox3iProperty;
      Abc::IBox3fProperty * mBox3fProperty;
      Abc::IBox3dProperty * mBox3dProperty;
      Abc::IM33fProperty * mM33fProperty;
      Abc::IM33dProperty * mM33dProperty;
      Abc::IM44fProperty * mM44fProperty;
      Abc::IM44dProperty * mM44dProperty;
      Abc::IQuatfProperty * mQuatfProperty;
      Abc::IQuatdProperty * mQuatdProperty;
      Abc::IC3hProperty * mC3hProperty;
      Abc::IC3fProperty * mC3fProperty;
      Abc::IC3cProperty * mC3cProperty;
      Abc::IC4hProperty * mC4hProperty;
      Abc::IC4fProperty * mC4fProperty;
      Abc::IC4cProperty * mC4cProperty;
      Abc::IN2fProperty * mN2fProperty;
      Abc::IN2dProperty * mN2dProperty;
      Abc::IN3fProperty * mN3fProperty;
      Abc::IN3dProperty * mN3dProperty;
      Abc::IBoolArrayProperty * mBoolArrayProperty;
      Abc::IUcharArrayProperty * mUcharArrayProperty;
      Abc::ICharArrayProperty * mCharArrayProperty;
      Abc::IUInt16ArrayProperty * mUInt16ArrayProperty;
      Abc::IInt16ArrayProperty * mInt16ArrayProperty;
      Abc::IUInt32ArrayProperty * mUInt32ArrayProperty;
      Abc::IInt32ArrayProperty * mInt32ArrayProperty;
      Abc::IUInt64ArrayProperty * mUInt64ArrayProperty;
      Abc::IInt64ArrayProperty * mInt64ArrayProperty;
      Abc::IHalfArrayProperty * mHalfArrayProperty;
      Abc::IFloatArrayProperty * mFloatArrayProperty;
      Abc::IDoubleArrayProperty * mDoubleArrayProperty;
      Abc::IStringArrayProperty * mStringArrayProperty;
      Abc::IWstringArrayProperty * mWstringArrayProperty;
      Abc::IV2sArrayProperty * mV2sArrayProperty;
      Abc::IV2iArrayProperty * mV2iArrayProperty;
      Abc::IV2fArrayProperty * mV2fArrayProperty;
      Abc::IV2dArrayProperty * mV2dArrayProperty;
      Abc::IV3sArrayProperty * mV3sArrayProperty;
      Abc::IV3iArrayProperty * mV3iArrayProperty;
      Abc::IV3fArrayProperty * mV3fArrayProperty;
      Abc::IV3dArrayProperty * mV3dArrayProperty;
      Abc::IP2sArrayProperty * mP2sArrayProperty;
      Abc::IP2iArrayProperty * mP2iArrayProperty;
      Abc::IP2fArrayProperty * mP2fArrayProperty;
      Abc::IP2dArrayProperty * mP2dArrayProperty;
      Abc::IP3sArrayProperty * mP3sArrayProperty;
      Abc::IP3iArrayProperty * mP3iArrayProperty;
      Abc::IP3fArrayProperty * mP3fArrayProperty;
      Abc::IP3dArrayProperty * mP3dArrayProperty;
      Abc::IBox2sArrayProperty * mBox2sArrayProperty;
      Abc::IBox2iArrayProperty * mBox2iArrayProperty;
      Abc::IBox2fArrayProperty * mBox2fArrayProperty;
      Abc::IBox2dArrayProperty * mBox2dArrayProperty;
      Abc::IBox3sArrayProperty * mBox3sArrayProperty;
      Abc::IBox3iArrayProperty * mBox3iArrayProperty;
      Abc::IBox3fArrayProperty * mBox3fArrayProperty;
      Abc::IBox3dArrayProperty * mBox3dArrayProperty;
      Abc::IM33fArrayProperty * mM33fArrayProperty;
      Abc::IM33dArrayProperty * mM33dArrayProperty;
      Abc::IM44fArrayProperty * mM44fArrayProperty;
      Abc::IM44dArrayProperty * mM44dArrayProperty;
      Abc::IQuatfArrayProperty * mQuatfArrayProperty;
      Abc::IQuatdArrayProperty * mQuatdArrayProperty;
      Abc::IC3hArrayProperty * mC3hArrayProperty;
      Abc::IC3fArrayProperty * mC3fArrayProperty;
      Abc::IC3cArrayProperty * mC3cArrayProperty;
      Abc::IC4hArrayProperty * mC4hArrayProperty;
      Abc::IC4fArrayProperty * mC4fArrayProperty;
      Abc::IC4cArrayProperty * mC4cArrayProperty;
      Abc::IN2fArrayProperty * mN2fArrayProperty;
      Abc::IN2dArrayProperty * mN2dArrayProperty;
      Abc::IN3fArrayProperty * mN3fArrayProperty;
      Abc::IN3dArrayProperty * mN3dArrayProperty;
   };
} iProperty;

// Two version of iProperty_new the first one is used by iCompoundProperty to create iProperty and also by the second version of the function
PyObject * iProperty_new(Abc::ICompoundProperty &in_compound, char * in_propName);
PyObject * iProperty_new(Abc::IObject in_Object, char * in_propName);

bool register_object_iProperty(PyObject *module);

#endif
