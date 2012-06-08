#include "extension.h"
#include "iproperty.h"
#include "icompoundproperty.h"   // to call iCompoundProperty_new in iProperty_new if it's an iCompoundProperty
#include "iobject.h"
#include "AlembicLicensing.h"

#ifdef __cplusplus__
extern "C"
{
#endif

static std::string iProperty_getName_func(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   iProperty * prop = (iProperty*)self;
   std::string name;
   if(prop->mIsArray)
      name = prop->mBaseArrayProperty->getName();
   else
      name = prop->mBaseScalarProperty->getName();
   return name;
   ALEMBIC_VALUE_CATCH_STATEMENT("")
}

static PyObject * iProperty_getName(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   return Py_BuildValue("s",iProperty_getName_func(self).c_str());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iProperty_getType(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iProperty * prop = (iProperty*)self;
   bool use_intent = false;

   std::string type;
   switch(prop->mPropType)
   {
      case propertyTP_unknown:
      {
         type.append("unknown");
         break;
      }
      case propertyTP_compound:		// TODO after debugging, remove because it should never be reached!
      {
         type.append("compound");
         break;
      }
      case propertyTP_boolean:
      case propertyTP_boolean_array:
      {
         type.append("bool");
         use_intent = true;
         break;
      }
      case propertyTP_uchar:
      case propertyTP_uchar_array:
      {
         type.append("uchar");
         use_intent = true;
         break;
      }
      case propertyTP_char:
      case propertyTP_char_array:
      {
         type.append("char");
         use_intent = true;
         break;
      }
      case propertyTP_uint16:
      case propertyTP_uint16_array:
      {
         type.append("uint16");
         use_intent = true;
         break;
      }
      case propertyTP_int16:
      case propertyTP_int16_array:
      {
         type.append("int16");
         use_intent = true;
         break;
      }
      case propertyTP_uint32:
      case propertyTP_uint32_array:
      {
         type.append("uint32");
         use_intent = true;
         break;
      }
      case propertyTP_int32:
      case propertyTP_int32_array:
      {
         type.append("int32");
         use_intent = true;
         break;
      }
      case propertyTP_uint64:
      case propertyTP_uint64_array:
      {
         type.append("uint64");
         use_intent = true;
         break;
      }
      case propertyTP_int64:
      case propertyTP_int64_array:
      {
         type.append("int64");
         use_intent = true;
         break;
      }
      case propertyTP_half:
      case propertyTP_half_array:
      {
         type.append("half");
         use_intent = true;
         break;
      }
      case propertyTP_float:
      case propertyTP_float_array:
      {
         type.append("float");
         use_intent = true;
         break;
      }
      case propertyTP_double:
      case propertyTP_double_array:
      {
         type.append("double");
         use_intent = true;
         break;
      }
      case propertyTP_string:
      case propertyTP_string_array:
      {
         type.append("string");
         break;
      }
      case propertyTP_wstring:
      case propertyTP_wstring_array:
      {
         type.append("wstring");
         break;
      }
      case propertyTP_v2s:
      case propertyTP_v2s_array:
      {
         type.append("vector2s");
         break;
      }
      case propertyTP_v2i:
      case propertyTP_v2i_array:
      {
         type.append("vector2i");
         break;
      }
      case propertyTP_v2f:
      case propertyTP_v2f_array:
      {
         type.append("vector2f");
         break;
      }
      case propertyTP_v2d:
      case propertyTP_v2d_array:
      {
         type.append("vector2d");
         break;
      }
      case propertyTP_v3s:
      case propertyTP_v3s_array:
      {
         type.append("vector3s");
         break;
      }
      case propertyTP_v3i:
      case propertyTP_v3i_array:
      {
         type.append("vector3i");
         break;
      }
      case propertyTP_v3f:
      case propertyTP_v3f_array:
      {
         type.append("vector3f");
         break;
      }
      case propertyTP_v3d:
      case propertyTP_v3d_array:
      {
         type.append("vector3d");
         break;
      }
      case propertyTP_p2s:
      case propertyTP_p2s_array:
      {
         type.append("point2s");
         break;
      }
      case propertyTP_p2i:
      case propertyTP_p2i_array:
      {
         type.append("point2i");
         break;
      }
      case propertyTP_p2f:
      case propertyTP_p2f_array:
      {
         type.append("point2f");
         break;
      }
      case propertyTP_p2d:
      case propertyTP_p2d_array:
      {
         type.append("point2d");
         break;
      }
      case propertyTP_p3s:
      case propertyTP_p3s_array:
      {
         type.append("point3s");
         break;
      }
      case propertyTP_p3i:
      case propertyTP_p3i_array:
      {
         type.append("point3i");
         break;
      }
      case propertyTP_p3f:
      case propertyTP_p3f_array:
      {
         type.append("point3f");
         break;
      }
      case propertyTP_p3d:
      case propertyTP_p3d_array:
      {
         type.append("point3d");
         break;
      }
      case propertyTP_box2s:
      case propertyTP_box2s_array:
      {
         type.append("box2s");
         break;
      }
      case propertyTP_box2i:
      case propertyTP_box2i_array:
      {
         type.append("box2i");
         break;
      }
      case propertyTP_box2f:
      case propertyTP_box2f_array:
      {
         type.append("box2f");
         break;
      }
      case propertyTP_box2d:
      case propertyTP_box2d_array:
      {
         type.append("box2d");
         break;
      }
      case propertyTP_box3s:
      case propertyTP_box3s_array:
      {
         type.append("box3s");
         break;
      }
      case propertyTP_box3i:
      case propertyTP_box3i_array:
      {
         type.append("box3i");
         break;
      }
      case propertyTP_box3f:
      case propertyTP_box3f_array:
      {
         type.append("box3f");
         break;
      }
      case propertyTP_box3d:
      case propertyTP_box3d_array:
      {
         type.append("box3d");
         break;
      }
      case propertyTP_m33f:
      case propertyTP_m33f_array:
      {
         type.append("matrix3f");
         break;
      }
      case propertyTP_m33d:
      case propertyTP_m33d_array:
      {
         type.append("matrix3d");
         break;
      }
      case propertyTP_quatf:
      case propertyTP_quatf_array:
      {
         type.append("quatf");
         break;
      }
      case propertyTP_quatd:
      case propertyTP_quatd_array:
      {
         type.append("quatd");
         break;
      }
      case propertyTP_m44f:
      case propertyTP_m44f_array:
      {
         type.append("matrix4f");
         break;
      }
      case propertyTP_m44d:
      case propertyTP_m44d_array:
      {
         type.append("matrix4d");
         break;
      }
      case propertyTP_c3h:
      case propertyTP_c3h_array:
      {
         type.append("color3h");
         break;
      }
      case propertyTP_c3f:
      case propertyTP_c3f_array:
      {
         type.append("color3f");
         break;
      }
      case propertyTP_c3c:
      case propertyTP_c3c_array:
      {
         type.append("color3c");
         break;
      }
      case propertyTP_c4h:
      case propertyTP_c4h_array:
      {
         type.append("color4h");
         break;
      }
      case propertyTP_c4f:
      case propertyTP_c4f_array:
      {
         type.append("color4f");
         break;
      }
      case propertyTP_c4c:
      case propertyTP_c4c_array:
      {
         type.append("color4c");
         break;
      }
      case propertyTP_n2f:
      case propertyTP_n2f_array:
      {
         type.append("normal2f");
         break;
      }
      case propertyTP_n2d:
      case propertyTP_n2d_array:
      {
         type.append("normal2d");
         break;
      }
      case propertyTP_n3f:
      case propertyTP_n3f_array:
      {
         type.append("normal3f");
         break;
      }
      case propertyTP_n3d:
      case propertyTP_n3d_array:
      {
         type.append("normal3d");
         break;
      }
      default:
         break;
   }

   if(prop->mIsArray)
      type.append("array");
   else if (use_intent && prop->intent > 1)
   {
      char sz_intent[9];
      sprintf(sz_intent, "[%d]", prop->intent);
      type.append(sz_intent);
   }
   return Py_BuildValue("s",type.c_str());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject * iProperty_getSampleTimes(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iProperty * prop = (iProperty*)self;
 
   Alembic::Abc::TimeSamplingPtr ts;
   if(prop->mIsArray)
      ts = prop->mBaseArrayProperty->getTimeSampling();
   else
      ts = prop->mBaseScalarProperty->getTimeSampling();

   if(ts)
   {
      const std::vector <Alembic::Abc::chrono_t> & times = ts->getStoredTimes();
      PyObject * tuple = PyTuple_New(times.size());
      for(size_t i=0;i<times.size();i++)
         PyTuple_SetItem(tuple,i,Py_BuildValue("f",(float)times[i]));
      return tuple;
   }
   return Py_BuildValue("s","unsupported");
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static size_t iProperty_getNbStoredSamples_func(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   iProperty * prop = (iProperty*)self;

   size_t numSamples = 0;
   if(prop->mIsArray)
      numSamples = prop->mBaseArrayProperty->getNumSamples();
   else
      numSamples = prop->mBaseScalarProperty->getNumSamples();
   return numSamples;
   ALEMBIC_VALUE_CATCH_STATEMENT(0)
}

static PyObject * iProperty_getNbStoredSamples(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   return Py_BuildValue("I",(unsigned int)iProperty_getNbStoredSamples_func(self));
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

#define _GET_SIZE_CASE_IMPL_(tp,base,arrayprop) \
   case tp: \
   { \
      Alembic::Abc::I##base##arrayprop::sample_ptr_type sample; \
      prop->m##base##arrayprop->get(sample,sampleIndex); \
      return Py_BuildValue("I",(unsigned int)sample->size()); \
   }
#define _GET_SIZE_CASE_(tp,base) _GET_SIZE_CASE_IMPL_(tp,base,ArrayProperty) 

static PyObject * iProperty_getSize(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iProperty * prop = (iProperty*)self;
   if(!prop->mIsArray)
      return Py_BuildValue("I",(unsigned int)1);

   unsigned long long sampleIndex = 0;
   PyArg_ParseTuple(args, "|K", &sampleIndex);

   size_t numSamples = iProperty_getNbStoredSamples_func(self);
   if(sampleIndex >= numSamples)
   {
      std::string msg;
      msg.append("SampleIndex for Property '");
      msg.append(iProperty_getName_func(self));
      msg.append("' is out of bounds!");
      PyErr_SetString(getError(), msg.c_str());
      return NULL;
   }

   std::string type;
   switch(prop->mPropType)
   {
      case propertyTP_unknown:
         return Py_BuildValue("I",(unsigned int)0);
      _GET_SIZE_CASE_(propertyTP_boolean_array,Bool)
      _GET_SIZE_CASE_(propertyTP_uchar_array,Uchar)
      _GET_SIZE_CASE_(propertyTP_char_array,Char)
      _GET_SIZE_CASE_(propertyTP_uint16_array,UInt16)
      _GET_SIZE_CASE_(propertyTP_int16_array,Int16)
      _GET_SIZE_CASE_(propertyTP_uint32_array,UInt32)
      _GET_SIZE_CASE_(propertyTP_int32_array,Int32)
      _GET_SIZE_CASE_(propertyTP_uint64_array,UInt64)
      _GET_SIZE_CASE_(propertyTP_int64_array,Int64)
      _GET_SIZE_CASE_(propertyTP_half_array,Half)
      _GET_SIZE_CASE_(propertyTP_float_array,Float)
      _GET_SIZE_CASE_(propertyTP_double_array,Double)
      _GET_SIZE_CASE_(propertyTP_string_array,String)
      _GET_SIZE_CASE_(propertyTP_wstring_array,Wstring)
      _GET_SIZE_CASE_(propertyTP_v2s_array,V2s)
      _GET_SIZE_CASE_(propertyTP_v2i_array,V2i)
      _GET_SIZE_CASE_(propertyTP_v2f_array,V2f)
      _GET_SIZE_CASE_(propertyTP_v2d_array,V2d)
      _GET_SIZE_CASE_(propertyTP_v3s_array,V3s)
      _GET_SIZE_CASE_(propertyTP_v3i_array,V3i)
      _GET_SIZE_CASE_(propertyTP_v3f_array,V3f)
      _GET_SIZE_CASE_(propertyTP_v3d_array,V3d)
      _GET_SIZE_CASE_(propertyTP_p2s_array,P2s)
      _GET_SIZE_CASE_(propertyTP_p2i_array,P2i)
      _GET_SIZE_CASE_(propertyTP_p2f_array,P2f)
      _GET_SIZE_CASE_(propertyTP_p2d_array,P2d)
      _GET_SIZE_CASE_(propertyTP_p3s_array,P3s)
      _GET_SIZE_CASE_(propertyTP_p3i_array,P3i)
      _GET_SIZE_CASE_(propertyTP_p3f_array,P3f)
      _GET_SIZE_CASE_(propertyTP_p3d_array,P3d)
      _GET_SIZE_CASE_(propertyTP_box2s_array,Box2s)
      _GET_SIZE_CASE_(propertyTP_box2i_array,Box2i)
      _GET_SIZE_CASE_(propertyTP_box2f_array,Box2f)
      _GET_SIZE_CASE_(propertyTP_box2d_array,Box2d)
      _GET_SIZE_CASE_(propertyTP_box3s_array,Box3s)
      _GET_SIZE_CASE_(propertyTP_box3i_array,Box3i)
      _GET_SIZE_CASE_(propertyTP_box3f_array,Box3f)
      _GET_SIZE_CASE_(propertyTP_box3d_array,Box3d)
      _GET_SIZE_CASE_(propertyTP_m33f_array,M33f)
      _GET_SIZE_CASE_(propertyTP_m33d_array,M33d)
      _GET_SIZE_CASE_(propertyTP_m44f_array,M44f)
      _GET_SIZE_CASE_(propertyTP_m44d_array,M44d)
      _GET_SIZE_CASE_(propertyTP_quatf_array,Quatf)
      _GET_SIZE_CASE_(propertyTP_quatd_array,Quatd)
      _GET_SIZE_CASE_(propertyTP_c3h_array,C3h)
      _GET_SIZE_CASE_(propertyTP_c3f_array,C3f)
      _GET_SIZE_CASE_(propertyTP_c3c_array,C3c)
      _GET_SIZE_CASE_(propertyTP_c4h_array,C4h)
      _GET_SIZE_CASE_(propertyTP_c4f_array,C4f)
      _GET_SIZE_CASE_(propertyTP_c4c_array,C4c)
      _GET_SIZE_CASE_(propertyTP_n2f_array,N2f)
      _GET_SIZE_CASE_(propertyTP_n2d_array,N2d)
      _GET_SIZE_CASE_(propertyTP_n3f_array,N3f)
      _GET_SIZE_CASE_(propertyTP_n3d_array,N3d)
      default:
         break;
   }
   return Py_BuildValue("I",(unsigned int)0);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

#define _GET_VALUE_INTENT(mXProperty, AlembicType, python_cast, cast_type) \
         if (prop->intent > 1) \
         { \
            tuple = PyTuple_New(prop->intent); \
            AlembicType *values = new AlembicType[prop->intent]; \
            prop->mBaseScalarProperty->get(values, sampleIndex); \
            for (int i = 0; i < prop->intent; ++i) \
               PyTuple_SetItem(tuple,i,Py_BuildValue(python_cast,(cast_type)values[i])); \
            delete [] values; \
         } \
         else \
         { \
            tuple = PyTuple_New(1); \
            AlembicType value; \
            prop->mXProperty->get(value,sampleIndex); \
            PyTuple_SetItem(tuple,0,Py_BuildValue(python_cast,(cast_type)value)); \
         }

static PyObject * iProperty_getValues(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   iProperty * prop = (iProperty*)self;
   if(prop->mPropType == propertyTP_unknown)
      return PyTuple_New(0);

   unsigned long long sampleIndex = 0;
   unsigned long long start = 0;
   unsigned long long count = 0;
   PyArg_ParseTuple(args, "|KKK", &sampleIndex,&start,&count);
   unsigned long long end = start + count;
   if(count == 0)
      end = ULLONG_MAX;
   unsigned long long offset = 0;

   size_t numSamples = iProperty_getNbStoredSamples_func(self);
   if(sampleIndex >= numSamples)
   {
      std::string msg;
      msg.append("SampleIndex for Property '");
      msg.append(iProperty_getName_func(self));
      msg.append("' is out of bounds!");
      PyErr_SetString(getError(), msg.c_str());
      return NULL;
   }

   if(!HasFullLicense())
   {
      if(sampleIndex > 75)
      {
         PyErr_SetString(getError(), "[ExocortexAlembic] Demo Mode: Max sampleindex is 75!");
         return NULL;
      }
   }

   PyObject * tuple = NULL;
   switch(prop->mPropType)
   {
      case propertyTP_boolean:
      {
         _GET_VALUE_INTENT(mBoolProperty, Alembic::Abc::bool_t, "i", bool);
         /*
         tuple = PyTuple_New(1);
         Alembic::Abc::bool_t value;
         prop->mBoolProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",value ? 1 : 0));
         */
         break;
      }
      case propertyTP_uchar:
      {
         _GET_VALUE_INTENT(mUcharProperty, unsigned char, "I", unsigned int);
         /*
         tuple = PyTuple_New(1);
         unsigned char value;
         prop->mUcharProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("I",(unsigned int)value));
         */
         break;
      }
      case propertyTP_char:
      {
         _GET_VALUE_INTENT(mCharProperty, signed char, "i", int);
         /*
         tuple = PyTuple_New(1);
         signed char value;
         prop->mCharProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value));
         */
         break;
      }
      case propertyTP_uint16:
      {
         _GET_VALUE_INTENT(mUInt16Property, Alembic::Abc::uint16_t, "I", unsigned int);
         /*
         tuple = PyTuple_New(1);
         Alembic::Abc::uint16_t value;
         prop->mUInt16Property->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("I",(unsigned int)value));
         */
         break;
      }
      case propertyTP_int16:
      {
         _GET_VALUE_INTENT(mInt16Property, Alembic::Abc::int16_t, "i", int);
         /*
         tuple = PyTuple_New(1);
         Alembic::Abc::int16_t value;
         prop->mInt16Property->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value));
         */
         break;
      }
      case propertyTP_uint32:
      {
         _GET_VALUE_INTENT(mUInt32Property, Alembic::Abc::uint32_t, "k", unsigned long);
         /*
         tuple = PyTuple_New(1);
         Alembic::Abc::uint32_t value;
         prop->mUInt32Property->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("k",(unsigned long)value));
         */
         break;
      }
      case propertyTP_int32:
      {
         _GET_VALUE_INTENT(mInt32Property, Alembic::Abc::int32_t, "l", long);
         /*
         tuple = PyTuple_New(1);
         Alembic::Abc::int32_t value;
         prop->mInt32Property->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("l",(long)value));
         */
         break;
      }
      case propertyTP_uint64:
      {
         _GET_VALUE_INTENT(mUInt64Property, Alembic::Abc::uint64_t, "K", unsigned long long);
         /*
         tuple = PyTuple_New(1);
         Alembic::Abc::uint64_t value;
         prop->mUInt64Property->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("K",(unsigned long long)value));
         */
         break;
      }
      case propertyTP_int64:
      {
         _GET_VALUE_INTENT(mInt64Property, Alembic::Abc::int64_t, "L", long long);
         /*
         tuple = PyTuple_New(1);
         Alembic::Abc::int64_t value;
         prop->mInt64Property->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("L",(long long)value));
         */
         break;
      }
      case propertyTP_half:
      {
         _GET_VALUE_INTENT(mHalfProperty, Alembic::Abc::float16_t, "f", float);
         /*
         tuple = PyTuple_New(1);
         Alembic::Abc::float16_t value;
         prop->mHalfProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",(float)value));
         */
         break;
      }
      case propertyTP_float:
      {
         _GET_VALUE_INTENT(mFloatProperty, Alembic::Abc::float32_t, "f", float);
         /*
         tuple = PyTuple_New(1);
         Alembic::Abc::float32_t value;
         prop->mFloatProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",(float)value));
         */
         break;
      }
      case propertyTP_double:
      {
         _GET_VALUE_INTENT(mDoubleProperty, Alembic::Abc::float64_t, "d", double);
         break;
      }
      case propertyTP_string:
      {
         tuple = PyTuple_New(1);
         std::string value;
         prop->mStringProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("s",value.c_str()));
         break;
      }
      case propertyTP_wstring:
      {
         tuple = PyTuple_New(1);
         std::wstring value;
         prop->mWstringProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("s",value.c_str()));
         break;
      }
      case propertyTP_v2s:
      {
         tuple = PyTuple_New(2);
         Alembic::Abc::IV2sProperty::value_type value;
         prop->mV2sProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.y));
         break;
      }
      case propertyTP_v2i:
      {
         tuple = PyTuple_New(2);
         Alembic::Abc::IV2iProperty::value_type value;
         prop->mV2iProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.y));
         break;
      }
      case propertyTP_v2f:
      {
         tuple = PyTuple_New(2);
         Alembic::Abc::IV2fProperty::value_type value;
         prop->mV2fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.y));
         break;
      }
      case propertyTP_v2d:
      {
         tuple = PyTuple_New(2);
         Alembic::Abc::IV2dProperty::value_type value;
         prop->mV2dProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.y));
         break;
      }
      case propertyTP_v3s:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IV3sProperty::value_type value;
         prop->mV3sProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("i",(int)value.z));
         break;
      }
      case propertyTP_v3i:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IV3iProperty::value_type value;
         prop->mV3iProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("i",(int)value.z));
         break;
      }
      case propertyTP_v3f:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IV3fProperty::value_type value;
         prop->mV3fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",value.z));
         break;
      }
      case propertyTP_v3d:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IV3dProperty::value_type value;
         prop->mV3dProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("d",value.z));
         break;
      }
      case propertyTP_p2s:
      {
         tuple = PyTuple_New(2);
         Alembic::Abc::IP2sProperty::value_type value;
         prop->mP2sProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.y));
         break;
      }
      case propertyTP_p2i:
      {
         tuple = PyTuple_New(2);
         Alembic::Abc::IP2iProperty::value_type value;
         prop->mP2iProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.y));
         break;
      }
      case propertyTP_p2f:
      {
         tuple = PyTuple_New(2);
         Alembic::Abc::IP2fProperty::value_type value;
         prop->mP2fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.y));
         break;
      }
      case propertyTP_p2d:
      {
         tuple = PyTuple_New(2);
         Alembic::Abc::IP2dProperty::value_type value;
         prop->mP2dProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.y));
         break;
      }
      case propertyTP_p3s:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IP3sProperty::value_type value;
         prop->mP3sProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("i",(int)value.z));
         break;
      }
      case propertyTP_p3i:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IP3iProperty::value_type value;
         prop->mP3iProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("i",(int)value.z));
         break;
      }
      case propertyTP_p3f:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IP3fProperty::value_type value;
         prop->mP3fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",value.z));
         break;
      }
      case propertyTP_p3d:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IP3dProperty::value_type value;
         prop->mP3dProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("d",value.z));
         break;
      }
      case propertyTP_box2s:
      {
         tuple = PyTuple_New(4);
         Alembic::Abc::IBox2sProperty::value_type value;
         prop->mBox2sProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.min.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.min.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("i",(int)value.max.x));
         PyTuple_SetItem(tuple,3,Py_BuildValue("i",(int)value.max.y));
         break;
      }
      case propertyTP_box2i:
      {
         tuple = PyTuple_New(4);
         Alembic::Abc::IBox2iProperty::value_type value;
         prop->mBox2iProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.min.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.min.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("i",(int)value.max.x));
         PyTuple_SetItem(tuple,3,Py_BuildValue("i",(int)value.max.y));
         break;
      }
      case propertyTP_box2f:
      {
         tuple = PyTuple_New(4);
         Alembic::Abc::IBox2fProperty::value_type value;
         prop->mBox2fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.min.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.min.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",value.max.x));
         PyTuple_SetItem(tuple,3,Py_BuildValue("f",value.max.y));
         break;
      }
      case propertyTP_box2d:
      {
         tuple = PyTuple_New(4);
         Alembic::Abc::IBox2dProperty::value_type value;
         prop->mBox2dProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.min.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.min.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("d",value.max.x));
         PyTuple_SetItem(tuple,3,Py_BuildValue("d",value.max.y));
         break;
      }
      case propertyTP_box3s:
      {
         tuple = PyTuple_New(6);
         Alembic::Abc::IBox3sProperty::value_type value;
         prop->mBox3sProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.min.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.min.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("i",(int)value.min.z));
         PyTuple_SetItem(tuple,3,Py_BuildValue("i",(int)value.max.x));
         PyTuple_SetItem(tuple,4,Py_BuildValue("i",(int)value.max.y));
         PyTuple_SetItem(tuple,5,Py_BuildValue("i",(int)value.max.z));
         break;
      }
      case propertyTP_box3i:
      {
         tuple = PyTuple_New(6);
         Alembic::Abc::IBox3iProperty::value_type value;
         prop->mBox3iProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("i",(int)value.min.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("i",(int)value.min.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("i",(int)value.min.z));
         PyTuple_SetItem(tuple,3,Py_BuildValue("i",(int)value.max.x));
         PyTuple_SetItem(tuple,4,Py_BuildValue("i",(int)value.max.y));
         PyTuple_SetItem(tuple,5,Py_BuildValue("i",(int)value.max.z));
         break;
      }
      case propertyTP_box3f:
      {
         tuple = PyTuple_New(6);
         Alembic::Abc::IBox3fProperty::value_type value;
         prop->mBox3fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.min.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.min.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",value.min.z));
         PyTuple_SetItem(tuple,3,Py_BuildValue("f",value.max.x));
         PyTuple_SetItem(tuple,4,Py_BuildValue("f",value.max.y));
         PyTuple_SetItem(tuple,5,Py_BuildValue("f",value.max.z));
         break;
      }
      case propertyTP_box3d:
      {
         tuple = PyTuple_New(6);
         Alembic::Abc::IBox3dProperty::value_type value;
         prop->mBox3dProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.min.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.min.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("d",value.min.z));
         PyTuple_SetItem(tuple,3,Py_BuildValue("d",value.max.x));
         PyTuple_SetItem(tuple,4,Py_BuildValue("d",value.max.y));
         PyTuple_SetItem(tuple,5,Py_BuildValue("d",value.max.z));
         break;
      }
      case propertyTP_m33f:
      {
         tuple = PyTuple_New(9);
         Alembic::Abc::IM33fProperty::value_type value;
         prop->mM33fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.x[0][0]));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.x[0][1]));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",value.x[0][2]));
         PyTuple_SetItem(tuple,3,Py_BuildValue("f",value.x[1][0]));
         PyTuple_SetItem(tuple,4,Py_BuildValue("f",value.x[1][1]));
         PyTuple_SetItem(tuple,5,Py_BuildValue("f",value.x[1][2]));
         PyTuple_SetItem(tuple,6,Py_BuildValue("f",value.x[2][0]));
         PyTuple_SetItem(tuple,7,Py_BuildValue("f",value.x[2][1]));
         PyTuple_SetItem(tuple,8,Py_BuildValue("f",value.x[2][2]));
         break;
      }
      case propertyTP_m33d:
      {
         tuple = PyTuple_New(9);
         Alembic::Abc::IM33dProperty::value_type value;
         prop->mM33dProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.x[0][0]));
         PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.x[0][1]));
         PyTuple_SetItem(tuple,2,Py_BuildValue("d",value.x[0][2]));
         PyTuple_SetItem(tuple,3,Py_BuildValue("d",value.x[1][0]));
         PyTuple_SetItem(tuple,4,Py_BuildValue("d",value.x[1][1]));
         PyTuple_SetItem(tuple,5,Py_BuildValue("d",value.x[1][2]));
         PyTuple_SetItem(tuple,6,Py_BuildValue("d",value.x[2][0]));
         PyTuple_SetItem(tuple,7,Py_BuildValue("d",value.x[2][1]));
         PyTuple_SetItem(tuple,8,Py_BuildValue("d",value.x[2][2]));
         break;
      }
      case propertyTP_m44f:
      {
         tuple = PyTuple_New(16);
         Alembic::Abc::IM44fProperty::value_type value;
         prop->mM44fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.x[0][0]));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.x[0][1]));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",value.x[0][2]));
         PyTuple_SetItem(tuple,3,Py_BuildValue("f",value.x[0][3]));
         PyTuple_SetItem(tuple,4,Py_BuildValue("f",value.x[1][0]));
         PyTuple_SetItem(tuple,5,Py_BuildValue("f",value.x[1][1]));
         PyTuple_SetItem(tuple,6,Py_BuildValue("f",value.x[1][2]));
         PyTuple_SetItem(tuple,7,Py_BuildValue("f",value.x[1][3]));
         PyTuple_SetItem(tuple,8,Py_BuildValue("f",value.x[2][0]));
         PyTuple_SetItem(tuple,9,Py_BuildValue("f",value.x[2][1]));
         PyTuple_SetItem(tuple,10,Py_BuildValue("f",value.x[2][2]));
         PyTuple_SetItem(tuple,11,Py_BuildValue("f",value.x[2][3]));
         PyTuple_SetItem(tuple,12,Py_BuildValue("f",value.x[3][0]));
         PyTuple_SetItem(tuple,13,Py_BuildValue("f",value.x[3][1]));
         PyTuple_SetItem(tuple,14,Py_BuildValue("f",value.x[3][2]));
         PyTuple_SetItem(tuple,15,Py_BuildValue("f",value.x[3][3]));
         break;
      }
      case propertyTP_m44d:
      {
         tuple = PyTuple_New(16);
         Alembic::Abc::IM44dProperty::value_type value;
         prop->mM44dProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.x[0][0]));
         PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.x[0][1]));
         PyTuple_SetItem(tuple,2,Py_BuildValue("d",value.x[0][2]));
         PyTuple_SetItem(tuple,3,Py_BuildValue("d",value.x[0][3]));
         PyTuple_SetItem(tuple,4,Py_BuildValue("d",value.x[1][0]));
         PyTuple_SetItem(tuple,5,Py_BuildValue("d",value.x[1][1]));
         PyTuple_SetItem(tuple,6,Py_BuildValue("d",value.x[1][2]));
         PyTuple_SetItem(tuple,7,Py_BuildValue("d",value.x[1][3]));
         PyTuple_SetItem(tuple,8,Py_BuildValue("d",value.x[2][0]));
         PyTuple_SetItem(tuple,9,Py_BuildValue("d",value.x[2][1]));
         PyTuple_SetItem(tuple,10,Py_BuildValue("d",value.x[2][2]));
         PyTuple_SetItem(tuple,11,Py_BuildValue("d",value.x[2][3]));
         PyTuple_SetItem(tuple,12,Py_BuildValue("d",value.x[3][0]));
         PyTuple_SetItem(tuple,13,Py_BuildValue("d",value.x[3][1]));
         PyTuple_SetItem(tuple,14,Py_BuildValue("d",value.x[3][2]));
         PyTuple_SetItem(tuple,15,Py_BuildValue("d",value.x[3][3]));
         break;
      }
      case propertyTP_quatf:
      {
         tuple = PyTuple_New(4);
         Alembic::Abc::IQuatfProperty::value_type value;
         prop->mQuatfProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.r));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.v.x));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",value.v.y));
         PyTuple_SetItem(tuple,3,Py_BuildValue("f",value.v.z));
         break;
      }
      case propertyTP_quatd:
      {
         tuple = PyTuple_New(4);
         Alembic::Abc::IQuatdProperty::value_type value;
         prop->mQuatdProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.r));
         PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.v.x));
         PyTuple_SetItem(tuple,2,Py_BuildValue("d",value.v.y));
         PyTuple_SetItem(tuple,3,Py_BuildValue("d",value.v.z));
         break;
      }
      case propertyTP_c3h:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IC3hProperty::value_type value;
         prop->mC3hProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",(float)value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",(float)value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",(float)value.z));
         break;
      }
      case propertyTP_c3f:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IC3fProperty::value_type value;
         prop->mC3fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",value.z));
         break;
      }
      case propertyTP_c3c:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IC3cProperty::value_type value;
         prop->mC3cProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("I",(unsigned int)value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("I",(unsigned int)value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("I",(unsigned int)value.z));
         break;
      }
      case propertyTP_c4h:
      {
         tuple = PyTuple_New(4);
         Alembic::Abc::IC4hProperty::value_type value;
         prop->mC4hProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",(float)value.r));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",(float)value.g));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",(float)value.b));
         PyTuple_SetItem(tuple,3,Py_BuildValue("f",(float)value.a));
         break;
      }
      case propertyTP_c4f:
      {
         tuple = PyTuple_New(4);
         Alembic::Abc::IC4fProperty::value_type value;
         prop->mC4fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.r));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.g));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",value.b));
         PyTuple_SetItem(tuple,3,Py_BuildValue("f",value.a));
         break;
      }
      case propertyTP_c4c:
      {
         tuple = PyTuple_New(4);
         Alembic::Abc::IC4cProperty::value_type value;
         prop->mC4cProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("I",(unsigned int)value.r));
         PyTuple_SetItem(tuple,1,Py_BuildValue("I",(unsigned int)value.g));
         PyTuple_SetItem(tuple,2,Py_BuildValue("I",(unsigned int)value.b));
         PyTuple_SetItem(tuple,3,Py_BuildValue("I",(unsigned int)value.a));
         break;
      }
      case propertyTP_n2f:
      {
         tuple = PyTuple_New(2);
         Alembic::Abc::IN2fProperty::value_type value;
         prop->mN2fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.y));
         break;
      }
      case propertyTP_n2d:
      {
         tuple = PyTuple_New(2);
         Alembic::Abc::IN2dProperty::value_type value;
         prop->mN2dProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.y));
         break;
      }
      case propertyTP_n3f:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IN3fProperty::value_type value;
         prop->mN3fProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("f",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("f",value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("f",value.z));
         break;
      }
      case propertyTP_n3d:
      {
         tuple = PyTuple_New(3);
         Alembic::Abc::IN3dProperty::value_type value;
         prop->mN3dProperty->get(value,sampleIndex);
         PyTuple_SetItem(tuple,0,Py_BuildValue("d",value.x));
         PyTuple_SetItem(tuple,1,Py_BuildValue("d",value.y));
         PyTuple_SetItem(tuple,2,Py_BuildValue("d",value.z));
         break;
      }
      case propertyTP_boolean_array:
      {
         Alembic::Abc::IBoolArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IBoolArrayProperty::value_type value;
         prop->mBoolArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",value ? 1 : 0));
            }
         }
         break;
      }
      case propertyTP_uchar_array:
      {
         Alembic::Abc::IUcharArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IUcharArrayProperty::value_type value;
         prop->mUcharArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("I",(unsigned int)value));
            }
         }
         break;
      }
      case propertyTP_char_array:
      {
         Alembic::Abc::ICharArrayProperty::sample_ptr_type sample;
         Alembic::Abc::ICharArrayProperty::value_type value;
         prop->mCharArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value));
            }
         }
         break;
      }
      case propertyTP_uint16_array:
      {
         Alembic::Abc::IUInt16ArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IUInt16ArrayProperty::value_type value;
         prop->mUInt16ArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("I",(unsigned int)value));
            }
         }
         break;
      }
      case propertyTP_int16_array:
      {
         Alembic::Abc::IInt16ArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IInt16ArrayProperty::value_type value;
         prop->mInt16ArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("I",(int)value));
            }
         }
         break;
      }
      case propertyTP_uint32_array:
      {
         Alembic::Abc::IUInt32ArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IUInt32ArrayProperty::value_type value;
         prop->mUInt32ArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("k",(unsigned long)value));
            }
         }
         break;
      }
      case propertyTP_int32_array:
      {
         Alembic::Abc::IInt32ArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IInt32ArrayProperty::value_type value;
         prop->mInt32ArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("l",(long)value));
            }
         }
         break;
      }
      case propertyTP_uint64_array:
      {
         Alembic::Abc::IUInt64ArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IUInt64ArrayProperty::value_type value;
         prop->mUInt64ArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("K",(unsigned long long)value));
            }
         }
         break;
      }
      case propertyTP_int64_array:
      {
         Alembic::Abc::IInt64ArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IInt64ArrayProperty::value_type value;
         prop->mInt64ArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("L",(long long)value));
            }
         }
         break;
      }
      case propertyTP_half_array:
      {
         Alembic::Abc::IHalfArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IHalfArrayProperty::value_type value;
         prop->mHalfArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",(float)value));
            }
         }
         break;
      }
      case propertyTP_float_array:
      {
         Alembic::Abc::IFloatArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IFloatArrayProperty::value_type value;
         prop->mFloatArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value));
            }
         }
         break;
      }
      case propertyTP_double_array:
      {
         Alembic::Abc::IDoubleArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IDoubleArrayProperty::value_type value;
         prop->mDoubleArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value));
            }
         }
         break;
      }
      case propertyTP_string_array:
      {
         Alembic::Abc::IStringArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IStringArrayProperty::value_type value;
         prop->mStringArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("s",value.c_str()));
            }
         }
         break;
      }
      case propertyTP_wstring_array:
      {
         Alembic::Abc::IWstringArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IWstringArrayProperty::value_type value;
         prop->mWstringArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New(end - start);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("s",value.c_str()));
            }
         }
         break;
      }
      case propertyTP_v2s_array:
      {
         Alembic::Abc::IV2sArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IV2sArrayProperty::value_type value;
         prop->mV2sArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 2);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.y));
            }
         }
         break;
      }
      case propertyTP_v2i_array:
      {
         Alembic::Abc::IV2iArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IV2iArrayProperty::value_type value;
         prop->mV2iArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 2);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.y));
            }
         }
         break;
      }
      case propertyTP_v2f_array:
      {
         Alembic::Abc::IV2fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IV2fArrayProperty::value_type value;
         prop->mV2fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 2);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.y));
            }
         }
         break;
      }
      case propertyTP_v2d_array:
      {
         Alembic::Abc::IV2dArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IV2dArrayProperty::value_type value;
         prop->mV2dArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 2);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.y));
            }
         }
         break;
      }
      case propertyTP_v3s_array:
      {
         Alembic::Abc::IV3sArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IV3sArrayProperty::value_type value;
         prop->mV3sArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.z));
            }
         }
         break;
      }
      case propertyTP_v3i_array:
      {
         Alembic::Abc::IV3iArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IV3iArrayProperty::value_type value;
         prop->mV3iArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.z));
            }
         }
         break;
      }
      case propertyTP_v3f_array:
      {
         Alembic::Abc::IV3fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IV3fArrayProperty::value_type value;
         prop->mV3fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.z));
            }
         }
         break;
      }
      case propertyTP_v3d_array:
      {
         Alembic::Abc::IV3dArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IV3dArrayProperty::value_type value;
         prop->mV3dArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.z));
            }
         }
         break;
      }
      case propertyTP_p2s_array:
      {
         Alembic::Abc::IP2sArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IP2sArrayProperty::value_type value;
         prop->mP2sArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 2);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.y));
            }
         }
         break;
      }
      case propertyTP_p2i_array:
      {
         Alembic::Abc::IP2iArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IP2iArrayProperty::value_type value;
         prop->mP2iArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 2);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.y));
            }
         }
         break;
      }
      case propertyTP_p2f_array:
      {
         Alembic::Abc::IP2fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IP2fArrayProperty::value_type value;
         prop->mP2fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 2);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.y));
            }
         }
         break;
      }
      case propertyTP_p2d_array:
      {
         Alembic::Abc::IP2dArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IP2dArrayProperty::value_type value;
         prop->mP2dArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 2);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.y));
            }
         }
         break;
      }
      case propertyTP_p3s_array:
      {
         Alembic::Abc::IP3sArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IP3sArrayProperty::value_type value;
         prop->mP3sArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.z));
            }
         }
         break;
      }
      case propertyTP_p3i_array:
      {
         Alembic::Abc::IP3iArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IP3iArrayProperty::value_type value;
         prop->mP3iArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.z));
            }
         }
         break;
      }
      case propertyTP_p3f_array:
      {
         Alembic::Abc::IP3fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IP3fArrayProperty::value_type value;
         prop->mP3fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.z));
            }
         }
         break;
      }
      case propertyTP_p3d_array:
      {
         Alembic::Abc::IP3dArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IP3dArrayProperty::value_type value;
         prop->mP3dArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.z));
            }
         }
         break;
      }
      case propertyTP_box2s_array:
      {
         Alembic::Abc::IBox2sArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IBox2sArrayProperty::value_type value;
         prop->mBox2sArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 4);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.min.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.min.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.max.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.max.y));
            }
         }
         break;
      }
      case propertyTP_box2i_array:
      {
         Alembic::Abc::IBox2iArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IBox2iArrayProperty::value_type value;
         prop->mBox2iArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 4);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.min.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.min.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.max.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.max.y));
            }
         }
         break;
      }
      case propertyTP_box2f_array:
      {
         Alembic::Abc::IBox2fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IBox2fArrayProperty::value_type value;
         prop->mBox2fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 4);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.min.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.min.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.max.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.max.y));
            }
         }
         break;
      }
      case propertyTP_box2d_array:
      {
         Alembic::Abc::IBox2dArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IBox2dArrayProperty::value_type value;
         prop->mBox2dArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 4);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.min.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.min.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.max.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.max.y));
            }
         }
         break;
      }
      case propertyTP_box3s_array:
      {
         Alembic::Abc::IBox3sArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IBox3sArrayProperty::value_type value;
         prop->mBox3sArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 6);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.min.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.min.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.min.z));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.max.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.max.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.max.z));
            }
         }
         break;
      }
      case propertyTP_box3i_array:
      {
         Alembic::Abc::IBox3iArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IBox3iArrayProperty::value_type value;
         prop->mBox3iArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 6);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.min.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.min.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.min.z));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.max.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.max.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("i",(int)value.max.z));
            }
         }
         break;
      }
      case propertyTP_box3f_array:
      {
         Alembic::Abc::IBox3fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IBox3fArrayProperty::value_type value;
         prop->mBox3fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 6);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.min.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.min.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.min.z));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.max.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.max.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.max.z));
            }
         }
         break;
      }
      case propertyTP_box3d_array:
      {
         Alembic::Abc::IBox3dArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IBox3dArrayProperty::value_type value;
         prop->mBox3dArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 6);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.min.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.min.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.min.z));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.max.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.max.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.max.z));
            }
         }
         break;
      }
      case propertyTP_m33f_array:
      {
         Alembic::Abc::IM33fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IM33fArrayProperty::value_type value;
         prop->mM33fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 9);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[0][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[0][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[0][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[1][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[1][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[1][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[2][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[2][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[2][2]));
            }
         }
         break;
      }
      case propertyTP_m33d_array:
      {
         Alembic::Abc::IM33dArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IM33dArrayProperty::value_type value;
         prop->mM33dArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 9);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[0][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[0][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[0][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[1][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[1][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[1][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[2][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[2][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[2][2]));
            }
         }
         break;
      }
      case propertyTP_m44f_array:
      {
         Alembic::Abc::IM44fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IM44fArrayProperty::value_type value;
         prop->mM44fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 16);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[0][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[0][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[0][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[0][3]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[1][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[1][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[1][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[1][3]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[2][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[2][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[2][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[2][3]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[3][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[3][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[3][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x[3][3]));
            }
         }
         break;
      }
      case propertyTP_m44d_array:
      {
         Alembic::Abc::IM44dArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IM44dArrayProperty::value_type value;
         prop->mM44dArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 16);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[0][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[0][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[0][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[0][3]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[1][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[1][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[1][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[1][3]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[2][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[2][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[2][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[2][3]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[3][0]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[3][1]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[3][2]));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x[3][3]));
            }
         }
         break;
      }
      case propertyTP_quatf_array:
      {
         Alembic::Abc::IQuatfArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IQuatfArrayProperty::value_type value;
         prop->mQuatfArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 4);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.r));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.v.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.v.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.v.z));
            }
         }
         break;
      }
      case propertyTP_quatd_array:
      {
         Alembic::Abc::IQuatdArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IQuatdArrayProperty::value_type value;
         prop->mQuatdArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 4);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.r));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.v.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.v.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.v.z));
            }
         }
         break;
      }
      case propertyTP_c3h_array:
      {
         Alembic::Abc::IC3hArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IC3hArrayProperty::value_type value;
         prop->mC3hArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",(float)value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",(float)value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",(float)value.z));
            }
         }
         break;
      }
      case propertyTP_c3f_array:
      {
         Alembic::Abc::IC3fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IC3fArrayProperty::value_type value;
         prop->mC3fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.z));
            }
         }
         break;
      }
      case propertyTP_c3c_array:
      {
         Alembic::Abc::IC3cArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IC3cArrayProperty::value_type value;
         prop->mC3cArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("I",(unsigned int)value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("I",(unsigned int)value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("I",(unsigned int)value.z));
            }
         }
         break;
      }
      case propertyTP_c4h_array:
      {
         Alembic::Abc::IC4hArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IC4hArrayProperty::value_type value;
         prop->mC4hArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 4);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",(float)value.r));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",(float)value.g));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",(float)value.b));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",(float)value.a));
            }
         }
         break;
      }
      case propertyTP_c4f_array:
      {
         Alembic::Abc::IC4fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IC4fArrayProperty::value_type value;
         prop->mC4fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 4);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.r));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.g));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.b));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.a));
            }
         }
         break;
      }
      case propertyTP_c4c_array:
      {
         Alembic::Abc::IC4cArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IC4cArrayProperty::value_type value;
         prop->mC4cArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 4);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("I",(unsigned int)value.r));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("I",(unsigned int)value.g));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("I",(unsigned int)value.b));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("I",(unsigned int)value.a));
            }
         }
         break;
      }
      case propertyTP_n2f_array:
      {
         Alembic::Abc::IN2fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IN2fArrayProperty::value_type value;
         prop->mN2fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 2);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.y));
            }
         }
         break;
      }
      case propertyTP_n2d_array:
      {
         Alembic::Abc::IN2dArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IN2dArrayProperty::value_type value;
         prop->mN2dArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 2);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.y));
            }
         }
         break;
      }
      case propertyTP_n3f_array:
      {
         Alembic::Abc::IN3fArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IN3fArrayProperty::value_type value;
         prop->mN3fArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("f",value.z));
            }
         }
         break;
      }
      case propertyTP_n3d_array:
      {
         Alembic::Abc::IN3dArrayProperty::sample_ptr_type sample;
         Alembic::Abc::IN3dArrayProperty::value_type value;
         prop->mN3dArrayProperty->get(sample,sampleIndex);
         if(start >= sample->size())
            tuple = PyTuple_New(0);
         else
         {
            if(end > sample->size())
               end = sample->size();
            tuple = PyTuple_New((end - start) * 3);
            for(unsigned long long i=start;i<end;i++)
            {
               value = sample->get()[i];
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.x));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.y));
               PyTuple_SetItem(tuple,offset++,Py_BuildValue("d",value.z));
            }
         }
         break;
      }
      default:
         break;
   }
   return tuple;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *iProperty_isCompound(PyObject *self, PyObject * args)
{
   Py_INCREF(Py_False);
   return Py_False;
}

