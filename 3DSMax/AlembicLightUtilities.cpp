#include "stdafx.h"
#include "Alembic.h"
#include "AlembicLightUtilities.h"
#include "utility.h"
#include <Alembic/AbcCoreAbstract/CompoundPropertyReader.h>
#include <Alembic/AbcCoreAbstract/ForwardDeclarations.h>
#include <Alembic/Util/PlainOldDataType.h>
//#include <Alembic/Abc/IScalarProperty.h>
#include "AlembicPropertyUtils.h"
#include "AlembicNames.h"
#include "AlembicCameraUtilities.h"



struct matShader
{
   std::string target;
   std::string type;
   

   std::string name;
   std::vector<AbcProp> props;

   matShader(std::string& target, std::string& type, std::string& name):target(target), type(type), name(name)
   {}

};

namespace InputLightType
{
   enum enumt{
      AREA_LIGHT,
      AMBIENT_LIGHT,
      NUM_INPUT_LIGHT_TYPES
   };
};

InputLightType::enumt readShader(AbcM::IMaterialSchema& matSchema, std::vector<matShader>& shaders)
{
   InputLightType::enumt ltype = InputLightType::NUM_INPUT_LIGHT_TYPES;

   std::vector<std::string> targetNames;

   matSchema.getTargetNames(targetNames);

   for(int j=0; j<targetNames.size(); j++){
      //ESS_LOG_WARNING("targetName: "<<targetNames[j]);

      std::vector<std::string> shaderTypeNames;
      matSchema.getShaderTypesForTarget(targetNames[j], shaderTypeNames);

      for(int k=0; k<shaderTypeNames.size(); k++){
         //ESS_LOG_WARNING("shaderTypeNames: "<<shaderTypeNames[k]);
         std::string shaderName;
         matSchema.getShader(targetNames[j], shaderTypeNames[k], shaderName);

         if(shaderName.find("rect") != std::string::npos || shaderName.find("area") != std::string::npos){
            ltype = InputLightType::AREA_LIGHT;
         }
         else if(shaderName.find("ambient") != std::string::npos){
            ltype = InputLightType::AMBIENT_LIGHT;
         }

         //std::stringstream nameStream;
         //nameStream<<targetNames[j]<<"."<<shaderTypeNames[k]<<"."<<shaderName;

         shaders.push_back( matShader(targetNames[j], shaderTypeNames[k], shaderName) );

         //ESS_LOG_WARNING("shaderName: "<<shaderName);

         Abc::ICompoundProperty propk = matSchema.getShaderParameters(targetNames[j], shaderTypeNames[k]);

         //ESS_LOG_WARNING("propertyName: "<<propk.getName());

         shaders.back().props.push_back(AbcProp(std::string("targetName"), targetNames[j], 1000000000));
         shaders.back().props.push_back(AbcProp(std::string("shaderType"), shaderTypeNames[j], 1000000000));
         
         readInputProperties(propk, shaders.back().props);
      }
   }

   return ltype;
}



