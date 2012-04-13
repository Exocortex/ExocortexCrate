#ifndef __ALEMBIC_TRANSFORM_CONTROLLER__H
#define __ALEMBIC_TRANSFORM_CONTROLLER__H

#include "Max.h"
#include "resource.h"
#include "iparamb2.h"
#include "ILockedTracks.h"
#include "AlembicNames.h"
#include <INodeMonitor.h>

const int ALEMBIC_XFORM_CONTROLLER_REF_PBLOCK = 0;
const int ALEMBIC_XFORM_CONTROLLER_REF_INODE = 1;

class AlembicXformController : public LockableStdControl
{
public:
    IParamBlock2* pblock;
   
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
		ID_CAMERA,
		ID_MUTED,
	};

    static IObjParam *ip;
    static AlembicXformController *editMod;

	AlembicXformController();
	~AlembicXformController();

	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	virtual Class_ID ClassID() { return ALEMBIC_XFORM_CONTROLLER_CLASSID; }		
	void GetClassName(TSTR& s) { s = _T(ALEMBIC_XFORM_CONTROLLER_NAME); }  
	TCHAR *GetObjectName() { return _T(ALEMBIC_XFORM_CONTROLLER_NAME); }

    void DeleteThis() { delete this; }
	RefTargetHandle Clone(RemapDir& remap);

    void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
    void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);

    int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
    IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
    IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	int NumRefs() { return 1; }
	void SetReference(int i, ReferenceTarget* pTarget); 
	RefTargetHandle GetReference(int i); 
	RefResult NotifyRefChanged(Interval, RefTargetHandle, PartID&, RefMessage);

    int NumSubs()  {return 1;} //because it uses the paramblock
    Animatable* SubAnim(int i) {return GetReference(i);}
    TSTR SubAnimName(int i) { return _T("Parameters"); }
    int SubNumToRefNum(int subNum) {if (subNum==0) return 0; else return -1;}

    void Copy(Control* pFrom){}
    virtual int IsKeyable() { return 0; }
    virtual BOOL IsLeaf() { return TRUE; }
    virtual BOOL IsReplaceable() { return TRUE; }

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
    std::string m_CachedAbcFile;
};

class AlembicXformControllerClassDesc : public ClassDesc2
{
public:
	AlembicXformControllerClassDesc() {}
	~AlembicXformControllerClassDesc() {}

	int				IsPublic()			{ return TRUE; }	// We do want the user to see this plug-in
	const MCHAR*	ClassName()			{ static const MSTR str(ALEMBIC_XFORM_CONTROLLER_NAME); return str; }
	SClass_ID		SuperClassID()		{ return CTRL_MATRIX3_CLASS_ID; }
	Class_ID		ClassID()			{ return ALEMBIC_XFORM_CONTROLLER_CLASSID; }
	const MCHAR*	Category()			{ return EXOCORTEX_ALEMBIC_CATEGORY; }
	void*			Create(BOOL loading=FALSE) { return new AlembicXformController; }
    const TCHAR*	InternalName()		{ return _T(ALEMBIC_XFORM_CONTROLLER_SCRIPTNAME); }	// returns fixed parsable name (scripter-visible name)
    HINSTANCE		HInstance()			{ return hInstance; }			// returns owning module handle
};

ClassDesc2* GetAlembicXformControllerClassDesc();

#endif
