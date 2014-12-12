#include "stdafx.h"
#include "AlembicLicensing.h"
#include "CommonLog.h"

using namespace std;

int s_alembicLicense = ALEMBIC_NO_LICENSE;

int GetAlembicLicense()
{
	return ALEMBIC_WRITER_LICENSE;
}


bool HasAlembicInvalidLicense() {
	return ( GetAlembicLicense() == ALEMBIC_INVALID_LICENSE );
}


bool HasAlembicWriterLicense() {
	return ( GetAlembicLicense() == ALEMBIC_WRITER_LICENSE );
}

bool HasAlembicReaderLicense() {
	return ( GetAlembicLicense() == ALEMBIC_WRITER_LICENSE )||( GetAlembicLicense() == ALEMBIC_READER_LICENSE );
}

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