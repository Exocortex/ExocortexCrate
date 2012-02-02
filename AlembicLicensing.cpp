#include <time.h>
#include "xsi_application.h"
#include "AlembicLicensing.h"

#include <string>
#include <sstream>

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

		#pragma message( "Exocortex Licensing mode: Fixed expiry date" )
		time_t now = time(NULL);
		if( now <= 1329264000 ) {  //http://unixtime-converter.com/
			static string pluginName(XSI::CString(PLUGIN_NAME).GetAsciiString());
			ESS_LOG_WARNING( "Expiry date licensing is being used for " << pluginName );
			gLicenseToken = EC_LICENSE_RESULT_FULL_LICENSE;
			return gLicenseToken;
		}		

		// if running on render farm, give license.
		/*	

		NO FREE NETWORK LICENSES

		if( ! XSI::Application().IsInteractive() ) {
			gLicenseToken = EC_LICENSE_RESULT_FULL_LICENSE;
			return gLicenseToken;
		}

		*/

		// check the license file
#if defined( EXOCORTEX_SERVICES )
	#pragma message( "Exocortex Licensing mode: Exocortex Services" )

		static string pluginName(XSI::CString(PLUGIN_NAME).GetAsciiString());
		static string pluginModuleName(XSI::CString(PLUGIN_MODULE_NAME).GetAsciiString());
		static string productUrl(XSI::CString(PLUGIN_PRODUCT_URL).GetAsciiString());
		static string purchaseUrl(XSI::CString(PLUGIN_PURCHASE_URL).GetAsciiString());
		static string licenseName(XSI::CString(PLUGIN_LICENSE_NAME).GetAsciiString());

		char const productPublicKeyArray[] = PLUGIN_LICENSE_PUBLIC_KEY;

		gLicenseToken = essLicenseCheck(
			pluginName.c_str(),
			productUrl.c_str(),
			purchaseUrl.c_str(),
			productPublicKeyArray,
			licenseName.c_str(),
			PLUGIN_LICENSE_VERSION,
			PLUGIN_DEMO_STYLE_LICENSING );

		if( gLicenseToken == EC_LICENSE_RESULT_FULL_LICENSE ) {
			return gLicenseToken;
		}

#else	// EXOCORTEX_SERVICES
	#if defined( EXOCORTEX_RLM_ONLY )
		{
			#pragma message( "Exocortex Licensing mode: RLM only" )
			static string pluginName(XSI::CString(PLUGIN_NAME).GetAsciiString());
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
	#endif // EXOCORTEX_RLM_ONLY*/
#endif // EXOCORTEX_SERVICES
	}

	return gLicenseToken;
}


#ifdef EXOCORTEX_SERVICES

namespace ExocortexSoftimageServices {
	void essOnDemandInitialization() {
		static string pluginName(XSI::CString(PLUGIN_NAME).GetAsciiString());
		static string pluginModuleName(XSI::CString(PLUGIN_MODULE_NAME).GetAsciiString());

		essInitialize( pluginName.c_str(), PLUGIN_MAJOR_VERSION, PLUGIN_MINOR_VERSION );
	}
}

#endif // EXOCORTEX_SERVICES


