#include "stdafx.h"
#include "AlembicLicensing.h"

using namespace std;


int s_alembicLicense = ALEMBIC_NO_LICENSE;

int GetAlembicLicense()
{
	return ALEMBIC_WRITER_LICENSE;
}

bool HasAlembicInvalidLicense() {
	return ( GetAlembicLicense() == ALEMBIC_INVALID_LICENSE );
}

bool HasAlembicReaderLicense()
{
	return ( GetAlembicLicense() == ALEMBIC_READER_LICENSE );
}
