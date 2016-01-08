#include "stdafx.h"

#include "AlembicPropertyNodes.h"
#include "Utility.h"

/// ICE NODE!
enum IDs {
  ID_IN_path = 10,
  ID_IN_identifier = 11,
  ID_IN_property = 12,
  ID_IN_isCustomProp = 13,
  ID_IN_time = 14,
  ID_IN_multifile = 15,
  ID_G_100 = 1005,
  ID_OUT_data = 12771,
  ID_OUT_valid = 12772,

  ID_TYPE_CNS = 1400,
  ID_STRUCT_CNS,
  ID_CTXT_CNS,
};

#define ID_UNDEF ((ULONG)-1)

using namespace XSI;

void getParams(ICENodeContext& in_ctxt, CString& path, bool& bMultifile,
               CString& identifier, CString& aproperty, bool& isDefault,
               double& time)
{
  CDataArrayString pathData(in_ctxt, ID_IN_path);
  path = pathData[0];
  CDataArrayBool multifile(in_ctxt, ID_IN_multifile);
  bMultifile = multifile[0];
  CDataArrayString identifierData(in_ctxt, ID_IN_identifier);
  identifier = identifierData[0];
  CDataArrayString propertyData(in_ctxt, ID_IN_property);
  aproperty = propertyData[0];
  CDataArrayBool isDefaultData(in_ctxt, ID_IN_isCustomProp);
  isDefault = isDefaultData[0];
  CDataArrayFloat timeData(in_ctxt, ID_IN_time);
  time = timeData[0];
}

// void addArchiveRef(ICENodeContext& in_ctxt, CString path)
//{
//	// check if we need t addref the archive
//	CValue udVal = in_ctxt.GetUserData();
//	ArchiveInfo * p = (ArchiveInfo*)(CValue::siPtrType)udVal;
//	if(p == NULL)
//	{
//		p = new ArchiveInfo;
//		p->path = path.GetAsciiString();
//		addRefArchive(path);
//		CValue val = (CValue::siPtrType) p;
//		in_ctxt.PutUserData( val ) ;
//	}
//}
//
// void delArchiveRef(Context& in_ctxt)
//{
//	CValue udVal = in_ctxt.GetUserData();
//	ArchiveInfo * p = (ArchiveInfo*)(CValue::siPtrType)udVal;
//	if(p != NULL)
//	{
//		delRefArchive(p->path);
//		delete(p);
//	}
//}

bool findProperty(Abc::ICompoundProperty& props,
                  AbcA::PropertyHeader& propHeader, CString aproperty)
{
  if (!props.valid()) {
    return false;
  }
  for (size_t i = 0; i < props.getNumProperties(); i++) {
    AbcA::PropertyHeader pheader = props.getPropertyHeader(i);
    if (pheader.getName() == std::string(aproperty.GetAsciiString())) {
      propHeader = pheader;
      return true;
    }
  }
  return false;
}

Abc::ICompoundProperty getProperty(const Abc::ICompoundProperty& props,
                                   const char* aproperty)
{
  for (size_t i = 0; i < props.getNumProperties(); i++) {
    AbcA::PropertyHeader pheader = props.getPropertyHeader(i);
    if (pheader.getName() == std::string(aproperty)) {
      return Abc::ICompoundProperty(props, aproperty);
    }
  }
  return Abc::ICompoundProperty();
}

char* getPropertyTypeStr(AbcA::PropertyType type)
{
  if (type == AbcA::kCompoundProperty) return "kCompoundProperty";
  if (type == AbcA::kScalarProperty) return "kScalerProperty";
  if (type == AbcA::kArrayProperty) return "kArrayProperty";
  return "kUnknownProperty";
}

char* getPodStr(AbcA::PlainOldDataType pod)
{
  if (pod == AbcA::kBooleanPOD) return "kBooleanPOD";
  if (pod == AbcA::kUint8POD) return "kUint8POD";
  if (pod == AbcA::kInt8POD) return "kInt8POD";
  if (pod == AbcA::kUint16POD) return "kUint16POD";
  if (pod == AbcA::kInt16POD) return "kInt16POD";
  if (pod == AbcA::kUint32POD) return "kUint32POD";
  if (pod == AbcA::kInt32POD) return "kInt32POD";
  if (pod == AbcA::kUint64POD) return "kUint64POD";
  if (pod == AbcA::kInt64POD) return "kInt64POD";
  if (pod == AbcA::kFloat16POD) return "kFloat16POD";
  if (pod == AbcA::kFloat32POD) return "kFloat32POD";
  if (pod == AbcA::kFloat64POD) return "kFloat64POD";
  if (pod == AbcA::kStringPOD) return "kStringPOD";
  if (pod == AbcA::kWstringPOD) return "kWstringPOD";
  if (pod == AbcA::kNumPlainOldDataTypes) return "kNumPlainOldDataTypes";
  // if(pod == AbcA::kUnknownPOD)
  return "kUnknownPOD";
}

