#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicPolyMeshModifier.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include "iparamb2.h"
#include "alembic.h"
#include "MeshNormalSpec.h"
#include "AlembicXForm.h"
#include "assetmanagement\AssetType.h"
#include "AlembicVisCtrl.h"

using namespace MaxSDK::AssetManagement;
const int POLYMESHMOD_MAX_PARAM_BLOCKS = 2;


typedef struct _alembic_fillmesh_options
{
    Alembic::AbcGeom::IObject *pIObj;
    TriObject *pTriObj;
    PolyObject *pPolyObj;
    TimeValue dTicks;
    AlembicDataFillFlags nDataFillFlags;

    _alembic_fillmesh_options()
    {
        pIObj = 0;
        pTriObj = 0;
        pPolyObj = 0;
        dTicks = 0;
        nDataFillFlags = 0;
    }
} alembic_fillmesh_options;

void AlembicImport_FillInPolyMesh(alembic_fillmesh_options &options);

class AlembicPolyMeshModifier : public Modifier {
public:
	IParamBlock2 *pblock[POLYMESHMOD_MAX_PARAM_BLOCKS];

	static IObjParam *ip;
	static AlembicPolyMeshModifier *editMod;

	AlembicPolyMeshModifier();

	// From Animatable
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic PolyMesh"); }  
	virtual Class_ID ClassID() { return EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Alembic PolyMesh"); }

	// From modifier
	ChannelMask ChannelsUsed()  { return TOPO_CHANNEL|GEOM_CHANNEL|TEXMAP_CHANNEL; }
	ChannelMask ChannelsChanged() { return TOPO_CHANNEL|GEOM_CHANNEL|TEXMAP_CHANNEL; }
	Class_ID InputType() { return polyObjectClassID; }
	void ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node);
	Interval LocalValidity(TimeValue t) { return GetValidity(t); }
	Interval GetValidity (TimeValue t);
	BOOL DependOnTopology(ModContext &mc) { return TRUE; }

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;} 
	void BeginEditParams(IObjParam  *ip, ULONG flags,Animatable *prev);
	void EndEditParams(IObjParam *ip,ULONG flags,Animatable *next);		

	int NumParamBlocks () { return POLYMESHMOD_MAX_PARAM_BLOCKS; }
	IParamBlock2 *GetParamBlock (int i) { return pblock[i]; }
	IParamBlock2 *GetParamBlockByID (short id);

	int NumRefs() { return POLYMESHMOD_MAX_PARAM_BLOCKS; }
	RefTargetHandle GetReference(int i) { return pblock[i]; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { pblock[i] = (IParamBlock2 *) rtarg; }
public:

	int NumSubs() {return POLYMESHMOD_MAX_PARAM_BLOCKS;}
	Animatable* SubAnim(int i) { return GetReference(i); }
	TSTR SubAnimName(int i);

	RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, 
		PartID& partID, RefMessage message);
private:
    alembic_nodeprops m_AlembicNodeProps;
public:
	void SetAlembicId(const std::string &file, const std::string &identifier);
    void SetAlembicUpdateDataFillFlags(unsigned int nFlags) { m_AlembicNodeProps.m_UpdateDataFillFlags = nFlags; }
    const std::string &GetAlembicArchive() { return m_AlembicNodeProps.m_File; }
    const std::string &GetAlembicObjectId() { return m_AlembicNodeProps.m_Identifier; }
};
//--- ClassDescriptor and class vars ---------------------------------

IObjParam *AlembicPolyMeshModifier::ip              = NULL;
AlembicPolyMeshModifier *AlembicPolyMeshModifier::editMod         = NULL;

