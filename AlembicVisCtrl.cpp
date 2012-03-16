#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicDefinitions.h"
#include "AlembicVisCtrl.h"
#include "Utility.h"
#include "dummy.h"
#include <ILockedTracks.h>
#include "iparamb2.h"
#include "AlembicNames.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// alembic_fillvis_options
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _alembic_fillvis_options
{
    Alembic::AbcGeom::IObject *pIObj;
    TimeValue dTicks;
    bool bVisibility;
    bool bOldVisibility;

    _alembic_fillvis_options()
    {
        pIObj = 0;
        dTicks = 0;
        bVisibility = false;
        bOldVisibility = false;
    }
} alembic_fillvis_options;

void AlembicImport_FillInVis(alembic_fillvis_options &options);

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicVisCtrl Declaration
///////////////////////////////////////////////////////////////////////////////////////////////////
class AlembicVisCtrl : public LockableStdControl
{
public:
    IParamBlock2* pblock;
    static IObjParam *ip;
    static AlembicVisCtrl *editMod;
public:
	AlembicVisCtrl();
	~AlembicVisCtrl();

	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }

    void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic Visibility"); }  
	virtual Class_ID ClassID() { return ALEMBIC_VIS_CTRL_CLASSID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Alembic Visibility"); }

    void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
    void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);

    int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
    IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
    IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	int NumRefs() { return 1; }
	void SetReference(int i, ReferenceTarget* pTarget);
	ReferenceTarget* GetReference(int i);
	RefResult NotifyRefChanged(Interval, RefTargetHandle, PartID&, RefMessage);

    int NumSubs()  {return 1;} //because it uses the paramblock
    Animatable* SubAnim(int i) {return GetReference(i);}
    TSTR SubAnimName(int i) { return _T("Parameters"); }
    int SubNumToRefNum(int subNum) {if (subNum==0) return 0; else return -1;}

    void Copy(Control* pFrom){}
    virtual int IsKeyable() { return 0; }
    virtual BOOL IsLeaf() { return TRUE; }
    virtual BOOL IsReplaceable() { return FALSE; }

    virtual void  GetValueLocalTime(TimeValue t, void *ptr, Interval &valid, GetSetMethod method = CTRL_ABSOLUTE);
    virtual void  SetValueLocalTime(TimeValue t, void *ptr, int commit, GetSetMethod method = CTRL_ABSOLUTE);
    virtual void  Extrapolate(Interval range, TimeValue t, void *val, Interval &valid, int type);     
    virtual void* CreateTempValue();    
    virtual void  DeleteTempValue(void *val);
    virtual void  ApplyValue(void *val, void *delta);
    virtual void  MultiplyValue(void *val, float m); 

    IOResult Save(ISave *isave);
    IOResult Load(ILoad *iload);

    void SetAlembicId(const std::string &file, const std::string &identifier, TimeValue t);
    const std::string &GetAlembicArchive() { return m_AlembicNodeProps.m_File; }
    const std::string &GetAlembicObjectId() { return m_AlembicNodeProps.m_Identifier; }

private:
    alembic_nodeprops m_AlembicNodeProps;
    bool m_bOldVisibility;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicVisCtrlClassDesc
///////////////////////////////////////////////////////////////////////////////////////////////////
class AlembicVisCtrlClassDesc : public ClassDesc2
{
public:
	AlembicVisCtrlClassDesc() {};
	~AlembicVisCtrlClassDesc() {};

	// ClassDesc methods.  
	// Max calls these functions to figure out what kind of plugin this class represents

	// Return TRUE if the user can create this plug-in.
	int			IsPublic()			{ return TRUE; }	// We do want the user to see this plug-in

	// Return the class name of this plug-in
	const MCHAR* ClassName()		{ static const MSTR str("Alembic Visibility"); return str; }

	// Return the SuperClassID - this ID should
	// match the implementation of the interface returned
	// by Create.
	SClass_ID	SuperClassID()		{ return CTRL_FLOAT_CLASS_ID; }

	// Return the unique ID that identifies this class
	// This is required when saving.  Max stores the ClassID
	// reported by the actual plugin, and on reload it recreates
	// the appropriate class by matching the stored ClassID with
	// the matching ClassDesc
	//
	// You can generate random ClassID's using the gencid program
	// supplied with the Max SDK
	Class_ID	ClassID()			{ return ALEMBIC_VIS_CTRL_CLASSID; }

