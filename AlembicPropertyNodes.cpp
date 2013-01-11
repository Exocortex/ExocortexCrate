#include "stdafx.h"

#include "AlembicPropertyNodes.h"



/// ICE NODE!
enum IDs
{
	ID_IN_path = 10,
	ID_IN_identifier = 11,
    ID_IN_property = 12,
	ID_IN_time = 14,
	ID_G_100 = 1005,
	ID_OUT_data = 12771,
    ID_OUT_valid = 12772,


	ID_TYPE_CNS = 1400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
};

#define ID_UNDEF ((ULONG)-1)

using namespace XSI;


void getParams(ICENodeContext& in_ctxt, CString& path, CString& identifier, CString& aproperty, double& time){
	CDataArrayString pathData( in_ctxt, ID_IN_path );
	path = pathData[0];
	CDataArrayString identifierData( in_ctxt, ID_IN_identifier );
	identifier = identifierData[0];
	CDataArrayString propertyData( in_ctxt, ID_IN_property );
	aproperty = propertyData[0];
	CDataArrayFloat timeData( in_ctxt, ID_IN_time);
	time = timeData[0];
}

void addArchiveRef(ICENodeContext& in_ctxt, CString path)
{
	// check if we need t addref the archive
	CValue udVal = in_ctxt.GetUserData();
	ArchiveInfo * p = (ArchiveInfo*)(CValue::siPtrType)udVal;
	if(p == NULL)
	{
		p = new ArchiveInfo;
		p->path = path.GetAsciiString();
		addRefArchive(path);
		CValue val = (CValue::siPtrType) p;
		in_ctxt.PutUserData( val ) ;
	}
}

void delArchiveRef(Context& in_ctxt)
{
	CValue udVal = in_ctxt.GetUserData();
	ArchiveInfo * p = (ArchiveInfo*)(CValue::siPtrType)udVal;
	if(p != NULL)
	{
		delRefArchive(p->path);
		delete(p);
	}
}

bool findProperty(Abc::ICompoundProperty& arbGeomParams, AbcA::PropertyHeader& propHeader, CString aproperty)
{
    for(size_t i=0; i<arbGeomParams.getNumProperties(); i++){
       AbcA::PropertyHeader pheader = arbGeomParams.getPropertyHeader(i);
       if(pheader.getName() == std::string(aproperty.GetAsciiString())){
          propHeader = pheader;
          return true;
       }
    } 
    return false;
}

namespace writeArrayRes
{
   enum enumT
   {
      SUCCESS,
      EMPTY,
      TYPE_MISMATCH
   };
};

template<class PROP, class SAMPLER, class TYPE> writeArrayRes::enumT writeArray3f(SampleInfo& sampleInfo, Abc::ICompoundProperty arbGeomParams, AbcA::PropertyHeader propHeader, 
                                             CDataArray2DVector3f outData, CDataArray2DVector3f::Accessor acc)
{
   if(PROP::matches(propHeader)){
      PROP propArray(arbGeomParams, propHeader.getName());
      SAMPLER propPtr1 = propArray.getValue(sampleInfo.floorIndex);

      if(propPtr1 == NULL || propPtr1->size() == 0){
         acc = outData.Resize(0,0);
         return writeArrayRes::EMPTY;
      }

      SAMPLER propPtr2 = propArray.getValue(sampleInfo.ceilIndex);

      acc = outData.Resize(0, (ULONG)propPtr1->size());

      const float t = (float)sampleInfo.alpha;

      if(sampleInfo.alpha != 0.0 && propPtr1->size() == propPtr2->size()){
         for(ULONG i=0; i<acc.GetCount(); i++){
            TYPE c1 = propPtr1->get()[i];
            TYPE c2 = propPtr2->get()[i];
            TYPE c = c2 + (1.0f - t)*c1;
            acc[i].Set(c.x , c.y, c.z);
         }
      }
      else{
         for(ULONG i=0; i<acc.GetCount(); i++){
            TYPE c = propPtr1->get()[i];
            acc[i].Set(c.x, c.y, c.z);
         }
      }

      return writeArrayRes::SUCCESS;
   }

   return writeArrayRes::TYPE_MISMATCH;
}


