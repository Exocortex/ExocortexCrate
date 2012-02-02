/**********************************************************************
 *<
	FILE: ToMesh.cpp

	DESCRIPTION:  Convert to Mesh Modifier

	CREATED BY: Steve Anderson

	HISTORY: Created February 2000

 *>	Copyright (c) 2000 Autodesk, All Rights Reserved.
 **********************************************************************/
#include "alembic.h"
#include "AlembicPolyMeshModifier.h"
#include "AlembicArchiveStorage.h"

void AlembicImport_FillInPolyMesh(Alembic::AbcGeom::IObject &iObj, TriObject *triobj);

static GenSubObjType SOT_Vertex(1);
static GenSubObjType SOT_Edge(2);
static GenSubObjType SOT_Face(4);

class AlembicPolyMeshModifier : public Modifier {
public:
	IParamBlock2 *pblock;

	static IObjParam *ip;
	static AlembicPolyMeshModifier *editMod;

	AlembicPolyMeshModifier();

	// From Animatable
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Exocortex Alembic PolyMesh"); }  
	virtual Class_ID ClassID() { return EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Exocortex Alembic PolyMesh"); }

	// From modifier
	ChannelMask ChannelsUsed()  { return OBJ_CHANNELS; }
	ChannelMask ChannelsChanged() { return OBJ_CHANNELS; }
	Class_ID InputType() { return mapObjectClassID; }
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
	std::string m_AlembicArchiveFile;
	std::string m_AlembicIdentifier;
public:
	void SetAlembicId(const std::string &file, const std::string &identifier);
};
//--- ClassDescriptor and class vars ---------------------------------

IObjParam *AlembicPolyMeshModifier::ip              = NULL;
AlembicPolyMeshModifier *AlembicPolyMeshModifier::editMod         = NULL;

class AlembicPolyMeshModifierClassDesc : public ClassDesc2 {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new AlembicPolyMeshModifier; }
	const TCHAR *	ClassName() { return _T("ExoCortex Alembic PolyMesh"); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID; }
	const TCHAR* 	Category() { return _T("MAX STANDARD"); }
	const TCHAR*	InternalName() { return _T("AlembicPolyMeshModifier"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

static AlembicPolyMeshModifierClassDesc AlembicPolyMeshModifierDesc;
ClassDesc* GetAlembicPolyMeshModifierDesc() {return &AlembicPolyMeshModifierDesc;}

// Parameter block IDs:
// Blocks themselves:
enum { turn_params };
// Parameters in first block:
enum { turn_use_invis, turn_sel_type, turn_softsel, turn_sel_level };

static ParamBlockDesc2 turn_param_desc ( turn_params, _T("ExoCortexAlembicPolyMeshModifier"),
									IDS_PARAMETERS, &AlembicPolyMeshModifierDesc,
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

AlembicPolyMeshModifier::AlembicPolyMeshModifier() {
	pblock = NULL;
	GetAlembicPolyMeshModifierDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicPolyMeshModifier::Clone(RemapDir& remap) {
	AlembicPolyMeshModifier *mod = new AlembicPolyMeshModifier();
	mod->ReplaceReference (0, remap.CloneRef(pblock));
	BaseClone(this, mod, remap);
	return mod;
}

IParamBlock2 *AlembicPolyMeshModifier::GetParamBlockByID (short id) {
	return (pblock->ID() == id) ? pblock : NULL; 
}

Interval AlembicPolyMeshModifier::GetValidity (TimeValue t) {
	Interval ret = FOREVER;
	pblock->GetValidity (t, ret);
	return ret;
}

void AlembicPolyMeshModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
   Interval ivalid=os->obj->ObjectValidity(t);

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(m_AlembicArchiveFile,m_AlembicIdentifier);
   if(!iObj.valid())
      return;

   AlembicImport_FillInPolyMesh(iObj, (TriObject*)os->obj);
}

void AlembicPolyMeshModifier::BeginEditParams (IObjParam  *ip, ULONG flags, Animatable *prev) {
	this->ip = ip;	
	editMod  = this;

	// throw up all the appropriate auto-rollouts
	AlembicPolyMeshModifierDesc.BeginEditParams(ip, this, flags, prev);

	// Necessary?
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void AlembicPolyMeshModifier::EndEditParams (IObjParam *ip,ULONG flags,Animatable *next) {
	AlembicPolyMeshModifierDesc.EndEditParams(ip, this, flags, next);
	this->ip = NULL;
	editMod  = NULL;
}

RefResult AlembicPolyMeshModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
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

void AlembicPolyMeshModifier::SetAlembicId(const std::string &file, const std::string &identifier)
{
	m_AlembicArchiveFile = file;
	m_AlembicIdentifier = identifier;
}

void AlembicImport_FillInPolyMesh(Alembic::AbcGeom::IObject &iObj, TriObject *triobj)
{
   Alembic::AbcGeom::IPolyMesh objMesh;
   Alembic::AbcGeom::ISubD objSubD;
   if(Alembic::AbcGeom::IPolyMesh::matches(iObj.getMetaData()))
      objMesh = Alembic::AbcGeom::IPolyMesh(iObj,Alembic::Abc::kWrapExisting);
   else
      objSubD = Alembic::AbcGeom::ISubD(iObj,Alembic::Abc::kWrapExisting);
   if(!objMesh.valid() && !objSubD.valid())
      return;

   SampleInfo sampleInfo;
   if(objMesh.valid())
      sampleInfo = getSampleInfo(
         0,
         objMesh.getSchema().getTimeSampling(),
         objMesh.getSchema().getNumSamples()
      );
   else
      sampleInfo = getSampleInfo(
         0,
         objSubD.getSchema().getTimeSampling(),
         objSubD.getSchema().getNumSamples()
      );

   Alembic::Abc::P3fArraySamplePtr meshPos;
   if(objMesh.valid())
   {
      Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
      objMesh.getSchema().get(sample,sampleInfo.floorIndex);
      meshPos = sample.getPositions();
   }
   else
   {
      Alembic::AbcGeom::ISubDSchema::Sample sample;
      objSubD.getSchema().get(sample,sampleInfo.floorIndex);
      meshPos = sample.getPositions();
   }

   Mesh &mesh = triobj->GetMesh();

   if(mesh.getNumVerts() != meshPos->size())
      return;

   std::vector<Point3> vArray;
   vArray.reserve(meshPos->size());
   for(size_t i=0;i<meshPos->size();i++)
      vArray.push_back(Point3(meshPos->get()[i].x,meshPos->get()[i].y,meshPos->get()[i].z));

   // blend
   if(sampleInfo.alpha != 0.0)
   {
      if(objMesh.valid())
      {
         Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
         objMesh.getSchema().get(sample,sampleInfo.ceilIndex);
         meshPos = sample.getPositions();
      }
      else
      {
         Alembic::AbcGeom::ISubDSchema::Sample sample;
         objSubD.getSchema().get(sample,sampleInfo.ceilIndex);
         meshPos = sample.getPositions();
      }
      for(size_t i=0;i<meshPos->size();i++)
	  {	
		  vArray[i].x += (meshPos->get()[i].x - vArray[i].x) * (float)sampleInfo.alpha; 
		  vArray[i].x += (meshPos->get()[i].y - vArray[i].y) * (float)sampleInfo.alpha; 
		  vArray[i].x += (meshPos->get()[i].z - vArray[i].z) * (float)sampleInfo.alpha;
	  }
   }

   for(int i=0;i<meshPos->size();i++)
      mesh.setVert(i, vArray[i].x, vArray[i].y, vArray[i].z);
}

int AlembicImport_PolyMesh(const std::string &file, const std::string &identifier, alembic_importoptions options)
{
	// Find the object in the archive
	Alembic::AbcGeom::IObject iObj = getObjectFromArchive(file,identifier);
	if(!iObj.valid())
		return alembic_failure;

	// Create the tri object and place it in the scene
	TriObject *triobj = CreateNewTriObject();
    if (!triobj)
        return alembic_failure;

	// Fill in the mesh
	AlembicImport_FillInPolyMesh(iObj, triobj);

	// Create the object node
	INode *node = GetCOREInterface12()->CreateObjectNode(triobj, identifier.c_str());
	if (!node)
		return alembic_failure;

	// Create the polymesh modifier
	AlembicPolyMeshModifier *pModifier = static_cast<AlembicPolyMeshModifier*>
		(GetCOREInterface()->CreateInstance(OSM_CLASS_ID, EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID));

	// Set the alembic id
	pModifier->SetAlembicId(file, identifier);

	// Add the modifier to the node
	GetCOREInterface12()->AddModifier(*node, *pModifier);

	return 0;
}
