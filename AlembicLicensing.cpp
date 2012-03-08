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

	bool isInteractive = XSI::Application().IsInteractive();
	if( isInteractive ) {
		if( getenv("EXOCORTEX_ALEMBIC_READER_ONLY") != NULL ) {
			isInteractive = false;
		}
	}

	vector<RlmProductID> rlmProductIds;
	int pluginLicenseResult;

	if( isInteractive ) {
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


