#include "iparamb.h"
#include "splshape.h"
#include "iparamm.h"
#include "simpspl.h"
#include "AlembicSimpleSpline.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include "iparamb2.h"
#include "AlembicVisCtrl.h"
#include <linshape.h>
#include <splshape.h>

#include "AlembicNames.h"

class AlembicSimpleSpline;

//////////////////////////////////////////////////////////////////////////////////////////
// Import options struct containing the information necessary to fill the Shape object
typedef struct _alembic_fillshape_options
{
public:
    _alembic_fillshape_options();

    Alembic::AbcGeom::IObject  *pIObj;
    BezierShape                *pBezierShape;
    PolyShape                  *pPolyShape;
    TimeValue                   dTicks;
    Interval                    validInterval;
    AlembicDataFillFlags        nDataFillFlags;
} 
alembic_fillshape_options;

_alembic_fillshape_options::_alembic_fillshape_options()
{
    pIObj = NULL;
    pBezierShape = NULL;
    pPolyShape = NULL;
    dTicks = 0;
}


//////////////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
void AlembicImport_FillInShape(alembic_fillshape_options &options);

#define MIN_RADIUS		float(0)
#define MAX_RADIUS		float( 1.0E30)

#define DEF_RADIUS		float(0.0)

class AlembicSimpleSplineCreateCallBack;

class AlembicSimpleSpline: public SimpleSpline, public IParamArray {			   

	friend class AlembicSimpleSplineCreateCallBack;
	
	public:
		// Class vars
		static IParamMap *pmapCreate;
		static IParamMap *pmapTypeIn;
		static IParamMap *pmapParam;
		static IObjParam *ip;
		static int dlgCreateMeth;
		static Point3 crtPos;		
		static float crtRadius;
		
		void BuildShape(TimeValue t,BezierShape& ashape);

		AlembicSimpleSpline();
		~AlembicSimpleSpline();

		//  inherited virtual methods:

        CreateMouseCallBack* GetCreateMouseCallBack();
		void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
		void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		TCHAR *GetObjectName() { return _T("Alembic Spline"); }
		void InitNodeName(TSTR& s) { s = _T("Alembic Spline"); }		
		Class_ID ClassID() { return ALEMBIC_SIMPLE_SPLINE_CLASSID; }  
		void GetClassName(TSTR& s) { s = _T("AlembicSpline"); }
		RefTargetHandle Clone(RemapDir& remap);
		BOOL ValidForDisplay(TimeValue t);

 		// From IParamArray
		BOOL SetValue(int i, TimeValue t, int v);
		BOOL SetValue(int i, TimeValue t, float v);
		BOOL SetValue(int i, TimeValue t, Point3 &v);
		BOOL GetValue(int i, TimeValue t, int &v, Interval &ivalid);
		BOOL GetValue(int i, TimeValue t, float &v, Interval &ivalid);
		BOOL GetValue(int i, TimeValue t, Point3 &v, Interval &ivalid);

		ParamDimension *GetParameterDim(int pbIndex);
		TSTR GetParameterName(int pbIndex);

		void InvalidateUI() { if (pmapParam) pmapParam->Invalidate(); }

		// IO
		IOResult Load(ILoad *iload);

        void SetAlembicId(const std::string &file, const std::string &identifier);
        void SetAlembicUpdateDataFillFlags(unsigned int nFlags) { m_AlembicNodeProps.m_UpdateDataFillFlags = nFlags; }
private:
    alembic_nodeprops m_AlembicNodeProps;
};				

//------------------------------------------------------

class AlembicSimpleSplineClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new AlembicSimpleSpline; }
	const TCHAR *	ClassName() { return _T("Spline"); }
	SClass_ID		SuperClassID() { return SHAPE_CLASS_ID; }
   	Class_ID		ClassID() { return ALEMBIC_SIMPLE_SPLINE_CLASSID; }
    const TCHAR* 	Category() { return EXOCORTEX_ALEMBIC_CATEGORY;  }
	void			ResetClassParams(BOOL fileReset);
	};

static AlembicSimpleSplineClassDesc sAlembicSimpleSplineDesc;

ClassDesc* GetAlembicSimpleSplineClassDesc() { return &sAlembicSimpleSplineDesc; }

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