void outputTypeWarning(const std::string& nodeType,
                       AbcA::PropertyHeader& propHeader, CString& aproperty)
{
  ESS_LOG_ERROR(
      nodeType
      << " node error: type mismatch, propertyName: "
      << aproperty.GetAsciiString()
      /*<<", propertyType: "<<getPropertyType(propHeader.getPropertyType())*/
      << ", pod: " << getPodStr(propHeader.getDataType().getPod())
      << ", extent: " << (int)propHeader.getDataType().getExtent()
      << ", interpretation: "
      << propHeader.getMetaData().get("interpretation"));
}

namespace writeArrayRes {
enum enumT {
  SUCCESS,
  INVALID_ALEMBIC_PARAM,
  NO_ALEMBIC_SAMPLES,
  EMPTY,
  TYPE_MISMATCH
};
};

template <class PROP, class SAMPLER, class TYPE>
writeArrayRes::enumT writeArray1i(SampleInfo& sampleInfo,
                                  Abc::ICompoundProperty& props,
                                  AbcA::PropertyHeader& propHeader,
                                  CDataArray2DLong& outData)
{
  if (PROP::matches(propHeader)) {
    CDataArray2DLong::Accessor acc;

    PROP propArray(props, propHeader.getName());
    if (!propArray.valid()) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::INVALID_ALEMBIC_PARAM;
    }
    if (propArray.getNumSamples() == 0) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::NO_ALEMBIC_SAMPLES;
    }

    SAMPLER propPtr1 = propArray.getValue(sampleInfo.floorIndex);

    if (propPtr1 == NULL || propPtr1->size() == 0) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::EMPTY;
    }

    acc = outData.Resize(0, (ULONG)propPtr1->size());

    for (ULONG i = 0; i < acc.GetCount(); i++) {
      TYPE c = propPtr1->get()[i];
      acc[i] = (TYPE)c;
    }

    return writeArrayRes::SUCCESS;
  }

  return writeArrayRes::TYPE_MISMATCH;
}

template <class PROP, class SAMPLER, class TYPE>
writeArrayRes::enumT writeArray1f(SampleInfo& sampleInfo,
                                  Abc::ICompoundProperty& props,
                                  AbcA::PropertyHeader& propHeader,
                                  CDataArray2DFloat& outData)
{
  if (PROP::matches(propHeader)) {
    CDataArray2DFloat::Accessor acc;

    PROP propArray(props, propHeader.getName());
    if (!propArray.valid()) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::INVALID_ALEMBIC_PARAM;
    }
    if (propArray.getNumSamples() == 0) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::NO_ALEMBIC_SAMPLES;
    }

    SAMPLER propPtr1 = propArray.getValue(sampleInfo.floorIndex);

    if (propPtr1 == NULL || propPtr1->size() == 0) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::EMPTY;
    }

    SAMPLER propPtr2 = propArray.getValue(sampleInfo.ceilIndex);

    acc = outData.Resize(0, (ULONG)propPtr1->size());

    const float t = (float)sampleInfo.alpha;

    if (sampleInfo.alpha != 0.0 && propPtr1->size() == propPtr2->size()) {
      for (ULONG i = 0; i < acc.GetCount(); i++) {
        TYPE c1 = propPtr1->get()[i];
        TYPE c2 = propPtr2->get()[i];
        TYPE c = c2 + (1.0f - t) * c1;
        acc[i] = c;
      }
    }
    else {
      for (ULONG i = 0; i < acc.GetCount(); i++) {
        TYPE c = propPtr1->get()[i];
        acc[i] = c;
      }
    }

    return writeArrayRes::SUCCESS;
  }

  return writeArrayRes::TYPE_MISMATCH;
}

