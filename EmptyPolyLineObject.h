#ifndef __EMPTY_POLYLINE_OBJECT__H
#define __EMPTY_POLYLINE_OBJECT__H

#include "Foundation.h"
#include "AlembicMax.h"
#include "resource.h"
#include "AlembicDefinitions.h"
#include "AlembicNames.h"


class EmptyPolyLineObject : public LinearShape {			   	
	public:
		void BuildShape(TimeValue t,BezierShape& ashape);

		EmptyPolyLineObject();
		~EmptyPolyLineObject();

		//  inherited virtual methods:

		CreateMouseCallBack* GetCreateMouseCallBack() { return NULL; }
		CONST_2013 TCHAR *GetObjectName() { return _T("PolyLine"); }
		void InitNodeName(TSTR& s) { s = _T("PolyLine"); }		
		Class_ID ClassID() { return EMPTY_POLYLINE_OBJECT_CLASSID; }  
		void GetClassName(TSTR& s) { s = _T("PolyLine"); }
	
		void InvalidateUI() {}

		RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, 
		PartID& partID, RefMessage message);
private:
};				

//------------------------------------------------------

class EmptyPolyLineObjectClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new EmptyPolyLineObject; }
	const TCHAR *	ClassName() { return _T("PolyLine"); }
	SClass_ID		SuperClassID() { return SHAPE_CLASS_ID; }
   	Class_ID		ClassID() { return EMPTY_POLYLINE_OBJECT_CLASSID; }
    const TCHAR* 	Category() { return EXOCORTEX_ALEMBIC_CATEGORY;  }
	void			ResetClassParams(BOOL fileReset) {}
	};

#endif // __EMPTY_SPLINE_OBJECT__H

