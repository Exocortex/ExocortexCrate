/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <objmode.h>

/**
 * This class is a base class for modifier plug-ins which is a modern 
 * implementation from SimpleMod and SimpleMod2. One feature that is 
 * missing from SimpleMod is support for ModifierLimits. There 
 * is no support for parameter block 1. 
 *
 * \par Like SimpleMod this provides automatic management of modes, and 
 * the the gizmo and center sub-object. To use this class you will 
 * have to implement the pure virtual function GetDeformer(). 
 *
 * \see         Modifier, SimpleMod, SimpleMod2, AnimatablePlugin
 */
template<typename Base_T>
class ModifierPlugin
    : public AnimatablePlugin<Base_T>
{   
private:

    MoveModBoxCMode*    moveMode;
    RotateModBoxCMode*  rotMode;
    UScaleModBoxCMode*  uscaleMode;
    NUScaleModBoxCMode* nuscaleMode;
    SquashModBoxCMode*  squashMode;

    GenSubObjType GizmoSubObject;
    GenSubObjType CenterSubObject; 

    static const int GIZMO_ICON_INDEX = 14;
    static const int CENTER_ICON_INDEX = 15;

public:

    ModifierPlugin(ClassDesc2* classDesc)
        : AnimatablePlugin(classDesc), GizmoSubObject(GIZMO_ICON_INDEX), CenterSubObject(CENTER_ICON_INDEX)
	{
	    moveMode    = NULL;
        rotMode 	= NULL;
        uscaleMode  = NULL;
        nuscaleMode = NULL;
        squashMode  = NULL;

		GizmoSubObject.SetName(_T("Gizmo"));
		CenterSubObject.SetName(_T("Center"));

        RegisterReference(1, NewDefaultMatrix3Controller(), false);
        RegisterReference(2, NewDefaultPositionController(), false);            

        RegisterSubAnim(1, 1, "Gizmo");
        RegisterSubAnim(2, 2, "Center");
	}


    ~ModifierPlugin()
    {
        DeleteModes();
    }


    /// \name Animatable Overrides
    //@{
    /**
     * \see         Animatable::BeginEditParams()
     * 
     * \param       ip An argument of type IObjParam *.
     * \param       flags An argument of type ULONG.
     * \param       prev An argument of type Animatable *.
     */
    virtual void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev )
	{
        DeleteModes();

        moveMode    = new MoveModBoxCMode(this,ip);
	    rotMode     = new RotateModBoxCMode(this,ip);
	    uscaleMode  = new UScaleModBoxCMode(this,ip);
	    nuscaleMode = new NUScaleModBoxCMode(this,ip);
	    squashMode  = new SquashModBoxCMode(this,ip);	
    	
	    TimeValue t = Now();
	    NotifyDependents(Interval(t,t), (PartID)PART_ALL, REFMSG_BEGIN_EDIT);
	    NotifyDependents(Interval(t,t), (PartID)PART_ALL, REFMSG_MOD_DISPLAY_ON);

        SetAFlag(A_MOD_BEING_EDITED);

        AnimatablePlugin::BeginEditParams(ip, flags, prev);
	}

    /**
     * \see         Animatable::EndEditParams()
     * 
     * \param       ip An argument of type IObjParam *.
     * \param       flags An argument of type ULONG.
     * \param       next An argument of type Animatable *.
     */
    virtual void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next)
	{
        AnimatablePlugin::EndEditParams(ip, flags, next);

        // NOTE: This flag must be cleared before sending the REFMSG_END_EDIT
	    ClearAFlag(A_MOD_BEING_EDITED);

	    TimeValue t = Now();
	    NotifyDependents(Interval(t,t), (PartID)PART_ALL, REFMSG_END_EDIT);
	    NotifyDependents(Interval(t,t), (PartID)PART_ALL, REFMSG_MOD_DISPLAY_OFF);

        DeleteModes();
	}
    //@}

    
    /// \name BaseObject Overrides
    //@{
    /**
     * \see         BaseObject::GetCreateMouseCallBack()
     * 
     * \return      Returns a value of type CreateMouseCallBack *.
     */
    virtual CreateMouseCallBack* GetCreateMouseCallBack() 
    {
        return NULL;
    }

    /**
     * \see         BaseObject::ActivateSubobjSel()
     * 
     * \param       level An argument of type int.
     * \param       modes An argument of type XFormModes &.
     */
    virtual void ActivateSubobjSel(int level, XFormModes& modes )
    {	
	    switch ( level ) 
        {
		    case 1: 
                // Modifier box
			    modes = XFormModes(moveMode, rotMode, nuscaleMode, uscaleMode, squashMode, NULL);
			    break;
		    case 2: 
                // Modifier Center
			    modes = XFormModes(moveMode, NULL, NULL, NULL, NULL, NULL);
			    break;
		}
	    NotifyDependents(FOREVER,PART_DISPLAY,REFMSG_CHANGE);
	}

    /**
     * \see         BaseObject::GetSubObjectCenters()
     * 
     * \param       cb An argument of type SubObjAxisCallback *.
     * \param       t An argument of type TimeValue.
     * \param       node An argument of type INode *.
     * \param       mc An argument of type ModContext *.
     */
    virtual void GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node, ModContext *mc)
    {
        Interval valid;
        Matrix3 modmat, tm, ntm = node->GetObjTMBeforeWSM(t,&valid), off, invoff;

        if (cb->Type() == SO_CENTER_PIVOT) {
	        tm = CompMatrix(t,*mc,ntm,valid,TRUE);
	        cb->Center(tm.GetTrans(),0);
        } else {
	        modmat = CompMatrix(t,*mc,ntm,valid,FALSE);
	        CompOffset(t,off,invoff);
	        BoxLineProc bp1(&modmat);	
	        DoModifiedBox(*mc->box, *GetDeformer(t,*mc,invoff,off), bp1);
	        cb->Center(bp1.Box().Center(),0);
	    }
	}

    /**
     * \see         BaseObject::GetSubObjectTMs()
     * 
     * \param       cb An argument of type SubObjAxisCallback *.
     * \param       t An argument of type TimeValue.
     * \param       node An argument of type INode *.
     * \param       mc An argument of type ModContext *.
     */
    virtual void GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node, ModContext *mc)
    {
        Interval valid;
        Matrix3 ntm = node->GetObjTMBeforeWSM(t, &valid);
        Matrix3 tm = CompMatrix(t, *mc, ntm, valid, TRUE);
        cb->TM(tm,0);
    }

    /**
     * \see         BaseObject::Rotate()
     * 
     * \param       t An argument of type TimeValue.
     * \param       partm An argument of type Matrix3 &.
     * \param       tmAxis An argument of type Matrix3 &.
     * \param       val An argument of type Quat &.
     * \param       localOrigin An argument of type BOOL.
     */
    virtual void Rotate( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin = FALSE) 
    {
	    SetXFormPacket pckt(val, localOrigin, partm, tmAxis);
	    GetTMController()->SetValue(t, &pckt, TRUE, CTRL_RELATIVE);		
	    macroRecorder->OpAssign(_T("+="), mr_prop, _T("gizmo.rotation"), mr_reftarg, this, mr_quat, &val);
	}

    /**
     * \see         BaseObject::Scale()
     * 
     * \param       t An argument of type TimeValue.
     * \param       partm An argument of type Matrix3 &.
     * \param       tmAxis An argument of type Matrix3 &.
     * \param       val An argument of type Point3 &.
     * \param       localOrigin An argument of type BOOL.
     */
    virtual void Scale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE) 
    {
	    SetXFormPacket pckt(val, localOrigin, partm, tmAxis);
	    GetTMController()->SetValue(t, &pckt, TRUE, CTRL_RELATIVE);		
	    macroRecorder->OpAssign(_T("*="), mr_prop, _T("gizmo.scale"), mr_reftarg, this, mr_point3, &val);
	}

    /**
     * \see         BaseObject::Display()
     * 
     * \param       t An argument of type TimeValue.
     * \param       inode An argument of type INode *.
     * \param       vpt An argument of type ViewExp *.
     * \param       flags An argument of type int.
     * \param       mc An argument of type ModContext *.
     * \return      Returns a value of type int.
     */
    virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, ModContext *mc) 
    {
        Interval valid;
        GraphicsWindow *gw = vpt->getGW();
        Matrix3 modmat, ntm = inode->GetObjTMBeforeWSM(t), off, invoff;

        if (mc->box->IsEmpty()) return 0;

        modmat = CompMatrix(t,*mc,ntm,valid,FALSE);
        CompOffset(t,off,invoff);
        gw->setTransform(modmat);
        if ( GetCOREInterface()->GetSubObjectLevel() == 1 ) {		
	        gw->setColor(LINE_COLOR,GetUIColor(COLOR_SEL_GIZMOS));
        } else {
	        gw->setColor(LINE_COLOR,GetUIColor(COLOR_GIZMOS));
	    }	
        if (mc->box->pmin==mc->box->pmax) {
	        gw->marker(&mc->box->pmin,ASTERISK_MRKR);
        } 
        else {
	        DoModifiedBox(*mc->box, *GetDeformer(t,*mc,invoff,off), DrawLineProc(gw));
	    }

        modmat = CompMatrix(t,*mc,ntm,valid,TRUE);
        gw->setTransform(modmat);
        if ( GetCOREInterface()->GetSubObjectLevel() == 1 || GetCOREInterface()->GetSubObjectLevel() == 2) 
        {		
	        gw->setColor( LINE_COLOR, (float)1.0, (float)1.0, (float)0.0);
        } 
        else 
        {
	        gw->setColor( LINE_COLOR, (float).85, (float).5, (float)0.0);
	    }	
        DrawCenterMark(DrawLineProc(gw),*mc->box);	

    	return 0;	
	}

    /**
     * \see         BaseObject::GetWorldBoundBox()
     * 
     * \param       t An argument of type TimeValue.
     * \param       inode An argument of type INode *.
     * \param       vpt An argument of type ViewExp *.
     * \param       box An argument of type Box3 &.
     * \param       mc An argument of type ModContext *.
     */
    virtual void GetWorldBoundBox(TimeValue t,INode* inode, ViewExp *vpt, Box3& box, ModContext *mc) 
    {
	    Interval valid;
	    Matrix3 modmat, ntm = inode->GetObjTMBeforeWSM(t), off, invoff;

	    if (mc->box->IsEmpty()) 
            return;

	    modmat = CompMatrix(t, *mc, ntm, valid, FALSE);
	    CompOffset(t,off,invoff);	
	    BoxLineProc bp1(&modmat);	
	    DoModifiedBox(*mc->box, *GetDeformer(t, *mc, invoff, off), bp1);
	    box = bp1.Box();

	    modmat = CompMatrix(t, *mc, ntm, valid, TRUE);	
	    BoxLineProc bp2(&modmat);		
	    DrawCenterMark(bp2,*mc->box);
	    box += bp2.Box();
	}

    /**
     * \see         BaseObject::HitTest()
     * 
     * \param       t An argument of type TimeValue.
     * \param       inode An argument of type INode *.
     * \param       type An argument of type int.
     * \param       crossing An argument of type int.
     * \param       flags An argument of type int.
     * \param       p An argument of type IPoint2 *.
     * \param       vpt An argument of type ViewExp *.
     * \param       mc An argument of type ModContext *.
     * \return      Returns a value of type int.
     */
    virtual int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc) 
    {
	    if (mc->box->IsEmpty()) 
            return 0;

	    Interval valid;
	    GraphicsWindow *gw = vpt->getGW();
	    HitRegion hr;
	    MakeHitRegion(hr,type, crossing,4,p);
	    gw->setHitRegion(&hr);
	    Matrix3 modmat, ntm = inode->GetObjTMBeforeWSM(t);

        HitTestModeRAII hitTestHelper(gw);
    	
        Interface* ip = GetCOREInterface();
	    if (ip->GetSubObjectLevel() == 1) {
		    Matrix3 off, invoff;
		    modmat = CompMatrix(t,*mc,ntm,valid,FALSE);
		    CompOffset(t,off,invoff);
		    gw->setTransform(modmat);		
		    if (mc->box->pmin==mc->box->pmax) {
			    gw->marker(&mc->box->pmin,ASTERISK_MRKR);
		    } 
            else {
			    DoModifiedBox(*mc->box,*GetDeformer(t,*mc,invoff,off), DrawLineProc(gw));
		    }
	    }

	    if (ip->GetSubObjectLevel() == 1 ||
	              ip->GetSubObjectLevel() == 2 ) {
		    modmat = CompMatrix(t,*mc,ntm,valid,TRUE);
		    gw->setTransform(modmat);
		    DrawCenterMark(DrawLineProc(gw),*mc->box);
	    }

	    if (gw->checkHitCode()) {
		    vpt->LogHit(inode, mc, gw->getHitDistance(), 0, NULL); 
		    return TRUE;
		}
        else {
	        return FALSE;
        }
	}

    /**
     * \see         BaseObject::NumSubObjTypes()
     * 
     * \return      Returns a value of type int.
     */
    virtual int NumSubObjTypes() 
    { 
	    return 2;
    }
		
    /**
     * \see         BaseObject::GetSubObjType()
     * 
     * \param       i An argument of type int.
     * \return      Returns a value of type ISubObjType *.
     */
    virtual ISubObjType* GetSubObjType(int i) 
    {
        switch (i)
        {
        case -1:
		    if(GetSubObjectLevel() > 0)
			    return GetSubObjType(GetSubObjectLevel()-1);
            break;
        case 0:
		    return &GizmoSubObject;
        case 1:
		    return &CenterSubObject;
        }
    	
	    return NULL;
    }

    /**
     * \see         BaseObject::Move()
     * 
     * \param       t An argument of type TimeValue.
     * \param       partm An argument of type Matrix3 &.
     * \param       tmAxis An argument of type Matrix3 &.
     * \param       val An argument of type Point3 &.
     * \param       localOrigin An argument of type BOOL.
     */
    virtual void Move( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE) 
    {
	    if ( GetCOREInterface()->GetSubObjectLevel() == 1) 
        {
		    SetXFormPacket pckt(val, partm, tmAxis);
		    GetTMController()->SetValue(t, &pckt, TRUE, CTRL_RELATIVE);		
		    macroRecorder->OpAssign(_T("+="), mr_prop, _T("gizmo.pos"), mr_reftarg, this, mr_point3, &val);
	    } 
        else 
        {		
            Interval valid = FOREVER;
		    Matrix3 ptm = partm;
            GetTMController()->GetValue(t,&ptm,valid,CTRL_RELATIVE);
		    GetPosController()->SetValue(t, VectorTransform(tmAxis*Inverse(ptm),val), TRUE, CTRL_RELATIVE);
		    macroRecorder->OpAssign(_T("+="), mr_prop, _T("center"), mr_reftarg, this, mr_point3, &val);
		}
	}
    //@}

	/// \name Modifier Overrides
	//@{
	/**
	 * \see         Modifier::ChannelsUsed()
	 * 
	 * \return      Returns a value of type ChannelMask.
	 */
	virtual ChannelMask ChannelsUsed() 
    { 
        return PART_GEOM|PART_TOPO|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL; 
    }
	
    /**
     * \see         Modifier::ChannelsChanged()
     * 
     * \return      Returns a value of type ChannelMask.
     */
    virtual ChannelMask ChannelsChanged() 
    { 
        return PART_GEOM; 
    }

    /**
     * \see         Modifier::InputType()
     * 
     * \return      Returns a value of type Class_ID.
     */
    virtual Class_ID InputType() 
    {
        return defObjectClassID;
    }

    /**
     * \see         Modifier::ModifyObject()
     * 
     * \param       t An argument of type TimeValue.
     * \param       mc An argument of type ModContext &.
     * \param       os An argument of type ObjectState *.
     * \param       node An argument of type INode *.
     */
    virtual void ModifyObject(TimeValue t, ModContext &mc, ObjectState * os, INode *node) 
	{	
	    Interval valid = LocalValidity(t);
    	
	    // These are inverted becuase that's what is usually needed for displaying/hit testing
	    Matrix3 minv   = CompMatrix(t,mc,idTM,valid,TRUE);
	    Matrix3 modmat = Inverse(minv);
    	
        // This calls the deform function of the object, using the virtual function GetDeformer
        Deformer* deformer = GetDeformer(t, mc, modmat, minv);
        DbgAssert(deformer != NULL && "The function GetDeformer must return a non-null object");
	    os->obj->Deform(deformer, TRUE);
	    os->obj->UpdateValidity(GEOM_CHAN_NUM, valid);	
	}

    /**
     * \see         Modifier::LocalValidity()
     * 
     * \param       t An argument of type TimeValue.
     * \return      Returns a value of type Interval.
     */
    virtual Interval LocalValidity(TimeValue t)
	{
	    // if being edited, return NEVER forces a cache to be built 
	    // after previous modifier.
	    if (IsBeingEdited())
		    return NEVER;  
	    Interval valid = FOREVER;
        for (int i=0; i < NumParamBlocks(); ++i)
            GetParamBlock(i)->GetValidity(t, valid);
	    Matrix3 mat(1);
        GetTMController()->GetValue(t, &mat, valid, CTRL_RELATIVE);
        mat.IdentityMatrix();
        GetPosController()->GetValue(t, &mat, valid, CTRL_RELATIVE);

	    return valid;
	}
	//@}

    //============================================================
    // Virtual function introduced by ModifierPlugin
    
    /// \name ModifierPlugin Virtual Functions
    //@{
    /**
     * Returns a Deformer instance that can be used to perform the 
     * actual modification of points.
     * 
     * \see         SimpleMod::GetDeformer()
     * 
     * \param       t An argument of type TimeValue.
     * \param       mc An argument of type ModContext &.
     * \param       mat An argument of type Matrix3 &.
     * \param       invmat An argument of type Matrix3 &.
     * \return      Returns a value of type Deformer *.
     */
    virtual Deformer* GetDeformer(TimeValue t,
        ModContext &mc, Matrix3& mat, Matrix3& invmat) = 0;
    //@}

    //============================================================
    // Utility functions

    /// \name Utility Functions
    //@{
    /**
     * Returns TRUE if the modifier is in edit mode.
     * 
     * \return      Returns a value of type BOOL.
     */
    BOOL IsBeingEdited()
    {
        return TestAFlag(A_MOD_BEING_EDITED);
    }

    /**
     * Returns the transform matrix controller.
     * 
     * \return      Returns a value of type Control *.
     */
    Control* GetTMController()
    {
        return dynamic_cast<Control*>(GetReference(1));
    }

    /**
     * Returns the position controller.
     * 
     * \return      Returns a value of type Control *.
     */
    Control* GetPosController()
    {
        return dynamic_cast<Control*>(GetReference(2));
    }	
    //@}