template<class PROP, class SAMPLER, class TYPE>  writeArrayRes::enumT writeArray1f(SampleInfo& sampleInfo, Abc::ICompoundProperty arbGeomParams, AbcA::PropertyHeader propHeader, 
                                             CDataArray2DFloat outData, CDataArray2DFloat::Accessor acc)
{
   if(PROP::matches(propHeader)){
      PROP propArray(arbGeomParams, propHeader.getName());
      SAMPLER propPtr1 = propArray.getValue(sampleInfo.floorIndex);

      if(propPtr1 == NULL || propPtr1->size() == 0){
         acc = outData.Resize(0,0);
         return writeArrayRes::EMPTY;
      }

      SAMPLER propPtr2 = propArray.getValue(sampleInfo.ceilIndex);

      acc = outData.Resize(0, (ULONG)propPtr1->size());

      const float t = (float)sampleInfo.alpha;

      if(sampleInfo.alpha != 0.0 && propPtr1->size() == propPtr2->size()){
         for(ULONG i=0; i<acc.GetCount(); i++){
            TYPE c1 = propPtr1->get()[i];
            TYPE c2 = propPtr2->get()[i];
            TYPE c = c2 + (1.0f - t)*c1;
            acc[i] = c;
         }
      }
      else{
         for(ULONG i=0; i<acc.GetCount(); i++){
            TYPE c = propPtr1->get()[i];
            acc[i] = c;
         }
      }

      return writeArrayRes::SUCCESS;
   }

   return writeArrayRes::TYPE_MISMATCH;
}


