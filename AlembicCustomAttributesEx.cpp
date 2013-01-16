#include "stdafx.h"
#include "AlembicCustomAttributesEx.h"

using namespace XSI;


XSI::CStatus AlembicCustomAttributesEx::defineCustomAttributes(XSI::Geometry geo, Abc::OCompoundProperty& argGeomParams, const AbcA::MetaData& metadata, unsigned int animatedTs)
{
   ICEAttribute exportAttr = geo.GetICEAttributeFromName(L"AlembicExportAttributeNames");
   bool bDefined = exportAttr.IsDefined();
   bool bValid = exportAttr.IsValid();
   if(exportAttr.IsDefined() && exportAttr.IsValid()){

      //TODO: add error checking

      CICEAttributeDataArrayString exportStringData;
      exportAttr.GetDataArray(exportStringData);

      if(exportStringData.GetCount() > 0){
         ESS_LOG_WARNING("exportStringData.GetCount() > 0");
      }

      CString exportString = exportStringData[0];
	 
      CStringArray exportArray = exportString.Split(",");

      //trim whitespace
      for(LONG i=0; i<exportArray.GetCount(); i++){
        std::string tstr(exportArray[i].GetAsciiString());
        exportArray[i] = tstr.c_str();
      }

      for(LONG i=0; i<exportArray.GetCount(); i++){
         ESS_LOG_WARNING("Read attribute "<<exportArray[i].GetAsciiString());

         ICEAttribute attr = geo.GetICEAttributeFromName(exportArray[i]);

         if(!attr.IsDefined() || !attr.IsValid()){
            continue;
         }

         XSI::siICENodeDataType datatype = attr.GetDataType();
         XSI::siICENodeStructureType datastruct = attr.GetStructureType();

         if(customProps.find(exportArray[i]) != customProps.end()){//skips attributes that have already been defined
            continue;
         }

         if(attr.GetDataType() == siICENodeDataString && attr.GetStructureType() == siICENodeStructureSingle){
            customProps[exportArray[i]] = Abc::OStringArrayProperty(argGeomParams, exportArray[i].GetAsciiString(), metadata, animatedTs);
         }
         else if(attr.GetDataType() == siICENodeDataFloat && attr.GetStructureType() == siICENodeStructureSingle){
            customProps[exportArray[i]] = Abc::OFloatArrayProperty(argGeomParams, exportArray[i].GetAsciiString(), metadata, animatedTs);
         }
         else if(attr.GetDataType() == siICENodeDataVector3 && attr.GetStructureType() == siICENodeStructureSingle){
            customProps[exportArray[i]] = Abc::OV3fArrayProperty(argGeomParams, exportArray[i].GetAsciiString(), metadata, animatedTs);
         }
      }
   }

   return CStatus::OK;
}

XSI::CStatus AlembicCustomAttributesEx::exportCustomAttributes(XSI::Geometry geo)
{
   for(propMap::iterator it = customProps.begin(); it != customProps.end(); it++){
      CString name = it->first;
      ICEAttribute attr = geo.GetICEAttributeFromName(it->first);

      if(!attr.IsDefined() || !attr.IsValid()){
         continue;
      }

      if(attr.GetDataType() == siICENodeDataString && attr.GetStructureType() == siICENodeStructureSingle){
         CICEAttributeDataArrayString stringArrayData;
         attr.GetDataArray(stringArrayData);

         //if(stringArrayData.GetCount() > 0){
         
         std::vector<std::string> outputStringArray;

         for(ULONG j=0; j<stringArrayData.GetCount(); j++){
            outputStringArray.push_back( stringArrayData[j].GetAsciiString() );
         }

         it->second.set(Abc::StringArraySample(outputStringArray));
         
      }
      else if(attr.GetDataType() == siICENodeDataFloat && attr.GetStructureType() == siICENodeStructureSingle){
         CICEAttributeDataArrayFloat floatArrayData;
         attr.GetDataArray(floatArrayData);

         std::vector<float> outputFloatArray;

         for(ULONG j=0; j<floatArrayData.GetCount(); j++){
            outputFloatArray.push_back( floatArrayData[j] );
         }

         it->second.set(Abc::FloatArraySample(outputFloatArray));
      }
      else if(attr.GetDataType() == siICENodeDataVector3 && attr.GetStructureType() == siICENodeStructureSingle){
         CICEAttributeDataArrayVector3f vecArrayData;
         attr.GetDataArray(vecArrayData);

         std::vector<Abc::V3f> outputVecArray;

         for(ULONG j=0; j<vecArrayData.GetCount(); j++){
            outputVecArray.push_back( Abc::V3f( vecArrayData[j].GetX(), vecArrayData[j].GetY(), vecArrayData[j].GetZ() ) );
         }

         it->second.set(Abc::V3fArraySample(outputVecArray));
      }
   }
   

   return CStatus::OK;
}

