#include "CommonImport.h"


#include <boost/algorithm/string.hpp>
#include <sstream>


bool parseBool(std::string value){
	//std::istringstream(valuePair[1]) >> bExportSelected;

	if( value.find("true") != std::string::npos || value.find("1") != std::string::npos ){
		return true;
	}
	else{
		return false;
	}
}

bool IJobStringParser::parse(const std::string& jobString)
{

	std::vector<std::string> tokens;
	boost::split(tokens, jobString, boost::is_any_of(";"));

   //if(tokens.empty()){
   //   return false;
   //}

	for(int j=0; j<tokens.size(); j++){

		std::vector<std::string> valuePair;
		boost::split(valuePair, tokens[j], boost::is_any_of("="));
		if(valuePair.size() != 2){
			//ESS_LOG_WARNING("Skipping invalid token: "<<tokens[j]);
			continue;
		}

		if(boost::iequals(valuePair[0], "filename")){
			filename = valuePair[1];
		}
		else if(boost::iequals(valuePair[0], "normals")){
			importNormals = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "uvs")){
			importUVs = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "facesets")){
         importFacesets = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "materialIds")){
         importMaterialIds = parseBool(valuePair[1]);
		}
      else if(boost::iequals(valuePair[0], "attachToExisting")){
         attachToExisting = parseBool(valuePair[1]);
      }
      else if(boost::iequals(valuePair[0], "importStandinProperties")){
         importStandinProperties = parseBool(valuePair[1]);
      }
      else if(boost::iequals(valuePair[0], "importBoundingBoxes")){
         importBoundingBoxes = parseBool(valuePair[1]);
      }
      else if(boost::iequals(valuePair[0], "importVisibilityControllers")){
         importVisibilityControllers = parseBool(valuePair[1]);
		}
	   else if(boost::iequals(valuePair[0], "failOnUnsupported")){
         failOnUnsupported = parseBool(valuePair[1]);
		}
      else if(boost::iequals(valuePair[0], "filters") || boost::iequals(valuePair[0], "identifiers")){  
	      boost::split(nodesToImport, valuePair[1], boost::is_any_of(","));
		}
	   else if(boost::iequals(valuePair[0], "includeChildren")){
         includeChildren = parseBool(valuePair[1]);
		}
		else
		{
			//ESS_LOG_WARNING("Skipping invalid token: "<<tokens[j]);
			continue;
		}
	}

   return true;
}

std::string IJobStringParser::buildJobString()
{
   ////Note: there are currently some minor differences between the apps. This function is somewhat XSI specific.
   std::stringstream stream;

   if(!filename.empty()){
      stream<<"filename="<<filename<<";";
   }

   stream<<"normals="<<importNormals<<";uvs="<<importUVs<<";facesets="<<importFacesets;
   stream<<";importVisibilityControllers="<<importVisibilityControllers<<";importStandinProperties="<<importStandinProperties;
   stream<<";importBoundingBoxes="<<importBoundingBoxes<<";attachToExisting="<<attachToExisting<<";failOnUnsupported="<<failOnUnsupported;
   
   if(!nodesToImport.empty()){
      stream<<";identifiers=";
      for(int i=0; i<nodesToImport.size(); i++){
         stream<<nodesToImport[i];
         if(i != nodesToImport.size()-1){
            stream<<",";
         }
      }
   }
   
   return stream.str();
}