// class variable for circle class.
IParamMap *AlembicSimpleSpline::pmapCreate = NULL;
IParamMap *AlembicSimpleSpline::pmapParam  = NULL;
IParamMap *AlembicSimpleSpline::pmapTypeIn = NULL;
IObjParam *AlembicSimpleSpline::ip         = NULL;
Point3 AlembicSimpleSpline::crtPos         = Point3(0,0,0);
float AlembicSimpleSpline::crtRadius       = 0.0f;
int AlembicSimpleSpline::dlgCreateMeth = 1; // create_radius

void AlembicSimpleSplineClassDesc::ResetClassParams(BOOL fileReset)
	{
	AlembicSimpleSpline::crtPos          = Point3(0,0,0);
	AlembicSimpleSpline::crtRadius       = 0.0f;
	AlembicSimpleSpline::dlgCreateMeth   = 1; // create_radius
	}

// Parameter map indices
#define PB_RADIUS		0

// Non-parameter block indices
#define PB_CREATEMETHOD		0
#define PB_TI_POS			1
#define PB_TI_RADIUS		2

// Vector length for unit circle
#define CIRCLE_VECTOR_LENGTH 0.5517861843f

//
//
//	Creation method

static int createMethIDs[] = {IDC_CREATEDIAMETER,IDC_CREATERADIUS};

static ParamUIDesc descCreate[] = {
	// Diameter/radius
	ParamUIDesc(PB_CREATEMETHOD,TYPE_RADIO,createMethIDs,2)
	};
#define CREATEDESC_LENGTH 1

//
//
// Type in

static ParamUIDesc descTypeIn[] = {
	
	// Position
	ParamUIDesc(
		PB_TI_POS,
		EDITTYPE_UNIVERSE,
		IDC_TI_POSX,IDC_TI_POSXSPIN,
		IDC_TI_POSY,IDC_TI_POSYSPIN,
		IDC_TI_POSZ,IDC_TI_POSZSPIN,
		-99999999.0f,99999999.0f,
		SPIN_AUTOSCALE),
	
	// Radius
	ParamUIDesc(
		PB_TI_RADIUS,
		EDITTYPE_UNIVERSE,
		IDC_RADIUS,IDC_RADSPINNER,
		MIN_RADIUS,MAX_RADIUS,
		SPIN_AUTOSCALE)
			
	};
#define TYPEINDESC_LENGTH 2

//
//
// Parameters

static ParamUIDesc descParam[] = {
	// Radius
	ParamUIDesc(
		PB_RADIUS,
		EDITTYPE_UNIVERSE,
		IDC_RADIUS,IDC_RADSPINNER,
		MIN_RADIUS,MAX_RADIUS,
		SPIN_AUTOSCALE),	
	
	};
#define PARAMDESC_LENGTH 1


static ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 },		
	{ TYPE_FLOAT, NULL, TRUE, 1 } };

static ParamBlockDescID descVer1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 } };		
#define PBLOCK_LENGTH	1

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer0,2,0)
	};
#define NUM_OLDVERSIONS	1

// Current version
#define CURRENT_VERSION	1
static ParamVersionDesc curVersion(descVer1,PBLOCK_LENGTH,CURRENT_VERSION);

//--- TypeInDlgProc --------------------------------

class CircleTypeInDlgProc : public ParamMapUserDlgProc {
	public:
		AlembicSimpleSpline *co;

