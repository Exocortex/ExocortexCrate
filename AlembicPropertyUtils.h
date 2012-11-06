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

   AbcProp(std::string n, std::string val, AbcA::PropertyHeader header):name(n), displayVal(val), propHeader(header)
   {}
   AbcProp(std::string n, std::string val):name(n), displayVal(val)
   {}

};

void createDisplayModifier(std::string modname, std::vector<AbcProp>& props);


#endif 