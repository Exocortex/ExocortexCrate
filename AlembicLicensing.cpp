#include <time.h>
#include "xsi_application.h"
#include "AlembicLicensing.h"

#include <string>
#include <sstream>

using namespace std;

#if defined( EXOCORTEX_RLM_ONLY )
	#include "RlmSingletonDeclarations.h"
#endif // EXOCORTEX_RLM_ONLY

int s_alembicLicense = -1;

int GetAlembicLicense() {
	static string pluginName(XSI::CString(PLUGIN_NAME).GetAsciiString());

	if( s_alembicLicense != ALEMBIC_NO_LICENSE ) {
		return s_alembicLicense;
	}

	bool isWriterLicense = XSI::Application().IsInteractive();

	bool isForceReader = ( getenv("EXOCORTEX_ALEMBIC_FORCE_READER") != NULL );
	bool isForceWriter = ( getenv("EXOCORTEX_ALEMBIC_FORCE_WRITER") != NULL );

	if( isForceReader && isForceWriter ) {
		ESS_LOG_ERROR( "Both environment variables EXOCORTEX_ALEMBIC_FORCE_READER and EXOCORTEX_ALEMBIC_FORCE_WRITER defined, these conflict" );
	}

	if( isWriterLicense ) {
		if( isForceReader ) {
			ESS_LOG_ERROR( "Environment variable EXOCORTEX_ALEMBIC_FORCE_WRITER defined, forcing usage of write-capable license." );
			isWriterLicense = false;
		}
	}	
	if( ! isWriterLicense ) {
		if( isForceWriter ) {
			ESS_LOG_ERROR( "Environment variable EXOCORTEX_ALEMBIC_FORCE_READER defined, forcing usage of read-only license." );
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


#ifdef EXOCORTEX_SERVICES

namespace Exocortex {
	void essOnDemandInitialization() {
		static string pluginName(XSI::CString(PLUGIN_NAME).GetAsciiString());
	
		essInitializeSoftimage( pluginName.c_str(), PLUGIN_MAJOR_VERSION, PLUGIN_MINOR_VERSION );
	}
}

#endif // EXOCORTEX_SERVICES


