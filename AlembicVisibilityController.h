#ifndef __ALEMBIC_VISIBILITY_CONTROLLER_H__
#define __ALEMBIC_VISIBILITY_CONTROLLER_H__

#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicDefinitions.h"
#include "dummy.h"
#include <ILockedTracks.h>
#include "iparamb2.h"
#include "AlembicNames.h"
#include "Max.h"
#include "resource.h"

ClassDesc2* GetAlembicVisibilityControllerClassDesc();

void AlembicImport_SetupVisControl( const std::string &file, const std::string &identifier, Alembic::AbcGeom::IObject &obj, INode *pNode, alembic_importoptions &options );


///////////////////////////////////////////////////////////////////////////////////////////////////
// alembic_fillvis_options
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _alembic_fillvis_options
{
    Alembic::AbcGeom::IObject *pIObj;
    TimeValue dTicks;
    bool bVisibility;
    bool bOldVisibility;

    _alembic_fillvis_options()
    {
        pIObj = 0;
        dTicks = 0;
        bVisibility = false;
        bOldVisibility = false;
    }
} alembic_fillvis_options;

void AlembicImport_FillInVis(alembic_fillvis_options &options);

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicVisibilityController Declaration
///////////////////////////////////////////////////////////////////////////////////////////////////
class AlembicVisibilityController : public LockableStdControl
{
public:
  
public:
	// Parameters in first block:
	enum 
	{ 
		ID_PATH,
		ID_IDENTIFIER,
		ID_TIME,
/*		ID_TOPOLOGY,
		ID_GEOMETRY,
		ID_GEOALPHA,
		ID_NORMALS,
		ID_UVS,*/
		ID_MUTED,
	};

	IParamBlock2* pblock;
    static IObjParam *ip;
    static AlembicVisibilityController *editMod;

	AlembicVisibilityController();
	~AlembicVisibilityController();

	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }

    void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic Visibility"); }  
	virtual Class_ID ClassID() { return ALEMBIC_VISIBILITY_CONTROLLER_CLASSID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Alembic Visibility"); }

    void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
    void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);

    int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
    IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
    IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	int NumRefs() { return 1; }
	void SetReference(int i, ReferenceTarget* pTarget) { if( i == 0 ) { pblock = (IParamBlock2*)pTarget; } }
	RefTargetHandle GetReference(int i) { return pblock; }
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

private:
    bool m_bOldVisibility;
    std::string m_CachedAbcFile;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicVisibilityControllerClassDesc
///////////////////////////////////////////////////////////////////////////////////////////////////
class AlembicVisibilityControllerClassDesc : public ClassDesc2
{
public:
	AlembicVisibilityControllerClassDesc() {};
	~AlembicVisibilityControllerClassDesc() {};

	// ClassDesc methods.  
	// Max calls these functions to figure out what kind of plugin this class represents

	// Return TRUE if the user can create this plug-in.
	int			IsPublic()			{ return TRUE; }	// We do want the user to see this plug-in

	// Return the class name of this plug-in
	const MCHAR* ClassName()		{ static const MSTR str("Alembic Visibility"); return str; }

	// Return the SuperClassID - this ID should
	// match the implementation of the interface returned
	// by Create.
	SClass_ID	SuperClassID()		{ return CTRL_FLOAT_CLASS_ID; }

	// Return the unique ID that identifies this class
	// This is required when saving.  Max stores the ClassID
	// reported by the actual plugin, and on reload it recreates
	// the appropriate class by matching the stored ClassID with
	// the matching ClassDesc
	//
	// You can generate random ClassID's using the gencid program
	// supplied with the Max SDK
	Class_ID	ClassID()			{ return ALEMBIC_VISIBILITY_CONTROLLER_CLASSID; }

	// If the plugin is an Object or Texture, this function returns
	// the category it can be assigned to.
	const MCHAR* Category()			{ return EXOCORTEX_ALEMBIC_CATEGORY; }

	// Return an instance of this plug-in.  Max will call this function
	// when it wants to start using our plug-in
	void* Create(BOOL loading=FALSE)
	{
		return new AlembicVisibilityController;
	}

    const TCHAR*	InternalName() { return _T("AlembicVisController"); }	// returns fixed parsable name (scripter-visible name)
    HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

#endif // __ALEMBIC_VISIBILITY_CONTROLLER_H__
