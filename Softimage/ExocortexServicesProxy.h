#ifndef __EXOCORTEX_SERVICES_PROXY_H
#define __EXOCORTEX_SERVICES_PROXY_H


#if defined( EXOCORTEX_SERVICES )

	// For explanation of bit fields in a custom exception code, see: http://msdn.microsoft.com/en-us/library/het71c37.aspx
	#define STATUS_FORCED_EXCEPTION       0xE0000001

	#define FORCE_CRASH_RAISE_EXCEPTION	{	RaiseException( STATUS_FORCED_EXCEPTION, 0, 0, NULL );	}
	#define FORCE_CRASH_INVALID_ACCESS_VIOLATION	{  int *pZero = (int*) 0;  *pZero = 0;  }
	#define FORCE_CRASH_STD_EXCEPTION	{	throw std::exception( "forced exception" );	}

	#include "ExocortexSoftimageServicesAPI.h"
	using namespace Exocortex;

#else  // EXOCORTEX_SERVICES

  #define ESS_CALLBACK_START( NodeName_CallbackName, ParamType )	XSIPLUGINCALLBACK CStatus NodeName_CallbackName( ParamType in_ctxt ) {
  #define ESS_CALLBACK_END }

#endif	// EXOCORTEX_SERVICES


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

	#include <xsi_application.h>

	#if defined( __GNUC__ )
		#define printf_s(buffer, buffer_size, stringbuffer, ...) ( printf(buffer, stringbuffer, __VA_ARGS__) )
 		#define sprintf_s(buffer, buffer_size, stringbuffer, ...) ( sprintf(buffer, stringbuffer, __VA_ARGS__) )
 		#define vsprintf_s(buffer, buffer_size, stringbuffer, ...) ( vsprintf(buffer, stringbuffer, __VA_ARGS__) )
	#endif
 
	#include "RlmSingleton.h"

#endif	// EXOCORTEX_RLM_ONLY


#endif // __EXOCORTEX_SERVICES_PROXY_H