	// If the plugin is an Object or Texture, this function returns
	// the category it can be assigned to.
	const MCHAR* Category()			{ return EXOCORTEX_ALEMBIC_CATEGORY; }

	// Return an instance of this plug-in.  Max will call this function
	// when it wants to start using our plug-in
	void* Create(BOOL loading=FALSE)
	{
		return new AlembicVisCtrl;
	}

    const TCHAR*	InternalName() { return _T("AlembicVisController"); }	// returns fixed parsable name (scripter-visible name)
    HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

// This function returns a pointer to a class descriptor for our Utility
// This is the function that informs max that our plug-in exists and is 
// available to use
static AlembicVisCtrlClassDesc sAlembicVisCtrlClassDesc;
ClassDesc2* GetAlembicVisCtrlClassDesc()
{
	return &sAlembicVisCtrlClassDesc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Alembic_vis_Ctrl_Param_Blk
///////////////////////////////////////////////////////////////////////////////////////////////////
enum 
{ 
    AlembicVisCtrl_params 
};

// Alembic vis params
enum 
{ 
	alembicvis_preview_abc_archive,
    alembicvis_preview_abc_id,
};

class PBVis_Accessor : public PBAccessor
{
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		AlembicVisCtrl *visCtrl = (AlembicVisCtrl*) owner;
		switch(id)
		{
        case alembicvis_preview_abc_archive:
            {
                const char *strArchive = visCtrl->GetAlembicArchive().c_str();
                visCtrl->GetParamBlock(AlembicVisCtrl_params)->SetValue(alembicvis_preview_abc_archive, t, strArchive);
            }
            break;
        case alembicvis_preview_abc_id:
            {
                const char *strObjectId = visCtrl->GetAlembicObjectId().c_str();
                visCtrl->GetParamBlock(AlembicVisCtrl_params)->SetValue(alembicvis_preview_abc_id, t, strObjectId);
            }
            break;
		default: 
            break;
		}

		GetCOREInterface12()->RedrawViews(GetCOREInterface()->GetTime());
	}

    void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		AlembicVisCtrl *visCtrl = (AlembicVisCtrl*) owner;
		switch(id)
		{
        case alembicvis_preview_abc_archive:
            {
                const char *strArchive = visCtrl->GetAlembicArchive().c_str();
                visCtrl->GetParamBlock(AlembicVisCtrl_params)->SetValue(alembicvis_preview_abc_archive, t, strArchive);
            }
            break;
        case alembicvis_preview_abc_id:
            {
                const char *strObjectId = visCtrl->GetAlembicObjectId().c_str();
                visCtrl->GetParamBlock(AlembicVisCtrl_params)->SetValue(alembicvis_preview_abc_id, t, strObjectId);
            }
            break;
		default: 
            break;
		}

		GetCOREInterface12()->RedrawViews(GetCOREInterface()->GetTime());
	}
};

static PBVis_Accessor sVis_PBAccessor;

static ParamBlockDesc2 vis_params_desc ( AlembicVisCtrl_params, _T("ExoCortexAlembicvisController"),
									IDS_PREVIEW, &sAlembicVisCtrlClassDesc,
									P_AUTO_CONSTRUCT | P_AUTO_UI, 0,
	//rollout description
	IDD_ALEMBIC_ID_PARAMS, IDS_PREVIEW, 0, 0, NULL,

    // params
	/*polymesh_preview_abc_archive, _T("previewAbcArchive"), TYPE_FILENAME, 0, IDS_ABC_ARCHIVE,
		p_ui, TYPE_FILEOPENBUTTON, IDC_ABC_ARCHIVE,
        p_caption, IDS_OPEN_ABC_CAPTION,
        p_file_types, IDS_ABC_FILE_TYPE,
        p_accessor,		&polymesh_preview_accessor,
		end,
        */

    alembicvis_preview_abc_archive, _T("previewAbcArchive"), TYPE_STRING, 0, IDS_ABC_ARCHIVE,
        p_ui, TYPE_EDITBOX, IDC_ABC_ARCHIVE,
        p_accessor,		&sVis_PBAccessor,
        end,

	alembicvis_preview_abc_id, _T("previewAbcId"), TYPE_STRING, 0, IDS_ABC_ID,
		p_ui, TYPE_EDITBOX, IDC_ABC_OBJECTID,
        p_accessor,		&sVis_PBAccessor,
		end,
	end
);

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicVisCtrl Methods
///////////////////////////////////////////////////////////////////////////////////////////////////
IObjParam *AlembicVisCtrl::ip = NULL;
AlembicVisCtrl *AlembicVisCtrl::editMod = NULL;

