#ifndef _ARNOLD_ALEMBIC_UTILITY_H_
#define _ARNOLD_ALEMBIC_UTILITY_H_

#include "foundation.h"
#include "AlembicLicensing.h"

struct SampleInfo
{
   Alembic::AbcCoreAbstract::index_t floorIndex;
   Alembic::AbcCoreAbstract::index_t ceilIndex;
   double alpha;
};

SampleInfo getSampleInfo
(
   double iFrame,
   Alembic::AbcCoreAbstract::TimeSamplingPtr iTime,
   size_t numSamps
);

bool removeUvsDuplicate(Alembic::AbcGeom::IV2fGeomParam &uvParam, SampleInfo &sampleInfo, AtArray *uvs, AtArray *uvsIdx);

template<class OBJTYPE, class DATATYPE>
bool getArbGeomParamPropertyAlembic( OBJTYPE obj, std::string name, Alembic::Abc::ITypedArrayProperty<DATATYPE> &pOut ) {
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

#endif