#ifndef __ALEMBIC_LICENSING_H
#define __ALEMBIC_LICENSING_H


#define _EC_WSTR( s ) L ## s
#define EC_WSTR( s ) _EC_WSTR( s )

#define _EC_QUOTE( x ) #x
#define EC_QUOTE( x ) _EC_QUOTE( x )



// IMPORTANT, we are not using wide characters here.
#define PLUGIN_NAME						"ExocortexAlembic3DSMax" EC_QUOTE( crate_MAJOR_VERSION ) "." EC_QUOTE( crate_MINOR_VERSION )
#define PLUGIN_MAJOR_VERSION			crate_MAJOR_VERSION
#define PLUGIN_MINOR_VERSION			crate_MINOR_VERSION
#define PLUGIN_LICENSE_NAME				"alembic_3dsmax"
#define PLUGIN_LICENSE_VERSION			(crate_MAJOR_VERSION*10)

//#include "ExocortexServicesProxy.h"



#endif // __ALEMBIC_LICENSING_H
