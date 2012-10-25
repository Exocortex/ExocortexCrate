#ifndef __COMMON_UTILITIES_H
#define __COMMON_UTILITIES_H

#include "CommonAlembic.h"

#define ALEMBIC_SAFE_DELETE(p)  if(p) delete p; p = 0;


struct SampleInfo
{
   Alembic::AbcCoreAbstract::index_t floorIndex;
   Alembic::AbcCoreAbstract::index_t ceilIndex;
   double alpha;
};

struct ArchiveInfo
{
   std::string path;
};


Alembic::Abc::IArchive * getArchiveFromID(std::string path);
std::string addArchive(Alembic::Abc::IArchive * archive);
void deleteArchive(std::string path);
void deleteAllArchives();
Alembic::Abc::IObject getObjectFromArchive(std::string path, std::string identifier);
std::string resolvePath(std::string path); 
std::string resolvePath_Internal(std::string path); // must be defined in binding applications.

// ref counting
bool archiveExists(std::string path);
int addRefArchive(std::string path);
int delRefArchive(std::string path);
int getRefArchive(std::string path);

      
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
	   GEOMETRY,
	   XFORM,
	   UNSUPPORTED
   };

   inline type get(Alembic::AbcGeom::IObject& iObj)
   {
	   if( Alembic::AbcGeom::IPolyMesh::matches(iObj.getMetaData()) ||
		   Alembic::AbcGeom::ICamera::matches(iObj.getMetaData()) ||
		   Alembic::AbcGeom::IPoints::matches(iObj.getMetaData()) ||
		   Alembic::AbcGeom::ICurves::matches(iObj.getMetaData()) ||
		   Alembic::AbcGeom::ISubD::matches(iObj.getMetaData())||
		   Alembic::AbcGeom::INuPatch::matches(iObj.getMetaData())) {
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


void getMergeInfo( Alembic::AbcGeom::IObject& iObj, bool& bCreateNullNode, int& nMergedGeomNodeIndex, Alembic::AbcGeom::IObject& mergedGeomChild, bool bAlwaysMerge=true);

int prescanAlembicHierarchy(Alembic::AbcGeom::IObject root, std::vector<std::string>& nodes, std::map<std::string, bool>& map);

//Alembic::Abc::N3f
//SortableV3f
template <class T, class S> void createIndexedArray(const std::vector<Alembic::Abc::int32_t> &faceIndicesVec, const std::vector<T>& inputVec, std::vector<T>& outputVec, std::vector<Alembic::Abc::uint32_t>& outputIndices)
{  
  struct map_key
  {
    Alembic::Abc::int32_t vid;
    S data;

    map_key(const Alembic::Abc::int32_t &_vid, const S &_data): vid(_vid), data(_data) {}

    bool operator < ( const map_key & other) const
    {
      if (vid == other.vid)
        return data < other.data;
      return vid < other.vid;
    }
  };

  std::map<map_key, size_t> normalMap;
  std::map<map_key, size_t>::const_iterator it;

  outputIndices.resize(inputVec.size());
  outputVec.clear();

  // loop over all data
  for(size_t i=0; i<inputVec.size(); ++i)
  {
    map_key mkey(faceIndicesVec[i], inputVec[i]);
    it = normalMap.find(mkey);
    if (it != normalMap.end())    // the pair <vertexId, S> was found, let reuse it's index!
      outputIndices[i] = (Alembic::Abc::uint32_t) it->second;
    else
    {
      const int map_size = (int) normalMap.size();
      outputVec.push_back(inputVec[i]);
      outputIndices[i] = map_size;
      normalMap[mkey] = map_size;
    }
  }
}


#endif // __COMMON_UTILITIES_H
