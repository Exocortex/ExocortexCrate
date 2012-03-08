#include <time.h>
#include <string>
#include <sstream>
#include "Foundation.h"
#include "AlembicLicensing.h"
#include "Alembic.h"

using namespace std;

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

			RlmProductID pluginLicenseIds2[] = PLUGIN_LICENSE_IDS;
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

namespace Exocortex {
	void essOnDemandInitialization() {
		static string pluginName(PLUGIN_NAME);
		
		essInitialize( pluginName.c_str(), PLUGIN_MAJOR_VERSION, PLUGIN_MINOR_VERSION,  );
	}
}

#endif // EXOCORTEX_SERVICES
