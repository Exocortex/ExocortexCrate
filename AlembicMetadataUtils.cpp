#include "stdafx.h"
#include "AlembicMetadataUtils.h"
#include "AlembicObject.h"
#include "utility.h"


void importMetadata(AbcG::IObject& iObj)
{
	AbcG::IObject* metadataChild = NULL;


	if(getCompoundFromObject(iObj).getPropertyHeader(".metadata") == NULL){
		return;
	}

	Abc::IStringArrayProperty metaDataProp = Abc::IStringArrayProperty( getCompoundFromObject(iObj), ".metadata" );
	Abc::StringArraySamplePtr ptr = metaDataProp.getValue(0);
	//for(unsigned int i=0;i<ptr->size();i++){
	//	const char* name = ptr->get()[i].c_str();
	//	Abc::StringArraySamplePtr ptr = metaDataProp.getValue(0);
	//
	//}

	std::string dataStr("#(");

	for(int i=0; i<ptr->size()-1; i++){
	
		dataStr += "\"";
		dataStr += ptr->get()[i];
		dataStr += "\", ";
	}
	dataStr += "\"";
	dataStr += ptr->get()[ptr->size()-1];
	dataStr += "\")";
	
	const size_t bufSize = dataStr.size() + 500;
	char* szBuffer = new char[bufSize];	
	sprintf_s( szBuffer, bufSize, 
		"include \"Exocortex-Metadata.mcr\" \n"
		"ImportMetadata $ %s \n",
		dataStr.c_str()
	);

	ExecuteMAXScriptScript( EC_UTF8_to_TCHAR( szBuffer ) );
	
	delete[] szBuffer;
}

void SaveMetaData(INode* node, AlembicObject* object)
{
	if(object == NULL){
		return;
	}
	if(object->GetNumSamples() > 0){
		return;
	}

	Modifier* pMod = FindModifier(node, Class_ID(0xd81fc3e, 0x1e4eacf5));

	bool bReadCustAttribs = false;
	if(!pMod){
		pMod = FindModifier(node, "Metadata");
		bReadCustAttribs = true;
	}

	if(!pMod){
		return;
	}

	std::vector<std::string> metaData;

	if(bReadCustAttribs){
	
		ICustAttribContainer* cont = pMod->GetCustAttribContainer();
		if(!cont){
			return;
		}
		for(int i=0; i<cont->GetNumCustAttribs(); i++)
		{
			CustAttrib* ca = cont->GetCustAttrib(i);
			std::string name = EC_MCHAR_to_UTF8( ca->GetName() );
		
			IParamBlock2 *pblock = ca->GetParamBlockByID(0);
			if(pblock){
				int nNumParams = pblock->NumParams();
				for(int i=0; i<nNumParams; i++){

					ParamID id = pblock->IndextoID(i);
					//MSTR name = pblock->GetLocalName(id, 0);
					MSTR value = pblock->GetStr(id, 0);
					metaData.push_back( EC_MSTR_to_UTF8( value ) );
				}
			}
		}

	}
	else{

		IParamBlock2 *pblock = pMod->GetParamBlockByID(0);
		if(pblock && pblock->NumParams() == 1){
			
			ParamID id = pblock->IndextoID(0);
			MSTR name = pblock->GetLocalName(id, 0);
			int nSize = pblock->Count(id);

			for(int i=0; i<nSize; i++){
				MSTR value = pblock->GetStr(id, 0, i);
				metaData.push_back( EC_MSTR_to_UTF8( value ) );
			}
		}

	}

	if(metaData.size() > 0){
		Abc::OStringArrayProperty metaDataProperty = Abc::OStringArrayProperty(
		 object->GetCompound(), ".metadata", object->GetCompound().getMetaData(), object->GetCurrentJob()->GetAnimatedTs() );
		Abc::StringArraySample metaDataSample(&metaData.front(),metaData.size());
		metaDataProperty.set(metaDataSample);
	}
}