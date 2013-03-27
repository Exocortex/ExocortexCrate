#ifndef __COMMON_UTILITIES_H
#define __COMMON_UTILITIES_H

#include "CommonAlembic.h"
#include "CommonAbcCache.h"

#include "CommonPBar.h"

#define ALEMBIC_SAFE_DELETE(p)  if(p) delete p; p = 0;


struct SampleInfo
{
   Alembic::AbcCoreAbstract::index_t floorIndex;
   Alembic::AbcCoreAbstract::index_t ceilIndex;
   double alpha;
};

#ifndef uint64_t
  typedef boost::uint64_t uint64_t;
#endif

template<class Key, class Data>
class MRUCache {
private:
	struct MRUCacheEntry {
		Key key;
		Data data;
		Abc::uint64_t lastAccess;
	};
	std::vector<MRUCacheEntry> entries;
	Abc::uint64_t nextAccess;
	int maxEntries;

public:
  MRUCache( int _maxEntries = 2 ): maxEntries(_maxEntries), nextAccess(0) {}

	bool contains( Key const& key ) const {
		for( int i = 0; i < entries.size(); i ++ ) {
			if( entries[i].key == key ) {
				return true;
			}
		}
		return false;
	}
	void touch( Key const& key ) {
		for( int i = 0; i < entries.size(); i ++ ) {
			if( entries[i].key == key ) {
				entries[i].lastAccess = nextAccess;
				nextAccess ++;
			}
		}
	}
	Data& get( Key const& key ) {
		for( int i = 0; i < entries.size(); i ++ ) {
			if( entries[i].key == key ) {
				entries[i].lastAccess = nextAccess;
				return entries[i].data;
			}
		}
	}
	void insert( Key const& key, Data& data ) {
		if( entries.size() >= maxEntries ) {
			int oldestIndex = -1;
			uint64_t oldestLastAccess;
			for( int i = 0; i < entries.size(); i ++ ) {
				uint64_t lastAccess = entries[i].lastAccess;
				if( oldestIndex == -1 || lastAccess < oldestLastAccess ) {
					oldestLastAccess = lastAccess;
					oldestIndex = i;
				}
			}
			if( oldestIndex >= 0 ) {
				entries.erase( entries.begin() + oldestIndex );
			}
		}
		MRUCacheEntry entry;
		entry.key = key;
		entry.data = data;
		entry.lastAccess = nextAccess;
		entries.push_back( entry );
		nextAccess ++;
	}
};

struct ArchiveInfo
{
   std::string path;
};

std::string getExporterName( std::string const& shortName );
std::string getExporterFileName( std::string const& fileName );

AbcArchiveCache* getArchiveCache( std::string const& path, CommonProgressBar *pBar = 0 );

AbcObjectCache* getObjectCacheFromArchive(std::string const& path, std::string const& identifier);

Alembic::Abc::IArchive * getArchiveFromID(std::string const& path);
std::string addArchive(Alembic::Abc::IArchive * archive);
void deleteArchive(std::string const& path);
void deleteAllArchives();
Alembic::Abc::IObject getObjectFromArchive(std::string const& path, std::string const& identifier);
std::string resolvePath(std::string const& path); 
std::string resolvePath_Internal(std::string const& path); // must be defined in binding applications.

// ref counting
bool archiveExists(std::string const& path);
int addRefArchive(std::string const& path);
int delRefArchive(std::string const& path);
int getRefArchive(std::string const& path);

void getPaths(std::vector<std::string>& paths);

bool parseTrailingNumber( std::string const& text, std::string const& requiredPrefix, int& number );
      
bool validate_filename_location(const char *filename);

typedef std::map<std::string,std::string> stringMap;
typedef std::map<std::string,std::string>::iterator stringMapIt;
typedef std::pair<std::string,std::string> stringPair;

// sortable math objects
class SortableV3f : public Alembic::Abc::V3f
{
public:  
   SortableV3f()
   {
      x = y = z = 0.0f;
   }

   SortableV3f(const Alembic::Abc::V3f & other)
   {
      x = other.x;
      y = other.y;
      z = other.z;
   }
   bool operator < ( const SortableV3f & other) const
   {
      if(other.x != x)
         return other.x > x;
      if(other.y != y)
         return other.y > y;
      return other.z > z;
   }
   bool operator > ( const SortableV3f & other) const
   {
      if(other.x != x)
         return other.x < x;
      if(other.y != y)
         return other.y < y;
      return other.z < z;
   }
   bool operator == ( const SortableV3f & other) const
   {
      if(other.x != x)
         return false;
      if(other.y != y)
         return false;
      return other.z == z;
   }
};

