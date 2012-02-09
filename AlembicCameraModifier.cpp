#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicCameraModifier.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include "iparamb2.h"
#include "alembic.h"

/*
typedef struct _alembic_fillcamera_options
{
    Alembic::AbcGeom::OCamera *pIObj;
    CameraObject *pCameraObj;
    double iFrame;
    AlembicDataFillFlags nDataFillFlags;
    AlembicFillContext nDataFillContext;
    unsigned int nValidityFlags;

    _alembic_fillcamera_options()
    {
        pIObj = 0;
        pCameraObj = 0;
        iFrame = 0;
        nDataFillFlags = 0;
        nValidityFlags = 0;
        nDataFillContext = ALEMBIC_FILLCONTEXT_NONE;
    }
} alembic_fillcamera_options;

void AlembicImport_FillInCamera(alembic_fillcamera_options &options);

class AlembicCameraModifier : public Modifier 
{
public:
	IParamBlock2 *pblock;

	static IObjParam *ip;
	static AlembicCameraModifier *editMod;

	AlembicCameraModifier();

	// From Animatable
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Exocortex Alembic Camera"); }  
	virtual Class_ID ClassID() { return EXOCORTEX_ALEMBIC_CAMERA_MODIFIER_ID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Exocortex Alembic Camera"); }

	// From modifier
	ChannelMask ChannelsUsed()  { return OBJ_CHANNELS; }
	ChannelMask ChannelsChanged() { return OBJ_CHANNELS; }
	Class_ID InputType() { return triObjectClassID; }
	void ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node);
	Interval LocalValidity(TimeValue t) { return GetValidity(t); }
	Interval GetValidity (TimeValue t);
	BOOL DependOnTopology(ModContext &mc) { return FALSE; }

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;} 
	void BeginEditParams(IObjParam  *ip, ULONG flags,Animatable *prev);
	void EndEditParams(IObjParam *ip,ULONG flags,Animatable *next);		

	int NumParamBlocks () { return 1; }
	IParamBlock2 *GetParamBlock (int i) { return pblock; }
	IParamBlock2 *GetParamBlockByID (short id);

	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return pblock; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { pblock = (IParamBlock2 *) rtarg; }
public:

	int NumSubs() {return 1;}
	Animatable* SubAnim(int i) { return GetReference(i); }
	TSTR SubAnimName(int i) {return _T("IDS_PARAMETERS");}

	RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, 
		PartID& partID, RefMessage message);

	void Convert (PolyObject *obj, TimeValue t, Mesh & m, Interval & ivalid);
	void Convert (TriObject *obj, TimeValue t, Mesh & m, Interval & ivalid);
	void Convert (PatchObject *obj, TimeValue t, Mesh & m, Interval & ivalid);

	int UI2SelLevel(int selLevel);
private:
    alembic_nodeprops m_AlembicNodeProps;
public:
	void SetAlembicId(const std::string &file, const std::string &identifier);
};
//--- ClassDescriptor and class vars ---------------------------------

IObjParam *AlembicCameraModifier::ip              = NULL;
AlembicCameraModifier *AlembicCameraModifier::editMod         = NULL;

class AlembicCameraModifierClassDesc : public ClassDesc2 
{
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new AlembicCameraModifier; }
	const TCHAR *	ClassName() { return _T("ExoCortex Alembic Camera"); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return EXOCORTEX_ALEMBIC_CAMERA_MODIFIER_ID; }
	const TCHAR* 	Category() { return _T("MAX STANDARD"); }
	const TCHAR*	InternalName() { return _T("AlembicCameraModifier"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

static AlembicCameraModifierClassDesc AlembicCameraModifierDesc;
ClassDesc* GetAlembicCameraModifierDesc() {return &AlembicCameraModifierDesc;}

// Parameter block IDs:
// Blocks themselves:
enum { turn_params };
// Parameters in first block:
enum { turn_use_invis, turn_sel_type, turn_softsel, turn_sel_level };

static ParamBlockDesc2 turn_param_desc ( turn_params, _T("ExoCortexAlembicCameraModifier"),
									IDS_PARAMETERS, &AlembicCameraModifierDesc,
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

AlembicCameraModifier::AlembicCameraModifier() {
	pblock = NULL;
	GetAlembicCameraModifierDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicCameraModifier::Clone(RemapDir& remap) {
	AlembicCameraModifier *mod = new AlembicCameraModifier();
	mod->ReplaceReference (0, remap.CloneRef(pblock));
	BaseClone(this, mod, remap);
	return mod;
}

IParamBlock2 *AlembicCameraModifier::GetParamBlockByID (short id) {
	return (pblock->ID() == id) ? pblock : NULL; 
}

Interval AlembicCameraModifier::GetValidity (TimeValue t) {
	Interval ret = FOREVER;
	pblock->GetValidity (t, ret);
	return ret;
}

void AlembicCameraModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
   Interval ivalid=os->obj->ObjectValidity(t);

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(m_AlembicNodeProps.m_File, m_AlembicNodeProps.m_Identifier);
   if(!iObj.valid())
      return;

   // Calculate the sample time in seconds
   double sampleTime = double(t) / ALEMBIC_3DSMAX_TICK_VALUE;

   alembic_fillcamera_options options;
   options.pIObj = &iObj;
   options.pTriObj = (TriObject*)os->obj;
   options.iFrame = sampleTime;
   options.nDataFillFlags = ALEMBIC_DATAFILL_CAMERA_STATIC_UPDATE;
   options.nDataFillContext = ALEMBIC_FILLCONTEXT_UPDATE;
   AlembicImport_FillInCamera(options);

   Interval alembicValid(t, t); 
   ivalid &= alembicValid;
    
    // update the validity channel
    os->obj->UpdateValidity(OBJ_CHANNELS, ivalid);
}

void AlembicCameraModifier::BeginEditParams (IObjParam  *ip, ULONG flags, Animatable *prev) {
	this->ip = ip;	
	editMod  = this;

	// throw up all the appropriate auto-rollouts
	AlembicCameraModifierDesc.BeginEditParams(ip, this, flags, prev);

	// Necessary?
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void AlembicCameraModifier::EndEditParams (IObjParam *ip,ULONG flags,Animatable *next) {
	AlembicCameraModifierDesc.EndEditParams(ip, this, flags, next);
	this->ip = NULL;
	editMod  = NULL;
}

RefResult AlembicCameraModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message) {
	switch (message) {
	case REFMSG_CHANGE:
		if (editMod!=this) break;
		// if this was caused by a NotifyDependents from pblock, LastNotifyParamID()
		// will contain ID to update, else it will be -1 => inval whole rollout
		if (pblock->LastNotifyParamID() == turn_sel_level) {
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

void AlembicCameraModifier::SetAlembicId(const std::string &file, const std::string &identifier)
{
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}

void AlembicImport_FillInCamera(alembic_fillcamera_options &options)
{
   Alembic::AbcGeom::OCamera objCamera;
   Alembic::AbcGeom::ISubD objSubD;

   if(Alembic::AbcGeom::OCamera::matches((*options.pIObj).getMetaData()))
       objMesh = Alembic::AbcGeom::ICamera(*options.pIObj,Alembic::Abc::kWrapExisting);
   else
       objSubD = Alembic::AbcGeom::ISubD(*options.pIObj,Alembic::Abc::kWrapExisting);

   if(!objMesh.valid() && !objSubD.valid())
       return;

  SampleInfo sampleInfo;
   if(objMesh.valid())
      sampleInfo = getSampleInfo(
         options.iFrame,
         objMesh.getSchema().getTimeSampling(),
         objMesh.getSchema().getNumSamples()
      );
   else
      sampleInfo = getSampleInfo(
         options.iFrame,
         objSubD.getSchema().getTimeSampling(),
         objSubD.getSchema().getNumSamples()
      );

   Mesh &mesh = options.pTriObj->GetMesh();
   Alembic::AbcGeom::ICameraSchema::Sample CameraSample;
   Alembic::AbcGeom::ISubDSchema::Sample subDSample;

   if(objMesh.valid())
       objMesh.getSchema().get(CameraSample,sampleInfo.floorIndex);
   else
       objSubD.getSchema().get(subDSample,sampleInfo.floorIndex);   

   
}

int AlembicImport_Camera(const std::string &file, const std::string &identifier, alembic_importoptions &options)
{
	// Find the object in the archive
    Alembic::AbcGeom::OCamera iObj = getObjectFromArchive(file,identifier);
	if(!iObj.valid())
		return alembic_failure;

	// Create the tri object and place it in the scene
    CameraObject *cameraobj = GenCamera::NewCamera();
    if (!triobj)
        return alembic_failure;

	// Fill in the mesh
    alembic_fillcamera_options dataFillOptions;
    dataFillOptions.pIObj = &iObj;
    dataFillOptions.pTriObj = triobj;
    dataFillOptions.iFrame = 0;
    dataFillOptions.nDataFillFlags = ALEMBIC_DATAFILL_CAMERA_STATIC_IMPORT;
    dataFillOptions.nDataFillContext = ALEMBIC_FILLCONTEXT_IMPORT;
	AlembicImport_FillInCamera(dataFillOptions);

	// Create the object node
	INode *node = GetCOREInterface12()->CreateObjectNode(triobj, iObj.getName().c_str());
	if (!node)
		return alembic_failure;

	// Create the Camera modifier
	AlembicCameraModifier *pModifier = static_cast<AlembicCameraModifier*>
		(GetCOREInterface()->CreateInstance(OSM_CLASS_ID, EXOCORTEX_ALEMBIC_CAMERA_MODIFIER_ID));

	// Set the alembic id
	pModifier->SetAlembicId(file, identifier);

	// Add the modifier to the node
	GetCOREInterface12()->AddModifier(*node, *pModifier);

    // Add the new inode to our current scene list
    SceneEntry *pEntry = options.sceneEnumProc.Append(node, triobj, OBTYPE_MESH, &std::string(iObj.getFullName())); 
    options.currentSceneList.Append(pEntry);

    // Set up any child links for this node
    AlembicImport_SetupChildLinks(iObj, options);

	return 0;
}
*/