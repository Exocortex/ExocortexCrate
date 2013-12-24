#ifndef _ALEMBIC_PROPERTY_UTILS_H_
#define _ALEMBIC_PROPERTY_UTILS_H_

#include <string>
#include <vector>
#include <Alembic/AbcCoreAbstract/PropertyHeader.h>
#include "AlembicDefinitions.h"

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

Modifier* createDisplayModifier(std::string modkey, std::string modname, std::vector<AbcProp>& props);


void addControllersToModifier(const std::string& modkey, const std::string& modname, std::vector<AbcProp>& props, 
                              const std::string& target, const std::string& type,
                              const std::string& file, const std::string& identifier, alembic_importoptions &options);


#endif 