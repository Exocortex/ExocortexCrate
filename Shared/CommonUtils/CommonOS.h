#ifndef __COMMON_OS_H
#define __COMMON_OS_H

#ifndef __COMMON_ALEMBIC_H
	#error "Must include CommonAlembic.h before CommonLog.h"
#endif

#if defined( __GNUC__ )
	#define printf_s(buffer, buffer_size, stringbuffer, ...) ( printf(buffer, stringbuffer, __VA_ARGS__) )
	#define sprintf_s(buffer, buffer_size, stringbuffer, ...) ( sprintf(buffer, stringbuffer, __VA_ARGS__) )
	#define vsprintf_s(buffer, buffer_size, stringbuffer, ...) ( vsprintf(buffer, stringbuffer, __VA_ARGS__) )
#endif


#endif
