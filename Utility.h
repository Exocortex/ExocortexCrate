#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "Foundation.h"
#include <xsi_ref.h>

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

XSI::CStatus alembicOp_Define( XSI::CRef& in_ctxt );
XSI::CStatus alembicOp_DefineLayout( XSI::CRef& in_ctxt );
XSI::CStatus alembicOp_Term( XSI::CRef& in_ctxt );

SampleInfo getSampleInfo(double iFrame,Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps);
std::string getIdentifierFromRef(XSI::CRef in_Ref, bool includeHierarchy = false);
XSI::CString truncateName(const XSI::CString & in_Name);
XSI::CString getFullNameFromIdentifier(std::string in_Identifier);
XSI::CRef getRefFromIdentifier(std::string in_Identifier);
int getNodeDepthFromRef(XSI::CRef in_Ref);
XSI::CRefArray getOperators( XSI::CRef in_Ref);
bool isRefAnimated(const XSI::CRef & in_Ref, bool xformCache = false, bool globalSpace = false);
bool returnIsRefAnimated(const XSI::CRef & in_Ref, bool animated);
void clearIsRefAnimatedCache();

// remapping imported names
void nameMapAdd(XSI::CString identifier, XSI::CString name);
XSI::CString nameMapGet(XSI::CString identifier);
void nameMapClear();

// utility mappings
Alembic::Abc::ICompoundProperty getCompoundFromObject(Alembic::Abc::IObject object);
Alembic::Abc::TimeSamplingPtr getTimeSamplingFromObject(Alembic::Abc::IObject object);
size_t getNumSamplesFromObject(Alembic::Abc::IObject object);

bool hasStandinSupport();
XSI::CString getDSOPath();



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

#endif  // _FOUNDATION_H_
