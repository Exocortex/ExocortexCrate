#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicDefinitions.h"
#include "AlembicSplineTopoModifier.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include "AlembicXForm.h"
#include "AlembicVisibilityController.h"

using namespace MaxSDK::AssetManagement;

IObjParam *AlembicSplineTopoModifier::ip              = NULL;
AlembicSplineTopoModifier *AlembicSplineTopoModifier::editMod         = NULL;

static AlembicSplineTopoModifierClassDesc AlembicSplineTopoModifierDesc;
ClassDesc2* GetAlembicSplineTopoModifierClassDesc() {return &AlembicSplineTopoModifierDesc;}

//--- Properties block -------------------------------

const int ALEMBIC_SPLINE_TOPO_MODIFIER_VERSION = 1;

static ParamBlockDesc2 AlembicSplineTopoModifierParams(
	0,
	_T(ALEMBIC_SPLINE_TOPO_MODIFIER_SCRIPTNAME),
	0,
	GetAlembicSplineTopoModifierClassDesc(),
	P_AUTO_CONSTRUCT | P_AUTO_UI | P_VERSION,
	ALEMBIC_SPLINE_TOPO_MODIFIER_VERSION,
	0,

	// rollout description 
	IDD_ALEMBIC_SPLINE_TOPO_PARAMS, IDS_ALEMBIC, 0, 0, NULL,

    // params
	AlembicSplineTopoModifier::ID_PATH, _T("path"), TYPE_FILENAME, P_RESET_DEFAULT, IDS_PATH,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_PATH_EDIT,
		p_assetTypeID,	kExternalLink,
		p_end,
        
	AlembicSplineTopoModifier::ID_IDENTIFIER, _T("identifier"), TYPE_STRING, P_RESET_DEFAULT, IDS_IDENTIFIER,
	    p_default, "",
	    p_ui,        TYPE_EDITBOX,		IDC_IDENTIFIER_EDIT,
		p_end,

	AlembicSplineTopoModifier::ID_TIME, _T("time"), TYPE_FLOAT, P_ANIMATABLE, IDS_TIME,
		p_default,       0.0f,
		p_range,         0.0f, 1000.0f,
		p_ui,            TYPE_SPINNER,       EDITTYPE_FLOAT, IDC_TIME_EDIT,    IDC_TIME_SPIN, 0.01f,
		p_end,

	AlembicSplineTopoModifier::ID_GEOMETRY, _T("geometry"), TYPE_BOOL, P_ANIMATABLE, IDS_GEOMETRY,
		p_default,       FALSE,
		p_end,

	/*AlembicSplineTopoModifier::ID_NORMALS, _T("normals"), TYPE_BOOL, P_ANIMATABLE, IDS_NORMALS,
		p_default,       FALSE,
		p_end,

	AlembicSplineTopoModifier::ID_UVS, _T("uvs"), TYPE_BOOL, P_ANIMATABLE, IDS_UVS,
		p_default,       FALSE,
		p_end,*/
		
	AlembicSplineTopoModifier::ID_MUTED, _T("muted"), TYPE_BOOL, P_ANIMATABLE, IDS_MUTED,
		p_default,       TRUE,
		p_ui,            TYPE_SINGLECHEKBOX,  IDC_MUTED_CHECKBOX,
		p_end,

	AlembicSplineTopoModifier::ID_IGNORE_SUBFRAME_SAMPLES, _T("Ignore subframe samples"), TYPE_BOOL, P_ANIMATABLE, IDS_MUTED,
		p_default,       FALSE,
		p_ui,            TYPE_SINGLECHEKBOX,  IDC_IGNORE_SUBFRAME_SAMPLES,
		p_end,

	p_end
);

//--- Modifier methods -------------------------------

AlembicSplineTopoModifier::AlembicSplineTopoModifier() 
{
    pblock = NULL;
    m_CachedAbcFile = "";

	GetAlembicSplineTopoModifierClassDesc()->MakeAutoParamBlocks(this);
}

AlembicSplineTopoModifier::~AlembicSplineTopoModifier() 
{
     delRefArchive(m_CachedAbcFile);
}

