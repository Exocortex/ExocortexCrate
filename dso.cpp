#include "foundation.h"
#include "utility.h"
#include <time.h>
#include <boost/lexical_cast.hpp>
#include <map>
#include <boost/thread/mutex.hpp>

#include <syslog.h>

struct instanceGroupInfo{
   std::vector<std::string> identifiers;
   std::vector<std::map<float,AtNode*> > nodes;
   std::vector<Alembic::Abc::IObject> objects;
   std::vector<Alembic::Abc::IObject> parents;
   std::vector<std::map<float,std::vector<Alembic::Abc::M44f> > > matrices;
};

struct instanceCloudInfo{
   std::vector<Alembic::Abc::P3fArraySamplePtr> pos;
   std::vector<Alembic::Abc::V3fArraySamplePtr> vel;
   std::vector<Alembic::Abc::UInt64ArraySamplePtr> id;
   std::vector<Alembic::Abc::FloatArraySamplePtr> width;
   std::vector<Alembic::Abc::V3fArraySamplePtr> scale;
   std::vector<Alembic::Abc::QuatfArraySamplePtr> rot;
   std::vector<Alembic::Abc::QuatfArraySamplePtr> ang;
   std::vector<Alembic::Abc::FloatArraySamplePtr> age;
   std::vector<Alembic::Abc::FloatArraySamplePtr> mass;
   std::vector<Alembic::Abc::C4fArraySamplePtr> color;
   std::vector<Alembic::Abc::UInt16ArraySamplePtr> shape;
   std::vector<Alembic::Abc::FloatArraySamplePtr> time;
   float timeAlpha;
   std::vector<instanceGroupInfo> groupInfos;
};

struct objectInfo{
   Alembic::Abc::IObject abc;
   AtNode * node;
   float centroidTime;
   bool hide;
   long ID;
   long instanceID;
   long instanceGroupID;
   instanceCloudInfo * instanceCloud;
   std::string suffix;

   objectInfo(float in_centroidTime)
   {
      hide = false;
      ID = -1;
      instanceID = -1;
      instanceGroupID = -1 ;
      instanceCloud = NULL;
      node = NULL;
      centroidTime = in_centroidTime;
      suffix = "_DSO";
   }
};

struct userData
{
   float gCentroidTime;
   char * gDataString;
   std::string gCurvesMode;
   std::string gPointsMode;
   AtArray * gProcShaders;
   AtArray * gProcDispMap;
   std::vector<objectInfo> gIObjects;
   std::vector<instanceCloudInfo> gInstances;
   std::vector<float> gMbKeys;
   float gTime;
   float gCurrTime;
   int proceduralDepth;

   std::vector<AtNode*> constructedNodes;
   std::vector<AtArray*> shadersToAssign;
};

#define sampleTolerance 0.00001
#define gCentroidPrecision 50.0f
#define roundCentroid(x) floorf(x * gCentroidPrecision) / gCentroidPrecision

// Reads the parameter value from data file and assign it to node
AtVoid ReadParameterValue(AtNode* curve_node, FILE* fp, const AtChar* param_name);

std::string getNameFromIdentifier(const std::string & identifier, long id = -1, long group = -1)
{
   std::string result;
   std::vector<std::string> parts;
   boost::split(parts, identifier, boost::is_any_of("/\\"));
   result = parts[parts.size()-1];
   for(int i=(int)parts.size()-3;i>=0;i--)
   {
      if(parts[i].empty())
         break;
      std::string suffix = parts[i].substr(parts[i].length()-3,3);
      if(suffix == "xfo" || suffix == "XFO" || suffix == "Xfo")
         result = parts[i].substr(0,parts[i].length()-3) + "." + result;
      else
         result = suffix + "." + result;
   }

   if(id >= 0)
      result += "."+boost::lexical_cast<std::string>(id);
   if(group >= 0)
      result += "."+boost::lexical_cast<std::string>(group);
   return result;
}  

boost::mutex gGlobalLock;

//#ifdef __UNIX__
#define GLOBAL_LOCK	   boost::mutex::scoped_lock writeLock( gGlobalLock );
//#else
//#define GLOBAL_LOCK
//#endif

std::map<std::string,std::string> gUsedArchives;

