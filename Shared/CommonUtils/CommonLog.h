#ifndef __COMMON_LOG_H
#define __COMMON_LOG_H

#ifndef __COMMON_ALEMBIC_H
	#error "Must include CommonAlembic.h before CommonLog.h"
#endif

void logError( const char* msg );
void logWarning( const char* msg );
void logInfo( const char* msg );

#define NO_DEFAULT_ESS_LOG_DEFINES

#define ESS_LOG_ERROR(a) do { std::stringstream __s; __s << "Alembic: " << a; logError( __s.str().c_str() ); } while(0)
#define ESS_LOG_WARNING(a) do { std::stringstream __s; __s << "Alembic: " << a; logWarning(__s.str().c_str() ); } while(0)
#define ESS_LOG_INFO(a) do { std::stringstream __s; __s << "Alembic: " << a; logInfo( __s.str().c_str() ); } while(0)

#define ESS_CPP_EXCEPTION_REPORTING_START
#define ESS_CPP_EXCEPTION_REPORTING_END
#define ESS_STRUCTURED_EXCEPTION_REPORTING_START
#define ESS_STRUCTURED_EXCEPTION_REPORTING_END

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

#endif // __COMMON_LOG_H