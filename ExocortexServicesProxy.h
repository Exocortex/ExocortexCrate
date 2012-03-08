#ifndef __EXOCORTEX_SERVICES_PROXY_H
#define __EXOCORTEX_SERVICES_PROXY_H
/*
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <Max.h>

#define EC_LICENSE_RESULT_NO_LICENSE	(0)
#define EC_LICENSE_RESULT_DEMO_LICENSE	(1)
#define EC_LICENSE_RESULT_FULL_LICENSE	(2)

#pragma message( "output RLM license log messages to CERR/COUT instead of to the console log." )
#define ESS_LOG_ERROR(a) do { std::stringstream s; s << a;  std::cerr << ( s.str().c_str() ); } while(0)
#define ESS_LOG_WARNING(a) do { std::stringstream s; s << a; std::cout << (s.str().c_str() ); } while(0)
#define ESS_LOG_INFO(a) do { std::stringstream s; s << a; std::cout << (s.str().c_str() ); } while(0)


#ifndef EC_LOG_ERROR
	#define EC_LOG_ERROR(a)		ESS_LOG_ERROR(a)
#endif
#ifndef EC_LOG_WARNING
	#define EC_LOG_WARNING(a)	ESS_LOG_WARNING(a)
#endif
#ifndef EC_LOG_INFO
	#define EC_LOG_INFO(a)		ESS_LOG_INFO(a)
#endif
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

	#if defined( __GNUC__ )
		#define printf_s(buffer, buffer_size, stringbuffer, ...) ( printf(buffer, stringbuffer, __VA_ARGS__) )
 		#define sprintf_s(buffer, buffer_size, stringbuffer, ...) ( sprintf(buffer, stringbuffer, __VA_ARGS__) )
 		#define vsprintf_s(buffer, buffer_size, stringbuffer, ...) ( vsprintf(buffer, stringbuffer, __VA_ARGS__) )
	#endif
 
	

#endif	// EXOCORTEX_RLM_ONLY


*/
#endif // __EXOCORTEX_SERVICES_PROXY_H