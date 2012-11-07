#ifndef _ALEMBIC_PROPERTY_UTILS_H_
#define _ALEMBIC_PROPERTY_UTILS_H_

#include <string>
#include <vector>
#include <Alembic/AbcCoreAbstract/PropertyHeader.h>

void createStringPropertyDisplayModifier(std::string modname, std::vector<std::pair<std::string, std::string>>& nameValuePairs);



struct AbcProp{
   std::string name;
   AbcA::PropertyHeader propHeader; //we need to know type information (and possibly interpretation, e.g. color, normal)
   std::string displayVal; //if constant, will set via controller if animated
   bool bConstant;

   AbcProp(std::string n, std::string val, AbcA::PropertyHeader header, bool bConstant):name(n), displayVal(val), propHeader(header), bConstant(bConstant)
   {}
   AbcProp(std::string n, std::string val):name(n), displayVal(val), bConstant(true)
   {}

};

void createDisplayModifier(std::string modkey, std::string modname, std::vector<AbcProp>& props);


#endif 