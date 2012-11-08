#include "CommonAlembic.h"
#include "CommonUtilities.h"
#include "CommonLicensing.h"
#include "CommonLog.h"
#include "CommonProfiler.h"
#include "CommonAbcCache.h"

#ifdef ESS_PROFILING
stats_map default_stats_policy::stats;
#endif // ESS_PROFILER


struct AlembicArchiveInfo
{
   Alembic::Abc::IArchive * archive;
   int refCount;

   AlembicArchiveInfo()
   {
      archive = NULL;
      refCount = 0;
   }

   AbcArchiveCache archiveCache;
};

void replaceString(std::string& str, const std::string& oldStr, const std::string& newStr)
{
  size_t pos = 0;
  while((pos = str.find(oldStr, pos)) != std::string::npos)
  {
     str.replace(pos, oldStr.length(), newStr);
     pos += newStr.length();
  }
}
#define _EC_QUOTE( x ) #x
#define EC_QUOTE( x ) _EC_QUOTE( x )


std::string getExporterName( std::string shortName ) {
	std::stringstream exporterName;
	exporterName << "Exocortex Crate for " << shortName << "," << EC_QUOTE(crate_ver) << "," << EC_QUOTE(alembic_ver) << "," << EC_QUOTE(hdf5_ver);
	return exporterName.str();
}

std::string getExporterFileName( std::string fileName ) {
	std::string sourceName = "Exported from: ";
	sourceName += fileName;

	// these symbols can't be in the meta data
	replaceString( sourceName, "=", "_" );
	replaceString( sourceName, ";", "_" );
	return sourceName;
}

bool parseTrailingNumber( std::string const& text, std::string const& requiredPrefix, int& number ) {
	number = 0;
	size_t prefixLength = requiredPrefix.size();
	if( text.size() <= prefixLength ) {
		return false;
	}
	std::string prefixText = text.substr( 0, prefixLength );
	if( prefixText.compare( requiredPrefix ) != 0 ) {
		return false;
	}
	std::string numberText = text.substr( prefixLength );

	number = atoi( numberText.c_str() );
	return true;
}

std::map<std::string,AlembicArchiveInfo> gArchives;

std::string resolvePath( std::string originalPath ) {
   ESS_PROFILE_SCOPE("resolvePath");
/*   static std::map<std::string,std::string> s_originalToResolvedPath;

   if( s_originalToResolvedPath.find( originalPath ) != s_originalToResolvedPath.end() ) {
	   return s_originalToResolvedPath[ originalPath ];
   }*/

   std::string resolvedPath = resolvePath_Internal( originalPath );

   //s_originalToResolvedPath.insert( std::pair<std::string,std::string>( originalPath, resolvedPath ) );

   return resolvedPath;
}

Alembic::Abc::IArchive * getArchiveFromID(std::string path)
{
	ESS_PROFILE_SCOPE("getArchiveFromID-1");
	std::map<std::string,AlembicArchiveInfo>::iterator it;
	std::string resolvedPath = resolvePath(path);
	it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
   {
     // check if the file exists
     if( ! HasAlembicReaderLicense() )
     {
         if(gArchives.size() == 1)
         {
            ESS_LOG_ERROR("[ExocortexAlembic] Reader license not found: Only one open archive at a time allowed!");
            return NULL;
         }
      }
   
		if( ! boost::filesystem::exists( resolvedPath.c_str() ) ) {
			ESS_LOG_ERROR( "Can't find Alembic file.  Path: " << path << "  Resolved path: " << resolvedPath );
			return  NULL;
		}

	  FILE * file = fopen(resolvedPath.c_str(),"rb");
    if(file == NULL) {
        return NULL;
      }
   else {
         fclose(file);
         addArchive(new Alembic::Abc::IArchive( Alembic::AbcCoreHDF5::ReadArchive(), resolvedPath));
         return gArchives.find(resolvedPath)->second.archive;
      }
   }
   return it->second.archive;
}

bool archiveExists(std::string path)
{
    ESS_PROFILE_SCOPE("archiveExists");
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   return it != gArchives.end();
}

AbcArchiveCache* getArchiveCache( std::string path ) {
	ESS_PROFILE_SCOPE("getArchiveCache");
	std::string resolvedPath = resolvePath(path);
    std::map<std::string,AlembicArchiveInfo>::iterator it;
    it = gArchives.find(resolvedPath);
    if(it == gArchives.end())
      return NULL;

	// compute cache if required.
	if( it->second.archiveCache.size() == 0 ) {
		createAbcArchiveCache( it->second.archive, &(it->second.archiveCache) );
	}
	return &(it->second.archiveCache);
};

