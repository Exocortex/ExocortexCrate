#include "extension.h"
#include "oproperty.h"
#include "ocompoundproperty.h"
#include "oobject.h"
#include "oarchive.h"
#include <boost/lexical_cast.hpp>
#include "AlembicLicensing.h"

static std::string oProperty_getName_func(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   oProperty * prop = (oProperty*)self;
   if(prop->mArchive == NULL)
   {
      PyErr_SetString(getError(), "Archive already closed!");
      return NULL;
   }
   std::string name;
   if(prop->mIsCompound)
      name = prop->mBaseCompoundProperty->getName();
   else if(prop->mIsArray)
      name = prop->mBaseArrayProperty->getName();
   else
      name = prop->mBaseScalarProperty->getName();
   return name;
   ALEMBIC_VALUE_CATCH_STATEMENT("")
}

static PyObject * oProperty_getName(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   oProperty * prop = (oProperty*)self;
   if(prop->mArchive == NULL)
   {
      PyErr_SetString(getError(), "Archive already closed!");
      return NULL;
   }
   return Py_BuildValue("s",oProperty_getName_func(self).c_str());
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

#define _CHECK_NUMBER_ITEMS_(expected) \
   if(nbItems != expected) \
   { \
      std::string error; \
      error.append("Incorrect number of sample items, should be "); \
      error.append(#expected); \
      error.append("!"); \
      PyErr_SetString(getError(), error.c_str()); \
      return NULL; \
   }
#define _CHECK_MODULO_NUMBER_ITEMS_(modulo) \
   if(nbItems % modulo) \
   { \
      std::string error; \
      error.append("Incorrect number of sample items, should be a multiple of "); \
      error.append(#modulo); \
      error.append("!"); \
      PyErr_SetString(getError(), error.c_str()); \
      return NULL; \
   }
#define _COPY_TUPLE_TO_VALUE_(ctype,format,expected) \
   _CHECK_NUMBER_ITEMS_(expected) \
   std::vector<ctype> tupleVec(expected); \
   for(size_t i=0;i<expected;i++) \
   { \
      PyObject * item = NULL; \
      if(PyTuple_Check(tuple)) \
         item = PyTuple_GetItem(tuple,i); \
      else \
         item = PyList_GetItem(tuple,i); \
      if(!PyArg_Parse(item,format,&tupleVec[i])) \
      { \
         std::string error; \
         error.append("Some item of the sample tuple is not of the right type, should be '"); \
         error.append(#ctype); \
         error.append("'!"); \
         PyErr_SetString(getError(), error.c_str()); \
         return NULL; \
      } \
   }
#define _COPY_TUPLE_TO_VECTOR_(ctype,ptype,format,modulo) \
   _CHECK_MODULO_NUMBER_ITEMS_(modulo) \
   std::vector<ctype> tupleVec(nbItems); \
   ptype tupleValue; \
   for(size_t i=0;i<nbItems;i++) \
   { \
      PyObject * item = NULL; \
      if(PyTuple_Check(tuple)) \
         item = PyTuple_GetItem(tuple,i); \
      else \
         item = PyList_GetItem(tuple,i); \
      if(!PyArg_Parse(item,format,&tupleValue)) \
      { \
         std::string error; \
         error.append("Some item of the sample tuple is not of the right type, should be '"); \
         error.append(#ptype); \
         error.append("'!"); \
         PyErr_SetString(getError(), error.c_str()); \
         return NULL; \
      } \
      tupleVec[i] = (ctype)tupleValue; \
   }
#define _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(stype,ctype,ptype,format,modulo) \
   _COPY_TUPLE_TO_VECTOR_(ctype,ptype,format,modulo); \
   std::vector<stype> values(nbItems/modulo); \
   memcpy(&values.front(),&tupleVec.front(),tupleVec.size() * sizeof(ctype));

static PyObject * oProperty_setValues(PyObject * self, PyObject * args)
{
   ALEMBIC_TRY_STATEMENT
   oProperty * prop = (oProperty*)self;
   if(prop->mArchive == NULL)
   {
      PyErr_SetString(getError(), "Archive already closed!");
      return NULL;
   }
   if(prop->mIsCompound)
   {
      PyErr_SetString(getError(), "Cannot store a sample on a compound property!");
      return NULL;
   }
   if(prop->mPropType == propertyTP_unknown)
   {
      PyErr_SetString(getError(), "Unknown property type!");
      return NULL;
   }
   long numSamples = 0;
   if(prop->mIsArray)
   {
      numSamples = (long)prop->mBaseArrayProperty->getNumSamples();
      if(numSamples >= prop->mBaseArrayProperty->getTimeSampling()->getNumStoredTimes())
      {
         PyErr_SetString(getError(), "Already stored the maximum number of samples!");
         return NULL;
      }
   }
   else
   {
      numSamples = (long)prop->mBaseScalarProperty->getNumSamples();
      if(numSamples >= prop->mBaseScalarProperty->getTimeSampling()->getNumStoredTimes())
      {
         PyErr_SetString(getError(), "Already stored the maximum number of samples!");
         return NULL;
      }
   }

   if(!HasFullLicense())
   {
      if(numSamples >= 75)
      {
         PyErr_SetString(getError(), "[ExocortexAlembic] Demo Mode: Max samplecount is 75!");
         return NULL;
      }
   }

   PyObject * tuple = NULL;
   if(!PyArg_ParseTuple(args, "O", &tuple))
   {
      PyErr_SetString(getError(), "No sample tuple specified!");
      return NULL;
   }
   if(!PyTuple_Check(tuple) && !PyList_Check(tuple))
   {
      PyErr_SetString(getError(), "Sample tuple argument is not a tuple!");
      return NULL;
   }
   size_t nbItems = 0;
   if(PyTuple_Check(tuple))
      nbItems = PyTuple_Size(tuple);
   else
      nbItems = PyList_Size(tuple);

   switch(prop->mPropType)
   {
      case propertyTP_boolean:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",1);
         prop->mBoolProperty->set(tupleVec[0] == 1);
         break;
      }
      case propertyTP_uchar:
      {
         _COPY_TUPLE_TO_VALUE_(unsigned int,"I",1);
         prop->mUcharProperty->set((unsigned char)tupleVec[0]);
         break;
      }
      case propertyTP_char:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",1);
         prop->mCharProperty->set((char)tupleVec[0]);
         break;
      }
      case propertyTP_uint16:
      {
         _COPY_TUPLE_TO_VALUE_(unsigned int,"I",1);
         prop->mUInt16Property->set(tupleVec[0]);
         break;
      }
      case propertyTP_int16:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",1);
         prop->mInt16Property->set(tupleVec[0]);
         break;
      }
      case propertyTP_uint32:
      {
         _COPY_TUPLE_TO_VALUE_(unsigned long,"k",1);
         prop->mUInt32Property->set(tupleVec[0]);
         break;
      }
      case propertyTP_int32:
      {
         _COPY_TUPLE_TO_VALUE_(long,"l",1);
         prop->mInt32Property->set(tupleVec[0]);
         break;
      }
      case propertyTP_uint64:
      {
         _COPY_TUPLE_TO_VALUE_(unsigned long long,"K",1);
         prop->mUInt64Property->set(tupleVec[0]);
         break;
      }
      case propertyTP_int64:
      {
         _COPY_TUPLE_TO_VALUE_(long long,"L",1);
         prop->mInt64Property->set(tupleVec[0]);
         break;
      }
      case propertyTP_half:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",1);
         prop->mHalfProperty->set(tupleVec[0]);
         break;
      }
      case propertyTP_float:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",1);
         prop->mFloatProperty->set(tupleVec[0]);
         break;
      }
      case propertyTP_double:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",1);
         prop->mDoubleProperty->set(tupleVec[0]);
         break;
      }
      case propertyTP_string:
      {
         _COPY_TUPLE_TO_VALUE_(const char *,"s",1);
         prop->mStringProperty->set(tupleVec[0]);
         break;
      }
      case propertyTP_wstring:
      {
         _COPY_TUPLE_TO_VALUE_(const char *,"s",1);
         prop->mWstringProperty->set((wchar_t*)tupleVec[0]);
         break;
      }
      case propertyTP_v2s:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",2);
         prop->mV2sProperty->set(Imath::V2s((short)tupleVec[0],(short)tupleVec[1]));
         break;
      }
      case propertyTP_v2i:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",2);
         prop->mV2iProperty->set(Imath::V2i(tupleVec[0],tupleVec[1]));
         break;
      }
      case propertyTP_v2f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",2);
         prop->mV2fProperty->set(Imath::V2f(tupleVec[0],tupleVec[1]));
         break;
      }
      case propertyTP_v2d:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",2);
         prop->mV2dProperty->set(Imath::V2d(tupleVec[0],tupleVec[1]));
         break;
      }
      case propertyTP_v3s:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",3);
         prop->mV3sProperty->set(Imath::V3s((short)tupleVec[0],(short)tupleVec[1],(short)tupleVec[2]));
         break;
      }
      case propertyTP_v3i:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",3);
         prop->mV3iProperty->set(Imath::V3i(tupleVec[0],tupleVec[1],tupleVec[2]));
         break;
      }
      case propertyTP_v3f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",3);
         prop->mV3fProperty->set(Imath::V3f(tupleVec[0],tupleVec[1],tupleVec[2]));
         break;
      }
      case propertyTP_v3d:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",3);
         prop->mV3dProperty->set(Imath::V3d(tupleVec[0],tupleVec[1],tupleVec[2]));
         break;
      }
      case propertyTP_p2s:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",2);
         prop->mP2sProperty->set(Imath::V2s((short)tupleVec[0],(short)tupleVec[1]));
         break;
      }
      case propertyTP_p2i:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",2);
         prop->mP2iProperty->set(Imath::V2i(tupleVec[0],tupleVec[1]));
         break;
      }
      case propertyTP_p2f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",2);
         prop->mP2fProperty->set(Imath::V2f(tupleVec[0],tupleVec[1]));
         break;
      }
      case propertyTP_p2d:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",2);
         prop->mP2dProperty->set(Imath::V2d(tupleVec[0],tupleVec[1]));
         break;
      }
      case propertyTP_p3s:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",3);
         prop->mP3sProperty->set(Imath::V3s((short)tupleVec[0],(short)tupleVec[1],(short)tupleVec[2]));
         break;
      }
      case propertyTP_p3i:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",3);
         prop->mP3iProperty->set(Imath::V3i(tupleVec[0],tupleVec[1],tupleVec[2]));
         break;
      }
      case propertyTP_p3f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",3);
         prop->mP3fProperty->set(Imath::V3f(tupleVec[0],tupleVec[1],tupleVec[2]));
         break;
      }
      case propertyTP_p3d:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",3);
         prop->mP3dProperty->set(Imath::V3d(tupleVec[0],tupleVec[1],tupleVec[2]));
         break;
      }
      case propertyTP_box2s:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",4);
         prop->mBox2sProperty->set(Imath::Box2s(Imath::V2s((short)tupleVec[0],(short)tupleVec[1]),Imath::V2s((short)tupleVec[2],(short)tupleVec[3])));
         break;
      }
      case propertyTP_box2i:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",4);
         prop->mBox2iProperty->set(Imath::Box2i(Imath::V2i(tupleVec[0],tupleVec[1]),Imath::V2i(tupleVec[2],tupleVec[3])));
         break;
      }
      case propertyTP_box2f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",4);
         prop->mBox2fProperty->set(Imath::Box2f(Imath::V2f(tupleVec[0],tupleVec[1]),Imath::V2f(tupleVec[2],tupleVec[3])));
         break;
      }
      case propertyTP_box2d:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",4);
         prop->mBox2dProperty->set(Imath::Box2d(Imath::V2d(tupleVec[0],tupleVec[1]),Imath::V2d(tupleVec[2],tupleVec[3])));
         break;
      }
      case propertyTP_box3s:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",6);
         prop->mBox3sProperty->set(Imath::Box3s(Imath::V3s((short)tupleVec[0],(short)tupleVec[1],(short)tupleVec[2]),Imath::V3s((short)tupleVec[3],(short)tupleVec[4],(short)tupleVec[5])));
         break;
      }
      case propertyTP_box3i:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",6);
         prop->mBox3iProperty->set(Imath::Box3i(Imath::V3i(tupleVec[0],tupleVec[1],tupleVec[2]),Imath::V3i(tupleVec[3],tupleVec[4],tupleVec[5])));
         break;
      }
      case propertyTP_box3f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",6);
         prop->mBox3fProperty->set(Imath::Box3f(Imath::V3f(tupleVec[0],tupleVec[1],tupleVec[2]),Imath::V3f(tupleVec[3],tupleVec[4],tupleVec[5])));
         break;
      }
      case propertyTP_box3d:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",6);
         prop->mBox3dProperty->set(Imath::Box3d(Imath::V3d(tupleVec[0],tupleVec[1],tupleVec[2]),Imath::V3d(tupleVec[3],tupleVec[4],tupleVec[5])));
         break;
      }
      case propertyTP_m33f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",9);
         prop->mM33fProperty->set(Imath::M33f(tupleVec[0],tupleVec[1],tupleVec[2],tupleVec[3],tupleVec[4],tupleVec[5],tupleVec[6],tupleVec[7],tupleVec[8]));
         break;
      }
      case propertyTP_m33d:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",9);
         prop->mM33dProperty->set(Imath::M33d(tupleVec[0],tupleVec[1],tupleVec[2],tupleVec[3],tupleVec[4],tupleVec[5],tupleVec[6],tupleVec[7],tupleVec[8]));
         break;
      }
      case propertyTP_m44f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",16);
         prop->mM44fProperty->set(Imath::M44f(tupleVec[0],tupleVec[1],tupleVec[2],tupleVec[3],tupleVec[4],tupleVec[5],tupleVec[6],tupleVec[7],
                                             tupleVec[8],tupleVec[9],tupleVec[10],tupleVec[11],tupleVec[12],tupleVec[13],tupleVec[14],tupleVec[15]));
         break;
      }
      case propertyTP_m44d:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",16);
         prop->mM44dProperty->set(Imath::M44d(tupleVec[0],tupleVec[1],tupleVec[2],tupleVec[3],tupleVec[4],tupleVec[5],tupleVec[6],tupleVec[7],
                                             tupleVec[8],tupleVec[9],tupleVec[10],tupleVec[11],tupleVec[12],tupleVec[13],tupleVec[14],tupleVec[15]));
         break;
      }
      case propertyTP_quatf:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",4);
         prop->mQuatfProperty->set(Imath::Quatf(tupleVec[0],Imath::V3f(tupleVec[1],tupleVec[2],tupleVec[3])));
         break;
      }
      case propertyTP_quatd:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",4);
         prop->mQuatdProperty->set(Imath::Quatd(tupleVec[0],Imath::V3d(tupleVec[1],tupleVec[2],tupleVec[3])));
         break;
      }
      case propertyTP_c3h:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",3);
         prop->mC3hProperty->set(Imath::C3h((half)tupleVec[0],(half)tupleVec[1],(half)tupleVec[2]));
         break;
      }
      case propertyTP_c3f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",3);
         prop->mC3fProperty->set(Imath::C3f(tupleVec[0],tupleVec[1],tupleVec[2]));
         break;
      }
      case propertyTP_c3c:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",3);
         prop->mC3cProperty->set(Imath::C3c((unsigned char)tupleVec[0],(unsigned char)tupleVec[1],(unsigned char)tupleVec[2]));
         break;
      }
      case propertyTP_c4h:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",4);
         prop->mC4hProperty->set(Imath::C4h((half)tupleVec[0],(half)tupleVec[1],(half)tupleVec[2],(half)tupleVec[3]));
         break;
      }
      case propertyTP_c4f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",4);
         prop->mC4fProperty->set(Imath::C4f(tupleVec[0],tupleVec[1],tupleVec[2],tupleVec[3]));
         break;
      }
      case propertyTP_c4c:
      {
         _COPY_TUPLE_TO_VALUE_(int,"i",4);
         prop->mC4cProperty->set(Imath::C4c((unsigned char)tupleVec[0],(unsigned char)tupleVec[1],(unsigned char)tupleVec[2],(unsigned char)tupleVec[3]));
         break;
      }
      case propertyTP_n2f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",2);
         prop->mN2fProperty->set(Imath::V2f(tupleVec[0],tupleVec[1]));
         break;
      }
      case propertyTP_n2d:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",2);
         prop->mN2dProperty->set(Imath::V2d(tupleVec[0],tupleVec[1]));
         break;
      }
      case propertyTP_n3f:
      {
         _COPY_TUPLE_TO_VALUE_(float,"f",3);
         prop->mN3fProperty->set(Imath::V3f(tupleVec[0],tupleVec[1],tupleVec[2]));
         break;
      }
      case propertyTP_n3d:
      {
         _COPY_TUPLE_TO_VALUE_(double,"d",3);
         prop->mN3dProperty->set(Imath::V3d(tupleVec[0],tupleVec[1],tupleVec[2]));
         break;
      }
      case propertyTP_boolean_array:
      {
         _COPY_TUPLE_TO_VECTOR_(int,int,"i",1);
         std::vector<Alembic::Abc::bool_t> values(nbItems);
         for(size_t i=0;i<nbItems;i++)
            values[i] = tupleVec[i] > 0;
         Alembic::Abc::OBoolArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OBoolArrayProperty::sample_type(&values.front(),values.size());
         prop->mBoolArrayProperty->set(sample);
         break;
      }
      case propertyTP_uchar_array:
      {
         _COPY_TUPLE_TO_VECTOR_(unsigned int,unsigned int,"I",1);
         std::vector<unsigned char> values(nbItems);
         for(size_t i=0;i<nbItems;i++)
            values[i] = tupleVec[i] > 0;
         Alembic::Abc::OUcharArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OUcharArrayProperty::sample_type(&values.front(),values.size());
         prop->mUcharArrayProperty->set(sample);
         break;
      }
      case propertyTP_char_array:
      {
         _COPY_TUPLE_TO_VECTOR_(int,int,"i",1);
         std::vector<signed char> values(nbItems);
         for(size_t i=0;i<nbItems;i++)
            values[i] = tupleVec[i] > 0;
         Alembic::Abc::OCharArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OCharArrayProperty::sample_type(&values.front(),values.size());
         prop->mCharArrayProperty->set(sample);
         break;
      }
      case propertyTP_uint16_array:
      {
         _COPY_TUPLE_TO_VECTOR_(unsigned short,unsigned int,"I",1);
         Alembic::Abc::OUInt16ArrayProperty::sample_type sample;
         if(tupleVec.size() > 0)
            sample = Alembic::Abc::OUInt16ArrayProperty::sample_type(&tupleVec.front(),tupleVec.size());
         prop->mUInt16ArrayProperty->set(sample);
         break;
      }
      case propertyTP_int16_array:
      {
         _COPY_TUPLE_TO_VECTOR_(short,int,"i",1);
         Alembic::Abc::OInt16ArrayProperty::sample_type sample;
         if(tupleVec.size() > 0)
            sample = Alembic::Abc::OInt16ArrayProperty::sample_type(&tupleVec.front(),tupleVec.size());
         prop->mInt16ArrayProperty->set(sample);
         break;
      }
      case propertyTP_uint32_array:
      {
         _COPY_TUPLE_TO_VECTOR_(boost::uint32_t,unsigned long,"k",1);
         Alembic::Abc::OUInt32ArrayProperty::sample_type sample;
         if(tupleVec.size() > 0)
            sample = Alembic::Abc::OUInt32ArrayProperty::sample_type(&tupleVec.front(),tupleVec.size());
         prop->mUInt32ArrayProperty->set(sample);
         break;
      }
      case propertyTP_int32_array:
      {
         _COPY_TUPLE_TO_VECTOR_(boost::int32_t,long,"l",1);
         Alembic::Abc::OInt32ArrayProperty::sample_type sample;
         if(tupleVec.size() > 0)
            sample = Alembic::Abc::OInt32ArrayProperty::sample_type(&tupleVec.front(),tupleVec.size());
         prop->mInt32ArrayProperty->set(sample);
         break;
      }
      case propertyTP_uint64_array:
      {
         _COPY_TUPLE_TO_VECTOR_(boost::uint64_t,unsigned long long,"K",1);
         Alembic::Abc::OUInt64ArrayProperty::sample_type sample;
         if(tupleVec.size() > 0)
            sample = Alembic::Abc::OUInt64ArrayProperty::sample_type(&tupleVec.front(),tupleVec.size());
         prop->mUInt64ArrayProperty->set(sample);
         break;
      }
      case propertyTP_int64_array:
      {
         _COPY_TUPLE_TO_VECTOR_(boost::int64_t,long long,"L",1);
         Alembic::Abc::OInt64ArrayProperty::sample_type sample;
         if(tupleVec.size() > 0)
            sample = Alembic::Abc::OInt64ArrayProperty::sample_type(&tupleVec.front(),tupleVec.size());
         prop->mInt64ArrayProperty->set(sample);
         break;
      }
      case propertyTP_half_array:
      {
         _COPY_TUPLE_TO_VECTOR_(Alembic::Abc::OHalfProperty::value_type,float,"f",1);
         Alembic::Abc::OHalfArrayProperty::sample_type sample;
         if(tupleVec.size() > 0)
            sample = Alembic::Abc::OHalfArrayProperty::sample_type(&tupleVec.front(),tupleVec.size());
         prop->mHalfArrayProperty->set(sample);
         break;
      }
      case propertyTP_float_array:
      {
         _COPY_TUPLE_TO_VECTOR_(float,float,"f",1);
         Alembic::Abc::OFloatArrayProperty::sample_type sample;
         if(tupleVec.size() > 0)
            sample = Alembic::Abc::OFloatArrayProperty::sample_type(&tupleVec.front(),tupleVec.size());
         prop->mFloatArrayProperty->set(sample);
         break;
      }
      case propertyTP_double_array:
      {
         _COPY_TUPLE_TO_VECTOR_(double,double,"d",1);
         Alembic::Abc::ODoubleArrayProperty::sample_type sample;
         if(tupleVec.size() > 0)
            sample = Alembic::Abc::ODoubleArrayProperty::sample_type(&tupleVec.front(),tupleVec.size());
         prop->mDoubleArrayProperty->set(sample);
         break;
      }
      case propertyTP_string_array:
      {
         _COPY_TUPLE_TO_VECTOR_(std::string,const char *,"s",1);
         Alembic::Abc::OStringArrayProperty::sample_type sample;
         if(tupleVec.size() > 0)
            sample = Alembic::Abc::OStringArrayProperty::sample_type(&tupleVec.front(),tupleVec.size());
         prop->mStringArrayProperty->set(sample);
         break;
      }
      case propertyTP_wstring_array:
      {
         _COPY_TUPLE_TO_VECTOR_(const wchar_t *,const char*,"s",1);
         std::vector<std::wstring> values(nbItems);
         for(size_t i=0;i<nbItems;i++)
            values[i] = tupleVec[i];
         Alembic::Abc::OWstringArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OWstringArrayProperty::sample_type(&values.front(),values.size());
         prop->mWstringArrayProperty->set(sample);
      }
      case propertyTP_v2s_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V2s,short,int,"i",2);
         Alembic::Abc::OV2sArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OV2sArrayProperty::sample_type(&values.front(),values.size());
         prop->mV2sArrayProperty->set(sample);
         break;
      }
      case propertyTP_v2i_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V2i,int,int,"i",2);
         Alembic::Abc::OV2iArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OV2iArrayProperty::sample_type(&values.front(),values.size());
         prop->mV2iArrayProperty->set(sample);
         break;
      }
      case propertyTP_v2f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V2f,float,float,"f",2);
         Alembic::Abc::OV2fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OV2fArrayProperty::sample_type(&values.front(),values.size());
         prop->mV2fArrayProperty->set(sample);
         break;
      }
      case propertyTP_v2d_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V2d,double,double,"d",2);
         Alembic::Abc::OV2dArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OV2dArrayProperty::sample_type(&values.front(),values.size());
         prop->mV2dArrayProperty->set(sample);
         break;
      }
      case propertyTP_v3s_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V3s,short,int,"i",3);
         Alembic::Abc::OV3sArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OV3sArrayProperty::sample_type(&values.front(),values.size());
         prop->mV3sArrayProperty->set(sample);
         break;
      }
      case propertyTP_v3i_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V3i,int,int,"i",3);
         Alembic::Abc::OV3iArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OV3iArrayProperty::sample_type(&values.front(),values.size());
         prop->mV3iArrayProperty->set(sample);
         break;
      }
      case propertyTP_v3f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V3f,float,float,"f",3);
         Alembic::Abc::OV3fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OV3fArrayProperty::sample_type(&values.front(),values.size());
         prop->mV3fArrayProperty->set(sample);
         break;
      }
      case propertyTP_v3d_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V3d,double,double,"d",3);
         Alembic::Abc::OV3dArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OV3dArrayProperty::sample_type(&values.front(),values.size());
         prop->mV3dArrayProperty->set(sample);
         break;
      }
      case propertyTP_p2s_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V2s,short,int,"i",2);
         Alembic::Abc::OP2sArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OP2sArrayProperty::sample_type(&values.front(),values.size());
         prop->mP2sArrayProperty->set(sample);
         break;
      }
      case propertyTP_p2i_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V2i,int,int,"i",2);
         Alembic::Abc::OP2iArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OP2iArrayProperty::sample_type(&values.front(),values.size());
         prop->mP2iArrayProperty->set(sample);
         break;
      }
      case propertyTP_p2f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V2f,float,float,"f",2);
         Alembic::Abc::OP2fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OP2fArrayProperty::sample_type(&values.front(),values.size());
         prop->mP2fArrayProperty->set(sample);
         break;
      }
      case propertyTP_p2d_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V2d,double,double,"d",2);
         Alembic::Abc::OP2dArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OP2dArrayProperty::sample_type(&values.front(),values.size());
         prop->mP2dArrayProperty->set(sample);
         break;
      }
      case propertyTP_p3s_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V3s,short,int,"i",3);
         Alembic::Abc::OP3sArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OP3sArrayProperty::sample_type(&values.front(),values.size());
         prop->mP3sArrayProperty->set(sample);
         break;
      }
      case propertyTP_p3i_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V3i,int,int,"i",3);
         Alembic::Abc::OP3iArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OP3iArrayProperty::sample_type(&values.front(),values.size());
         prop->mP3iArrayProperty->set(sample);
         break;
      }
      case propertyTP_p3f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V3f,float,float,"f",3);
         Alembic::Abc::OP3fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OP3fArrayProperty::sample_type(&values.front(),values.size());
         prop->mP3fArrayProperty->set(sample);
         break;
      }
      case propertyTP_p3d_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V3d,double,double,"d",3);
         Alembic::Abc::OP3dArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OP3dArrayProperty::sample_type(&values.front(),values.size());
         prop->mP3dArrayProperty->set(sample);
         break;
      }
      case propertyTP_box2s_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::Box2s,short,int,"i",4);
         Alembic::Abc::OBox2sArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OBox2sArrayProperty::sample_type(&values.front(),values.size());
         prop->mBox2sArrayProperty->set(sample);
         break;
      }
      case propertyTP_box2i_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::Box2i,int,int,"i",4);
         Alembic::Abc::OBox2iArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OBox2iArrayProperty::sample_type(&values.front(),values.size());
         prop->mBox2iArrayProperty->set(sample);
         break;
      }
      case propertyTP_box2f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::Box2f,float,float,"f",4);
         Alembic::Abc::OBox2fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OBox2fArrayProperty::sample_type(&values.front(),values.size());
         prop->mBox2fArrayProperty->set(sample);
         break;
      }
      case propertyTP_box2d_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::Box2d,double,double,"d",4);
         Alembic::Abc::OBox2dArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OBox2dArrayProperty::sample_type(&values.front(),values.size());
         prop->mBox2dArrayProperty->set(sample);
         break;
      }
      case propertyTP_box3s_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::Box3s,short,int,"i",6);
         Alembic::Abc::OBox3sArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OBox3sArrayProperty::sample_type(&values.front(),values.size());
         prop->mBox3sArrayProperty->set(sample);
         break;
      }
      case propertyTP_box3i_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::Box3i,int,int,"i",6);
         Alembic::Abc::OBox3iArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OBox3iArrayProperty::sample_type(&values.front(),values.size());
         prop->mBox3iArrayProperty->set(sample);
         break;
      }
      case propertyTP_box3f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::Box3f,float,float,"f",6);
         Alembic::Abc::OBox3fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OBox3fArrayProperty::sample_type(&values.front(),values.size());
         prop->mBox3fArrayProperty->set(sample);
         break;
      }
      case propertyTP_box3d_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::Box3d,double,double,"d",6);
         Alembic::Abc::OBox3dArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OBox3dArrayProperty::sample_type(&values.front(),values.size());
         prop->mBox3dArrayProperty->set(sample);
         break;
      }
      case propertyTP_m33f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::M33f,float,float,"f",9);
         Alembic::Abc::OM33fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OM33fArrayProperty::sample_type(&values.front(),values.size());
         prop->mM33fArrayProperty->set(sample);
         break;
      }
      case propertyTP_m33d_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::M33d,double,double,"d",9);
         Alembic::Abc::OM33dArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OM33dArrayProperty::sample_type(&values.front(),values.size());
         prop->mM33dArrayProperty->set(sample);
         break;
      }
      case propertyTP_m44f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::M44f,float,float,"f",16);
         Alembic::Abc::OM44fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OM44fArrayProperty::sample_type(&values.front(),values.size());
         prop->mM44fArrayProperty->set(sample);
         break;
      }
      case propertyTP_m44d_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::M44d,double,double,"d",16);
         Alembic::Abc::OM44dArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OM44dArrayProperty::sample_type(&values.front(),values.size());
         prop->mM44dArrayProperty->set(sample);
         break;
      }
      case propertyTP_quatf_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::Quatf,float,float,"f",4);
         Alembic::Abc::OQuatfArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OQuatfArrayProperty::sample_type(&values.front(),values.size());
         prop->mQuatfArrayProperty->set(sample);
         break;
      }
      case propertyTP_quatd_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::Quatd,double,double,"d",4);
         Alembic::Abc::OQuatdArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OQuatdArrayProperty::sample_type(&values.front(),values.size());
         prop->mQuatdArrayProperty->set(sample);
         break;
      }
      case propertyTP_c3h_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::C3h,half,float,"f",3);
         Alembic::Abc::OC3hArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OC3hArrayProperty::sample_type(&values.front(),values.size());
         prop->mC3hArrayProperty->set(sample);
         break;
      }
      case propertyTP_c3f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::C3f,float,float,"f",3);
         Alembic::Abc::OC3fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OC3fArrayProperty::sample_type(&values.front(),values.size());
         prop->mC3fArrayProperty->set(sample);
         break;
      }
      case propertyTP_c3c_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::C3c,unsigned char,int,"i",3);
         Alembic::Abc::OC3cArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OC3cArrayProperty::sample_type(&values.front(),values.size());
         prop->mC3cArrayProperty->set(sample);
         break;
      }
      case propertyTP_c4h_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::C4h,half,float,"f",4);
         Alembic::Abc::OC4hArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OC4hArrayProperty::sample_type(&values.front(),values.size());
         prop->mC4hArrayProperty->set(sample);
         break;
      }
      case propertyTP_c4f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::C4f,float,float,"f",4);
         Alembic::Abc::OC4fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OC4fArrayProperty::sample_type(&values.front(),values.size());
         prop->mC4fArrayProperty->set(sample);
         break;
      }
      case propertyTP_c4c_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::C4c,unsigned char,int,"i",4);
         Alembic::Abc::OC4cArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::OC4cArrayProperty::sample_type(&values.front(),values.size());
         prop->mC4cArrayProperty->set(sample);
         break;
      }
      case propertyTP_n2f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V2f,float,float,"f",2);
         Alembic::Abc::ON2fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::ON2fArrayProperty::sample_type(&values.front(),values.size());
         prop->mN2fArrayProperty->set(sample);
         break;
      }
      case propertyTP_n2d_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V2d,double,double,"d",2);
         Alembic::Abc::ON2dArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::ON2dArrayProperty::sample_type(&values.front(),values.size());
         prop->mN2dArrayProperty->set(sample);
         break;
      }
      case propertyTP_n3f_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V3f,float,float,"f",3);
         Alembic::Abc::ON3fArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::ON3fArrayProperty::sample_type(&values.front(),values.size());
         prop->mN3fArrayProperty->set(sample);
         break;
      }
      case propertyTP_n3d_array:
      {
         _COPY_TUPLE_TO_VECTOR_AND_CONVERT_(Imath::V3d,double,double,"d",3);
         Alembic::Abc::ON3dArrayProperty::sample_type sample;
         if(values.size() > 0)
            sample = Alembic::Abc::ON3dArrayProperty::sample_type(&values.front(),values.size());
         prop->mN3dArrayProperty->set(sample);
         break;
      }
      default:
      {
         PyErr_SetString(getError(), "Unsupported property type for setting values.");
         return NULL;
      }
   }

   return Py_BuildValue("l",numSamples);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

