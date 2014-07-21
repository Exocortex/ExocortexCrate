#ifndef _ALEMBIC_PROPERTY_UTILS_H_
#define _ALEMBIC_PROPERTY_UTILS_H_

#include <string>
#include <vector>
#include <Alembic/AbcCoreAbstract/PropertyHeader.h>
#include "AlembicDefinitions.h"
#include "CommonUtilities.h"
#include <Alembic/Abc/ICompoundProperty.h>

void createStringPropertyDisplayModifier(std::string modname, std::vector<std::pair<std::string, std::string>>& nameValuePairs);



struct AbcProp{
   std::string name;
   AbcA::PropertyHeader propHeader; //we need to know type information (and possibly interpretation, e.g. color, normal)
   std::string displayVal; //if constant, will set via controller if animated
   bool bConstant;
   int sortId;

   int computeSortId(){
      
      const AbcA::DataType& datatype = propHeader.getDataType();
      const AbcA::MetaData& metadata = propHeader.getMetaData();

      return datatype.getPod() * datatype.getExtent();
   }

   AbcProp(std::string n, std::string val, AbcA::PropertyHeader header, bool bConstant, int sortid):name(n), displayVal(val), propHeader(header), bConstant(bConstant)
   {
      if(sortid != 0){
         sortId = sortid;
      }
      else{
         sortId = computeSortId();
      }
   }
   AbcProp(std::string n, std::string val, int sortid):name(n), displayVal(val), bConstant(true)
   {
      if(sortid != 0){
         sortId = sortid;
      }
      else{
         sortId = computeSortId();
      }
   }

};

bool sortFunc(AbcProp p1, AbcProp p2);

void readInputProperties( Abc::ICompoundProperty prop, std::vector<AbcProp>& props );

void addFloatController(std::stringstream& evalStream, alembic_importoptions &options,
                        const std::string& modkey, std::string propName, const std::string& file, const std::string& identifier, 
                        std::string propertyID);

void addFloatControllerV2(std::stringstream& evalStream, alembic_importoptions &options, std::string nodeName,
                        const std::string& modkey, std::string propName, const std::string& file, const std::string& identifier, const std::string& category, 
                        std::string propertyID);

Modifier* createDisplayModifier(std::string modkey, std::string modname, std::vector<AbcProp>& props, INode* pNode=NULL);


void addControllersToModifier(const std::string& modkey, const std::string& modname, std::vector<AbcProp>& props, 
                              const std::string& target, const std::string& type,
                              const std::string& file, const std::string& identifier, alembic_importoptions &options);

void addControllersToModifierV2(const std::string& modkey, const std::string& modname, std::vector<AbcProp>& props, 
                              const std::string& file, const std::string& identifier, const std::string& category, alembic_importoptions &options, INode* pNode);


class AlembicCustomAttributesEx
{
   typedef std::map<std::string, Abc::OScalarProperty*> propMap;
   propMap customProps;
   IParamBlock2 *pblock;
   std::string modName;

public:

   AlembicCustomAttributesEx(std::string modNamein):pblock(NULL),modName(modNamein)
   {} 

   ~AlembicCustomAttributesEx();

   bool defineCustomAttributes(INode* node, Abc::OCompoundProperty& compoundProp, const AbcA::MetaData& metadata, unsigned int animatedTs);
   bool exportCustomAttributes(INode* node, double time);
};


template<class PT> PT readScalarProperty(Abc::ICompoundProperty propk, const std::string& propName)
{
   for(size_t i=0; i<propk.getNumProperties(); i++){
      AbcA::PropertyHeader pheader = propk.getPropertyHeader(i);
      AbcA::PropertyType propType = pheader.getPropertyType();

      if( propType == AbcA::kScalarProperty ){

         if(boost::iequals(pheader.getName(), propName) && PT::matches(pheader))
         {
            return PT(propk, pheader.getName());
         }

      }

   }
   
   return PT();
}




template<class P, class PT> float readScalarPropertyExt3(Abc::ICompoundProperty propk, const SampleInfo& sampleInfo, const std::string& propName, const std::string& propComp)
{
   P fProp = readScalarProperty<P>(propk, propName);
   if(fProp.valid()){
      PT v3f;
      fProp.get(v3f, sampleInfo.floorIndex);
      if(propComp == "x"){
         return v3f.x;
      }
      else if(propComp == "y"){
         return v3f.y;
      }
      else if(propComp == "z"){
         return v3f.z;
      }
      else{
         ESS_LOG_WARNING("Float Controller Error: invalid component: "<<propComp);
         return -1.0;
      }
   }
   else{
      ESS_LOG_WARNING("Float Controller Error: could not read user property "<<propName);
      return -1.0;
   }
}

void setupPropertyModifiers( AbcG::IObject& iObj, INode* pMaxNode, const std::string& file, const std::string& identifier, alembic_importoptions &options, const std::string prefix=std::string("") );



#endif 