std::string addArchive(Alembic::Abc::IArchive * archive)
{
    ESS_PROFILE_SCOPE("addArchive");
   AlembicArchiveInfo info;
   info.archive = archive;
   gArchives.insert(std::pair<std::string,AlembicArchiveInfo>(archive->getName(),info));
   return archive->getName().c_str();
}

void deleteArchive(std::string path)
{
   ESS_PROFILE_SCOPE("deleteArchive");
  std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return;
   it->second.archive->reset();
   delete(it->second.archive);
   gArchives.erase(it);
}

void deleteAllArchives()
{
   ESS_PROFILE_SCOPE("deleteAllArchives");
 std::map<std::string,AlembicArchiveInfo>::iterator it;
   for(it = gArchives.begin(); it != gArchives.end(); it++)
   {
      it->second.archive->reset();
      delete(it->second.archive);
   }
   gArchives.clear();
}


AbcObjectCache* getObjectCacheFromArchive(std::string path, std::string identifier)
{
	AbcArchiveCache* abcArchiveCache = getArchiveCache( resolvePath( path ) );
	if( abcArchiveCache == NULL ) {
		return NULL;
	}
	AbcArchiveCache::iterator it = abcArchiveCache->find( identifier );
	if( it == abcArchiveCache->end() ) {
		return NULL;
	}
	return &(it->second);
}

Abc::IObject getObjectFromArchive(std::string path, std::string identifier)
{
   ESS_PROFILE_SCOPE("getObjectFromArchive");
	AbcObjectCache* objectCache = getObjectCacheFromArchive(path,identifier );
	if( objectCache == NULL ) {
		return Alembic::Abc::IObject();
	}
	return objectCache->obj;
}
int addRefArchive(std::string path)
{
   ESS_PROFILE_SCOPE("addRefArchive");
   if(path.empty())
      return -1;
   std::string resolvedPath = resolvePath(path);

   // call get archive to ensure to create it!
   getArchiveFromID(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return -1;
   it->second.refCount++;
   return it->second.refCount;
}

int delRefArchive(std::string path)
{
   ESS_PROFILE_SCOPE("delRefArchive");
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return -1;
   it->second.refCount--;
   if(it->second.refCount==0)
   {
      deleteArchive(resolvedPath);
      return 0;
   }
   return it->second.refCount;
}

int getRefArchive(std::string path)
{
   ESS_PROFILE_SCOPE("getRefArchive");
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return -1;
   return it->second.refCount;
}

bool validate_filename_location(const char *filename)
{
   std::ofstream fout(filename);
   if (!fout.is_open())
      return false;
   fout.close();
   return true;
}

SampleInfo getSampleInfo
(
   double iFrame,
   Alembic::AbcCoreAbstract::TimeSamplingPtr iTime,
   size_t numSamps
)
{
   ESS_PROFILE_SCOPE("getSampleInfo");
   SampleInfo result;
   if (numSamps == 0)
      numSamps = 1;

   std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndex =
   iTime->getFloorIndex(iFrame, numSamps);

   result.floorIndex = floorIndex.first;
   result.ceilIndex = result.floorIndex;

   // check if we have a full license
   if(!HasAlembicReaderLicense())
   {
      if(result.floorIndex > 75)
      {
         EC_LOG_ERROR("[ExocortexAlembic] Demo Mode: Cannot open sample indices higher than 75.");
         result.floorIndex = 75;
         result.ceilIndex = 75;
         result.alpha = 0.0;
         return result;
      }
   }

   if (fabs(iFrame - floorIndex.second) < 0.0001) {
      result.alpha = 0.0f;
      return result;
   }

   std::pair<Alembic::AbcCoreAbstract::index_t, double> ceilIndex =
   iTime->getCeilIndex(iFrame, numSamps);

   if (fabs(iFrame - ceilIndex.second) < 0.0001) {
      result.floorIndex = ceilIndex.first;
      result.ceilIndex = result.floorIndex;
      result.alpha = 0.0f;
      return result;
   }

   if (result.floorIndex == ceilIndex.first) {
      result.alpha = 0.0f;
      return result;
   }

   result.ceilIndex = ceilIndex.first;

   result.alpha = (iFrame - floorIndex.second) / (ceilIndex.second - floorIndex.second);
   return result;
}


Imath::M33d extractRotation(Imath::M44d& m)
{
	double values[3][3];

	for(int i=0; i<3; i++){
		for(int j=0; j<3; j++){
			values[i][j] = m[i][j];
		}
	}
	
	return Imath::M33d(values);
}

std::string getModelName( const std::string &identifier )
{
    // Max Scene nodes are also identified by their transform nodes since an INode contains
    // both the transform and the shape.  So if we find an "xfo" at the end of the identifier
    // then we extract the model name from the identifier
    std::string modelName;
    size_t pos = identifier.rfind("Xfo", identifier.length()-3, 3);
    if (pos == identifier.npos)
        modelName = identifier;
    else
        modelName = identifier.substr(0, identifier.length()-3);

    return modelName;
}

std::string removeXfoSuffix(const std::string& importName)
{
	size_t found = importName.find("Xfo");
	if(found == std::string::npos){
		found = importName.find("xfo");
	}
	if(found != std::string::npos){
		return importName.substr(0, found);
	}
	return importName;
}

Alembic::Abc::ICompoundProperty getCompoundFromObject(Alembic::Abc::IObject &object)
{
	ESS_PROFILE_SCOPE("getCompoundFromObject"); 
    const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::IFaceSet::matches(md)) {
      return Alembic::AbcGeom::IFaceSet(object,Alembic::Abc::kWrapExisting).getSchema();
   }
   return Alembic::Abc::ICompoundProperty();
}