static PyMethodDef iProperty_methods[] = {
   {"getName", (PyCFunction)iProperty_getName, METH_NOARGS, "Returns the name of this property."},
   {"getType", (PyCFunction)iProperty_getType, METH_NOARGS, "Returns the type of this property."},
   {"getSampleTimes", (PyCFunction)iProperty_getSampleTimes, METH_NOARGS, "Returns the TimeSampling this object is linked to."},
   {"getNbStoredSamples", (PyCFunction)iProperty_getNbStoredSamples, METH_NOARGS, "Returns the actual number of stored samples."},
   {"getSize", (PyCFunction)iProperty_getSize, METH_VARARGS, "Returns the size of the property. For single value properties, this method returns 1, for array value properties it returns the size of the array."},
   {"getValues", (PyCFunction)iProperty_getValues, METH_VARARGS, "Returns the values of the property at the (optional) sample index."},
   {"isCompound", (PyCFunction)iProperty_isCompound, METH_NOARGS, "To distinguish between an iProperty and an iCompoundProperty, always returns false for iProperty."},
   {NULL, NULL}
};

static PyObject * iProperty_getAttr(PyObject * self, char * attrName)
{
   ALEMBIC_TRY_STATEMENT
   return Py_FindMethod(iProperty_methods, self, attrName);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static void iProperty_delete(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   // delete the property
   iProperty * prop = (iProperty*)self;
   // first delete the base property
   if(prop->mIsArray)
      delete(prop->mBaseArrayProperty);
   else
      delete(prop->mBaseScalarProperty);
   // now delete the specialized one
   switch(prop->mPropType)
   {
      case propertyTP_boolean: { delete(prop->mBoolProperty); break; }
      case propertyTP_uchar: { delete(prop->mUcharProperty); break; }
      case propertyTP_char: { delete(prop->mCharProperty); break; }
      case propertyTP_uint16: { delete(prop->mUInt16Property); break; }
      case propertyTP_int16: { delete(prop->mInt16Property); break; }
      case propertyTP_uint32: { delete(prop->mUInt32Property); break; }
      case propertyTP_int32: { delete(prop->mInt32Property); break; }
      case propertyTP_uint64: { delete(prop->mUInt64Property); break; }
      case propertyTP_int64: { delete(prop->mInt64Property); break; }
      case propertyTP_half: { delete(prop->mHalfProperty); break; }
      case propertyTP_float: { delete(prop->mFloatProperty); break; }
      case propertyTP_double: { delete(prop->mDoubleProperty); break; }
      case propertyTP_string: { delete(prop->mStringProperty); break; }
      case propertyTP_wstring: { delete(prop->mWstringProperty); break; }
      case propertyTP_v2s: { delete(prop->mV2sProperty); break; }
      case propertyTP_v2i: { delete(prop->mV2iProperty); break; }
      case propertyTP_v2f: { delete(prop->mV2fProperty); break; }
      case propertyTP_v2d: { delete(prop->mV2dProperty); break; }
      case propertyTP_v3s: { delete(prop->mV3sProperty); break; }
      case propertyTP_v3i: { delete(prop->mV3iProperty); break; }
      case propertyTP_v3f: { delete(prop->mV3fProperty); break; }
      case propertyTP_v3d: { delete(prop->mV3dProperty); break; }
      case propertyTP_p2s: { delete(prop->mP2sProperty); break; }
      case propertyTP_p2i: { delete(prop->mP2iProperty); break; }
      case propertyTP_p2f: { delete(prop->mP2fProperty); break; }
      case propertyTP_p2d: { delete(prop->mP2dProperty); break; }
      case propertyTP_p3s: { delete(prop->mP3sProperty); break; }
      case propertyTP_p3i: { delete(prop->mP3iProperty); break; }
      case propertyTP_p3f: { delete(prop->mP3fProperty); break; }
      case propertyTP_p3d: { delete(prop->mP3dProperty); break; }
      case propertyTP_box2s: { delete(prop->mBox2sProperty); break; }
      case propertyTP_box2i: { delete(prop->mBox2iProperty); break; }
      case propertyTP_box2f: { delete(prop->mBox2fProperty); break; }
      case propertyTP_box2d: { delete(prop->mBox2dProperty); break; }
      case propertyTP_box3s: { delete(prop->mBox3sProperty); break; }
      case propertyTP_box3i: { delete(prop->mBox3iProperty); break; }
      case propertyTP_box3f: { delete(prop->mBox3fProperty); break; }
      case propertyTP_box3d: { delete(prop->mBox3dProperty); break; }
      case propertyTP_m33f: { delete(prop->mM33fProperty); break; }
      case propertyTP_m33d: { delete(prop->mM33dProperty); break; }
      case propertyTP_m44f: { delete(prop->mM44fProperty); break; }
      case propertyTP_m44d: { delete(prop->mM44dProperty); break; }
      case propertyTP_quatf: { delete(prop->mQuatfProperty); break; }
      case propertyTP_quatd: { delete(prop->mQuatdProperty); break; }
      case propertyTP_c3h: { delete(prop->mC3hProperty); break; }
      case propertyTP_c3f: { delete(prop->mC3fProperty); break; }
      case propertyTP_c3c: { delete(prop->mC3cProperty); break; }
      case propertyTP_c4h: { delete(prop->mC4hProperty); break; }
      case propertyTP_c4f: { delete(prop->mC4fProperty); break; }
      case propertyTP_c4c: { delete(prop->mC4cProperty); break; }
      case propertyTP_n2f: { delete(prop->mN2fProperty); break; }
      case propertyTP_n2d: { delete(prop->mN2dProperty); break; }
      case propertyTP_n3f: { delete(prop->mN3fProperty); break; }
      case propertyTP_n3d: { delete(prop->mN3dProperty); break; }
      case propertyTP_boolean_array: { delete(prop->mBoolArrayProperty); break; }
      case propertyTP_uchar_array: { delete(prop->mUcharArrayProperty); break; }
      case propertyTP_char_array: { delete(prop->mCharArrayProperty); break; }
      case propertyTP_uint16_array: { delete(prop->mUInt16ArrayProperty); break; }
      case propertyTP_int16_array: { delete(prop->mInt16ArrayProperty); break; }
      case propertyTP_uint32_array: { delete(prop->mUInt32ArrayProperty); break; }
      case propertyTP_int32_array: { delete(prop->mInt32ArrayProperty); break; }
      case propertyTP_uint64_array: { delete(prop->mUInt64ArrayProperty); break; }
      case propertyTP_int64_array: { delete(prop->mInt64ArrayProperty); break; }
      case propertyTP_half_array: { delete(prop->mHalfArrayProperty); break; }
      case propertyTP_float_array: { delete(prop->mFloatArrayProperty); break; }
      case propertyTP_double_array: { delete(prop->mDoubleArrayProperty); break; }
      case propertyTP_string_array: { delete(prop->mStringArrayProperty); break; }
      case propertyTP_wstring_array: { delete(prop->mWstringArrayProperty); break; }
      case propertyTP_v2s_array: { delete(prop->mV2sArrayProperty); break; }
      case propertyTP_v2i_array: { delete(prop->mV2iArrayProperty); break; }
      case propertyTP_v2f_array: { delete(prop->mV2fArrayProperty); break; }
      case propertyTP_v2d_array: { delete(prop->mV2dArrayProperty); break; }
      case propertyTP_v3s_array: { delete(prop->mV3sArrayProperty); break; }
      case propertyTP_v3i_array: { delete(prop->mV3iArrayProperty); break; }
      case propertyTP_v3f_array: { delete(prop->mV3fArrayProperty); break; }
      case propertyTP_v3d_array: { delete(prop->mV3dArrayProperty); break; }
      case propertyTP_p2s_array: { delete(prop->mP2sArrayProperty); break; }
      case propertyTP_p2i_array: { delete(prop->mP2iArrayProperty); break; }
      case propertyTP_p2f_array: { delete(prop->mP2fArrayProperty); break; }
      case propertyTP_p2d_array: { delete(prop->mP2dArrayProperty); break; }
      case propertyTP_p3s_array: { delete(prop->mP3sArrayProperty); break; }
      case propertyTP_p3i_array: { delete(prop->mP3iArrayProperty); break; }
      case propertyTP_p3f_array: { delete(prop->mP3fArrayProperty); break; }
      case propertyTP_p3d_array: { delete(prop->mP3dArrayProperty); break; }
      case propertyTP_box2s_array: { delete(prop->mBox2sArrayProperty); break; }
      case propertyTP_box2i_array: { delete(prop->mBox2iArrayProperty); break; }
      case propertyTP_box2f_array: { delete(prop->mBox2fArrayProperty); break; }
      case propertyTP_box2d_array: { delete(prop->mBox2dArrayProperty); break; }
      case propertyTP_box3s_array: { delete(prop->mBox3sArrayProperty); break; }
      case propertyTP_box3i_array: { delete(prop->mBox3iArrayProperty); break; }
      case propertyTP_box3f_array: { delete(prop->mBox3fArrayProperty); break; }
      case propertyTP_box3d_array: { delete(prop->mBox3dArrayProperty); break; }
      case propertyTP_m33f_array: { delete(prop->mM33fArrayProperty); break; }
      case propertyTP_m33d_array: { delete(prop->mM33dArrayProperty); break; }
      case propertyTP_m44f_array: { delete(prop->mM44fArrayProperty); break; }
      case propertyTP_m44d_array: { delete(prop->mM44dArrayProperty); break; }
      case propertyTP_quatf_array: { delete(prop->mQuatfArrayProperty); break; }
      case propertyTP_quatd_array: { delete(prop->mQuatdArrayProperty); break; }
      case propertyTP_c3h_array: { delete(prop->mC3hArrayProperty); break; }
      case propertyTP_c3f_array: { delete(prop->mC3fArrayProperty); break; }
      case propertyTP_c3c_array: { delete(prop->mC3cArrayProperty); break; }
      case propertyTP_c4h_array: { delete(prop->mC4hArrayProperty); break; }
      case propertyTP_c4f_array: { delete(prop->mC4fArrayProperty); break; }
      case propertyTP_c4c_array: { delete(prop->mC4cArrayProperty); break; }
      case propertyTP_n2f_array: { delete(prop->mN2fArrayProperty); break; }
      case propertyTP_n2d_array: { delete(prop->mN2dArrayProperty); break; }
      case propertyTP_n3f_array: { delete(prop->mN3fArrayProperty); break; }
      case propertyTP_n3d_array: { delete(prop->mN3dArrayProperty); break; }
      default:
         break;
   }

   PyObject_FREE(prop);
   ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject iProperty_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                                // op_size
  "iProperty",                      // tp_name
  sizeof(iProperty),                // tp_basicsize
  0,                                // tp_itemsize
  (destructor)iProperty_delete,     // tp_dealloc
  0,                                // tp_print
  (getattrfunc)iProperty_getAttr,   // tp_getattr
  0,                                // tp_setattr
  0,                                // tp_compare
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "This is the input property. It provides access to the propery's data, such as name, type and per sample values.",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  iProperty_methods,             /* tp_methods */
};

#ifdef __cplusplus__
}
#endif

