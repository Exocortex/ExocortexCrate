#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicCameraModifier.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include "iparamb2.h"
#include "AlembicVisCtrl.h"

extern HINSTANCE hInstance;

//////////////////////////////////////////////////////////////////////////////////////////
// Import options struct containing the information necessary to fill the camera object
typedef struct _alembic_fillcamera_options
{
public:
    _alembic_fillcamera_options();

    Alembic::AbcGeom::IObject  *pIObj;
    GenCamera                  *pCameraObj;
    TimeValue                   dTicks;
}
alembic_fillcamera_options;

_alembic_fillcamera_options::_alembic_fillcamera_options()
{
    pIObj = NULL;
    pCameraObj = NULL;
    dTicks = 0;
}


//////////////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
void AlembicImport_FillInCamera(alembic_fillcamera_options &options);


//////////////////////////////////////////////////////////////////////////////////////////
// Camera Modifier object
class AlembicCameraModifier : public Modifier 
{
public:
	static IObjParam *ip;
	static AlembicCameraModifier *editMod;

	AlembicCameraModifier();

	// From Animatable
	RefTargetHandle Clone(RemapDir& remap);
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic Camera"); }  
	virtual Class_ID ClassID() { return EXOCORTEX_ALEMBIC_CAMERA_MODIFIER_ID; }		
	TCHAR *GetObjectName() { return _T("Alembic Camera"); }

	// From modifier
	ChannelMask ChannelsUsed() { return DISP_ATTRIB_CHANNEL; }		// TODO: What channels do we actually need?
	ChannelMask ChannelsChanged() { return DISP_ATTRIB_CHANNEL; }
	Class_ID InputType() { return defObjectClassID; }
	void ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node);
	Interval LocalValidity(TimeValue t) { return GetValidity(t); }
	Interval GetValidity (TimeValue t);
	BOOL DependOnTopology(ModContext &mc) { return FALSE; }

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;} 
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);		

	int NumParamBlocks () { return 1; }
	IParamBlock2 *GetParamBlock (int i) { return pblock; }
	IParamBlock2 *GetParamBlockByID (short id);

	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return pblock; }

	int NumSubs() { return 1; }
	Animatable* SubAnim(int i) { return GetReference(i); }
	TSTR SubAnimName(int i) { return _T("IDS_PARAMETERS"); }

	RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
		                       PartID& partID, RefMessage message);

	void SetAlembicId(const std::string &file, const std::string &identifier);
    void SetCamera(GenCamera *pCam);

private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { pblock = (IParamBlock2 *) rtarg; }

    alembic_nodeprops m_AlembicNodeProps;
	IParamBlock2     *pblock;
    GenCamera        *m_pCamera;
};

//--- ClassDescriptor and class vars ---------------------------------

IObjParam *AlembicCameraModifier::ip                    = NULL;
AlembicCameraModifier *AlembicCameraModifier::editMod   = NULL;

class AlembicCameraModifierClassDesc : public ClassDesc2 
{
public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new AlembicCameraModifier; }
	const TCHAR *	ClassName() { return _T("Alembic Camera"); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return EXOCORTEX_ALEMBIC_CAMERA_MODIFIER_ID; }
	const TCHAR* 	Category() { return _T("MAX STANDARD"); }
	const TCHAR*	InternalName() { return _T("AlembicCameraModifier"); }  // returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }                       // returns owning module handle
};

static AlembicCameraModifierClassDesc s_AlembicCameraModifierDesc;
ClassDesc* GetAlembicCameraModifierDesc() { return &s_AlembicCameraModifierDesc; }

// Parameter block IDs:
// Blocks themselves:
enum { turn_params };
// Parameters in first block:
enum { turn_use_invis, turn_sel_type, turn_softsel, turn_sel_level };

