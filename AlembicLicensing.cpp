#include "Foundation.h"
#include "AlembicMax.h"
#include "AlembicLicensing.h"
#include "Alembic.h"
#include <boost/algorithm/string.hpp>
#include <boost/thread/mutex.hpp>

using namespace std;

#pragma warning( disable : 4996 )

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


#include "RlmSingleton.h"

bool g_bVerboseLogging = false;

int gLicenseToken = EC_LICENSE_RESULT_NO_LICENSE;

#if defined( EXOCORTEX_RLM_ONLY )
	#include "RlmSingletonDeclarations.h"
#endif // EXOCORTEX_RLM_ONLY

boost::mutex gGlobalLock;

int s_alembicLicense = -1;

int GetAlembicLicense() {
	boost::mutex::scoped_lock writeLock( gGlobalLock );

	static string pluginName(PLUGIN_NAME);

	if( s_alembicLicense != ALEMBIC_NO_LICENSE ) {
		return s_alembicLicense;
	}

	bool isWriterLicense = false;

#if MAX_PRODUCT_YEAR_NUMBER < 2013
	isWriterLicense = ! GET_MAX_INTERFACE()->InSlaveMode();
#else
	isWriterLicense = ! GET_MAX_INTERFACE()->IsNetworkRenderServer();
#endif

	bool isForceReader = ( getenv("EXOCORTEX_ALEMBIC_FORCE_READER") != NULL );
	bool isForceWriter = ( getenv("EXOCORTEX_ALEMBIC_FORCE_WRITER") != NULL );

	if( isForceReader && isForceWriter ) {
		ESS_LOG_ERROR( "Both environment variables EXOCORTEX_ALEMBIC_FORCE_READER and EXOCORTEX_ALEMBIC_FORCE_WRITER defined, these conflict" );
	}

	if( isWriterLicense ) {
		if( isForceReader ) {
			ESS_LOG_WARNING( "Environment variable EXOCORTEX_ALEMBIC_FORCE_READER defined, forcing usage of read-only license." );
			isWriterLicense = false;
		}
	}	
	if( ! isWriterLicense ) {
		if( isForceWriter ) {
			ESS_LOG_WARNING( "Environment variable EXOCORTEX_ALEMBIC_FORCE_WRITER defined, forcing usage of write-capable license." );
			isWriterLicense = true;
		}
	}

	vector<RlmProductID> rlmProductIds;
	int pluginLicenseResult;

	if( isWriterLicense ) {
		RlmProductID pluginLicenseIds[] = ALEMBIC_WRITER_LICENSE_IDS;
		for( int i = 0; i < sizeof( pluginLicenseIds ) / sizeof( RlmProductID ); i ++ ) {
			rlmProductIds.push_back( pluginLicenseIds[i] );
		}
		pluginLicenseResult = ALEMBIC_WRITER_LICENSE;
	}
	else {
		RlmProductID pluginLicenseIds[] = ALEMBIC_READER_LICENSE_IDS;
		for( int i = 0; i < sizeof( pluginLicenseIds ) / sizeof( RlmProductID ); i ++ ) {
			rlmProductIds.push_back( pluginLicenseIds[i] );
		}
		pluginLicenseResult = ALEMBIC_READER_LICENSE;
	}
				
	ESS_LOG_INFO( "Looking for RLM license for " << pluginName << "..." );
	Exocortex::RlmSingleton& rlmSingleton = Exocortex::RlmSingleton::getSingleton();
	if( rlmSingleton.checkoutLicense( "", pluginName, rlmProductIds ) ) {
		s_alembicLicense = pluginLicenseResult;
	}
	else {
		s_alembicLicense = ALEMBIC_DEMO_LICENSE;
	}

#if defined( EXOCORTEX_BETA_EXPIRY_DATE )
	if( s_alembicLicense == ALEMBIC_DEMO_LICENSE ) {
		#pragma message( "Exocortex Licensing mode: Fixed expiry date" )
		time_t now = time(NULL);
		if( now <= EXOCORTEX_BETA_EXPIRY_DATE ) {  //http://unixtime-converter.com/
			static string pluginName(PLUGIN_NAME);
			ESS_LOG_WARNING( "Expiry date licensing is being used for " << pluginName );
			s_alembicLicense = pluginLicenseResult;
		}
	}
#endif // Exocortex_BETA_EXPIRY_DATE 

	return s_alembicLicense;
}

bool HasAlembicWriterLicense() {
	return ( GetAlembicLicense() == ALEMBIC_WRITER_LICENSE );
}

bool HasAlembicReaderLicense() {
	return ( GetAlembicLicense() == ALEMBIC_WRITER_LICENSE )||( GetAlembicLicense() == ALEMBIC_READER_LICENSE );
}

