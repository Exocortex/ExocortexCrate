#ifndef __ALEMBIC_LICENSING_H
#define __ALEMBIC_LICENSING_H


#define _EC_WSTR( s ) L ## s
#define EC_WSTR( s ) _EC_WSTR( s )

#define _EC_QUOTE( x ) #x
#define EC_QUOTE( x ) _EC_QUOTE( x )

#define EC_WQUOTE( x ) EC_WSTR( EC_QUOTE( x ) )


#define PLUGIN_LICENSE_PUBLIC_KEY		{48, -127, -99, 48, 13, 6, 9, 42, -122, 72, -122, -9, 13, 1, 1, 1, 5, 0, 3, -127, -117, 0, 48, -127, -121, 2, -127, -127, 0, -111, 29, -8, 21, -22, -74, -58, -46, -32, 101, -109, 106, 107, 3, -23, -120, 48, -109, -112, 28, 42, 58, -59, 57, -31, 37, -125, 30, -39, -23, -54, 48, 93, -77, -16, -99, -93, -26, -3, 65, -27, 51, -97, 6, 42, 63, 36, 53, 22, -64, -53, -92, 54, -127, 101, -2, -40, 58, 29, 72, 96, 115, -4, 57, 65, -57, -30, -106, 4, -124, 11, -127, -63, 120, 54, 36, -83, -102, -67, -119, -58, -17, 82, 93, 6, 61, -23, -94, 119, -71, -27, 90, 16, -32, 67, -27, -1, -78, -95, 18, -86, 61, 5, 74, -18, -110, 25, -101, -29, 6, -69, 119, -46, 108, -3, 14, 47, -68, 75, -99, -55, 75, -80, 37, 84, -18, 78, -81, 2, 1, 17}

#define PLUGIN_AUTHOR					L"Exocortex / Helge Mathee"
#define PLUGIN_EMAIL					L"support@exocortex.com"
#define PLUGIN_NAME						L"ExocortexAlembicSoftimage" EC_WQUOTE( crate_MAJOR_VERSION ) L"." EC_WQUOTE( crate_MINOR_VERSION )
#define PLUGIN_MODULE_NAME				PLUGIN_NAME L".dll"
#define PLUGIN_MAJOR_VERSION			crate_MAJOR_VERSION
#define PLUGIN_MINOR_VERSION			crate_MINOR_VERSION
#define PLUGIN_LICENSE_NAME				"alembic_softimage"
#define PLUGIN_LICENSE_VERSION			(crate_MAJOR_VERSION*10)
#define PLUGIN_PRODUCT_URL				L"http://www.exocortex.com/alembic"
#define PLUGIN_PURCHASE_URL				L"http://www.exocortex.com/alembic"

#include "ExocortexServicesProxy.h"

#define ALEMBIC_WRITER_LICENSE_IDS	{ RlmProductID( "alembic_softimage", 10 ), RlmProductID( "alembic", 10 ) }
#define ALEMBIC_READER_LICENSE_IDS	{ RlmProductID( "alembic_softimage", 10 ), RlmProductID( "alembic_reader", 10 ), RlmProductID( "alembic", 10 ) }

#define ALEMBIC_NO_LICENSE -1
#define ALEMBIC_DEMO_LICENSE 0
#define ALEMBIC_WRITER_LICENSE 1
#define ALEMBIC_READER_LICENSE 2
#define ALEMBIC_INVALID_LICENSE 3

int GetAlembicLicense();
bool HasAlembicInvalidLicense();
bool HasAlembicWriterLicense();
bool HasAlembicReaderLicense();


#endif // __ALEMBIC_LICENSING_H