Alembic::Abc::TimeSamplingPtr getTimeSamplingFromObject(Alembic::Abc::IObject &object)
{
	ESS_PROFILE_SCOPE("getTimeSamplingFromObject"); 
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::IFaceSet::matches(md)) {
      return Alembic::AbcGeom::IFaceSet(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   }
   return Alembic::Abc::TimeSamplingPtr();
}

Alembic::Abc::TimeSamplingPtr getTimeSamplingFromObject(Alembic::Abc::OObject *object)
{
	ESS_PROFILE_SCOPE("getTimeSamplingFromObject"); 
   const Alembic::Abc::MetaData &md = object->getMetaData();
   if(Alembic::AbcGeom::OXform::matches(md)) {
      return ((Alembic::AbcGeom::OXform*)(&object))->getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::OPolyMesh::matches(md)) {
      return ((Alembic::AbcGeom::OPolyMesh*)(&object))->getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::OCurves::matches(md)) {
      return ((Alembic::AbcGeom::OCurves*)(&object))->getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ONuPatch::matches(md)) {
      return ((Alembic::AbcGeom::ONuPatch*)(&object))->getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::OPoints::matches(md)) {
      return ((Alembic::AbcGeom::OPoints*)(&object))->getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::OSubD::matches(md)) {
      return ((Alembic::AbcGeom::OSubD*)(&object))->getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::OCamera::matches(md)) {
      return ((Alembic::AbcGeom::OCamera*)(&object))->getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::OFaceSet::matches(md)) {
      return ((Alembic::AbcGeom::OFaceSet*)(&object))->getSchema().getTimeSampling();
   }
   return Alembic::Abc::TimeSamplingPtr();
}

size_t getNumSamplesFromObject(Alembic::Abc::IObject &object)
{
	ESS_PROFILE_SCOPE("getNumSamplesFromObject"); 
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::IFaceSet::matches(md)) {
      return Alembic::AbcGeom::IFaceSet(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   }
   return 0;
}

bool isObjectSchemaConstant( AbcG::IPolyMeshSchema& schema ) {
	return schema.isConstant();
}
bool isObjectSchemaConstant( AbcG::IXformSchema& schema ) {
	return schema.isConstant();
}
bool isObjectSchemaConstant( AbcG::ICurvesSchema& schema ) {
	return schema.isConstant();
}
bool isObjectSchemaConstant( AbcG::INuPatchSchema& schema ) {
	return schema.isConstant();
}
bool isObjectSchemaConstant( AbcG::IPointsSchema& schema ) {
	return schema.isConstant();
}
bool isObjectSchemaConstant( AbcG::ISubDSchema& schema ) {
	return schema.isConstant();
}
bool isObjectSchemaConstant( AbcG::ICameraSchema& schema ) {
	return schema.isConstant();
}
bool isObjectSchemaConstant( AbcG::IFaceSetSchema& schema ) {
	return schema.isConstant();
}


bool isObjectConstant(Alembic::Abc::IObject &object ) {
   ESS_PROFILE_SCOPE("isObjectConstant");  
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
	   return isObjectSchemaConstant( Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema() );
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return isObjectSchemaConstant( Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema() );
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return isObjectSchemaConstant( Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema() );
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return isObjectSchemaConstant( Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema() );
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return isObjectSchemaConstant( Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema() );
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return isObjectSchemaConstant( Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema() );
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return isObjectSchemaConstant( Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema() );
   } else if(Alembic::AbcGeom::IFaceSet::matches(md)) {
      return isObjectSchemaConstant( Alembic::AbcGeom::IFaceSet(object,Alembic::Abc::kWrapExisting).getSchema() );
   }
   return true;
}

