#include "utility.h"
#include "stdafx.h"

void logError(const char* msg) { AiMsgError(msg); }
void logWarning(const char* msg) { AiMsgWarning(msg); }
void logInfo(const char* msg) { AiMsgInfo(msg); }
