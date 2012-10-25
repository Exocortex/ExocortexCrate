#include "stdafx.h"
#include "EmptyPolyLineObject.h"
#include "AlembicArchiveStorage.h"
#include "AlembicVisibilityController.h"
#include "AlembicLicensing.h"
#include "AlembicNames.h"
#include "Utility.h"

	


static EmptyPolyLineObjectClassDesc sEmptyPolyLineObjectDesc;

ClassDesc2* GetEmptyPolyLineObjectClassDesc() { return &sEmptyPolyLineObjectDesc; }



EmptyPolyLineObject::EmptyPolyLineObject() : LinearShape() 
	{
	//ReadyInterpParameterBlock();		// Build the interpolations parameter block in SimpleSpline
 	}

EmptyPolyLineObject::~EmptyPolyLineObject()
	{
	//DeleteAllRefsFromMe();
	//UnReadyInterpParameterBlock();
	}



RefResult EmptyPolyLineObject::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message) {

	return REF_SUCCEED;
}