static int Init(AtNode *mynode, void **user_ptr)
{
	GLOBAL_LOCK;

   userData * ud = new userData();
   *user_ptr = ud;
   ud->gProcShaders = NULL;
   ud->gProcDispMap= NULL;

   ud->gDataString = (char*) AiNodeGetStr(mynode, "data");
   ud->gProcShaders = AiArrayCopy(AiNodeGetArray(mynode, "shader"));
   ud->gProcDispMap = AiNodeGetArray(mynode, "disp_map");

   // empty the procedural's shader
   //AtArray * emptyShaders = AiArrayAllocate(1,1,AI_TYPE_NODE);
   //AiArraySetPtr(emptyShaders,0,NULL);
   //AiNodeSetArray(mynode, "shader", emptyShaders);

   // set defaults for options
   ud->gCurvesMode = "ribbon";
   ud->gPointsMode = "";
   ud->gMbKeys.clear();

   // check the data string
   std::string completeStr(ud->gDataString);
   if(completeStr.length() == 0)
   {
      AiMsgError("[ExocortexAlembicArnold] No data string specified.");
      return NULL;
   }

   ESS_LOG_INFO( "ExocortexAlembicArnoldDSO: Init: DataString: " + completeStr );

   // split the string using boost
   std::vector<std::string> nameValuePairs;
   boost::split(nameValuePairs, completeStr, boost::is_any_of("&"));

   std::vector<std::string> paths(2);
   std::string identifier;
   ud->gTime = FLT_MAX;
   ud->gCurrTime = FLT_MAX;
   for(size_t i=0;i<nameValuePairs.size();i++)
   {
      std::vector<std::string> token;
      boost::split(token, nameValuePairs[i], boost::is_any_of("="));
      if(token.size() != 2)
      {
         AiMsgError("[ExocortexAlembicArnold] Invalid dsodata token pair '%s' specified!",nameValuePairs[i].c_str());
         return NULL;
      }
      if(token[0] == "path")
      {
         paths[0] = token[1];
         if(paths[1].empty())
            paths[1] = paths[0];
      }
      else if(token[0] == "instancespath")
         paths[1] = token[1];
      else if(token[0] == "identifier")
         identifier = token[1];
      else if(token[0] == "time")
         ud->gTime = (float)atof(token[1].c_str());
      else if(token[0] == "currtime")
         ud->gCurrTime = (float)atof(token[1].c_str());
      else if(token[0] == "curvesmode")
         ud->gCurvesMode = token[1];
      else if(token[0] == "pointsmode")
         ud->gPointsMode = token[1];
      else if(token[0] == "mbkeys")
      {
         std::vector<std::string> sampleTimes;
         boost::split(sampleTimes, token[1], boost::is_any_of(";"));
         ud->gMbKeys.resize(sampleTimes.size(),ud->gTime);
         for(size_t j=0;j<sampleTimes.size();j++)
            ud->gMbKeys[j] = (float)atof(sampleTimes[j].c_str());
      }
      else
      {
         AiMsgError("[ExocortexAlembicArnold] Invalid dsodata token name '%s' specified!",token[0].c_str());
         return NULL;
      }
   }

   // compute the central time
   ud->gCentroidTime = 0.0f;
   for(size_t j=0;j<ud->gMbKeys.size();j++)
      ud->gCentroidTime += ud->gMbKeys[j];
   if( ud->gMbKeys.size() > 0 ) {
      ud->gCentroidTime /= float(ud->gMbKeys.size());
   }
   ud->gCentroidTime = roundCentroid(ud->gCentroidTime);

   // check if we have all important values
   if(paths[0].length() == 0)
   {
      AiMsgError("[ExocortexAlembicArnold] path token not specified in '%s'.",ud->gDataString);
      return NULL;
   }
   if(identifier.length() == 0)
   {
      AiMsgError("[ExocortexAlembicArnold] identifier token not specified in '%s'.",ud->gDataString);
      return NULL;
   }
   if(ud->gTime == FLT_MAX)
   {
      AiMsgError("[ExocortexAlembicArnold] time token not specified in '%s'.",ud->gDataString);
      return NULL;
   }
   if(ud->gCurrTime == FLT_MAX)
   {
      AiMsgError("[ExocortexAlembicArnold] currtime token not specified in '%s'.",ud->gDataString);
      return NULL;
   }
   if(ud->gMbKeys.size() == 0)
      ud->gMbKeys.push_back(ud->gTime);

   // fix all paths
   for(size_t pathIndex = 0; pathIndex < paths.size(); pathIndex ++)
   {
      // check if we already know this path
      if(!HasFullLicense())
      {
         if(gUsedArchives.size() > 1 && 
            gUsedArchives.find(paths[pathIndex]) == gUsedArchives.end())
         {
            AiMsgError("[ExocortexAlembic] Demo Mode: Only two alembic archives at a time.");
            return NULL;
         }
         gUsedArchives.insert(std::pair<std::string,std::string>(paths[pathIndex],paths[pathIndex]));
      }

#ifdef _WIN32
      // #54: UNC paths don't work
      // add another backslash if we start with a backslash
      if(paths[pathIndex].at(0) == '\\' && paths[pathIndex].at(1) != '\\')
         paths[pathIndex] = "\\" + paths[pathIndex];
#endif

      // resolve tokens
      int insideToken = 0;
      std::string result;
      std::string tokenName;
      std::string tokenValue;

      for(size_t i=0;i<paths[pathIndex].size();i++)
      {
         if(insideToken == 0)
         {
            if(paths[pathIndex][i] == '{')
               insideToken = 1;
            else
               result += paths[pathIndex][i];
         }
         else if(insideToken == 1)
         {
            if(paths[pathIndex][i] == '}')
            {
               AiMsgError("[ExocortexAlembicArnold] Invalid tokens in '%s'.",paths[pathIndex].c_str());
               return NULL;
            }
            else if(paths[pathIndex][i] == ' ')
               insideToken = 2;
            else if(paths[pathIndex][i] == '}')
            {
               insideToken = 0;
               std::transform(tokenName.begin(), tokenName.end(), tokenName.begin(), ::tolower);
               // todo: eventually deal with unary tokens here
               //if(tokenName == "mytoken")
               //{
               //}
               //else
               {
                  AiMsgError("[ExocortexAlembicArnold] Unknown unary token '%s'.",tokenName.c_str());
                  return NULL;
               }
            }
            else
               tokenName += paths[pathIndex][i];
         }
         else if(insideToken == 2)
         {
            if(paths[pathIndex][i] == '{')
            {
               AiMsgError("[ExocortexAlembicArnold] Invalid tokens in '%s'.",paths[pathIndex].c_str());
               return NULL;
            }
            else if(paths[pathIndex][i] == '}')
            {
               // binary tokens
               insideToken = 0;
               std::transform(tokenName.begin(), tokenName.end(), tokenName.begin(), ::tolower);
               if(tokenName == "env")
               {
                  bool found = false;
                  if(getenv(tokenValue.c_str()) != NULL)
                  {
                     std::string envValue = getenv(tokenValue.c_str());
                     if(!envValue.empty())
                     {
                        result += envValue;
                        found = true;
                     }
                  }
                  if(!found)
                  {
                     AiMsgError("[ExocortexAlembicArnold] Environment variable '%s' is not defined.",tokenValue.c_str());
                     return NULL;
                  }
               }
               else
               {
                  AiMsgError("[ExocortexAlembicArnold] Unknown binary token '%s'.",tokenName.c_str());
                  return NULL;
               }
            }
            else
               tokenValue += paths[pathIndex][i];
         }
      }
      paths[pathIndex] = result;
   }

   AiMsgDebug("[ExocortexAlembicArnold] path used: %s",paths[0].c_str());
   AiMsgDebug("[ExocortexAlembicArnold] identifier used: %s",identifier.c_str());
   AiMsgDebug("[ExocortexAlembicArnold] time used: %f",time);

   // now let's test if the archive exists
   FILE * file = fopen(paths[0].c_str(),"rb");
   if(file == NULL)
   {
      AiMsgError("[ExocortexAlembicArnold] File '%s' does not exist.",paths[0].c_str());
      return NULL;
   }
   fclose(file);

   // also check the instancesPath if it is different
   if(paths[1] != paths[0])
   {
      FILE * file = fopen(paths[1].c_str(),"rb");
      if(file == NULL)
      {
         AiMsgError("[ExocortexAlembicArnold] File '%s' does not exist.",paths[1].c_str());
         return NULL;
      }
      fclose(file);
   }

   // open the archive
   ESS_LOG_INFO(paths[0].c_str());
   Alembic::Abc::IArchive archive(Alembic::AbcCoreHDF5::ReadArchive(), paths[0]);
   	if(!archive.getTop().valid()) {
		 AiMsgError("[ExocortexAlembicArnold] Not a valid Alembic data stream.  Path: %s", paths[0].c_str() );
		return NULL;
	}
   ESS_LOG_INFO(paths[1].c_str());
   Alembic::Abc::IArchive instancesArchive(Alembic::AbcCoreHDF5::ReadArchive(), paths[1]);
   	if(!instancesArchive.getTop().valid()) {
		 AiMsgError("[ExocortexAlembicArnold] Not a valid Alembic data stream.  Path: %s", paths[1].c_str() );
		return NULL;
	}

	std::vector<std::string> parts;
   boost::split(parts, identifier, boost::is_any_of("/\\"));
   ud->proceduralDepth = (int)(parts.size() - 2);

   // recurse to find the object
   Alembic::Abc::IObject object = archive.getTop();
   for(size_t i=1;i<parts.size();i++)
   {
      Alembic::Abc::IObject child(object,parts[i]);
      object = child;
      if(!object)
      {
         AiMsgError("[ExocortexAlembicArnold] Cannot find object '%s'.",identifier.c_str());
         return NULL;
      }
   }

   // push all objects to process into the static list
   std::vector<Alembic::Abc::IObject> objects;
   objects.push_back(object);
   for(size_t i=0;i<objects.size();i++)
   {
      if(Alembic::AbcGeom::IXform::matches(objects[i].getMetaData()))
      {
         for(size_t j=0;j<objects[i].getNumChildren();j++)
            objects.push_back(objects[i].getChild(j));
      }
      else if(Alembic::AbcGeom::IPoints::matches(objects[i].getMetaData()))
      {
         // cast to curves
         Alembic::AbcGeom::IPoints typedObject(objects[i],Alembic::Abc::kWrapExisting);

         // first thing to check is if this is an instancing cloud
         if ( typedObject.getSchema().getPropertyHeader( ".shapetype" ) != NULL &&
              typedObject.getSchema().getPropertyHeader( ".shapeinstanceid" ) != NULL &&
              typedObject.getSchema().getPropertyHeader( ".instancenames" ) != NULL )
         {
            size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1 ? typedObject.getSchema().getNumSamples() : ud->gMbKeys.size();
            Alembic::Abc::IUInt16ArrayProperty shapeTypeProp = Alembic::Abc::IUInt16ArrayProperty( typedObject.getSchema(), ".shapetype" );
            Alembic::Abc::IUInt16ArrayProperty shapeInstanceIDProp = Alembic::Abc::IUInt16ArrayProperty( typedObject.getSchema(), ".shapeinstanceid" );
            Alembic::Abc::IStringArrayProperty shapeInstanceNamesProp = Alembic::Abc::IStringArrayProperty( typedObject.getSchema(), ".instancenames" );
            if(shapeTypeProp.getNumSamples() > 0 && shapeInstanceIDProp.getNumSamples() > 0 && shapeInstanceNamesProp.getNumSamples() > 0)
            {
               // ok, we are for sure an instancing cloud...
               // let's skip this, we'll do it the second time around
               continue;
            }
         }

         objectInfo info(ud->gCentroidTime);
         info.abc = objects[i];
         ud->gIObjects.push_back(info);
      }
      else
      {
         objectInfo info(ud->gCentroidTime);
         info.abc = objects[i];
         ud->gIObjects.push_back(info);
      }
   }

   // loop a second time, only for the instancing clouds
   for(size_t i=0;i<objects.size();i++)
   {
      if(Alembic::AbcGeom::IPoints::matches(objects[i].getMetaData()))
      {
         // cast to curves
         Alembic::AbcGeom::IPoints typedObject(objects[i],Alembic::Abc::kWrapExisting);

         // first thing to check is if this is an instancing cloud
         if ( typedObject.getSchema().getPropertyHeader( ".shapetype" ) != NULL &&
              typedObject.getSchema().getPropertyHeader( ".shapeinstanceid" ) != NULL &&
              typedObject.getSchema().getPropertyHeader( ".instancenames" ) != NULL )
         {
            size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1 ? typedObject.getSchema().getNumSamples() : ud->gMbKeys.size();
            Alembic::Abc::IUInt16ArrayProperty shapeTypeProp = Alembic::Abc::IUInt16ArrayProperty( typedObject.getSchema(), ".shapetype" );
            Alembic::Abc::IUInt16ArrayProperty shapeInstanceIDProp = Alembic::Abc::IUInt16ArrayProperty( typedObject.getSchema(), ".shapeinstanceid" );
            Alembic::Abc::IStringArrayProperty shapeInstanceNamesProp = Alembic::Abc::IStringArrayProperty( typedObject.getSchema(), ".instancenames" );
            if(shapeTypeProp.getNumSamples() > 0 && shapeInstanceIDProp.getNumSamples() > 0 && shapeInstanceNamesProp.getNumSamples() > 0)
            {
               // check all of the nodes to be instanced
               // for this we will use the very last sample
               instanceCloudInfo cloudInfo;
               Alembic::Abc::StringArraySamplePtr lastSampleInstanceNames = shapeInstanceNamesProp.getValue(shapeInstanceNamesProp.getNumSamples()-1);
               for(size_t j=0;j<lastSampleInstanceNames->size();j++)
               {
                  instanceGroupInfo groupInfo;
                  std::string identifier = lastSampleInstanceNames->get()[j];

                  // first, we need to figure out if this is a transform
                  bool found = false;
                  for(size_t k=0;k<ud->gIObjects.size();k++)
                  {
                     if(ud->gIObjects[k].abc.getFullName() == identifier)
                     {
                        // if we find the object in our export list, it is not a transform!
                        // we don't push matrices, so that the transform is used without an offset.
                        // furthermore we push 
                        groupInfo.identifiers.push_back(identifier);
                        groupInfo.objects.push_back(ud->gIObjects[k].abc);
                        groupInfo.nodes.push_back(std::map<float,AtNode*>());
                        groupInfo.nodes[groupInfo.nodes.size()-1].insert(std::pair<float,AtNode*>(ud->gCentroidTime,NULL));
                        found = true;
                        break;
                     }
                  }
                  // only do this search if we don't require time shifting
                  AtNode * masterNode = NULL;
                  if(!found)
                  {
                     // this means we have not exported this yes, neither did we find it in Arnold
                     // as an exported node. so we will search in the alembic file for it
                     std::vector<std::string> parts;
                     boost::split(parts, identifier, boost::is_any_of("/\\"));

                     // recurse to find the object
                     objectInfo info(ud->gCentroidTime);
                     info.hide = true;
                     info.abc = instancesArchive.getTop();
                     info.suffix = "_INSTANCE";
                     found = true;
                     for(size_t k=1;k<parts.size();k++)
                     {
                        Alembic::Abc::IObject child(info.abc,parts[k]);
                        info.abc = child;
                        if(!info.abc)
                        {
                           found = false;
                           break;
                        }
                     }
                     if(found)
                     {
                        // if this is an alembic transform, then we need to build exports for every
                        // shape below it
                        if(Alembic::AbcGeom::IXform::matches(info.abc.getMetaData()))
                        {
                           // collect all of the children 
                           Alembic::Abc::IObject parent = info.abc;
                           std::vector<Alembic::Abc::IObject> children;
                           children.push_back(info.abc);
                           for(size_t k=0;k<children.size();k++)
                           {
                              Alembic::Abc::IObject child = children[k];
                              if(Alembic::AbcGeom::IXform::matches(child.getMetaData()))
                              {
                                 for(size_t l=0;l<child.getNumChildren();l++)
                                    children.push_back(child.getChild(l));
                              }
                              else
                              {
                                 // check if we already exported this object
                                 // and push it to the export list if we didn't
                                 bool nodeFound = false;
                                 masterNode = AiNodeLookUpByName((getNameFromIdentifier(child.getFullName())+"_DSO").c_str());
                                 if(!masterNode)
                                    masterNode = AiNodeLookUpByName(getNameFromIdentifier(child.getFullName()).c_str());
                                 for(size_t l=0;l<ud->gIObjects.size();l++)
                                 {
                                    if(ud->gIObjects[l].abc.getFullName() == child.getFullName())
                                    {
                                       nodeFound = true;
                                       break;
                                    }
                                 }
                                 if(!nodeFound && masterNode == NULL)
                                 {
                                    objectInfo childInfo(ud->gCentroidTime);
                                    childInfo.hide = true;
                                    childInfo.abc = child;
                                    ud->gIObjects.push_back(childInfo);
                                 }

                                 // push this to our group info
                                 groupInfo.identifiers.push_back(child.getFullName());
                                 groupInfo.objects.push_back(child);
                                 groupInfo.nodes.push_back(std::map<float,AtNode*>());
                                 groupInfo.nodes[groupInfo.nodes.size()-1].insert(std::pair<float,AtNode*>(ud->gCentroidTime,masterNode));

                                 // we also store the parent
                                 // this will enforce to compute actual offset matrices for each given time
                                 groupInfo.parents.push_back(parent);
                                 groupInfo.matrices.push_back(std::map<float,std::vector<Alembic::Abc::M44f> >());
                              }
                           }
                        }
                        else
                        {
                           // just push it for the export
                           if(masterNode == NULL)
                              ud->gIObjects.push_back(info);

                           // also update our groupInfo
                           groupInfo.identifiers.push_back(identifier);
                           groupInfo.objects.push_back(info.abc);
                           groupInfo.nodes.push_back(std::map<float,AtNode*>());
                           groupInfo.nodes[groupInfo.nodes.size()-1].insert(std::pair<float,AtNode*>(ud->gCentroidTime,masterNode));
                        }
                     }
                  }

                  if(!found)
                  {
                     // let's try to find the node inside Arnold directly.
                     masterNode = AiNodeLookUpByName((getNameFromIdentifier(identifier)+"_DSO").c_str());
                     if(!masterNode)
                        masterNode = AiNodeLookUpByName(getNameFromIdentifier(identifier).c_str());

                     // if we now found a node, we can use it straight
                     if(masterNode != NULL && cloudInfo.time.size() == 0) 
                     {
                        groupInfo.identifiers.push_back(identifier);
                        groupInfo.objects.push_back(Alembic::Abc::IObject());
                        groupInfo.nodes.push_back(std::map<float,AtNode*>());
                        groupInfo.nodes[groupInfo.nodes.size()-1].insert(std::pair<float,AtNode*>(ud->gCentroidTime,masterNode));
                        found = true;
                     }
                  }

                  // if we found this, let's push it
                  if(found)
                  {
                     cloudInfo.groupInfos.push_back(groupInfo);
                  }
                  else
                  {
                     AiMsgError("[ExocortexAlembicArnold] Identifier '%s' is not part of file '%s'. Aborting.",identifier.c_str(),paths[1].c_str());
                     return FALSE;
                  }
               }

               Alembic::AbcGeom::IPointsSchema::Sample sampleFloor;
               Alembic::AbcGeom::IPointsSchema::Sample sampleCeil;

               // now let's get all of the positions etc
               for(size_t j=0;j<minNumSamples;j++)
               {
                 SampleInfo sampleInfo = getSampleInfo(
                     ud->gMbKeys[j],
                     typedObject.getSchema().getTimeSampling(),
                     typedObject.getSchema().getNumSamples()
                  );
                  typedObject.getSchema().get(sampleFloor,sampleInfo.floorIndex);
                  typedObject.getSchema().get(sampleCeil,sampleInfo.ceilIndex);
                  
                  cloudInfo.pos.push_back(sampleFloor.getPositions());
                  cloudInfo.vel.push_back(sampleFloor.getVelocities());
                  cloudInfo.id.push_back(sampleFloor.getIds());
                  cloudInfo.pos.push_back(sampleCeil.getPositions());
                  cloudInfo.vel.push_back(sampleCeil.getVelocities());
                  cloudInfo.id.push_back(sampleCeil.getIds());
               }

               // store the widths
               Alembic::AbcGeom::IFloatGeomParam widthParam = typedObject.getSchema().getWidthsParam();
               if(widthParam.valid())
               {
                  for(size_t j=0;j<minNumSamples;j++)
                  {
                    SampleInfo sampleInfo = getSampleInfo(
                        ud->gMbKeys[j],
                        widthParam.getTimeSampling(),
                        widthParam.getNumSamples()
                     );
                     cloudInfo.width.push_back(widthParam.getExpandedValue(sampleInfo.floorIndex).getVals());
                     cloudInfo.width.push_back(widthParam.getExpandedValue(sampleInfo.ceilIndex).getVals());
                  }
               }

               // store the scale
               if ( typedObject.getSchema().getPropertyHeader( ".scale" ) != NULL )
               {
                  Alembic::Abc::IV3fArrayProperty prop = Alembic::Abc::IV3fArrayProperty( typedObject.getSchema(), ".scale" );
                  if(prop.valid())
                  {
                     if(prop.getNumSamples() > 0)
                     {
                        for(size_t j=0;j<minNumSamples;j++)
                        {
                          SampleInfo sampleInfo = getSampleInfo(
                              ud->gMbKeys[j],
                              prop.getTimeSampling(),
                              prop.getNumSamples()
                           );
                           cloudInfo.scale.push_back(prop.getValue(sampleInfo.floorIndex));
                           cloudInfo.scale.push_back(prop.getValue(sampleInfo.ceilIndex));
                        }
                     }
                  }
               }

               // store the orientation
               if ( typedObject.getSchema().getPropertyHeader( ".orientation" ) != NULL )
               {
                  Alembic::Abc::IQuatfArrayProperty prop = Alembic::Abc::IQuatfArrayProperty( typedObject.getSchema(), ".orientation" );
                  if(prop.valid())
                  {
                     if(prop.getNumSamples() > 0)
                     {
                        for(size_t j=0;j<minNumSamples;j++)
                        {
                          SampleInfo sampleInfo = getSampleInfo(
                              ud->gMbKeys[j],
                              prop.getTimeSampling(),
                              prop.getNumSamples()
                           );
                           cloudInfo.rot.push_back(prop.getValue(sampleInfo.floorIndex));
                        }
                     }
                  }
               }

               // store the angular velocity
               if ( typedObject.getSchema().getPropertyHeader( ".angularvelocity" ) != NULL )
               {
                  Alembic::Abc::IQuatfArrayProperty prop = Alembic::Abc::IQuatfArrayProperty( typedObject.getSchema(), ".angularvelocity" );
                  if(prop.valid())
                  {
                     if(prop.getNumSamples() > 0)
                     {
                        for(size_t j=0;j<minNumSamples;j++)
                        {
                          SampleInfo sampleInfo = getSampleInfo(
                              ud->gMbKeys[j],
                              prop.getTimeSampling(),
                              prop.getNumSamples()
                           );
                           cloudInfo.ang.push_back(prop.getValue(sampleInfo.floorIndex));
                        }
                     }
                  }
               }

               // store the age
               if ( typedObject.getSchema().getPropertyHeader( ".age" ) != NULL )
               {
                  Alembic::Abc::IFloatArrayProperty prop = Alembic::Abc::IFloatArrayProperty( typedObject.getSchema(), ".age" );
                  if(prop.valid())
                  {
                     if(prop.getNumSamples() > 0)
                     {
                        for(size_t j=0;j<minNumSamples;j++)
                        {
                          SampleInfo sampleInfo = getSampleInfo(
                              ud->gMbKeys[j],
                              prop.getTimeSampling(),
                              prop.getNumSamples()
                           );
                           cloudInfo.age.push_back(prop.getValue(sampleInfo.floorIndex));
                        }
                     }
                  }
               }

               // store the mass
               if ( typedObject.getSchema().getPropertyHeader( ".mass" ) != NULL )
               {
                  Alembic::Abc::IFloatArrayProperty prop = Alembic::Abc::IFloatArrayProperty( typedObject.getSchema(), ".mass" );
                  if(prop.valid())
                  {
                     if(prop.getNumSamples() > 0)
                     {
                        for(size_t j=0;j<minNumSamples;j++)
                        {
                          SampleInfo sampleInfo = getSampleInfo(
                              ud->gMbKeys[j],
                              prop.getTimeSampling(),
                              prop.getNumSamples()
                           );
                           cloudInfo.mass.push_back(prop.getValue(sampleInfo.floorIndex));
                        }
                     }
                  }
               }

               // store the shape id
               if ( typedObject.getSchema().getPropertyHeader( ".shapeinstanceid" ) != NULL )
               {
                  Alembic::Abc::IUInt16ArrayProperty prop = Alembic::Abc::IUInt16ArrayProperty( typedObject.getSchema(), ".shapeinstanceid" );
                  if(prop.valid())
                  {
                     if(prop.getNumSamples() > 0)
                     {
                        for(size_t j=0;j<minNumSamples;j++)
                        {
                          SampleInfo sampleInfo = getSampleInfo(
                              ud->gMbKeys[j],
                              prop.getTimeSampling(),
                              prop.getNumSamples()
                           );
                           cloudInfo.shape.push_back(prop.getValue(sampleInfo.floorIndex));
                        }
                     }
                  }
               }

               // store the shape time
               if ( typedObject.getSchema().getPropertyHeader( ".shapetime" ) != NULL )
               {
                  Alembic::Abc::IFloatArrayProperty prop = Alembic::Abc::IFloatArrayProperty( typedObject.getSchema(), ".shapetime" );
                  if(prop.valid())
                  {
                     if(prop.getNumSamples() > 0)
                     {
                        SampleInfo sampleInfo = getSampleInfo(
                           ud->gCentroidTime,
                           prop.getTimeSampling(),
                           prop.getNumSamples()
                        );
                        cloudInfo.time.push_back(prop.getValue(sampleInfo.floorIndex));
                        cloudInfo.timeAlpha = (float)sampleInfo.alpha;
                        if(sampleInfo.alpha != 0.0)
                           cloudInfo.time.push_back(prop.getValue(sampleInfo.ceilIndex));
                     }
                  }
               }

               // store the color
               if ( typedObject.getSchema().getPropertyHeader( ".color" ) != NULL )
               {
                  Alembic::Abc::IC4fArrayProperty prop = Alembic::Abc::IC4fArrayProperty( typedObject.getSchema(), ".color" );
                  if(prop.valid())
                  {
                     if(prop.getNumSamples() > 0)
                     {
                        for(size_t j=0;j<minNumSamples;j++)
                        {
                          SampleInfo sampleInfo = getSampleInfo(
                              ud->gMbKeys[j],
                              prop.getTimeSampling(),
                              prop.getNumSamples()
                           );
                           cloudInfo.color.push_back(prop.getValue(sampleInfo.floorIndex));
                        }
                     }
                  }
               }

               // now check if we have the time offsets, and if so let's export all of these master nodes as well
               if(cloudInfo.time.size() > 0 && cloudInfo.shape.size() > 0)
               {
                  Alembic::Abc::FloatArraySamplePtr times = cloudInfo.time[0];
                  Alembic::Abc::UInt16ArraySamplePtr shapes = cloudInfo.shape[cloudInfo.shape.size()-1];

                  // for all of the particles
                  size_t numParticles = times->size() > shapes->size() ? times->size() : shapes->size();
                  for(size_t j=0;j<numParticles;j++)
                  {
                     float centroidTime = times->get()[j < times->size() ? j : times->size()-1];
                     if(cloudInfo.time.size() > 1)
                     {
                        // interpolate the time if necessary
                        centroidTime = (1.0f - cloudInfo.timeAlpha) * centroidTime + cloudInfo.timeAlpha * 
                           cloudInfo.time[1]->get()[j < cloudInfo.time[1]->size() ? j : cloudInfo.time[1]->size() - 1];
                     }
                     centroidTime = roundCentroid(centroidTime);

                     // get the id of the shape
                     size_t shapeID = (size_t)shapes->get()[j < shapes->size() ? j : shapes->size()-1];

                     // check if we have this shapeID already
                     if(shapeID >= cloudInfo.groupInfos.size())
                     {
                        AiMsgError("[ExocortexAlembicArnold] shapeID '%d' is not valid. Aborting.",(int)shapeID);
                        return NULL;
                     }

                     instanceGroupInfo * groupInfo = &cloudInfo.groupInfos[shapeID];
                     for(size_t g=0;g<groupInfo->identifiers.size();g++)
                     {
                        std::map<float,AtNode*>::iterator it = groupInfo->nodes[g].find(centroidTime);
                        if(it == groupInfo->nodes[g].end())
                        {
                           // check if we have this gObject somewhere...
                           Alembic::Abc::IObject abcMasterObject;
                           objectInfo objInfo(ud->gCentroidTime);
                           objInfo.hide = true;
                           for(size_t k=0;k<ud->gIObjects.size();k++)
                           {
                              if(ud->gIObjects[k].abc.getFullName() == groupInfo->identifiers[g])
                              {
                                 objInfo.abc = ud->gIObjects[k].abc;
                                 break;
                              }
                           }
                           if(!objInfo.abc.valid() && groupInfo->objects[g].valid())
                           {
                              objInfo.abc = groupInfo->objects[g];
                           }
                           if(!objInfo.abc.valid())
                           {
                              AiMsgError("[ExocortexAlembicArnold] Identifier '%s' is not part of file '%s'. Aborting.",groupInfo->identifiers[g].c_str(),paths[1].c_str());
                              return NULL;
                           }

                           // push it to the map. This way we can ensure to export it!
                           objInfo.centroidTime = centroidTime;
                           ud->gIObjects.push_back(objInfo);

                           groupInfo->nodes[g].insert(std::pair<float,AtNode*>(centroidTime,NULL));

                        }
                     }
                  }
               }

               ud->gInstances.push_back(cloudInfo);

               // now let's get the number of position of the first sample
               // and create an instance export for that one
               for(size_t j=0;j<cloudInfo.pos[0]->size();j++)
               {
                  objectInfo objInfo(ud->gCentroidTime);
                  objInfo.abc = objects[i];
                  objInfo.instanceCloud = &ud->gInstances[ud->gInstances.size()-1];
                  objInfo.instanceID = (long)(cloudInfo.shape[0]->size() > j ? cloudInfo.shape[0]->get()[j] : cloudInfo.shape[0]->get()[0]);
                  objInfo.ID = (long)j;
                  for(size_t k=0;k<cloudInfo.groupInfos[objInfo.instanceID].nodes.size();k++)
                  {
                     objInfo.instanceGroupID = (long)k;
                     ud->gIObjects.push_back(objInfo);
                  }
               }
            }
         }
      }
   }
   return TRUE;
}