/*
bool HasFullLicense()
{
	return ( GetLicense() == EC_LICENSE_RESULT_FULL_LICENSE );
}


int GetLicense()
{
	boost::mutex::scoped_lock writeLock( gGlobalLock );

	if( gLicenseToken == EC_LICENSE_RESULT_NO_LICENSE )
	{
		// default license status, could be overriden below.
		gLicenseToken = EC_LICENSE_RESULT_DEMO_LICENSE;

		// check RLM license first, so that users see that RLM is either used or not prior to expiry.	
#if defined( EXOCORTEX_RLM_ONLY )
		{
			#pragma message( "Exocortex Licensing mode: RLM only" )
			static string pluginName(PLUGIN_NAME);
			ESS_LOG_INFO( "Looking for RLM license for " << pluginName << "..." );
			Exocortex::RlmSingleton& rlmSingleton = Exocortex::RlmSingleton::getSingleton();

			RlmProductID pluginLicenseIds2[] = ALEMBIC_WRITER_LICENSE_IDS;
			vector<RlmProductID> rlmProductIds;
			for( int i = 0; i < sizeof( pluginLicenseIds2 ) / sizeof( RlmProductID ); i ++ ) {
				rlmProductIds.push_back( pluginLicenseIds2[i] );
			}

			if( rlmSingleton.checkoutLicense( "", pluginName, rlmProductIds ) ) {
				gLicenseToken = EC_LICENSE_RESULT_FULL_LICENSE;
				return gLicenseToken;
			}
		}
#endif // EXOCORTEX_RLM_ONLY

#if defined( EXOCORTEX_BETA_EXPIRY_DATE )
		{
			#pragma message( "Exocortex Licensing mode: Fixed expiry date" )
			time_t now = time(NULL);
			if( now <= EXOCORTEX_BETA_EXPIRY_DATE ) {  //http://unixtime-converter.com/
				static string pluginName(PLUGIN_NAME);
				ESS_LOG_WARNING( "Expiry date licensing is being used for " << pluginName );
				gLicenseToken = EC_LICENSE_RESULT_FULL_LICENSE;
				return gLicenseToken;
			}
		}
#endif // Exocortex_BETA_EXPIRY_DATE 

	}

	return gLicenseToken;
}
*/

#ifdef EXOCORTEX_SERVICES

void MaxLogSink(const char* szLogMessage, Exocortex::ecLogLevel::Value level ) {
	switch( level ) {
	case Exocortex::ecLogLevel::Info:
		//mprintf( EC_UTF8_to_TCHAR( "Exocortex Alembic: %s\n" ), EC_UTF8_to_TCHAR( szLogMessage ) );
		break;
	case Exocortex::ecLogLevel::Warning:
		mprintf( EC_UTF8_to_TCHAR( "Exocortex Alembic Warning: %s\n" ), EC_UTF8_to_TCHAR( szLogMessage ) );
		break;
	case Exocortex::ecLogLevel::Error:
		mprintf( EC_UTF8_to_TCHAR( "Exocortex Alembic Error: %s\n" ), EC_UTF8_to_TCHAR( szLogMessage ) );
		break;
	}
}

// trick from: http://www.codeproject.com/KB/DLL/DLLModuleFileName.aspx
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace Exocortex {
	void essOnDemandInitialization() {


		char* loggingEnv = getenv("EXOCORTEX_ALEMBIC_VERBOSE_LOGGING");
		if(loggingEnv){
			g_bVerboseLogging = boost::iequals(loggingEnv, "true");
		}

		static string pluginName(PLUGIN_NAME);

		char szTempPath[4096];
		GetTempPathA( 4096, szTempPath );

		char szLogPath[4096];
		sprintf_s( szLogPath, 4096, "%sExocortexAlembic", szTempPath );

		essInitialize( pluginName.c_str(), PLUGIN_MAJOR_VERSION, PLUGIN_MINOR_VERSION, szLogPath, MaxLogSink );
		ESS_LOG_WARNING( "Exocortex Alembic logs location: " << szLogPath );
		ESS_LOG_INFO( "------------------------------------------------------------------------------------------" );
		ESS_LOG_INFO( "Build date: " << __DATE__ << " " << __TIME__ );
		OSVERSIONINFOEXA ver;
		ZeroMemory(&ver, sizeof(OSVERSIONINFOEXA));
		ver.dwOSVersionInfoSize = sizeof(ver);
		if (GetVersionExA( (OSVERSIONINFOA*) &ver) != FALSE) {
			char szOsVersion[4096];
			sprintf_s(szOsVersion, 4096, "OS version: %d.%d.%d (%s) 0x%x-0x%x", 
				ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber,
				ver.szCSDVersion, ver.wSuiteMask, ver.wProductType);
			ESS_LOG_INFO( szOsVersion );		
		}
		char szExePath[_MAX_PATH];
		GetModuleFileNameA( NULL, szExePath, _MAX_PATH );
		ESS_LOG_INFO( "Executable path: " << szExePath );

		char szDllPath[_MAX_PATH];
		GetModuleFileNameA((HINSTANCE)&__ImageBase, szDllPath, _MAX_PATH);
		ESS_LOG_INFO( "Dll path: " << szDllPath );
		ESS_LOG_INFO( "------------------------------------------------------------------------------------------" );
	}
}
#endif // EXOCORTEX_SERVICES