static PyObject *oProperty_isCompound(PyObject *self)
{
   Py_INCREF(Py_False);
   return Py_False;
}

static PyMethodDef oProperty_methods[] = {
   {"getName", (PyCFunction)oProperty_getName, METH_NOARGS, "Returns the name of the property."},
   {"setValues", (PyCFunction)oProperty_setValues, METH_VARARGS, "Appends a new sample to the property, given the values provided. The values have to be a flat list of components, matching the count of the property. For example if this is a vector3farray property the tuple has to contain a multiple of 3 float values."},
   {"isCompound", (PyCFunction)oProperty_isCompound, METH_NOARGS, "To distinguish between an oProperty and an oCompoundProperty, always returns false for oProperty."},
   {NULL, NULL}
};

static PyObject * oProperty_getAttr(PyObject * self, char * attrName)
{
   ALEMBIC_TRY_STATEMENT
   return Py_FindMethod(oProperty_methods, self, attrName);
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

void oProperty_deletePointers(oProperty * prop)
{
   ALEMBIC_TRY_STATEMENT
   if(prop->mBaseScalarProperty == NULL)
      return;

   // first delete the base property
   if(prop->mIsCompound)      // TODO remove this after debuggin everything
      ;//delete(prop->mBaseCompoundProperty);
   else if(prop->mIsArray)
      delete(prop->mBaseArrayProperty);
   else
      delete(prop->mBaseScalarProperty);
   prop->mBaseScalarProperty = NULL;
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
   prop->mBoolProperty = NULL;
   ALEMBIC_VOID_CATCH_STATEMENT
}


static void oProperty_delete(PyObject * self)
{
   ALEMBIC_TRY_STATEMENT
   oProperty * prop = (oProperty*)self;
   oProperty_deletePointers(prop);
   PyObject_FREE(prop);
   ALEMBIC_VOID_CATCH_STATEMENT
}

static PyTypeObject oProperty_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                                // op_size
  "oProperty",                      // tp_name
  sizeof(oProperty),                // tp_basicsize
  0,                                // tp_itemsize
  (destructor)oProperty_delete,     // tp_dealloc
  0,                                // tp_print
  (getattrfunc)oProperty_getAttr,   // tp_getattr
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
  "This is the output property. It provides methods for setting data on the property.",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  oProperty_methods,             /* tp_methods */
};


