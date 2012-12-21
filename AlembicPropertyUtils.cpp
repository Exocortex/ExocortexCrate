#include "stdafx.h"
#include "AlembicPropertyUtils.h"
#include <sstream>
#include "AlembicMAXScript.h"

void createStringPropertyDisplayModifier(std::string modname, std::vector<std::pair<std::string, std::string>>& nameValuePairs)
{
   //Example usage:
   //
   //GET_MAX_INTERFACE()->SelectNode( pNode );
   //std::vector<std::pair<std::string, std::string>> nameValuePairs;
   //nameValuePairs.push_back(std::pair<std::string, std::string>("Prop1", "Prop1Val"));
   //nameValuePairs.push_back(std::pair<std::string, std::string>("Prop2", "Prop2Val"));
   //nameValuePairs.push_back(std::pair<std::string, std::string>("Prop3", "Prop3Val"));
   //nameValuePairs.push_back(std::pair<std::string, std::string>("Prop4", "Prop4Val"));
   //createPropertiesDisplayModifier(std::string("My Properties"), nameValuePairs);

   //the script assumes a single object is selected

   std::stringstream evalStream;
   evalStream<<"propModifier = EmptyModifier()"<<"\n";
   evalStream<<"propModifier.name = \""<<modname<<"\""<<"\n";
   evalStream<<"modCount = $.modifiers.count"<<"\n";
   evalStream<<"addmodifier $ propModifier before:modCount"<<"\n";
   evalStream<<"$.modifiers[\""<<modname<<"\"].enabled = false"<<"\n";

   evalStream<<"propAttribute = attributes propAttribute"<<"\n";
   evalStream<<"("<<"\n";

   evalStream<<"parameters propAttributePRM1 rollout:propAttributeRLT1"<<"\n";
   evalStream<<"("<<"\n";
   for(int i=0; i<nameValuePairs.size(); i++){
      std::string& name = nameValuePairs[i].first;
      std::string& val = nameValuePairs[i].second;
      evalStream<<name<<" type:#string ui:e"<<name<<" default:\""<<val<<"\""<<"\n";
   }
   evalStream<<")"<<"\n";
   
   evalStream<<"rollout propAttributeRLT1 \""<<modname<<"\""<<"\n";
   evalStream<<"("<<"\n";
   for(int i=0; i<nameValuePairs.size(); i++){
      std::string& name = nameValuePairs[i].first;
      //std::string& val = nameValuePairs[i].second;
      evalStream<<"edittext e"<<name<<" \""<<name<<"\" fieldWidth:140 labelOnTop:true"<<"\n";
   }
   evalStream<<")"<<"\n";
   evalStream<<")"<<"\n";


   evalStream<<"custattributes.add $.modifiers[\""<<modname<<"\"] propAttribute baseobject:false"<<"\n";

   ExecuteMAXScriptScript( EC_UTF8_to_TCHAR((char*)evalStream.str().c_str()) );
}

Modifier* createDisplayModifier(std::string modkey, std::string modname, std::vector<AbcProp>& props)
{
   ESS_PROFILE_FUNC();


   //the script assumes a single object is selected

   std::stringstream evalStream;
   evalStream<<"propModifier = EmptyModifier()"<<"\n";
   evalStream<<"propModifier.name = \""<<modkey<<"\""<<"\n";
   evalStream<<"modCount = $.modifiers.count"<<"\n";
   evalStream<<"addmodifier $ propModifier before:modCount"<<"\n";
   evalStream<<"$.modifiers[\""<<modkey<<"\"].enabled = false"<<"\n";

   evalStream<<"propAttribute = attributes propAttribute"<<"\n";
   evalStream<<"("<<"\n";

   evalStream<<"parameters propAttributePRM1 rollout:propAttributeRLT1"<<"\n";
   evalStream<<"("<<"\n";
   for(int i=0; i<props.size(); i++){
      std::string& name = props[i].name;
      std::string& val = props[i].displayVal;
      bool& bConstant = props[i].bConstant;

      const AbcA::DataType& datatype = props[i].propHeader.getDataType();
      const AbcA::MetaData& metadata = props[i].propHeader.getMetaData();

      if(datatype.getPod() == AbcA::kFloat32POD && datatype.getExtent() == 1){
         evalStream<<name<<" type:#float animatable:true ui:e"<<name<<" default:"<<val;
      }
      else if(datatype.getPod() == AbcA::kFloat32POD && datatype.getExtent() == 3){
         //if(metadata.get("interpretation") == std::string("rgb")){
         //   evalStream<<name<<" type:#color animatable:false ui:e"<<name<<" default:(["<<val<<"] as color)"; 
         //}
         //else{
            //evalStream<<name<<" type:#point3 animatable:false ui:e"<<name<<" ["<<val<<"]"; 
            evalStream<<name<<"x type:#float animatable:true ui:e"<<name<<"x default:300\n";
            evalStream<<name<<"y type:#float animatable:true ui:e"<<name<<"y default:300\n";
            evalStream<<name<<"z type:#float animatable:true ui:e"<<name<<"z default:300\n";
         //}
      }
      else{
         evalStream<<name<<" type:#string animatable:false ui:e"<<name<<" default:\""<<val<<"\"";
      }
      evalStream<<"\n";
   }
   evalStream<<")"<<"\n";
   
   evalStream<<"rollout propAttributeRLT1 \""<<modname<<"\""<<"\n";
   evalStream<<"("<<"\n";

   for(int i=0; i<props.size(); i++){
      std::string& name = props[i].name;
      bool& bConstant = props[i].bConstant;
      const AbcA::DataType& datatype = props[i].propHeader.getDataType();
      const AbcA::MetaData& metadata = props[i].propHeader.getMetaData();

      if(datatype.getPod() == AbcA::kFloat32POD && datatype.getExtent() == 1){
         evalStream<<"label lbl"<<name<<" \""<<name<<"\" align:#left fieldWidth:140\n";
         evalStream<<"spinner e"<<name<<" \"\" type:#float align:#left labelOnTop:true\n";
      }
      else if(datatype.getPod() == AbcA::kFloat32POD && datatype.getExtent() == 3){

         evalStream<<"label lbl"<<name<<" \""<<name<<"\" align:#left fieldWidth:140\n";
         evalStream<<"spinner e"<<name<<"x\"\" across:3 type:#float fieldWidth:39 readOnly:true\n";
         evalStream<<"spinner e"<<name<<"y\"\" type:#float fieldWidth:39\n";
         evalStream<<"spinner e"<<name<<"z\"\" type:#float fieldWidth:39\n";
      }
      else{
         evalStream<<"edittext e"<<name<<" \" "<<name<<"\" fieldWidth:140 labelOnTop:true";
         if(bConstant) evalStream<<" readOnly:true";
      }

      evalStream<<"\n";
   }

   //evalStream<<"on propAttributePRM1 changed do\n";
   //evalStream<<"("<<"\n";

   //for(int i=0; i<props.size(); i++){
   //   std::string& name = props[i].name;

   //   const AbcA::DataType& datatype = props[i].propHeader.getDataType();
   //   const AbcA::MetaData& metadata = props[i].propHeader.getMetaData();

   //   evalStream<<"e"<<name<<".text = "<<name<<" as string";
   //   evalStream<<"\n";
   //}

   //evalStream<<")"<<"\n";
   evalStream<<")"<<"\n";
   evalStream<<")"<<"\n";


   evalStream<<"custattributes.add $.modifiers[\""<<modkey<<"\"] propAttribute baseobject:false"<<"\n";

   

   //ESS_LOG_WARNING(evalStream.str());

   evalStream<<"$.modifiers[\""<<modkey<<"\"]\n";
   FPValue fpVal;
   ExecuteMAXScriptScript( EC_UTF8_to_TCHAR((char*)evalStream.str().c_str()), 0, &fpVal);
   Modifier* pMod = (Modifier*)fpVal.r;
   return pMod;
}

