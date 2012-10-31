#ifndef _ALEMBIC_LIGHT_UTILITIES_H_
#define _ALEMBIC_LIGHT_UTILITIES_H_

#include <string>


int AlembicImport_Light(const std::string &path, AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode);



#endif