#ifndef __COMMON_LOG_H
#define __COMMON_LOG_H

void logError( const char* msg );
void logWarning( const char* msg );
void logInfo( const char* msg );

#define ESS_LOG_ERROR(a) do { std::stringstream __s; __s << "Alembic: " << a; logError( __s.str().c_str() ); } while(0)
#define ESS_LOG_WARNING(a) do { std::stringstream __s; __s << "Alembic: " << a; logWarning(__s.str().c_str() ); } while(0)
#define ESS_LOG_INFO(a) do { std::stringstream __s; __s << "Alembic: " << a; logInfo( __s.str().c_str() ); } while(0)

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

/*
void logError( const char* msg ) {
	MGlobal::displayError( msg );
}
void logWarning( const char* msg ) {
	MGlobal::displayWarning( msg );
}
void logInfo( const char* msg ) {
	MGlobal::displayInfo( msg );
}*/

#endif // __COMMON_LOG_H