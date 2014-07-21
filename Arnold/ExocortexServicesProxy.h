#ifndef __EXOCORTEX_SERVICES_PROXY_H
#define __EXOCORTEX_SERVICES_PROXY_H


  // #ifdef _DEBUG
//	   #include <syslog.h>
//	   #define ESS_LOG_SYSLOG(msg_type, msg) do { std::stringstream ss; ss << "[" << msg_type << "] " << __FILE__ << ":" << __LINE__ << " -> " << msg; syslog(LOG_USER, "%s", ss.str().c_str()); } while(0)
   //#else
	   #define ESS_LOG_SYSLOG(msg_type, msg)
   //#endif

#ifndef EC_ASSERT
	#define EC_ASSERT(a)		
#endif

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
