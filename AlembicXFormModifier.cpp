#include "Alembic.h"
#include "iparamm2.h"
#include "simpmod.h"
#include "AlembicXFormModifier.h"
#include "MeshNormalSpec.h"
#include "AlembicDefinitions.h"
#include "AlembicArchiveStorage.h"
#include "Utility.h"
#include "alembic.h"
#include "dummy.h"

static GenSubObjType SOT_Vertex(1);
static GenSubObjType SOT_Edge(2);
static GenSubObjType SOT_Face(4);

class AlembicXFormModifier : public Modifier {
public:
	IParamBlock2 *pblock;

	static IObjParam *ip;
	static AlembicXFormModifier *editMod;

	AlembicXFormModifier();

	// From Animatable
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Exocortex Alembic Xform"); }  
	virtual Class_ID ClassID() { return EXOCORTEX_ALEMBIC_XFORM_MODIFIER_ID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Exocortex Alembic Xform"); }

	// From modifier
	ChannelMask ChannelsUsed()  { return TM_CHANNEL; }
	ChannelMask ChannelsChanged() { return TM_CHANNEL; }
	Class_ID InputType() { return defObjectClassID; }
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
    Interval m_CurrentAlembicInterval;
public:
	void SetAlembicId(const std::string &file, const std::string &identifier);
};

//--- ClassDescriptor and class vars ---------------------------------

IObjParam *AlembicXFormModifier::ip                 = NULL;
AlembicXFormModifier *AlembicXFormModifier::editMod         = NULL;

class AlembicXFormModifierClassDesc : public ClassDesc2 {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new AlembicXFormModifier; }
	const TCHAR *	ClassName() { return _T("ExoCortex Alembic XForm"); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return EXOCORTEX_ALEMBIC_XFORM_MODIFIER_ID; }
	const TCHAR* 	Category() { return _T("MAX STANDARD"); }
	const TCHAR*	InternalName() { return _T("AlembicXFormModifier"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

static AlembicXFormModifierClassDesc convertToMeshDesc;
ClassDesc* GetAlembicXFormModifierDesc() {return &convertToMeshDesc;}

// Parameter block IDs:
// Blocks themselves:
enum { turn_params };
// Parameters in first block:
enum { turn_use_invis, turn_sel_type, turn_softsel, turn_sel_level };

static ParamBlockDesc2 turn_param_desc ( turn_params, _T("ExoCortexAlembicXFormModifier"),
									IDS_PARAMETERS, &convertToMeshDesc,
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

AlembicXFormModifier::AlembicXFormModifier() {
	pblock = NULL;
	GetAlembicXFormModifierDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicXFormModifier::Clone(RemapDir& remap) {
	AlembicXFormModifier *mod = new AlembicXFormModifier();
	mod->ReplaceReference (0, remap.CloneRef(pblock));
	BaseClone(this, mod, remap);
	return mod;
}

IParamBlock2 *AlembicXFormModifier::GetParamBlockByID (short id) {
	return (pblock->ID() == id) ? pblock : NULL; 
}

Interval AlembicXFormModifier::GetValidity (TimeValue t) 
{
	// Interval ret = FOREVER;
	// pblock->GetValidity (t, ret);
    Interval ret(t, t);
	return ret;
}

void AlembicXFormModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
	Interval ivalid=os->obj->ObjectValidity (t);

    Alembic::AbcGeom::IObject iObj = getObjectFromArchive(m_AlembicNodeProps.m_File, m_AlembicNodeProps.m_Identifier);
    
    if(!iObj.valid())
        return;

    Alembic::AbcGeom::IXform obj(iObj,Alembic::Abc::kWrapExisting);

    if(!obj.valid())
        return;

    // Calculate the sample time
    double sampleTime = GetSecondsFromTimeValue(t);

    SampleInfo sampleInfo = getSampleInfo(
        sampleTime,
        obj.getSchema().getTimeSampling(),
        obj.getSchema().getNumSamples()
        );

    Alembic::AbcGeom::XformSample sample;
    obj.getSchema().get(sample,sampleInfo.floorIndex);
    Alembic::Abc::M44d matrix = sample.getMatrix();

    // blend - need to reconfigure this possibly, doesn't seem to make sense to lerp a matrix
    /*if(sampleInfo.alpha != 0.0)
    {
        obj.getSchema().get(sample,sampleInfo.ceilIndex);
        Alembic::Abc::M44d ceilMatrix = sample.getMatrix();
        matrix = (1.0 - sampleInfo.alpha) * matrix + sampleInfo.alpha * ceilMatrix;
    }
    */

    Matrix3 objMatrix(
        Point3(matrix.getValue()[0], matrix.getValue()[1], matrix.getValue()[2]),
        Point3(matrix.getValue()[4], matrix.getValue()[5], matrix.getValue()[6]),
        Point3(matrix.getValue()[8], matrix.getValue()[9], matrix.getValue()[10]),
        Point3(matrix.getValue()[12], matrix.getValue()[13], matrix.getValue()[14]));

    Interval alembicValid(t, t); 
    ivalid = alembicValid;
    
    // set matrix explicitly 
    os->obj->UpdateValidity(TM_CHANNEL, ivalid);
    os->ApplyTM( &objMatrix, ivalid );
}

void AlembicXFormModifier::BeginEditParams (IObjParam  *ip, ULONG flags, Animatable *prev) {
	this->ip = ip;	
	editMod  = this;

	// throw up all the appropriate auto-rollouts
	convertToMeshDesc.BeginEditParams(ip, this, flags, prev);

	// Necessary?
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void AlembicXFormModifier::EndEditParams (IObjParam *ip,ULONG flags,Animatable *next) {
	convertToMeshDesc.EndEditParams(ip, this, flags, next);
	this->ip = NULL;
	editMod  = NULL;
}

RefResult AlembicXFormModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
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

void AlembicXFormModifier::SetAlembicId(const std::string &file, const std::string &identifier)
{
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}

/*bool AlembicXFormModifier::ChangesSelType()
{
	int selLevel;
	Interval ivalid = FOREVER;
	pblock->GetValue (turn_sel_level, GetCOREInterface()->GetTime(), selLevel, ivalid);
	
	if(selLevel == 0) 
		return false;
	else 
		return true;
}

*/

int AlembicImport_XForm(const std::string &file, const std::string &identifier, alembic_importoptions &options)
{
    // Find the object in the archive
	Alembic::AbcGeom::IObject iObj = getObjectFromArchive(file,identifier);
	if(!iObj.valid())
		return alembic_failure;

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

        Alembic::AbcGeom::IXform objxForm(iObj,Alembic::Abc::kWrapExisting);

        if(!objxForm.valid())
            return alembic_failure;

        SampleInfo sampleInfo = getSampleInfo(
            0,
            objxForm.getSchema().getTimeSampling(),
            objxForm.getSchema().getNumSamples()
            );

        Alembic::AbcGeom::XformSample sample;
        objxForm.getSchema().get(sample,sampleInfo.floorIndex);
        const Alembic::Abc::Box3d &box3d = sample.getChildBounds();

        // Hard code these values for now
        pDummy->SetColor(Point3(0.6f, 0.8f, 1.0f));

        Box3 box(Point3(box3d.min.x, box3d.min.y, box3d.min.z), 
            Point3(box3d.max.x, box3d.max.y, box3d.max.z));
        pDummy->SetBox(box);

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
	AlembicXFormModifier *pModifier = static_cast<AlembicXFormModifier*>
		(GetCOREInterface()->CreateInstance(OSM_CLASS_ID, EXOCORTEX_ALEMBIC_XFORM_MODIFIER_ID));

	// Set the alembic id
	pModifier->SetAlembicId(file, identifier);

	// Add the modifier to the node
	GetCOREInterface12()->AddModifier(*pNode, *pModifier);

	return alembic_success;
}



