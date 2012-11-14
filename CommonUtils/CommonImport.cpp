#include "CommonImport.h"

#include "CommonUtilities.h"
#include "CommonAlembic.h"

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
			ESS_LOG_WARNING("Skipping invalid token: "<<tokens[j]);
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
			ESS_LOG_WARNING("Skipping invalid token: "<<tokens[j]);
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


SceneNode::nodeTypeE getNodeType(Alembic::Abc::IObject iObj)
{
   AbcG::MetaData metadata = iObj.getMetaData();
   if(AbcG::IXform::matches(metadata)){
      return SceneNode::ITRANSFORM;
      //Note: this method assumes everything is an ITRANSFORM, this is corrected later on in the the method that builds the common scene graph
   }
	else if(AbcG::IPolyMesh::matches(metadata) || 
		AbcG::ISubD::matches(metadata) ){
      return SceneNode::POLYMESH;
	}
	else if(AbcG::ICamera::matches(metadata)){
      return SceneNode::CAMERA;
	}
	else if(AbcG::IPoints::matches(metadata)){
      return SceneNode::PARTICLES;
	}
	else if(AbcG::ICurves::matches(metadata)){
      return SceneNode::CURVES;
	}
	else if(AbcG::ILight::matches(metadata)){
      return SceneNode::LIGHT;
	}
   else if(AbcG::INuPatch::matches(metadata)){
      return SceneNode::SURFACE;
	}
   return SceneNode::UNKNOWN;
}

struct AlembicISceneBuildElement
{
   AbcObjectCache *pObjectCache;
   SceneNodePtr parentNode;

   AlembicISceneBuildElement(AbcObjectCache *pMyObjectCache, SceneNodePtr node):pObjectCache(pMyObjectCache), parentNode(node)
   {}
};


SceneNodePtr buildCommonSceneGraph(AbcArchiveCache *pArchiveCache, AbcObjectCache *pRootObjectCache, int& nNumNodes)
{
   std::list<AlembicISceneBuildElement> sceneStack;

   Alembic::Abc::IObject rootObj = pRootObjectCache->obj;

   SceneNodePtr sceneRoot(new SceneNode());
   sceneRoot->name = rootObj.getName();
   sceneRoot->dccIdentifier = rootObj.getFullName();
   sceneRoot->type = SceneNode::SCENE_ROOT;

   for(size_t j=0; j<pRootObjectCache->childIdentifiers.size(); j++)
   {
      AbcObjectCache *pChildObjectCache = &( pArchiveCache->find( pRootObjectCache->childIdentifiers[j] )->second );
      Alembic::AbcGeom::IObject childObj = pChildObjectCache->obj;
      NodeCategory::type childCat = NodeCategory::get(childObj);
      //we should change this to explicity check which node types are not support (e.g. facesets), so that we can still give out warnings
      if( childCat == NodeCategory::UNSUPPORTED ) continue;// skip over unsupported types

      sceneStack.push_back(AlembicISceneBuildElement(pChildObjectCache, sceneRoot));
   }  

   //sceneStack.push_back(AlembicISceneBuildElement(pRootObjectCache, NULL));

   int numNodes = 0;

   while( !sceneStack.empty() )
   {
      AlembicISceneBuildElement sElement = sceneStack.back();
      Alembic::Abc::IObject iObj = sElement.pObjectCache->obj;
      SceneNodePtr parentNode = sElement.parentNode;
      sceneStack.pop_back();

      numNodes++;

      SceneNodePtr newNode(new SceneNodeAlembic(iObj));

      newNode->name = iObj.getName();
      newNode->dccIdentifier = iObj.getFullName();
      newNode->type = getNodeType(iObj);
      //select every node by default
      newNode->selected = true;
      
      if(parentNode){ //create bi-direction link if there is a parent
         newNode->parent = parentNode.get();
         parentNode->children.push_back(newNode);

         //the parent transforms of geometry nodes should be to be external transforms 
         //(we don't a transform's type until we have seen what type(s) of child it has)
         if( NodeCategory::get(iObj) == NodeCategory::GEOMETRY ){
            if(parentNode->type == SceneNode::ITRANSFORM){
               parentNode->type = SceneNode::ETRANSFORM;
            }
            else{
               ESS_LOG_WARNING("node "<<iObj.getFullName()<<" does not have a parent transform.");
            }
         }
      }

      //push the children as the last step, since we need to who the parent is first (we may have merged)
      for(size_t j=0; j<sElement.pObjectCache->childIdentifiers.size(); j++)
      {
         AbcObjectCache *pChildObjectCache = &( pArchiveCache->find( sElement.pObjectCache->childIdentifiers[j] )->second );
         Alembic::AbcGeom::IObject childObj = pChildObjectCache->obj;
         NodeCategory::type childCat = NodeCategory::get(childObj);
         //we should change this to explicity check which node types are not support (e.g. facesets), so that we can still give out warnings
         if( childCat == NodeCategory::UNSUPPORTED ) continue;// skip over unsupported types

         sceneStack.push_back(AlembicISceneBuildElement(pChildObjectCache, newNode));
      }  
   }

   nNumNodes = numNodes;
   
   return sceneRoot;
}





struct ImportStackElement
{
   SceneNodePtr sceneNode;
   SceneNodePtr parentNode;

   ImportStackElement(SceneNodePtr node):sceneNode(node)
   {}
   ImportStackElement(SceneNodePtr node, CRef parent):sceneNode(node), parentNode(parent)
   {}

};



bool ImportSceneFile(const IJobStringParser& jobParams, SceneNodePtr fileRoot)
{
   std::list<ImportStackElement> sceneStack;

   for(SceneChildIterator it = sceneRoot->children.begin(); it != sceneRoot->children.end(); it++){
      sceneStack.push_back(ImportStackElement(*it, importRootNode));
   }

   //TODO: abstract progress

   //int intermittentUpdateInterval = std::max( (int)(nNumNodes / 100), (int)1 );
   //int i = 0;
   while( !sceneStack.empty() )
   {
      ImportStackElement sElement = sceneStack.back();
      SceneNodePtr currFileNode = sElement.sceneNode;
      SceneNodePtr parentAppNode = sElement.parentNode;
      sceneStack.pop_back();

      //if( i % intermittentUpdateInterval == 0 ) {
      //   prog.PutCaption(L"Importing "+CString(iObj.getFullName().c_str())+L" ...");
      //}
      //i++;

       
      SceneNodePtr newAppNode = parentAppNode.attachChild(currFileNode, jobParams);

      if(newAppNode){

         //push the children as the last step, since we need to who the parent is first (we may have merged)
         for(SceneChildIterator it = sceneNode->children.begin(); it != sceneNode->children.end(); it++){
            AbcG::IObject childObj = (*it)->getObject();
            if( NodeCategory::get(childObj) == NodeCategory::UNSUPPORTED ) continue;// skip over unsupported types

            if( (*it)->selected == true ){
               sceneStack.push_back( ImportStackElement( *it, newAppNode ) );
            }
            else{//selected was set false (because of a merge)
               sceneStack.push_back( ImportStackElement( *it, parentAppNode ) );
            }
         }
         
      }
      //else{
	     // if( pObjectCache->childIdentifiers.size() > 0 ) {
		    //  EC_LOG_WARNING("Unsupported node: " << iObj.getFullName().c_str() << " has children that have not been imported." );
	     // }
      //}

      //if(prog.IsCancelPressed()){
      //   break;
      //}
      //prog.Increment();
   }




}