#include "stdafx.h"
#include "Alembic.h"
#include "AlembicLightUtilities.h"
#include "utility.h"

int AlembicImport_Light(const std::string &path, AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode)
{

//#define OMNI_LIGHT_CLASS_ID  		0x1011
//#define SPOT_LIGHT_CLASS_ID  		0x1012
//#define DIR_LIGHT_CLASS_ID  		0x1013
//#define FSPOT_LIGHT_CLASS_ID  		0x1014
//#define TDIR_LIGHT_CLASS_ID  		0x1015

   // Create the object pNode
	INode *pNode = *pMaxNode;
	bool bReplaceExisting = false;
	if(!pNode){
		Object* newObject = reinterpret_cast<Object*>(GET_MAX_INTERFACE()->CreateInstance(LIGHT_CLASS_ID, Class_ID(OMNI_LIGHT_CLASS_ID, 0)));
		if (newObject == NULL){
			return alembic_failure;
		}
      Abc::IObject parent = iObj.getParent();
      std::string name = removeXfoSuffix(iObj.getName().c_str());
      pNode = GET_MAX_INTERFACE()->CreateObjectNode(newObject, EC_UTF8_to_TCHAR(name.c_str()));
		if (pNode == NULL){
			return alembic_failure;
		}
		*pMaxNode = pNode;
	}
	else{
		bReplaceExisting = true;
	}



   return alembic_success;
}