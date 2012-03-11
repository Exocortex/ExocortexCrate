#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicDefinitions.h"
#include "AlembicXFormCtrl.h"
#include "Utility.h"
#include "dummy.h"
#include <ILockedTracks.h>
#include "iparamb2.h"
#include "AlembicNames.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// alembic_fillxform_options
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _alembic_fillxform_options
{
    Alembic::AbcGeom::IObject *pIObj;
    Matrix3 maxMatrix;
    Box3    maxBoundingBox;
    TimeValue dTicks;
    bool bIsCameraTransform;

    _alembic_fillxform_options()
    {
        pIObj = 0;
        dTicks = 0;
        maxMatrix.IdentityMatrix();
        bIsCameraTransform = false;
    }
} alembic_fillxform_options;

void AlembicImport_FillInXForm(alembic_fillxform_options &options);

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicXFormCtrl Declaration
///////////////////////////////////////////////////////////////////////////////////////////////////
class AlembicXFormCtrl : public LockableStdControl
{
public:
    IParamBlock2* pblock;
    static IObjParam *ip;
    static AlembicXFormCtrl *editMod;
public:
	AlembicXFormCtrl();
	~AlembicXFormCtrl();

	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }

    void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic Xform"); }  
	virtual Class_ID ClassID() { return ALEMBIC_XFORM_CTRL_CLASSID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Alembic Xform"); }

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
    void SetIsCameraTransform( bool camera) { m_bIsCameraTransform = camera; }
    bool GetIsCameraTransform() { return m_bIsCameraTransform; }


private:
    alembic_nodeprops m_AlembicNodeProps;
    Interval m_CurrentAlembicInterval;
    bool m_bIsCameraTransform;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicXFormCtrlClassDesc
///////////////////////////////////////////////////////////////////////////////////////////////////
class AlembicXFormCtrlClassDesc : public ClassDesc2
{
public:
	AlembicXFormCtrlClassDesc() {};
	~AlembicXFormCtrlClassDesc() {};

	// ClassDesc methods.  
	// Max calls these functions to figure out what kind of plugin this class represents

	// Return TRUE if the user can create this plug-in.
	int			IsPublic()			{ return TRUE; }	// We do want the user to see this plug-in

	// Return the class name of this plug-in
	const MCHAR* ClassName()		{ static const MSTR str("Alembic Xform"); return str; }

	// Return the SuperClassID - this ID should
	// match the implementation of the interface returned
	// by Create.
	SClass_ID	SuperClassID()		{ return CTRL_MATRIX3_CLASS_ID; }

	// Return the unique ID that identifies this class
	// This is required when saving.  Max stores the ClassID
	// reported by the actual plugin, and on reload it recreates
	// the appropriate class by matching the stored ClassID with
	// the matching ClassDesc
	//
	// You can generate random ClassID's using the gencid program
	// supplied with the Max SDK
	Class_ID	ClassID()			{ return ALEMBIC_XFORM_CTRL_CLASSID; }

	// If the plugin is an Object or Texture, this function returns
	// the category it can be assigned to.
	const MCHAR* Category()			{ return EXOCORTEX_ALEMBIC_CATEGORY; }

	// Return an instance of this plug-in.  Max will call this function
	// when it wants to start using our plug-in
	void* Create(BOOL loading=FALSE)
	{
		return new AlembicXFormCtrl;
	}

    const TCHAR*	InternalName() { return _T("AlembicXformController"); }	// returns fixed parsable name (scripter-visible name)
    HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

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

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicImport_FillInXForm
///////////////////////////////////////////////////////////////////////////////////////////////////
void AlembicImport_FillInXForm(alembic_fillxform_options &options)
{
    if(!options.pIObj->valid())
        return;
    
    // Alembic::AbcGeom::IXform obj;
    Alembic::AbcGeom::IXform obj(*(options.pIObj), Alembic::Abc::kWrapExisting);

    if(!obj.valid())
        return;

   float masterScaleUnitMeters = (float)GetMasterScale(UNITS_METERS);

   double SampleTime = GetSecondsFromTimeValue(options.dTicks);

    SampleInfo sampleInfo = getSampleInfo(
        SampleTime,
        obj.getSchema().getTimeSampling(),
        obj.getSchema().getNumSamples()
        );

    Alembic::AbcGeom::XformSample sample;
    obj.getSchema().get(sample,sampleInfo.floorIndex);
    Alembic::Abc::M44d matrix = sample.getMatrix();

    const Alembic::Abc::Box3d &box3d = sample.getChildBounds();

    options.maxBoundingBox = Box3(Point3(box3d.min.x, box3d.min.y, box3d.min.z), 
        Point3(box3d.max.x, box3d.max.y, box3d.max.z));

    // blend 
    if(sampleInfo.alpha != 0.0)
    {
        obj.getSchema().get(sample,sampleInfo.ceilIndex);
        Alembic::Abc::M44d ceilMatrix = sample.getMatrix();
        matrix = (1.0 - sampleInfo.alpha) * matrix + sampleInfo.alpha * ceilMatrix;
    }

    Matrix3 objMatrix(
        Point3(matrix.getValue()[0], matrix.getValue()[1], matrix.getValue()[2]),
        Point3(matrix.getValue()[4], matrix.getValue()[5], matrix.getValue()[6]),
        Point3(matrix.getValue()[8], matrix.getValue()[9], matrix.getValue()[10]),
        Point3(matrix.getValue()[12], matrix.getValue()[13], matrix.getValue()[14]));

    ConvertAlembicMatrixToMaxMatrix(objMatrix, masterScaleUnitMeters, options.maxMatrix);

    if (options.bIsCameraTransform)
    {
        // Cameras in Max are already pointing down the negative z-axis (as is expected from Alembic).
        // So we rotate it by 90 degrees so that it is pointing down the positive y-axis.
        Matrix3 rotation(TRUE);
        rotation.RotateX(HALFPI);
        options.maxMatrix = rotation * options.maxMatrix;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicImport_XForm
///////////////////////////////////////////////////////////////////////////////////////////////////
int AlembicImport_XForm(const std::string &file, const std::string &identifier, alembic_importoptions &options)
{
    // Find the object in the archive
	Alembic::AbcGeom::IObject iObj = getObjectFromArchive(file,identifier);
	if(!iObj.valid())
		return alembic_failure;

    // Find out if we're dealing with a camera
    std::string modelfullid = getModelFullName(std::string(iObj.getFullName()));
    Alembic::AbcGeom::IObject iChildObj = getObjectFromArchive(file,modelfullid);
    bool bIsCamera = iChildObj.valid() && Alembic::AbcGeom::ICamera::matches(iChildObj.getMetaData());

    // Get the matrix for the current time 
    alembic_fillxform_options xformOptions;
    xformOptions.pIObj = &iObj;
    xformOptions.dTicks = GetCOREInterface12()->GetTime();
    xformOptions.bIsCameraTransform = bIsCamera;
    AlembicImport_FillInXForm(xformOptions);

    // Find the scene node that this transform belongs too
    // If the node does not exist, then this is just a transform, so we create
    // a dummy helper object to attach the modifier
    std::string modelid = getModelName(std::string(iObj.getName()));
    INode *pNode = options.currentSceneList.FindNodeWithName(modelid);
    if (!pNode)
    {
        Object* obj = static_cast<Object*>(CreateInstance(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID,0)));

        if (!obj)
            return alembic_failure;

        DummyObject *pDummy = static_cast<DummyObject*>(obj);

        // Hard code these values for now
        pDummy->SetColor(Point3(0.6f, 0.8f, 1.0f));
        pDummy->SetBox(xformOptions.maxBoundingBox);

        pDummy->EnableDisplay();

        pNode = GetCOREInterface12()->CreateObjectNode(obj, modelid.c_str());

        if (!pNode)
            return alembic_failure;

        // Add it to our current scene list
        SceneEntry *pEntry = options.sceneEnumProc.Append(pNode, obj, OBTYPE_DUMMY, &std::string(iObj.getFullName())); 
        options.currentSceneList.Append(pEntry);

        // Set up any child links for this node
        AlembicImport_SetupChildLinks(iObj, options);
    }

	// Create the xform modifier
	AlembicXFormCtrl *pCtrl = static_cast<AlembicXFormCtrl*>
		(GetCOREInterface()->CreateInstance(CTRL_MATRIX3_CLASS_ID, ALEMBIC_XFORM_CTRL_CLASSID));

	// Set the alembic id
	pCtrl->SetAlembicId(file, identifier, xformOptions.dTicks);
    pCtrl->SetIsCameraTransform(xformOptions.bIsCameraTransform);

	// Add the modifier to the node
    pNode->SetTMController(pCtrl);
    pNode->InvalidateTreeTM();

    // Lock the transform
    LockNodeTransform(pNode, true);

	return alembic_success;
}