class SortableV2f : public Alembic::Abc::V2f
{
public:  
   SortableV2f()
   {
      x = y = 0.0f;
   }

   SortableV2f(const Alembic::Abc::V2f & other)
   {
      x = other.x;
      y = other.y;
   }
   bool operator < ( const SortableV2f & other) const
   {
      if(other.x != x)
         return other.x > x;
      return other.y > y;
   }
   bool operator > ( const SortableV2f & other) const
   {
      if(other.x != x)
         return other.x < x;
      return other.y < y;
   }
   bool operator == ( const SortableV2f & other) const
   {
      if(other.x != x)
         return false;
      return other.y == y;
   }
};

Imath::M33d extractRotation(Imath::M44d& m);

SampleInfo getSampleInfo(double iFrame,Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps);
Alembic::Abc::ICompoundProperty getCompoundFromObject(Alembic::Abc::IObject &object);
Alembic::Abc::TimeSamplingPtr getTimeSamplingFromObject(Alembic::Abc::IObject &object);
Alembic::Abc::TimeSamplingPtr getTimeSamplingFromObject(Alembic::Abc::OObject *object);
size_t getNumSamplesFromObject(Alembic::Abc::IObject &object);
size_t getNumSamplesFromObject(Alembic::Abc::OObject *object);
bool isObjectConstant(Alembic::Abc::IObject &object );

struct BasicSchemaData
{
	enum SCHEMA_TYPE
	{
		__XFORM,
		__POLYMESH,
		__CURVES,
		__NUPATCH,
		__POINTS,
		__SUBDIV,
		__CAMERA,
		__FACESET
	};
	SCHEMA_TYPE type;
	bool isConstant;
	size_t nbSamples;
};
bool getBasicSchemaDataFromObject(Alembic::Abc::IObject &object, BasicSchemaData &bsd);

float getTimeOffsetFromObject( Alembic::Abc::IObject &object, SampleInfo const& sampleInfo );

template<typename SCHEMA>
float getTimeOffsetFromSchema( SCHEMA &schema, SampleInfo const& sampleInfo ) {
	Alembic::Abc::TimeSamplingPtr timeSampling = schema.getTimeSampling();
	if( timeSampling.get() == NULL ) {
		return 0;
	}
	else {
		return (float)( ( timeSampling->getSampleTime(sampleInfo.ceilIndex) -
			timeSampling->getSampleTime(sampleInfo.floorIndex) ) * sampleInfo.alpha );
	}
}

std::string getModelName( const std::string &identifier );
std::string removeXfoSuffix(const std::string& importName);

template<class OBJTYPE, class DATATYPE>
bool getArbGeomParamPropertyAlembic( OBJTYPE obj, std::string name, Alembic::Abc::ITypedArrayProperty<DATATYPE> &pOut ) {
	if( ! obj.valid() || ! obj.getSchema().valid() ) {
		return false;
	}
	// look for name with period on it.
	std::string nameWithDotPrefix = std::string(".") + name;
	if ( obj.getSchema().getPropertyHeader( nameWithDotPrefix ) != NULL ) {
		Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema(), nameWithDotPrefix );
		if( prop.valid() && prop.getNumSamples() > 0 ) {
			pOut = prop;
			return true;
		}
	}
	if( obj.getSchema().getArbGeomParams() != NULL ) {
		if ( obj.getSchema().getArbGeomParams().getPropertyHeader( name ) != NULL ) {
			Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema().getArbGeomParams(), name );
			if( prop.valid() && prop.getNumSamples() > 0 ) {
				pOut = prop;
				return true;
			}
		}
		if ( obj.getSchema().getArbGeomParams().getPropertyHeader( nameWithDotPrefix ) != NULL ) {
			Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema().getArbGeomParams(), nameWithDotPrefix );
			if( prop.valid() && prop.getNumSamples() > 0 ) {
				pOut = prop;
				return true;
			}
		}
	}

	return false;
}

