#include <time.h>
#include <string>
#include <sstream>
#include "Foundation.h"
#include "AlembicLicensing.h"

using namespace std;


#if defined( EXOCORTEX_RLM_ONLY )
	#include "RlmSingletonDeclarations.h"
#endif // EXOCORTEX_RLM_ONLY

int s_alembicLicense = -1;

int GetAlembicLicense() {
	static string pluginName(PLUGIN_NAME);
			
	if( s_alembicLicense != ALEMBIC_NO_LICENSE ) {
		return s_alembicLicense;
	}

#if defined( EXOCORTEX_BETA_EXPIRY_DATE )
		{
			#pragma message( "Exocortex Licensing mode: Fixed expiry date" )
			time_t now = time(NULL);
			if( now <= EXOCORTEX_BETA_EXPIRY_DATE ) {  //http://unixtime-converter.com/
				static string pluginName(PLUGIN_NAME);
				ESS_LOG_WARNING( "Expiry date licensing is being used for " << pluginName );
				s_alembicLicense = ALEMBIC_WRITER_LICENSE;
				return s_alembicLicense;
			}
		}
#endif // Exocortex_BETA_EXPIRY_DATE

	bool isWriterLicense = true;//XSI::Application().IsInteractive();

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
				
	Exocortex::RlmSingleton& rlmSingleton = Exocortex::RlmSingleton::getSingleton();
	if( rlmSingleton.checkoutLicense( "", pluginName, rlmProductIds ) ) {
		s_alembicLicense = pluginLicenseResult;
	}
	else {
		s_alembicLicense = ALEMBIC_DEMO_LICENSE;
	}

	return s_alembicLicense;
}

bool HasAlembicWriterLicense() {
	return ( GetAlembicLicense() == ALEMBIC_WRITER_LICENSE );
}

bool HasAlembicReaderLicense() {
	return ( GetAlembicLicense() == ALEMBIC_WRITER_LICENSE )||( GetAlembicLicense() == ALEMBIC_READER_LICENSE );
}

/*
#ifdef EXOCORTEX_SERVICES

namespace Exocortex {
	void essOnDemandInitialization() {
		static string pluginName(XSI::CString(PLUGIN_NAME).GetAsciiString());
	
		essInitializeMaya( pluginName.c_str(), PLUGIN_MAJOR_VERSION, PLUGIN_MINOR_VERSION );

		ESS_LOG_INFO( "------------------------------------------------------------------------------------------" );
		ESS_LOG_INFO( "Build date: " << __DATE__ << " " << __TIME__ );
#ifdef _MSC_VER
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
#endif
		ESS_LOG_INFO( "------------------------------------------------------------------------------------------" );
	}
}

#endif // EXOCORTEX_SERVICES


*/
