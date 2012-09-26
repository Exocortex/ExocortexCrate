#ifndef __EXOCORTEX_SERVICES_PROXY_H
#define __EXOCORTEX_SERVICES_PROXY_H

#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include "Python.h"

#define EC_LICENSE_RESULT_NO_LICENSE	(0)
#define EC_LICENSE_RESULT_DEMO_LICENSE	(1)
#define EC_LICENSE_RESULT_FULL_LICENSE	(2)



#if defined( EXOCORTEX_RLM_ONLY )

	#ifndef PLUGIN_LICENSE_NAME
		#error "PLUGIN_LICENSE_NAME not defined, required for EXOCORTEX_RLM_ONLY-style licensing"
	#endif
	#ifndef PLUGIN_LICENSE_VERSION
		#error "PLUGIN_LICENSE_VERSION not defined, required for EXOCORTEX_RLM_ONLY-style licensing"
	#endif

	#ifndef PLUGIN_LICENSE_IDS
		#define PLUGIN_LICENSE_IDS	{ RlmProductID( PLUGIN_LICENSE_NAME, PLUGIN_LICENSE_VERSION ) }
	#endif

	//#include <xsi_application.h>
 
	#include "RlmSingleton.h"

#endif	// EXOCORTEX_RLM_ONLY



#endif // __EXOCORTEX_SERVICES_PROXY_H