AbcM::IMaterialSchema getMatSchema(AbcG::ILight& objLight)
{
   Abc::ICompoundProperty props = objLight.getProperties();

   for(int i=0; i<props.getNumProperties(); i++){
      Abc::PropertyHeader propHeader = props.getPropertyHeader(i);
      if(AbcM::IMaterialSchema::matches(propHeader)){
         return AbcM::IMaterialSchema(props, propHeader.getName());
      }
   }

   return AbcM::IMaterialSchema();
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

   if(options.attachToExisting){
      ESS_LOG_WARNING("Attach to existing for lights is not yet supported. Could not attach "<<iObj.getFullName());
      return alembic_success;
   }

   std::vector<matShader> shaders;

   AbcG::ILight objLight = AbcG::ILight(iObj, Alembic::Abc::kWrapExisting);

   std::string identifier = objLight.getFullName();

   //CompoundPropertyReaderPtr propReader = objLight.getProperties();

   Abc::ICompoundProperty props = objLight.getProperties();

   InputLightType::enumt lightType = InputLightType::NUM_INPUT_LIGHT_TYPES;

   for(int i=0; i<props.getNumProperties(); i++){
      Abc::PropertyHeader propHeader = props.getPropertyHeader(i);
      if(AbcM::IMaterialSchema::matches(propHeader)){
         AbcM::IMaterialSchema matSchema(props, propHeader.getName());
   
         //ESS_LOG_WARNING("MaterialSchema present on light.");

         lightType = readShader(matSchema, shaders);
      }

      //ESS_LOG_WARNING("name: "<<propHeader.getName());

      //if( AbcG::ICameraSchema::matches(propHeader) ){
      //   ESS_LOG_WARNING("Found light camera.");
      //   //AbcG::ICameraSchema camSchema(props, propHeader.getName());

      //}
   }



   bool bReplaceExisting = false;
   int nodeRes = alembic_failure;
   if(lightType == InputLightType::AMBIENT_LIGHT){
      nodeRes = createNode(iObj, LIGHT_CLASS_ID, Class_ID(OMNI_LIGHT_CLASS_ID, 0), pMaxNode, bReplaceExisting);

      //Modifier* pModifier = FindModifier(*pMaxNode, Class_ID(OMNI_LIGHT_CLASS_ID, 0));

      //if(pModifier){
      //   ESS_LOG_WARNING("NumParamBlocks: "<<pModifier->NumParamBlocks());
      //}
   
      //printControllers(*pMaxNode);

      //pMaxNode>GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );


      GET_MAX_INTERFACE()->SelectNode(*pMaxNode);

      //set the ambient check box, intensity controller, and light colour controller (not sure how to this in C++)
      std::stringstream evalStream;
      std::string modkey("");

      for(int s=0; s<shaders.size(); s++){
         std::string target = shaders[s].target;
         std::string type = shaders[s].type;

         for(int i=0; i<shaders[s].props.size(); i++){
            std::string propName = shaders[s].props[i].name;
            std::string& val = shaders[s].props[i].displayVal;
            bool& bConstant = shaders[s].props[i].bConstant;

            const AbcA::DataType& datatype = shaders[s].props[i].propHeader.getDataType();
            const AbcA::MetaData& metadata = shaders[s].props[i].propHeader.getMetaData();

            if(datatype.getPod() == AbcA::kFloat32POD){

               std::stringstream propStream;
               propStream<<target<<"."<<type<<"."<<propName;
               if(datatype.getExtent() == 1 && propName.find("intensity") != std::string::npos ){ //intensity property found, so attach controller
                  addFloatController(evalStream, options, modkey, std::string("multiplier"), path, iObj.getFullName(), propStream.str());
               }
               else if(datatype.getExtent() == 3 && propName.find("lightcolor") != std::string::npos ){ //color property found, so attach controller

                  std::stringstream xStream, yStream, zStream;

                  xStream<<propStream.str()<<"."<<metadata.get("interpretation")<<".x";
                  yStream<<propStream.str()<<"."<<metadata.get("interpretation")<<".y";
                  zStream<<propStream.str()<<"."<<metadata.get("interpretation")<<".z";

                  evalStream<<"$.rgb.controller = Color_RGB()\n";

                  addFloatController(evalStream, options, modkey, std::string("rgb.controller.r"), path, iObj.getFullName(), xStream.str());
                  addFloatController(evalStream, options, modkey, std::string("rgb.controller.g"), path, iObj.getFullName(), yStream.str());
                  addFloatController(evalStream, options, modkey, std::string("rgb.controller.b"), path, iObj.getFullName(), zStream.str());
               }
            }
            else{
               
            }
            evalStream<<"\n";
         }
      }

      evalStream<<"$.ambientOnly = true\n";
      ExecuteMAXScriptScript( EC_UTF8_to_TCHAR((char*)evalStream.str().c_str()));
   }
   else{//create a null, if we don't know what type of light this is
      nodeRes = createNode(iObj, HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID,0), pMaxNode, bReplaceExisting);
   }

   if(nodeRes == alembic_failure){
      return nodeRes;
   }


   GET_MAX_INTERFACE()->SelectNode(*pMaxNode);

   for(int i=0; i<shaders.size(); i++){

      std::sort(shaders[i].props.begin(), shaders[i].props.end(), sortFunc);

      Modifier* pMod = createDisplayModifier("Shader Properties", shaders[i].name, shaders[i].props);

      std::string target = shaders[i].target;
      std::string type = shaders[i].type;

      addControllersToModifier("Shader Properties", shaders[i].name, shaders[i].props, target, type, path, iObj.getFullName(), options);
   }

   // ----- TODO: add camera modifier
   //createCameraModifier(path, identifier, *pMaxNode);


   // ----- TODO: don't attach controllers for constant parameters


   //TODO: make the spinners read only



   return alembic_success;
}