void AlembicVisCtrl::GetValueLocalTime(TimeValue t, void *ptr, Interval &valid, GetSetMethod method)
{
    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(m_AlembicNodeProps.m_File, m_AlembicNodeProps.m_Identifier);
    
    if(!iObj.valid())
        return;

    alembic_fillvis_options visOptions;
    visOptions.pIObj = &iObj;
    visOptions.dTicks = t;
    visOptions.bOldVisibility = m_bOldVisibility;
    AlembicImport_FillInVis(visOptions);

    float fBool = visOptions.bVisibility ? 1.0f : 0.0f;
    m_bOldVisibility = visOptions.bVisibility;

	valid.Set(t,t);

	if (method == CTRL_ABSOLUTE)
	{
		float* fInVal = (float*)ptr;
		*fInVal = fBool;
	}
	else // CTRL_RELATIVE
	{
		float* fInVal = (float*)ptr;
		*fInVal = fBool * (*fInVal);
	}
}

AlembicVisCtrl::AlembicVisCtrl()
{
    pblock = NULL;
    sAlembicVisCtrlClassDesc.MakeAutoParamBlocks(this);
    m_bOldVisibility = true;
}

AlembicVisCtrl::~AlembicVisCtrl()
{
}

RefTargetHandle AlembicVisCtrl::Clone(RemapDir& remap) 
{
	AlembicVisCtrl *ctrl = new AlembicVisCtrl();
    ctrl->ReplaceReference (0, remap.CloneRef(pblock));
	
    BaseClone(this, ctrl, remap);
	return ctrl;
}

void AlembicVisCtrl::SetValueLocalTime(TimeValue t, void *ptr, int commit, GetSetMethod method)
{
}

void* AlembicVisCtrl::CreateTempValue()
{
    return new float;
}

void AlembicVisCtrl::DeleteTempValue(void *val)
{
    delete (float*)val;
}

void AlembicVisCtrl::ApplyValue(void *val, void *delta)
{
    float &fdelta = *((float*)delta);
    float &fval = *((float*)val);
    fval = fdelta * fval;
}

void AlembicVisCtrl::MultiplyValue(void *val, float m)
{
     float *fVal = (float*)val;
     *fVal = (*fVal) * m;
}

void AlembicVisCtrl::Extrapolate(Interval range, TimeValue t, void *val, Interval &valid, int type)
{
}

void AlembicVisCtrl::SetAlembicId(const std::string &file, const std::string &identifier, TimeValue t)
{
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}

#define LOCK_CHUNK		0x2535  //the lock value
IOResult AlembicVisCtrl::Save(ISave *isave)
{
	Control::Save(isave);

	// note: if you add chunks, it must follow the LOCK_CHUNK chunk due to Renoir error in 
	// placement of Control::Save(isave);
	ULONG nb;
	int on = (mLocked==true) ? 1 :0;
	isave->BeginChunk(LOCK_CHUNK);
	isave->Write(&on,sizeof(on),&nb);	
	isave->EndChunk();	

	return IO_OK;
}

IOResult AlembicVisCtrl::Load(ILoad *iload)
{
	ULONG nb;
	IOResult res;

	res = Control::Load(iload);
	if (res!=IO_OK)  return res;

	// We can't do the standard 'while' loop of opening chunks and checking ID
	// since that will eat the Control ORT chunks that were saved improperly in Renoir
	USHORT next = iload->PeekNextChunkID();
	if (next == LOCK_CHUNK) 
	{
		iload->OpenChunk();
		int on;
		res=iload->Read(&on,sizeof(on),&nb);
		if(on)
			mLocked = true;
		else
			mLocked = false;
		iload->CloseChunk();
		if (res!=IO_OK)  return res;
	}

	// Only do anything if this is the control base classes chunk
	next = iload->PeekNextChunkID();
	if (next == CONTROLBASE_CHUNK) 
		res = Control::Load(iload);  // handle improper Renoir Save order
	return res;	
}

RefTargetHandle AlembicVisCtrl::GetReference(int i)
{
    switch (i)
    {
    case 0:
        return pblock;
    }

    return NULL;
}