XSI::CStatus defineNode(XSI::PluginRegistrar& in_reg, ULONG in_nDataType, ULONG in_nStructType, CString name)
{
	//Application().LogMessage( "Register_alembic_polyMesh" );
	ICENodeDef nodeDef = Application().GetFactory().CreateICENodeDef(name, name);

	CStatus st = nodeDef.PutColor(255, 188, 102);
	st.AssertSucceeded( ) ;

	st = nodeDef.PutThreadingModel(XSI::siICENodeSingleThreading);
	st.AssertSucceeded( ) ;

	// Add input ports and groups.
	st = nodeDef.AddPortGroup(ID_G_100);
	st.AssertSucceeded( ) ;

	st = nodeDef.AddInputPort(ID_IN_path,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"path",L"path",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
	st = nodeDef.AddInputPort(ID_IN_identifier,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"identifier",L"identifier",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
	st = nodeDef.AddInputPort(ID_IN_property,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"property",L"property",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
	st = nodeDef.AddInputPort(ID_IN_time,ID_G_100,siICENodeDataFloat,siICENodeStructureSingle,siICENodeContextSingleton,L"time",L"time",0.0f,ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;


	// Add output ports.
	st = nodeDef.AddOutputPort(ID_OUT_data, in_nDataType, in_nStructType, siICENodeContextSingleton, L"data",	L"data");
	st.AssertSucceeded( ) ;
	st = nodeDef.AddOutputPort(ID_OUT_valid, siICENodeDataBool,	siICENodeStructureSingle, siICENodeContextSingleton, L"valid",	L"valid");
	st.AssertSucceeded( ) ;


	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(L"Custom ICENode");

    return CStatus::OK;
}

//    AbcA::PropertyType propType = propHeader.getPropertyType();
//
//    if( propType == AbcA::kScalarProperty){
//       if(Abc::IStringProperty::matches(propHeader)){
//          Abc::IStringProperty stringProp(arbGeomParams, propHeader.getName());
//          std::string val = stringProp.getValue(sampleInfo.floorIndex);
//          
//       }
//       else if(Abc::IFloatProperty::matches(propHeader)){
//          Abc::IFloatProperty floatProp(arbGeomParams, propHeader.getName());
//          float val = floatProp.getValue(sampleInfo.floorIndex);
//       }
//       else if(Abc::IC3fProperty::matches(propHeader)){
//          Abc::IC3fProperty colorProp(arbGeomParams, propHeader.getName());
//          Abc::C3f val = colorProp.getValue(sampleInfo.floorIndex);
//       }
//       else if(Abc::IV3fProperty::matches(propHeader)){
//          Abc::IV3fProperty vecProp(arbGeomParams, propHeader.getName());
//          Abc::V3f val = vecProp.getValue(sampleInfo.floorIndex);
//       }
//       else if(Abc::IN3fProperty::matches(propHeader)){
//          Abc::IN3fProperty normalProp(arbGeomParams, propHeader.getName());
//          Abc::N3f val = normalProp.getValue(sampleInfo.floorIndex);
//       }
//    }
//    else if( propType == AbcA::kArrayProperty ){
///*       if(Abc::IStringProperty::matches(propHeader)){
//          Abc::IStringProperty stringProp(arbGeomParams, propHeader.getName());
//          std::string val = stringProp.getValue(sampleInfo.floorIndex);
//          
//       }
//       else */if(Abc::IFloatArrayProperty::matches(propHeader)){
//          Abc::IFloatArrayProperty floatProp(arbGeomParams, propHeader.getName());
//          
//       }
//       else if(Abc::IC3fProperty::matches(propHeader)){
//          Abc::IC3fProperty colorProp(arbGeomParams, propHeader.getName());
//          Abc::C3f val = colorProp.getValue(sampleInfo.floorIndex);
//       }
//       else if(Abc::IV3fProperty::matches(propHeader)){
//          Abc::IV3fProperty vecProp(arbGeomParams, propHeader.getName());
//          Abc::V3f val = vecProp.getValue(sampleInfo.floorIndex);
//       }
//       else if(Abc::IN3fProperty::matches(propHeader)){
//          Abc::IN3fProperty normalProp(arbGeomParams, propHeader.getName());
//          Abc::N3f val = normalProp.getValue(sampleInfo.floorIndex);
//       }
//    }



XSIPLUGINCALLBACK CStatus alembic_vec3f_array_Evaluate(ICENodeContext& in_ctxt)
{
	//Application().LogMessage( "alembic_polyMesh2_Evaluate" );
	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	CString path, identifier, aproperty;
	double time;
    getParams(in_ctxt, path, identifier, aproperty, time);

    addArchiveRef(in_ctxt, path);

	AbcG::IObject iObj = getObjectFromArchive(path,identifier);
    if(!iObj.valid()){
        ESS_LOG_WARNING("vec3f_array node error: Could not find "<<iObj.getFullName());
		return CStatus::OK;//return error instead (so that node shows up as red)?
    }  

	AbcA::TimeSamplingPtr timeSampling;
	int nSamples;
    Abc::ICompoundProperty arbGeomParams = getArbGeomParams(iObj, timeSampling, nSamples);

	SampleInfo sampleInfo = getSampleInfo( time, timeSampling, nSamples );

    AbcA::PropertyHeader propHeader;

    if(!findProperty(arbGeomParams, propHeader, aproperty)){ 
       ESS_LOG_WARNING("vec3f_array node error: Could not find "<<aproperty.GetAsciiString());
       return CStatus::OK;
    }

	switch( out_portID )
	{
    case ID_OUT_valid:
       {
            CDataArrayBool outData( in_ctxt );
            outData.Set( 0, true );
       }
       break;
	case ID_OUT_data:
		{
			CDataArray2DVector3f outData( in_ctxt );
			CDataArray2DVector3f::Accessor acc;

            writeArrayRes::enumT res = writeArray3f<Abc::IC3fArrayProperty, Abc::C3fArraySamplePtr, Abc::C3f>(sampleInfo, arbGeomParams, propHeader, outData, acc);
            if(res != writeArrayRes::SUCCESS){
               res = writeArray3f<Abc::IV3fArrayProperty, Abc::V3fArraySamplePtr, Abc::V3f>(sampleInfo, arbGeomParams, propHeader, outData, acc);
            }
            if(res != writeArrayRes::SUCCESS){
               res = writeArray3f<Abc::IN3fArrayProperty, Abc::N3fArraySamplePtr, Abc::N3f>(sampleInfo, arbGeomParams, propHeader, outData, acc);
            }
            if(res == writeArrayRes::TYPE_MISMATCH){
               ESS_LOG_WARNING("vec3f_array node error: type mismatch "<<aproperty.GetAsciiString());
            }

		}
		break;

	}

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_vec3f_array_Term(CRef& in_ctxt)
{
    Context ctxt( in_ctxt );
	delArchiveRef(ctxt);
	return CStatus::OK;
}

XSI::CStatus Register_alembic_vec3f_array( XSI::PluginRegistrar& in_reg )
{
   return defineNode(in_reg, siICENodeDataVector3, siICENodeStructureArray, L"alembic_vec3f_array");
}



XSIPLUGINCALLBACK CStatus alembic_float_array_Evaluate(ICENodeContext& in_ctxt)
{
	//Application().LogMessage( "alembic_polyMesh2_Evaluate" );
	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	CString path, identifier, aproperty;
	double time;
    getParams(in_ctxt, path, identifier, aproperty, time);

    addArchiveRef(in_ctxt, path);

	AbcG::IObject iObj = getObjectFromArchive(path,identifier);
    if(!iObj.valid()){
        ESS_LOG_WARNING("float_array node error: Could not find "<<iObj.getFullName());
		return CStatus::OK;//return error instead (so that node shows up as red)?
    }

	AbcA::TimeSamplingPtr timeSampling;
	int nSamples;
    Abc::ICompoundProperty arbGeomParams = getArbGeomParams(iObj, timeSampling, nSamples);

	SampleInfo sampleInfo = getSampleInfo( time, timeSampling, nSamples );

    AbcA::PropertyHeader propHeader;

    if(!findProperty(arbGeomParams, propHeader, aproperty)){ 
       ESS_LOG_WARNING("float_array node error: Could not find "<<aproperty.GetAsciiString());
       return CStatus::OK;
    }

	switch( out_portID )
	{
    case ID_OUT_valid:
       {
            CDataArrayBool outData( in_ctxt );
            outData.Set( 0, true );
       }
       break;
	case ID_OUT_data:
		{
			CDataArray2DFloat outData( in_ctxt );
			CDataArray2DFloat::Accessor acc;

            writeArrayRes::enumT res = writeArray1f<Abc::IFloatArrayProperty, Abc::FloatArraySamplePtr, float>(sampleInfo, arbGeomParams, propHeader, outData, acc);
            
            if(res == writeArrayRes::TYPE_MISMATCH){
               ESS_LOG_WARNING("vec3f_array node error: type mismatch "<<aproperty.GetAsciiString());
            }
		}
		break;

	}

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_float_array_Term(CRef& in_ctxt)
{
    Context ctxt( in_ctxt );
	delArchiveRef(ctxt);
	return CStatus::OK;
}

XSI::CStatus Register_alembic_float_array( XSI::PluginRegistrar& in_reg )
{
   return defineNode(in_reg, siICENodeDataFloat, siICENodeStructureArray, L"alembic_float_array");
}



XSIPLUGINCALLBACK CStatus alembic_string_array_Evaluate(ICENodeContext& in_ctxt)
{
	//Application().LogMessage( "alembic_polyMesh2_Evaluate" );
	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	CString path, identifier, aproperty;
	double time;
    getParams(in_ctxt, path, identifier, aproperty, time);

    addArchiveRef(in_ctxt, path);

	AbcG::IObject iObj = getObjectFromArchive(path,identifier);
    if(!iObj.valid()){
        ESS_LOG_WARNING("string node error: Could not find "<<iObj.getFullName());
		return CStatus::OK;//return error instead (so that node shows up as red)?
    }

	AbcA::TimeSamplingPtr timeSampling;
	int nSamples;
    Abc::ICompoundProperty arbGeomParams = getArbGeomParams(iObj, timeSampling, nSamples);

	SampleInfo sampleInfo = getSampleInfo( time, timeSampling, nSamples );

    AbcA::PropertyHeader propHeader;

    if(!findProperty(arbGeomParams, propHeader, aproperty)){  
       ESS_LOG_WARNING("string node error: Could not find "<<aproperty.GetAsciiString());
       return CStatus::OK;
    }

	switch( out_portID )
	{
    case ID_OUT_valid:
       {
            CDataArrayBool outData( in_ctxt );
            outData.Set( 0, true );
       }
       break;
	case ID_OUT_data:
		{
			CDataArray2DString outData( in_ctxt );
			CDataArray2DString::Accessor acc;

			
            if(Abc::IStringArrayProperty::matches(propHeader)){      
               Abc::IStringArrayProperty propArray(arbGeomParams, propHeader.getName());
               Abc::StringArraySamplePtr propPtr1 = propArray.getValue(sampleInfo.floorIndex);

               if(propPtr1 == NULL || propPtr1->size() == 0){
                  acc = outData.Resize(0,0);
                  return CStatus::OK;
               }

               acc = outData.Resize(0, (ULONG)propPtr1->size());

               for(ULONG i=0; i<acc.GetCount(); i++){
                  std::string c = propPtr1->get()[i];
                  acc[i] = CString(c.c_str());
               }
               
            }
            else{
               ESS_LOG_WARNING("string node error: type mismatch "<<aproperty.GetAsciiString());
            }
		}
		break;

	}

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_string_array_Term(CRef& in_ctxt)
{
    Context ctxt( in_ctxt );
	delArchiveRef(ctxt);
	return CStatus::OK;
}

XSI::CStatus Register_alembic_string_array( XSI::PluginRegistrar& in_reg )
{
   return defineNode(in_reg, siICENodeDataString, siICENodeStructureArray, L"alembic_string_array");
}