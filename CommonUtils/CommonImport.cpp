#include "CommonImport.h"

#include "CommonUtilities.h"
#include "CommonMeshUtilities.h"
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


SceneNodeAlembicPtr buildAlembicSceneGraph(AbcArchiveCache *pArchiveCache, AbcObjectCache *pRootObjectCache, int& nNumNodes)
{
	ESS_PROFILE_SCOPE("buildAlembicSceneGraph");
   std::list<AlembicISceneBuildElement> sceneStack;

   Alembic::Abc::IObject rootObj = pRootObjectCache->obj;

   SceneNodeAlembicPtr sceneRoot(new SceneNodeAlembic(pRootObjectCache));
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

      SceneNodeAlembicPtr newNode(new SceneNodeAlembic(sElement.pObjectCache));

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



struct AttachStackElement
{
   SceneNodeAppPtr currAppNode;
   SceneNodeAlembicPtr currFileNode;

   AttachStackElement(SceneNodeAppPtr appNode, SceneNodeAlembicPtr fileNode): currAppNode(appNode), currFileNode(fileNode)
   {}

};

bool AttachSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams)
{
   //TODO: how to account for filtering?
   //it would break the sibling namespace assumption. Perhaps we should require that all parent nodes of selected are imported.
   //We would then not traverse unselected children

   std::list<AttachStackElement> sceneStack;


   for(SceneChildIterator it = appRoot->children.begin(); it != appRoot->children.end(); it++){
      SceneNodeAppPtr appNode = reinterpret<SceneNode, SceneNodeApp>(*it);
      sceneStack.push_back(AttachStackElement(appNode, fileRoot));
   }

   //TODO: abstract progress

   //int intermittentUpdateInterval = std::max( (int)(nNumNodes / 100), (int)1 );
   //int i = 0;
   while( !sceneStack.empty() )
   {
      AttachStackElement sElement = sceneStack.back();
      SceneNodeAppPtr currAppNode = sElement.currAppNode;
      SceneNodeAlembicPtr currFileNode = sElement.currFileNode;
      sceneStack.pop_back();

      //if( i % intermittentUpdateInterval == 0 ) {
      //   prog.PutCaption(L"Importing "+CString(iObj.getFullName().c_str())+L" ...");
      //}
      //i++;

      SceneNodeAlembicPtr newFileNode = currFileNode;
      //Each set of siblings names in an Alembic file exist within a namespace
      //This is not true for 3DS Max scene graphs, so we check for such conflicts using the "attached appNode flag"
      bool bChildAttached = false;
      for(SceneChildIterator it = currFileNode->children.begin(); it != currFileNode->children.end(); it++){
         SceneNodeAlembicPtr fileNode = reinterpret<SceneNode, SceneNodeAlembic>(*it);
         if(currAppNode->name == fileNode->name){
            ESS_LOG_WARNING("nodeMatch: "<<(*it)->name<<" = "<<fileNode->name);
            if(fileNode->isAttached()){
               ESS_LOG_ERROR("More than one match for node "<<(*it)->name);
               return false;
            }
            else{
               bChildAttached = currAppNode->replaceData(fileNode, jobParams, newFileNode);
            }
         }
      }

      //push the children as the last step, since we need to who the parent is first (we may have merged)
      for(SceneChildIterator it = currAppNode->children.begin(); it != currAppNode->children.end(); it++){
         SceneNodeAppPtr appNode = reinterpret<SceneNode, SceneNodeApp>(*it);
         sceneStack.push_back( AttachStackElement( appNode, newFileNode ) );
      }

      //if(prog.IsCancelPressed()){
      //   break;
      //}
      //prog.Increment();
   }

   return true;
}


struct ImportStackElement
{
   SceneNodeAlembicPtr currFileNode;
   SceneNodeAppPtr parentAppNode;

   ImportStackElement(SceneNodeAlembicPtr node, SceneNodeAppPtr parent):currFileNode(node), parentAppNode(parent)
   {}

};

bool ImportSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams)
{
	ESS_PROFILE_SCOPE("ImportSceneFile");
   //TODO skip unselected children, if thats we how we do filtering.

   //compare to application scene graph to see if we need to rename nodes (or maybe we might throw an error)

   std::list<ImportStackElement> sceneStack;

   for(SceneChildIterator it = fileRoot->children.begin(); it != fileRoot->children.end(); it++){
      SceneNodeAlembicPtr fileNode = reinterpret<SceneNode, SceneNodeAlembic>(*it);
      sceneStack.push_back(ImportStackElement(fileNode, appRoot));
   }

   //TODO: abstract progress

   //int intermittentUpdateInterval = std::max( (int)(nNumNodes / 100), (int)1 );
   //int i = 0;
   while( !sceneStack.empty() )
   {
      ImportStackElement sElement = sceneStack.back();
      SceneNodeAlembicPtr currFileNode = sElement.currFileNode;
      SceneNodeAppPtr parentAppNode = sElement.parentAppNode;
      sceneStack.pop_back();

      //if( i % intermittentUpdateInterval == 0 ) {
      //   prog.PutCaption(L"Importing "+CString(iObj.getFullName().c_str())+L" ...");
      //}
      //i++;
       
      SceneNodeAppPtr newAppNode;
      bool bContinue = parentAppNode->addChild(currFileNode, jobParams, newAppNode);

      if(!bContinue){
         return false;
      }

      if(newAppNode){
         //ESS_LOG_WARNING("newAppNode: "<<newAppNode->name<<" useCount: "<<newAppNode.use_count());

         //push the children as the last step, since we need to who the parent is first (we may have merged)
         for(SceneChildIterator it = currFileNode->children.begin(); it != currFileNode->children.end(); it++){
            SceneNodeAlembicPtr fileNode = reinterpret<SceneNode, SceneNodeAlembic>(*it);
            if(!fileNode->isSupported()) continue;

            if( fileNode->isMerged() ){
               //The child node was merged with its parent, so skip this child, and add its children
               //(Although this case is technically possible, I think it will not be common)
               SceneNodePtr& mergedChild = *it;

               for(SceneChildIterator cit = mergedChild->children.begin(); cit != mergedChild->children.end(); cit++){
                  SceneNodeAlembicPtr cfileNode = reinterpret<SceneNode, SceneNodeAlembic>(*cit);
                  sceneStack.push_back( ImportStackElement( cfileNode, newAppNode ) );
               }
            }
            else{
               sceneStack.push_back( ImportStackElement( fileNode, newAppNode ) );
            }
         }
         
      }
      else{
         //ESS_LOG_WARNING("newAppNode useCount: "<<newAppNode.use_count());
	      if( currFileNode->children.empty() == false ) {
		      EC_LOG_WARNING("Unsupported node: " << currFileNode->name << " has children that have not been imported." );
	      }
      }

      //if(prog.IsCancelPressed()){
      //   break;
      //}
      //prog.Increment();
   }

   return true;
}