size_t getNumSamplesFromObject(Alembic::Abc::OObject *object)
{
	ESS_PROFILE_SCOPE("getNumSamplesFromObject"); 
   const Alembic::Abc::MetaData &md = object->getMetaData();
   if(Alembic::AbcGeom::OXform::matches(md)) {
      return  ((Alembic::AbcGeom::OXform*)(&object))->getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::OPolyMesh::matches(md)) {
      return ((Alembic::AbcGeom::OPolyMesh*)(&object))->getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::OCurves::matches(md)) {
      return ((Alembic::AbcGeom::OCurves*)(&object))->getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::ONuPatch::matches(md)) {
      return ((Alembic::AbcGeom::ONuPatch*)(&object))->getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::OPoints::matches(md)) {
      return ((Alembic::AbcGeom::OPoints*)(&object))->getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::OSubD::matches(md)) {
      return ((Alembic::AbcGeom::OSubD*)(&object))->getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::OCamera::matches(md)) {
      return ((Alembic::AbcGeom::OCamera*)(&object))->getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::OFaceSet::matches(md)) {
	   return ((Alembic::AbcGeom::OFaceSet*)(&object))->getSchema().getNumSamples();
   }
   return 0;
}

float getTimeOffsetFromObject( Alembic::Abc::IObject &object, SampleInfo const& sampleInfo ) {
	Alembic::Abc::TimeSamplingPtr timeSampling = getTimeSamplingFromObject( object );
	if( timeSampling.get() == NULL ) {
		return 0;
	}
	else {
		return (float)( ( timeSampling->getSampleTime(sampleInfo.ceilIndex) -
			timeSampling->getSampleTime(sampleInfo.floorIndex) ) * sampleInfo.alpha );
	}
}

void getMergeInfo( AbcArchiveCache *pArchiveCache, AbcObjectCache *pObjectCache, bool& bCreateNullNode, int& nMergedGeomNodeIndex, AbcObjectCache **ppMergedChildObjectCache)
{
   NodeCategory::type cat = NodeCategory::get(pObjectCache->obj);
   if(cat == NodeCategory::XFORM)
	{	//if a transform node, decide whether or not use a dummy node OR merge this dummy node with geometry node child

		unsigned geomNodeCount = 0;
      int mergeIndex;
      for(int j=0; j<(int)pObjectCache->childIdentifiers.size(); j++)
		{
      AbcObjectCache *pChildObjectCache = &( pArchiveCache->find( pObjectCache->childIdentifiers[j] )->second );
         Alembic::AbcGeom::IObject childObj = pChildObjectCache->obj;
         if( NodeCategory::get( childObj ) == NodeCategory::GEOMETRY ){
				(*ppMergedChildObjectCache) = pChildObjectCache;
				mergeIndex = j;
				geomNodeCount++;
			}
		} 

		if(geomNodeCount == 0 ){//create dummy node
			bCreateNullNode = true;
		}
		else if(geomNodeCount == 1){ //create geometry node
         nMergedGeomNodeIndex = mergeIndex;
		}
		else if(geomNodeCount > 1){ //create dummy node
			bCreateNullNode = true;
		}
	}
}

//int getNumberOfNodesToBeImported(Alembic::AbcGeom::IObject root)
//{
//   std::list<Alembic::Abc::IObject> sceneStack;
//
//	for(size_t j=0; j<root.getNumChildren(); j++){
//      sceneStack.push_back(root.getChild(j));
//	} 
//
//   int nNumNodes = 0;
//   while( !sceneStack.empty() )
//   {
//      Alembic::Abc::IObject iObj = sceneStack.back();
//      sceneStack.pop_back();
//      
//      nNumNodes++;
//
//      bool bCreateNullNode = false;
//      int nMergedGeomNodeIndex = -1;
//		Alembic::AbcGeom::IObject mergedGeomChild;
//      getMergeInfo(iObj, bCreateNullNode, nMergedGeomNodeIndex, mergedGeomChild);
//      
//      //push the children as the last step, since we need to who the parent is first (we may have merged)
//      for(size_t j=0; j<iObj.getNumChildren(); j++)
//      {
//         NodeCategory::type childCat = NodeCategory::get(iObj.getChild(j));
//         if( childCat == NodeCategory::UNSUPPORTED ) continue;// skip over unsupported types
//
//         //I assume that geometry nodes are always leaf nodes. Thus, if we merged a geometry node will its parent transform, we don't
//         //need to push that geometry node to the stack.
//         //A geometry node can't be combined with its transform node, the transform node has other tranform nodes as children. These
//         //nodes must be pushed.
//         if( nMergedGeomNodeIndex != j ){
//            sceneStack.push_back(iObj.getChild(j));
//         }
//      }  
//   }
//
//   return nNumNodes;
//}


