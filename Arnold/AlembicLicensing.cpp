#include "stdafx.h"
#include "AlembicLicensing.h"

using namespace std;


int s_alembicLicense = ALEMBIC_NO_LICENSE;

#if defined( EXOCORTEX_RLM_ONLY )
	#include "RlmSingletonDeclarations.h"
#endif // EXOCORTEX_RLM_ONLY

int GetAlembicLicense()
{
	if( s_alembicLicense == ALEMBIC_NO_LICENSE )
	{

   	bool isNoDemo = ( getenv("EXOCORTEX_ALEMBIC_NO_DEMO") != NULL );

		// default license status, could be overriden below.
    if( isNoDemo ) {
      s_alembicLicense = ALEMBIC_INVALID_LICENSE;
    }
    else {
		  s_alembicLicense = ALEMBIC_DEMO_LICENSE;
    }

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
				s_alembicLicense = ALEMBIC_READER_LICENSE;
				ESS_LOG_INFO( "Exocortex Alembic for Arnold - license found." );
				return s_alembicLicense;
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
				s_alembicLicense = ALEMBIC_READER_LICENSE;
				return s_alembicLicense;
			}
		}
#endif // Exocortex_BETA_EXPIRY_DATE

	}

	return s_alembicLicense;
}

bool HasAlembicInvalidLicense() {
	return ( GetAlembicLicense() == ALEMBIC_INVALID_LICENSE );
}

bool HasAlembicReaderLicense()
{
	return ( GetAlembicLicense() == ALEMBIC_READER_LICENSE );
}
