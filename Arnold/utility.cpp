#include "stdafx.h"
#include "utility.h"


void logError( const char* msg ) {
	AiMsgError( msg );
}

void logWarning( const char* msg ) {
	AiMsgWarning( msg );
}

void logInfo( const char* msg ) {
	AiMsgInfo( msg );
}
