#include "stdafx.h"
#include "AlembicLicensing.h"


using namespace std;


int s_alembicLicense = -1;

int GetAlembicLicense() {
	return 	ALEMBIC_WRITER_LICENSE;
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
