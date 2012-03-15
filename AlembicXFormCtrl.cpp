#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicDefinitions.h"
#include "AlembicXFormCtrl.h"
#include "Utility.h"
#include "dummy.h"
#include <ILockedTracks.h>
#include "iparamb2.h"
#include <iparamm2.h>

#include "AlembicTransformUtilities.h"

// This function returns a pointer to a class descriptor for our Utility
// This is the function that informs max that our plug-in exists and is 
// available to use
static AlembicXFormCtrlClassDesc sAlembicXFormCtrlClassDesc;
ClassDesc2* GetAlembicXFormCtrlClassDesc()
{
	return &sAlembicXFormCtrlClassDesc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Alembic_XForm_Ctrl_Param_Blk
///////////////////////////////////////////////////////////////////////////////////////////////////
enum 
{ 
    AlembicXFormCtrl_params 
};

// Alembic xform params
enum 
{ 
	alembicxform_abc_archive,
    alembicxform_abc_id,
    alembicxform_timeoffset,
    alembicxform_timescale,
    alembicxform_timehidden,
    alembicxform_muted,
    alembicxform_cameratransform,
};

static ParamBlockDesc2 xform_params_desc ( AlembicXFormCtrl_params, _T("ExoCortexAlembicXFormController"),
									IDS_PREVIEW, &sAlembicXFormCtrlClassDesc,
									P_AUTO_CONSTRUCT | P_AUTO_UI, 0,
	//rollout description
	IDD_ALEMBIC_ID_PARAMS, IDS_PREVIEW, 0, 0, NULL,

    // params
	/*alembicxform_abc_archive, _T("path"), TYPE_FILENAME, 0, IDS_ABC_ARCHIVE,
		p_ui, TYPE_FILEOPENBUTTON, IDC_ABC_ARCHIVE,
        p_caption, IDS_OPEN_ABC_CAPTION,
        p_file_types, IDS_ABC_FILE_TYPE,
        p_accessor,		&sXForm_PBAccessor,
		end,
        */

    alembicxform_abc_archive, _T("path"), TYPE_STRING, P_RESET_DEFAULT, IDS_ABC_ARCHIVE,
        p_ui, TYPE_EDITBOX, IDC_ABC_ARCHIVE,
        end,

	alembicxform_abc_id, _T("identifier"), TYPE_STRING, P_RESET_DEFAULT, IDS_ABC_ID,
		p_ui, TYPE_EDITBOX, IDC_ABC_OBJECTID,
		end,

    alembicxform_timeoffset, _T("timeOffset"), TYPE_FLOAT, P_ANIMATABLE, IDS_TIMEOFFSET,
        p_default,	0.0f,
		p_ui,		TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_TIME_OFFSET, IDC_SPIN_TIME_OFFSET, 0.1f,
        end,

    alembicxform_timescale, _T("timeScale"), TYPE_FLOAT, P_ANIMATABLE, IDS_TIMESCALE,
        p_default,	1.0f,
		p_ui,		TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_TIME_SCALE, IDC_SPIN_TIME_SCALE, 0.1f,
        end,

    alembicxform_muted, _T("muted"), TYPE_BOOL, P_ANIMATABLE, IDS_MUTED,
        p_default, FALSE,
		p_ui, TYPE_SINGLECHEKBOX, IDC_CHECK_MUTED,
		end,

    alembicxform_timehidden, _T("currentTimeHidden"), TYPE_FLOAT, 0, IDS_CURRENTTIMEHIDDEN,
        end,

    alembicxform_cameratransform, _T("isCameraTransform"), TYPE_BOOL, 0,  IDS_CAMERATRANSFORM, 
        end,
	end
);
/*
class AlembicXFormCtrlDlgProc : public ParamMap2UserDlgProc {
  public:
	AlembicXFormCtrl* mpParent;
    char *m_Buffer;
	BOOL initialized; //set to true after an init dialog message
	AlembicXFormCtrlDlgProc( AlembicXFormCtrl* parent ) { this->mpParent=parent; initialized=FALSE; m_Buffer = 0; }
    ~AlembicXFormCtrlDlgProc() { if (m_Buffer) free(m_Buffer); }
	void DeleteThis() { delete this; }

	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) 
        {
		case WM_INITDIALOG: //called after BeginEditParams whenever rollout is displayed
			mpParent->MainPanelInitDialog(hWnd);
            mpParent->MainPanelUpdateUI();
			initialized = TRUE;
			break;
		case WM_DESTROY: //called from EndEditParams
			mpParent->MainPanelDestroy(hWnd);
			initialized = FALSE;
			break;
        case WM_COMMAND:
            {
                switch( LOWORD(wParam) ) 
                {
                case IDC_ABC_ARCHIVE:
                case IDC_ABC_OBJECTID:
                    {
                        switch(HIWORD(wParam)) 
                        {
                        case EN_SETFOCUS:
                            DisableAccelerators();					
                            break;
                        case EN_KILLFOCUS:
                            EnableAccelerators();
                            break;
                        case EN_CHANGE:
                            {
                                int nDlgItem = LOWORD(wParam);
                                HWND hDlgWnd = GetDlgItem(hWnd, nDlgItem);
                                char text[MAX_PATH] = "\0";
                                int nLength = Edit_GetTextLength(hDlgWnd);
                                nLength += 1;
                                if (m_Buffer == 0 || strlen(m_Buffer) < nLength)
                                    m_Buffer = (char*)realloc(m_Buffer, nLength);

                                Edit_GetText(hDlgWnd, m_Buffer, nLength);
                                int nParamId = nDlgItem == IDC_ABC_ARCHIVE ? alembicxform_abc_archive : alembicxform_abc_id;
                                TimeValue t = GetCOREInterface12()->GetTime();
                                mpParent->pblock->SetValue(nParamId, t, m_Buffer);
                            }
                            break;
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            break;
		default:
            return FALSE;
	  }
	  return TRUE;
	}
};
*/

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicXFormCtrl Methods
///////////////////////////////////////////////////////////////////////////////////////////////////
IObjParam *AlembicXFormCtrl::ip = NULL;
AlembicXFormCtrl *AlembicXFormCtrl::editMod = NULL;

void AlembicXFormCtrl::GetValueLocalTime(TimeValue t, void *ptr, Interval &valid, GetSetMethod method)
{
    MCHAR const* strPath = NULL;
	pblock->GetValue( alembicxform_abc_archive, t, strPath, valid);

    MCHAR const* strIdentifier = NULL;
	pblock->GetValue( alembicxform_abc_id, t, strIdentifier, valid);

    BOOL bIsCameraTransform;
    pblock->GetValue( alembicxform_cameratransform, t, bIsCameraTransform, valid);

    float fCurrentTimeHidden;
	pblock->GetValue( alembicxform_timehidden, t, fCurrentTimeHidden, valid);

	float fTimeOffset;
	pblock->GetValue( alembicxform_timeoffset, t, fTimeOffset, valid);

	float fTimeScale;
	pblock->GetValue( alembicxform_timescale, t, fTimeScale, valid); 

    BOOL bMuted;
	pblock->GetValue( alembicxform_muted, t, bMuted, valid);

    if (bMuted)
    {
        return;
    }

    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(strPath, strIdentifier);

    if(!iObj.valid())
    {
        return;
    }

    double dataTime = GetSecondsFromTimeValue(t);
    dataTime = fTimeOffset + dataTime * fTimeScale;

    alembic_fillxform_options xformOptions;
    xformOptions.pIObj = &iObj;
    xformOptions.dTicks = GetTimeValueFromSeconds(dataTime);
    xformOptions.bIsCameraTransform = bIsCameraTransform?true:false;
    AlembicImport_FillInXForm(xformOptions);

    Interval alembicInterval(t,t);
    valid = valid & alembicInterval;

	if (method == CTRL_ABSOLUTE)
	{
		Matrix3* m3InVal = (Matrix3*)ptr;
		*m3InVal = xformOptions.maxMatrix;
	}
	else // CTRL_RELATIVE
	{
		Matrix3* m3InVal = (Matrix3*)ptr;
		*m3InVal = xformOptions.maxMatrix * (*m3InVal);
	}
}

AlembicXFormCtrl::AlembicXFormCtrl()
{
    pblock = NULL;
    sAlembicXFormCtrlClassDesc.MakeAutoParamBlocks(this);
}

AlembicXFormCtrl::~AlembicXFormCtrl()
{
}

RefTargetHandle AlembicXFormCtrl::Clone(RemapDir& remap) 
{
	AlembicXFormCtrl *ctrl = new AlembicXFormCtrl();
    ctrl->ReplaceReference (0, remap.CloneRef(pblock));
	
    BaseClone(this, ctrl, remap);
	return ctrl;
}

void AlembicXFormCtrl::SetValueLocalTime(TimeValue t, void *ptr, int commit, GetSetMethod method)
{

}

void* AlembicXFormCtrl::CreateTempValue()
{
    return new Matrix3(1);
}

void AlembicXFormCtrl::DeleteTempValue(void *val)
{
    delete (Matrix3*)val;
}

void AlembicXFormCtrl::ApplyValue(void *val, void *delta)
{
    Matrix3 &deltatm = *((Matrix3*)delta);
    Matrix3 &valtm = *((Matrix3*)val);
    valtm = deltatm * valtm;
}

void AlembicXFormCtrl::MultiplyValue(void *val, float m)
{
     Matrix3 *valtm = (Matrix3*)val;
     valtm->PreScale(Point3(m,m,m));
}

void AlembicXFormCtrl::Extrapolate(Interval range, TimeValue t, void *val, Interval &valid, int type)
{
}

/*
void AlembicXFormCtrl::SetAlembicId(const std::string &file, const std::string &identifier, TimeValue t)
{
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}*/


#define LOCK_CHUNK		0x2535  //the lock value
IOResult AlembicXFormCtrl::Save(ISave *isave)
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

IOResult AlembicXFormCtrl::Load(ILoad *iload)
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

RefTargetHandle AlembicXFormCtrl::GetReference(int i)
{
    switch (i)
    {
    case 0:
        return pblock;
    }

    return NULL;
}

void AlembicXFormCtrl::SetReference(int i, RefTargetHandle rtarg)
{
    switch (i)
    {
    case 0:
        pblock = (IParamBlock2*)rtarg; 
        break;
    }
}

RefResult AlembicXFormCtrl::NotifyRefChanged(
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
                xform_params_desc.InvalidateUI(changing_param);
            }
            break;

        case REFMSG_OBJECT_CACHE_DUMPED:
            return REF_STOP;
            break;
    }

    return REF_SUCCEED;
}

