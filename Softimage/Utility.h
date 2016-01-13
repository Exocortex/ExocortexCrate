#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "CommonUtilities.h"

struct ArchiveInfo {
  std::string path;

  bool bTimeSamplingInit;
  AbcA::TimeSamplingPtr timeSampling;

  ArchiveInfo() : bTimeSamplingInit(false) {}
};

XSI::CStatus alembicOp_Define(XSI::CRef& in_ctxt);
XSI::CStatus alembicOp_DefineLayout(XSI::CRef& in_ctxt);
XSI::CStatus alembicOp_Init(XSI::CRef& in_ctxt);
XSI::CStatus alembicOp_Term(XSI::CRef& in_ctxt);
XSI::CStatus alembicOp_PathEdit(XSI::CRef& in_ctxt, XSI::CString& path);
XSI::CStatus alembicOp_TimeSamplingInit(
    XSI::CRef& in_ctxt);  //, AbcA::TimeSamplingPtr timeSampling );
XSI::CStatus alembicOp_getFrameNum(XSI::CRef& in_ctxt, double sampleTime,
                                   int& frameNum);
XSI::CStatus alembicOp_Multifile(XSI::CRef& in_ctxt, bool multifile,
                                 double time, XSI::CString& path);

std::string getIdentifierFromRef(XSI::CRef in_Ref,
                                 bool includeHierarchy = false);
XSI::CString truncateName(const XSI::CString& in_Name);
XSI::CString stripNamespacePrefix(const XSI::CString& in_Name);
XSI::CString getFullNameFromIdentifier(XSI::CRef importRootNode,
                                       std::string in_Identifier,
                                       bool bMergedLeaf = true);
// XSI::CRef getRefFromIdentifier(XSI::CRef importRootNode, std::string
// in_Identifier, bool bMergedLeaf);
int getNodeDepthFromRef(XSI::CRef in_Ref);
XSI::CRefArray getOperators(XSI::CRef in_Ref);
bool isRefAnimated(const XSI::CRef& in_Ref, bool xformCache = false,
                   bool globalSpace = false);
bool returnIsRefAnimated(const XSI::CRef& in_Ref, bool animated);
void clearIsRefAnimatedCache();
void updateOperatorInfo(XSI::Operator& op, SampleInfo& sampleInfo,
                        AbcA::TimeSamplingPtr timeSamplingPtr,
                        int nPointsPrimitive, int nPointsCache);

bool hasStandinSupport();
XSI::CString getDSOPath();

Abc::M44f CMatrix4_to_M44f(const XSI::MATH::CMatrix4& m);
Abc::M44d CMatrix4_to_M44d(const XSI::MATH::CMatrix4& m);

#endif  // _FOUNDATION_H_
