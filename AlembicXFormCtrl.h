/**********************************************************************
 *<
	FILE: AlembicXFormCtrl.h

	DESCRIPTION:

	CREATED BY: Tom Hudson

	HISTORY:

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#ifndef __AlembicXFormCtrl__H
#define __AlembicXFormCtrl__H

#include "Max.h"
#include "resource.h"
#include "iparamb2.h"
#include "ILockedTracks.h"
#include "AlembicNames.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicXFormCtrl Declaration
///////////////////////////////////////////////////////////////////////////////////////////////////
class AlembicXFormCtrl : public LockableStdControl
{
public:
    IParamBlock2* pblock;
    static IObjParam *ip;
    static AlembicXFormCtrl *editMod;
public:
	AlembicXFormCtrl();
	~AlembicXFormCtrl();

	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }

    void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s = _T("Alembic Xform"); }  
	virtual Class_ID ClassID() { return ALEMBIC_XFORM_CTRL_CLASSID; }		
	RefTargetHandle Clone(RemapDir& remap);
	TCHAR *GetObjectName() { return _T("Alembic Xform"); }

    void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
    void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);

    int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
    IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
    IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	int NumRefs() { return 1; }
	void SetReference(int i, ReferenceTarget* pTarget);
	ReferenceTarget* GetReference(int i);
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

    void MainPanelInitDialog( HWND hWnd );
    void MainPanelDestroy( HWND hWnd );
    void MainPanelUpdateUI();

    void SetAlembicId(const std::string &file, const std::string &identifier, TimeValue t);
    const std::string &GetAlembicArchive() { return m_AlembicNodeProps.m_File; }
    const std::string &GetAlembicObjectId() { return m_AlembicNodeProps.m_Identifier; }
    void SetIsCameraTransform( bool camera) { m_bIsCameraTransform = camera; }
    bool GetIsCameraTransform() { return m_bIsCameraTransform; }


private:
    alembic_nodeprops m_AlembicNodeProps;
    Interval m_CurrentAlembicInterval;
    bool m_bIsCameraTransform;
    HWND mhPanel;
    bool mbSuspendPanelUpdate;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicXFormCtrlClassDesc
///////////////////////////////////////////////////////////////////////////////////////////////////
class AlembicXFormCtrlClassDesc : public ClassDesc2
{
public:
	AlembicXFormCtrlClassDesc() {};
	~AlembicXFormCtrlClassDesc() {};

	// ClassDesc methods.  
	// Max calls these functions to figure out what kind of plugin this class represents

	// Return TRUE if the user can create this plug-in.
	int			IsPublic()			{ return TRUE; }	// We do want the user to see this plug-in

	// Return the class name of this plug-in
	const MCHAR* ClassName()		{ static const MSTR str("Alembic Xform"); return str; }

	// Return the SuperClassID - this ID should
	// match the implementation of the interface returned
	// by Create.
	SClass_ID	SuperClassID()		{ return CTRL_MATRIX3_CLASS_ID; }

	// Return the unique ID that identifies this class
	// This is required when saving.  Max stores the ClassID
	// reported by the actual plugin, and on reload it recreates
	// the appropriate class by matching the stored ClassID with
	// the matching ClassDesc
	//
	// You can generate random ClassID's using the gencid program
	// supplied with the Max SDK
	Class_ID	ClassID()			{ return ALEMBIC_XFORM_CTRL_CLASSID; }

	// If the plugin is an Object or Texture, this function returns
	// the category it can be assigned to.
	const MCHAR* Category()			{ return EXOCORTEX_ALEMBIC_CATEGORY; }

	// Return an instance of this plug-in.  Max will call this function
	// when it wants to start using our plug-in
	void* Create(BOOL loading=FALSE)
	{
		return new AlembicXFormCtrl;
	}

    const TCHAR*	InternalName() { return _T("AlembicXformController"); }	// returns fixed parsable name (scripter-visible name)
    HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

ClassDesc2* GetAlembicXFormCtrlClassDesc();


extern HINSTANCE hInstance;

#endif
