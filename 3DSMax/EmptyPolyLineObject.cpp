#include "EmptyPolyLineObject.h"
#include "AlembicArchiveStorage.h"
#include "AlembicLicensing.h"
#include "AlembicNames.h"
#include "AlembicVisibilityController.h"
#include "Utility.h"
#include "stdafx.h"

static EmptyPolyLineObjectClassDesc sEmptyPolyLineObjectDesc;

ClassDesc2* GetEmptyPolyLineObjectClassDesc()
{
  return &sEmptyPolyLineObjectDesc;
}

EmptyPolyLineObject::EmptyPolyLineObject() : LinearShape()
{
  // ReadyInterpParameterBlock();		// Build the interpolations parameter
  // block in SimpleSpline
}

EmptyPolyLineObject::~EmptyPolyLineObject()
{
  // DeleteAllRefsFromMe();
  // UnReadyInterpParameterBlock();
}

#if (crate_Max_Version >= 2015)
RefResult EmptyPolyLineObject::NotifyRefChanged(const Interval& changeInt,
                                                RefTargetHandle hTarget,
                                                PartID& partID,
                                                RefMessage message,
                                                BOOL propagate)
{
#else
RefResult EmptyPolyLineObject::NotifyRefChanged(Interval changeInt,
                                                RefTargetHandle hTarget,
                                                PartID& partID,
                                                RefMessage message)
{
#endif
  return REF_SUCCEED;
}