private:

    Matrix3 CompMatrix(TimeValue t, ModContext& mc, Matrix3& ntm, Interval& valid, BOOL needOffset) 
    {
	    Matrix3 tm;
    	
	    if (mc.tm) 
		    tm = *mc.tm; else 
		    tm.IdentityMatrix();	

	    Matrix3 mat;
	    mat.IdentityMatrix();
	    GetTMController()->GetValue(t,&mat,valid,CTRL_RELATIVE);
	    tm = tm*Inverse(mat);		
	    
	    mat.IdentityMatrix();		
	    GetPosController()->GetValue(t,&mat,valid,CTRL_RELATIVE);
	    tm = tm*Inverse(mat);		
    	
	    return Inverse(tm) * ntm;	
	}

    void CompOffset( TimeValue t, Matrix3& offset, Matrix3& invoffset)
    {	
        Interval valid;
        offset.IdentityMatrix();
        invoffset.IdentityMatrix();
	    GetPosController()->GetValue(t,&offset,valid,CTRL_RELATIVE);
	    invoffset = Inverse(offset);
    }

    void DeleteModes()
    {
        Interface* ip = GetCOREInterface();

	    ip->DeleteMode(moveMode);
	    ip->DeleteMode(rotMode);
	    ip->DeleteMode(uscaleMode);
	    ip->DeleteMode(nuscaleMode);
	    ip->DeleteMode(squashMode);	

	    DeleteAndNullify(moveMode);
        DeleteAndNullify(rotMode);
	    DeleteAndNullify(uscaleMode );
	    DeleteAndNullify(nuscaleMode);
	    DeleteAndNullify(squashMode);
    }
};
