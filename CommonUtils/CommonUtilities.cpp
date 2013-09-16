#include "CommonAlembic.h"
#include "CommonUtilities.h"
#include "CommonLicensing.h"
#include "CommonAbcCache.h"
#include "CommonRegex.h"

#ifdef ESS_PROFILING
stats_map default_stats_policy::stats;
#endif // ESS_PROFILER

#include "CommonPBar.h"

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


std::string getExporterName( std::string const& shortName ) {
	std::stringstream exporterName;

  std::string shortNameSafe = shortName;
  
  replaceString( shortNameSafe, "\"", "" );
	
	exporterName << "Exocortex Crate for " << shortNameSafe << "," << EC_QUOTE(crate_ver) << "," << EC_QUOTE(alembic_ver) << "," << EC_QUOTE(hdf5_ver);
	return exporterName.str();
}

std::string getExporterFileName( std::string const& fileName ) {
	std::string sourceName = "Exported from: ";
	sourceName += fileName;

	// these symbols can't be in the meta data
	replaceString( sourceName, "=", "_" );
	replaceString( sourceName, ";", "_" );
  replaceString( sourceName, "\\", "/" );
	
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

std::string resolvePath( std::string const& originalPath ) {
   ESS_PROFILE_SCOPE("resolvePath");
/*   static std::map<std::string,std::string> s_originalToResolvedPath;

   if( s_originalToResolvedPath.find( originalPath ) != s_originalToResolvedPath.end() ) {
	   return s_originalToResolvedPath[ originalPath ];
   }*/

   std::string resolvedPath = resolvePath_Internal( EnvVariables::replace( originalPath ) );

   //s_originalToResolvedPath.insert( std::pair<std::string,std::string>( originalPath, resolvedPath ) );

   return resolvedPath;
}

Alembic::Abc::IArchive * getArchiveFromID(std::string const& path)
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
        if( HasAlembicInvalidLicense() ) {
            ESS_LOG_ERROR("[alembic] No license available and EXOCORTEX_ALEMBIC_NO_DEMO defined, no Alembic files opened." );
            return NULL;
         }

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

         AbcF::IFactory iFactory;
         AbcF::IFactory::CoreType oType;
         addArchive( new Abc::IArchive(iFactory.getArchive( resolvedPath, oType )) );

         //addArchive(new Abc::IArchive( Alembic::AbcCoreHDF5::ReadArchive(), resolvedPath));

         Abc::IArchive *pArchive = gArchives.find(resolvedPath)->second.archive;
         EC_LOG_INFO( "Opening Abc Archive: " << pArchive->getName() );
         return pArchive;
      }
   }
   return it->second.archive;
}

bool archiveExists(std::string const& path)
{
    ESS_PROFILE_SCOPE("archiveExists");
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   return it != gArchives.end();
}

AbcArchiveCache* getArchiveCache( std::string const& path, CommonProgressBar *pBar ) {
	ESS_PROFILE_SCOPE("getArchiveCache");
  getArchiveFromID(path);
	std::string resolvedPath = resolvePath(path);
    std::map<std::string,AlembicArchiveInfo>::iterator it;
    it = gArchives.find(resolvedPath);
    if(it == gArchives.end())
      return NULL;

	// compute cache if required.
	if( it->second.archiveCache.size() == 0 )
	{
		if (!createAbcArchiveCache( it->second.archive, &(it->second.archiveCache), pBar ))
		{
			it->second.archiveCache.clear();
			return 0;
		}
	}
	return &(it->second.archiveCache);
}

std::string addArchive(Alembic::Abc::IArchive * archive)
{
    ESS_PROFILE_SCOPE("addArchive");
   AlembicArchiveInfo info;
   info.archive = archive;
   gArchives.insert(std::pair<std::string,AlembicArchiveInfo>(archive->getName(),info));
   return archive->getName().c_str();
}

void deleteArchive(std::string const& path)
{
   ESS_PROFILE_SCOPE("deleteArchive");
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return;

   EC_LOG_INFO( "Closing Abc Archive: " << it->second.archive->getName() );
   it->second.archive->reset();
   delete(it->second.archive);
   gArchives.erase(it);
}

void deleteAllArchives()
{
   ESS_PROFILE_SCOPE("deleteAllArchives");
   for(std::map<std::string,AlembicArchiveInfo>::iterator it = gArchives.begin(); it != gArchives.end(); ++it)
   {
      it->second.archive->reset();
      delete(it->second.archive);
   }
   gArchives.clear();
}

