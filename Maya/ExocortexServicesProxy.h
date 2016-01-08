#ifndef __EXOCORTEX_SERVICES_PROXY_H
#define __EXOCORTEX_SERVICES_PROXY_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef EC_LOG_ERROR
#define EC_LOG_ERROR(a) ESS_LOG_ERROR(a)
#endif
#ifndef EC_LOG_WARNING
#define EC_LOG_WARNING(a) ESS_LOG_WARNING(a)
#endif
#ifndef EC_LOG_INFO
#define EC_LOG_INFO(a) ESS_LOG_INFO(a)
#endif
#ifndef EC_ASSERT
#define EC_ASSERT(a)
#endif

#if defined(__GNUC__)
#define printf_s(buffer, buffer_size, stringbuffer, ...) \
  (printf(buffer, stringbuffer, __VA_ARGS__))
#define sprintf_s(buffer, buffer_size, stringbuffer, ...) \
  (sprintf(buffer, stringbuffer, __VA_ARGS__))
#define vsprintf_s(buffer, buffer_size, stringbuffer, ...) \
  (vsprintf(buffer, stringbuffer, __VA_ARGS__))
#endif

#endif  // __EXOCORTEX_SERVICES_PROXY_H