void addFloatController(bool loadTimeControl, std::stringstream& evalStream, const std::string& timeControlName,
                        const std::string& modkey, std::string propName, const std::string& file, const std::string& identifier, 
                        std::string propertyID, const std::string& propComponent=std::string(""), const std::string& propInterp=std::string(""))
{
   if(propComponent.length() > 0){
      propName += propComponent;
      propertyID += ".";
      propertyID += propInterp;
      propertyID += ".";
      propertyID += propComponent;
   }

   evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<propName<<".controller = AlembicFloatController()\n";
   evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<propName<<".controller.path = \""<<file<<"\"\n";
   evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<propName<<".controller.identifier = \""<<identifier<<"\"\n";
   evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<propName<<".controller.property = \""<<propertyID<<"\"\n";
   


   evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<propName<<".controller.time.controller = float_expression()\n";
   if(loadTimeControl){
      evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<propName<<".controller.time.controller.AddScalarTarget \"current\" $'"<<timeControlName<<"'.current.controller\n";
      evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<propName<<".controller.time.controller.AddScalarTarget \"offset\" $'"<<timeControlName<<"'.offset.controller\n";
      evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<propName<<".controller.time.controller.AddScalarTarget \"factor\" $'"<<timeControlName<<"'.factor.controller\n";
      evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<propName<<".controller.time.controller.setExpression \"current * factor + offset\"\n";
   }
   else{
      evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<propName<<".controller.time.controller.setExpression \"S\"\n";
   }

}


void addControllersToModifier(const std::string& modkey, const std::string& modname, std::vector<AbcProp>& props, 
                              const std::string& target, const std::string& type,
                              const std::string& file, const std::string& identifier, alembic_importoptions &options)
{
   ESS_PROFILE_FUNC();

   //the script assumes a single object is selected

   std::string timeControlName = EC_MCHAR_to_UTF8( options.pTimeControl->GetObjectName() );

   std::stringstream evalStream;

   for(int i=0; i<props.size(); i++){
      std::string& propName = props[i].name;
      std::string& val = props[i].displayVal;
      bool& bConstant = props[i].bConstant;

      const AbcA::DataType& datatype = props[i].propHeader.getDataType();
      const AbcA::MetaData& metadata = props[i].propHeader.getMetaData();

      if(datatype.getPod() == AbcA::kFloat32POD){

         std::stringstream propStream;
         propStream<<target<<"."<<type<<"."<<propName;
         if(datatype.getExtent() == 1){
            addFloatController(options.loadTimeControl, evalStream, timeControlName, modkey, propName, file, identifier, propStream.str());
         }
         else if(datatype.getExtent() == 3){
            addFloatController(options.loadTimeControl, evalStream, timeControlName, modkey, propName, file, identifier, propStream.str(), "x", metadata.get("interpretation"));
            addFloatController(options.loadTimeControl, evalStream, timeControlName, modkey, propName, file, identifier, propStream.str(), "y", metadata.get("interpretation"));
            addFloatController(options.loadTimeControl, evalStream, timeControlName, modkey, propName, file, identifier, propStream.str(), "z", metadata.get("interpretation"));
         }
      }
      else{
         
      }
      evalStream<<"\n";
   }

   

   //ESS_LOG_WARNING(evalStream.str());

   ExecuteMAXScriptScript( EC_UTF8_to_TCHAR((char*)evalStream.str().c_str()));
}