AbcObjectCache* getObjectCacheFromArchive(std::string const& path, std::string const& identifier)
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

Abc::IObject getObjectFromArchive(std::string const& path, std::string const& identifier)
{
   ESS_PROFILE_SCOPE("getObjectFromArchive");
	AbcObjectCache* objectCache = getObjectCacheFromArchive(path,identifier );
	if( objectCache == NULL ) {
		return Alembic::Abc::IObject();
	}
	return objectCache->obj;
}
int addRefArchive(std::string const& path)
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
#ifdef _DEBUG
   EC_LOG_INFO("ref count (a): " << it->second.refCount);
#endif
   return it->second.refCount;
}

int decRefArchive(std::string const& path)
{
   ESS_PROFILE_SCOPE("decRefArchive");
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return -1;
   it->second.refCount--;
#ifdef _DEBUG
   EC_LOG_INFO("ref count (d): " << it->second.refCount);
#endif
   return it->second.refCount;
}

int delRefArchive(std::string const& path)
{
   ESS_PROFILE_SCOPE("delRefArchive");
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return -1;
   it->second.refCount--;
#ifdef _DEBUG
   EC_LOG_INFO("ref count (d): " << it->second.refCount);
#endif
   if(it->second.refCount==0)
   {
#ifdef _DEBUG
   EC_LOG_INFO("ref delete");
#endif
      deleteArchive(resolvedPath);
      return 0;
   }
   return it->second.refCount;
}

int getRefArchive(std::string const& path)
{
   ESS_PROFILE_SCOPE("getRefArchive");
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return -1;
   return it->second.refCount;
}