		CircleTypeInDlgProc(AlembicSimpleSpline *c) {co=c;}
		INT_PTR DlgProc(TimeValue t,IParamMap *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
		void DeleteThis() {delete this;}
	};

INT_PTR CircleTypeInDlgProc::DlgProc(
		TimeValue t,IParamMap *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
	{
	switch (msg) {
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_TI_CREATE: {
					if (co->crtRadius==0.0) return TRUE;

					// Return focus to the top spinner
					SetFocus(GetDlgItem(hWnd, IDC_TI_POSX));
					
					// We only want to set the value if the object is 
					// not in the scene.
					if (co->TestAFlag(A_OBJ_CREATING)) {
						co->pblock->SetValue(PB_RADIUS,0,co->crtRadius);
						}

					Matrix3 tm(1);
					tm.SetTrans(co->crtPos);
					co->ip->NonMouseCreate(tm);
					// NOTE that calling NonMouseCreate will cause this
					// object to be deleted. DO NOT DO ANYTHING BUT RETURN.
					return TRUE;	
					}
				}
			break;	
		}
	return FALSE;
	}

void AlembicSimpleSpline::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev )
	{
	SimpleSpline::BeginEditParams(ip,flags,prev);
	this->ip = ip;

	if (pmapCreate && pmapParam) {
		
		// Left over from last circle ceated
		pmapCreate->SetParamBlock(this);
		pmapTypeIn->SetParamBlock(this);
		pmapParam->SetParamBlock(pblock);
	} else {
		
		// Gotta make a new one.
		if (flags&BEGIN_EDIT_CREATE) {
			pmapCreate = CreateCPParamMap(
				descCreate,CREATEDESC_LENGTH,
				this,
				ip,
				hInstance,
				MAKEINTRESOURCE(IDD_CIRCLEPARAM1),
				_T("Creation Method"),
				0);

			pmapTypeIn = CreateCPParamMap(
				descTypeIn,TYPEINDESC_LENGTH,
				this,
				ip,
				hInstance,
				MAKEINTRESOURCE(IDD_CIRCLEPARAM3),
				_T("Keyboard Entry"),
				APPENDROLL_CLOSED);
			}

		pmapParam = CreateCPParamMap( 
			descParam,PARAMDESC_LENGTH,
			pblock,
			ip,
			hInstance,
			MAKEINTRESOURCE(IDD_CIRCLEPARAM2),
			_T("Parameters"),
			0);
		}

	if(pmapTypeIn) {
		// A callback for the type in.
		pmapTypeIn->SetUserDlgProc(new CircleTypeInDlgProc(this));
		}
	}
		
void AlembicSimpleSpline::EndEditParams( IObjParam *ip,ULONG flags,Animatable *next )
	{
	SimpleSpline::EndEditParams(ip,flags,next);
	this->ip = NULL;

	if (flags&END_EDIT_REMOVEUI ) {
		if (pmapCreate) DestroyCPParamMap(pmapCreate);
		if (pmapTypeIn) DestroyCPParamMap(pmapTypeIn);
		DestroyCPParamMap(pmapParam);
		pmapParam  = NULL;
		pmapTypeIn = NULL;
		pmapCreate = NULL;
		}

	// Save these values in class variables so the next object created will inherit them.
	}

static void MakeCircle(BezierShape& ashape, float radius) {
	float vector = CIRCLE_VECTOR_LENGTH * radius;
	// Delete all points in the existing spline
	Spline3D *spline = ashape.NewSpline();

	// Now add all the necessary points
	for(int ix=0; ix<4; ++ix) {
		float angle = 6.2831853f * (float)ix / 4.0f;
		float sinfac = (float)sin(angle), cosfac = (float)cos(angle);
		Point3 p(cosfac * radius, sinfac * radius, 0.0f);
		Point3 rotvec = Point3(sinfac * vector, -cosfac * vector, 0.0f);
		spline->AddKnot(SplineKnot(KTYPE_BEZIER,LTYPE_CURVE,p,p + rotvec,p - rotvec));
		}
	spline->SetClosed();
	spline->ComputeBezPoints();
	}

void AlembicSimpleSpline::BuildShape(TimeValue t, BezierShape& ashape) 
{
    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(m_AlembicNodeProps.m_File, m_AlembicNodeProps.m_Identifier);
    if (!iObj.valid())
    {
        return;
    }

    alembic_fillshape_options options;
    options.pIObj =  &iObj;
    options.pBezierShape = &ashape;
    options.dTicks = t;
    options.nDataFillFlags = m_AlembicNodeProps.m_UpdateDataFillFlags;
   
    AlembicImport_FillInShape(options);

    // Set the current inteval for the cache
    ivalid = options.validInterval;
}

AlembicSimpleSpline::AlembicSimpleSpline() : SimpleSpline() 
	{
	ReadyInterpParameterBlock();		// Build the interpolations parameter block in SimpleSpline
	ReplaceReference(USERPBLOCK, CreateParameterBlock(descVer0, PBLOCK_LENGTH, CURRENT_VERSION));
	assert(pblock);
	
	pblock->SetValue(PB_RADIUS,0,crtRadius);
 	}

AlembicSimpleSpline::~AlembicSimpleSpline()
	{
	DeleteAllRefsFromMe();
	pblock = NULL;
	UnReadyInterpParameterBlock();
	}

class AlembicSimpleSplineCreateCallBack: public CreateMouseCallBack {
	AlembicSimpleSpline *ob;
	Point3 p[2];
	IPoint2 sp0;
	Point3 center;
	int createType;
	public:
		int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
		void SetObj(AlembicSimpleSpline *obj) { ob = obj; }
	};

int AlembicSimpleSplineCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
	float r;