// All done, deallocate stuff
static int Cleanup(void *user_ptr)
{
	GLOBAL_LOCK;

	userData * ud = (userData*)user_ptr;

   // assign remaining shaders
   //for(size_t i=0;i<ud->constructedNodes.size();i++)
   //{
   //   if(ud->shadersToAssign[i] == NULL)
   //      continue;
   //   AiNodeSetArray(ud->constructedNodes[i],"shader",ud->shadersToAssign[i]);
   //}

   ud->gIObjects.clear();
   ud->gInstances.clear();
   delete(ud);

   return TRUE;
}


// Get number of nodes
static int NumNodes(void *user_ptr)
{
	GLOBAL_LOCK;

	userData * ud = (userData*)user_ptr;
   int size = (int)ud->gIObjects.size();
   return size;
}


// Get the i_th node
static AtNode *GetNode(void *user_ptr, int i)
{
	GLOBAL_LOCK;

	userData * ud = (userData*)user_ptr;
   // check if this is a known object
   if(i >= (int)ud->gIObjects.size())
      return NULL;

   // now check the camera node for shutter settings
   AtNode * cameraNode = AiUniverseGetCamera();
   float shutterStart = AiNodeGetFlt(cameraNode,"shutter_start");
   float shutterEnd = AiNodeGetFlt(cameraNode,"shutter_end");

   // construct the timesamples
   size_t nbSamples = ud->gMbKeys.size();
   std::vector<float> samples = ud->gMbKeys;
   for(size_t j=0;j<nbSamples;j++)
      samples[j] += ud->gIObjects[i].centroidTime - ud->gCentroidTime;
   bool shifted = fabsf(ud->gIObjects[i].centroidTime - ud->gCentroidTime) > 0.001f;
   bool createdShifted = shifted;

   // create the resulting node
   AtNode * shapeNode = NULL;

   // now check if this is supposed to be an instance
   if(ud->gIObjects[i].instanceID > -1)
   {
	   ESS_LOG_INFO( "ExocortexAlembicArnoldDSO: GetNode: InstanceID: " + ud->gIObjects[i].instanceID );
       Alembic::AbcGeom::IPoints typedObject(ud->gIObjects[i].abc,Alembic::Abc::kWrapExisting);

      instanceCloudInfo * info = ud->gIObjects[i].instanceCloud;

      // check that we have the masternode
      size_t id = (size_t)ud->gIObjects[i].ID;
      size_t instanceID = (size_t)ud->gIObjects[i].instanceID;
      if(instanceID >= info->groupInfos.size())
      {
         AiMsgError("[ExocortexAlembicArnold] Instance '%s.%d' has an invalid instanceID  . Aborting.",ud->gIObjects[i].abc.getFullName().c_str(),(int)id);
         return NULL;
      }
      size_t groupID = (size_t)ud->gIObjects[i].instanceGroupID;
      if(groupID >= info->groupInfos[instanceID].identifiers.size())
      {
         AiMsgError("[ExocortexAlembicArnold] Instance '%s.%d' has an invalid instanceGroupID. Aborting.",ud->gIObjects[i].abc.getFullName().c_str(),(int)id);
         return NULL;
      }

      instanceGroupInfo * group = &info->groupInfos[instanceID];

      // get the right centroidTime
      float centroidTime = ud->gCentroidTime;
      if(info->time.size() > 0)
      {
         centroidTime = info->time[0]->get()[id < info->time[0]->size() ? id : info->time[0]->size() - 1];
         if(info->time.size() > 1)
         {
            centroidTime = (1.0f - info->timeAlpha) * centroidTime + info->timeAlpha * 
               info->time[1]->get()[id < info->time[1]->size() ? id : info->time[1]->size() - 1];
         }
         centroidTime = roundCentroid(centroidTime);
      }

      std::map<float,AtNode*>::iterator it = group->nodes[groupID].find(centroidTime);
      if(it == group->nodes[groupID].end())
      {
         AiMsgError("[ExocortexAlembicArnold] Cannot find masterNode '%s' for centroidTime '%f'. Aborting.",group->identifiers[groupID].c_str(),centroidTime);
         return NULL;
      }
      AtNode * usedMasterNode = it->second;

      shapeNode = AiNode("ginstance");

      // setup name, id and the master node
      AiNodeSetStr(shapeNode, "name", getNameFromIdentifier(ud->gIObjects[i].abc.getFullName(),ud->gIObjects[i].ID,(long)groupID).c_str());
      AiNodeSetInt(shapeNode, "id", ud->gIObjects[i].instanceID); 
      AiNodeSetPtr(shapeNode, "node", usedMasterNode);

      // declare color on the ginstance
      if(info->color.size() > 0)
      {
         if (AiNodeDeclare(shapeNode, "Color", "constant RGBA"))
         {
            Alembic::Abc::C4f color = info->color[0]->get()[id < info->color[0]->size() ? id : info->color[0]->size() - 1];
            AiNodeSetRGBA(shapeNode, "Color", color.r, color.g, color.b, color.a);
         }
      }

      // now let's take care of the transform
      AtArray * matrices = AiArrayAllocate(1,(AtInt)ud->gMbKeys.size(),AI_TYPE_MATRIX);
      for(size_t j=0;j<ud->gMbKeys.size();j++)
      {
         SampleInfo sampleInfo = getSampleInfo(
            ud->gMbKeys[j],
            typedObject.getSchema().getTimeSampling(),
            typedObject.getSchema().getNumSamples()
         );

         Alembic::Abc::M44f matrixAbc;
         matrixAbc.makeIdentity();
         size_t floorIndex = j * 2 + 0;
         size_t ceilIndex = j * 2 + 1;

         // apply translation
         if(info->pos[floorIndex]->size() == info->pos[ceilIndex]->size())
         {
            matrixAbc.setTranslation(float(1.0 - sampleInfo.alpha) * info->pos[floorIndex]->get()[id < info->pos[floorIndex]->size() ? id : info->pos[floorIndex]->size() - 1] + 
                                     float(sampleInfo.alpha) * info->pos[ceilIndex]->get()[id < info->pos[ceilIndex]->size() ? id : info->pos[ceilIndex]->size() - 1]);
         }
         else
         {
            matrixAbc.setTranslation(info->pos[floorIndex]->get()[id < info->pos[floorIndex]->size() ? id : info->pos[floorIndex]->size() - 1] + 
                                     info->vel[floorIndex]->get()[id < info->vel[floorIndex]->size() ? id : info->vel[floorIndex]->size() - 1] * (float)sampleInfo.alpha);
         }

         // now take care of rotation
         if(info->rot.size() == ud->gMbKeys.size())
         {
            Alembic::Abc::Quatf rotAbc = info->rot[j]->get()[id < info->rot[j]->size() ? id : info->rot[j]->size() - 1];
            if(info->ang.size() == ud->gMbKeys.size() && sampleInfo.alpha > 0.0)
            {
               Alembic::Abc::Quatf angAbc = info->ang[j]->get()[id < info->ang[j]->size() ? id : info->ang[j]->size() -1] * (float)sampleInfo.alpha;
               if(angAbc.axis().length2() != 0.0f && angAbc.r != 0.0f)
               {
                  rotAbc = angAbc * rotAbc;
                  rotAbc.normalize();
               }
            }
            Alembic::Abc::M44f matrixAbcRot;
            matrixAbcRot.setAxisAngle(rotAbc.axis(),rotAbc.angle());
            matrixAbc = matrixAbcRot * matrixAbc;
         }

         // and finally scaling
         if(info->scale.size() == ud->gMbKeys.size() * 2)
         {
            Alembic::Abc::V3f scalingAbc = info->scale[floorIndex]->get()[id < info->scale[floorIndex]->size() ? id : info->scale[floorIndex]->size() - 1] * 
                                           info->width[floorIndex]->get()[id < info->width[floorIndex]->size() ? id : info->width[floorIndex]->size() - 1] * float(1.0 - sampleInfo.alpha) + 
                                           info->scale[ceilIndex]->get()[id < info->scale[ceilIndex]->size() ? id : info->scale[ceilIndex]->size() - 1] * 
                                           info->width[ceilIndex]->get()[id < info->width[ceilIndex]->size() ? id : info->width[ceilIndex]->size() - 1] * float(sampleInfo.alpha);
            matrixAbc.scale(scalingAbc);
         }
         else
         {
            float width = info->width[floorIndex]->get()[id < info->width[floorIndex]->size() ? id : info->width[floorIndex]->size() - 1] * float(1.0 - sampleInfo.alpha) + 
                          info->width[ceilIndex]->get()[id < info->width[ceilIndex]->size() ? id : info->width[ceilIndex]->size() - 1] * float(sampleInfo.alpha);
            matrixAbc.scale(Alembic::Abc::V3f(width,width,width));
         }

         // if we have offset matrices
         if(group->parents.size() > groupID && group->matrices.size() > groupID)
         {
            if(group->objects[groupID].valid() && group->parents[groupID].valid())
            {
               // we have a matrix map and a parent.
               // now we need to check if we already exported the matrices
               std::map<float,std::vector<Alembic::Abc::M44f> >::iterator it;
               std::vector<Alembic::Abc::M44f> offsets;
               it = group->matrices[groupID].find(centroidTime);
               if(it == group->matrices[groupID].end())
               {
                  std::vector<float> samples(ud->gMbKeys.size());
                  offsets.resize(ud->gMbKeys.size());
                  for(AtInt sampleIndex=0;sampleIndex<(AtInt)ud->gMbKeys.size();sampleIndex++)
                  {
                     offsets[sampleIndex].makeIdentity();
                     // centralize the time once more
                     samples[sampleIndex] = centroidTime + ud->gMbKeys[sampleIndex] - ud->gCentroidTime;
                  }

                  // if the transform differs, we need to compute the offset matrices
                  // get the parent, which should be a transform
                  Alembic::Abc::IObject parent = group->parents[groupID];
                  Alembic::Abc::IObject xform = group->objects[groupID].getParent();
                  while(Alembic::AbcGeom::IXform::matches(xform.getMetaData()) && xform.getFullName() != parent.getFullName())
                  {
                     // cast to a xform
                     Alembic::AbcGeom::IXform parentXform(xform,Alembic::Abc::kWrapExisting);
                     if(parentXform.getSchema().getNumSamples() == 0)
                        break;

                     // loop over all samples
                     for(size_t sampleIndex=0;sampleIndex<ud->gMbKeys.size();sampleIndex++)
                     {
                        SampleInfo sampleInfo = getSampleInfo(
                           samples[sampleIndex],
                           parentXform.getSchema().getTimeSampling(),
                           parentXform.getSchema().getNumSamples()
                        );

                        // get the data and blend it if necessary
                        Alembic::AbcGeom::XformSample sample;
                        parentXform.getSchema().get(sample,sampleInfo.floorIndex);
                        Alembic::Abc::M44f abcMatrix;
                        Alembic::Abc::M44d abcMatrixd = sample.getMatrix();
                        for(int x=0;x<4;x++)
                           for(int y=0;y<4;y++)
                              abcMatrix[x][y] = (float)abcMatrixd[x][y];
                             
                        if(sampleInfo.alpha >= sampleTolerance)
                        {
                           parentXform.getSchema().get(sample,sampleInfo.ceilIndex);
                           Alembic::Abc::M44d ceilAbcMatrixd = sample.getMatrix();
                           Alembic::Abc::M44f ceilAbcMatrix;
                           for(int x=0;x<4;x++)
                              for(int y=0;y<4;y++)
                                 ceilAbcMatrix[x][y] = (float)ceilAbcMatrixd[x][y];
                           abcMatrix = float(1.0 - sampleInfo.alpha) * abcMatrix + float(sampleInfo.alpha) * ceilAbcMatrix;
                        }

                        offsets[sampleIndex] = abcMatrix * offsets[sampleIndex];
                     }

                     // go upwards
                     xform = xform.getParent();
                  }
                  group->matrices[groupID].insert(std::pair<float,std::vector<Alembic::Abc::M44f> >(centroidTime,offsets));
               }
               else
                  offsets = it->second;

               // this means we have the right amount of matrices to blend against
               if(offsets.size() > j)
                  matrixAbc = offsets[j] * matrixAbc;
            }
         }

         // store it to the array
         AiArraySetMtx(matrices,(AtULong)j,matrixAbc.x);
      }

      AiNodeSetArray(shapeNode,"matrix",matrices);
      AiNodeSetBool(shapeNode, "inherit_xform", FALSE);

      return shapeNode;
   }

   AtArray * shaders = NULL;
   AtArray * shaderIndices = NULL;

   // check what kind of object it is
   Alembic::Abc::IObject object = ud->gIObjects[i].abc;
   
	if(!object.valid()) {
		 AiMsgError("[ExocortexAlembicArnold] Not a valid Alembic data stream." );
		return NULL;
	}

   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IPolyMesh::matches(md))
   {
           ESS_LOG_INFO(object.getFullName().c_str());

	   ESS_LOG_INFO( "ExocortexAlembicArnoldDSO: GetNode: IPolyMesh" );
		

      // cast to polymesh and ensure we have got the 
      // normals parameter!
      Alembic::AbcGeom::IPolyMesh typedObject(object,Alembic::Abc::kWrapExisting);
      size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1 ? typedObject.getSchema().getNumSamples() : samples.size();
      Alembic::AbcGeom::IN3fGeomParam normalParam = typedObject.getSchema().getNormalsParam();
      if(!normalParam.valid())
      {
         AiMsgError("[ExocortexAlembicArnold] Mesh '%s' does not contain normals. Aborting.",object.getFullName().c_str());
         return NULL;
      }
     // super_debug_var = 0;

      // create the arnold node
      if(shifted)
      {
         bool doBreak = false;
         for(size_t j=0;j<ud->gInstances.size();j++)
         {
            for(size_t k=0;k<ud->gInstances[j].groupInfos.size();k++)
            {
               for(size_t l=0;k<ud->gInstances[j].groupInfos[k].identifiers.size();l++)
               {
                  if(ud->gInstances[j].groupInfos[k].identifiers[l] == object.getFullName())
                  {
                     std::map<float,AtNode*>::iterator it = ud->gInstances[j].groupInfos[k].nodes[l].find(ud->gCentroidTime);
                     if(it != ud->gInstances[j].groupInfos[k].nodes[l].end())
                     {
                        if(it->second != NULL)
                        {
                           shaders = AiNodeGetArray(it->second, "shader");
                           //shapeNode = AiNodeClone(it->second);
                        }
                     }
                     doBreak = true;
                     break;
                  }
               }
               if(doBreak) break;
            }
            if(doBreak) break;
         }
      }
      if(!shapeNode)
      {
         shapeNode = AiNode("polymesh");
         createdShifted = false;
      }

      // create arrays to hold the data
      AtArray * pos = NULL;
      AtArray * nor = NULL;
      AtArray * uvsIdx = NULL;

      // check if we have dynamic topology
      bool dynamicTopology = false;
      Alembic::Abc::IInt32ArrayProperty faceIndicesProp = Alembic::Abc::IInt32ArrayProperty(typedObject.getSchema(),".faceIndices");
      if(faceIndicesProp.valid())
         dynamicTopology = !faceIndicesProp.isConstant();

      // loop over all samples
      AtULong posOffset = 0;
      AtULong norOffset = 0;
      size_t firstSampleCount = 0;
      Alembic::Abc::Int32ArraySamplePtr abcFaceCounts;
      Alembic::Abc::Int32ArraySamplePtr abcFaceIndices;

      for(size_t sampleIndex = 0; sampleIndex < minNumSamples; sampleIndex++)
      {
         SampleInfo sampleInfo = getSampleInfo(
            samples[sampleIndex],
            typedObject.getSchema().getTimeSampling(),
            typedObject.getSchema().getNumSamples()
         );

         // get the floor sample
         Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
         typedObject.getSchema().get(sample,sampleInfo.floorIndex);

         // take care of the topology
         if(sampleIndex == 0 && !createdShifted)
         {
            abcFaceCounts = sample.getFaceCounts();
            abcFaceIndices = sample.getFaceIndices();
            if(abcFaceCounts->get()[0] == 0)
               return NULL;
            AtArray * faceCounts = AiArrayAllocate((AtInt)abcFaceCounts->size(), 1, AI_TYPE_UINT);
            AtArray * faceIndices = AiArrayAllocate((AtInt)abcFaceIndices->size(), 1, AI_TYPE_UINT);
            uvsIdx = AiArrayAllocate((AtInt)(abcFaceIndices->size()),1,AI_TYPE_UINT);
            AtUInt offset = 0;
            for(AtULong i=0;i<faceCounts->nelements;i++)
            {
               AiArraySetUInt(faceCounts,i,abcFaceCounts->get()[i]);
               for(AtLong j=0;j<abcFaceCounts->get()[i];j++)
               {
                  AiArraySetUInt(faceIndices,offset+j,abcFaceIndices->get()[offset+abcFaceCounts->get()[i]-(j+1)]);
                  AiArraySetUInt(uvsIdx,offset+j,offset+abcFaceCounts->get()[i]-(j+1));
               }
               offset += abcFaceCounts->get()[i];
            }
            AiNodeSetArray(shapeNode, "nsides", faceCounts);
            AiNodeSetArray(shapeNode, "vidxs", faceIndices);
            AiNodeSetBool(shapeNode, "smoothing", true);

            // check if we have UVs in the alembic file
            Alembic::AbcGeom::IV2fGeomParam uvParam= typedObject.getSchema().getUVsParam();
            if(uvParam.valid())
            {
               Alembic::Abc::V2fArraySamplePtr abcUvs = uvParam.getExpandedValue(sampleInfo.floorIndex).getVals();
               AtArray * uvs = AiArrayAllocate((AtInt)abcUvs->size() * 2, 1, AI_TYPE_FLOAT);
               AtULong offset = 0;
               for(AtULong i=0;i<abcUvs->size();i++)
               {
                  AiArraySetFlt(uvs,offset++,abcUvs->get()[i].x);
                  AiArraySetFlt(uvs,offset++,abcUvs->get()[i].y);
               }
               AiNodeSetArray(shapeNode, "uvlist", uvs);
               AiNodeSetArray(shapeNode, "uvidxs", AiArrayCopy(uvsIdx));

               // check if we have uvOptions
               if(typedObject.getSchema().getPropertyHeader( ".uvOptions" ) != NULL)
               {
                  Alembic::Abc::IFloatArrayProperty prop = Alembic::Abc::IFloatArrayProperty( typedObject.getSchema(), ".uvOptions" );
                  if(prop.getNumSamples() > 0)
                  {
                     Alembic::Abc::FloatArraySamplePtr ptr = prop.getValue(0);
                     if(ptr->size() > 1)
                     {
                        bool uWrap = ptr->get()[0] != 0.0f;
                        bool vWrap = ptr->get()[1] != 0.0f;

                        // fill the arnold array
                        AtArray * uvOptions = AiArrayAllocate(2,1,AI_TYPE_BOOLEAN);
                        AiArraySetBool(uvOptions,0,uWrap);
                        AiArraySetBool(uvOptions,1,vWrap);

                        // we need to define this two times, once for a named and once
                        // for an unnamed texture projection.
                        AiNodeDeclare(shapeNode, "Texture_Projection_wrap", "constant ARRAY BOOL");
                        AiNodeDeclare(shapeNode, "_wrap", "constant ARRAY BOOL");
                        AiNodeSetArray(shapeNode, "Texture_Projection_wrap", uvOptions);
                        AiNodeSetArray(shapeNode, "_wrap", uvOptions);
                     }
                  }
               }
            }

            // check if we have a bindpose in the alembic file
            if ( typedObject.getSchema().getPropertyHeader( ".bindpose" ) != NULL )
            {
               Alembic::Abc::IV3fArrayProperty prop = Alembic::Abc::IV3fArrayProperty( typedObject.getSchema(), ".bindpose" );
               if(prop.valid())
               {
                  Alembic::Abc::V3fArraySamplePtr abcBindPose = prop.getValue(sampleInfo.floorIndex);
                  AiNodeDeclare(shapeNode, "Pref", "varying POINT");

                  AtArray * bindPose = AiArrayAllocate((AtInt)abcBindPose->size(), 1, AI_TYPE_POINT);
                  AtPoint pnt;
                  for(AtULong i=0;i<abcBindPose->size();i++)
                  {
                     pnt.x = abcBindPose->get()[i].x;
                     pnt.y = abcBindPose->get()[i].y;
                     pnt.z = abcBindPose->get()[i].z;
                     AiArraySetPnt(bindPose,i,pnt);
                  }
                  AiNodeSetArray(shapeNode, "Pref", bindPose);
               }
            }
         }

         // access the positions
         Alembic::Abc::P3fArraySamplePtr abcPos = sample.getPositions();
         Alembic::Abc::V3fArraySamplePtr abcVel = sample.getVelocities();
         if(pos == NULL)
         {
            firstSampleCount = sample.getFaceIndices()->size();
            pos = AiArrayAllocate((AtInt)(abcPos->size() * 3),(AtInt)minNumSamples,AI_TYPE_FLOAT);
         }

         if(dynamicTopology)
         {
            SampleInfo sampleInfoFirst = getSampleInfo(
               samples[0],
               typedObject.getSchema().getTimeSampling(),
               typedObject.getSchema().getNumSamples()
            );
            typedObject.getSchema().get(sample,sampleInfoFirst.floorIndex);
            abcPos = sample.getPositions();

            sampleInfoFirst.alpha += double(sampleInfo.floorIndex) - double(sampleInfoFirst.floorIndex);
            sampleInfo = sampleInfoFirst;
         }

         // access the normals
         Alembic::Abc::N3fArraySamplePtr abcNor = normalParam.getExpandedValue(sampleInfo.floorIndex).getVals();
         if(nor == NULL)
            nor = AiArrayAllocate((AtInt)(abcNor->size()),(AtInt)minNumSamples,AI_TYPE_VECTOR);

         // if we have to interpolate
         if(sampleInfo.alpha <= sampleTolerance)
         {
            for(size_t i=0;i<abcPos->size();i++)
            {
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
            }
            if(abcNor != NULL)
            {
               for(size_t i=0;i<abcNor->size();i++)
               {
                  AtVector n;
                  n.x = abcNor->get()[i].x;
                  n.y = abcNor->get()[i].y;
                  n.z = abcNor->get()[i].z;
                  AiArraySetVec(nor,norOffset++,n);
               }
            }
         }
         else
         {
            Alembic::AbcGeom::IPolyMeshSchema::Sample sample2;
            typedObject.getSchema().get(sample2,sampleInfo.ceilIndex);
            Alembic::Abc::P3fArraySamplePtr abcPos2 = sample2.getPositions();
            float alpha = (float)sampleInfo.alpha;
            float ialpha = 1.0f - alpha;
            if(!dynamicTopology)
            {
               for(size_t i=0;i<abcPos->size();i++)
               {
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x * ialpha + abcPos2->get()[i].x * alpha);
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y * ialpha + abcPos2->get()[i].y * alpha);
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z * ialpha + abcPos2->get()[i].z * alpha);
               }
            }
            else if(abcVel)
            {
               if(abcVel->size() == abcPos->size())
               {
                  for(size_t i=0;i<abcPos->size();i++)
                  {
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x + alpha * abcVel->get()[i].x);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y + alpha * abcVel->get()[i].y);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z + alpha * abcVel->get()[i].z);
                  }
               }
               else
               {
                  for(size_t i=0;i<abcPos->size();i++)
                  {
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
                  }
               }
            }
            else
            {
               for(size_t i=0;i<abcPos->size();i++)
               {
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
               }
            }
            if(abcNor != NULL)
            {
               Alembic::Abc::N3fArraySamplePtr abcNor2 = normalParam.getExpandedValue(sampleInfo.ceilIndex).getVals();
               if(!dynamicTopology)
               {
                  for(size_t i=0;i<abcNor->size();i++)
                  {
                     AtVector n;
                     n.x = abcNor->get()[i].x * ialpha + abcNor2->get()[i].x * alpha;
                     n.y = abcNor->get()[i].y * ialpha + abcNor2->get()[i].y * alpha;
                     n.z = abcNor->get()[i].z * ialpha + abcNor2->get()[i].z * alpha;
                     AiArraySetVec(nor,norOffset++,n);
                  }
               }
               else
               {
                  for(size_t i=0;i<abcNor->size();i++)
                  {
                     AtVector n;
                     n.x = abcNor->get()[i].x;
                     n.y = abcNor->get()[i].y;
                     n.z = abcNor->get()[i].z;
                     AiArraySetVec(nor,norOffset++,n);
                  }
               }
            }
         }
      }
      AiNodeSetArray(shapeNode, "vlist", pos);
      AiNodeSetArray(shapeNode, "nlist", nor);
      if(!createdShifted)
      {
         AiNodeSetArray(shapeNode, "nidxs", uvsIdx);

         if(ud->gProcShaders != NULL && shaders == NULL)
         {
            //shaders = ud->gProcShaders;
            if(ud->gProcShaders->nelements > 1)
            {
               // check if we have facesets on this node
               std::vector<std::string> faceSetNames;
               typedObject.getSchema().getFaceSetNames(faceSetNames);
               if(faceSetNames.size() > 0)
               {
                  // allocate the shader index array
                  size_t shaderIndexCount = abcFaceCounts->size();
                  shaderIndices = AiArrayAllocate((AtUInt32)shaderIndexCount,1,AI_TYPE_BYTE);
                  for(size_t i=0;i<shaderIndexCount;i++)
                     AiArraySetByte(shaderIndices,(AtUInt32)i,0);

                  for(size_t i=0;i<faceSetNames.size();i++)
                  {
                     Alembic::AbcGeom::IFaceSetSchema faceSet = typedObject.getSchema().getFaceSet(faceSetNames[i]).getSchema();
                     Alembic::AbcGeom::IFaceSetSchema::Sample faceSetSample = faceSet.getValue();
                     Alembic::Abc::Int32ArraySamplePtr faces = faceSetSample.getFaces();
                     for(size_t j=0;j<faces->size();j++)
                     {
                        if((size_t)faces->get()[j] < shaderIndexCount && i+1 < ud->gProcShaders->nelements)
                        {
                           AiArraySetByte(shaderIndices,(AtUInt32)faces->get()[j],(AtByte)(i+1));
                        }
                     }
                  }
               }
            }
         }
      }

   } else if(Alembic::AbcGeom::ISubD::matches(md)) {

      ESS_LOG_INFO( "ExocortexAlembicArnoldDSO: GetNode: ISubD" );
	
      // cast to subd
      Alembic::AbcGeom::ISubD typedObject(object,Alembic::Abc::kWrapExisting);
      size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1 ? typedObject.getSchema().getNumSamples() : samples.size();

      // create the arnold node
      if(shifted)
      {
         bool doBreak = false;
         for(size_t j=0;j<ud->gInstances.size();j++)
         {
            for(size_t k=0;k<ud->gInstances[j].groupInfos.size();k++)
            {
               for(size_t l=0;k<ud->gInstances[j].groupInfos[k].identifiers.size();l++)
               {
                  if(ud->gInstances[j].groupInfos[k].identifiers[l] == object.getFullName())
                  {
                     std::map<float,AtNode*>::iterator it = ud->gInstances[j].groupInfos[k].nodes[l].find(ud->gCentroidTime);
                     if(it != ud->gInstances[j].groupInfos[k].nodes[l].end())
                     {
                        if(it->second != NULL)
                        {
                           shaders = AiNodeGetArray(it->second, "shader");
                           //shapeNode = AiNodeClone(it->second);
                        }
                     }
                     doBreak = true;
                     break;
                  }
               }
               if(doBreak) break;
            }
            if(doBreak) break;
         }
      }
      if(!shapeNode)
      {
         shapeNode = AiNode("polymesh");
         createdShifted = false;
      }

      // create arrays to hold the data
      AtArray * pos = NULL;

      // check if we have dynamic topology
      bool dynamicTopology = false;
      Alembic::Abc::IInt32ArrayProperty faceIndicesProp = Alembic::Abc::IInt32ArrayProperty(typedObject.getSchema(),".faceIndices");
      if(faceIndicesProp.valid())
         dynamicTopology = !faceIndicesProp.isConstant();

      // loop over all samples
      size_t firstSampleCount = 0;
      AtULong posOffset = 0;
      Alembic::Abc::Int32ArraySamplePtr abcFaceCounts;

      for(size_t sampleIndex = 0; sampleIndex < minNumSamples; sampleIndex++)
      {
         SampleInfo sampleInfo = getSampleInfo(
            samples[sampleIndex],
            typedObject.getSchema().getTimeSampling(),
            typedObject.getSchema().getNumSamples()
         );

         // get the floor sample
         Alembic::AbcGeom::ISubDSchema::Sample sample;
         typedObject.getSchema().get(sample,sampleInfo.floorIndex);

         // take care of the topology
         if(sampleIndex == 0 && !createdShifted)
         {
            abcFaceCounts = sample.getFaceCounts();
            Alembic::Abc::Int32ArraySamplePtr abcFaceIndices = sample.getFaceIndices();
            if(abcFaceCounts->get()[0] == 0)
               return NULL;
            AtArray * faceCounts = AiArrayAllocate((AtInt)abcFaceCounts->size(), 1, AI_TYPE_UINT);
            AtArray * faceIndices = AiArrayAllocate((AtInt)abcFaceIndices->size(), 1, AI_TYPE_UINT);
            AtArray * uvsIdx = AiArrayAllocate((AtInt)(abcFaceIndices->size()),1,AI_TYPE_UINT);
            AtUInt offset = 0;
            for(AtULong i=0;i<faceCounts->nelements;i++)
            {
               AiArraySetUInt(faceCounts,i,abcFaceCounts->get()[i]);
               for(AtLong j=0;j<abcFaceCounts->get()[i];j++)
               {
                  AiArraySetUInt(faceIndices,offset+j,abcFaceIndices->get()[offset+abcFaceCounts->get()[i]-(j+1)]);
                  AiArraySetUInt(uvsIdx,offset+j,offset+abcFaceCounts->get()[i]-(j+1));
               }
               offset += abcFaceCounts->get()[i];
            }
            AiNodeSetArray(shapeNode, "nsides", faceCounts);
            AiNodeSetArray(shapeNode, "vidxs", faceIndices);

            // subdiv settings
            AiNodeSetStr(shapeNode, "subdiv_type", "catclark");
            AiNodeSetInt(shapeNode, "subdiv_iterations", (AtInt)sample.getFaceVaryingInterpolateBoundary());
            AiNodeSetFlt(shapeNode, "subdiv_pixel_error", 0.0f);
            AiNodeSetBool(shapeNode, "smoothing", true);

            // check if we have UVs in the alembic file
            Alembic::AbcGeom::IV2fGeomParam uvParam= typedObject.getSchema().getUVsParam();
            if(uvParam.valid())
            {
               Alembic::Abc::V2fArraySamplePtr abcUvs = uvParam.getExpandedValue(sampleInfo.floorIndex).getVals();
               AtArray * uvs = AiArrayAllocate((AtInt)abcUvs->size() * 2, 1, AI_TYPE_FLOAT);
               AtArray * uvsIdx = AiArrayAllocate((AtInt)(abcUvs->size()),1,AI_TYPE_UINT);
               AtULong offset = 0;
               for(AtULong i=0;i<abcUvs->size();i++)
               {
                  AiArraySetFlt(uvs,offset++,abcUvs->get()[i].x);
                  AiArraySetFlt(uvs,offset++,abcUvs->get()[i].y);
               }
               AiNodeSetArray(shapeNode, "uvlist", uvs);
               AiNodeSetArray(shapeNode, "uvidxs", uvsIdx);

               // check if we have uvOptions
               if(typedObject.getSchema().getPropertyHeader( ".uvOptions" ) != NULL)
               {
                  Alembic::Abc::IFloatArrayProperty prop = Alembic::Abc::IFloatArrayProperty( typedObject.getSchema(), ".uvOptions" );
                  if(prop.getNumSamples() > 0)
                  {
                     Alembic::Abc::FloatArraySamplePtr ptr = prop.getValue(0);
                     if(ptr->size() > 1)
                     {
                        bool uWrap = ptr->get()[0] != 0.0f;
                        bool vWrap = ptr->get()[1] != 0.0f;

                        // fill the arnold array
                        AtArray * uvOptions = AiArrayAllocate(2,1,AI_TYPE_BOOLEAN);
                        AiArraySetBool(uvOptions,0,uWrap);
                        AiArraySetBool(uvOptions,1,vWrap);

                        // we need to define this two times, once for a named and once
                        // for an unnamed texture projection.
                        AiNodeDeclare(shapeNode, "Texture_Projection_wrap", "constant ARRAY BOOL");
                        AiNodeDeclare(shapeNode, "_wrap", "constant ARRAY BOOL");
                        AiNodeSetArray(shapeNode, "Texture_Projection_wrap", uvOptions);
                        AiNodeSetArray(shapeNode, "_wrap", uvOptions);
                     }
                  }
               }
            }
            else
            {
               // we don't need the uvindices
               AiArrayDestroy(uvsIdx);
            }
         }

         // access the positions
         Alembic::Abc::P3fArraySamplePtr abcPos = sample.getPositions();
         if(pos == NULL)
         {
            pos = AiArrayAllocate((AtInt)(abcPos->size() * 3),(AtInt)minNumSamples,AI_TYPE_FLOAT);
            firstSampleCount = sample.getFaceIndices()->size();
         }

         // if the count has changed, let's move back to the first sample
         if(dynamicTopology)
         {
            SampleInfo sampleInfoFirst = getSampleInfo(
               samples[0],
               typedObject.getSchema().getTimeSampling(),
               typedObject.getSchema().getNumSamples()
            );
            typedObject.getSchema().get(sample,sampleInfoFirst.floorIndex);
            abcPos = sample.getPositions();

            sampleInfoFirst.alpha += double(sampleInfo.floorIndex) - double(sampleInfoFirst.floorIndex);
            sampleInfo = sampleInfoFirst;
         }

         // if we have to interpolate
         if(sampleInfo.alpha <= sampleTolerance)
         {
            for(size_t i=0;i<abcPos->size();i++)
            {
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
            }
         }
         else
         {
            Alembic::AbcGeom::ISubDSchema::Sample sample2;
            typedObject.getSchema().get(sample2,sampleInfo.ceilIndex);
            Alembic::Abc::P3fArraySamplePtr abcPos2 = sample2.getPositions();
            float alpha = (float)sampleInfo.alpha;
            float ialpha = 1.0f - alpha;
            if(!dynamicTopology)
            {
               for(size_t i=0;i<abcPos->size();i++)
               {
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x * ialpha + abcPos2->get()[i].x * alpha);
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y * ialpha + abcPos2->get()[i].y * alpha);
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z * ialpha + abcPos2->get()[i].z * alpha);
               }
            }
            else if(typedObject.getSchema().getPropertyHeader( ".velocities" ) != NULL)
            {
               Alembic::Abc::IV3fArrayProperty velocitiesProp = Alembic::Abc::IV3fArrayProperty( typedObject.getSchema(), ".velocities" );
               SampleInfo velSampleInfo = getSampleInfo(
                  samples[sampleIndex],
                  velocitiesProp.getTimeSampling(),
                  velocitiesProp.getNumSamples()
               );

               Alembic::Abc::V3fArraySamplePtr abcVel = velocitiesProp.getValue(velSampleInfo.floorIndex);
               if(abcVel->size() == abcPos->size())
               {
                  for(size_t i=0;i<abcPos->size();i++)
                  {
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x + alpha * abcVel->get()[i].x);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y + alpha * abcVel->get()[i].y);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z + alpha * abcVel->get()[i].z);
                  }
               }
               else
               {
                  for(size_t i=0;i<abcPos->size();i++)
                  {
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
                  }
               }
            }
            else
            {
               for(size_t i=0;i<abcPos->size();i++)
               {
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
               }
            }
         }
      }
      AiNodeSetArray(shapeNode, "vlist", pos);

      if(ud->gProcShaders != NULL && !createdShifted && shaders == NULL)
      {
         shaders = ud->gProcShaders;
         if(shaders->nelements > 1)
         {
            // check if we have facesets on this node
            std::vector<std::string> faceSetNames;
            typedObject.getSchema().getFaceSetNames(faceSetNames);
            if(faceSetNames.size() > 0)
            {
               // allocate the shader index array
               size_t shaderIndexCount = abcFaceCounts->size();
               shaderIndices = AiArrayAllocate((AtUInt32)shaderIndexCount,1,AI_TYPE_BYTE);
               for(size_t i=0;i<shaderIndexCount;i++)
                  AiArraySetByte(shaderIndices,(AtUInt32)i,0);

               for(size_t i=0;i<faceSetNames.size();i++)
               {
                  Alembic::AbcGeom::IFaceSetSchema faceSet = typedObject.getSchema().getFaceSet(faceSetNames[i]).getSchema();
                  Alembic::AbcGeom::IFaceSetSchema::Sample faceSetSample = faceSet.getValue();
                  Alembic::Abc::Int32ArraySamplePtr faces = faceSetSample.getFaces();
                  for(size_t j=0;j<faces->size();j++)
                  {
                     if((size_t)faces->get()[j] < shaderIndexCount && i+1 < ud->gProcShaders->nelements)
                     {
                        AiArraySetByte(shaderIndices,(AtUInt32)faces->get()[j],(AtByte)(i+1));
                     }
                  }
               }
            }
         }
      }

   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      ESS_LOG_INFO( "ExocortexAlembicArnoldDSO: GetNode: ICurves" );

      // cast to curves
      Alembic::AbcGeom::ICurves typedObject(object,Alembic::Abc::kWrapExisting);
      size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1 ? typedObject.getSchema().getNumSamples() : samples.size();

      // create the arnold node
      if(shifted)
      {
         bool doBreak = false;
         for(size_t j=0;j<ud->gInstances.size();j++)
         {
            for(size_t k=0;k<ud->gInstances[j].groupInfos.size();k++)
            {
               for(size_t l=0;k<ud->gInstances[j].groupInfos[k].identifiers.size();l++)
               {
                  if(ud->gInstances[j].groupInfos[k].identifiers[l] == object.getFullName())
                  {
                     std::map<float,AtNode*>::iterator it = ud->gInstances[j].groupInfos[k].nodes[l].find(ud->gCentroidTime);
                     if(it != ud->gInstances[j].groupInfos[k].nodes[l].end())
                     {
                        if(it->second != NULL)
                        {
                           shaders = AiNodeGetArray(it->second, "shader");
                           //shapeNode = AiNodeClone(it->second);
                        }
                     }
                     doBreak = true;
                     break;
                  }
               }
               if(doBreak) break;
            }
            if(doBreak) break;
         }
      }
      if(!shapeNode)
      {
         shapeNode = AiNode("curves");
         createdShifted = false;
      }

      // create arrays to hold the data
      AtArray * pos = NULL;

      // loop over all samples
      AtULong posOffset = 0;
      size_t totalNumPoints =  0;
      size_t totalNumPositions = 0;
      for(size_t sampleIndex = 0; sampleIndex < minNumSamples; sampleIndex++)
      {
         SampleInfo sampleInfo = getSampleInfo(
            samples[sampleIndex],
            typedObject.getSchema().getTimeSampling(),
            typedObject.getSchema().getNumSamples()
         );

         // get the floor sample
         Alembic::AbcGeom::ICurvesSchema::Sample sample;
         typedObject.getSchema().get(sample,sampleInfo.floorIndex);

         // access the num points
         Alembic::Abc::Int32ArraySamplePtr abcNumPoints = sample.getCurvesNumVertices();

         // take care of the topology
         if(sampleIndex == 0 &&!createdShifted)
         {
            // hard coded pixel width, basis and mode
            AiNodeSetFlt(shapeNode, "min_pixel_width", 0.25f);
            AiNodeSetStr(shapeNode, "basis", "catmull-rom");
            AiNodeSetStr(shapeNode, "mode", ud->gCurvesMode.c_str());

            // setup the num_points
            AtArray * numPoints = AiArrayAllocate((AtInt)abcNumPoints->size(),1,AI_TYPE_UINT);
            for(size_t i=0;i<abcNumPoints->size();i++)
            {
               totalNumPoints += abcNumPoints->get()[i];
               totalNumPositions += abcNumPoints->get()[i] + 2;
               AiArraySetUInt(numPoints,(AtULong)i,(AtUInt)(abcNumPoints->get()[i]+2));
            }
            AiNodeSetArray(shapeNode,"num_points",numPoints);

            // check if we have a radius
            if( typedObject.getSchema().getPropertyHeader( ".radius" ) != NULL )
            {
               Alembic::Abc::IFloatArrayProperty prop = Alembic::Abc::IFloatArrayProperty( typedObject.getSchema(), ".radius" );
               Alembic::Abc::FloatArraySamplePtr abcRadius = prop.getValue(sampleInfo.floorIndex);

               AtArray * radius = AiArrayAllocate((AtInt)abcRadius->size(),1,AI_TYPE_FLOAT);
               for(size_t i=0;i<abcRadius->size();i++)
                  AiArraySetFlt(radius,(AtULong)i,abcRadius->get()[i]);
               AiNodeSetArray(shapeNode,"radius",radius);
            }

            // check if we have uvs
            Alembic::AbcGeom::IV2fGeomParam uvsParam = typedObject.getSchema().getUVsParam();
            if(uvsParam.valid())
            {
               Alembic::Abc::V2fArraySamplePtr abcUvs = uvsParam.getExpandedValue(sampleInfo.floorIndex).getVals();
               if(AiNodeDeclare(shapeNode, "Texture_Projection", "uniform POINT2"))
               {
                  AtArray* uvs = AiArrayAllocate((AtInt)abcUvs->size(), 1, AI_TYPE_POINT2);
                  AtPoint2 uv;
                  for(size_t i=0; i<abcUvs->size(); i++)
                  {
                     uv.x = abcUvs->get()[i].x;
                     uv.y = abcUvs->get()[i].y;
                     AiArraySetPnt2(uvs, (AtULong)i, uv);
                  }
                  AiNodeSetArray(shapeNode, "Texture_Projection", uvs);
               }
            }

            // check if we have colors
            if ( typedObject.getSchema().getPropertyHeader( ".color" ) != NULL )
            {
               Alembic::Abc::IC4fArrayProperty prop = Alembic::Abc::IC4fArrayProperty( typedObject.getSchema(), ".color" );
               Alembic::Abc::C4fArraySamplePtr abcColors = prop.getValue(sampleInfo.floorIndex);
               AtBoolean result = false;
               if(abcColors->size() == 1)
                  result = AiNodeDeclare(shapeNode, "Color", "constant RGBA");
               else if(abcColors->size() == abcNumPoints->size())
                  result = AiNodeDeclare(shapeNode, "Color", "uniform RGBA");
               else
                  result = AiNodeDeclare(shapeNode, "Color", "varying RGBA");
               if(result)
               {
                  AtArray * colors = AiArrayAllocate((AtInt)abcColors->size(),1,AI_TYPE_RGBA);
                  AtRGBA color;
                  for(size_t i=0; i<abcColors->size(); i++)
                  {
                     color.r = abcColors->get()[i].r;
                     color.g = abcColors->get()[i].g;
                     color.b = abcColors->get()[i].b;
                     color.a = abcColors->get()[i].a;
                     AiArraySetRGBA(colors, (AtULong)i, color);
                  }
                  AiNodeSetArray(shapeNode, "Color", colors);
               }
            }
         }

         // access the positions
         Alembic::Abc::P3fArraySamplePtr abcPos = sample.getPositions();
         if(pos == NULL)
            pos = AiArrayAllocate((AtInt)(totalNumPositions * 3),(AtInt)minNumSamples,AI_TYPE_FLOAT);

         // if we have to interpolate
         bool done = false;
         if(sampleInfo.alpha > sampleTolerance)
         {
            Alembic::AbcGeom::ICurvesSchema::Sample sample2;
            typedObject.getSchema().get(sample2,sampleInfo.ceilIndex);
            Alembic::Abc::P3fArraySamplePtr abcPos2 = sample2.getPositions();
            float alpha = (float)sampleInfo.alpha;
            float ialpha = 1.0f - alpha;
            size_t offset = 0;
            if(abcPos2->size() == abcPos->size())
            {
               for(size_t i=0;i<abcNumPoints->size();i++)
               {
                  // add the first and last point manually (catmull clark)
                  for(size_t j=0;j<abcNumPoints->get()[i];j++)
                  {
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].x * ialpha + abcPos2->get()[offset].x * alpha);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].y * ialpha + abcPos2->get()[offset].y * alpha);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].z * ialpha + abcPos2->get()[offset].z * alpha);
                     if(j==0 || j == abcNumPoints->get()[i]-1)
                     {
                        AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].x * ialpha + abcPos2->get()[offset].x * alpha);
                        AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].y * ialpha + abcPos2->get()[offset].y * alpha);
                        AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].z * ialpha + abcPos2->get()[offset].z * alpha);
                     }
                     offset++;
                  }
               }
               done = true;
            }
            else
            {
               Alembic::Abc::P3fArraySamplePtr abcVel = sample.getPositions();
               if(abcVel)
               {
                  if(abcVel->size() == abcPos->size())
                  {
                     for(size_t i=0;i<abcNumPoints->size();i++)
                     {
                        // add the first and last point manually (catmull clark)
                        for(size_t j=0;j<abcNumPoints->get()[i];j++)
                        {
                           AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].x + abcVel->get()[offset].x * alpha);
                           AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].y + abcVel->get()[offset].y * alpha);
                           AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].z + abcVel->get()[offset].z * alpha);
                           if(j==0 || j == abcNumPoints->get()[i]-1)
                           {
                              AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].x + abcVel->get()[offset].x * alpha);
                              AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].y + abcVel->get()[offset].y * alpha);
                              AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].z + abcVel->get()[offset].z * alpha);
                           }
                           offset++;
                        }
                     }
                     done = true;
                  }
               }
            }
         }
         if(!done)
         {
            size_t offset = 0;
            for(size_t i=0;i<abcNumPoints->size();i++)
            {
               // add the first and last point manually (catmull clark)
               for(size_t j=0;j<abcNumPoints->get()[i];j++)
               {
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].x);
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].y);
                  AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].z);
                  if(j==0 || j == abcNumPoints->get()[i]-1)
                  {
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].x);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].y);
                     AiArraySetFlt(pos,posOffset++,abcPos->get()[offset].z);
                  }
                  offset++;
               }
            }
         }
      }

      AiNodeSetArray(shapeNode, "points", pos);

   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      ESS_LOG_INFO( "ExocortexAlembicArnoldDSO: GetNode: INuPatch" );
	  AiMsgWarning("[ExocortexAlembicArnold] This object type is not YET implemented: '%s'.",md.get("schema").c_str());
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {

      ESS_LOG_INFO( "ExocortexAlembicArnoldDSO: GetNode: IPoints" );
	  
	  // cast to curves
      Alembic::AbcGeom::IPoints typedObject(object,Alembic::Abc::kWrapExisting);
      size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1 ? typedObject.getSchema().getNumSamples() : samples.size();

      // create the arnold node
      shapeNode = AiNode("points");

      // create arrays to hold the data
      AtArray * pos = NULL;

      shifted = false;

      // loop over all samples
      AtULong posOffset = 0;
      for(size_t sampleIndex = 0; sampleIndex < minNumSamples; sampleIndex++)
      {
         SampleInfo sampleInfo = getSampleInfo(
            samples[sampleIndex],
            typedObject.getSchema().getTimeSampling(),
            typedObject.getSchema().getNumSamples()
         );

         // get the floor sample
         Alembic::AbcGeom::IPointsSchema::Sample sample;
         typedObject.getSchema().get(sample,sampleInfo.floorIndex);

         // access the points
         Alembic::Abc::P3fArraySamplePtr abcPos = sample.getPositions();

         // take care of the topology
         if(sampleIndex == 0)
         {
            // hard coded mode
            if(!ud->gPointsMode.empty())
               AiNodeSetStr(shapeNode, "mode", ud->gPointsMode.c_str());

            // check if we have a radius
            Alembic::AbcGeom::IFloatGeomParam widthParam = typedObject.getSchema().getWidthsParam();
            if(widthParam.valid())
            {
               Alembic::Abc::FloatArraySamplePtr abcRadius = widthParam.getExpandedValue(sampleInfo.floorIndex).getVals();
               AtArray * radius = AiArrayAllocate((AtInt)abcRadius->size(),1,AI_TYPE_FLOAT);
               for(size_t i=0;i<abcRadius->size();i++)
                  AiArraySetFlt(radius,(AtULong)i,abcRadius->get()[i]);
               AiNodeSetArray(shapeNode,"radius",radius);
            }

            // check if we have colors
            if ( typedObject.getSchema().getPropertyHeader( ".color" ) != NULL )
            {
               Alembic::Abc::IC4fArrayProperty prop = Alembic::Abc::IC4fArrayProperty( typedObject.getSchema(), ".color" );
               Alembic::Abc::C4fArraySamplePtr abcColors = prop.getValue(sampleInfo.floorIndex);
               AtBoolean result = false;
               if(abcColors->size() == 1)
                  result = AiNodeDeclare(shapeNode, "Color", "constant RGBA");
               else
                  result = AiNodeDeclare(shapeNode, "Color", "uniform RGBA");
               if(result)
               {
                  AtArray * colors = AiArrayAllocate((AtInt)abcColors->size(),1,AI_TYPE_RGBA);
                  AtRGBA color;
                  for(size_t i=0; i<abcColors->size(); i++)
                  {
                     color.r = abcColors->get()[i].r;
                     color.g = abcColors->get()[i].g;
                     color.b = abcColors->get()[i].b;
                     color.a = abcColors->get()[i].a;
                     AiArraySetRGBA(colors, (AtULong)i, color);
                  }
                  AiNodeSetArray(shapeNode, "Color", colors);
               }
            }
         }

         // access the positions
         if(pos == NULL)
            pos = AiArrayAllocate((AtInt)(abcPos->size() * 3),(AtInt)minNumSamples,AI_TYPE_FLOAT);

         // if we have to interpolate
         if(sampleInfo.alpha <= sampleTolerance)
         {
            for(size_t i=0;i<abcPos->size();i++)
            {
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
            }
         }
         else
         {
            Alembic::Abc::V3fArraySamplePtr abcVel = sample.getVelocities();
            float alpha = (float)sampleInfo.alpha;
            for(size_t i=0;i<abcPos->size();i++)
            {
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x + alpha * abcVel->get()[i].x);
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y + alpha * abcVel->get()[i].y);
               AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z + alpha * abcVel->get()[i].z);
            }
         }
      }

      AiNodeSetArray(shapeNode, "points", pos);

   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      AiMsgWarning("[ExocortexAlembicArnold] Cameras are not supported.");
   } else {
      AiMsgError("[ExocortexAlembicArnold] This object type is not supported: '%s'.",md.get("schema").c_str());
   }

   // if we have a shape
   if(shapeNode != NULL)
   {
      ud->constructedNodes.push_back(shapeNode);
      if(shaders != NULL)
         ud->shadersToAssign.push_back(AiArrayCopy(shaders));
      else if(ud->gProcShaders != NULL)
         ud->shadersToAssign.push_back(AiArrayCopy(ud->gProcShaders));
      else
         ud->shadersToAssign.push_back(NULL);

      if(ud->gProcDispMap != NULL)
         AiNodeSetArray(shapeNode,"disp_map",AiArrayCopy(ud->gProcDispMap));
      if(shaderIndices != NULL)
         AiNodeSetArray(shapeNode, "shidxs", shaderIndices);


      // allocate the matrices for arnold and initiate them with identities
      AtArray * matrices = AiArrayAllocate(1,(AtInt)nbSamples,AI_TYPE_MATRIX);
      for(AtInt i=0;i<nbSamples;i++)
      {
         AtMatrix matrix;
         AiM4Identity(matrix);
         AiArraySetMtx(matrices,i,matrix);
      }

      // check if we have a parent that is a transform
      Alembic::Abc::IObject parent = object.getParent();

      // count the depth
      int depth = 0;
      while(Alembic::AbcGeom::IXform::matches(parent.getMetaData()))
      {
         depth++;
         parent = parent.getParent();
      }

      // loop until we hit the procedural's depth
      parent = object.getParent();
      while(Alembic::AbcGeom::IXform::matches(parent.getMetaData()) && depth > ud->proceduralDepth)
      {
         depth--;

         // cast to a xform
         Alembic::AbcGeom::IXform parentXform(parent,Alembic::Abc::kWrapExisting);
         if(parentXform.getSchema().getNumSamples() == 0)
            break;

         // loop over all samples
         for(size_t sampleIndex=0;sampleIndex<nbSamples;sampleIndex++)
         {
            SampleInfo sampleInfo = getSampleInfo(
               samples[sampleIndex],
               parentXform.getSchema().getTimeSampling(),
               parentXform.getSchema().getNumSamples()
            );

            // get the data and blend it if necessary
            Alembic::AbcGeom::XformSample sample;
            parentXform.getSchema().get(sample,sampleInfo.floorIndex);
            Alembic::Abc::M44d abcMatrix = sample.getMatrix();
            if(sampleInfo.alpha >= sampleTolerance)
            {
               parentXform.getSchema().get(sample,sampleInfo.ceilIndex);
               Alembic::Abc::M44d ceilAbcMatrix = sample.getMatrix();
               abcMatrix = (1.0 - sampleInfo.alpha) * abcMatrix + sampleInfo.alpha * ceilAbcMatrix;
            }

            // now convert to an arnold matrix
            AtMatrix parentMatrix, childMatrix, globalMatrix;
            size_t offset = 0;
            for(size_t row=0;row<4;row++)
               for(size_t col=0;col<4;col++,offset++)
                  parentMatrix[row][col] = (AtFloat)abcMatrix.getValue()[offset];

            // multiply the matrices since we want to know where we are in global space
            AiArrayGetMtx(matrices,(AtULong)sampleIndex,childMatrix);
            AiM4Mult(globalMatrix,childMatrix,parentMatrix);
            AiArraySetMtx(matrices,(AtULong)sampleIndex,globalMatrix);
         }

         // go upwards
         parent = parent.getParent();
      }

      AiNodeSetArray(shapeNode,"matrix",matrices);
   }

   // set the name of the node
   std::string nameStr = getNameFromIdentifier(object.getFullName(),ud->gIObjects[i].instanceID,ud->gIObjects[i].instanceGroupID)+ud->gIObjects[i].suffix;
   if(shifted)
      nameStr += ".time"+boost::lexical_cast<std::string>((int)(ud->gIObjects[i].centroidTime*1000.0f+0.5f));
   if(ud->gIObjects[i].hide)
      AiNodeSetInt(shapeNode, "visibility", 0);
   AiNodeSetStr(shapeNode,"name",nameStr.c_str());

   // set the pointer inside the map
   ud->gIObjects[i].node = shapeNode;

   // now update the instance maps
   for(size_t j=0;j<ud->gInstances.size();j++)
   {
      for(size_t k=0;k<ud->gInstances[j].groupInfos.size();k++)
      {
         for(size_t l=0;l<ud->gInstances[j].groupInfos[k].identifiers.size();l++)
         {
            if(ud->gInstances[j].groupInfos[k].identifiers[l] == object.getFullName())
            {
               std::map<float,AtNode*>::iterator it = ud->gInstances[j].groupInfos[k].nodes[l].find(ud->gIObjects[i].centroidTime);
               if(it != ud->gInstances[j].groupInfos[k].nodes[l].end())
                  it->second = shapeNode;
               break;
            }
         }
      }
   }

   return shapeNode;
}

// DSO hook
#ifdef __cplusplus
extern "C" {
#endif

AI_EXPORT_LIB int ProcLoader(AtProcVtable *vtable) 
{
   vtable->Init     = Init;
   vtable->Cleanup  = Cleanup;
   vtable->NumNodes = NumNodes;
   vtable->GetNode  = GetNode;
   
   sprintf(vtable->version, AI_VERSION);

   return 1;
}

#ifdef __cplusplus
}
#endif
