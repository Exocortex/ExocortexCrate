#include "stdafx.h"

#include "EmptySplineObject.h"
#include "AlembicArchiveStorage.h"
#include "AlembicLicensing.h"
#include "AlembicNames.h"
#include "AlembicVisibilityController.h"
#include "Utility.h"

static EmptySplineObjectClassDesc sEmptySplineObjectDesc;

ClassDesc2 *GetEmptySplineObjectClassDesc() { return &sEmptySplineObjectDesc; }
void EmptySplineObject::BeginEditParams(IObjParam *ip, ULONG flags,
                                        Animatable *prev)
{
  SimpleSpline::BeginEditParams(ip, flags, prev);
}

void EmptySplineObject::EndEditParams(IObjParam *ip, ULONG flags,
                                      Animatable *next)
{
  SimpleSpline::EndEditParams(ip, flags, next);
}

void EmptySplineObject::BuildShape(TimeValue t, BezierShape &ashape)
{
  // Set the current inteval for the cache
  ivalid = FOREVER;
}

EmptySplineObject::EmptySplineObject() : SimpleSpline()
{
  ReadyInterpParameterBlock();  // Build the interpolations parameter block in
                                // SimpleSpline
}

EmptySplineObject::~EmptySplineObject()
{
  DeleteAllRefsFromMe();
  UnReadyInterpParameterBlock();
}

RefTargetHandle EmptySplineObject::Clone(RemapDir &remap)
{
  EmptySplineObject *newob = new EmptySplineObject();
  newob->SimpleSplineClone(this, remap);
  newob->ivalid.SetEmpty();
  BaseClone(this, newob, remap);
  return (newob);
}

BOOL EmptySplineObject::ValidForDisplay(TimeValue t) { return TRUE; }
IOResult EmptySplineObject::Load(ILoad *iload)
{
  return SimpleSpline::Load(iload);
}
