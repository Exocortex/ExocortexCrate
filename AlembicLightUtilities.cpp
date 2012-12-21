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

//template<class T> printProperty(

template<class PT, class FT> bool readPropExt1(const Abc::ICompoundProperty& prop, const AbcA::PropertyHeader& pheader, std::string& val, bool& isConstant)
{
   if(PT::matches(pheader))
   {
      PT aProp(prop, pheader.getName());

      FT val1;
      aProp.get(val1);

      std::stringstream valStream;
      valStream<<val1;
      val = valStream.str();

      isConstant = aProp.isConstant();

      return true;
   }
   return false;
}

template<class PT, class FT> bool readPropExt3(const Abc::ICompoundProperty& prop, const AbcA::PropertyHeader& pheader, std::string& val, bool& isConstant)
{
   if(PT::matches(pheader))
   {
      PT aProp(prop, pheader.getName());

      FT val3;
      aProp.get(val3);

      std::stringstream valStream;
      valStream<<val3.x<<","<<val3.y<<","<<val3.z;
      val = valStream.str();

      return true;
   }
   return false;
}



void readInputProperties( Abc::ICompoundProperty prop, std::vector<AbcProp>& props )
{
   if(!prop){
      return;
   }

   for(size_t i=0; i<prop.getNumProperties(); i++){
      AbcA::PropertyHeader pheader = prop.getPropertyHeader(i);
      AbcA::PropertyType propType = pheader.getPropertyType();

      

      if( propType == AbcA::kCompoundProperty ){
         //printInputProperties(Abc::ICompoundProperty(prop, pheader.getName()));
         ESS_LOG_WARNING("Unsupported compound property: "<<pheader.getName());
      }
      else if( propType == AbcA::kScalarProperty ){

         //ESS_LOG_WARNING("Scaler property: "<<pheader.getName());
         //

         std::string displayVal;
         bool bConstant = true;

         if(Abc::IBoolProperty::matches(pheader)){

            //I need to know the name and type only if animated; an appropriate controller will handle reading the data.
            //If not animated, the value will set directly on the light and/or display modifier

            Abc::IBoolProperty boolProp(prop, pheader.getName());
            /*if(boolProp.isConstant()){*/
               AbcU::bool_t bVal = false;
               boolProp.get(bVal);
               if(bVal == true) displayVal = "true";
               else displayVal = "false";
            //}
            //else{
            //  
            //}
         }
         else if(readPropExt1<Abc::IFloatProperty, float>(prop, pheader, displayVal, bConstant));
         else if(readPropExt3<Abc::IC3fProperty, Abc::C3f>(prop, pheader, displayVal, bConstant));
         else if(readPropExt3<Abc::IV3fProperty, Abc::V3f>(prop, pheader, displayVal, bConstant));
         else if(readPropExt3<Abc::IN3fProperty, Abc::N3f>(prop, pheader, displayVal, bConstant));
         else if(Abc::IStringProperty::matches(pheader)){
            
            Abc::IStringProperty stringProp(prop, pheader.getName());
            stringProp.get(displayVal);
         }

         props.push_back(AbcProp(pheader.getName(), displayVal, pheader, bConstant));
      }
      else if( propType == AbcA::kArrayProperty ){
         ESS_LOG_WARNING("Unsupported array property: "<<pheader.getName());
      }
      else{
         ESS_LOG_WARNING("Unsupported input property: "<<pheader.getName());
      }

   }
}

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

         shaders.back().props.push_back(AbcProp(std::string("targetName"), targetNames[j]));
         shaders.back().props.push_back(AbcProp(std::string("shaderType"), shaderTypeNames[j]));
         
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

      ESS_LOG_WARNING("name: "<<propHeader.getName());

      if( AbcG::ICameraSchema::matches(propHeader) ){
         ESS_LOG_WARNING("Found light camera.");
         //AbcG::ICameraSchema camSchema(props, propHeader.getName());

      }
   }



   bool bReplaceExisting = false;
   int nodeRes = alembic_failure;
   if(lightType == InputLightType::AMBIENT_LIGHT){
      nodeRes = createNode(iObj, LIGHT_CLASS_ID, Class_ID(OMNI_LIGHT_CLASS_ID, 0), pMaxNode, bReplaceExisting);

      //connect intensity controller
      //connect light colour controller
      //set ambient check box
   }
   else{//create a null, if we don't know what type of light this is
      nodeRes = createNode(iObj, HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID,0), pMaxNode, bReplaceExisting);
   }

   if(nodeRes == alembic_failure){
      return nodeRes;
   }


   GET_MAX_INTERFACE()->SelectNode(*pMaxNode);

   //for(int i=0; i<shaders.size(); i++){
   //   Modifier* pMod = createDisplayModifier("Shader Properties", shaders[i].name, shaders[i].props);

   //   std::string target = shaders[i].target;
   //   std::string type = shaders[i].type;

   //   addControllersToModifier("Shader Properties", shaders[i].name, shaders[i].props, target, type, path, iObj.getFullName(), options);
   //}

   //TODO: add camera modifier

   createCameraModifier(path, identifier, *pMaxNode);

   //TODO: don't attach controllers for constant parameters

   //TODO: make the spinners read only

   //TODO: verify it works with attach to existing

   return alembic_success;
}