void AlembicXFormCtrl::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;
    editMod  = this;

    LockableStdControl::BeginEditParams(ip, flags, prev);
	sAlembicXFormCtrlClassDesc.BeginEditParams(ip, this, flags, prev);
    
//    AlembicXFormCtrlDlgProc* dlgProc;
//	dlgProc = new AlembicXFormCtrlDlgProc(this);
//	xform_params_desc.SetUserDlgProc( AlembicXFormCtrl_params, dlgProc );

    // Necessary?
	// NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void AlembicXFormCtrl::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
    LockableStdControl::EndEditParams(ip, flags, next);
	sAlembicXFormCtrlClassDesc.EndEditParams(ip, this, flags, next);

	this->ip = NULL;
    editMod  = NULL;
}
/*
void AlembicXFormCtrl::MainPanelInitDialog( HWND hWnd ) 
{
	mhPanel = hWnd;
	MainPanelUpdateUI();
}

void AlembicXFormCtrl::MainPanelDestroy( HWND hWnd ) 
{
	mhPanel = NULL;
}

void AlembicXFormCtrl::MainPanelUpdateUI()
{
	if( (mhPanel == NULL) || mbSuspendPanelUpdate) 
		return;

	mbSuspendPanelUpdate = true;

    TimeValue t = GetCOREInterface12()->GetTime();
    Interval valid = FOREVER;

    // Update the abc archive and identifier
    MCHAR const* strPath = NULL;
	pblock->GetValue( alembicxform_abc_archive, t, strPath, valid);
    HWND hDlg = GetDlgItem(mhPanel,IDC_ABC_ARCHIVE);
    SetWindowText(hDlg, strPath);

    MCHAR const* strIdentifier = NULL;
	pblock->GetValue( alembicxform_abc_id, t, strIdentifier, valid);
    SetWindowText(GetDlgItem(mhPanel,IDC_ABC_OBJECTID), strIdentifier);

	mbSuspendPanelUpdate = false;
}
*/
