#include "stdafx.h"
#include "AlembicLicensing.h"
#include "CommonLog.h"

using namespace std;

int gLicenseToken = EC_LICENSE_RESULT_NO_LICENSE;

#if defined( EXOCORTEX_RLM_ONLY )
	#include "RlmSingletonDeclarations.h"
#endif // EXOCORTEX_RLM_ONLY

bool HasAlembicWriterLicense()
{
	return ( GetLicense() == EC_LICENSE_RESULT_FULL_LICENSE );
}

bool HasAlembicReaderLicense()
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
			ESS_LOG_INFO( "Looking for RLM license for " << pluginName << "...\n" );
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
				ESS_LOG_WARNING( "Expiry date licensing is being used for " << pluginName << "\n" );
				gLicenseToken = EC_LICENSE_RESULT_FULL_LICENSE;
				return gLicenseToken;
			}
		}
#endif // Exocortex_BETA_EXPIRY_DATE

	}

	return gLicenseToken;
}

void logError( const char* msg ) {
	std::cerr << "ExocortexAlembic Error: " << msg << std::endl;
}
void logWarning( const char* msg ) {
#ifdef _DEBUG
	std::cerr << "ExocortexAlembic Warning: " << msg << std::endl;
#else
	std::cout << "ExocortexAlembic Warning: " << msg << std::endl;
#endif
}
void logInfo( const char* msg ) {
#ifdef _DEBUG
	std::cerr << "ExocortexAlembic Info: " << msg << std::endl;
#else
	std::cout << "ExocortexAlembic Info: " << msg << std::endl;
#endif
}