struct AlembicSelectionStackElement
{
   AbcObjectCache *pObjectCache;
   bool bSelected;

   AlembicSelectionStackElement(AbcObjectCache *pMyObjectCache, bool selected):pObjectCache(pMyObjectCache), bSelected(selected)
   {}
};


//returns the number of nodes
int prescanAlembicHierarchy(AbcArchiveCache *pArchiveCache, AbcObjectCache *pRootObjectCache, std::vector<std::string>& nodes, std::map<std::string, bool>& map, bool bIncludeChildren)
{
   for(int i=0; i<nodes.size(); i++){
      boost::to_upper(nodes[i]);
   }

   std::list<AlembicSelectionStackElement> sceneStack;

  for(size_t j=0; j<pRootObjectCache->childIdentifiers.size(); j++){
      AbcObjectCache *pChildObjectCache = &( pArchiveCache->find( pRootObjectCache->childIdentifiers[j] )->second );
      sceneStack.push_back(AlembicSelectionStackElement(pChildObjectCache, false));
	} 

   int nNumNodes = 0;
   while( !sceneStack.empty() )
   {
      AlembicSelectionStackElement sElement = sceneStack.back();
      Alembic::Abc::IObject iObj = sElement.pObjectCache->obj;
      bool bSelected = sElement.bSelected;
      sceneStack.pop_back();

      nNumNodes++;

      bool bCreateNullNode = false;
      int nMergedGeomNodeIndex = -1;
    AbcObjectCache *pMergedChildObjectCache = NULL;
      getMergeInfo(pArchiveCache, sElement.pObjectCache, bCreateNullNode, nMergedGeomNodeIndex, &pMergedChildObjectCache);
      
      std::string name;
      std::string fullname;

	   if(nMergedGeomNodeIndex != -1){//we are merging
    		Alembic::AbcGeom::IObject mergedGeomChild = pMergedChildObjectCache->obj;
         name = mergedGeomChild.getName();
         fullname = mergedGeomChild.getFullName();
	   }
	   else{ //geometry node(s) under a dummy node (in pParentMaxNode)
         name = iObj.getName();
         fullname = iObj.getFullName();
      }

     

      bool bSelectChildren = false;

      if(bSelected){
         map[fullname] = true;
         bSelectChildren = true;
      }
      else{
          boost::to_upper(name);
         for(int i=0; i<nodes.size(); i++){
            
            const char* cstrName = name.c_str();
            const char* cstrNode = nodes[i].c_str(); 

            if( name.find( nodes[i] ) != std::string::npos ){
               if(bIncludeChildren){
                  bSelectChildren = true;              
               }

               std::vector<std::string> parts;
		         boost::split(parts, fullname, boost::is_any_of("/"));

               std::string nodeName;
               for(int j=1; j<parts.size(); j++){
                  nodeName += "/";
                  nodeName += parts[j];
                  map[nodeName] = true;
               }
            }
         }
      }

      //push the children as the last step, since we need to who the parent is first (we may have merged)
      for(size_t j=0; j<sElement.pObjectCache->childIdentifiers.size(); j++)
      {
         AbcObjectCache *pChildObjectCache = &( pArchiveCache->find( sElement.pObjectCache->childIdentifiers[j] )->second );
        Alembic::AbcGeom::IObject childObj = pChildObjectCache->obj;
         NodeCategory::type childCat = NodeCategory::get(childObj);
         if( childCat == NodeCategory::UNSUPPORTED ) continue;// skip over unsupported types

         //I assume that geometry nodes are always leaf nodes. Thus, if we merged a geometry node will its parent transform, we don't
         //need to push that geometry node to the stack.
         //A geometry node can't be combined with its transform node, the transform node has other tranform nodes as children. These
         //nodes must be pushed.
         if( nMergedGeomNodeIndex != j ){
            sceneStack.push_back(AlembicSelectionStackElement(pChildObjectCache, bSelectChildren));
         }
      }  
   }
   
   return nNumNodes;
}