#ifdef _3D_CREATE
	DWORD snapdim = SNAP_IN_3D;
#else
	DWORD snapdim = SNAP_IN_PLANE;
#endif

#ifdef _OSNAP
	if (msg == MOUSE_FREEMOVE)
	{
			vpt->SnapPreview(m,m,NULL, snapdim);
	}
#endif
	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
			case 0:
				ob->suspendSnap = TRUE;
				sp0 = m;
				createType = ob->dlgCreateMeth;
				p[0] = vpt->SnapPoint(m,m,NULL,snapdim);
				mat.SetTrans(p[0]); // Set Node's transform
				ob->pblock->SetValue(PB_RADIUS,0,0.01f);
				ob->pmapParam->Invalidate();
				break;
			case 1: 
				p[1] = vpt->SnapPoint(m,m,NULL,snapdim);
				if ( createType ) {	// radius	
					r = Length(p[1]-p[0]);
					center = p[0];
					}
				else {// diameter
					center = (p[0]+p[1]) / 2.0f;
					r = Length(center-p[0]);
					mat.SetTrans(center);  // Modify Node's transform
					}
				ob->pblock->SetValue(PB_RADIUS,0,r);
				ob->pmapParam->Invalidate();
				if (msg==MOUSE_POINT) {
					ob->suspendSnap = FALSE;
					return (Length(m-sp0)<3 || 
						Length(p[1]-p[0])<0.1f) ? CREATE_ABORT: CREATE_STOP;
					}
				break;
			}
		}
	else
	if (msg == MOUSE_ABORT) {
		return CREATE_ABORT;
		}

	return TRUE;
	}

static AlembicSimpleSplineCreateCallBack circleCreateCB;

CreateMouseCallBack* AlembicSimpleSpline::GetCreateMouseCallBack() 
{
    return NULL;

    /*circleCreateCB.SetObj(this);
    return(&circleCreateCB);
    */
}

RefTargetHandle AlembicSimpleSpline::Clone(RemapDir& remap) 
{
    AlembicSimpleSpline* newob = new AlembicSimpleSpline();
    newob->SimpleSplineClone(this, remap); 
    newob->ReplaceReference(USERPBLOCK,remap.CloneRef(pblock));	
    newob->ivalid.SetEmpty();	
    BaseClone(this, newob, remap);
    return(newob);
}

BOOL AlembicSimpleSpline::ValidForDisplay(TimeValue t) 
{
    return TRUE;
    /*float radius;
    pblock->GetValue(PB_RADIUS, t, radius, ivalid);
    return (radius == 0.0f) ? FALSE : TRUE;
    */
}

ParamDimension *AlembicSimpleSpline::GetParameterDim(int pbIndex) 
{
    switch (pbIndex) 
    {
    case PB_RADIUS:
        return stdWorldDim;			
    default:
        return defaultDim;
    }
}

TSTR AlembicSimpleSpline::GetParameterName(int pbIndex) 
{
    switch (pbIndex) 
    {
    case PB_RADIUS:
        return TSTR("Radius");			
    default:
        return TSTR(_T(""));
    }
}

// From ParamArray
BOOL AlembicSimpleSpline::SetValue(int i, TimeValue t, int v) 
{
    switch (i) 
    {
        case PB_CREATEMETHOD: dlgCreateMeth = v; break;
    }		
    return TRUE;
}

BOOL AlembicSimpleSpline::SetValue(int i, TimeValue t, float v)
{
    switch (i) 
    {				
        case PB_TI_RADIUS: crtRadius = v; break;
    }	
    return TRUE;
}

BOOL AlembicSimpleSpline::SetValue(int i, TimeValue t, Point3 &v) 
{
    switch (i) 
    {
        case PB_TI_POS: crtPos = v; break;
    }	

    return TRUE;
}

BOOL AlembicSimpleSpline::GetValue(int i, TimeValue t, int &v, Interval &ivalid) 
{
    switch (i) 
    {
        case PB_CREATEMETHOD: v = dlgCreateMeth; break;
    }
    return TRUE;
}

BOOL AlembicSimpleSpline::GetValue(int i, TimeValue t, float &v, Interval &ivalid) 
{	
    switch (i) {		
        case PB_TI_RADIUS: v = crtRadius; break;
    }
    return TRUE;
}

BOOL AlembicSimpleSpline::GetValue(int i, TimeValue t, Point3 &v, Interval &ivalid) 
{	
    switch (i) {		
        case PB_TI_POS: v = crtPos; break;		
    }
    return TRUE;
}

