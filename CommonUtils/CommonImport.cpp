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

bool IJobStringParser::paramIsSet(const std::string& param)
{
   if( extraParameters.find(param) == extraParameters.end()){
      return false;
   }
   return parseBool(extraParameters[param]);
}

void IJobStringParser::setParam(const std::string& param, bool bVal)
{
   if(bVal){
      extraParameters[param] = "1";
   }
   else{
      extraParameters[param] = "0";
   }
}

bool IJobStringParser::parse(const std::string& jobString)
{
	std::string search_str, replace_str;
	std::vector<std::string> tokens;
	boost::split(tokens, jobString, boost::is_any_of(";"));

	for(int j=0; j<tokens.size(); ++j)
	{
		std::vector<std::string> valuePair;
		boost::split(valuePair, tokens[j], boost::is_any_of("="));
		if(valuePair.size() != 2)
		{
			ESS_LOG_WARNING("Skipping invalid token: "<<tokens[j]);
			continue;
		}

		if(boost::iequals(valuePair[0], "filename"))
		{
			filename = valuePair[1];
		}
		else if(boost::iequals(valuePair[0], "normals"))
		{
			importNormals = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "uvs"))
		{
			importUVs = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "facesets"))
		{
			importFacesets = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "materialIds"))
		{
			importMaterialIds = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "attachToExisting"))
		{
			attachToExisting = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "importStandinProperties"))
		{
			importStandinProperties = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "importBoundingBoxes"))
		{
			importBoundingBoxes = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "importVisibilityControllers"))
		{
			importVisibilityControllers = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "failOnUnsupported"))
		{
			failOnUnsupported = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "selectShapes"))
		{
			selectShapes = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "filters") || boost::iequals(valuePair[0], "identifiers"))
		{
			boost::split(nodesToImport, valuePair[1], boost::is_any_of(","));
			for(int i=0; i<nodesToImport.size(); ++i)
			{
				boost::trim(nodesToImport[i]);
			}
		}
		else if(boost::iequals(valuePair[0], "includeChildren"))
		{
			includeChildren = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "skipUnattachedNodes"))
		{
			skipUnattachedNodes = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "enableImportRootSelection"))
		{
			enableImportRootSelection = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "stripMayaNamespaces"))
		{
			stripMayaNamespaces = parseBool(valuePair[1]);
		}
		else if(boost::iequals(valuePair[0], "importCurvesAsStrands"))
		{
			importCurvesAsStrands = parseBool(valuePair[1]);
		}
 		else if(boost::iequals(valuePair[0], "defaultXformNode"))
		{
           if(boost::iequals(valuePair[1], "model")){
               xformTypes = XSI_XformTypes::XMODEL;
           }
           else{
              xformTypes = XSI_XformTypes::XNULL;
           }
			
		}    
		// search/replace
		else if(boost::iequals(valuePair[0], "search"))
		{
			search_str = valuePair[1];
		}
		else if(boost::iequals(valuePair[0], "replace"))
		{
			replace_str = valuePair[1];
		}

		else
		{
			extraParameters[valuePair[0]] = valuePair[1];
		}
	}

	// check if the search/replace strings are valid!
	if (search_str.length() ? !replace_str.length() : replace_str.length())	// either search or replace string is missing or empty!
	{
		ESS_LOG_WARNING("Missing search or replace parameter. No strings will be replaced.");
		replacer = SearchReplace::createReplacer();
	}
	else
		replacer = SearchReplace::createReplacer(search_str, replace_str);
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
   stream<<";importBoundingBoxes="<<importBoundingBoxes<<";attachToExisting="<<attachToExisting<<";skipUnattachedNodes="<<skipUnattachedNodes;
   stream<<";failOnUnsupported="<<failOnUnsupported<<";enableImportRootSelection="<<enableImportRootSelection<<";stripMayaNamespaces="<<stripMayaNamespaces;
   stream<<";importCurvesAsStrands="<<importCurvesAsStrands<<";defaultXformNode=";

   if( xformTypes == XSI_XformTypes::XMODEL){
      stream<<"model";
   }
   else if( xformTypes = XSI_XformTypes::XNULL){
      stream<<"null";
   }

   if(!nodesToImport.empty()){
      stream<<";identifiers=";
      for(int i=0; i<nodesToImport.size(); i++){
         stream<<nodesToImport[i];
         if(i != nodesToImport.size()-1){
            stream<<",";
         }
      }
   }

   for (std::map<std::string, std::string>::iterator beg = extraParameters.begin(); beg != extraParameters.end(); ++beg)
	   stream << ";" << beg->first << "=" << beg->second;

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