template <class PROP, class SAMPLER, class TYPE>
writeArrayRes::enumT writeArray2f(SampleInfo& sampleInfo,
                                  Abc::ICompoundProperty& props,
                                  AbcA::PropertyHeader& propHeader,
                                  CDataArray2DVector2f& outData)
{
  if (PROP::matches(propHeader)) {
    CDataArray2DVector2f::Accessor acc;
    PROP propArray(props, propHeader.getName());

    if (!propArray.valid()) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::INVALID_ALEMBIC_PARAM;
    }
    if (propArray.getNumSamples() == 0) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::NO_ALEMBIC_SAMPLES;
    }

    SAMPLER propPtr1 = propArray.getValue(sampleInfo.floorIndex);

    if (propPtr1 == NULL || propPtr1->size() == 0) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::EMPTY;
    }

    SAMPLER propPtr2 = propArray.getValue(sampleInfo.ceilIndex);

    acc = outData.Resize(0, (ULONG)propPtr1->size());

    const float t = (float)sampleInfo.alpha;

    if (sampleInfo.alpha != 0.0 && propPtr1->size() == propPtr2->size()) {
      for (ULONG i = 0; i < acc.GetCount(); i++) {
        TYPE c1 = propPtr1->get()[i];
        TYPE c2 = propPtr2->get()[i];
        TYPE c = c2 + (1.0f - t) * c1;
        acc[i].PutX(c.x);
        acc[i].PutY(c.y);
      }
    }
    else {
      for (ULONG i = 0; i < acc.GetCount(); i++) {
        TYPE c = propPtr1->get()[i];
        acc[i].PutX(c.x);
        acc[i].PutY(c.y);
      }
    }

    return writeArrayRes::SUCCESS;
  }

  return writeArrayRes::TYPE_MISMATCH;
}

template <class PROP, class SAMPLER, class TYPE>
writeArrayRes::enumT writeArray3f(SampleInfo& sampleInfo,
                                  Abc::ICompoundProperty& props,
                                  AbcA::PropertyHeader& propHeader,
                                  CDataArray2DVector3f& outData)
{
  if (PROP::matches(propHeader)) {
    CDataArray2DVector3f::Accessor acc;
    PROP propArray(props, propHeader.getName());

    if (!propArray.valid()) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::INVALID_ALEMBIC_PARAM;
    }
    if (propArray.getNumSamples() == 0) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::NO_ALEMBIC_SAMPLES;
    }

    SAMPLER propPtr1 = propArray.getValue(sampleInfo.floorIndex);

    if (propPtr1 == NULL || propPtr1->size() == 0) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::EMPTY;
    }

    SAMPLER propPtr2 = propArray.getValue(sampleInfo.ceilIndex);

    acc = outData.Resize(0, (ULONG)propPtr1->size());

    const float t = (float)sampleInfo.alpha;

    if (sampleInfo.alpha != 0.0 && propPtr1->size() == propPtr2->size()) {
      for (ULONG i = 0; i < acc.GetCount(); i++) {
        TYPE c1 = propPtr1->get()[i];
        TYPE c2 = propPtr2->get()[i];
        TYPE c = c2 + (1.0f - t) * c1;
        acc[i].Set(c.x, c.y, c.z);
      }
    }
    else {
      for (ULONG i = 0; i < acc.GetCount(); i++) {
        TYPE c = propPtr1->get()[i];
        acc[i].Set(c.x, c.y, c.z);
      }
    }

    return writeArrayRes::SUCCESS;
  }

  return writeArrayRes::TYPE_MISMATCH;
}

template <class PROP, class SAMPLER, class TYPE>
writeArrayRes::enumT writeArray4f(SampleInfo& sampleInfo,
                                  Abc::ICompoundProperty& props,
                                  AbcA::PropertyHeader& propHeader,
                                  CDataArray2DVector4f& outData)
{
  if (PROP::matches(propHeader)) {
    CDataArray2DVector4f::Accessor acc;
    PROP propArray(props, propHeader.getName());

    if (!propArray.valid()) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::INVALID_ALEMBIC_PARAM;
    }
    if (propArray.getNumSamples() == 0) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::NO_ALEMBIC_SAMPLES;
    }

    SAMPLER propPtr1 = propArray.getValue(sampleInfo.floorIndex);

    if (propPtr1 == NULL || propPtr1->size() == 0) {
      acc = outData.Resize(0, 0);
      return writeArrayRes::EMPTY;
    }

    SAMPLER propPtr2 = propArray.getValue(sampleInfo.ceilIndex);

    acc = outData.Resize(0, (ULONG)propPtr1->size());

    const float t = (float)sampleInfo.alpha;

    if (sampleInfo.alpha != 0.0 && propPtr1->size() == propPtr2->size()) {
      for (ULONG i = 0; i < acc.GetCount(); i++) {
        TYPE c1 = propPtr1->get()[i];
        TYPE c2 = propPtr2->get()[i];
        TYPE c = c2 + (1.0f - t) * c1;
        acc[i].Set(c.r, c.g, c.b, c.a);
      }
    }
    else {
      for (ULONG i = 0; i < acc.GetCount(); i++) {
        TYPE c = propPtr1->get()[i];
        acc[i].Set(c.r, c.g, c.b, c.a);
      }
    }

    return writeArrayRes::SUCCESS;
  }

  return writeArrayRes::TYPE_MISMATCH;
}

