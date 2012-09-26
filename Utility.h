#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "Foundation.h"
#include "CommonUtilities.h"
#include <xsi_ref.h>

XSI::CStatus alembicOp_Define( XSI::CRef& in_ctxt );
XSI::CStatus alembicOp_DefineLayout( XSI::CRef& in_ctxt );
XSI::CStatus alembicOp_Term( XSI::CRef& in_ctxt );

std::string getIdentifierFromRef(XSI::CRef in_Ref, bool includeHierarchy = false);
XSI::CString truncateName(const XSI::CString & in_Name);
XSI::CString getFullNameFromIdentifier(std::string in_Identifier);
XSI::CRef getRefFromIdentifier(std::string in_Identifier);
XSI::CRefArray getOperators( XSI::CRef in_Ref);
bool isRefAnimated(const XSI::CRef & in_Ref, bool xformCache = false, bool globalSpace = false);
bool returnIsRefAnimated(const XSI::CRef & in_Ref, bool animated);
void clearIsRefAnimatedCache();

// remapping imported names
void nameMapAdd(XSI::CString identifier, XSI::CString name);
XSI::CString nameMapGet(XSI::CString identifier);
void nameMapClear();

bool hasStandinSupport();
XSI::CString getDSOPath();


#endif  // _FOUNDATION_H_