RefTargetHandle AlembicSplineTopoModifier::Clone(RemapDir& remap) 
{
	AlembicSplineTopoModifier *mod = new AlembicSplineTopoModifier();

    mod->ReplaceReference (0, remap.CloneRef(pblock));
	
    BaseClone(this, mod, remap);
	return mod;
}


void AlembicSplineTopoModifier::EnumAuxFiles(AssetEnumCallback& nameEnum, DWORD flags)  {
	if ((flags&FILE_ENUM_CHECK_AWORK1)&&TestAFlag(A_WORK1)) return; // LAM - 4/11/03

	if (!(flags&FILE_ENUM_INACTIVE)) return; // not needed by renderer

	if(flags & FILE_ENUM_ACCESSOR_INTERFACE)	{
		IEnumAuxAssetsCallback* callback = static_cast<IEnumAuxAssetsCallback*>(&nameEnum);
		callback->DeclareAsset(AlembicPathAccessor(this));		
	}
	//else {
	//	IPathConfigMgr::GetPathConfigMgr()->RecordInputAsset( this->GetParamBlockByID( 0 )->GetAssetUser( GetParamIdByName( this, 0, "path" ), 0 ), nameEnum, flags);
	//}

	ReferenceTarget::EnumAuxFiles(nameEnum, flags);
} 

void AlembicSplineTopoModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
	ESS_CPP_EXCEPTION_REPORTING_START

	Interval interval = FOREVER;//os->obj->ObjectValidity(t);
	//ESS_LOG_INFO( "Interval Start: " << interval.Start() << " End: " << interval.End() );

    MCHAR const* strPath = NULL;
	this->pblock->GetValue( AlembicSplineTopoModifier::ID_PATH, t, strPath, interval);

	MCHAR const* strIdentifier = NULL;
	this->pblock->GetValue( AlembicSplineTopoModifier::ID_IDENTIFIER, t, strIdentifier, interval);
 
	float fTime;
	this->pblock->GetValue( AlembicSplineTopoModifier::ID_TIME, t, fTime, interval);

	BOOL bTopology = true;
	float fGeoAlpha = 1.0f;

	BOOL bGeometry;
	this->pblock->GetValue( AlembicSplineTopoModifier::ID_GEOMETRY, t, bGeometry, interval);

	/*BOOL bNormals;
	this->pblock->GetValue( AlembicSplineTopoModifier::ID_NORMALS, t, bNormals, interval);

	BOOL bUVs;
	this->pblock->GetValue( AlembicSplineTopoModifier::ID_UVS, t, bUVs, interval);*/


	BOOL bMuted;
	this->pblock->GetValue( AlembicSplineTopoModifier::ID_MUTED, t, bMuted, interval);
	
	BOOL bIgnoreSubframeSamples;
	this->pblock->GetValue( AlembicSplineTopoModifier::ID_IGNORE_SUBFRAME_SAMPLES, t, bIgnoreSubframeSamples, interval);

	//ESS_LOG_INFO( "AlembicSplineTopoModifier::ModifyObject strPath: " << strPath << " strIdentifier: " << strIdentifier << " fTime: " << fTime << 
	//	" bTopology: " << bTopology << " bGeometry: " << bGeometry << " bNormals: " << bNormals << " bUVs: " << bUVs << " bMuted: " << bMuted );

	if( bMuted ) {
		return;
	}

	if( strlen( strPath ) == 0 ) {
	   ESS_LOG_ERROR( "No filename specified." );
	   return;
	}
	if( strlen( strIdentifier ) == 0 ) {
	   ESS_LOG_ERROR( "No path specified." );
	   return;
	}

	if( ! fs::exists( strPath ) ) {
		ESS_LOG_ERROR( "Can't file Alembic file.  Path: " << strPath );
		return;
	}

	Alembic::AbcGeom::IObject iObj;
	try {
		iObj = getObjectFromArchive(strPath, strIdentifier);
	} catch( std::exception exp ) {
		ESS_LOG_ERROR( "Can not open Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier << " reason: " << exp.what() );
		return;
	}

	if(!iObj.valid()) {
		ESS_LOG_ERROR( "Not a valid Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier );
		return;
	}

	ShapeObject *shape = (ShapeObject *)os->obj;

	BezierShape bezierShape;
	PolyShape polyShape;
	
	alembic_fillshape_options options;
    options.pIObj =  &iObj;
	options.pShapeObject = shape;
    options.dTicks = GetTimeValueFromSeconds( fTime );
  	options.nDataFillFlags = 0;
    options.nDataFillFlags |= ALEMBIC_DATAFILL_SPLINE_KNOTS;
	if( bGeometry ) {
		options.nDataFillFlags |= ALEMBIC_DATAFILL_VERTEX;
		options.nDataFillFlags |= ALEMBIC_DATAFILL_BOUNDINGBOX;
	}
 
	if(bIgnoreSubframeSamples){
		options.nDataFillFlags |= ALEMBIC_DATAFILL_IGNORE_SUBFRAME_SAMPLES;
	}

	SClass_ID superClassID = os->obj->SuperClassID();
	Class_ID classID = os->obj->ClassID();
	if( superClassID != SHAPE_CLASS_ID ) {
		ESS_LOG_ERROR( "Can not convert internal object data into a ShapeObject, confused. (1)" );
		return;
	}
	
	if (classID == Class_ID(SPLINESHAPE_CLASS_ID,0)) {
		SplineShape *pSplineShape = (SplineShape*) os->obj;
		options.pBezierShape = &pSplineShape->shape;
	}
	else if (classID == Class_ID(LINEARSHAPE_CLASS_ID,0)){
		LinearShape *pLinearShape = (LinearShape*) os->obj;
		options.pPolyShape = &pLinearShape->shape;
	}
	else {
		ESS_LOG_ERROR( "Can not convert internal object data into a ShapeObject, confused. (2)" );
	}

   try {
		AlembicImport_FillInShape(options);
   }
   catch(std::exception exp ) {
		ESS_LOG_ERROR( "Error reading shape from Alembic data stream.  Path: " << strPath << " identifier: " << strIdentifier << " reason: " << exp.what() );
		return;
   }

  // update the validity channel
    if( bTopology ) {
		os->obj->UpdateValidity(TOPO_CHAN_NUM, interval);
		os->obj->UpdateValidity(GEOM_CHAN_NUM, interval);
	}
    if( bGeometry ) {
		os->obj->UpdateValidity(GEOM_CHAN_NUM, interval);
	}

   	ESS_CPP_EXCEPTION_REPORTING_END
}

RefResult AlembicSplineTopoModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message) {
	ESS_CPP_EXCEPTION_REPORTING_START

    switch (message) 
    {
        case REFMSG_CHANGE:
            if (hTarget == pblock) 
            {
                ParamID changing_param = pblock->LastNotifyParamID();
                switch(changing_param)
                {
                case ID_PATH:
                    {
                        delRefArchive(m_CachedAbcFile);
                        MCHAR const* strPath = NULL;
                        TimeValue t = GetCOREInterface()->GetTime();
                        pblock->GetValue( AlembicSplineTopoModifier::ID_PATH, t, strPath, changeInt);
                        m_CachedAbcFile = strPath;
                        addRefArchive(m_CachedAbcFile);
                    }
                    break;
                default:
                    break;
                }

                AlembicSplineTopoModifierParams.InvalidateUI(changing_param);
            }
            break;
 
    case REFMSG_WANT_SHOWPARAMLEVEL:

        break;
	}

	ESS_CPP_EXCEPTION_REPORTING_END

	return REF_SUCCEED;
}


void AlembicSplineTopoModifier::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;
    editMod  = this;

	AlembicSplineTopoModifierDesc.BeginEditParams(ip, this, flags, prev);
}

void AlembicSplineTopoModifier::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
	AlembicSplineTopoModifierDesc.EndEditParams(ip, this, flags, next);

	this->ip = NULL;
    editMod  = NULL;
}