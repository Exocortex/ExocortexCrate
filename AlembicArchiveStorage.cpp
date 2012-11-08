#include "stdafx.h"
#include "AlembicArchiveStorage.h"
#include "AlembicLicensing.h"
#include "CommonLog.h"


std::string resolvePath_Internal(std::string originalPath)
{
   ESS_PROFILE_SCOPE("resolvePath");

   MString path( originalPath.c_str() );

  if( originalPath.find( '[' ) != std::string::npos ) {
     // for each token
     int openPos = path.index('[');
     while(openPos > -1)
     {
        int closePos = path.index(']');
        if(closePos < openPos || closePos == openPos + 1)
        {
		    EC_LOG_ERROR("[ExocortexAlembic] Invalid token bracketing in path: " << path.asChar());
		    return std::string( path.asChar() );
        }
        MString prefix = (openPos > 0) ? path.substring(0,openPos-1) : "";
        MString suffix = (closePos < ((int)path.length() - 1)) ? path.substring(closePos+1,path.length()-1) : "";
        MString token = path.substring(openPos+1,closePos-1);
        MStringArray tokens;
        token.split(' ',tokens);
        if(tokens.length() != 2)
        {
		    EC_LOG_ERROR("[ExocortexAlembic] Invalid token '" << token.asChar() << "' in path: " << path.asChar());
		    return std::string( path.asChar() );
        }
        if(tokens[0].toLowerCase() == "env")
        {
           MString value = getenv(tokens[1].asChar());
           if(value.length() == 0)
           {
			      EC_LOG_ERROR("[ExocortexAlembic] Environment variable '" << tokens[1].asChar() << "' not found!");
			      return std::string( path.asChar() );
           }
           path = prefix + value + suffix;
        }
        else if(tokens[0].toLowerCase() == "scene" && tokens[1].toLowerCase() == "folder")
        {
           MFileIO io;
           std::string value = io.currentFile().asChar();
           for(size_t i=0;i<value.length();i++)
           {
              if(value[i] == '\\')
                 value[i] = '/';
           }
           if(value.rfind('/') > 0)
              value = value.substr(0,value.rfind('/'));
           path = prefix + value.c_str() + suffix;
        }
        else
        {
           EC_LOG_ERROR("[ExocortexAlembic] Invalid token '" << token.asChar() << "' in path: " << path.asChar());
		    return std::string( path.asChar() );
        }
        openPos = path.index('[');
     }
  }


  std::string expandedPath( path.asChar() );

   static std::map<std::string,std::string> s_originalToResolvedPath;

   if( s_originalToResolvedPath.find( expandedPath ) != s_originalToResolvedPath.end() ) {
	   return s_originalToResolvedPath[ expandedPath ];
   }
   else {
     MFileObject file;
     file.setRawFullName(path);
     path = file.resolvedFullName();
     s_originalToResolvedPath.insert( std::pair<std::string,std::string>( expandedPath, std::string( path.asChar() ) ) );
   }

   return std::string( path.asChar() );
}