IOResult AlembicSimpleSpline::Load(ILoad *iload)
{
    iload->RegisterPostLoadCallback(
        new ParamBlockPLCB(versions,NUM_OLDVERSIONS,&curVersion,this,USERPBLOCK));
    return SimpleSpline::Load(iload);
}

void AlembicSimpleSpline::SetAlembicId(const std::string &file, const std::string &identifier)
{
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}

void AlembicImport_FillInShape(alembic_fillshape_options &options)
{
   float masterScaleUnitMeters = (float)GetMasterScale(UNITS_METERS);

   Alembic::AbcGeom::ICurves obj(*options.pIObj,Alembic::Abc::kWrapExisting);

   if(!obj.valid())
   {
      return;
   }

   double sampleTime = GetSecondsFromTimeValue(options.dTicks);

   SampleInfo sampleInfo = getSampleInfo(
      sampleTime,
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );

   // Compute the time interval this fill is good for
   if (sampleInfo.alpha != 0)
   {
       options.validInterval = Interval(options.dTicks, options.dTicks);
   }
   else
   {
       double startSeconds = obj.getSchema().getTimeSampling()->getSampleTime(sampleInfo.floorIndex);
       double endSeconds = obj.getSchema().getTimeSampling()->getSampleTime(sampleInfo.ceilIndex);
       TimeValue start = GetTimeValueFromSeconds(startSeconds);
       TimeValue end = GetTimeValueFromSeconds(endSeconds);
       if (start == 0  && end == 0)
       {
           options.validInterval = FOREVER;
       }
       else
       {
            options.validInterval.Set(start, end);
       }
   }

   Alembic::AbcGeom::ICurvesSchema::Sample curveSample;
   obj.getSchema().get(curveSample,sampleInfo.floorIndex);

   // check for valid curve types...!
   if(curveSample.getType() != Alembic::AbcGeom::ALEMBIC_VERSION_NS::kLinear &&
      curveSample.getType() != Alembic::AbcGeom::ALEMBIC_VERSION_NS::kCubic)
   {
      // Application().LogMessage(L"[ExocortexAlembic] Skipping curve '"+identifier+L"', invalid curve type.",siWarningMsg);
      return;
   }

   if (curveSample.getType() == Alembic::AbcGeom::ALEMBIC_VERSION_NS::kCubic && !options.pBezierShape)
   {
       return;
   }

   if (curveSample.getType() == Alembic::AbcGeom::ALEMBIC_VERSION_NS::kLinear && !options.pPolyShape)
   {
       return;
   }

   Alembic::Abc::Int32ArraySamplePtr curveNbVertices = curveSample.getCurvesNumVertices();
   Alembic::Abc::P3fArraySamplePtr curvePos = curveSample.getPositions();

   // Prepare the knots
   if (options.nDataFillFlags & ALEMBIC_DATAFILL_SPLINE_KNOTS)
   {
       Alembic::Abc::Int32ArraySamplePtr curveNbVertices = curveSample.getCurvesNumVertices();

       if (options.pBezierShape)
       {
           options.pBezierShape->NewShape();

           for (int i = 0; i < curveNbVertices->size(); i += 1)
           {
               Spline3D *pSpline = options.pBezierShape->NewSpline();
               
               if (curveSample.getWrap() == Alembic::AbcGeom::ALEMBIC_VERSION_NS::kPeriodic)
                   pSpline->SetClosed();

               SplineKnot knot;
               int nNumKnots = (curveNbVertices->get()[i]+3-1)/3;
               for (int j = 0; j < nNumKnots; j += 1)
                   pSpline->AddKnot(knot);
           }
       }
       else if (options.pPolyShape)
       {
           options.pPolyShape->NewShape();

           for (int i = 0; i < curveNbVertices->size(); i += 1)
           {
               PolyLine *pLine = options.pPolyShape->NewLine();
               int nNumPoints = curveNbVertices->get()[i];
               pLine->SetNumPts(nNumPoints);
           }
       }
   }

   // Set the control points
   if (options.nDataFillFlags & ALEMBIC_DATAFILL_VERTEX)
   {
       if (options.pBezierShape)
       {
           int nVertexOffset = 0;
           Point3 in, p, out;

           for (int i = 0; i < options.pBezierShape->SplineCount(); i +=1)
           {
               Spline3D *pSpline = options.pBezierShape->GetSpline(i);
               int knots = pSpline->KnotCount();
               int kType;
               for(int ix = 0; ix < knots; ++ix) 
               {
                   if (ix == 0 && !pSpline->Closed())
                   {
                       p = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       out = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       in = p;
                       kType = KTYPE_BEZIER_CORNER;
                   }
                   else if ( ix == knots-1 && !pSpline->Closed())
                   {
                       in = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       p = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       out = p;
                       kType = KTYPE_BEZIER_CORNER;
                   }
                   else
                   {
                       in = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       p = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       out = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters); 
                       nVertexOffset += 1;
                       kType = KTYPE_BEZIER;
                   }

                   pSpline->SetKnot(ix, SplineKnot(kType, LTYPE_CURVE, p, in, out)); 
               }

               pSpline->ComputeBezPoints();
           }
       }
       else if (options.pPolyShape)
       {
           int nVertexOffset = 0;
           Point3 p;

           for (int i = 0; i < options.pPolyShape->numLines; i += 1)
           {
               PolyLine &pLine = options.pPolyShape->lines[i];
               for (int j = 0; j < pLine.numPts; j += 1)
               {
                   p = ConvertAlembicPointToMaxPoint(curvePos->get()[nVertexOffset], masterScaleUnitMeters);
                   nVertexOffset += 1;
                   pLine[j].p = p;
               }
           }
       }
   }

   if (options.pBezierShape)
   {
       options.pBezierShape->UpdateSels();
       options.pBezierShape->InvalidateGeomCache();
   }
   else if (options.pPolyShape)
   {
       options.pPolyShape->UpdateSels();
       options.pPolyShape->InvalidateGeomCache(0);
       options.pPolyShape->InvalidateCapCache();
   }
}


