#ifndef __ALEMBIC_LICENSING_H
#define __ALEMBIC_LICENSING_H


#define _EC_WSTR( s ) L ## s
#define EC_WSTR( s ) _EC_WSTR( s )

#define _EC_QUOTE( x ) #x
#define EC_QUOTE( x ) _EC_QUOTE( x )

#define EC_WQUOTE( x ) EC_WSTR( EC_QUOTE( x ) )


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
