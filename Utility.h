#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "Foundation.h"
#include "CommonUtilities.h"

#include <xsi_ref.h>
#include <xsi_operator.h>

#include "Alembic\AbcCoreAbstract\TimeSampling.h"


XSI::CStatus alembicOp_Define( XSI::CRef& in_ctxt );
XSI::CStatus alembicOp_DefineLayout( XSI::CRef& in_ctxt );
XSI::CStatus alembicOp_Term( XSI::CRef& in_ctxt );

std::string getIdentifierFromRef(XSI::CRef in_Ref, bool includeHierarchy = false);
XSI::CString truncateName(const XSI::CString & in_Name);
XSI::CString getFullNameFromIdentifier(XSI::CRef importRootNode, std::string in_Identifier);
XSI::CRef getRefFromIdentifier(XSI::CRef importRootNode, std::string in_Identifier);
int getNodeDepthFromRef(XSI::CRef in_Ref);
XSI::CRefArray getOperators( XSI::CRef in_Ref);
bool isRefAnimated(const XSI::CRef & in_Ref, bool xformCache = false, bool globalSpace = false);
bool returnIsRefAnimated(const XSI::CRef & in_Ref, bool animated);
void clearIsRefAnimatedCache();
void updateOperatorInfo( XSI::Operator& op, SampleInfo& sampleInfo, AbcA::TimeSamplingPtr timeSamplingPtr, int nPointsPrimitive, int nPointsCache);

// remapping imported names
void nameMapAdd(XSI::CString identifier, XSI::CString name);
XSI::CString nameMapGet(XSI::CString identifier);
void nameMapClear();

bool hasStandinSupport();
XSI::CString getDSOPath();


#endif  // _FOUNDATION_H_