//
//XSI::CStatus exportCustomAttributes(XSI::Geometry geo)
//{
//   ICEAttribute exportAttr = geo.GetICEAttributeFromName(L"AlembicExportAttributeNames");
//   bool bDefined = exportAttr.IsDefined();
//   bool bValid = exportAttr.IsValid();
//   if(exportAttr.IsDefined() && exportAttr.IsValid()){
//
//     
//      
//      //TODO: add error checking
//
//         //CICEAttributeDataArrayVector3f velocitiesData;
//         //velocitiesAttr.GetDataArray(velocitiesData);
//
//      CICEAttributeDataArrayString exportStringData;
//      exportAttr.GetDataArray(exportStringData);
//
//      if(exportStringData.GetCount() > 0){
//         ESS_LOG_WARNING("exportStringData.GetCount() > 0");
//      }
//
//      CString exportString = exportStringData[0];
//	 
//      CStringArray exportArray = exportString.Split(",");
//
//      for(LONG i=0; i<exportArray.GetCount(); i++){
//         ESS_LOG_WARNING("Read attribute "<<exportArray[i].GetAsciiString());
//
//         ICEAttribute attr = geo.GetICEAttributeFromName(exportArray[i]);
//
//         if(!attr.IsDefined() || !exportAttr.IsValid()){
//            continue;
//         }
//
//         XSI::siICENodeDataType datatype = attr.GetDataType();
//         XSI::siICENodeStructureType datastruct = attr.GetStructureType();
//
//         if(attr.GetDataType() == siICENodeDataString && attr.GetStructureType() == siICENodeStructureSingle){
//            CICEAttributeDataArrayString stringArrayData;
//            attr.GetDataArray(stringArrayData);
//
//            if(stringArrayData.GetCount() > 0){
//               Abc::OStringArrayProperty abcAttrStrArray(argGeomParams, exportArray[i].GetAsciiString(), metadata, animatedTs);
//
//               //std::vector<std::string> outputStringArray;
//
//               //for(ULONG j=0; j<stringArrayData.GetCount(); j++){
//               //   outputStringArray.push_back( stringArrayData[j].GetAsciiString() );
//               //}
//
//               //abcAttrStrArray.set(Abc::StringArraySample(outputStringArray));
//            }
//         }
//         else if(attr.GetDataType() == siICENodeDataFloat && attr.GetStructureType() == siICENodeStructureSingle){
//            CICEAttributeDataArrayFloat floatArrayData;
//            attr.GetDataArray(floatArrayData);
//
//            if(floatArrayData.GetCount() > 0){
//
//            }
//
//            //Abc::OFloatArrayProperty abcAttrFloatArray(argGeomParams, exportArray[i].GetAsciiString(), metadata, animatedTs);
//
//
//         }
//         else if(attr.GetDataType() == siICENodeDataVector3 && attr.GetStructureType() == siICENodeStructureSingle){
//            CICEAttributeDataArrayVector3f vecArrayData;
//            attr.GetDataArray(vecArrayData);
//
//            if(vecArrayData.GetCount() > 0){
//
//            }
//
//
//            //Abc::OV3fArrayProperty abcAttrVec3Array(argGeomParams, exportArray[i].GetAsciiString(), metadata, animatedTs);
//
//
//         }
//      }
//   }
//
//   return CStatus::OK;
//}