#define _NEW_PROP_IMPL_(tp,base,singleprop,arrayprop,writer) \
   if(prop->mIsArray) \
   { \
      prop->mPropType = (propertyTP)(tp - propertyTP_boolean + propertyTP_boolean_array); \
      prop->m##base##arrayprop = new Alembic::Abc::O##base##arrayprop( prop->mBaseArrayProperty->getPtr(),Alembic::Abc::kWrapExisting);\
   } \
   else \
   { \
      prop->mPropType = tp; \
      prop->m##base##singleprop = new Alembic::Abc::O##base##singleprop( prop->mBaseScalarProperty->getPtr(),Alembic::Abc::kWrapExisting);\
   }
#define _NEW_PROP_(tp,base) _NEW_PROP_IMPL_(tp,base,Property,ArrayProperty,Writer)
#define _NEW_PROP_CASE_IMPL_(pod,tp,base,singleprop,arrayprop,writer) \
   case Alembic::Abc::pod: \
   { \
      _NEW_PROP_IMPL_(tp,base,singleprop,arrayprop,writer) \
      break; \
   }
#define _NEW_PROP_CASE_(pod,tp,base) _NEW_PROP_CASE_IMPL_(pod,tp,base,Property,ArrayProperty,Writer)

#define _IF_CREATE_PROP_IMPL_(tp,base,str,singleprop,arrayprop) \
   if(propType == str) \
   { \
      if(prop->mIsArray) \
      { \
         prop->mPropType = (propertyTP)(tp - propertyTP_boolean + propertyTP_boolean_array); \
         prop->m##base##arrayprop = new Alembic::Abc::O##base##arrayprop(compound,in_propName,compound.getMetaData(),tsIndex); \
         prop->mBaseArrayProperty = new Alembic::Abc::OArrayProperty( prop->m##base##arrayprop->getPtr(),Alembic::Abc::kWrapExisting);\
      } \
      else \
      { \
         prop->mPropType = tp; \
         prop->m##base##singleprop = new Alembic::Abc::O##base##singleprop(compound,in_propName,compound.getMetaData(),tsIndex); \
         prop->mBaseScalarProperty = new Alembic::Abc::OScalarProperty( prop->m##base##singleprop->getPtr(),Alembic::Abc::kWrapExisting);\
      } \
   }
#define _IF_CREATE_PROP_(tp,base,str) _IF_CREATE_PROP_IMPL_(tp,base,str,Property,ArrayProperty)

PyObject * oProperty_new(Alembic::Abc::OCompoundProperty compound, std::string compoundFullName, char * in_propName, char * in_propType, int tsIndex, void * in_Archive)
{
   ALEMBIC_TRY_STATEMENT

   // check if we already have this property somewhere
   //Alembic::Abc::OCompoundProperty compound = getCompoundFromOObject(in_casted);
   std::string identifier = compoundFullName;
   identifier.append("/");
   identifier.append(in_propName);

   INFO_MSG("identifier: " << identifier);

   oArchive * archive = (oArchive*)in_Archive;

   // check if it's an iCompoundProperty, they shouldn't have similar names to avoid confusion when reading it!
   {
      oCompoundProperty *cprop = oArchive_getCompPropElement(archive, identifier);
      if (cprop)
         return (PyObject*)cprop;
   }

   oProperty * prop = oArchive_getPropElement(archive, identifier);
   if(prop)
      return (PyObject*)prop;

   // if we don't have it yet, create a new one and insert it into our map
   prop = PyObject_NEW(oProperty, &oProperty_Type);
   prop->mIsCompound = false;
   prop->mBaseScalarProperty = NULL;
   prop->mBoolProperty = NULL;
   prop->mArchive = in_Archive;
   //oArchive_registerPropElement(archive,identifier,prop); // moved where it won't interfere with compounds

   // get the compound property writer
   Alembic::Abc::CompoundPropertyWriterPtr compoundWriter = GetCompoundPropertyWriterPtr(compound);      // this variable is unused!
   const Alembic::Abc::PropertyHeader * propHeader = compound.getPropertyHeader( in_propName );
   if(propHeader != NULL)
   {
      // this property already exists
      Alembic::Abc::OBaseProperty baseProp = compound.getProperty( in_propName );
      std::string interpretation;
      if(baseProp.isCompound())
      {
         /*
         prop->mIsArray = false;
         prop->mIsCompound = true;
         prop->mBaseCompoundProperty = new Alembic::Abc::OCompoundProperty(
            boost::dynamic_pointer_cast<Alembic::Abc::CompoundPropertyWriter>(baseProp.getPtr()), 
            Alembic::Abc::kWrapExisting
         );
         */
         // is a compound, must return an oCompoundProperty
         INFO_MSG("It's a compound");
         PyObject_FREE(prop);
         return oCompoundProperty_new(compound, compoundFullName, in_propName, tsIndex, in_Archive);
      }
      else if(baseProp.isArray())
      {
         prop->mIsArray = true;
         prop->mIsCompound = false;
         prop->mBaseArrayProperty = new Alembic::Abc::OArrayProperty(
            boost::dynamic_pointer_cast<Alembic::Abc::ArrayPropertyWriter>(baseProp.getPtr()), 
            Alembic::Abc::kWrapExisting
         );
         interpretation = prop->mBaseArrayProperty->getMetaData().get("interpretation");
      }
      else
      {
         prop->mIsArray = false;
         prop->mIsCompound = false;
         prop->mBaseScalarProperty = new Alembic::Abc::OScalarProperty(
            boost::dynamic_pointer_cast<Alembic::Abc::ScalarPropertyWriter>(baseProp.getPtr()), 
            Alembic::Abc::kWrapExisting
         );
         interpretation = prop->mBaseScalarProperty->getMetaData().get("interpretation");
      }

      oArchive_registerPropElement(archive,identifier,prop);

      // check for custom structs
      if(interpretation.empty())
         if(propHeader->getDataType().getExtent() > 1)
            interpretation = "unknown";

      // now let's determine the specialized property type
      if(prop->mIsCompound)
      {
         prop->mPropType = propertyTP_compound;
      }
      else if(interpretation.empty())
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
            _NEW_PROP_CASE_(kUint8POD,propertyTP_c3h,C3h)
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
            _NEW_PROP_CASE_(kUint8POD,propertyTP_c4h,C4h)
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
   else
   {
      INFO_MSG("CREATING property " << in_propName << " of type " << in_propType);

      // here we need the property type
      if(in_propType == NULL)
      {
         PyErr_SetString(getError(), "No property type specified!");
         PyObject_FREE(prop);
         return NULL;
      }
      else if (strcmp(in_propType, "compound") == 0)
      {
         PyObject_FREE(prop);
         return oCompoundProperty_new(compound, compoundFullName, in_propName, tsIndex, in_Archive);
      }

      oArchive_registerPropElement(archive,identifier,prop);
      prop->mIsCompound = false;

      // check if this is an array
      std::string propType = in_propType;
      if(propType.length() > 5)
      {
         prop->mIsArray = propType.substr(propType.length()-5,5) == "array";
         if(prop->mIsArray)
            propType = propType.substr(0,propType.length()-5);
      }
      else
         prop->mIsArray = false;

#ifdef PYTHON_DEBUG
      printf("Creating new property '%s' of type '%s'\n",in_propName,propType.c_str());
#endif

      _IF_CREATE_PROP_(propertyTP_boolean,Bool,"bool")
      else _IF_CREATE_PROP_(propertyTP_uchar,Uchar,"uchar")
      else _IF_CREATE_PROP_(propertyTP_char,Char,"char")
      else _IF_CREATE_PROP_(propertyTP_uint16,UInt16,"uint16")
      else _IF_CREATE_PROP_(propertyTP_int16,Int16,"int16")
      else _IF_CREATE_PROP_(propertyTP_uint32,UInt32,"uint32")
      else _IF_CREATE_PROP_(propertyTP_int32,Int32,"int32")
      else _IF_CREATE_PROP_(propertyTP_uint64,UInt64,"uint64")
      else _IF_CREATE_PROP_(propertyTP_int64,Int64,"int64")
      else _IF_CREATE_PROP_(propertyTP_half,Half,"half")
      else _IF_CREATE_PROP_(propertyTP_float,Float,"float")
      else _IF_CREATE_PROP_(propertyTP_double,Double,"double")
      else _IF_CREATE_PROP_(propertyTP_string,String,"string")
      else _IF_CREATE_PROP_(propertyTP_wstring,Wstring,"wstring")
      else _IF_CREATE_PROP_(propertyTP_v2s,V2s,"vector2s")
      else _IF_CREATE_PROP_(propertyTP_v2i,V2i,"vector2i")
      else _IF_CREATE_PROP_(propertyTP_v2f,V2f,"vector2f")
      else _IF_CREATE_PROP_(propertyTP_v2d,V2d,"vector2d")
      else _IF_CREATE_PROP_(propertyTP_v3s,V3s,"vector3s")
      else _IF_CREATE_PROP_(propertyTP_v3i,V3i,"vector3i")
      else _IF_CREATE_PROP_(propertyTP_v3f,V3f,"vector3f")
      else _IF_CREATE_PROP_(propertyTP_v3d,V3d,"vector3d")
      else _IF_CREATE_PROP_(propertyTP_p2s,P2s,"point2s")
      else _IF_CREATE_PROP_(propertyTP_p2i,P2i,"point2i")
      else _IF_CREATE_PROP_(propertyTP_p2f,P2f,"point2f")
      else _IF_CREATE_PROP_(propertyTP_p2d,P2d,"point2d")
      else _IF_CREATE_PROP_(propertyTP_p3s,P3s,"point3s")
      else _IF_CREATE_PROP_(propertyTP_p3i,P3i,"point3i")
      else _IF_CREATE_PROP_(propertyTP_p3f,P3f,"point3f")
      else _IF_CREATE_PROP_(propertyTP_p3d,P3d,"point3d")
      else _IF_CREATE_PROP_(propertyTP_box2s,Box2s,"box2s")
      else _IF_CREATE_PROP_(propertyTP_box2i,Box2i,"box2i")
      else _IF_CREATE_PROP_(propertyTP_box2f,Box2f,"box2f")
      else _IF_CREATE_PROP_(propertyTP_box2d,Box2d,"box2d")
      else _IF_CREATE_PROP_(propertyTP_box3s,Box3s,"box3s")
      else _IF_CREATE_PROP_(propertyTP_box3i,Box3i,"box3i")
      else _IF_CREATE_PROP_(propertyTP_box3f,Box3f,"box3f")
      else _IF_CREATE_PROP_(propertyTP_box3d,Box3d,"box3d")
      else _IF_CREATE_PROP_(propertyTP_m33f,M33f,"matrix3f")
      else _IF_CREATE_PROP_(propertyTP_m33d,M33d,"matrix3d")
      else _IF_CREATE_PROP_(propertyTP_m44f,M44f,"matrix4f")
      else _IF_CREATE_PROP_(propertyTP_m44d,M44d,"matrix4d")
      else _IF_CREATE_PROP_(propertyTP_quatf,Quatf,"quatf")
      else _IF_CREATE_PROP_(propertyTP_quatd,Quatd,"quatd")
      else _IF_CREATE_PROP_(propertyTP_c3h,C3h,"color3h")
      else _IF_CREATE_PROP_(propertyTP_c3f,C3f,"color3f")
      else _IF_CREATE_PROP_(propertyTP_c3c,C3c,"color3c")
      else _IF_CREATE_PROP_(propertyTP_c4h,C4h,"color4h")
      else _IF_CREATE_PROP_(propertyTP_c4f,C4f,"color4f")
      else _IF_CREATE_PROP_(propertyTP_c4c,C4c,"color4c")
      else _IF_CREATE_PROP_(propertyTP_n2f,N2f,"normal2f")
      else _IF_CREATE_PROP_(propertyTP_n2d,N2d,"normal2d")
      else _IF_CREATE_PROP_(propertyTP_n3f,N3f,"normal3f")
      else _IF_CREATE_PROP_(propertyTP_n3d,N3d,"normal3d")
      else
      {
         std::string error;
         error.append("Invalid property type '");
         error.append(in_propType);
         error.append("' for property '");
         error.append(identifier);
         error.append("'!");
         PyErr_SetString(getError(), error.c_str());
         return NULL;
      }
   }

   return (PyObject *)prop;
   ALEMBIC_PYOBJECT_CATCH_STATEMENT
}

bool register_object_oProperty(PyObject *module)
{
  return register_object(module, oProperty_Type, "oProperty");
}



