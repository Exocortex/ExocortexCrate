#include "stdafx.h"
#include "Alembic.h"
#include "AlembicLightUtilities.h"
#include "utility.h"
#include <Alembic/AbcCoreAbstract/CompoundPropertyReader.h>
#include <Alembic/AbcCoreAbstract/ForwardDeclarations.h>
//#include <Alembic/Abc/IScalarProperty.h>

//template<class T> printProperty(


void printInputProperties( Abc::ICompoundProperty prop )
{
   if(!prop){
      return;
   }

   for(size_t i=0; i<prop.getNumProperties(); i++){
      AbcA::PropertyHeader pheader = prop.getPropertyHeader(i);
      AbcA::PropertyType propType = pheader.getPropertyType();

      

      if( propType == AbcA::kCompoundProperty ){
         //printInputProperties(Abc::ICompoundProperty(prop, pheader.getName()));
      }
      else if( propType == AbcA::kScalarProperty ){

         ESS_LOG_WARNING("Scaler property: "<<pheader.getName());
         
         if(Abc::IBoolProperty::matches(pheader)){
            Abc::IBoolProperty boolProp(prop, pheader.getName());
            
         }
      }
      else if( propType == AbcA::kArrayProperty ){
         ESS_LOG_WARNING("Unsupported array property: "<<pheader.getName());
      }
      else{
         ESS_LOG_WARNING("Unsupported input property: "<<pheader.getName());
      }

   }
}



int AlembicImport_Light(const std::string &path, AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode)
{



//#define OMNI_LIGHT_CLASS_ID  		0x1011
//#define SPOT_LIGHT_CLASS_ID  		0x1012
//#define DIR_LIGHT_CLASS_ID  		0x1013
//#define FSPOT_LIGHT_CLASS_ID  		0x1014
//#define TDIR_LIGHT_CLASS_ID  		0x1015

//#define OMNI_LIGHT		0	// Omnidirectional
//#define TSPOT_LIGHT		1	// Targeted
//#define DIR_LIGHT		2	// Directional
//#define FSPOT_LIGHT		3	// Free
//#define TDIR_LIGHT		4   // Targeted directional

   AbcG::ILight objLight = AbcG::ILight(iObj, Alembic::Abc::kWrapExisting);

   //CompoundPropertyReaderPtr propReader = objLight.getProperties();

   Abc::ICompoundProperty props = objLight.getProperties();

   for(int i=0; i<props.getNumProperties(); i++){
      Abc::PropertyHeader propHeader = props.getPropertyHeader(i);
      if(AbcM::IMaterialSchema::matches(propHeader)){
         AbcM::IMaterialSchema matSchema(props, propHeader.getName());
   
         ESS_LOG_WARNING("MaterialSchema present on light.");

         std::vector<std::string> targetNames;

         matSchema.getTargetNames(targetNames);

         for(int j=0; j<targetNames.size(); j++){
            ESS_LOG_WARNING("targetName: "<<targetNames[j]);

            std::vector<std::string> shaderTypeNames;
            matSchema.getShaderTypesForTarget(targetNames[j], shaderTypeNames);

            for(int k=0; k<shaderTypeNames.size(); k++){
               ESS_LOG_WARNING("shaderTypeNames: "<<shaderTypeNames[k]);
               
               std::string shaderName;
               matSchema.getShader(targetNames[j], shaderTypeNames[k], shaderName);

               ESS_LOG_WARNING("shaderName: "<<shaderName);

               Abc::ICompoundProperty propk = matSchema.getShaderParameters(targetNames[j], shaderTypeNames[k]);

               ESS_LOG_WARNING("propertyName: "<<propk.getName());
               
               printInputProperties(propk);
            }
         }
         break;
         
      }
   }

   // Create the object pNode
	INode *pNode = *pMaxNode;
	bool bReplaceExisting = false;
	if(!pNode){
      Object* newObject = reinterpret_cast<Object*>( GET_MAX_INTERFACE()->CreateInstance(LIGHT_CLASS_ID, Class_ID(OMNI_LIGHT_CLASS_ID, 0)) );
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