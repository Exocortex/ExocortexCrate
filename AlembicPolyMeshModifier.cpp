#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicPolyMeshModifier.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include "iparamb2.h"
#include "alembic.h"

void AlembicImport_FillInPolyMesh(Alembic::AbcGeom::IObject &iObj, TriObject *triobj, double iFrame, AlembicDataFillFlags flags);

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

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(m_AlembicNodeProps.m_File, m_AlembicNodeProps.m_Identifier);
   if(!iObj.valid())
      return;

   // Calculate the sample time in seconds
   double sampleTime = double(t) / ALEMBIC_3DSMAX_TICK_VALUE;

   AlembicImport_FillInPolyMesh(iObj, (TriObject*)os->obj, sampleTime, ALEMBIC_DATAFILL_POLYMESH_UPDATE);
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
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}

void AlembicImport_FillInPolyMesh(Alembic::AbcGeom::IObject &iObj, TriObject *triobj, double iFrame, AlembicDataFillFlags flags)
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
         iFrame,
         objMesh.getSchema().getTimeSampling(),
         objMesh.getSchema().getNumSamples()
      );
   else
      sampleInfo = getSampleInfo(
         iFrame,
         objSubD.getSchema().getTimeSampling(),
         objSubD.getSchema().getNumSamples()
      );

   Mesh &mesh = triobj->GetMesh();
   Alembic::AbcGeom::IPolyMeshSchema::Sample polyMeshSample;
   Alembic::AbcGeom::ISubDSchema::Sample subDSample;

   if(objMesh.valid())
       objMesh.getSchema().get(polyMeshSample,sampleInfo.floorIndex);
   else
       objSubD.getSchema().get(subDSample,sampleInfo.floorIndex);

   if ( flags & ALEMBIC_DATAFILL_VERTEX_IMPORT || flags & ALEMBIC_DATAFILL_VERTEX_UPDATE )
   {
	   Alembic::Abc::P3fArraySamplePtr meshPos;

       if(objMesh.valid())
           meshPos = polyMeshSample.getPositions();
       else
           meshPos = subDSample.getPositions();

       if (flags & ALEMBIC_DATAFILL_VERTEX_IMPORT)
       {
           mesh.setNumVerts((int)meshPos->size());
       }
       else if ( flags & ALEMBIC_DATAFILL_VERTEX_UPDATE)
       {
           if(mesh.getNumVerts() != meshPos->size())
               return;
       }

	   std::vector<Point3> vArray;
	   vArray.reserve(meshPos->size());
	   for(size_t i=0;i<meshPos->size();i++)
		  vArray.push_back(Point3(meshPos->get()[i].x,meshPos->get()[i].y,meshPos->get()[i].z));

	   // blend
	   if(sampleInfo.alpha != 0.0)
	   {
		  if(objMesh.valid())
		  {
			 Alembic::AbcGeom::IPolyMeshSchema::Sample polyMeshSample2;
			 objMesh.getSchema().get(polyMeshSample2,sampleInfo.ceilIndex);
			 meshPos = polyMeshSample2.getPositions();
		  }
		  else
		  {
			 Alembic::AbcGeom::ISubDSchema::Sample subDSample2;
			 objSubD.getSchema().get(subDSample2,sampleInfo.ceilIndex);
			 meshPos = subDSample2.getPositions();
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

   if ( flags & ALEMBIC_DATAFILL_FACELIST )
   {
		Alembic::Abc::Int32ArraySamplePtr meshFaceCount;
		Alembic::Abc::Int32ArraySamplePtr meshFaceIndices;

		if(objMesh.valid())
		{
            meshFaceCount = polyMeshSample.getFaceCounts();
            meshFaceIndices = polyMeshSample.getFaceIndices();
		}
		else
		{
            meshFaceCount = subDSample.getFaceCounts();
            meshFaceIndices = subDSample.getFaceIndices();
		}

        // Count the number of triangles
        // Right now, we're only dealing with faces that are triangles, but we may have to account for higher primitives later
        int numTriangles = 0;
        for (size_t i = 0; i < meshFaceCount->size(); i += 1)
        {
            if( meshFaceCount->get()[i] != 3)
                continue;

            numTriangles += 1;
        }

        // Set up the index buffer
        mesh.setNumFaces(numTriangles);
        for (size_t i = 0; i < meshFaceCount->size(); i += 1)
        {
            if( meshFaceCount->get()[i] != 3)
                continue;

            // three vertex indices of a triangle
            int v0 = meshFaceIndices->get()[i*3];
            int v1 =  meshFaceIndices->get()[i*3+1];
            int v2 =  meshFaceIndices->get()[i*3+2];

            // vertex positions
            Face &face = mesh.faces[i];
            face.setMatID(1);
            face.setEdgeVisFlags(1, 1, 1);
            face.setVerts(v0, v1, v2);
        }

        mesh.InvalidateGeomCache();
        mesh.InvalidateTopologyCache();
   }
}

int AlembicImport_PolyMesh(const std::string &file, const std::string &identifier, alembic_importoptions &options)
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
	AlembicImport_FillInPolyMesh(iObj, triobj, 0, ALEMBIC_DATAFILL_POLYMESH_IMPORT);

	// Create the object node
	INode *node = GetCOREInterface12()->CreateObjectNode(triobj, iObj.getName().c_str());
	if (!node)
		return alembic_failure;

	// Create the polymesh modifier
	AlembicPolyMeshModifier *pModifier = static_cast<AlembicPolyMeshModifier*>
		(GetCOREInterface()->CreateInstance(OSM_CLASS_ID, EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID));

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
