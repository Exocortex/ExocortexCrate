#include "stdafx.h"

#include "AlembicCustomAttributesEx.h"

using namespace XSI;

XSI::CStatus AlembicCustomAttributesEx::defineCustomAttributes(
    XSI::Geometry geo, Abc::OCompoundProperty& argGeomParams,
    const AbcA::MetaData& metadata, unsigned int animatedTs)
{
  ICEAttribute exportAttr =
      geo.GetICEAttributeFromName(L"AlembicExportAttributeNames");
  bool bDefined = exportAttr.IsDefined();
  bool bValid = exportAttr.IsValid();
  if (exportAttr.IsDefined() && exportAttr.IsValid()) {
    // TODO: add error checking

    CICEAttributeDataArrayString exportStringData;
    exportAttr.GetDataArray(exportStringData);

    // if(exportStringData.GetCount() > 0){
    //   ESS_LOG_WARNING("exportStringData.GetCount() > 0");
    //}

    CString exportString = exportStringData[0];

    // ESS_LOG_WARNING("export String: "<<exportString.GetAsciiString());

    CStringArray exportArray = exportString.Split(",");

    // trim whitespace
    for (LONG i = 0; i < exportArray.GetCount(); i++) {
      // ESS_LOG_WARNING("Read attribute "<<exportArray[i].GetAsciiString());
      std::string tstr(exportArray[i].GetAsciiString());
      exportArray[i] = tstr.c_str();
    }

    for (LONG i = 0; i < exportArray.GetCount(); i++) {
      ICEAttribute attr = geo.GetICEAttributeFromName(exportArray[i]);

      if (!attr.IsDefined() || !attr.IsValid()) {
        ESS_LOG_WARNING(
            "Could not find attribute: " << exportArray[i].GetAsciiString());
        continue;
      }

      XSI::siICENodeDataType datatype = attr.GetDataType();
      XSI::siICENodeStructureType datastruct = attr.GetStructureType();

      if (customProps.find(exportArray[i]) !=
          customProps
              .end()) {  // skips attributes that have already been defined
        continue;
      }

      if (attr.GetDataType() == siICENodeDataString &&
          attr.GetStructureType() == siICENodeStructureSingle) {
        // ESS_LOG_WARNING("Defining siICENodeDataString
        // siICENodeStructureSingle");
        customProps[exportArray[i]] = Abc::OStringArrayProperty(
            argGeomParams, exportArray[i].GetAsciiString(), metadata,
            animatedTs);
      }
      else if (attr.GetDataType() == siICENodeDataFloat &&
               attr.GetStructureType() == siICENodeStructureSingle) {
        // ESS_LOG_WARNING("Defining siICENodeDataFloat
        // siICENodeStructureSingle");
        customProps[exportArray[i]] = Abc::OFloatArrayProperty(
            argGeomParams, exportArray[i].GetAsciiString(), metadata,
            animatedTs);
      }
      else if (attr.GetDataType() == siICENodeDataVector3 &&
               attr.GetStructureType() == siICENodeStructureSingle) {
        // ESS_LOG_WARNING("Defining siICENodeDataVector3
        // siICENodeStructureSingle");
        customProps[exportArray[i]] = Abc::OV3fArrayProperty(
            argGeomParams, exportArray[i].GetAsciiString(), metadata,
            animatedTs);
      }
      else if (attr.GetDataType() == siICENodeDataLong &&
               attr.GetStructureType() == siICENodeStructureSingle) {
        // ESS_LOG_WARNING("Defining siICENodeDataLong
        // siICENodeStructureSingle");
        customProps[exportArray[i]] = Abc::OInt32ArrayProperty(
            argGeomParams, exportArray[i].GetAsciiString(), metadata,
            animatedTs);
      }
      else {
        ESS_LOG_WARNING(
            "Unsupported attribute type: " << exportArray[i].GetAsciiString());
      }
    }
  }

  return CStatus::OK;
}