class AlembicPolyMeshModifierClassDesc : public ClassDesc2 {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new AlembicPolyMeshModifier; }
	const TCHAR *	ClassName() { return _T("Alembic PolyMesh"); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID; }
	const TCHAR* 	Category() { return _T("Exocortex Alembic for 3DS Max"); }
	const TCHAR*	InternalName() { return _T("AlembicPolyMeshModifier"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

static AlembicPolyMeshModifierClassDesc AlembicPolyMeshModifierDesc;
ClassDesc* GetAlembicPolyMeshModifierDesc() {return &AlembicPolyMeshModifierDesc;}

//--- Properties block -------------------------------

// Parameter block IDs:
// Blocks themselves:
enum { polymesh_props = 0 };

// Parameters in first block:
enum 
{ 
    polymesh_props_muted,
    polymesh_props_time,
};

static ParamBlockDesc2 polymesh_props_desc ( polymesh_props, _T("AlembicPolyMeshModifier"),
									IDS_PROPS, &AlembicPolyMeshModifierDesc,
									P_AUTO_CONSTRUCT | P_AUTO_UI, REF_PBLOCK,
	// rollout description
	IDD_ALEMBIC_PROPS, IDS_PROPS, 0, 0, NULL,

    // params
	polymesh_props_muted, _T("propsMuted"), TYPE_BOOL, P_RESET_DEFAULT, IDS_MUTED,
		p_ui, TYPE_CHECKBUTTON, IDC_CHECK_MUTED,
		end,
        
	polymesh_props_time, _T("propsTime"), TYPE_INT, P_RESET_DEFAULT, IDS_TIME,
		p_default, 1,
		p_range, 0, 1000000,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_EDIT_TIME, IDC_SPIN_TIME, 1,
		end,
        	
    end
);

//--- Preview Param block -------------------------------

// Parameter block IDs:
// Blocks themselves:
enum { polymesh_preview = 1 };

// Parameters in first block:
enum 
{ 
    polymesh_preview_abc_archive,
    polymesh_preview_abc_id,
};


class PBPolyMesh_Preview_Accessor : public PBAccessor
{
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		AlembicPolyMeshModifier *polymeshMod = (AlembicPolyMeshModifier*) owner;
		switch(id)
		{
        case polymesh_preview_abc_archive:
            {
                const char *strArchive = polymeshMod->GetAlembicArchive().c_str();
                polymeshMod->GetParamBlock(polymesh_preview)->SetValue(polymesh_preview_abc_archive, t, strArchive);
            }
            break;
        case polymesh_preview_abc_id:
            {
                const char *strObjectId = polymeshMod->GetAlembicObjectId().c_str();
                polymeshMod->GetParamBlock(polymesh_preview)->SetValue(polymesh_preview_abc_archive, t, strObjectId);
            }
            break;
		default: 
            break;
		}
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());

	}
};

static PBPolyMesh_Preview_Accessor polymesh_preview_accessor;

static ParamBlockDesc2 polymesh_preview_desc ( polymesh_preview, _T("AlembicPolyMeshModifier"),
									IDS_PREVIEW, &AlembicPolyMeshModifierDesc,
									P_AUTO_CONSTRUCT | P_AUTO_UI, REF_PBLOCK1,
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

    polymesh_preview_abc_archive, _T("previewAbcArchive"), TYPE_STRING, P_RESET_DEFAULT|P_ANIMATABLE, IDS_ABC_ARCHIVE,
        p_ui, TYPE_EDITBOX, IDC_ABC_ARCHIVE,
        p_accessor,		&polymesh_preview_accessor,
        end,

	polymesh_preview_abc_id, _T("previewAbcId"), TYPE_STRING, P_RESET_DEFAULT|P_ANIMATABLE, IDS_ABC_ID,
		p_ui, TYPE_EDITBOX, IDC_ABC_OBJECTID,
        p_accessor,		&polymesh_preview_accessor,
		end,
	end
);

//--- Render Param block -------------------------------

// Parameter block IDs:
// Blocks themselves:
//enum { polymesh_render = 2 };
//
//// Parameters in first block:
//enum 
//{ 
//    polymesh_render_abc_archive,
//    polymesh_render_abc_id,
//};
//
//
//class PBPolyMesh_Render_Accessor : public PBAccessor
//{
//	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
//	{
//		// CubeMap *map = (CubeMap*) owner;
//		switch(id)
//		{
//		case polymesh_render_abc_archive: 
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
//static PBPolyMesh_Render_Accessor polymesh_render_accessor;
//
//static ParamBlockDesc2 polymesh_render_desc ( polymesh_render, _T("ExoCortexAlembicPolyMeshModifier"),
//									IDS_RENDER, &AlembicPolyMeshModifierDesc,
//									P_AUTO_CONSTRUCT | P_AUTO_UI, REF_PBLOCK2,
//	// rollout description
//	IDD_ALEMBIC_ID_PARAMS, IDS_RENDER, 0, 0, NULL,
//
//    // params
//	/*polymesh_preview_abc_archive, _T("previewAbcArchive"), TYPE_FILENAME, 0, IDS_ABC_ARCHIVE,
//		p_ui, TYPE_FILEOPENBUTTON, IDC_ABC_ARCHIVE,
//        p_caption, IDS_OPEN_ABC_CAPTION,
//        p_file_types, IDS_ABC_FILE_TYPE,
//        p_accessor,		&polymesh_preview_accessor,
//		end,
//        */
//
//    polymesh_preview_abc_archive, _T("renderAbcArchive"), TYPE_STRING, P_RESET_DEFAULT, IDS_ABC_ARCHIVE,
//        p_ui, TYPE_EDITBOX, IDC_ABC_ARCHIVE,
//        end,
//
//	polymesh_preview_abc_id, _T("renderAbcId"), TYPE_STRING, P_RESET_DEFAULT, IDS_ABC_ID,
//		p_ui, TYPE_EDITBOX, IDC_ABC_OBJECTID,
//		end,
//	end
//);