XSI::CStatus defineNode(XSI::PluginRegistrar& in_reg, ULONG in_nDataType,
                        ULONG in_nStructType, CString name)
{
  // Application().LogMessage( "Register_alembic_polyMesh" );
  ICENodeDef nodeDef = Application().GetFactory().CreateICENodeDef(name, name);

  CStatus st = nodeDef.PutColor(255, 188, 102);
  st.AssertSucceeded();

  st = nodeDef.PutThreadingModel(XSI::siICENodeSingleThreading);
  st.AssertSucceeded();

  // Add input ports and groups.
  st = nodeDef.AddPortGroup(ID_G_100);
  st.AssertSucceeded();

  st =
      nodeDef.AddInputPort(ID_IN_path, ID_G_100, siICENodeDataString,
                           siICENodeStructureSingle, siICENodeContextSingleton,
                           L"path", L"path", L"", ID_UNDEF, ID_UNDEF, ID_UNDEF);
  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_multifile, ID_G_100, siICENodeDataBool,
                            siICENodeStructureSingle, siICENodeContextSingleton,
                            L"multifile", L"multifile", L"", ID_UNDEF, ID_UNDEF,
                            ID_UNDEF);
  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_identifier, ID_G_100, siICENodeDataString,
                            siICENodeStructureSingle, siICENodeContextSingleton,
                            L"identifier", L"identifier", L"", ID_UNDEF,
                            ID_UNDEF, ID_UNDEF);
  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_property, ID_G_100, siICENodeDataString,
                            siICENodeStructureSingle, siICENodeContextSingleton,
                            L"property", L"property", L"", ID_UNDEF, ID_UNDEF,
                            ID_UNDEF);
  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_isCustomProp, ID_G_100, siICENodeDataBool,
                            siICENodeStructureSingle, siICENodeContextSingleton,
                            L"custom", L"custom", true, ID_UNDEF, ID_UNDEF,
                            ID_UNDEF);
  st.AssertSucceeded();
  st = nodeDef.AddInputPort(ID_IN_time, ID_G_100, siICENodeDataFloat,
                            siICENodeStructureSingle, siICENodeContextSingleton,
                            L"time", L"time", 0.0f, ID_UNDEF, ID_UNDEF,
                            ID_UNDEF);
  st.AssertSucceeded();

  // Add output ports.
  st = nodeDef.AddOutputPort(ID_OUT_data, in_nDataType, in_nStructType,
                             siICENodeContextSingleton, L"data", L"data");
  st.AssertSucceeded();
  st = nodeDef.AddOutputPort(ID_OUT_valid, siICENodeDataBool,
                             siICENodeStructureSingle,
                             siICENodeContextSingleton, L"valid", L"valid");
  st.AssertSucceeded();

  PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
  nodeItem.PutCategories(L"Alembic");

  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_vec2f_array_Evaluate(ICENodeContext& in_ctxt)
{
  // Application().LogMessage( "alembic_polyMesh2_Evaluate" );
  // The current output port being evaluated...
  ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID();

  CString path, identifier, aproperty;
  double time;
  bool isCustomProp = true;
  bool bMultifile = false;
  getParams(in_ctxt, path, bMultifile, identifier, aproperty, isCustomProp,
            time);

  alembicOp_Init(in_ctxt);  // Softimage will crash if this is done Init handler
  alembicOp_Multifile(in_ctxt, bMultifile, time, path);
  CStatus pathEditStat = alembicOp_PathEdit(in_ctxt, path);

  AbcG::IObject iObj = getObjectFromArchive(path, identifier);
  if (!iObj.valid()) {
    ESS_LOG_ERROR("vec2f_array node error: Could not find "
                  << identifier.GetAsciiString());
    return CStatus::OK;  // return error instead (so that node shows up as red)?
  }

  AbcA::TimeSamplingPtr timeSampling;
  int nSamples = 0;

  Abc::ICompoundProperty props = getArbGeomParams(iObj, timeSampling, nSamples);
  if (!isCustomProp) {
    props = getProperty(iObj.getProperties(), ".geom");
  }

  SampleInfo sampleInfo = getSampleInfo(time, timeSampling, nSamples);

  AbcA::PropertyHeader propHeader;

  if (!findProperty(props, propHeader, aproperty)) {
    ESS_LOG_ERROR("vec2f_array node error: Could not find "
                  << aproperty.GetAsciiString());
    return CStatus::OK;
  }

  switch (out_portID) {
    case ID_OUT_valid: {
      // CDataArrayBool outData( in_ctxt );
      // outData.Resize(1);
      // outData.Set( 0, true );
    } break;
    case ID_OUT_data: {
      CDataArray2DVector2f outData(in_ctxt);

      writeArrayRes::enumT res =
          writeArray2f<Abc::IV2fArrayProperty, Abc::V2fArraySamplePtr,
                       Abc::V2f>(sampleInfo, props, propHeader, outData);
      if (res == writeArrayRes::TYPE_MISMATCH) {
        res = writeArray2f<Abc::IP2fArrayProperty, Abc::P2fArraySamplePtr,
                           Abc::V2f>(sampleInfo, props, propHeader, outData);
      }
      if (res == writeArrayRes::TYPE_MISMATCH) {
        outputTypeWarning("vec2f_array", propHeader, aproperty);
        // ESS_LOG_ERROR("vec2f_array node error: type mismatch
        // "<<aproperty.GetAsciiString());
      }

    } break;
  }

  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_vec2f_array_Term(CRef& in_ctxt)
{
  return alembicOp_Term(in_ctxt);
}

XSI::CStatus Register_alembic_vec2f_array(XSI::PluginRegistrar& in_reg)
{
  return defineNode(in_reg, siICENodeDataVector2, siICENodeStructureArray,
                    L"alembic_vec2f_array");
}

XSIPLUGINCALLBACK CStatus alembic_vec3f_array_Evaluate(ICENodeContext& in_ctxt)
{
  // Application().LogMessage( "alembic_polyMesh2_Evaluate" );
  // The current output port being evaluated...
  ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID();

  CString path, identifier, aproperty;
  double time;
  bool isCustomProp = true;
  bool bMultifile = false;
  getParams(in_ctxt, path, bMultifile, identifier, aproperty, isCustomProp,
            time);

  alembicOp_Init(in_ctxt);  // Softimage will crash if this is done Init handler
  alembicOp_Multifile(in_ctxt, bMultifile, time, path);
  CStatus pathEditStat = alembicOp_PathEdit(in_ctxt, path);

  AbcG::IObject iObj = getObjectFromArchive(path, identifier);
  if (!iObj.valid()) {
    ESS_LOG_ERROR("vec3f_array node error: Could not find "
                  << identifier.GetAsciiString());
    return CStatus::OK;  // return error instead (so that node shows up as red)?
  }

  AbcA::TimeSamplingPtr timeSampling;
  int nSamples = 0;

  Abc::ICompoundProperty props = getArbGeomParams(iObj, timeSampling, nSamples);
  if (!isCustomProp) {
    props = getProperty(iObj.getProperties(), ".geom");
  }

  SampleInfo sampleInfo = getSampleInfo(time, timeSampling, nSamples);

  AbcA::PropertyHeader propHeader;

  if (!findProperty(props, propHeader, aproperty)) {
    ESS_LOG_ERROR("vec3f_array node error: Could not find "
                  << aproperty.GetAsciiString());
    return CStatus::OK;
  }

  switch (out_portID) {
    case ID_OUT_valid: {
      // CDataArrayBool outData( in_ctxt );
      // outData.Resize(1);
      // outData.Set( 0, true );
    } break;
    case ID_OUT_data: {
      CDataArray2DVector3f outData(in_ctxt);
      // CDataArray2DVector3f::Accessor acc;

      // siICENodeDataType out_type;
      // siICENodeStructureType out_struct;
      // siICENodeContextType out_context;
      // in_ctxt.GetPortInfo(out_portID, out_type, out_struct, out_context);

      // if(out_context == siICENodeContextSingleton){
      //   ESS_LOG_ERROR("siICENodeContextSingleton");
      //}
      // else if(out_context == siICENodeContextComponent0D){
      //   ESS_LOG_ERROR("siICENodeContextComponent0D");
      //}

      writeArrayRes::enumT res =
          writeArray3f<Abc::IC3fArrayProperty, Abc::C3fArraySamplePtr,
                       Abc::C3f>(sampleInfo, props, propHeader, outData);
      if (res == writeArrayRes::TYPE_MISMATCH) {
        res = writeArray3f<Abc::IV3fArrayProperty, Abc::V3fArraySamplePtr,
                           Abc::V3f>(sampleInfo, props, propHeader, outData);
      }
      if (res == writeArrayRes::TYPE_MISMATCH) {
        res = writeArray3f<Abc::IN3fArrayProperty, Abc::N3fArraySamplePtr,
                           Abc::N3f>(sampleInfo, props, propHeader, outData);
      }
      if (res == writeArrayRes::TYPE_MISMATCH) {
        res = writeArray3f<Abc::IP3fArrayProperty, Abc::P3fArraySamplePtr,
                           Abc::V3f>(sampleInfo, props, propHeader, outData);
      }
      if (res == writeArrayRes::TYPE_MISMATCH) {
        outputTypeWarning("vec3f_array", propHeader, aproperty);
        // ESS_LOG_ERROR("vec3f_array node error: type mismatch
        // "<<aproperty.GetAsciiString());
      }

    } break;
  }

  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_vec3f_array_Term(CRef& in_ctxt)
{
  return alembicOp_Term(in_ctxt);
}

XSI::CStatus Register_alembic_vec3f_array(XSI::PluginRegistrar& in_reg)
{
  return defineNode(in_reg, siICENodeDataVector3, siICENodeStructureArray,
                    L"alembic_vec3f_array");
}

XSIPLUGINCALLBACK CStatus alembic_vec4f_array_Evaluate(ICENodeContext& in_ctxt)
{
  // Application().LogMessage( "alembic_polyMesh2_Evaluate" );
  // The current output port being evaluated...
  ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID();

  CString path, identifier, aproperty;
  double time;
  bool isCustomProp = true;
  bool bMultifile = false;
  getParams(in_ctxt, path, bMultifile, identifier, aproperty, isCustomProp,
            time);

  alembicOp_Init(in_ctxt);  // Softimage will crash if this is done Init handler
  alembicOp_Multifile(in_ctxt, bMultifile, time, path);
  CStatus pathEditStat = alembicOp_PathEdit(in_ctxt, path);

  AbcG::IObject iObj = getObjectFromArchive(path, identifier);
  if (!iObj.valid()) {
    ESS_LOG_ERROR("vec4f_array node error: Could not find "
                  << identifier.GetAsciiString());
    return CStatus::OK;  // return error instead (so that node shows up as red)?
  }

  AbcA::TimeSamplingPtr timeSampling;
  int nSamples = 0;

  Abc::ICompoundProperty props = getArbGeomParams(iObj, timeSampling, nSamples);
  if (!isCustomProp) {
    props = getProperty(iObj.getProperties(), ".geom");
  }

  SampleInfo sampleInfo = getSampleInfo(time, timeSampling, nSamples);

  AbcA::PropertyHeader propHeader;

  if (!findProperty(props, propHeader, aproperty)) {
    ESS_LOG_ERROR("vec4f_array node error: Could not find "
                  << aproperty.GetAsciiString());
    return CStatus::OK;
  }

  switch (out_portID) {
    case ID_OUT_valid: {
      // CDataArrayBool outData( in_ctxt );
      // outData.Resize(1);
      // outData.Set( 0, true );
    } break;
    case ID_OUT_data: {
      CDataArray2DVector4f outData(in_ctxt);

      writeArrayRes::enumT res =
          writeArray4f<Abc::IC4fArrayProperty, Abc::C4fArraySamplePtr,
                       Abc::C4f>(sampleInfo, props, propHeader, outData);
      if (res == writeArrayRes::TYPE_MISMATCH) {
        res = writeArray4f<Abc::IC4hArrayProperty, Abc::C4hArraySamplePtr,
                           Abc::C4h>(sampleInfo, props, propHeader, outData);
      }
      if (res == writeArrayRes::TYPE_MISMATCH) {
        outputTypeWarning("vec4f", propHeader, aproperty);
        // ESS_LOG_ERROR("vec4f_array node error: type mismatch
        // "<<aproperty.GetAsciiString());
      }

    } break;
  }

  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_vec4f_array_Term(CRef& in_ctxt)
{
  return alembicOp_Term(in_ctxt);
}

XSI::CStatus Register_alembic_vec4f_array(XSI::PluginRegistrar& in_reg)
{
  return defineNode(in_reg, siICENodeDataVector4, siICENodeStructureArray,
                    L"alembic_vec4f_array");
}

XSIPLUGINCALLBACK CStatus alembic_float_array_Evaluate(ICENodeContext& in_ctxt)
{
  // Application().LogMessage( "alembic_polyMesh2_Evaluate" );
  // The current output port being evaluated...
  ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID();

  CString path, identifier, aproperty;
  double time;
  bool isCustomProp = true;
  bool bMultifile = false;
  getParams(in_ctxt, path, bMultifile, identifier, aproperty, isCustomProp,
            time);

  alembicOp_Init(in_ctxt);  // Softimage will crash if this is done Init handler
  alembicOp_Multifile(in_ctxt, bMultifile, time, path);
  CStatus pathEditStat = alembicOp_PathEdit(in_ctxt, path);

  AbcG::IObject iObj = getObjectFromArchive(path, identifier);
  if (!iObj.valid()) {
    ESS_LOG_ERROR("float_array node error: Could not find "
                  << identifier.GetAsciiString());
    return CStatus::OK;  // return error instead (so that node shows up as red)?
  }

  AbcA::TimeSamplingPtr timeSampling;
  int nSamples = 0;

  Abc::ICompoundProperty props = getArbGeomParams(iObj, timeSampling, nSamples);
  if (!isCustomProp) {
    props = getProperty(iObj.getProperties(), ".geom");
  }

  SampleInfo sampleInfo = getSampleInfo(time, timeSampling, nSamples);

  AbcA::PropertyHeader propHeader;

  if (!findProperty(props, propHeader, aproperty)) {
    ESS_LOG_ERROR("float_array node error: Could not find "
                  << aproperty.GetAsciiString());
    return CStatus::OK;
  }

  switch (out_portID) {
    case ID_OUT_valid: {
      // CDataArrayBool outData( in_ctxt );
      // outData.Set( 0, true );
    } break;
    case ID_OUT_data: {
      CDataArray2DFloat outData(in_ctxt);

      writeArrayRes::enumT res =
          writeArray1f<Abc::IFloatArrayProperty, Abc::FloatArraySamplePtr,
                       float>(sampleInfo, props, propHeader, outData);

      if (res == writeArrayRes::TYPE_MISMATCH) {
        outputTypeWarning("float_array", propHeader, aproperty);
        // ESS_LOG_ERROR("float_array node error: type mismatch
        // "<<aproperty.GetAsciiString());
      }
    } break;
  }

  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_float_array_Term(CRef& in_ctxt)
{
  return alembicOp_Term(in_ctxt);
}

XSI::CStatus Register_alembic_float_array(XSI::PluginRegistrar& in_reg)
{
  return defineNode(in_reg, siICENodeDataFloat, siICENodeStructureArray,
                    L"alembic_float_array");
}

XSIPLUGINCALLBACK CStatus alembic_int_array_Evaluate(ICENodeContext& in_ctxt)
{
  // Application().LogMessage( "alembic_polyMesh2_Evaluate" );
  // The current output port being evaluated...
  ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID();

  CString path, identifier, aproperty;
  double time;
  bool isCustomProp = true;
  bool bMultifile = false;
  getParams(in_ctxt, path, bMultifile, identifier, aproperty, isCustomProp,
            time);

  alembicOp_Init(in_ctxt);  // Softimage will crash if this is done Init handler
  alembicOp_Multifile(in_ctxt, bMultifile, time, path);
  CStatus pathEditStat = alembicOp_PathEdit(in_ctxt, path);

  AbcG::IObject iObj = getObjectFromArchive(path, identifier);
  if (!iObj.valid()) {
    ESS_LOG_ERROR("long_array node error: Could not find "
                  << identifier.GetAsciiString());
    return CStatus::OK;  // return error instead (so that node shows up as red)?
  }

  AbcA::TimeSamplingPtr timeSampling;
  int nSamples = 0;

  Abc::ICompoundProperty props = getArbGeomParams(iObj, timeSampling, nSamples);
  if (!isCustomProp) {
    props = getProperty(iObj.getProperties(), ".geom");
  }

  SampleInfo sampleInfo = getSampleInfo(time, timeSampling, nSamples);

  AbcA::PropertyHeader propHeader;

  if (!findProperty(props, propHeader, aproperty)) {
    ESS_LOG_ERROR("long_array node error: Could not find "
                  << aproperty.GetAsciiString());
    return CStatus::OK;
  }

  switch (out_portID) {
    case ID_OUT_valid: {
      // CDataArrayBool outData( in_ctxt );
      // outData.Set( 0, true );
    } break;
    case ID_OUT_data: {
      CDataArray2DLong outData(in_ctxt);

      writeArrayRes::enumT res =
          writeArray1i<Abc::IInt32ArrayProperty, Abc::Int32ArraySamplePtr,
                       long>(sampleInfo, props, propHeader, outData);
      if (res == writeArrayRes::TYPE_MISMATCH) {
        res = writeArray1i<Abc::IInt16ArrayProperty, Abc::Int16ArraySamplePtr,
                           long>(sampleInfo, props, propHeader, outData);
      }
      if (res == writeArrayRes::TYPE_MISMATCH) {
        res = writeArray1i<Abc::IUInt16ArrayProperty, Abc::UInt16ArraySamplePtr,
                           long>(sampleInfo, props, propHeader, outData);
      }
      if (res == writeArrayRes::TYPE_MISMATCH) {
        res = writeArray1i<Abc::ICharArrayProperty, Abc::CharArraySamplePtr,
                           long>(sampleInfo, props, propHeader, outData);
      }
      if (res == writeArrayRes::TYPE_MISMATCH) {
        res = writeArray1i<Abc::IUcharArrayProperty, Abc::UcharArraySamplePtr,
                           long>(sampleInfo, props, propHeader, outData);
      }

      // won't support for now. Should we truncate and import with warning?
      // typedef ITypedArrayProperty<Uint32TPTraits> IUInt32ArrayProperty;
      // typedef ITypedArrayProperty<Uint64TPTraits> IUInt64ArrayProperty;
      // typedef ITypedArrayProperty<Int64TPTraits> IInt64ArrayProperty;

      if (res == writeArrayRes::TYPE_MISMATCH) {
        outputTypeWarning("long_array", propHeader, aproperty);
      }
    } break;
  }

  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_int_array_Term(CRef& in_ctxt)
{
  return alembicOp_Term(in_ctxt);
}

XSI::CStatus Register_alembic_int_array(XSI::PluginRegistrar& in_reg)
{
  return defineNode(in_reg, siICENodeDataLong, siICENodeStructureArray,
                    L"alembic_int_array");
}

/*std::stringstream getErrorMessage()
{
   std::stringstream ret;

   if(propHeader.

   ret<<"Alembic type information, property type: "<<propHeader.getMetaData());
   return ret;
}*/

XSIPLUGINCALLBACK CStatus alembic_string_array_Evaluate(ICENodeContext& in_ctxt)
{
  // Application().LogMessage( "alembic_polyMesh2_Evaluate" );
  // The current output port being evaluated...
  ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID();

  CString path, identifier, aproperty;
  double time;
  bool isCustomProp = true;
  bool bMultifile = false;
  getParams(in_ctxt, path, bMultifile, identifier, aproperty, isCustomProp,
            time);

  alembicOp_Init(in_ctxt);  // Softimage will crash if this is done Init handler
  alembicOp_Multifile(in_ctxt, bMultifile, time, path);
  CStatus pathEditStat = alembicOp_PathEdit(in_ctxt, path);

  AbcG::IObject iObj = getObjectFromArchive(path, identifier);
  if (!iObj.valid()) {
    ESS_LOG_ERROR("string node error: Could not find "
                  << identifier.GetAsciiString());
    return CStatus::OK;  // return error instead (so that node shows up as red)?
  }

  AbcA::TimeSamplingPtr timeSampling;
  int nSamples = 0;

  Abc::ICompoundProperty props = getArbGeomParams(iObj, timeSampling, nSamples);
  if (!isCustomProp) {
    props = getProperty(iObj.getProperties(), ".geom");
  }

  SampleInfo sampleInfo = getSampleInfo(time, timeSampling, nSamples);

  AbcA::PropertyHeader propHeader;

  if (!findProperty(props, propHeader, aproperty)) {
    ESS_LOG_ERROR("string node error: Could not find "
                  << aproperty.GetAsciiString());
    return CStatus::OK;
  }

  switch (out_portID) {
    case ID_OUT_valid: {
      // CDataArrayBool outData( in_ctxt );
      // outData.Set( 0, true );
    } break;
    case ID_OUT_data: {
      CDataArray2DString outData(in_ctxt);
      CDataArray2DString::Accessor acc;

      if (Abc::IStringArrayProperty::matches(propHeader)) {
        Abc::IStringArrayProperty propArray(props, propHeader.getName());
        if (!propArray.valid()) {
          acc = outData.Resize(0, 0);
          return CStatus::OK;
        }
        if (propArray.getNumSamples() == 0) {
          acc = outData.Resize(0, 0);
          return CStatus::OK;
        }

        Abc::StringArraySamplePtr propPtr1 =
            propArray.getValue(sampleInfo.floorIndex);

        if (propPtr1 == NULL || propPtr1->size() == 0) {
          acc = outData.Resize(0, 0);
          return CStatus::OK;
        }

        acc = outData.Resize(0, (ULONG)propPtr1->size());

        for (ULONG i = 0; i < acc.GetCount(); i++) {
          acc.SetData(i, propPtr1->get()[i].c_str());
        }
      }
      else {
        outputTypeWarning("string", propHeader, aproperty);
      }
    } break;
  }

  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_string_array_Term(CRef& in_ctxt)
{
  return alembicOp_Term(in_ctxt);
}

XSI::CStatus Register_alembic_string_array(XSI::PluginRegistrar& in_reg)
{
  return defineNode(in_reg, siICENodeDataString, siICENodeStructureArray,
                    L"alembic_string_array");
}
