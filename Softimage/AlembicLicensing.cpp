#include "stdafx.h"
#include "AlembicLicensing.h"


using namespace std;

#ifdef _MSC_VER
// trick from: http://www.codeproject.com/KB/DLL/DLLModuleFileName.aspx
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#endif

int s_alembicLicense = ALEMBIC_NO_LICENSE;

int GetAlembicLicense() {
	return 	ALEMBIC_WRITER_LICENSE;
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