SceneNodeAlembicPtr buildAlembicSceneGraph(AbcArchiveCache *pArchiveCache, AbcObjectCache *pRootObjectCache, int& nNumNodes, const IJobStringParser& jobParams, bool countMergableChildren, CommonProgressBar *pBar)
{
   ESS_PROFILE_FUNC();
   std::list<AlembicISceneBuildElement> sceneStack;

   Alembic::Abc::IObject rootObj = pRootObjectCache->obj;

   SceneNodeAlembicPtr sceneRoot(new SceneNodeAlembic(pRootObjectCache));
   sceneRoot->name = rootObj.getName();
   sceneRoot->dccIdentifier = rootObj.getFullName();
   sceneRoot->type = SceneNode::SCENE_ROOT;

   for(size_t j=0; j<pRootObjectCache->childIdentifiers.size(); j++)
   {
	   if (pBar && j % 20 == 0)
	   {
		   pBar->incr(1);
		   if (pBar->isCancelled())
			   return SceneNodeAlembicPtr();
	   }

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
	   if (pBar && numNodes % 20 == 0)
	   {
		   pBar->incr(1);
		   if (pBar->isCancelled())
			   return SceneNodeAlembicPtr();
	   }

      AlembicISceneBuildElement sElement = sceneStack.back();
      Alembic::Abc::IObject iObj = sElement.pObjectCache->obj;
      SceneNodePtr parentNode = sElement.parentNode;
      sceneStack.pop_back();

      numNodes++;

      SceneNodeAlembicPtr newNode(new SceneNodeAlembic(sElement.pObjectCache));

      newNode->name = jobParams.replacer->replace( iObj.getName() );
      newNode->dccIdentifier = iObj.getFullName();
      newNode->type = getNodeType(iObj);
      newNode->selected = false;

      //check if this newNode is actually an ETRANFORM
      if(newNode->type == SceneNode::ITRANSFORM){
         unsigned geomNodeCount = 0;

         for(int j=0; j<(int)sElement.pObjectCache->childIdentifiers.size(); j++)
         {
            AbcObjectCache *pChildObjectCache = &( pArchiveCache->find( sElement.pObjectCache->childIdentifiers[j] )->second );
            Alembic::AbcGeom::IObject childObj = pChildObjectCache->obj;
            if( NodeCategory::get( childObj ) == NodeCategory::GEOMETRY ){
               geomNodeCount++;
            }
         } 

         if(geomNodeCount == 1){ // the xform has only one geometry, child so it is possible to merge. Thus, this is an ETRANFORM.
            newNode->type = SceneNode::ETRANSFORM;
            if(!countMergableChildren){
               numNodes--;
            }
         }
         //else{ 
         //   ESS_LOG_WARNING("ITRANFORM!!!!");
         //}
      }
      
      if(parentNode){ //create bi-direction link if there is a parent
         newNode->parent = parentNode.get();
         parentNode->children.push_back(newNode);
      }

      //push the children as the last step, since we need to who the parent is first (we may have merged)
	  std::vector<std::string>::iterator chIter = sElement.pObjectCache->childIdentifiers.begin(),
										 chEnd  = sElement.pObjectCache->childIdentifiers.end();
	  for (; chIter != chEnd; ++chIter)
      {
		 AbcObjectCache *pChildObjectCache = &( pArchiveCache->find( *chIter )->second );
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


struct ValidateStackElement
{
   SceneNodeAlembicPtr currFileNode;

   ValidateStackElement(SceneNodeAlembicPtr node):currFileNode(node)
   {}

};

bool validateSceneFileAttached(SceneNodeAlembicPtr fileRoot, bool bSkipUnattachedNodes)
{
   ESS_PROFILE_FUNC();
   std::list<ValidateStackElement> sceneStack;

   for(SceneChildIterator it = fileRoot->children.begin(); it != fileRoot->children.end(); it++){
      SceneNodeAlembicPtr fileNode = reinterpret<SceneNode, SceneNodeAlembic>(*it);
      sceneStack.push_back(ValidateStackElement(fileNode));
   }

   bool bSuccess = true;

   while( !sceneStack.empty() )
   {
      ValidateStackElement sElement = sceneStack.back();
      SceneNodeAlembicPtr currFileNode = sElement.currFileNode;
      sceneStack.pop_back();

      if(!currFileNode->isAttached() /*&& currFileNode->selected*/){
         if(bSkipUnattachedNodes){
            ESS_LOG_WARNING("Node failed to attach: "<<currFileNode->dccIdentifier);
         }
         else{
            ESS_LOG_ERROR("Node failed to attach: "<<currFileNode->dccIdentifier);
         }
         bSuccess = false;
      }

      for(SceneChildIterator it = currFileNode->children.begin(); it != currFileNode->children.end(); it++){
         SceneNodeAlembicPtr fileNode = reinterpret<SceneNode, SceneNodeAlembic>(*it);
         if(!fileNode->isSupported()) continue;

         sceneStack.push_back( ValidateStackElement(fileNode) );
      }
   }

   if(bSkipUnattachedNodes){
      return true;
   }
   return bSuccess;
}



//SceneNode::nodeTypeE nodeTypeAttachTable[][] = {
//                  /*SCENE_ROOT*/ /*ETRANSFORM*/ /*ITRANSFORM*/ /*CAMERA*/ /*POLYMESH,*/ /*SUBD*/ /*SURFACE*/ /*CURVES*/ /*PARTICLES*/ /*HAIR*/ /*LIGHT*/ /*UNKNOWN*/
//   //we never actually import the scene root. we have to use the application scene root.
//   /*SCENE_ROOT*/ {false,            false,         false,         false,      false,     false,    false,      false,      false,      false,   false,   false},
//   //E
///*ETRANSFORM*/    {false,            false,         false,         false,      false,     false,    false,      false,      false,      false,   false,   false},
///*ITRANSFORM*/    {false,            false,         true,          false,      false,     false,    false,      false,      false,      false,   false,   false},
///*CAMERA*/
///*POLYMESH,*/
///*SUBD*/
///*SURFACE*/
///*CURVES*/
///*PARTICLES*/
///*HAIR*/
///*LIGHT*/
///*UNKNOWN*/
//
//}


typedef std::map<std::string, SceneNodeAlembicPtr> NodeMap;
typedef boost::shared_ptr<NodeMap> NodeMapPtr;
   
NodeMapPtr buildChildMap(SceneNodeAlembicPtr parent)
{
   ESS_PROFILE_FUNC();

   NodeMapPtr map(new NodeMap());

   SceneChildIterator endIt = parent->children.end();
   for(SceneChildIterator it = parent->children.begin(); it != endIt; it++){
      SceneNodeAlembicPtr node = reinterpret<SceneNode, SceneNodeAlembic>(*it);

      //Note: this algorithm assumes that parents of each selected node are also selected
      //if(!node->selected){
      //   continue;
      //}

      //for AttachToScene. Geometry nodes that have been merged with a parent transform should be skipped.
      if(node->isMerged()){
         continue;
      }
      
      const std::string& name = removeXfoSuffix(node->name);
      
      if(name.size() == 0){
         ESS_LOG_WARNING("Warning: node name is empty. Cannot add \""<<name<<"\" to map.");
      }

      if( map->find(name) != map->end() ){
         ESS_LOG_WARNING("Warning: duplicate node name. Cannot add \""<<name<<"\" to map.");
      }

      (*map)[name] = node;
   }

   return map;
}



struct AttachStackElement
{
   SceneNodeAppPtr currAppNode;
   NodeMapPtr childMapPtr;

   AttachStackElement(SceneNodeAppPtr appNode, NodeMapPtr mapPtr): currAppNode(appNode), childMapPtr(mapPtr)
   {}
};


bool AttachSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams, CommonProgressBar *pbar)
{
   ESS_PROFILE_FUNC();

   std::list<AttachStackElement> sceneStack;

   {
      NodeMapPtr map = buildChildMap(fileRoot);

      for(SceneChildIterator it = appRoot->children.begin(); it != appRoot->children.end(); it++){
         SceneNodeAppPtr appNode = reinterpret<SceneNode, SceneNodeApp>(*it);
         sceneStack.push_back(AttachStackElement(appNode, map));
      }
   }


   if (pbar) pbar->start();
   const int maxCount = pbar->getUpdateCount();
   int count = maxCount;
   while( !sceneStack.empty() )
   {
      AttachStackElement sElement = sceneStack.back();
      SceneNodeAppPtr currAppNode = sElement.currAppNode;
      NodeMapPtr childMapPtr = sElement.childMapPtr;
      sceneStack.pop_back();

      if (count == 0)
      {
         count = maxCount;
         if (pbar)
         {
            if (pbar->isCancelled())
            {
               EC_LOG_WARNING("Attach job cancelled by user");
               pbar->stop();
               return false;
            }
            pbar->incr(maxCount);
            pbar->setCaption(currAppNode->dccIdentifier);
         }
      }
      --count;


      const std::string& appNodeName = removeXfoSuffix(currAppNode->name);

      bool bChildAttached = false;
      NodeMap::iterator fileNodeIt = childMapPtr->find(appNodeName);
      if(fileNodeIt != childMapPtr->end()){//we have a match
         SceneNodeAlembicPtr fileNode = fileNodeIt->second;

         ESS_LOG_WARNING("nodeMatch: "<<appNodeName<<" = "<<fileNode->name);

         if(fileNode->isAttached()){
            ESS_LOG_ERROR("More than one match for node "<<fileNode->name);
			if (pbar) pbar->stop();
            return false;
         }
         else{
            SceneNodeAlembicPtr ignoreFileNode;
            bChildAttached = currAppNode->replaceData(fileNode, jobParams, ignoreFileNode);
           
            if(!bChildAttached){
               ESS_LOG_ERROR("replaceData operation failed on node "<<appNodeName);
               if(pbar) pbar->stop();
               return false;
            }

            childMapPtr = buildChildMap(fileNode);
         }
      }

      //push the children as the last step, since we need to who the parent is first (we may have merged)
      for(SceneChildIterator it = currAppNode->children.begin(); it != currAppNode->children.end(); it++){
         SceneNodeAppPtr appNode = reinterpret<SceneNode, SceneNodeApp>(*it);
         sceneStack.push_back( AttachStackElement(appNode, childMapPtr) );
      }
   }

	if (pbar) pbar->stop();
	
    return validateSceneFileAttached(fileRoot, jobParams.skipUnattachedNodes);
}


struct ImportStackElement
{
   SceneNodeAlembicPtr currFileNode;
   SceneNodeAppPtr parentAppNode;

   ImportStackElement(SceneNodeAlembicPtr node, SceneNodeAppPtr parent):currFileNode(node), parentAppNode(parent)
   {}

};

bool ImportSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams, CommonProgressBar *pbar, std::list<SceneNodeAppPtr> *newNodes)
{
   ESS_PROFILE_FUNC();

   //compare to application scene graph to see if we need to rename nodes (or maybe we might throw an error)


   std::list<ImportStackElement> sceneStack;

   for(SceneChildIterator it = fileRoot->children.begin(); it != fileRoot->children.end(); it++){
      SceneNodeAlembicPtr fileNode = reinterpret<SceneNode, SceneNodeAlembic>(*it);
      //if(!fileNode->selected){
      //   continue;
      //}
      sceneStack.push_back(ImportStackElement(fileNode, appRoot));
   }

   if (pbar) pbar->start();
   const int maxCount = pbar ? pbar->getUpdateCount() : 20;
   int count = 0;
   while( !sceneStack.empty() )
   {
      ImportStackElement sElement = sceneStack.back();
      SceneNodeAlembicPtr currFileNode = sElement.currFileNode;
      SceneNodeAppPtr parentAppNode = sElement.parentAppNode;
      sceneStack.pop_back();

      if (count % maxCount == 0)
      {
         count = maxCount;
         if (pbar)
         {
            if (pbar->isCancelled())
            {
               EC_LOG_WARNING("Import job cancelled by user");
               pbar->stop();
               return false;
            }
            pbar->incr(maxCount);
		std::string impMsg("Importing ");
		impMsg.append(currFileNode->dccIdentifier);
            pbar->setCaption(impMsg);
         }
      }
      ++count;
      
      //ESS_LOG_WARNING("Importing "<<currFileNode->pObjCache->obj.getFullName());
       
      SceneNodeAppPtr newAppNode;
      bool bContinue = parentAppNode->addChild(currFileNode, jobParams, newAppNode);

      if(!bContinue){
		  if (pbar) pbar->stop();
         return false;
      }

      if(newAppNode){
         //ESS_LOG_WARNING("newAppNode: "<<newAppNode->name<<" useCount: "<<newAppNode.use_count());

         //push the children as the last step, since we need to who the parent is first (we may have merged)
         for(SceneChildIterator it = currFileNode->children.begin(); it != currFileNode->children.end(); it++){
            SceneNodeAlembicPtr fileNode = reinterpret<SceneNode, SceneNodeAlembic>(*it);
            if(!fileNode->isSupported()) continue;

            //if(!fileNode->selected){
            //   continue;
            //}

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
         
         if(newNodes){
            newNodes->push_back(newAppNode);
         }
      }
      else{
         //ESS_LOG_WARNING("newAppNode useCount: "<<newAppNode.use_count());
	      if( currFileNode->children.empty() == false ) {
		      EC_LOG_WARNING("Unsupported node: " << currFileNode->name << " has children that have not been imported." );
	      }
      }
   }

   if (pbar) pbar->stop();
   return true;
}


struct SampleRangeStackElement
{
   SceneNodeAlembicPtr currFileNode;

   SampleRangeStackElement(SceneNodeAlembicPtr node):currFileNode(node)
   {}

};

template<class T> void GetSampleRangeT(Abc::IObject& obj, std::size_t& oMinSample, std::size_t& oMaxSample, double& oMinTime, double& oMaxTime)
{
   T xObj(obj, Abc::kWrapExisting);
   Alembic::AbcCoreAbstract::TimeSamplingPtr ts = xObj.getSchema().getTimeSampling();
   std::size_t numSamples = xObj.getSchema().getNumSamples();
   oMinSample = 0;
   oMaxSample = std::max(oMaxSample, (numSamples-1));
   oMinTime = std::min(ts->getSampleTime(0), oMinTime);
   oMaxTime = std::max(ts->getSampleTime(numSamples-1), oMaxTime);
}


void GetSampleRange(SceneNodeAlembicPtr fileRoot, std::size_t& oMinSample, std::size_t& oMaxSample, double& oMinTime, double& oMaxTime)
{
   ESS_PROFILE_FUNC();

   std::list<SampleRangeStackElement> sceneStack;

   for(SceneChildIterator it = fileRoot->children.begin(); it != fileRoot->children.end(); it++){
      SceneNodeAlembicPtr fileNode = reinterpret<SceneNode, SceneNodeAlembic>(*it);
      sceneStack.push_back(SampleRangeStackElement(fileNode));
   }

   

   while( !sceneStack.empty() )
   {
      SampleRangeStackElement sElement = sceneStack.back();
      SceneNodeAlembicPtr currFileNode = sElement.currFileNode;
      sceneStack.pop_back();

      SceneNodeAlembicPtr alembicNode = reinterpret<SceneNode, SceneNodeAlembic>(currFileNode);
      Abc::IObject obj = alembicNode->getObject();
      
      AbcG::MetaData metadata = obj.getMetaData();
      if(AbcG::IXform::matches(metadata)){
         GetSampleRangeT<AbcG::IXform>(obj, oMinSample, oMaxSample, oMinTime, oMaxTime);
      }
      else if(AbcG::IPolyMesh::matches(metadata)){
         GetSampleRangeT<AbcG::IPolyMesh>(obj, oMinSample, oMaxSample, oMinTime, oMaxTime);
      }
      else if(AbcG::ISubD::matches(metadata)){
         GetSampleRangeT<AbcG::ISubD>(obj, oMinSample, oMaxSample, oMinTime, oMaxTime);
	  }
	  else if(AbcG::ICamera::matches(metadata)){
         GetSampleRangeT<AbcG::ICamera>(obj, oMinSample, oMaxSample, oMinTime, oMaxTime);
	  }
	  else if(AbcG::IPoints::matches(metadata)){
         GetSampleRangeT<AbcG::IPoints>(obj, oMinSample, oMaxSample, oMinTime, oMaxTime);
	  }
	  else if(AbcG::ICurves::matches(metadata)){
         GetSampleRangeT<AbcG::ICurves>(obj, oMinSample, oMaxSample, oMinTime, oMaxTime);
	  }
	  else if(AbcG::ILight::matches(metadata)){
         GetSampleRangeT<AbcG::ILight>(obj, oMinSample, oMaxSample, oMinTime, oMaxTime);
	  }
      else if(AbcG::INuPatch::matches(metadata)){
         GetSampleRangeT<AbcG::INuPatch>(obj, oMinSample, oMaxSample, oMinTime, oMaxTime);
	  }
      else{
         ESS_LOG_WARNING("Warning: time range cannot be read for unrecoginized type.");
      }


   }

}



typedef std::map<std::string, SceneNodeAppPtr> AppNodeMap;
typedef boost::shared_ptr<AppNodeMap> AppNodeMapPtr;
   
AppNodeMapPtr buildChildMap(SceneNodeAppPtr parent)
{
   ESS_PROFILE_FUNC();

   AppNodeMapPtr map(new AppNodeMap());

   if(parent){
      SceneChildIterator endIt = parent->children.end();
      for(SceneChildIterator it = parent->children.begin(); it != endIt; it++){
         SceneNodeAppPtr node = reinterpret<SceneNode, SceneNodeApp>(*it);

         (*map)[node->name] = node;
      }
   }

   return map;
}

struct MergeStackElement
{
   SceneNodeAlembicPtr currFileNode;
   SceneNodeAppPtr parentAppNode;
   AppNodeMapPtr parentChildMapPtr;

   MergeStackElement(SceneNodeAlembicPtr node, SceneNodeAppPtr parent, AppNodeMapPtr pcmap):currFileNode(node), parentAppNode(parent), parentChildMapPtr(pcmap)
   {}

};

bool MergeSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams, CommonProgressBar *pbar, std::list<SceneNodeAppPtr> *newNodes)
{
   ESS_PROFILE_FUNC();

   //compare to application scene graph to see if we need to rename nodes (or maybe we might throw an error)

   std::list<MergeStackElement> sceneStack;

   for(SceneChildIterator it = fileRoot->children.begin(); it != fileRoot->children.end(); it++){
      SceneNodeAlembicPtr fileNode = reinterpret<SceneNode, SceneNodeAlembic>(*it);
      //if(!fileNode->selected){
      //   continue;
      //}
      sceneStack.push_back(MergeStackElement(fileNode, appRoot, buildChildMap(appRoot)));
   }

   if (pbar) pbar->start();
   const int maxCount = pbar ? pbar->getUpdateCount() : 20;
   int count = 0;
   while( !sceneStack.empty() )
   {
      MergeStackElement sElement = sceneStack.back();
      SceneNodeAlembicPtr currFileNode = sElement.currFileNode;
      SceneNodeAppPtr parentAppNode = sElement.parentAppNode;
      AppNodeMapPtr childMapPtr = sElement.parentChildMapPtr;
      sceneStack.pop_back();

      if (count % maxCount == 0)
      {
         count = maxCount;
         if (pbar)
         {
            if (pbar->isCancelled())
            {
               EC_LOG_WARNING("Import job cancelled by user");
               pbar->stop();
               return false;
            }
            pbar->incr(maxCount);
		std::string impMsg("Importing ");
		impMsg.append(currFileNode->dccIdentifier);
            pbar->setCaption(impMsg);
         }
      }
      ++count;
      
      //ESS_LOG_WARNING("Importing "<<currFileNode->pObjCache->obj.getFullName());
       
      SceneNodeAppPtr newAppNode;

      //if matches a child of parent attach to it, and set "new" app node to the matching node
      //otherwise create the node and so on...


      const std::string& fileNodeName = currFileNode->name;



      AppNodeMap::iterator appNodeIt = childMapPtr->find(fileNodeName);
      if(appNodeIt != childMapPtr->end()){//we have a match
         SceneNodeAppPtr appNode = appNodeIt->second;

         ESS_LOG_WARNING("nodeMatch: "<<fileNodeName<<" = "<<appNode->name);

         SceneNodeAlembicPtr ignoreFileNode;
         bool bChildAttached = appNode->replaceData(currFileNode, jobParams, ignoreFileNode);

         if(!bChildAttached){
            ESS_LOG_ERROR("replaceData operation failed on node "<<appNode->name);
            if(pbar)pbar->stop();
            return false;
         }
         
         newAppNode = appNode;        
      }
      else{
         bool bContinue = parentAppNode->addChild(currFileNode, jobParams, newAppNode);
         if(!bContinue){
            ESS_LOG_ERROR("Could not create new child: "<<fileNodeName);
		    if (pbar) pbar->stop();
            return false;
         }
      }

      childMapPtr = buildChildMap(newAppNode);

      if(newAppNode){
         //ESS_LOG_WARNING("newAppNode: "<<newAppNode->name<<" useCount: "<<newAppNode.use_count());

         //push the children as the last step, since we need to who the parent is first (we may have merged)
         for(SceneChildIterator it = currFileNode->children.begin(); it != currFileNode->children.end(); it++){
            SceneNodeAlembicPtr fileNode = reinterpret<SceneNode, SceneNodeAlembic>(*it);
            if(!fileNode->isSupported()) continue;

            //if(!fileNode->selected){
            //   continue;
            //}

            if( fileNode->isMerged() ){
               //The child node was merged with its parent, so skip this child, and add its children
               //(Although this case is technically possible, I think it will not be common)
               SceneNodePtr& mergedChild = *it;

               for(SceneChildIterator cit = mergedChild->children.begin(); cit != mergedChild->children.end(); cit++){
                  SceneNodeAlembicPtr cfileNode = reinterpret<SceneNode, SceneNodeAlembic>(*cit);
                  sceneStack.push_back( MergeStackElement( cfileNode, newAppNode, childMapPtr ) );
               }
            }
            else{
               sceneStack.push_back( MergeStackElement( fileNode, newAppNode, childMapPtr ) );
            }
         }
         
         if(newNodes){
            newNodes->push_back(newAppNode);
         }
      }
      else{
         //ESS_LOG_WARNING("newAppNode useCount: "<<newAppNode.use_count());
	      if( currFileNode->children.empty() == false ) {
		      EC_LOG_WARNING("Unsupported node: " << currFileNode->name << " has children that have not been imported." );
	      }
      }
   }

   if (pbar) pbar->stop();
   return true;
}
