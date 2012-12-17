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
         if(metadata.get("interpretation") == std::string("rgb")){
            evalStream<<name<<" type:#color animatable:true ui:e"<<name<<" default:(["<<val<<"] as color)"; 
         }
         else{
            evalStream<<name<<" type:#point3 animatable:true ui:e"<<name<<" ["<<val<<"]"; 
         }
      }
      else{
         evalStream<<name<<" type:#string animatable:true ui:e"<<name<<" default:\""<<val<<"\"";
      }
      evalStream<<"\n";
   }
   evalStream<<")"<<"\n";
   
   evalStream<<"rollout propAttributeRLT1 \""<<modname<<"\""<<"\n";
   evalStream<<"("<<"\n";

   for(int i=0; i<props.size(); i++){
      std::string& name = props[i].name;
      bool& bConstant = props[i].bConstant;

      evalStream<<"edittext e"<<name<<" \" "<<name<<"\" fieldWidth:140 labelOnTop:true";
      if(bConstant) evalStream<<" readOnly:true";
      
      evalStream<<"\n";
   }

   evalStream<<"on propAttributeRLT1 open do\n";
   evalStream<<"("<<"\n";

   for(int i=0; i<props.size(); i++){
      std::string& name = props[i].name;

      const AbcA::DataType& datatype = props[i].propHeader.getDataType();
      const AbcA::MetaData& metadata = props[i].propHeader.getMetaData();

      evalStream<<"e"<<name<<".text = "<<name<<" as string";
      evalStream<<"\n";
   }

   evalStream<<")"<<"\n";
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

void addControllersToModifier(std::string modkey, std::string modname, std::vector<AbcProp>& props, 
                              const std::string& file, const std::string& identifier, alembic_importoptions &options)
{


   //the script assumes a single object is selected

   std::stringstream evalStream;

   for(int i=0; i<props.size(); i++){
      std::string& name = props[i].name;
      std::string& val = props[i].displayVal;
      bool& bConstant = props[i].bConstant;

      const AbcA::DataType& datatype = props[i].propHeader.getDataType();
      const AbcA::MetaData& metadata = props[i].propHeader.getMetaData();

      if(datatype.getPod() == AbcA::kFloat32POD && datatype.getExtent() == 1){
         evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<name<<".controller = AlembicFloatController()\n";
         evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<name<<".controller.path = \""<<file<<"\"\n";
         evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<name<<".controller.identifier = \""<<identifier<<"\"\n";
         
	     std::string objectNameName = EC_MCHAR_to_UTF8( options.pTimeControl->GetObjectName() );
	
         evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<name<<".controller.time.controller = float_expression()\n";
         evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<name<<".controller.time.controller.AddScalarTarget \"current\" $'"<<objectNameName.c_str()<<"'.current.controller\n";
         evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<name<<".controller.time.controller.AddScalarTarget \"offset\" $'"<<objectNameName.c_str()<<"'.offset.controller\n";
         evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<name<<".controller.time.controller.AddScalarTarget \"factor\" $'"<<objectNameName.c_str()<<"'.factor.controller\n";
         evalStream<<"$.modifiers[\""<<modkey<<"\"]."<<name<<".controller.time.controller.setExpression \"current * factor + offset\"\n";
      }
      else if(datatype.getPod() == AbcA::kFloat32POD && datatype.getExtent() == 3){

      }
      else{
         
      }
      evalStream<<"\n";
   }

   

   ESS_LOG_WARNING(evalStream.str());

   ExecuteMAXScriptScript( EC_UTF8_to_TCHAR((char*)evalStream.str().c_str()));
}