#include "AlembicMetadataUtils.h"
#include <icustattribcontainer.h> 
#include <custattrib.h> 
#include "AlembicObject.h"


Alembic::Abc::ICompoundProperty getCompoundFromObject(Alembic::Abc::IObject& object)
{
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema();
   }
   return Alembic::Abc::ICompoundProperty();
}


void importMetadata(Alembic::AbcGeom::IObject& iObj)
{
	Alembic::AbcGeom::IObject* metadataChild = NULL;


	if(getCompoundFromObject(iObj).getPropertyHeader(".metadata") == NULL){
		return;
	}

	Alembic::Abc::IStringArrayProperty metaDataProp = Alembic::Abc::IStringArrayProperty( getCompoundFromObject(iObj), ".metadata" );
	Alembic::Abc::StringArraySamplePtr ptr = metaDataProp.getValue(0);
	//for(unsigned int i=0;i<ptr->size();i++){
	//	const char* name = ptr->get()[i].c_str();
	//	Alembic::Abc::StringArraySamplePtr ptr = metaDataProp.getValue(0);
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

	ExecuteMAXScriptScript( szBuffer );
	
	delete[] szBuffer;
}

Modifier* FindModifier(INode* node, char* name)
{
	int i = 0;
	int idx = 0;
	Modifier* pRetMod = NULL;
	while(true){
		Modifier* pMod;
		IDerivedObject* pObj = GET_MAX_INTERFACE()->FindModifier(*node, i, idx, pMod);
		if(!pObj){
			break;
		}

		if(strstr(pMod->GetName(), name) != NULL){
			pRetMod = pMod;
			break;
		}

		//const char* cname = pObj->GetClassName();
		//const char* oname = pMod->GetObjectName();
		//const char* name = pMod->GetName();
		i++;
	}

	return pRetMod;
}


void SaveMetaData(INode* node, AlembicObject* object)
{
	if(object == NULL){
		return;
	}
	if(object->GetNumSamples() > 0){
		return;
	}

	Modifier* pMod = FindModifier(node, "Metadata");
	if(!pMod){
		return;
	}
	
	ICustAttribContainer* cont = pMod->GetCustAttribContainer();
	if(!cont){
		return;
	}

	std::vector<std::string> metaData;

	for(int i=0; i<cont->GetNumCustAttribs(); i++)
	{
		CustAttrib* ca = cont->GetCustAttrib(i);
		const char* name = ca->GetName();
	
		IParamBlock2 *pblock = ca->GetParamBlockByID(0);
		if(pblock){
			int nNumParams = pblock->NumParams();
			for(int i=0; i<nNumParams; i++){

				ParamID id = pblock->IndextoID(i);
				//MSTR name = pblock->GetLocalName(id, 0);
				MSTR value = pblock->GetStr(id, 0);
				metaData.push_back(std::string(value));
			}
		}
	}

	if(metaData.size() > 0){
		Alembic::Abc::OStringArrayProperty metaDataProperty = Alembic::Abc::OStringArrayProperty(
		 object->GetCompound(), ".metadata", object->GetCompound().getMetaData(), object->GetCurrentJob()->GetAnimatedTs() );
		Alembic::Abc::StringArraySample metaDataSample(&metaData.front(),metaData.size());
		metaDataProperty.set(metaDataSample);
	}
}