template<class OBJTYPE, class DATATYPE>
bool getArbGeomParamPropertyAlembic_Permissive( OBJTYPE obj, std::string name, Alembic::Abc::ITypedArrayProperty<DATATYPE> &pOut ) {
	if( ! obj.valid() || ! obj.getSchema().valid() ) {
		return false;
	}
	// look for name with period on it.
	std::string nameWithDotPrefix = std::string(".") + name;
	if ( obj.getSchema().getPropertyHeader( nameWithDotPrefix ) != NULL ) {
		Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema(), nameWithDotPrefix );
		if( prop.valid() ) {
			pOut = prop;
			return true;
		}
	}
	if( obj.getSchema().getArbGeomParams() != NULL ) {
		if ( obj.getSchema().getArbGeomParams().getPropertyHeader( name ) != NULL ) {
			Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema().getArbGeomParams(), name );
			if( prop.valid() ) {
				pOut = prop;
				return true;
			}
		}
		if ( obj.getSchema().getArbGeomParams().getPropertyHeader( nameWithDotPrefix ) != NULL ) {
			Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema().getArbGeomParams(), nameWithDotPrefix );
			if( prop.valid() ) {
				pOut = prop;
				return true;
			}
		}
	}

	return false;
}

namespace NodeCategory
{
   enum type{
	   GEOMETRY,//probably should be called MERGEABLE
	   XFORM,
	   UNSUPPORTED
   };

   inline type get(Alembic::AbcGeom::IObject& iObj)
   {
	   if( Alembic::AbcGeom::IPolyMesh::matches(iObj.getMetaData()) ||
		   Alembic::AbcGeom::ICamera::matches(iObj.getMetaData()) ||
		   Alembic::AbcGeom::IPoints::matches(iObj.getMetaData()) ||
		   Alembic::AbcGeom::ICurves::matches(iObj.getMetaData()) ||
		   Alembic::AbcGeom::ISubD::matches(iObj.getMetaData()) ||
		   Alembic::AbcGeom::INuPatch::matches(iObj.getMetaData()) ||
         Alembic::AbcGeom::ILight::matches(iObj.getMetaData())
         ) {
		   return GEOMETRY;
	   }
	   else if(Alembic::AbcGeom::IXform::matches(iObj.getMetaData())){
		   return XFORM;
	   }
	   else {
		   return UNSUPPORTED;
	   }
   }
};


void getMergeInfo( AbcArchiveCache *pArchiveCache, AbcObjectCache *pObjectCache, bool& bCreateNullNode, int& nMergedGeomNodeIndex, AbcObjectCache **ppMergedObjectCache );

int prescanAlembicHierarchy(AbcArchiveCache *pArchiveCache, AbcObjectCache *pRootObjectCache, std::vector<std::string>& nodes, std::map<std::string, bool>& map, bool bIncludeChildren=false);

template<class S>
struct cia_map_key
{
 Alembic::Abc::int32_t vid;
 S data;

 cia_map_key(const Alembic::Abc::int32_t &_vid, const S &_data): vid(_vid), data(_data) {}

 bool operator < ( const cia_map_key & other) const
 {
   if (vid == other.vid)
     return data < other.data;
   return vid < other.vid;
 }
};

//Alembic::Abc::N3f
//SortableV3f
template <class T, class S> void createIndexedArray(const std::vector<Alembic::Abc::int32_t> &faceIndicesVec, const std::vector<T>& inputVec, std::vector<T>& outputVec, std::vector<Alembic::Abc::uint32_t>& outputIndices)
{  
  std::map<cia_map_key<S>, size_t> normalMap;

  outputIndices.resize(inputVec.size());
  outputVec.clear();

  // loop over all data
  for(size_t i=0; i<inputVec.size() && i<faceIndicesVec.size(); ++i)
  {
    cia_map_key<S> mkey(faceIndicesVec[i], inputVec[i]);
    if (normalMap.find(mkey) != normalMap.end())    // the pair <vertexId, S> was found, let reuse it's index!
      outputIndices[i] = (Alembic::Abc::uint32_t) normalMap.find(mkey)->second;
    else
    {
      const int map_size = (int) normalMap.size();
      outputVec.push_back(inputVec[i]);
      outputIndices[i] = map_size;
      normalMap[mkey] = map_size;
    }
  }
}

Abc::ICompoundProperty getArbGeomParams(const AbcG::IObject& iObj, AbcA::TimeSamplingPtr& timeSampling, int& nSamples);

Abc::FloatArraySamplePtr getKnotVector(AbcG::ICurves& obj);
Abc::UInt16ArraySamplePtr getCurveOrders(AbcG::ICurves& obj);
bool validateCurveData( Abc::P3fArraySamplePtr pCurvePos, Abc::Int32ArraySamplePtr pCurveNbVertices, Abc::UInt16ArraySamplePtr pOrders, Abc::FloatArraySamplePtr pKnotVec, AbcG::CurveType type );
int getCurveOrder(int i, Abc::UInt16ArraySamplePtr pOrders, AbcG::CurveType type);

#endif // __COMMON_UTILITIES_H
