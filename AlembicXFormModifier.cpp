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
#include "AlembicXform.h"

static GenSubObjType SOT_Vertex(1);
static GenSubObjType SOT_Edge(2);
static GenSubObjType SOT_Face(4);

class AlembicXFormModifier : public Modifier 
{
public:
	IParamBlock2 *pblock[XFORMMOD_MAX_PARAM_BLOCKS];

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

	int NumParamBlocks () { return XFORMMOD_MAX_PARAM_BLOCKS; }
	IParamBlock2 *GetParamBlock (int i) { return pblock[i]; }
	IParamBlock2 *GetParamBlockByID(short id);

	int NumRefs() { return XFORMMOD_MAX_PARAM_BLOCKS; }
	RefTargetHandle GetReference(int i) { return pblock[i]; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { pblock[i] = (IParamBlock2 *) rtarg; }
public:

	int NumSubs() {return XFORMMOD_MAX_PARAM_BLOCKS;}
	Animatable* SubAnim(int i) { return GetReference(i); }
	TSTR SubAnimName(int i);

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
    const std::string &GetAlembicArchive() { return m_AlembicNodeProps.m_File; }
    const std::string &GetAlembicObjectId() { return m_AlembicNodeProps.m_Identifier; }
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

static AlembicXFormModifierClassDesc alembicXFormModifierDesc;
ClassDesc* GetAlembicXFormModifierDesc() {return &alembicXFormModifierDesc;}

//--- Properties block -------------------------------

// Parameter block IDs:
// Blocks themselves:
enum { xform_props = 0 };

// Parameters in first block:
enum 
{ 
    xform_props_muted,
    xform_props_time,
};

static ParamBlockDesc2 xform_props_desc ( xform_props, _T("ExoCortexAlembicXFormModifier"),
									IDS_PROPS, &alembicXFormModifierDesc,
									P_AUTO_CONSTRUCT | P_AUTO_UI, XFORM_REF_PBLOCK,
	// rollout description
	IDD_ALEMBIC_PROPS, IDS_PROPS, 0, 0, NULL,

    // params
	xform_props_muted, _T("propsMuted"), TYPE_BOOL, P_RESET_DEFAULT, IDS_MUTED,
		p_ui, TYPE_CHECKBUTTON, IDC_CHECK_MUTED,
		end,
        
	xform_props_time, _T("propsTime"), TYPE_INT, P_RESET_DEFAULT, IDS_TIME,
		p_default, 1,
		p_range, 0, 1000000,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_EDIT_TIME, IDC_SPIN_TIME, 1,
		end,
        	
    end
);

//--- Preview Param block -------------------------------

// Parameter block IDs:
// Blocks themselves:
enum { xform_preview = 1 };

// Parameters in first block:
enum 
{ 
    xform_preview_abc_archive,
    xform_preview_abc_id,
};


class PBxform_Preview_Accessor : public PBAccessor
{
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		AlembicXFormModifier *xformMod = (AlembicXFormModifier*) owner;
		switch(id)
		{
        case xform_preview_abc_archive:
            {
                const char *strArchive = xformMod->GetAlembicArchive().c_str();
                xformMod->GetParamBlock(xform_preview)->SetValue(xform_preview_abc_archive, t, strArchive);
            }
            break;
        case xform_preview_abc_id:
            {
                const char *strObjectId = xformMod->GetAlembicObjectId().c_str();
                xformMod->GetParamBlock(xform_preview)->SetValue(xform_preview_abc_archive, t, strObjectId);
            }
            break;
		default: 
            break;
		}
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());

	}
};

static PBxform_Preview_Accessor xform_preview_accessor;

static ParamBlockDesc2 xform_preview_desc ( xform_preview, _T("ExoCortexAlembicXFormModifier"),
									IDS_PREVIEW, &alembicXFormModifierDesc,
									P_AUTO_CONSTRUCT | P_AUTO_UI, XFORM_REF_PBLOCK1,
	//rollout description
	IDD_ALEMBIC_ID_PARAMS, IDS_PREVIEW, 0, 0, NULL,

    // params
	/*xform_preview_abc_archive, _T("previewAbcArchive"), TYPE_FILENAME, 0, IDS_ABC_ARCHIVE,
		p_ui, TYPE_FILEOPENBUTTON, IDC_ABC_ARCHIVE,
        p_caption, IDS_OPEN_ABC_CAPTION,
        p_file_types, IDS_ABC_FILE_TYPE,
        p_accessor,		&xform_preview_accessor,
		end,
        */

    xform_preview_abc_archive, _T("previewAbcArchive"), TYPE_STRING, P_RESET_DEFAULT|P_ANIMATABLE, IDS_ABC_ARCHIVE,
        p_ui, TYPE_EDITBOX, IDC_ABC_ARCHIVE,
        p_accessor,		&xform_preview_accessor,
        end,

	xform_preview_abc_id, _T("previewAbcId"), TYPE_STRING, P_RESET_DEFAULT|P_ANIMATABLE, IDS_ABC_ID,
		p_ui, TYPE_EDITBOX, IDC_ABC_OBJECTID,
        p_accessor,		&xform_preview_accessor,
		end,
	end
);

//--- Render Param block -------------------------------

