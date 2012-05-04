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
	

	char szBuffer[10000];	
	sprintf_s( szBuffer, 10000, 
		"include \"Exocortex-Metadata.mcr\"\n"
		"CreateAlembicMetadataModifier $\n"
		"InitAlembicMetadataModifier $ \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \n",

		(0 < ptr->size()) ? ptr->get()[0].c_str() : "",
		(1 < ptr->size()) ? ptr->get()[1].c_str() : "",
		(2 < ptr->size()) ? ptr->get()[2].c_str() : "",
		(3 < ptr->size()) ? ptr->get()[3].c_str() : "",
		(4 < ptr->size()) ? ptr->get()[4].c_str() : "",
		(5 < ptr->size()) ? ptr->get()[5].c_str() : "",
		(6 < ptr->size()) ? ptr->get()[6].c_str() : "",
		(7 < ptr->size()) ? ptr->get()[7].c_str() : "",
		(8 < ptr->size()) ? ptr->get()[8].c_str() : "",
		(9 < ptr->size()) ? ptr->get()[9].c_str() : "",
		(10 < ptr->size()) ? ptr->get()[10].c_str() : "",
		(11 < ptr->size()) ? ptr->get()[11].c_str() : "",
		(12 < ptr->size()) ? ptr->get()[12].c_str() : "",
		(13 < ptr->size()) ? ptr->get()[13].c_str() : "",
		(14 < ptr->size()) ? ptr->get()[14].c_str() : "",
		(15 < ptr->size()) ? ptr->get()[15].c_str() : "",
		(16 < ptr->size()) ? ptr->get()[16].c_str() : "",
		(17 < ptr->size()) ? ptr->get()[17].c_str() : "",
		(18 < ptr->size()) ? ptr->get()[18].c_str() : "",
		(19 < ptr->size()) ? ptr->get()[19].c_str() : ""

		//"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20"
	);

	ExecuteMAXScriptScript( szBuffer );
	
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
	Modifier* pMod = FindModifier(node, "metadata");
	
	ICustAttribContainer* cont = pMod->GetCustAttribContainer();
	if(!cont){
		return;
	}

	std::vector<std::string> metaData(20);
	size_t offset = 0;

	//for(int i=0; i<cont->GetNumCustAttribs(); i++)
	//{
		CustAttrib* ca = cont->GetCustAttrib(0);
		//const char* name = ca->GetName();
	
		IParamBlock2 *pblock = ca->GetParamBlockByID(0);
		if(pblock){
			int nNumParams = pblock->NumParams();
			for(int i=0; i<nNumParams; i++){

				ParamID id = pblock->IndextoID(i);
				//MSTR name = pblock->GetLocalName(id, 0);
				MSTR value = pblock->GetStr(id, 0);
				metaData[offset++] = value;
			}
		}
	//}

	//Alembic::Abc::OStringArrayProperty metaDataProperty = Alembic::Abc::OStringArrayProperty(
	// object->GetCompound(), ".metadata", object->GetCompound().getMetaData(), object->GetCurrentJob()->GetAnimatedTs() );
	//Alembic::Abc::StringArraySample metaDataSample(&metaData.front(),metaData.size());
	//metaDataProperty.set(metaDataSample);
}