#define _NEW_PROP_IMPL_(tp,base,singleprop,arrayprop) \
   if(prop->mIsArray) \
   { \
      prop->mPropType = (propertyTP)(tp - propertyTP_boolean + propertyTP_boolean_array); \
      prop->m##base##arrayprop = new Alembic::Abc::I##base##arrayprop(compound,in_propName); \
   } \
   else \
   { \
      prop->mPropType = tp; \
      prop->m##base##singleprop = new Alembic::Abc::I##base##singleprop(compound,in_propName); \
   }
#define _NEW_PROP_(tp,base) _NEW_PROP_IMPL_(tp,base,Property,ArrayProperty)
#define _NEW_PROP_CASE_IMPL_(pod,tp,base,singleprop,arrayprop) \
   case Alembic::Abc::pod: \
   { \
      _NEW_PROP_IMPL_(tp,base,singleprop,arrayprop) \
      break; \
   }
#define _NEW_PROP_CASE_(pod,tp,base) _NEW_PROP_CASE_IMPL_(pod,tp,base,Property,ArrayProperty)

PyObject * iProperty_new(Alembic::Abc::ICompoundProperty &compound, char * in_propName)
{
   const Alembic::Abc::PropertyHeader * propHeader = compound.getPropertyHeader( in_propName );
   iProperty * prop = PyObject_NEW(iProperty, &iProperty_Type);
   //INFO_MSG(in_propName << " of type " << propHeader->getDataType());
   if (prop != NULL)
   {
      if(propHeader->isCompound())
      {
         PyObject_FREE(prop);    // Free it because one will be created in iCompoundProperty_new
         return iCompoundProperty_new(compound, in_propName);
      }
      else
      {
         prop->mIsArray = propHeader->isArray();
         prop->intent = propHeader->getDataType().getExtent();    // NEW
         std::string interpretation;
         if(prop->mIsArray)
         {
            prop->intent = 0;
            prop->mBaseArrayProperty = new Alembic::Abc::IArrayProperty(compound,in_propName);
            interpretation = prop->mBaseArrayProperty->getMetaData().get("interpretation");
         }
         else
         {
            prop->mBaseScalarProperty = new Alembic::Abc::IScalarProperty(compound,in_propName);
            interpretation = prop->mBaseScalarProperty->getMetaData().get("interpretation");

            Alembic::Abc::MetaData md = prop->mBaseScalarProperty->getMetaData();
         }

         if(interpretation.empty())
         {
            // check for custom structs
            if(prop->intent > 1)
               interpretation = "intent_data";  // NEW, previously unknown
         }

         if(interpretation.empty() || interpretation == "intent_data")
         {
            switch(propHeader->getDataType().getPod())
            {
               _NEW_PROP_CASE_(kBooleanPOD,propertyTP_boolean,Bool)
               _NEW_PROP_CASE_(kUint8POD,propertyTP_uchar,Uchar)
               _NEW_PROP_CASE_(kInt8POD,propertyTP_char,Char)
               _NEW_PROP_CASE_(kUint16POD,propertyTP_uint16,UInt16)
               _NEW_PROP_CASE_(kInt16POD,propertyTP_int16,Int16)
               _NEW_PROP_CASE_(kUint32POD,propertyTP_uint32,UInt32)
               _NEW_PROP_CASE_(kInt32POD,propertyTP_int32,Int32)
               _NEW_PROP_CASE_(kUint64POD,propertyTP_uint64,UInt64)
               _NEW_PROP_CASE_(kInt64POD,propertyTP_int64,Int64)
               _NEW_PROP_CASE_(kFloat16POD,propertyTP_half,Half)
               _NEW_PROP_CASE_(kFloat32POD,propertyTP_float,Float)
               _NEW_PROP_CASE_(kFloat64POD,propertyTP_double,Double)
               _NEW_PROP_CASE_(kStringPOD,propertyTP_string,String)
               _NEW_PROP_CASE_(kWstringPOD,propertyTP_string,Wstring)
               default:
                  break;
            }
         }
         else if(interpretation == "vector")
         {
            if(propHeader->getDataType().getExtent() == 2)
            {
               switch(propHeader->getDataType().getPod())
               {
                  _NEW_PROP_CASE_(kInt16POD,propertyTP_v2s,V2s)
                  _NEW_PROP_CASE_(kInt32POD,propertyTP_v2i,V2i)
                  _NEW_PROP_CASE_(kFloat32POD,propertyTP_v2f,V2f)
                  _NEW_PROP_CASE_(kFloat64POD,propertyTP_v2d,V2d)
                  default:
                     break;
               }
            }
            else if(propHeader->getDataType().getExtent() == 3)
            {
               switch(propHeader->getDataType().getPod())
               {
                  _NEW_PROP_CASE_(kInt16POD,propertyTP_v3s,V3s)
                  _NEW_PROP_CASE_(kInt32POD,propertyTP_v3i,V3i)
                  _NEW_PROP_CASE_(kFloat32POD,propertyTP_v3f,V3f)
                  _NEW_PROP_CASE_(kFloat64POD,propertyTP_v3d,V3d)
                  default:
                     break;
               }
            }
            else
            {
               PyErr_SetString(getError(), "Invalid extend for vector property.");
               return NULL;
            }
         }
         else if(interpretation == "point")
         {
            if(propHeader->getDataType().getExtent() == 2)
            {
               switch(propHeader->getDataType().getPod())
               {
                  _NEW_PROP_CASE_(kInt16POD,propertyTP_p2s,P2s)
                  _NEW_PROP_CASE_(kInt32POD,propertyTP_p2i,P2i)
                  _NEW_PROP_CASE_(kFloat32POD,propertyTP_p2f,P2f)
                  _NEW_PROP_CASE_(kFloat64POD,propertyTP_p2d,P2d)
                  default:
                     break;
               }
            }
            else if(propHeader->getDataType().getExtent() == 3)
            {
               switch(propHeader->getDataType().getPod())
               {
                  _NEW_PROP_CASE_(kInt16POD,propertyTP_p3s,P3s)
                  _NEW_PROP_CASE_(kInt32POD,propertyTP_p3i,P3i)
                  _NEW_PROP_CASE_(kFloat32POD,propertyTP_p3f,P3f)
                  _NEW_PROP_CASE_(kFloat64POD,propertyTP_p3d,P3d)
                  default:
                     break;
               }
            }
            else
            {
               PyErr_SetString(getError(), "Invalid extend for point property.");
               return NULL;
            }
         }
         else if(interpretation == "box")
         {
            if(propHeader->getDataType().getExtent() == 4)
            {
               switch(propHeader->getDataType().getPod())
               {
                  _NEW_PROP_CASE_(kInt16POD,propertyTP_box2s,Box2s)
                  _NEW_PROP_CASE_(kInt32POD,propertyTP_box2i,Box2i)
                  _NEW_PROP_CASE_(kFloat32POD,propertyTP_box2f,Box2f)
                  _NEW_PROP_CASE_(kFloat64POD,propertyTP_box2d,Box2d)
                  default:
                     break;
               }
            }
            else if(propHeader->getDataType().getExtent() == 6)
            {
               switch(propHeader->getDataType().getPod())
               {
                  _NEW_PROP_CASE_(kInt16POD,propertyTP_box3s,Box3s)
                  _NEW_PROP_CASE_(kInt32POD,propertyTP_box3i,Box3i)
                  _NEW_PROP_CASE_(kFloat32POD,propertyTP_box3f,Box3f)
                  _NEW_PROP_CASE_(kFloat64POD,propertyTP_box3d,Box3d)
                  default:
                     break;
               }
            }
            else
            {
               PyErr_SetString(getError(), "Invalid extend for box property.");
               return NULL;
            }
         }
         else if(interpretation == "matrix")
         {
            if(propHeader->getDataType().getExtent() == 9)
            {
               switch(propHeader->getDataType().getPod())
               {
                  _NEW_PROP_CASE_(kFloat32POD,propertyTP_m33f,M33f)
                  _NEW_PROP_CASE_(kFloat64POD,propertyTP_m33d,M33d)
                  default:
                     break;
               }
            }
            else if(propHeader->getDataType().getExtent() == 16)
            {
               switch(propHeader->getDataType().getPod())
               {
                  _NEW_PROP_CASE_(kFloat32POD,propertyTP_m44f,M44f)
                  _NEW_PROP_CASE_(kFloat64POD,propertyTP_m44d,M44d)
                  default:
                     break;
               }
            }
            else
            {
               PyErr_SetString(getError(), "Invalid extend for matrix property.");
               return NULL;
            }
         }
         else if(interpretation == "quat")
         {
            switch(propHeader->getDataType().getPod())
            {
               _NEW_PROP_CASE_(kFloat32POD,propertyTP_quatf,Quatf)
               _NEW_PROP_CASE_(kFloat64POD,propertyTP_quatd,Quatd)
               default:
                  break;
            }
         }
         else if(interpretation == "rgb")
         {
            switch(propHeader->getDataType().getPod())
            {
               _NEW_PROP_CASE_(kFloat16POD,propertyTP_c3h,C3h)
               _NEW_PROP_CASE_(kFloat32POD,propertyTP_c3f,C3f)
               _NEW_PROP_CASE_(kUint8POD,propertyTP_c3c,C3c)
               default:
                  break;
            }
         }
         else if(interpretation == "rgba")
         {
            switch(propHeader->getDataType().getPod())
            {
               _NEW_PROP_CASE_(kFloat16POD,propertyTP_c4h,C4h)
               _NEW_PROP_CASE_(kFloat32POD,propertyTP_c4f,C4f)
               _NEW_PROP_CASE_(kUint8POD,propertyTP_c4c,C4c)
               default:
                  break;
            }
         }
         else if(interpretation == "normal")
         {
            if(propHeader->getDataType().getExtent() == 2)
            {
               switch(propHeader->getDataType().getPod())
               {
                  _NEW_PROP_CASE_(kFloat32POD,propertyTP_n2f,N2f)
                  _NEW_PROP_CASE_(kFloat64POD,propertyTP_n2d,N2d)
                  default:
                     break;
               }
            }
            else if(propHeader->getDataType().getExtent() == 3)
            {
               switch(propHeader->getDataType().getPod())
               {
                  _NEW_PROP_CASE_(kFloat32POD,propertyTP_n3f,N3f)
                  _NEW_PROP_CASE_(kFloat64POD,propertyTP_n3d,N3d)
                  default:
                     break;
               }
            }
            else
            {
               PyErr_SetString(getError(), "Invalid extend for normal property.");
               return NULL;
            }
         }
         else if(interpretation == "unknown")
         {
            prop->mPropType = propertyTP_unknown;
         }
      }
   }
   return (PyObject *)prop;
}

PyObject * iProperty_new(Alembic::Abc::IObject in_Object, char * in_propName)
{
   ALEMBIC_TRY_STATEMENT
   Alembic::Abc::ICompoundProperty compound = getCompoundFromIObject(in_Object);
   if (compound.getPropertyHeader( in_propName ) == NULL )
   {
      std::string msg;
      msg.append("Property '");
      msg.append(in_propName);
      msg.append("' not found on '");
      msg.append(in_Object.getFullName());
      msg.append("'!");
      PyErr_SetString(getError(), msg.c_str());
      return NULL;
   }
   return iProperty_new(compound, in_propName);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool register_object_iProperty(PyObject *module)
{
  return register_object(module, iProperty_Type, "iProperty");
}