//--- Modifier methods -------------------------------

AlembicPolyMeshModifier::AlembicPolyMeshModifier() 
{
    for (int i = 0; i < POLYMESHMOD_MAX_PARAM_BLOCKS; i += 1)
	    pblock[i] = NULL;

	GetAlembicPolyMeshModifierDesc()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicPolyMeshModifier::Clone(RemapDir& remap) 
{
	AlembicPolyMeshModifier *mod = new AlembicPolyMeshModifier();

    for (int i = 0; i < POLYMESHMOD_MAX_PARAM_BLOCKS; i += 1)
	    mod->ReplaceReference (i, remap.CloneRef(pblock[i]));
	
    BaseClone(this, mod, remap);
	return mod;
}

IParamBlock2 *AlembicPolyMeshModifier::GetParamBlockByID (short id) 
{
    for (int i = 0; i < POLYMESHMOD_MAX_PARAM_BLOCKS; i += 1)
    {
        if (pblock[i] && pblock[i]->ID() == id)
            return pblock[i];
    }

    return NULL;
}

Interval AlembicPolyMeshModifier::GetValidity (TimeValue t) {
	// Interval ret = FOREVER;
	// pblock->GetValidity (t, ret);

    // PeterM this will need to be rethought out
    Interval ret(t,t);
	return ret;
}

void AlembicPolyMeshModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node) 
{
   Interval ivalid=os->obj->ObjectValidity(t);

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(m_AlembicNodeProps.m_File, m_AlembicNodeProps.m_Identifier);
   if(!iObj.valid())
      return;

   alembic_fillmesh_options options;
   options.pIObj = &iObj;
   options.pTriObj = NULL;
   options.pPolyObj = NULL;
   options.dTicks = t;
   options.nDataFillFlags = m_AlembicNodeProps.m_UpdateDataFillFlags;

   // Find out if we are modifying a poly object or a tri object
   if (os->obj->CanConvertToType(Class_ID(POLYOBJ_CLASS_ID, 0)))
   {
      options.pPolyObj = reinterpret_cast<PolyObject *>(os->obj->ConvertToType(t, Class_ID(POLYOBJ_CLASS_ID, 0)));
    
      if (os->obj != options.pPolyObj)
          os->obj = options.pPolyObj;

   }
   else if (os->obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
   {
      options.pTriObj = reinterpret_cast<TriObject *>(os->obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0)));

       if (os->obj != options.pTriObj)
          os->obj = options.pTriObj;
   }

   AlembicImport_FillInPolyMesh(options);

   Interval alembicValid(t, t); 
   ivalid = alembicValid;

   // update the validity channel
   os->obj->UpdateValidity(TOPO_CHAN_NUM, ivalid);
   os->obj->UpdateValidity(GEOM_CHAN_NUM, ivalid);
   os->obj->UpdateValidity(TEXMAP_CHAN_NUM, ivalid);
}

