#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicDefinitions.h"
#include "AlembicXFormCtrl.h"
#include "Utility.h"
#include "dummy.h"
#include <ILockedTracks.h>
#include "iparamb2.h"

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
	alembicxform_preview_abc_archive,
    alembicxform_preview_abc_id,
};

class PBXForm_Accessor : public PBAccessor
{
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		AlembicXFormCtrl *xformCtrl = (AlembicXFormCtrl*) owner;
		switch(id)
		{
        case alembicxform_preview_abc_archive:
            {
                const char *strArchive = xformCtrl->GetAlembicArchive().c_str();
                xformCtrl->GetParamBlock(AlembicXFormCtrl_params)->SetValue(alembicxform_preview_abc_archive, t, strArchive);
            }
            break;
        case alembicxform_preview_abc_id:
            {
                const char *strObjectId = xformCtrl->GetAlembicObjectId().c_str();
                xformCtrl->GetParamBlock(AlembicXFormCtrl_params)->SetValue(alembicxform_preview_abc_id, t, strObjectId);
            }
            break;
		default: 
            break;
		}

		GetCOREInterface12()->RedrawViews(GetCOREInterface()->GetTime());
	}

    void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		AlembicXFormCtrl *xformCtrl = (AlembicXFormCtrl*) owner;
		switch(id)
		{
        case alembicxform_preview_abc_archive:
            {
                const char *strArchive = xformCtrl->GetAlembicArchive().c_str();
                xformCtrl->GetParamBlock(AlembicXFormCtrl_params)->SetValue(alembicxform_preview_abc_archive, t, strArchive);
            }
            break;
        case alembicxform_preview_abc_id:
            {
                const char *strObjectId = xformCtrl->GetAlembicObjectId().c_str();
                xformCtrl->GetParamBlock(AlembicXFormCtrl_params)->SetValue(alembicxform_preview_abc_id, t, strObjectId);
            }
            break;
		default: 
            break;
		}

		GetCOREInterface12()->RedrawViews(GetCOREInterface()->GetTime());
	}
};

static PBXForm_Accessor sXForm_PBAccessor;

static ParamBlockDesc2 xform_params_desc ( AlembicXFormCtrl_params, _T("ExoCortexAlembicXFormController"),
									IDS_PREVIEW, &sAlembicXFormCtrlClassDesc,
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

    alembicxform_preview_abc_archive, _T("previewAbcArchive"), TYPE_STRING, 0, IDS_ABC_ARCHIVE,
        p_ui, TYPE_EDITBOX, IDC_ABC_ARCHIVE,
        p_accessor,		&sXForm_PBAccessor,
        end,

	alembicxform_preview_abc_id, _T("previewAbcId"), TYPE_STRING, 0, IDS_ABC_ID,
		p_ui, TYPE_EDITBOX, IDC_ABC_OBJECTID,
        p_accessor,		&sXForm_PBAccessor,
		end,
	end
);

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicXFormCtrl Methods
///////////////////////////////////////////////////////////////////////////////////////////////////
IObjParam *AlembicXFormCtrl::ip = NULL;
AlembicXFormCtrl *AlembicXFormCtrl::editMod = NULL;

void AlembicXFormCtrl::GetValueLocalTime(TimeValue t, void *ptr, Interval &valid, GetSetMethod method)
{
    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(m_AlembicNodeProps.m_File, m_AlembicNodeProps.m_Identifier);
    
    if(!iObj.valid())
        return;

    alembic_fillxform_options xformOptions;
    xformOptions.pIObj = &iObj;
    xformOptions.dTicks = t;
    xformOptions.bIsCameraTransform = m_bIsCameraTransform;
    AlembicImport_FillInXForm(xformOptions);

	valid.Set(t,t);

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
    m_bIsCameraTransform = false;
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

void AlembicXFormCtrl::SetAlembicId(const std::string &file, const std::string &identifier, TimeValue t)
{
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}

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

	sAlembicXFormCtrlClassDesc.BeginEditParams(ip, this, flags, prev);

    // Necessary?
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void AlembicXFormCtrl::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
	sAlembicXFormCtrlClassDesc.EndEditParams(ip, this, flags, next);

	this->ip = NULL;
    editMod  = NULL;
}
