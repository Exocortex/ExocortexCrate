#ifndef __EXOCORTEX_SERVICES_PROXY_H
#define __EXOCORTEX_SERVICES_PROXY_H


#define ESS_CALLBACK_START( NodeName_CallbackName, ParamType )	XSIPLUGINCALLBACK CStatus NodeName_CallbackName( ParamType in_ctxt ) {
#define ESS_CALLBACK_END }

#include <xsi_application.h>

#if defined( __GNUC__ )
	#define printf_s(buffer, buffer_size, stringbuffer, ...) ( printf(buffer, stringbuffer, __VA_ARGS__) )
	#define sprintf_s(buffer, buffer_size, stringbuffer, ...) ( sprintf(buffer, stringbuffer, __VA_ARGS__) )
	#define vsprintf_s(buffer, buffer_size, stringbuffer, ...) ( vsprintf(buffer, stringbuffer, __VA_ARGS__) )
#endif
 

#endif // __EXOCORTEX_SERVICES_PROXY_H