void getPaths(std::vector<std::string>& paths)
{
   for(std::map<std::string,AlembicArchiveInfo>::iterator it=gArchives.begin(); it != gArchives.end(); it++)
   {
      paths.push_back( it->first.c_str() );
   }
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

   std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndex = iTime->getFloorIndex(iFrame, numSamps);

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

template<typename T> inline bool isObjectSchemaConstant(T &schema)
{
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

template<typename T> bool __getBasicSchemaDataFromObject(BasicSchemaData::SCHEMA_TYPE type, T &schema, BasicSchemaData &bsd)
{
	bsd.type = type;
	bsd.isConstant = schema.isConstant();
	if (bsd.isConstant)
	{
		Alembic::Abc::IObject iObj = schema.getObject();
		AbcG::IVisibilityProperty visibilityProperty = AbcG::GetVisibilityProperty(iObj);
        if (visibilityProperty.valid()){
			bsd.isConstant = visibilityProperty.isConstant();
            //if(visibilityProperty.getNumSamples() > 0 ){
            //   ESS_LOG_WARNING("BAD Sample check happened!!!!");
            //}
        }
	}
	bsd.nbSamples  = schema.getNumSamples();
	return true;
}
bool getBasicSchemaDataFromObject(Alembic::Abc::IObject &object, BasicSchemaData &bsd)
{
	ESS_PROFILE_SCOPE("getBasicSchemaDataFromObject"); 
	const Alembic::Abc::MetaData &md = object.getMetaData();
	if(Alembic::AbcGeom::IXform::matches(md))
		return __getBasicSchemaDataFromObject(BasicSchemaData::__XFORM, Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema(), bsd);

	else if(Alembic::AbcGeom::IPolyMesh::matches(md))
		return __getBasicSchemaDataFromObject(BasicSchemaData::__POLYMESH, Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema(), bsd);

	else if(Alembic::AbcGeom::ICurves::matches(md))
		return __getBasicSchemaDataFromObject(BasicSchemaData::__CURVES, Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema(), bsd);

	else if(Alembic::AbcGeom::INuPatch::matches(md))
		return __getBasicSchemaDataFromObject(BasicSchemaData::__NUPATCH, Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema(), bsd);

	else if(Alembic::AbcGeom::IPoints::matches(md))
		return __getBasicSchemaDataFromObject(BasicSchemaData::__POINTS, Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema(), bsd);

	else if(Alembic::AbcGeom::ISubD::matches(md))
		return __getBasicSchemaDataFromObject(BasicSchemaData::__SUBDIV, Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema(), bsd);

	else if(Alembic::AbcGeom::ICamera::matches(md))
		return __getBasicSchemaDataFromObject(BasicSchemaData::__CAMERA, Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema(), bsd);

	else if(Alembic::AbcGeom::IFaceSet::matches(md))
		return __getBasicSchemaDataFromObject(BasicSchemaData::__FACESET, Alembic::AbcGeom::IFaceSet(object,Alembic::Abc::kWrapExisting).getSchema(), bsd);

	return false;
};

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


Abc::ICompoundProperty getArbGeomParams(const AbcG::IObject& iObj, AbcA::TimeSamplingPtr& timeSampling, int& nSamples)
{
	if(AbcG::IXform::matches(iObj.getMetaData())){
       AbcG::IXform obj(iObj, Abc::kWrapExisting);
	   timeSampling = obj.getSchema().getTimeSampling();
	   nSamples = (int) obj.getSchema().getNumSamples();
       return obj.getSchema().getArbGeomParams();
	} 	
	else if(AbcG::IPolyMesh::matches(iObj.getMetaData())){
	   AbcG::IPolyMesh obj(iObj, Abc::kWrapExisting);
	   timeSampling = obj.getSchema().getTimeSampling();
	   nSamples = (int) obj.getSchema().getNumSamples();
       return obj.getSchema().getArbGeomParams();
    }
    else if(AbcG::ISubD::matches(iObj.getMetaData())){
	   AbcG::ISubD obj(iObj, Abc::kWrapExisting);
	   timeSampling = obj.getSchema().getTimeSampling();
	   nSamples = (int) obj.getSchema().getNumSamples();
       return obj.getSchema().getArbGeomParams();
	}
	else if(AbcG::ICamera::matches(iObj.getMetaData())){
	   AbcG::ICamera obj(iObj, Abc::kWrapExisting);
	   timeSampling = obj.getSchema().getTimeSampling();
	   nSamples = (int) obj.getSchema().getNumSamples();
       return obj.getSchema().getArbGeomParams();
	}
	else if(AbcG::IPoints::matches(iObj.getMetaData())){
	   AbcG::IPoints obj(iObj, Abc::kWrapExisting);
	   timeSampling = obj.getSchema().getTimeSampling();
	   nSamples = (int) obj.getSchema().getNumSamples();
       return obj.getSchema().getArbGeomParams();
	}
	else if(AbcG::ICurves::matches(iObj.getMetaData())){
	   AbcG::ICurves obj(iObj, Abc::kWrapExisting);
	   timeSampling = obj.getSchema().getTimeSampling();
	   nSamples = (int) obj.getSchema().getNumSamples();
       return obj.getSchema().getArbGeomParams();
	}
	else if(AbcG::ILight::matches(iObj.getMetaData())){
	   AbcG::ILight obj(iObj, Abc::kWrapExisting);
	   timeSampling = obj.getSchema().getTimeSampling();
	   nSamples = (int) obj.getSchema().getNumSamples();
       return obj.getSchema().getArbGeomParams();
	}
    else if(AbcG::INuPatch::matches(iObj.getMetaData())){
	   AbcG::INuPatch obj(iObj, Abc::kWrapExisting);
	   timeSampling = obj.getSchema().getTimeSampling();
	   nSamples = (int) obj.getSchema().getNumSamples();
       return obj.getSchema().getArbGeomParams();
    }
    else{
       ESS_LOG_WARNING("Could not read ArgGeomParams from "<<iObj.getFullName());
       return Abc::ICompoundProperty();
    }
}

Abc::FloatArraySamplePtr getKnotVector(AbcG::ICurves& obj)
{
   ESS_PROFILE_FUNC();

   Abc::ICompoundProperty arbGeom = obj.getSchema().getArbGeomParams();

   if(!arbGeom.valid()){
      return Abc::FloatArraySamplePtr();
   }

   if ( arbGeom.getPropertyHeader( ".knot_vectors" ) != NULL ){
      Abc::IFloatArrayProperty knotProp = Abc::IFloatArrayProperty( arbGeom, ".knot_vectors" );
      if(knotProp.valid() && knotProp.getNumSamples() != 0){
         return knotProp.getValue(0);
      }
   }
   if ( arbGeom.getPropertyHeader( ".knot_vector" ) != NULL ){
      Abc::IFloatArrayProperty knotProp = Abc::IFloatArrayProperty( arbGeom, ".knot_vector" );
      if(knotProp.valid() && knotProp.getNumSamples() != 0){
         return knotProp.getValue(0);
      }
   }

   return Abc::FloatArraySamplePtr();
}

Abc::UInt16ArraySamplePtr getCurveOrders(AbcG::ICurves& obj)
{
   ESS_PROFILE_FUNC();

   Abc::ICompoundProperty arbGeom = obj.getSchema().getArbGeomParams();

   if(!arbGeom.valid()){
      return Abc::UInt16ArraySamplePtr();
   }

   if ( arbGeom.getPropertyHeader( ".orders" ) != NULL ){
      Abc::IUInt16ArrayProperty orders = Abc::IUInt16ArrayProperty( arbGeom, ".orders" );
      if(orders.valid() && orders.getNumSamples() != 0){
         return orders.getValue(0);
      }
   }

   return Abc::UInt16ArraySamplePtr();
}



bool validateCurveData( Abc::P3fArraySamplePtr pCurvePos, Abc::Int32ArraySamplePtr pCurveNbVertices, Abc::UInt16ArraySamplePtr pOrders, Abc::FloatArraySamplePtr pKnotVec, AbcG::CurveType type )
{
   ESS_PROFILE_FUNC();

   int nDefaultOrder = 4;
   const int numCurves = (int)pCurveNbVertices->size();
   const int numControl = (int)pCurvePos->size();

   int numControlNB = 0;

   for(int i=0; i<pCurveNbVertices->size(); i++){
      numControlNB += pCurveNbVertices->get()[i];
   }

   if(numControl != numControlNB){
      ESS_LOG_ERROR("Size mismatch between vertices and nbVertices. Cannot load curve.");
      return false;
   }

   if(pOrders && pCurveNbVertices->size() != pOrders->size()){
      ESS_LOG_ERROR("Size mismatch between numOrders and nbVertices. Cannot load curve.");
      return false;
   }

   if(pKnotVec){

      int abcTotalKnots = 0;//numControl + numCurves * (nDefaultOrder - 2) ;

      if(pOrders){
         //calculate the expected knot vec size
         for(int i=0; i<pCurveNbVertices->size(); i++){
            abcTotalKnots += pCurveNbVertices->get()[i] + pOrders->get()[i] - 2; 
         }
      }
      else{//single order
         int nOrder = 0;
         if(type == AbcG::kCubic){
            nOrder = 4;
         }
         else if(type == AbcG::kLinear){
            nOrder = 2;
         }

         abcTotalKnots = numControl + numCurves * (nOrder - 2) ;
      }

      if( abcTotalKnots != pKnotVec->size() ){
         ESS_LOG_ERROR("Knot vector has the wrong size. Cannot load curve.");
      }
   }

   return true;
}

int getCurveOrder(int i, Abc::UInt16ArraySamplePtr pOrders, AbcG::CurveType type)
{
   if(pOrders){
      return pOrders->get()[i];
   }
   else if(type == AbcG::kCubic){
      return 4;
   }
   else if(type == AbcG::kLinear){
      return 2;
   }
   return 4;
}

AbcG::IVisibilityProperty getAbcVisibilityProperty(Abc::IObject shapeObj)
{
    Abc::IObject parent = shapeObj.getParent();
    if(parent.valid()){
        //ESS_LOG_WARNING("loading vis from xform");
	    AbcG::IVisibilityProperty visProp = AbcG::GetVisibilityProperty(parent);
        if(visProp.valid()){
           return visProp;
        }
    }
    //ESS_LOG_WARNING("loading vis from shape");
	return AbcG::GetVisibilityProperty(shapeObj);
}

typedef std::map<std::string, int> identifierCountMap;
identifierCountMap identifierCount;

void clearIdentifierMap()
{
   identifierCount.clear();
}

std::string getUniqueName(const std::string& parentFullName, std::string& name)
{
   std::string identifier = parentFullName;
   identifier += "/";
   identifier += name;

   //ESS_LOG_WARNING("lookup: "<<identifier);

   identifierCountMap::iterator it = identifierCount.find(identifier);

   if(it == identifierCount.end()){
      identifierCount[identifier] = 0;
      return name;
   }
   else{
      identifierCount[identifier] = it->second + 1;
      
      std::stringstream stream;
      stream<<removeXfoSuffix(name)<<"_"<<it->second<<"Xfo";

      ESS_LOG_WARNING("Renaming object "<<stream.str());

      return stream.str();
   }
}