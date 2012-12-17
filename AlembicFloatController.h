#ifndef __ALEMBIC_FLOAT_CONTROLLER_H__
#define __ALEMBIC_FLOAT_CONTROLLER_H__

#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicDefinitions.h"
#include "dummy.h"
#include <ILockedTracks.h>
#include "iparamb2.h"
#include "AlembicNames.h"
#include "Max.h"
#include "resource.h"
#include "AlembicLightUtilities.h"

ClassDesc2* GetAlembicFloatControllerClassDesc();


///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicFloatController Declaration
///////////////////////////////////////////////////////////////////////////////////////////////////
class AlembicFloatController : public LockableStdControl
{
public:
  
public:
	// Parameters in first block:
	enum 
	{ 
		ID_PATH,
		ID_IDENTIFIER,
		ID_PROPERTY,
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
    static AlembicFloatController *editMod;

	AlembicFloatController();
	~AlembicFloatController();

	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }

    void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic Float"); }  
	virtual Class_ID ClassID() { return ALEMBIC_FLOAT_CONTROLLER_CLASSID; }		
	RefTargetHandle Clone(RemapDir& remap);

	void EnumAuxFiles(AssetEnumCallback& nameEnum, DWORD flags);

	TCHAR *GetObjectName() { return _T("Alembic Float"); }

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

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicFloatControllerClassDesc
///////////////////////////////////////////////////////////////////////////////////////////////////
class AlembicFloatControllerClassDesc : public ClassDesc2
{
public:
	AlembicFloatControllerClassDesc() {};
	~AlembicFloatControllerClassDesc() {};

	// ClassDesc methods.  
	// Max calls these functions to figure out what kind of plugin this class represents

	// Return TRUE if the user can create this plug-in.
	int			IsPublic()			{ return TRUE; }	// We do want the user to see this plug-in

	// Return the class name of this plug-in
	const MCHAR* ClassName()		{ static const MSTR str(_T("Alembic Camera Float Controller")); return str; }

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
	Class_ID	ClassID()			{ return ALEMBIC_FLOAT_CONTROLLER_CLASSID; }

	// If the plugin is an Object or Texture, this function returns
	// the category it can be assigned to.
	const MCHAR* Category()			{ return EXOCORTEX_ALEMBIC_CATEGORY; }

	// Return an instance of this plug-in.  Max will call this function
	// when it wants to start using our plug-in
	void* Create(BOOL loading=FALSE)
	{
		return new AlembicFloatController;
	}

    const TCHAR*	InternalName() { return _T("AlembicFloatController"); }	// returns fixed parsable name (scripter-visible name)
    HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

bool getCameraSampleVal(AbcG::ICamera& objCamera, SampleInfo& sampleInfo, AbcG::CameraSample sample, const char* name, double& sampleVal);

#endif // __ALEMBIC_FLOAT_CONTROLLER_H__
