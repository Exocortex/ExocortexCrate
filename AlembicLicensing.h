#ifndef __ALEMBIC_LICENSING_H
#define __ALEMBIC_LICENSING_H


#define _EC_WSTR( s ) L ## s
#define EC_WSTR( s ) _EC_WSTR( s )

#define _EC_QUOTE( x ) #x
#define EC_QUOTE( x ) _EC_QUOTE( x )



// IMPORTANT, we are not using wide characters here.
#define PLUGIN_NAME						"ExocortexAlembicMaya" EC_QUOTE( alembic_MAJOR_VERSION ) "." EC_QUOTE( alembic_MINOR_VERSION )
#define PLUGIN_MAJOR_VERSION			alembic_MAJOR_VERSION
#define PLUGIN_MINOR_VERSION			alembic_MINOR_VERSION
#define PLUGIN_LICENSE_NAME				"alembic_maya"
#define PLUGIN_LICENSE_VERSION			(alembic_MAJOR_VERSION*10)

#include "CommonLog.h"
#include "ExocortexServicesProxy.h"

#define ALEMBIC_WRITER_LICENSE_IDS	{ RlmProductID( "alembic_maya", 10 ), RlmProductID( "alembic", 10 ) }
#define ALEMBIC_READER_LICENSE_IDS	{ RlmProductID( "alembic_maya", 10 ), RlmProductID( "alembic_reader", 10 ), RlmProductID( "alembic", 10 ) }

#define ALEMBIC_NO_LICENSE -1
#define ALEMBIC_DEMO_LICENSE 0
#define ALEMBIC_WRITER_LICENSE 1
#define ALEMBIC_READER_LICENSE 2

int GetAlembicLicense();
bool HasAlembicWriterLicense();
bool HasAlembicReaderLicense();


#endif // __ALEMBIC_LICENSING_H