void AlembicVisCtrl::SetReference(int i, RefTargetHandle rtarg)
{
    switch (i)
    {
    case 0:
        pblock = (IParamBlock2*)rtarg; 
        break;
    }
}

RefResult AlembicVisCtrl::NotifyRefChanged(
    Interval iv, 
    RefTargetHandle hTarg, 
    PartID& partID, 
    RefMessage msg) 
{
    switch (msg) 
    {
        case REFMSG_CHANGE:
            if (editMod != this) 
            {
                break;
            }

            if (hTarg == pblock) 
            {
                ParamID changing_param = pblock->LastNotifyParamID();
                vis_params_desc.InvalidateUI(changing_param);
            }
            break;

        case REFMSG_OBJECT_CACHE_DUMPED:
            return REF_STOP;
            break;
    }

    return REF_SUCCEED;
}

void AlembicVisCtrl::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;
    editMod  = this;

	sAlembicVisCtrlClassDesc.BeginEditParams(ip, this, flags, prev);

    // Necessary?
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void AlembicVisCtrl::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
	sAlembicVisCtrlClassDesc.EndEditParams(ip, this, flags, next);

	this->ip = NULL;
    editMod  = NULL;
}



void AlembicImport_FillInVis_Internal(alembic_fillvis_options &options);

void AlembicImport_FillInVis(alembic_fillvis_options &options)
{
	ESS_STRUCTURED_EXCEPTION_REPORTING_START
		AlembicImport_FillInVis_Internal( options );
	ESS_STRUCTURED_EXCEPTION_REPORTING_END
}

void AlembicImport_FillInVis_Internal(alembic_fillvis_options &options)
{
    if(!options.pIObj->valid())
    {
        options.bVisibility = options.bOldVisibility;
        return;
    }

    Alembic::AbcGeom::IVisibilityProperty visibilityProperty = 
        Alembic::AbcGeom::GetVisibilityProperty(*options.pIObj);
    
    if(!visibilityProperty.valid())
    {
        options.bVisibility = options.bOldVisibility;
        return;
    }

    double sampleTime = GetSecondsFromTimeValue(options.dTicks); 
    SampleInfo sampleInfo = getSampleInfo(
        sampleTime,
        getTimeSamplingFromObject(*options.pIObj),
        visibilityProperty.getNumSamples()
        );

    boost::int8_t rawVisibilityValue = visibilityProperty.getValue ( sampleInfo.floorIndex );
    Alembic::AbcGeom::ObjectVisibility visibilityValue = Alembic::AbcGeom::ObjectVisibility ( rawVisibilityValue );

    switch(visibilityValue)
    {
    case Alembic::AbcGeom::kVisibilityVisible:
        {
            options.bVisibility = true;
            break;
        }
    case Alembic::AbcGeom::kVisibilityHidden:
        {
            options.bVisibility = false;
            break;
        }
    default:
        {
            options.bVisibility = options.bOldVisibility;
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicImport_vis
///////////////////////////////////////////////////////////////////////////////////////////////////
void AlembicImport_SetupVisControl( Alembic::AbcGeom::IObject &obj, INode *pNode, alembic_importoptions &options )
{
    if (!pNode)
        return;

    if (options.importVisibility == VisImport_JustImportValue)
    {
        alembic_fillvis_options visFillOptions;
        visFillOptions.pIObj = &obj;
        visFillOptions.dTicks = 0;
        visFillOptions.bOldVisibility = true;
        AlembicImport_FillInVis(visFillOptions);
        BOOL bVis = visFillOptions.bVisibility?TRUE:FALSE;
        float fBool = bVis ? 1.0f : 0.0f;
        pNode->SetVisibility(0, fBool);
    }
    else if (options.importVisibility == VisImport_ConnectedControllers)
    {
        // Create the xform modifier
        AlembicVisCtrl *pCtrl = static_cast<AlembicVisCtrl*>
            (GetCOREInterface()->CreateInstance(CTRL_FLOAT_CLASS_ID, ALEMBIC_VIS_CTRL_CLASSID));

        // Set the alembic id
        TimeValue t = GetCOREInterface12()->GetTime();
        pCtrl->SetAlembicId(obj.getArchive().getName(), obj.getFullName(), t);

        // Add the modifier to the node
        pNode->SetVisController(pCtrl);
    }
}