void AlembicPolyMeshModifier::BeginEditParams (IObjParam  *ip, ULONG flags, Animatable *prev) {
	this->ip = ip;	
	editMod  = this;

	// throw up all the appropriate auto-rollouts
	AlembicPolyMeshModifierDesc.BeginEditParams(ip, this, flags, prev);

    /*TimeValue t = GetCOREInterface()->GetTime();
    const char *strArchive = GetAlembicArchive().c_str();
    GetParamBlock(polymesh_preview)->SetValue(polymesh_preview_abc_archive, t, strArchive);

    const char *strObjectId = GetAlembicObjectId().c_str();
    GetParamBlock(polymesh_preview)->SetValue(polymesh_preview_abc_archive, t, strObjectId);
    */

	// Necessary?
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void AlembicPolyMeshModifier::EndEditParams (IObjParam *ip,ULONG flags,Animatable *next) 
{
	AlembicPolyMeshModifierDesc.EndEditParams(ip, this, flags, next);
	this->ip = NULL;
	editMod  = NULL;
}

RefResult AlembicPolyMeshModifier::NotifyRefChanged (Interval changeInt, RefTargetHandle hTarget,
										   PartID& partID, RefMessage message) {
	switch (message) 
    {
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
        for (int i = 0; i < POLYMESHMOD_MAX_PARAM_BLOCKS; i += 1)
        {
		    polymesh_props_desc.InvalidateUI(pblock[i]->LastNotifyParamID());
            polymesh_preview_desc.InvalidateUI(pblock[i]->LastNotifyParamID());
            // polymesh_render_desc.InvalidateUI(pblock[i]->LastNotifyParamID());
        }
		break;

    case REFMSG_WANT_SHOWPARAMLEVEL:

        break;
	}

	return REF_SUCCEED;
}

void AlembicPolyMeshModifier::SetAlembicId(const std::string &file, const std::string &identifier)
{
    m_AlembicNodeProps.m_File = file;
    m_AlembicNodeProps.m_Identifier = identifier;
}

TSTR AlembicPolyMeshModifier::SubAnimName(int i)
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

void AlembicImport_FillInPolyMesh(alembic_fillmesh_options &options)
{
   Alembic::AbcGeom::IPolyMesh objMesh;
   Alembic::AbcGeom::ISubD objSubD;

   if(Alembic::AbcGeom::IPolyMesh::matches((*options.pIObj).getMetaData()))
       objMesh = Alembic::AbcGeom::IPolyMesh(*options.pIObj,Alembic::Abc::kWrapExisting);
   else
       objSubD = Alembic::AbcGeom::ISubD(*options.pIObj,Alembic::Abc::kWrapExisting);

   if(!objMesh.valid() && !objSubD.valid())
       return;

  double sampleTime = GetSecondsFromTimeValue(options.dTicks);

  SampleInfo sampleInfo;
   if(objMesh.valid())
      sampleInfo = getSampleInfo(
         sampleTime,
         objMesh.getSchema().getTimeSampling(),
         objMesh.getSchema().getNumSamples()
      );
   else
      sampleInfo = getSampleInfo(
         sampleTime,
         objSubD.getSchema().getTimeSampling(),
         objSubD.getSchema().getNumSamples()
      );

   Alembic::AbcGeom::IPolyMeshSchema::Sample polyMeshSample;
   Alembic::AbcGeom::ISubDSchema::Sample subDSample;

   if(objMesh.valid())
       objMesh.getSchema().get(polyMeshSample,sampleInfo.floorIndex);
   else
       objSubD.getSchema().get(subDSample,sampleInfo.floorIndex);

   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_VERTEX )
   {
	   Alembic::Abc::P3fArraySamplePtr meshPos;
       Alembic::Abc::V3fArraySamplePtr meshVel;

       bool hasDynamicTopo = false;
       if(objMesh.valid())
       {
           meshPos = polyMeshSample.getPositions();
           meshVel = polyMeshSample.getVelocities();

           Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objMesh.getSchema(),".faceCounts");
           if(faceCountProp.valid())
               hasDynamicTopo = !faceCountProp.isConstant();
       }
       else
       {
           meshPos = subDSample.getPositions();
           meshVel = subDSample.getVelocities();

           Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objSubD.getSchema(),".faceCounts");
           if(faceCountProp.valid())
               hasDynamicTopo = !faceCountProp.isConstant();
       }

       int currentNumVerts = (options.pPolyObj != NULL) ? options.pPolyObj->GetMesh().numv :
           (options.pTriObj != NULL) ? options.pTriObj->GetMesh().getNumVerts() : 0;

       if (currentNumVerts != meshPos->size())
       {
           int numVerts = static_cast<int>(meshPos->size());
           if (options.pPolyObj != NULL)
           {
               options.pPolyObj->GetMesh().setNumVerts(numVerts);
           }
           if (options.pTriObj != NULL)
           {
               options.pTriObj->GetMesh().setNumVerts(numVerts);
           }
       } 

       std::vector<Point3> vArray;
	   vArray.reserve(meshPos->size());
	   for(size_t i=0;i<meshPos->size();i++)
		  vArray.push_back(Point3(meshPos->get()[i].x,meshPos->get()[i].y,meshPos->get()[i].z));

	   // blend - either between samples or using point velocities
	   if(sampleInfo.alpha != 0.0)
	   {
           bool bSampleInterpolate = false;
           bool bVelInterpolate = false;

		  if(objMesh.valid())
		  {
              Alembic::AbcGeom::IPolyMeshSchema::Sample polyMeshSample2;
              objMesh.getSchema().get(polyMeshSample2,sampleInfo.ceilIndex);
              meshPos = polyMeshSample2.getPositions();
             
              if(meshPos->size() == vArray.size() && !hasDynamicTopo)
                  bSampleInterpolate = true;
              else if(meshVel && meshVel->size() == meshPos->size())
                  bVelInterpolate = true;
          }
		  else
		  {
              Alembic::AbcGeom::ISubDSchema::Sample subDSample2;
              objSubD.getSchema().get(subDSample2,sampleInfo.ceilIndex);
              meshPos = subDSample2.getPositions();
              bSampleInterpolate = true;
		  }

          if (bSampleInterpolate)
          {
              for(size_t i=0;i<meshPos->size();i++)
              {	
                  vArray[i].x += (meshPos->get()[i].x - vArray[i].x) * (float)sampleInfo.alpha; 
                  vArray[i].y += (meshPos->get()[i].y - vArray[i].y) * (float)sampleInfo.alpha; 
                  vArray[i].z += (meshPos->get()[i].z - vArray[i].z) * (float)sampleInfo.alpha;
              }
          }
          else if (bVelInterpolate)
          {
              for(size_t i=0;i<meshVel->size();i++)
              {
                  vArray[i].x += meshVel->get()[i].x * (float)sampleInfo.alpha;
                  vArray[i].y += meshVel->get()[i].y * (float)sampleInfo.alpha;
                  vArray[i].z += meshVel->get()[i].z * (float)sampleInfo.alpha;
              }
          }
	   }

	   for(int i=0;i<vArray.size();i++)
       {
          Point3 maxPoint;
          ConvertAlembicPointToMaxPoint(vArray[i], maxPoint);

          int numVerts = static_cast<int>(meshPos->size());
          if (options.pPolyObj != NULL)
          {
             options.pPolyObj->GetMesh().V(i)->p.Set(maxPoint.x, maxPoint.y, maxPoint.z);
          }
          if (options.pTriObj != NULL)
          {
             options.pTriObj->GetMesh().setVert(i, maxPoint.x, maxPoint.y, maxPoint.z);
          }
       }
   }

   Alembic::Abc::Int32ArraySamplePtr meshFaceCount;
   Alembic::Abc::Int32ArraySamplePtr meshFaceIndices;

   if (objMesh.valid())
   {
       meshFaceCount = polyMeshSample.getFaceCounts();
       meshFaceIndices = polyMeshSample.getFaceIndices();
   }
   else
   {
       meshFaceCount = subDSample.getFaceCounts();
       meshFaceIndices = subDSample.getFaceIndices();
   }
   int numFaces = static_cast<int>(meshFaceCount->size());

   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_FACELIST )
   {
        // Set up the index buffer
        if (options.pPolyObj != NULL)
        {
            options.pPolyObj->GetMesh().setNumFaces(numFaces);
        }
        if (options.pTriObj != NULL)
        {
            options.pTriObj->GetMesh().setNumFaces(numFaces);
        }

        int offset = 0;
        for (int i = 0; i < numFaces; ++i)
        {
            int degree = meshFaceCount->get()[i];
            if (options.pPolyObj != NULL)
            {
                MNFace *pFace = options.pPolyObj->GetMesh().F(i);
                pFace->SetDeg(degree);
                pFace->material = 1;

                for (int j = degree - 1; j >= 0; --j)
                {
                    pFace->vtx[j] = meshFaceIndices->get()[offset];
                    ++offset;
                }
            }
            if (options.pTriObj != NULL && degree == 3)
            {
                // three vertex indices of a triangle
                int v2 =  meshFaceIndices->get()[i*3];
                int v1 =  meshFaceIndices->get()[i*3+1];
                int v0 =  meshFaceIndices->get()[i*3+2];

                // vertex positions
                Face &face = options.pTriObj->GetMesh().faces[i];
                face.setMatID(1);
                face.setEdgeVisFlags(1, 1, 1);
                face.setVerts(v0, v1, v2);
            }
        }
   }

   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_NORMALS && objMesh.valid() )
   {
       Alembic::AbcGeom::IN3fGeomParam meshNormalsParam = objMesh.getSchema().getNormalsParam();

       if(meshNormalsParam.valid())
       {
           Alembic::Abc::N3fArraySamplePtr meshNormalsFloor = meshNormalsParam.getExpandedValue(sampleInfo.floorIndex).getVals();
           std::vector<Point3> normalsToSet;
           normalsToSet.reserve(meshNormalsFloor->size());
           Alembic::Abc::N3fArraySamplePtr meshNormalsCeil = meshNormalsParam.getExpandedValue(sampleInfo.ceilIndex).getVals();

           // Blend
           if (sampleInfo.alpha != 0.0f && meshNormalsFloor->size() == meshNormalsCeil->size())
           {
               int offset = 0;
               for (int i = 0; i < numFaces; i += 1)
               {
                   int degree = meshFaceCount->get()[i];
                   int first = offset + degree - 1;
                   int last = offset;
                   for (int j = first; j >= last; j -= 1)
                   {
                       Point3 alembicNormal(meshNormalsFloor->get()[j].x, meshNormalsFloor->get()[j].y, meshNormalsFloor->get()[j].z);
                       Point3 ceilNormal(meshNormalsCeil->get()[j].x, meshNormalsCeil->get()[j].y, meshNormalsCeil->get()[j].z);
                       Point3 delta = (ceilNormal - alembicNormal) * float(sampleInfo.alpha);
                       alembicNormal += delta;
                       Point3 maxNormal;
                       ConvertAlembicNormalToMaxNormal(alembicNormal, maxNormal);
                       normalsToSet.push_back(maxNormal);
                   }

                   offset += degree;
               }
           }
           else
           {
               int offset = 0;
               for (int i = 0; i < numFaces; i += 1)
               {
                   int degree = meshFaceCount->get()[i];
                   int first = offset + degree - 1;
                   int last = offset;
                   for (int j = first; j >= last; j -=1)
                   {
                       Point3 alembicNormal(meshNormalsFloor->get()[j].x, meshNormalsFloor->get()[j].y, meshNormalsFloor->get()[j].z);
                       Point3 maxNormal;
                       ConvertAlembicNormalToMaxNormal(alembicNormal, maxNormal);
                       normalsToSet.push_back(maxNormal);
                   }

                   offset += degree;
               }
           }

           // Set up the specify normals
           if (options.pPolyObj != NULL)
           {
               options.pPolyObj->GetMesh().SpecifyNormals();
               MNNormalSpec *normalSpec = options.pPolyObj->GetMesh().GetSpecifiedNormals();
               normalSpec->ClearNormals();
               normalSpec->SetNumFaces(numFaces);
               normalSpec->SetFlag(MESH_NORMAL_MODIFIER_SUPPORT, true);
               normalSpec->SetNumNormals((int)normalsToSet.size());

               for (int i = 0; i < normalsToSet.size(); i += 1)
               {
                   normalSpec->Normal(i) = normalsToSet[i];
                   normalSpec->SetNormalExplicit(i, true);
               }

               // Set up the normal faces
               int offset = 0;
               for (int i =0; i < numFaces; i += 1)
               {
                   int degree = meshFaceCount->get()[i];

                   MNNormalFace &normalFace = normalSpec->Face(i);
                   normalFace.SetDegree(degree);
                   normalFace.SpecifyAll();

                   for (int j = 0; j < degree; ++j)
                   {
                       normalFace.SetNormalID(j, offset);
                       ++offset;
                   }
               }

               // Fill in any normals we may have not gotten specified.  Also allocates space for the RVert array
               // which we need for doing any normal vector queries
               normalSpec->CheckNormals();
               options.pPolyObj->GetMesh().checkNormals(TRUE);
           }

           if (options.pTriObj != NULL)
           {
               options.pTriObj->GetMesh().SpecifyNormals();
               MeshNormalSpec *normalSpec = options.pTriObj->GetMesh().GetSpecifiedNormals();
               normalSpec->ClearNormals();
               normalSpec->SetNumFaces(numFaces);
               normalSpec->SetFlag(MESH_NORMAL_MODIFIER_SUPPORT, true);
               normalSpec->SetNumNormals((int)normalsToSet.size());

               for (int i = 0; i < normalsToSet.size(); i += 1)
               {
                   normalSpec->Normal(i) = normalsToSet[i];
                   normalSpec->SetNormalExplicit(i, true);
               }

               // Set up the normal faces
               for (int i =0; i < numFaces; i += 1)
               {
                   MeshNormalFace &normalFace = normalSpec->Face(i);
                   normalFace.SpecifyAll();
                   normalFace.SetNormalID(0, i*3);
                   normalFace.SetNormalID(1, i*3+1);
                   normalFace.SetNormalID(2, i*3+2);
               }

               // Fill in any normals we may have not gotten specified.  Also allocates space for the RVert array
               // which we need for doing any normal vector queries
               normalSpec->CheckNormals();
               options.pTriObj->GetMesh().checkNormals(TRUE);
           }
       }
   }

   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_UVS )
   {
       Alembic::AbcGeom::IV2fGeomParam meshUvParam;
       if(objMesh.valid())
           meshUvParam = objMesh.getSchema().getUVsParam();
       else
           meshUvParam = objSubD.getSchema().getUVsParam();

       if(meshUvParam.valid())
       {
           SampleInfo sampleInfo = getSampleInfo(
               sampleTime,
               meshUvParam.getTimeSampling(),
               meshUvParam.getNumSamples()
               );

           Alembic::Abc::V2fArraySamplePtr meshUVsFloor = meshUvParam.getExpandedValue(sampleInfo.floorIndex).getVals();
           Alembic::Abc::V2fArraySamplePtr meshUVsCeil = meshUvParam.getExpandedValue(sampleInfo.ceilIndex).getVals();
           std::vector<Point3> uvsToSet;
           uvsToSet.reserve(meshUVsFloor->size());

           // Blend
           if (sampleInfo.alpha != 0.0f && meshUVsFloor->size() == meshUVsCeil->size())
           {
               int offset = 0;
               for (int i = 0; i < numFaces; i += 1)
               {
                   int degree = meshFaceCount->get()[i];
                   int first = offset + degree - 1;
                   int last = offset;
                   for (int j = first; j >= last; j -= 1)
                   {
                       Point3 floorUV(meshUVsFloor->get()[j].x, meshUVsFloor->get()[j].y, 0.0f);
                       Point3 ceilUV(meshUVsCeil->get()[j].x, meshUVsCeil->get()[j].y, 0.0f);
                       Point3 delta = (ceilUV - floorUV) * float(sampleInfo.alpha);
                       Point3 maxUV = floorUV + delta;
                       uvsToSet.push_back(maxUV);
                   }

                   offset += degree;
               }
           }
           else
           {
               int offset = 0;
               for (int i = 0; i < numFaces; i += 1)
               {
                   int degree = meshFaceCount->get()[i];
                   int first = offset + degree - 1;
                   int last = offset;
                   for (int j = first; j >= last; j -=1)
                   {
                       Point3 maxUV(meshUVsFloor->get()[j].x, meshUVsFloor->get()[j].y, 0.0f);
                       uvsToSet.push_back(maxUV);
                   }

                   offset += degree;
               }
           }


           // Set up the default texture map channel
           if (options.pPolyObj != NULL)
           {
               int numVertices = static_cast<int>(uvsToSet.size());

               options.pPolyObj->GetMesh().SetMapNum(2);
               options.pPolyObj->GetMesh().InitMap(0);
               options.pPolyObj->GetMesh().InitMap(1);
               MNMap *map = options.pPolyObj->GetMesh().M(1);
               map->setNumVerts(numVertices);
               map->setNumFaces(numFaces);

               for (int i = 0; i < numVertices; i += 1)
               {
                   map->v[i] = uvsToSet[i];
               }

               int offset = 0;
               for (int i =0; i < numFaces; i += 1)
               {
                   int degree = options.pPolyObj->GetMesh().F(i)->deg;
                   map->f[i].SetSize(degree);

                   for (int j = degree-1; j >= 0; j -= 1)
                   {
                       map->f[i].tv[j] = offset;
                       ++offset;
                   }
               }
           }
           
           if (options.pTriObj != NULL)
           {
               options.pTriObj->GetMesh().setNumMaps(2);
               options.pTriObj->GetMesh().setMapSupport(1, TRUE);
               MeshMap &map = options.pTriObj->GetMesh().Map(1);
               map.setNumVerts((int)uvsToSet.size());
               map.setNumFaces(numFaces);

               // Set the map texture vertices
               for (int i = 0; i < uvsToSet.size(); i += 1)
                    map.tv[i] = uvsToSet[i];

               // Set up the map texture faces
               for (int i =0; i < numFaces; i += 1)
                   map.tf[i].setTVerts(i*3, i*3+1, i*3+2);
           }
       }
   }

   if (options.pPolyObj != NULL)
   {
       options.pPolyObj->GetMesh().InvalidateGeomCache();
       options.pPolyObj->GetMesh().InvalidateTopoCache();

       if (!options.pPolyObj->GetMesh().GetFlag(MN_MESH_FILLED_IN ))
           options.pPolyObj->GetMesh().FillInMesh();
   }

   if (options.pTriObj != NULL)
   {
       options.pTriObj->GetMesh().InvalidateGeomCache();
       options.pTriObj->GetMesh().InvalidateTopologyCache();
   }
}

