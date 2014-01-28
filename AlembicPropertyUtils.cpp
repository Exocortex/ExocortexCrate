#include "stdafx.h"
#include "AlembicPropertyUtils.h"
#include <sstream>
#include "AlembicMAXScript.h"
#include "Utility.h"
#include "AlembicObject.h"
#include "CommonUtilities.h"


void createStringPropertyDisplayModifier(std::string modname, std::vector<std::pair<std::string, std::string>>& nameValuePairs)
{
   //Example usage:
   //
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

Modifier* createDisplayModifier(std::string modkey, std::string modname, std::vector<AbcProp>& props, INode* pNode)
{
   ESS_PROFILE_FUNC();

   //the script assumes a single object is selected
   std::stringstream evalStream;
   std::string nodeName("$");
   if(pNode){
      evalStream<<GET_MAXSCRIPT_NODE(pNode);
      nodeName = std::string("mynode2113");
   }
   evalStream<<"propModifier = EmptyModifier()"<<"\n";
   evalStream<<"propModifier.name = \""<<modkey<<"\""<<"\n";
   evalStream<<"modCount = "<<nodeName<<".modifiers.count"<<"\n";
   evalStream<<"addmodifier "<<nodeName<<" propModifier before:modCount"<<"\n";
   evalStream<<nodeName<<".modifiers[\""<<modkey<<"\"].enabled = false"<<"\n";

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

      if(datatype.getPod() == AbcA::kInt32POD && datatype.getExtent() == 1){
         evalStream<<name<<" type:#integer animatable:true ui:e"<<name<<" default:"<<val;
      }
      else if(datatype.getPod() == AbcA::kFloat32POD && datatype.getExtent() == 1){
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
      const int nSize = (const int) props[i].displayVal.size();

      if(datatype.getPod() == AbcA::kInt32POD && datatype.getExtent() == 1){
         evalStream<<"label lbl"<<name<<" \""<<name<<"\" align:#left fieldWidth:140\n";
         evalStream<<"spinner e"<<name<<" \"\" type:#integer range:[-9999999,9999999,0] align:#left labelOnTop:true\n";
      }
      else if(datatype.getPod() == AbcA::kFloat32POD && datatype.getExtent() == 1){
         evalStream<<"label lbl"<<name<<" \""<<name<<"\" align:#left fieldWidth:140\n";
         evalStream<<"spinner e"<<name<<" \"\" type:#float range:[-9999999,9999999,0]align:#left labelOnTop:true\n";
      }
      else if(datatype.getPod() == AbcA::kFloat32POD && datatype.getExtent() == 3){

         evalStream<<"label lbl"<<name<<" \""<<name<<"\" align:#left fieldWidth:140\n";
         evalStream<<"spinner e"<<name<<"x\"\" across:3 type:#float range:[-9999999,9999999,0] fieldWidth:39 readOnly:true\n";
         evalStream<<"spinner e"<<name<<"y\"\" type:#float range:[-9999999,9999999,0] fieldWidth:39\n";
         evalStream<<"spinner e"<<name<<"z\"\" type:#float range:[-9999999,9999999,0] fieldWidth:39\n";
      }
      else{
         //TODO: better fit for large strings
         evalStream<<"edittext e"<<name<<" \" "<<name<<"\" fieldWidth:140 ";
         if(nSize > 140){
            evalStream<<"height:54";
         }
         evalStream<<" labelOnTop:true";
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


   evalStream<<"custattributes.add "<<nodeName<<".modifiers[\""<<modkey<<"\"] propAttribute baseobject:false"<<"\n";

   

   //ESS_LOG_WARNING(evalStream.str());

   evalStream<<nodeName<<".modifiers[\""<<modkey<<"\"]\n";
   FPValue fpVal;
   ExecuteMAXScriptScript( EC_UTF8_to_TCHAR((char*)evalStream.str().c_str()), 0, &fpVal);
   Modifier* pMod = (Modifier*)fpVal.r;
   return pMod;
}

void addFloatController(std::stringstream& evalStream, alembic_importoptions &options,
                        const std::string& modkey, std::string propName, const std::string& file, const std::string& identifier, 
                        std::string propertyID/*, const std::string& propComponent=std::string(""), const std::string& propInterp=std::string("")*/)
{
   //if(propComponent.length() > 0){
   //   propName += propComponent;
   //   propertyID += ".";
   //   propertyID += propInterp;
   //   propertyID += ".";
   //   propertyID += propComponent;
   //}

   std::string mkStr;
   if(!modkey.empty()){
      std::stringstream mkStream;
      mkStream<<".modifiers[\""<<modkey<<"\"]";
      mkStr = mkStream.str();
   }

   std::string timeControlName;
   if(options.pTimeControl){
      timeControlName = EC_MCHAR_to_UTF8( options.pTimeControl->GetObjectName() );
   }

   evalStream<<"$"<<mkStr<<"."<<propName<<".controller = AlembicFloatController()\n";
   evalStream<<"$"<<mkStr<<"."<<propName<<".controller.path = \""<<file<<"\"\n";
   evalStream<<"$"<<mkStr<<"."<<propName<<".controller.identifier = \""<<identifier<<"\"\n";
   evalStream<<"$"<<mkStr<<"."<<propName<<".controller.property = \""<<propertyID<<"\"\n";
   


   evalStream<<"$"<<mkStr<<"."<<propName<<".controller.time.controller = float_expression()\n";
   if(options.loadTimeControl){
      evalStream<<"$"<<mkStr<<"."<<propName<<".controller.time.controller.AddScalarTarget \"current\" $'"<<timeControlName<<"'.time.controller\n";
      evalStream<<"$"<<mkStr<<"."<<propName<<".controller.time.controller.setExpression \"current\"\n";
   }
   else{
      evalStream<<"$"<<mkStr<<"."<<propName<<".controller.time.controller.setExpression \"S\"\n";
   }

}



void addControllersToModifier(const std::string& modkey, const std::string& modname, std::vector<AbcProp>& props, 
                              const std::string& target, const std::string& type,
                              const std::string& file, const std::string& identifier, alembic_importoptions &options)
{
   ESS_PROFILE_FUNC();

   //the script assumes a single object is selected

   std::stringstream evalStream;

   for(int i=0; i<props.size(); i++){
      std::string propName = props[i].name;
      std::string& val = props[i].displayVal;
      bool& bConstant = props[i].bConstant;

      const AbcA::DataType& datatype = props[i].propHeader.getDataType();
      const AbcA::MetaData& metadata = props[i].propHeader.getMetaData();

      if(datatype.getPod() == AbcA::kFloat32POD){

         std::stringstream propStream;
         propStream<<target<<"."<<type<<"."<<propName;
         if(datatype.getExtent() == 1){
            addFloatController(evalStream, options, modkey, propName, file, identifier, propStream.str());
         }
         else if(datatype.getExtent() == 3){

            std::stringstream xStream, yStream, zStream;

            xStream<<propStream.str()<<"."<<metadata.get("interpretation")<<".x";
            yStream<<propStream.str()<<"."<<metadata.get("interpretation")<<".y";
            zStream<<propStream.str()<<"."<<metadata.get("interpretation")<<".z";

            addFloatController(evalStream, options, modkey, propName + std::string("x"), file, identifier, xStream.str());
            addFloatController(evalStream, options, modkey, propName + std::string("y"), file, identifier, yStream.str());
            addFloatController(evalStream, options, modkey, propName + std::string("z"), file, identifier, zStream.str());
         }
      }
      else{
         
      }
      evalStream<<"\n";
   }

  
   //ESS_LOG_WARNING(evalStream.str());

   ExecuteMAXScriptScript( EC_UTF8_to_TCHAR((char*)evalStream.str().c_str()));
}


void addControllersToModifierV2(const std::string& modkey, const std::string& modname, std::vector<AbcProp>& props, 
                              const std::string& file, const std::string& identifier, const std::string& category, alembic_importoptions &options, INode* pNode)
{
   ESS_PROFILE_FUNC();

   std::stringstream evalStream;
   std::string nodeName("$");
   if(pNode){
      evalStream<<GET_MAXSCRIPT_NODE(pNode);
      nodeName = std::string("mynode2113");
   }

   for(int i=0; i<props.size(); i++){
      std::string propName = props[i].name;
      std::string& val = props[i].displayVal;
      bool& bConstant = props[i].bConstant;

      const AbcA::DataType& datatype = props[i].propHeader.getDataType();
      const AbcA::MetaData& metadata = props[i].propHeader.getMetaData();

      if(datatype.getPod() == AbcA::kFloat32POD){

         std::stringstream propStream;
         propStream<<propName;
         if(datatype.getExtent() == 1){
            addFloatControllerV2(evalStream, options, nodeName, modkey, propName, file, identifier, category, propStream.str());
         }
         else if(datatype.getExtent() == 3){

            std::stringstream xStream, yStream, zStream;

            xStream<<propStream.str()<<"."<<metadata.get("interpretation")<<".x";
            yStream<<propStream.str()<<"."<<metadata.get("interpretation")<<".y";
            zStream<<propStream.str()<<"."<<metadata.get("interpretation")<<".z";

            addFloatControllerV2(evalStream, options, nodeName, modkey, propName + std::string("x"), file, identifier, category, xStream.str());
            addFloatControllerV2(evalStream, options, nodeName, modkey, propName + std::string("y"), file, identifier, category, yStream.str());
            addFloatControllerV2(evalStream, options, nodeName, modkey, propName + std::string("z"), file, identifier, category, zStream.str());
         }
      }
      else if(datatype.getPod() == AbcA::kInt32POD){
         std::stringstream propStream;
         propStream<<propName;
         if(datatype.getExtent() == 1){
            addFloatControllerV2(evalStream, options, nodeName, modkey, propName, file, identifier, category, propStream.str());
         }
      }
      else{
         
      }
      evalStream<<"\n";
   }

  
   //ESS_LOG_WARNING(evalStream.str());

   ExecuteMAXScriptScript( EC_UTF8_to_TCHAR((char*)evalStream.str().c_str()));
}

void addFloatControllerV2(std::stringstream& evalStream, alembic_importoptions &options, std::string nodeName,
                        const std::string& modkey, std::string propName, const std::string& file, const std::string& identifier, const std::string& category,
                        std::string propertyID)
{
   std::string mkStr;
   if(!modkey.empty()){
      std::stringstream mkStream;
      mkStream<<".modifiers[\""<<modkey<<"\"]";
      mkStr = mkStream.str();
   }

   std::string timeControlName;
   if(options.pTimeControl){
      timeControlName = EC_MCHAR_to_UTF8( options.pTimeControl->GetObjectName() );
   }

   evalStream<<nodeName<<mkStr<<"."<<propName<<".controller = AlembicFloatController()\n";
   evalStream<<nodeName<<mkStr<<"."<<propName<<".controller.path = \""<<file<<"\"\n";
   evalStream<<nodeName<<mkStr<<"."<<propName<<".controller.identifier = \""<<identifier<<"\"\n";
   evalStream<<nodeName<<mkStr<<"."<<propName<<".controller.propCategory = \""<<category<<"\"\n";
   evalStream<<nodeName<<mkStr<<"."<<propName<<".controller.property = \""<<propertyID<<"\"\n";
   


   evalStream<<nodeName<<mkStr<<"."<<propName<<".controller.time.controller = float_expression()\n";
   if(options.loadTimeControl){
      evalStream<<nodeName<<mkStr<<"."<<propName<<".controller.time.controller.AddScalarTarget \"current\" $'"<<timeControlName<<"'.time.controller\n";
      evalStream<<nodeName<<mkStr<<"."<<propName<<".controller.time.controller.setExpression \"current\"\n";
   }
   else{
      evalStream<<nodeName<<mkStr<<"."<<propName<<".controller.time.controller.setExpression \"S\"\n";
   }

}



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

bool sortFunc(AbcProp p1, AbcProp p2) { return p1.sortId > p2.sortId; }

char* getPodStr(AbcA::PlainOldDataType pod)
{
    if(pod == AbcA::kBooleanPOD) return "kBooleanPOD";
    if(pod == AbcA::kUint8POD) return "kUint8POD";
    if(pod == AbcA::kInt8POD) return "kInt8POD";
    if(pod == AbcA::kUint16POD) return "kUint16POD";
    if(pod == AbcA::kInt16POD) return "kInt16POD";
    if(pod == AbcA::kUint32POD) return "kUint32POD";
    if(pod == AbcA::kInt32POD) return "kInt32POD";
    if(pod == AbcA::kUint64POD) return "kUint64POD";
    if(pod == AbcA::kInt64POD) return "kInt64POD";
    if(pod == AbcA::kFloat16POD) return "kFloat16POD";
    if(pod == AbcA::kFloat32POD) return "kFloat32POD";
    if(pod == AbcA::kFloat64POD) return "kFloat64POD";
    if(pod == AbcA::kStringPOD) return "kStringPOD";
    if(pod == AbcA::kWstringPOD) return "kWstringPOD";
    if(pod == AbcA::kNumPlainOldDataTypes) return "kNumPlainOldDataTypes";
    //if(pod == AbcA::kUnknownPOD)  
    return "kUnknownPOD";
}

//from the MAXScript docs section titled "Punctuation and Symbols"
const char* invalidStrTable[] = {
"(",
")",
"+",
"*",
"-",
"/",
"^",
"=",
";",
",",
"[",
"]",
":",
"'",
"&",
".",
"{",
"}",
"#",
"!",
"<",
">",
"?",
"$",
//"\", 
"}",
};

const int invalidStrTableSize = sizeof(invalidStrTable)/sizeof(invalidStrTable[0]);

int containsInvalidString(std::string str)
{
   for(int i=0; i<invalidStrTableSize; i++){
      std::size_t found = str.find(invalidStrTable[i]);
      if (found!=std::string::npos) return i;
   }
   return -1;
}

void readInputProperties( Abc::ICompoundProperty prop, std::vector<AbcProp>& props )
{
   if(!prop){
      return;
   }

   for(size_t i=0; i<prop.getNumProperties(); i++){
      AbcA::PropertyHeader pheader = prop.getPropertyHeader(i);
      AbcA::PropertyType propType = pheader.getPropertyType();



      //ESS_LOG_WARNING("Property, propName: "<<pheader.getName()<<", pod: "<<getPodStr(pheader.getDataType().getPod()) \
      // <<", extent: "<<(int)pheader.getDataType().getExtent()<<", interpretation: "<<pheader.getMetaData().get("interpretation"));
      
      int invalidStrIndex = containsInvalidString(pheader.getName());
      if( invalidStrIndex > 0 ){
         ESS_LOG_WARNING("Skipping property "<<pheader.getName()<<" because it contains an invalid character: "<<invalidStrTable[invalidStrIndex]);
         continue;
      }

      if( propType == AbcA::kCompoundProperty ){
         //printInputProperties(Abc::ICompoundProperty(prop, pheader.getName()));
         ESS_LOG_WARNING("Unsupported compound property: "<<pheader.getName());
      }
      else if( propType == AbcA::kScalarProperty ){

         //ESS_LOG_WARNING("Scaler property: "<<pheader.getName());
         //

         std::string displayVal;
         bool bConstant = true;
         int sortId = 0;
         int size = 0;

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
         else if(readPropExt1<Abc::IInt32Property, int>(prop, pheader, displayVal, bConstant));
         else if(readPropExt1<Abc::IFloatProperty, float>(prop, pheader, displayVal, bConstant));
         else if(readPropExt3<Abc::IC3fProperty, Abc::C3f>(prop, pheader, displayVal, bConstant));
         else if(readPropExt3<Abc::IV3fProperty, Abc::V3f>(prop, pheader, displayVal, bConstant));
         else if(readPropExt3<Abc::IN3fProperty, Abc::N3f>(prop, pheader, displayVal, bConstant));
         else if(Abc::IStringProperty::matches(pheader)){
            
            Abc::IStringProperty stringProp(prop, pheader.getName());
            stringProp.get(displayVal);
            sortId = 1000000000;
         }
         else{
      //   Abc::PropertyHeader propHeader = props.getPropertyHeader(i);
      //   AbcA::PropertyType propType = propHeader.getPropertyType();

            ESS_LOG_WARNING("Unsupported property, propName: "<<pheader.getName()<<", pod: "<<getPodStr(pheader.getDataType().getPod()) \
             <<", extent: "<<(int)pheader.getDataType().getExtent()<<", interpretation: "<<pheader.getMetaData().get("interpretation"));

         }

         props.push_back(AbcProp(pheader.getName(), displayVal, pheader, bConstant, sortId));
      }
      else if( propType == AbcA::kArrayProperty ){
         //ESS_LOG_WARNING("Unsupported array property: "<<pheader.getName());
         ESS_LOG_WARNING("Unsupported array property, propName: "<<pheader.getName()<<", pod: "<<getPodStr(pheader.getDataType().getPod()) \
         <<", extent: "<<(int)pheader.getDataType().getExtent()<<", interpretation: "<<pheader.getMetaData().get("interpretation"));
      }
      else{
         ESS_LOG_WARNING("Unsupported input property: "<<pheader.getName());
      }

   }
}

AlembicCustomAttributesEx::~AlembicCustomAttributesEx()
{
   for(propMap::iterator it = customProps.begin(); it != customProps.end(); it++){
      delete it->second;
   }
}

bool AlembicCustomAttributesEx::defineCustomAttributes(INode* node, Abc::OCompoundProperty& compoundProp, const AbcA::MetaData& metadata, unsigned int animatedTs)
{
   Modifier* pMod = FindModifier(node, (char*)this->modName.c_str());

   if(!pMod){
      return false;
   }

	ICustAttribContainer* cont = pMod->GetCustAttribContainer();
	if(!cont){
		return false;
	}

	for(int i=0; i<cont->GetNumCustAttribs(); i++)
	{
		CustAttrib* ca = cont->GetCustAttrib(i);
		std::string name = EC_MCHAR_to_UTF8( ca->GetName() );
	
		pblock = ca->GetParamBlockByID(0);
      break;
	}

   if(!pblock){
      return false;
   }


	for(int i=0, nNumParams = pblock->NumParams(); i<nNumParams; i++){

		ParamID id = pblock->IndextoID(i);
		MSTR name = pblock->GetLocalName(id, 0);

      std::stringstream propName;
      propName<<EC_MSTR_to_UTF8(name);

      ParamType2 type = pblock->GetParameterType(id);
      if(type == TYPE_STRING){
         customProps[propName.str()] = new Abc::OStringProperty(compoundProp, propName.str().c_str(), metadata, animatedTs );
      }
      else if(type == TYPE_FLOAT){
         customProps[propName.str()] = new Abc::OFloatProperty(compoundProp, propName.str().c_str(), metadata, animatedTs );
      }
      else if(type == TYPE_INT){
         customProps[propName.str()] = new Abc::OInt32Property(compoundProp, propName.str().c_str(), metadata, animatedTs );
      }
	}
   
   return true;
}

bool AlembicCustomAttributesEx::exportCustomAttributes(INode* node, double time)
{
   if(!pblock){
      return false;
   }

   TimeValue ticks = GetTimeValueFromFrame(time);

	int nNumParams = pblock->NumParams();
	for(int i=0; i<nNumParams; i++){

		ParamID id = pblock->IndextoID(i);
		MSTR name = pblock->GetLocalName(id, 0);

      std::stringstream propName;
      propName<<EC_MSTR_to_UTF8(name);

      ParamType2 type = pblock->GetParameterType(id);
      if(type == TYPE_STRING){
         MSTR value = pblock->GetStr(id, 0);

         Abc::OStringProperty* stringProp = (Abc::OStringProperty*)customProps[propName.str()];
         if(stringProp){
            stringProp->set(EC_MSTR_to_UTF8(value));
         }
      }
      else if(type == TYPE_FLOAT){
         float value = pblock->GetFloat(id, ticks);

         Abc::OFloatProperty* floatProp = (Abc::OFloatProperty*)customProps[propName.str()];    
         if(floatProp){
            floatProp->set(value);
         }
      }
      else if(type == TYPE_INT){
         int value = pblock->GetInt(id, ticks);
 
         Abc::OInt32Property* intProp = (Abc::OInt32Property*)customProps[propName.str()];         
         if(intProp){
            intProp->set(value);
         }
      }
	}

   return true;
}


void setupPropertyModifiers( AbcG::IObject& iObj, INode* pMaxNode, const std::string& file, const std::string& identifier, alembic_importoptions &options, const std::string prefix )
{

   Abc::ICompoundProperty userProps = AbcNodeUtils::getUserProperties(iObj);
   if(userProps.valid()){
      std::vector<AbcProp> propsVec;
      readInputProperties(userProps, propsVec);

      if(!propsVec.empty())
      {
         std::sort(propsVec.begin(), propsVec.end(), sortFunc);
         std::string name = prefix + " User Properties";
         createDisplayModifier(name, name, propsVec, pMaxNode);

         addControllersToModifierV2(name, name, propsVec, file, identifier, "userProperties", options, pMaxNode);
      }
   }

   //Abc::ICompoundProperty geomProps = AbcNodeUtils::getArbGeomParams(iObj);
   //if(geomProps.valid()){
   //   std::vector<AbcProp> propsVec;
   //   readInputProperties(geomProps, propsVec);

   //   if(!propsVec.empty())
   //   {
   //      std::sort(propsVec.begin(), propsVec.end(), sortFunc);
   //      std::string name = prefix + "ArbGeom Properties";
   //      createDisplayModifier(name, name, propsVec, pMaxNode);

   //      //addControllersToModifierV2("ArbGeom Properties", "ArbGeom Properties", propsVec, file, iObjXform.getFullName(), options, pMaxNode);
   //   }
   //}

}