XSI::CStatus AlembicCustomAttributesEx::exportCustomAttributes(
    XSI::Geometry geo)
{
  for (propMap::iterator it = customProps.begin(); it != customProps.end();
       it++) {
    CString name = it->first;
    ICEAttribute attr = geo.GetICEAttributeFromName(it->first);

    // ESS_LOG_WARNING("Read attribute "<<name.GetAsciiString());

    if (!attr.IsDefined() || !attr.IsValid()) {
      continue;
    }

    XSI::siICENodeDataType datatype = attr.GetDataType();
    XSI::siICENodeStructureType datastruct = attr.GetStructureType();

    if (attr.GetDataType() == siICENodeDataString &&
        attr.GetStructureType() == siICENodeStructureSingle) {
      // ESS_LOG_WARNING("Writing siICENodeDataString
      // siICENodeStructureSingle");

      CICEAttributeDataArrayString stringArrayData;
      attr.GetDataArray(stringArrayData);

      // if(stringArrayData.GetCount() > 0){

      std::vector<std::string> outputStringArray;

      for (ULONG j = 0; j < stringArrayData.GetCount(); j++) {
        outputStringArray.push_back(stringArrayData[j].GetAsciiString());
      }

      it->second.set(Abc::StringArraySample(outputStringArray));
    }
    else if (attr.GetDataType() == siICENodeDataFloat &&
             attr.GetStructureType() == siICENodeStructureSingle) {
      // ESS_LOG_WARNING("Writing siICENodeDataFloat siICENodeStructureSingle");
      CICEAttributeDataArrayFloat floatArrayData;
      attr.GetDataArray(floatArrayData);

      std::vector<float> outputFloatArray;

      for (ULONG j = 0; j < floatArrayData.GetCount(); j++) {
        outputFloatArray.push_back(floatArrayData[j]);
      }

      it->second.set(Abc::FloatArraySample(outputFloatArray));
    }
    else if (attr.GetDataType() == siICENodeDataVector3 &&
             attr.GetStructureType() == siICENodeStructureSingle) {
      // ESS_LOG_WARNING("Writing siICENodeDataVector3
      // siICENodeStructureSingle");

      // if(attr.GetStructureType() == siICENodeStructureSingle){

      CICEAttributeDataArrayVector3f vecArrayData;
      attr.GetDataArray(vecArrayData);

      std::vector<Abc::V3f> outputVecArray;

      for (ULONG j = 0; j < vecArrayData.GetCount(); j++) {
        outputVecArray.push_back(Abc::V3f(vecArrayData[j].GetX(),
                                          vecArrayData[j].GetY(),
                                          vecArrayData[j].GetZ()));
      }

      it->second.set(Abc::V3fArraySample(outputVecArray));

      // else if(attr.GetStructureType() == siICENodeStructureArray){

      //   CICEAttributeDataArray2DVector3f vecArrayData;
      //   attr.GetDataArray(vecArrayData);

      //   std::vector<Abc::V3f> outputVecArray;

      //   for(ULONG j=0; j<vecArrayData.GetCount(); j++){

      //      CICEAttributeDataArrayVector3f vecArraySubData;
      //      vecArrayData.GetSubArray(j, vecArraySubData);

      //      outputVecArray.push_back( Abc::V3f( vecArraySubData[0].GetX(),
      //      vecArraySubData[0].GetY(), vecArraySubData[0].GetZ() ) );
      //   }

      //   it->second.set(Abc::V3fArraySample(outputVecArray));
      //}
    }
    else if (attr.GetDataType() == siICENodeDataLong &&
             attr.GetStructureType() == siICENodeStructureSingle) {
      // ESS_LOG_WARNING("Writing siICENodeDataLong siICENodeStructureSingle");
      CICEAttributeDataArrayLong longArrayData;
      attr.GetDataArray(longArrayData);

      std::vector<Abc::int32_t> outputLongArray;

      for (ULONG j = 0; j < longArrayData.GetCount(); j++) {
        outputLongArray.push_back(longArrayData[j]);
      }

      it->second.set(Abc::Int32ArraySample(outputLongArray));
    }
  }

  return CStatus::OK;
}