bool AlembicImport_IsPolyObject(Alembic::AbcGeom::IPolyMeshSchema::Sample &polyMeshSample)
{
    Alembic::Abc::Int32ArraySamplePtr meshFaceCount = polyMeshSample.getFaceCounts();

    // Go through each face and check the number of vertices.  If a face does not have 3 vertices,
    // we consider it to be a polymesh, otherwise it is a triangle mesh
    for (size_t i = 0; i < meshFaceCount->size(); ++i)
    {
        int vertexCount = meshFaceCount->get()[i];
        if (vertexCount != 3)
        {
            return true;
        }
    }

    return false;
}

int AlembicImport_PolyMesh(const std::string &file, const std::string &identifier, alembic_importoptions &options)
{
	// Find the object in the archive
	Alembic::AbcGeom::IObject iObj = getObjectFromArchive(file,identifier);
	if(!iObj.valid())
		return alembic_failure;

	// Fill in the mesh
    alembic_fillmesh_options dataFillOptions;
    dataFillOptions.pIObj = &iObj;
    dataFillOptions.pTriObj = NULL;
    dataFillOptions.pPolyObj = NULL;
    dataFillOptions.dTicks = GetCOREInterface12()->GetTime();

    dataFillOptions.nDataFillFlags = ALEMBIC_DATAFILL_VERTEX|ALEMBIC_DATAFILL_FACELIST;
    dataFillOptions.nDataFillFlags |= options.importNormals ? ALEMBIC_DATAFILL_NORMALS : 0;
    dataFillOptions.nDataFillFlags |= options.importUVs ? ALEMBIC_DATAFILL_UVS : 0;
    dataFillOptions.nDataFillFlags |= options.importBboxes ? ALEMBIC_DATAFILL_BOUNDINGBOX : 0;
    dataFillOptions.nDataFillFlags |= options.importClusters ? ALEMBIC_DATAFILL_FACESETS : 0;

    // Create the poly or tri object and place it in the scene
    // Need to use the attach to existing import flag here 
    Object *newObject = NULL;

    if (!Alembic::AbcGeom::IPolyMesh::matches(iObj.getMetaData()))
    {
        return alembic_failure;
    }

    Alembic::AbcGeom::IPolyMesh objMesh = Alembic::AbcGeom::IPolyMesh(iObj, Alembic::Abc::kWrapExisting);
    if (!objMesh.valid())
    {
        return alembic_failure;
    }

    // TODO: Do we need to check more than the first sample?
    Alembic::AbcGeom::IPolyMeshSchema::Sample polyMeshSample;
    objMesh.getSchema().get(polyMeshSample, 0);

    if (AlembicImport_IsPolyObject(polyMeshSample))
    {
		
        dataFillOptions.pPolyObj = (PolyObject *) GetPolyObjDescriptor()->Create();//CreateEditablePolyObject();
	    newObject = dataFillOptions.pPolyObj;
    }
    else
    {
        dataFillOptions.pTriObj = (TriObject *) GetTriObjDescriptor()->Create();//CreateNewTriObject();
	    newObject = dataFillOptions.pTriObj;
    }

    if (newObject == NULL)
    {
        return alembic_failure;
    }

	// we will not be filling in the initial polymesh data.
	//AlembicImport_FillInPolyMesh(dataFillOptions);

	// Create the object node
	INode *node = GetCOREInterface12()->CreateObjectNode(newObject, iObj.getName().c_str());
	if (!node)
		return alembic_failure;

	// Create the polymesh modifier
	AlembicPolyMeshModifier *pModifier = static_cast<AlembicPolyMeshModifier*>
		(GetCOREInterface12()->CreateInstance(OSM_CLASS_ID, EXOCORTEX_ALEMBIC_POLYMESH_MODIFIER_ID));

	// Set the alembic id
	pModifier->SetAlembicId(file, identifier);

    // Set the update data fill flags
    // PeterM TO DO:  We'll need to fix this after we determine if we have a dynamic topology mesh or not
    // For now, we'll just use the import flags
    unsigned int nUpdateDataFillFlags = dataFillOptions.nDataFillFlags;
    pModifier->SetAlembicUpdateDataFillFlags(nUpdateDataFillFlags);

	// Add the modifier to the node
	GetCOREInterface12()->AddModifier(*node, *pModifier);

    // Add the new inode to our current scene list
    SceneEntry *pEntry = options.sceneEnumProc.Append(node, newObject, OBTYPE_MESH, &std::string(iObj.getFullName())); 
    options.currentSceneList.Append(pEntry);

    // Set the visibility controller
    AlembicImport_SetupVisControl(iObj, node, options);

	return 0;
}
