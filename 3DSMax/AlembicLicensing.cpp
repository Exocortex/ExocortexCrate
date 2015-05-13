#include "stdafx.h"
#include "AlembicLicensing.h"
#include "Alembic.h"

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

bool g_bVerboseLogging = false;

boost::mutex gGlobalLock;


