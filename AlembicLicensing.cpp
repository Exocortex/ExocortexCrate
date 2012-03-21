#include <time.h>
#include <string>
#include <sstream>
#include "Foundation.h"
#include "AlembicLicensing.h"
#include "Alembic.h"
#include <maxscript/maxscript.h>

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

int gLicenseToken = EC_LICENSE_RESULT_NO_LICENSE;

#if defined( EXOCORTEX_RLM_ONLY )
	#include "RlmSingletonDeclarations.h"
#endif // EXOCORTEX_RLM_ONLY

bool HasFullLicense()
{
	return ( GetLicense() == EC_LICENSE_RESULT_FULL_LICENSE );
}

int GetLicense()
{
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


#ifdef EXOCORTEX_SERVICES

void MaxLogSink(const char* szLogMessage, Exocortex::ecLogLevel::Value level ) {
	switch( level ) {
	case Exocortex::ecLogLevel::Info:
		//mprintf( "Exocortex Alembic: %s\n", szLogMessage );
		break;
	case Exocortex::ecLogLevel::Warning:
		mprintf( "Exocortex Alembic Warning: %s\n", szLogMessage );
		break;
	case Exocortex::ecLogLevel::Error:
		mprintf( "Exocortex Alembic Error: %s\n", szLogMessage );
		break;
	}
}

namespace Exocortex {
	void essOnDemandInitialization() {
		static string pluginName(PLUGIN_NAME);

		char szTempPath[4096];
		GetTempPath( 4096, szTempPath );

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
		GetModuleFileName( NULL, szExePath, _MAX_PATH );
		ESS_LOG_INFO( "Executable path: " << szExePath );

		char szDllPath[_MAX_PATH];
		GetModuleFileName((HINSTANCE)&__ImageBase, szDllPath, _MAX_PATH);
		ESS_LOG_INFO( "Dll path: " << szDllPath );
		ESS_LOG_INFO( "------------------------------------------------------------------------------------------" );
	}
}
#endif // EXOCORTEX_SERVICES