static ParamBlockDesc2 turn_param_desc
(
    turn_params, _T("AlembicCameraModifier"),
    IDS_PARAMETERS, &s_AlembicCameraModifierDesc,
    P_AUTO_CONSTRUCT | P_AUTO_UI, REF_PBLOCK,

	//rollout description
	IDD_TO_MESH, IDS_PARAMETERS, 0, 0, NULL,

	// params
	turn_use_invis, _T("useInvisibleEdges"), TYPE_BOOL, P_RESET_DEFAULT|P_ANIMATABLE, IDS_USE_INVIS,
		p_default, TRUE,
		p_ui, TYPE_SINGLECHEKBOX, IDC_USE_INVIS,
		end,

	turn_sel_type, _T("selectionConversion"), TYPE_INT, P_RESET_DEFAULT, IDS_SEL_TYPE,
		p_default, 0, // Preserve selection
		p_ui, TYPE_RADIO, 3, IDC_SEL_PRESERVE, IDC_SEL_CLEAR, IDC_SEL_INVERT,
		end,

	turn_softsel, _T("useSoftSelection"), TYPE_BOOL, P_RESET_DEFAULT, IDS_USE_SOFTSEL,
		p_default, TRUE,
		p_ui, TYPE_SINGLECHEKBOX, IDC_USE_SOFTSEL,
		end,

	turn_sel_level, _T("selectionLevel"), TYPE_INT, P_RESET_DEFAULT, IDS_SEL_LEVEL,
		p_default, 0, // Object level.
		p_ui, TYPE_RADIO, 5, IDC_SEL_PIPELINE, IDC_SEL_OBJ, IDC_SEL_VERT, IDC_SEL_EDGE, IDC_SEL_FACE,
		end,
	end
);

//--- Modifier methods -------------------------------

AlembicCameraModifier::AlembicCameraModifier()
{
	pblock = NULL;
    m_pCamera = NULL;
	GetAlembicCameraModifierDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicCameraModifier::Clone(RemapDir& remap)
{
	AlembicCameraModifier *mod = new AlembicCameraModifier();
	mod->ReplaceReference (0, remap.CloneRef(pblock));
	BaseClone(this, mod, remap);
	return mod;
}

IParamBlock2 *AlembicCameraModifier::GetParamBlockByID (short id)
{
	return (pblock->ID() == id) ? pblock : NULL; 
}

Interval AlembicCameraModifier::GetValidity (TimeValue t)
{
	Interval ret = FOREVER;
	pblock->GetValidity (t, ret);
	return ret;
}

void AlembicCameraModifier::ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(m_AlembicNodeProps.m_File, m_AlembicNodeProps.m_Identifier);
    if (!iObj.valid())
    {
        return;
    }

    if (m_pCamera == NULL)
    {
        return;
    }

    alembic_fillcamera_options dataFillOptions;
    dataFillOptions.pIObj = &iObj;
    dataFillOptions.pCameraObj = m_pCamera;
    dataFillOptions.dTicks = t;
    AlembicImport_FillInCamera(dataFillOptions);

    // Determine the validity interval
    Interval ivalid = os->obj->ObjectValidity(t);
    Interval alembicValid(t, t); 
    ivalid &= alembicValid;
    os->obj->UpdateValidity(DISP_ATTRIB_CHAN_NUM, ivalid);
}

void AlembicCameraModifier::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	this->ip = ip;	
	editMod  = this;

	// throw up all the appropriate auto-rollouts
	s_AlembicCameraModifierDesc.BeginEditParams(ip, this, flags, prev);

	// Necessary?
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void AlembicCameraModifier::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	s_AlembicCameraModifierDesc.EndEditParams(ip, this, flags, next);
	this->ip = NULL;
	editMod  = NULL;
}