// Parameter block IDs:
// Blocks themselves:
//enum { xform_render = 2 };
//
//// Parameters in first block:
//enum 
//{ 
//    xform_render_abc_archive,
//    xform_render_abc_id,
//};
//
//
//class PBxform_Render_Accessor : public PBAccessor
//{
//	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
//	{
//		// CubeMap *map = (CubeMap*) owner;
//		switch(id)
//		{
//		case xform_render_abc_archive: 
//			{
//				/*IAssetManager* assetMgr = IAssetManager::GetInstance();
//				if(assetMgr)
//				{
//					map->SetCubeMapFile(assetMgr->GetAsset(v.s,kBitmapAsset)); break;
//				}
//                */
//                break;
//			}
//		default: break;
//		}
//		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
//
//	}
//};
//
//static PBxform_Render_Accessor xform_render_accessor;
//
//static ParamBlockDesc2 xform_render_desc ( xform_render, _T("ExoCortexAlembicPolyMeshModifier"),
//									IDS_RENDER, &AlembicPolyMeshModifierDesc,
//									P_AUTO_CONSTRUCT | P_AUTO_UI, REF_PBLOCK2,
//	// rollout description
//	IDD_ALEMBIC_ID_PARAMS, IDS_RENDER, 0, 0, NULL,
//
//    // params
//	/*xform_preview_abc_archive, _T("previewAbcArchive"), TYPE_FILENAME, 0, IDS_ABC_ARCHIVE,
//		p_ui, TYPE_FILEOPENBUTTON, IDC_ABC_ARCHIVE,
//        p_caption, IDS_OPEN_ABC_CAPTION,
//        p_file_types, IDS_ABC_FILE_TYPE,
//        p_accessor,		&xform_preview_accessor,
//		end,
//        */
//
//    xform_preview_abc_archive, _T("renderAbcArchive"), TYPE_STRING, P_RESET_DEFAULT, IDS_ABC_ARCHIVE,
//        p_ui, TYPE_EDITBOX, IDC_ABC_ARCHIVE,
//        end,
//
//	xform_preview_abc_id, _T("renderAbcId"), TYPE_STRING, P_RESET_DEFAULT, IDS_ABC_ID,
//		p_ui, TYPE_EDITBOX, IDC_ABC_OBJECTID,
//		end,
//	end
//);

//--- Modifier methods -------------------------------

AlembicXFormModifier::AlembicXFormModifier() 
{
    for (int i = 0; i < XFORMMOD_MAX_PARAM_BLOCKS; i += 1)
        pblock[i] = NULL;

	GetAlembicXFormModifierDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicXFormModifier::Clone(RemapDir& remap) 
{
	AlembicXFormModifier *mod = new AlembicXFormModifier();

    for (int i = 0; i < XFORMMOD_MAX_PARAM_BLOCKS; i += 1)
        mod->ReplaceReference (i, remap.CloneRef(pblock[i]));

	BaseClone(this, mod, remap);
	return mod;
}

IParamBlock2 *AlembicXFormModifier::GetParamBlockByID (short id) 
{
    for (int i = 0; i < XFORMMOD_MAX_PARAM_BLOCKS; i += 1)
    {
        if (pblock[i] && pblock[i]->ID() == id)
            return pblock[i];
    }

    return NULL;
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
    
    AlembicDebug_PrintTransform(objMatrix);
    Matrix3 maxMatrix;
    ConvertAlembicMatrixToMaxMatrix(objMatrix, maxMatrix);
    AlembicDebug_PrintTransform(maxMatrix);

    Interval alembicValid(t, t); 
    ivalid = alembicValid;
    
    // set matrix explicitly 
    os->obj->UpdateValidity(TM_CHANNEL, ivalid);
    os->ApplyTM( &maxMatrix, ivalid );
}

void AlembicXFormModifier::BeginEditParams (IObjParam  *ip, ULONG flags, Animatable *prev) {
	this->ip = ip;	
	editMod  = this;

	// throw up all the appropriate auto-rollouts
	alembicXFormModifierDesc.BeginEditParams(ip, this, flags, prev);

	// Necessary?
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void AlembicXFormModifier::EndEditParams (IObjParam *ip,ULONG flags,Animatable *next) {
	alembicXFormModifierDesc.EndEditParams(ip, this, flags, next);
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
		/*if (pblock->LastNotifyParamID() == turn_sel_level) {
			// Notify stack that subobject info has changed:
			NotifyDependents(changeInt, partID, message);
			NotifyDependents(FOREVER, 0, REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
			return REF_STOP;
		}
        */
        for (int i = 0; i < XFORMMOD_MAX_PARAM_BLOCKS; i += 1)
        {
            xform_props_desc.InvalidateUI(pblock[i]->LastNotifyParamID());
            xform_preview_desc.InvalidateUI(pblock[i]->LastNotifyParamID());
            // polymesh_render_desc.InvalidateUI(pblock[i]->LastNotifyParamID());
        }
		break;
	}

	return REF_SUCCEED;
}

void AlembicXFormModifier::SetAlembicId(const std::string &file, const std::string &identifier)
{
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}

TSTR AlembicXFormModifier::SubAnimName(int i)
{
    if ( i == 0)
        return _T("IDS_PROPS");
    else if (i == 1)
        return _T("IDS_PREVIEW");
    else if (i == 2)
        return _T("IDS_RENDER");
    else
        return "";
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


