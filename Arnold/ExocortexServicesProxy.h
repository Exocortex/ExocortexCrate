#ifndef __EXOCORTEX_SERVICES_PROXY_H
#define __EXOCORTEX_SERVICES_PROXY_H

// #ifdef _DEBUG
//	   #include <syslog.h>
//	   #define ESS_LOG_SYSLOG(msg_type, msg) do { std::stringstream ss; ss
//<< "[" << msg_type << "] " << __FILE__ << ":" << __LINE__ << " -> " << msg;
// syslog(LOG_USER, "%s", ss.str().c_str()); } while(0)
//#else
#define ESS_LOG_SYSLOG(msg_type, msg)
//#endif

#ifndef EC_ASSERT
#define EC_ASSERT(a)
#endif

#endif  // __EXOCORTEX_SERVICES_PROXY_H