RefResult AlembicCameraModifier::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message)
{
	switch (message)
    {
	case REFMSG_CHANGE:
		if (editMod != this)
        {
            break;
        }

        // if this was caused by a NotifyDependents from pblock, LastNotifyParamID()
		// will contain ID to update, else it will be -1 => inval whole rollout
		if (pblock->LastNotifyParamID() == turn_sel_level)
        {
			// Notify stack that subobject info has changed:
			NotifyDependents(changeInt, partID, message);
			NotifyDependents(FOREVER, 0, REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
			return REF_STOP;
		}

        turn_param_desc.InvalidateUI(pblock->LastNotifyParamID());
		break;
	}

	return REF_SUCCEED;
}

void AlembicCameraModifier::SetCamera(GenCamera *pCam)
{
    m_pCamera = pCam;
}

void AlembicCameraModifier::SetAlembicId(const std::string &file, const std::string &identifier)
{
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}


void AlembicImport_FillInCamera(alembic_fillcamera_options &options)
{
   	ESS_CPP_EXCEPTION_REPORTING_START

	if (options.pCameraObj == NULL ||
        !Alembic::AbcGeom::ICamera::matches((*options.pIObj).getMetaData()))
    {
        return;
    }

    Alembic::AbcGeom::ICamera objCamera = Alembic::AbcGeom::ICamera(*options.pIObj, Alembic::Abc::kWrapExisting);
    if (!objCamera.valid())
    {
        return;
    }

    double sampleTime = GetSecondsFromTimeValue(options.dTicks);
    SampleInfo sampleInfo = getSampleInfo(sampleTime,
                                          objCamera.getSchema().getTimeSampling(),
                                          objCamera.getSchema().getNumSamples());
    Alembic::AbcGeom::CameraSample sample;
    objCamera.getSchema().get(sample, sampleInfo.floorIndex);

    // Extract the camera values from the sample
    double focalLength = sample.getFocalLength();
    double fov = sample.getHorizontalAperture();
    double nearClipping = sample.getNearClippingPlane();
    double farClipping = sample.getFarClippingPlane();

    // Blend the camera values, if necessary
    if (sampleInfo.alpha != 0.0)
    {
        objCamera.getSchema().get(sample, sampleInfo.ceilIndex);
        focalLength = (1.0 - sampleInfo.alpha) * focalLength + sampleInfo.alpha * sample.getFocalLength();
        fov = (1.0 - sampleInfo.alpha) * fov + sampleInfo.alpha * sample.getHorizontalAperture();
        nearClipping = (1.0 - sampleInfo.alpha) * nearClipping + sampleInfo.alpha * sample.getNearClippingPlane();
        farClipping = (1.0 - sampleInfo.alpha) * farClipping + sampleInfo.alpha * sample.getFarClippingPlane();
    }

    options.pCameraObj->SetTDist(options.dTicks, static_cast<float>(focalLength));
    options.pCameraObj->SetFOVType(0);  // Width FoV = 0
    options.pCameraObj->SetFOV(options.dTicks, static_cast<float>(fov));
    options.pCameraObj->SetClipDist(options.dTicks, CAM_HITHER_CLIP, static_cast<float>(nearClipping));
    options.pCameraObj->SetClipDist(options.dTicks, CAM_YON_CLIP, static_cast<float>(farClipping));
    options.pCameraObj->SetManualClip(TRUE);

	ESS_CPP_EXCEPTION_REPORTING_END
}

int AlembicImport_Camera(const std::string &file, const std::string &identifier, alembic_importoptions &options)
{
    // Find the object in the archive
    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(file, identifier);
	if (!iObj.valid())
    {
		return alembic_failure;
    }

	// Create the camera object and place it in the scene
    GenCamera *pCameraObj = GetCOREInterface12()->CreateCameraObject(FREE_CAMERA);
    if (pCameraObj == NULL)
    {
        return alembic_failure;
    }
    pCameraObj->Enable(TRUE);
    pCameraObj->SetConeState(TRUE);

    // Fill in the mesh
    alembic_fillcamera_options dataFillOptions;
    dataFillOptions.pIObj = &iObj;
    dataFillOptions.pCameraObj = pCameraObj;
    dataFillOptions.dTicks =  GetCOREInterface12()->GetTime();
	AlembicImport_FillInCamera(dataFillOptions);

    // Create the object node
	INode *pNode = GetCOREInterface12()->CreateObjectNode(pCameraObj, iObj.getName().c_str());
	if (pNode == NULL)
    {
		return alembic_failure;
    }

	// Create the Camera modifier
	AlembicCameraModifier *pModifier = static_cast<AlembicCameraModifier*>
		(GetCOREInterface12()->CreateInstance(OSM_CLASS_ID, EXOCORTEX_ALEMBIC_CAMERA_MODIFIER_ID));

	// Set the alembic id
	pModifier->SetAlembicId(file, identifier);
    pModifier->SetCamera(pCameraObj);

	// Add the modifier to the node
	GetCOREInterface12()->AddModifier(*pNode, *pModifier);

    // Add the new inode to our current scene list
    SceneEntry *pEntry = options.sceneEnumProc.Append(pNode, pCameraObj, OBTYPE_CAMERA, &std::string(iObj.getFullName())); 
    options.currentSceneList.Append(pEntry);

    // Set the visibility controller
    AlembicImport_SetupVisControl(iObj, pNode, options);

	return 0;


}
