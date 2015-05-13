#include "stdafx.h"
#include "AlembicLicensing.h"
#include "CommonLog.h"

using namespace std;

void logError( const char* msg ) {
	std::cerr << "ExocortexAlembic Error: " << msg << std::endl;
}
void logWarning( const char* msg ) {
#ifdef _DEBUG
	std::cerr << "ExocortexAlembic Warning: " << msg << std::endl;
#else
	std::cout << "ExocortexAlembic Warning: " << msg << std::endl;
#endif
}
void logInfo( const char* msg ) {
#ifdef _DEBUG
	std::cerr << "ExocortexAlembic Info: " << msg << std::endl;
#else
	std::cout << "ExocortexAlembic Info: " << msg << std::endl;
#endif
}