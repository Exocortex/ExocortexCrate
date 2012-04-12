#ifndef __EMPTY_SPLINE_OBJECT__H
#define __EMPTY_SPLINE_OBJECT__H

#include "Foundation.h"
#include "MNMath.h"
#include "resource.h"
#include "surf_api.h"
#include "AlembicDefinitions.h"
#include <iparamb2.h>
#include "AlembicNames.h"


class EmptySplineObject: public SimpleSpline {			   	
	public:
		void BuildShape(TimeValue t,BezierShape& ashape);

		EmptySplineObject();
		~EmptySplineObject();

		//  inherited virtual methods:

        CreateMouseCallBack* GetCreateMouseCallBack() { return NULL; }
		void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
		void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		TCHAR *GetObjectName() { return _T("Spline"); }
		void InitNodeName(TSTR& s) { s = _T("Spline"); }		
		Class_ID ClassID() { return EMPTY_SPLINE_OBJECT_CLASSID; }  
		void GetClassName(TSTR& s) { s = _T("Spline"); }
		RefTargetHandle Clone(RemapDir& remap);
		BOOL ValidForDisplay(TimeValue t);

		void InvalidateUI() {}

		// IO
		IOResult Load(ILoad *iload);
private:
};				

//------------------------------------------------------

class EmptySplineObjectClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new EmptySplineObject; }
	const TCHAR *	ClassName() { return _T("Spline"); }
	SClass_ID		SuperClassID() { return SHAPE_CLASS_ID; }
   	Class_ID		ClassID() { return EMPTY_SPLINE_OBJECT_CLASSID; }
    const TCHAR* 	Category() { return EXOCORTEX_ALEMBIC_CATEGORY;  }
	void			ResetClassParams(BOOL fileReset) {}
	};

#endif // __EMPTY_SPLINE_OBJECT__H

