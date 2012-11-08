#include "stdafx.h"
#include "AlembicArchiveStorage.h"
#include "AlembicLicensing.h"
#include "CommonProfiler.h"

std::string resolvePath_Internal(std::string path)
{
  if( path.find( '[' ) != std::string::npos ) {
	  return std::string( XSI::CUtils::ResolveTokenString(XSI::CString( path.c_str() ),XSI::CTime(),false).GetAsciiString() );
  }
  return path;
}

#pragma warning( disable: 4996 )

