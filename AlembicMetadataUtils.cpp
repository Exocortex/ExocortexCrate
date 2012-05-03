#include "AlembicMetadataUtils.h"
#include "AlembicMax.h"

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
		"AlembicMetaDataModifier = EmptyModifier()\n"
		"AlembicMetaDataModifier.name = \"alembic_metadata\"\n"
		"addmodifier $ AlembicMetaDataModifier\n"
		"AlembicMetadataCA = attributes AlembicMetadata\n"
		"(\n"
			"parameters AlembicMetadataPRM rollout:AlembicMetadataRLT\n"
			"(\n"
				"name0 type:#string ui:eName0 default:\"%s\"\n"
				"value0 type:#string ui:eValue0 default:\"%s\"\n"
				"name1 type:#string ui:eName1 default:\"%s\"\n"
				"value1 type:#string ui:eValue1 default:\"%s\"\n"
				"name2 type:#string ui:eName2 default:\"%s\"\n"
				"value2 type:#string ui:eValue2 default:\"%s\"\n"
				"name3 type:#string ui:eName3 default:\"%s\"\n"
				"value3 type:#string ui:eValue3 default:\"%s\"\n"
				"name4 type:#string ui:eName4 default:\"%s\"\n"
				"value4 type:#string ui:eValue4 default:\"%s\"\n"
				"name5 type:#string ui:eName5 default:\"%s\"\n"
				"value5 type:#string ui:eValue5 default:\"%s\"\n"
				"name6 type:#string ui:eName6 default:\"%s\"\n"
				"value6 type:#string ui:eValue6 default:\"%s\"\n"
				"name7 type:#string ui:eName7 default:\"%s\"\n"
				"value7 type:#string ui:eValue7 default:\"%s\"\n"
				"name8 type:#string ui:eName8 default:\"%s\"\n"
				"value8 type:#string ui:eValue8 default:\"%s\"\n"
				"name9 type:#string ui:eName9 default:\"%s\"\n"
				"value9 type:#string ui:eValue9 default:\"%s\"\n"
			")\n"
			"rollout AlembicMetadataRLT \"alembic_metadata\"\n"
			"(\n"
				"edittext eName0 \"n0\" fieldWidth:120 labelOnTop:false\n"
				"edittext eValue0 \"v0\" fieldWidth:120 labelOnTop:false\n"
				"edittext eName1 \"n1\" fieldWidth:120 labelOnTop:false\n"
				"edittext eValue1 \"v1\" fieldWidth:120 labelOnTop:false\n"
				"edittext eName2 \"n2\" fieldWidth:120 labelOnTop:false\n"
				"edittext eValue2 \"v2\" fieldWidth:120 labelOnTop:false\n"
				"edittext eName3 \"n3\" fieldWidth:120 labelOnTop:false\n"
				"edittext eValue3 \"v3\" fieldWidth:120 labelOnTop:false\n"
				"edittext eName4 \"n4\" fieldWidth:120 labelOnTop:false\n"
				"edittext eValue4 \"v4\" fieldWidth:120 labelOnTop:false\n"
				"edittext eName5 \"n5\" fieldWidth:120 labelOnTop:false\n"
				"edittext eValue5 \"v5\" fieldWidth:120 labelOnTop:false\n"
				"edittext eName6 \"n6\" fieldWidth:120 labelOnTop:false\n"
				"edittext eValue6 \"v6\" fieldWidth:120 labelOnTop:false\n"
				"edittext eName7 \"n7\" fieldWidth:120 labelOnTop:false\n"
				"edittext eValue7 \"v7\" fieldWidth:120 labelOnTop:false\n"
				"edittext eName8 \"n8\" fieldWidth:120 labelOnTop:false\n"
				"edittext eValue8 \"v8\" fieldWidth:120 labelOnTop:false\n"
				"edittext eName9 \"n9\" fieldWidth:120 labelOnTop:false\n"
				"edittext eValue9 \"v9\" fieldWidth:120 labelOnTop:false\n"
			")\n"
		")\n"
		"custattributes.add $.modifiers[\"alembic_metadata\"] AlembicMetadataCA baseobject:false\n", 

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