int AlembicImport_Shape(const std::string &file, const std::string &identifier, alembic_importoptions &options)
{
    // Find the object in the archive
	Alembic::AbcGeom::IObject iObj = getObjectFromArchive(file,identifier);
	if(!iObj.valid())
		return alembic_failure;

    Object *newObject = NULL;
    AlembicSimpleSpline *pAlembicSpline = NULL;
    LinearShape *pAlembicShape = NULL;

    if (!Alembic::AbcGeom::ICurves::matches(iObj.getMetaData()))
    {
        return alembic_failure;
    }

    Alembic::AbcGeom::ICurves objCurves = Alembic::AbcGeom::ICurves(iObj, Alembic::Abc::kWrapExisting);
    if (!objCurves.valid())
    {
        return alembic_failure;
    }

    Alembic::AbcGeom::ICurvesSchema::Sample curveSample;
    objCurves.getSchema().get(curveSample, 0);

    if (curveSample.getType() == Alembic::AbcGeom::ALEMBIC_VERSION_NS::kCubic)
    {
        pAlembicSpline = static_cast<AlembicSimpleSpline*>(GetCOREInterface12()->CreateInstance(SHAPE_CLASS_ID, ALEMBIC_SIMPLE_SPLINE_CLASSID));
	    newObject = pAlembicSpline;
    }
    else
    {
        // PeterM : ToDo: Fill in an alembic linear shape class
        pAlembicShape = 0;
    }

    if (newObject == NULL)
    {
        return alembic_failure;
    }

    // Set the update data fill flags
    unsigned int nDataFillFlags = ALEMBIC_DATAFILL_VERTEX|ALEMBIC_DATAFILL_SPLINE_KNOTS;
    nDataFillFlags |= options.importBboxes ? ALEMBIC_DATAFILL_BOUNDINGBOX : 0;

    // Fill in the alembic object
    if (pAlembicSpline)
    {
        pAlembicSpline->SetAlembicId(file, identifier);
	    pAlembicSpline->SetAlembicUpdateDataFillFlags(nDataFillFlags);
    }
    else if (pAlembicShape)
    {

    }

	// Create the object node
	INode *node = GetCOREInterface12()->CreateObjectNode(newObject, iObj.getName().c_str());

	if (!node)
    {
        ALEMBIC_SAFE_DELETE(pAlembicSpline);
        ALEMBIC_SAFE_DELETE(pAlembicShape);
		return alembic_failure;
    }

    // Add the new inode to our current scene list
    SceneEntry *pEntry = options.sceneEnumProc.Append(node, newObject, OBTYPE_CURVES, &std::string(iObj.getFullName())); 
    options.currentSceneList.Append(pEntry);

    // Set the visibility controller
    AlembicImport_SetupVisControl(iObj, node, options);

	return 0;
}
