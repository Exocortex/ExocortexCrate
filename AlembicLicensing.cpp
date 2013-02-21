#include "stdafx.h"
#include "AlembicLicensing.h"
#include "CommonLog.h"

using namespace std;


#if defined( EXOCORTEX_RLM_ONLY )
	#include "RlmSingletonDeclarations.h"
#endif // EXOCORTEX_RLM_ONLY


int s_alembicLicense = ALEMBIC_NO_LICENSE;

int GetAlembicLicense()
{
	if( s_alembicLicense == ALEMBIC_NO_LICENSE )
	{
		// default license status, could be overriden below.
   	bool isNoDemo = ( getenv("EXOCORTEX_ALEMBIC_NO_DEMO") != NULL );

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
			ESS_LOG_INFO( "Looking for RLM license for " << pluginName << "...\n" );
			Exocortex::RlmSingleton& rlmSingleton = Exocortex::RlmSingleton::getSingleton();

			RlmProductID pluginLicenseIds2[] = PLUGIN_LICENSE_IDS;
			vector<RlmProductID> rlmProductIds;
			for( int i = 0; i < sizeof( pluginLicenseIds2 ) / sizeof( RlmProductID ); i ++ ) {
				rlmProductIds.push_back( pluginLicenseIds2[i] );
			}

			if( rlmSingleton.checkoutLicense( "", pluginName, rlmProductIds ) ) {
				s_alembicLicense = ALEMBIC_WRITER_LICENSE;
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
				ESS_LOG_WARNING( "Expiry date licensing is being used for " << pluginName << "\n" );
				s_alembicLicense = ALEMBIC_WRITER_LICENSE;
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


bool HasAlembicWriterLicense() {
	return ( GetAlembicLicense() == ALEMBIC_WRITER_LICENSE );
}

bool HasAlembicReaderLicense() {
	return ( GetAlembicLicense() == ALEMBIC_WRITER_LICENSE )||( GetAlembicLicense() == ALEMBIC_READER_LICENSE );
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