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


template<class OBJTYPE, class DATATYPE>
bool getArbGeomParamPropertyAlembic( OBJTYPE obj, std::string name, Alembic::Abc::ITypedArrayProperty<DATATYPE> &pOut ) {
	if ( obj.getSchema().getPropertyHeader( name ) != NULL ) {
		Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema(), name );
		if( prop.valid() && prop.getNumSamples() > 0 ) {
			pOut = prop;
			return true;
		}
	}
	if ( obj.getSchema().getArbGeomParams().getPropertyHeader( name ) != NULL ) {
		Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema().getArbGeomParams(), name );
		if( prop.valid() && prop.getNumSamples() > 0 ) {
			pOut = prop;
			return true;
		}
	}

	return false;
}

template<class OBJTYPE, class DATATYPE>
bool getArbGeomParamPropertyAlembic_Permissive( OBJTYPE obj, std::string name, Alembic::Abc::ITypedArrayProperty<DATATYPE> &pOut ) {
	if ( obj.getSchema().getPropertyHeader( name ) != NULL ) {
		Alembic::Abc::ITypedArrayProperty<DATATYPE> prop = Alembic::Abc::ITypedArrayProperty<DATATYPE>( obj.getSchema(), name );
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
	}

	return false;
}

#endif