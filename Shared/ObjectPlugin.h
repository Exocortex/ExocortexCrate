/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <object.h>
#include <triobj.h>
#include <patchobj.h>

//=========================================================================
// Replaces SimpleObject2 and SimpleObject
//
// The assumption is that Base_T derives from "Object" class (directly or indirectly)

template<typename Base_T>
class ObjectPlugin
    : public AnimatablePlugin<Base_T>
{
private:

	Mesh mesh;
	Interval ivalid;
    int extDispFlags;

public:
	
    // Constructor / Destructor 
    ObjectPlugin(ClassDesc2* classDesc) 
        : AnimatablePlugin(classDesc)
    { 
        mesh.EnableEdgeList(1);
        ivalid.SetEmpty();
        extDispFlags = 0;
    }

    ~ObjectPlugin()
    {
    }

    /// \name New ObjectPlugin Virtual Functions
    //@{
    /**
     * The implementation of this function should construct a mesh at a specific time. 
     * 
     * \param       t An argument of type TimeValue.
     * \param       m An argument of type Mesh &.
     */
    virtual void BuildMesh(TimeValue t, Mesh& m) = 0;

    /**
     * Override this function to return FALSE during any special degenerate cases when the 
     * object should not be displayed (e.g. size == 0).
     * 
     * \param       t An argument of type TimeValue.
     * \return      Returns a value of type BOOL.
     */
    virtual BOOL OKtoDisplay(TimeValue t) 
    {
        return TRUE;
    }
    //@}

    /// \name Utility Functions
    //@{
    /**
     * Triggers a rebuild of the mesh, if the time value falls outside of the validity interval.
     * 
     * \param       t An argument of type TimeValue.
     */
    void UpdateMesh(TimeValue t)
    {
	    if (!ivalid.InInterval(t) ) {
		    BuildMesh(t, mesh);
            ivalid.SetInfinite();
		}
    }
    /**
     * Returns true if in creation mode. 
     * 
     * \return      Returns a value of type bool.
     */
    bool IsCreating() 
    {
        return TestAFlag(A_OBJ_CREATING) == TRUE;
    }
    /**
     * Returns the cached mesh. 
     * 
     * \return      Returns a value of type Mesh &.
     */
    Mesh& GetMesh() 
    {
        return mesh;
    }
    /**
     * Returns the validity reference.
     * 
     * \return      Returns a value of type Interval &.
     */
    Interval& GetValidityRef() 
    {
        return ivalid;
    }
    /**
     * Returns true if the EXT_DISP_SELECTED flag is set.
     * 
     * \return      Returns a value of type bool.
     */
    bool IsSelected() 
    {
        return (extDispFlags & EXT_DISP_SELECTED) != 0;
    }
    /**
     * Returns true if the EXT_DISP_TARGET_SELECTED flag is set.
     * 
     * \return      Returns a value of type bool.
     */
    bool IsTargetSelected()
    {
        return (extDispFlags & EXT_DISP_TARGET_SELECTED) != 0;
    }
    /**
     * Returns true if the EXT_DISP_TARGET_SELECTED flag is set.
     * 
     * \return      Returns a value of type bool.
     */
    bool IsLookatSelected()
    {
        return (extDispFlags & EXT_DISP_LOOKAT_SELECTED) != 0;
    }
    /**
     * Returns true if the EXT_DISP_TARGET_SELECTED flag is set.
     * 
     * \return      Returns a value of type bool.
     */
    bool IsOnlySelected()
    {
        return (extDispFlags & EXT_DISP_ONLY_SELECTED) != 0;
    }
    /**
     * Returns true if the EXT_DISP_DRAGGING flag is set.
     * 
     * \return      Returns a value of type bool.
     */
    bool IsDragging()
    {
        return (extDispFlags & EXT_DISP_DRAGGING) != 0;
    }
    /**
     * Returns true if the EXT_DISP_ZOOM_EXT flag is set.
     * 
     * \return      Returns a value of type bool.
     */
    bool IsZoomExtent()
    {
        return (extDispFlags & EXT_DISP_ZOOM_EXT) != 0;
    }
    /**
     * Returns true if the EXT_DISP_GROUP_EXT flag is set.
     * 
     * \return      Returns a value of type bool.
     */
    bool IsGroupExtext()
    {
        return (extDispFlags & EXT_DISP_GROUP_EXT) != 0;
    }
    /**
     * Returns true if the EXT_DISP_ZOOMSEL_EXT flag is set.
     * 
     * \return      Returns a value of type bool.
     */
    bool IsZoomSelectedExtent()
    {
        return (extDispFlags & EXT_DISP_ZOOMSEL_EXT) != 0;
    }
    /**
     * Converts the object to a TriObject. 
     * 
     * \param       t An argument of type TimeValue.
     * \return      Returns a value of type TriObject *.
     */
    TriObject* ConvertToTriObject(TimeValue t) 
    {
	    TriObject* triob = new TriObject();
	    triob->GetMesh() = mesh;
	    triob->SetChannelValidity(TOPO_CHAN_NUM, ObjectValidity(t));
	    triob->SetChannelValidity(GEOM_CHAN_NUM, ObjectValidity(t));		
	    return triob;
    }
    /**
     * Converts the object to a PatchObject. 
     * 
     * \param       t An argument of type TimeValue.
     * \return      Returns a value of type PatchObject *.
     */
    PatchObject* ConvertToPatchObject(TimeValue t) 
    {
	    PatchObject *patchob = new PatchObject();
	    patchob->patch = mesh;		
	    patchob->SetChannelValidity(TOPO_CHAN_NUM, ObjectValidity(t));
	    patchob->SetChannelValidity(GEOM_CHAN_NUM, ObjectValidity(t));		
        return patchob;
    }
    /**
     * Returns the selected node that is associated with this object. 
     * 
     * \return      Returns a value of type INode *.
     */
    INode* GetSelectedSelfNode()
    {
        for (int i=0; i < GetCOREInterface()->GetSelNodeCount(); ++i) {
            INode* node = GetCOREInterface()->GetSelNode(i);
            if (node->GetObjectRef() == this)
                return node;
        }
        return NULL;
    }
    //@}

    /// \name Animatable Overrides
    //@{
    /**
     * \see         Animatable::EndEditParams.
     * 
     * \param       ip An argument of type IObjParam *.
     * \param       flags An argument of type ULONG.
     * \param       next An argument of type Animatable *.
     */
    virtual void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) 
    {
        if (IsCreating()) 
            ClearAFlag(A_OBJ_CREATING);
        AnimatablePlugin::EndEditParams(ip, flags, next);
    }   
    /**
     * Clears all caches, and resets the validity interval.
     * 
     * \see         Animatable::FreeCaches()
     * 
     */
    virtual void FreeCaches() 
	{
	    ivalid.SetEmpty();
	    mesh.FreeAll();
	} 
    //@}

    /// \name BaseObject Overrides
    //@{
    /**
     * \see         BaseObject::GetDeformBBox()
     * 
     * \param       t An argument of type TimeValue.
     * \param       box An argument of type Box3 &.
     * \param       tm An argument of type Matrix3 *.
     * \param       useSel An argument of type BOOL.
     */
    virtual void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel )
	{
    	UpdateMesh(t);
	    box = mesh.getBoundingBox(tm);
	}
    /**
     * \see         BaseObject::GetLocalBoundBox()
     * 
     * \param       t An argument of type TimeValue.
     * \param       inode An argument of type INode *.
     * \param       vpt An argument of type ViewExp *.
     * \param       box An argument of type Box3 &.
     */
    virtual void GetLocalBoundBox(TimeValue t, INode *inode,ViewExp* vpt,  Box3& box ) 
	{
	    UpdateMesh(t);
	    box = mesh.getBoundingBox();
	}
    /**
     * \see         BaseObject::GetWorldBoundBox()
     * 
     * \param       t An argument of type TimeValue.
     * \param       inode An argument of type INode *.
     * \param       vpt An argument of type ViewExp *.
     * \param       box An argument of type Box3 &.
     */
    virtual void GetWorldBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box )
	{
	    Matrix3 mat = inode->GetObjectTM(t);
	    UpdateMesh(t);
	    box = mesh.getBoundingBox();
	    box = box * mat;
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
     * \return      Returns a value of type int.
     */
    virtual int HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) 
	{
	    Point2 pt( (float)p[0].x, (float)p[0].y );
	    HitRegion hitRegion;
	    GraphicsWindow *gw = vpt->getGW();	
	    UpdateMesh(t);
	    gw->setTransform(inode->GetObjectTM(t));
	    MakeHitRegion(hitRegion, type, crossing, 4, p);
	    return mesh.select(gw, inode->Mtls(), &hitRegion, flags & HIT_ABORTONHIT, inode->NumMtls());
	}
    /**
     * \see         BaseObject::Snap()
     * 
     * \param       t An argument of type TimeValue.
     * \param       inode An argument of type INode *.
     * \param       snap An argument of type SnapInfo *.
     * \param       p An argument of type IPoint2 *.
     * \param       vpt An argument of type ViewExp *.
     */
    virtual void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt) 
	{
        // No snap to ourself while creating!
	    if (IsCreating())	
		    return;
	    Matrix3 tm = inode->GetObjectTM(t);
	    GraphicsWindow *gw = vpt->getGW();
	    UpdateMesh(t);
	    gw->setTransform(tm);
	    mesh.snap(gw, snap, p, tm);
	}
    /**
     * \see         BaseObject::Display()
     * 
     * \param       t An argument of type TimeValue.
     * \param       inode An argument of type INode *.
     * \param       vpt An argument of type ViewExp *.
     * \param       flags An argument of type int.
     * \return      Returns a value of type int.
     */
    virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) 
	{
    	if (!OKtoDisplay(t)) 
            return 0;
	    GraphicsWindow *gw = vpt->getGW();
	    Matrix3 mat = inode->GetObjectTM(t);	 
	    UpdateMesh(t);		
	    gw->setTransform(mat);
        if (flags & USE_DAMAGE_RECT)
        {
            RECT r = vpt->GetDammageRect();
	        mesh.render(gw, inode->Mtls(), &r, COMP_ALL, inode->NumMtls());
        }
        else
        {
	        mesh.render(gw, inode->Mtls(), NULL, COMP_ALL, inode->NumMtls());
        }
	    return 0;
    }
    /**
     * \see         BaseObject::GetCreateMouseCallBack()
     * 
     * \return      Returns a value of type CreateMouseCallBack *.
     */
    virtual CreateMouseCallBack* GetCreateMouseCallBack()
    {
        return NULL;
    }
    //@}

    /// \name Object Overrides
    //@{
    /**
     * \see         Object::Eval()
     * 
     * \param       time An argument of type TimeValue.
     * \return      Returns a value of type ObjectState.
     */
    virtual ObjectState Eval(TimeValue time) 
	{
        UpdateMesh(time);
	    return ObjectState(this);
	}
    /**
     * \see         Object::SetExtendedDisplay()
     * 
     * \param       flags An argument of type int.
     */
    virtual void SetExtendedDisplay(int flags)
    {
	    extDispFlags = flags;
    }
    /**
     * \see         Object::ObjectValidity()
     * 
     * \param       t An argument of type TimeValue.
     * \return      Returns a value of type Interval.
     */
    virtual Interval ObjectValidity(TimeValue t) 
	{        
        return ivalid;
	}
    /**    
     * \see         Object::IntersectRay()
     * 
     * \param       t An argument of type TimeValue.
     * \param       ray An argument of type Ray &.
     * \param       at An argument of type float &.
     * \param       norm An argument of type Point3 &.
     * \return      Returns a value of type int.
     */
    virtual int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm)
	{	
	    UpdateMesh(t);
	    return mesh.IntersectRay(ray,at,norm);	
	}
    /**    
     * \see         Object::CanConvertToType()
     * 
     * \param       obtype An argument of type Class_ID.
     * \return      Returns a value of type int.
     */
    virtual int CanConvertToType(Class_ID obtype) 
	{
	    if (obtype==defObjectClassID) return TRUE;
	    if (obtype==mapObjectClassID) return TRUE;
	    if (obtype==triObjectClassID) return TRUE;
	    if (obtype==patchObjectClassID) return TRUE;
	    if (Object::CanConvertToType(obtype)) return TRUE;
	    if (CanConvertTriObject(obtype)) return TRUE;
	    return FALSE;
    }
    /**
     * \see         Object::ConvertToType()
     * 
     * \param       t An argument of type TimeValue.
     * \param       obtype An argument of type Class_ID.
     * \return      Returns a value of type Object *.
     */
    virtual Object* ConvertToType(TimeValue t, Class_ID obtype) 
	{
        if (CanConvertToType(obtype))
            return NULL;

        UpdateMesh(t);

	    if (obtype==defObjectClassID || obtype==triObjectClassID || obtype==mapObjectClassID) {
            return ConvertToTriObject(t);
	    }
	    else if (obtype == patchObjectClassID) {
		    return ConvertToPatchObject(t);
	    }
	    else if (Object::CanConvertToType (obtype)) {
		    return Object::ConvertToType(t,obtype);
	    }
	    else if (CanConvertTriObject(obtype)) {
		    TriObject *triob = ConvertToTriObject(t);
		    Object *ob = triob->ConvertToType (t, obtype);		    
            // ob should never = tob but just to be safe, we check before 
            // deleting.
            if (ob != triob) 
                triob->DeleteThis();	
		    return ob;
	    }
	    return NULL;
    }
    //@}

    /// \name GeomObject Overrides
    //@{
    /**
     * \see         GeomObject::GetRenderMesh()
     * 
     * \param       t An argument of type TimeValue.
     * \param       inode An argument of type INode *.
     * \param       view An argument of type View &.
     * \param       needDelete An argument of type BOOL &.
     * \return      Returns a value of type Mesh *.
     */
    virtual Mesh* GetRenderMesh(TimeValue t, INode *inode, View &view, BOOL& needDelete) 
    {
	    UpdateMesh(t);
	    needDelete = FALSE;
	    return &mesh;
	}
    /**
     * \see         GeomObject::PolygonCount()
     * 
     * \param       t An argument of type TimeValue.
     * \param       numFaces An argument of type int &.
     * \param       numVerts An argument of type int &.
     * \return      Returns a value of type BOOL.
     */
    virtual BOOL PolygonCount(TimeValue t, int& numFaces, int& numVerts) 
    {
        UpdateMesh(t);
        numFaces = mesh.getNumFaces();
        numVerts = mesh.getNumVerts();
        return TRUE;
    }
    //@}

    
    /// \name AnimatablePlugin Overrides
    //@{
    /**
     * Resets the validity interval whenever a parameter changes. 
     * 
     * \param       id An argument of type ParamID.
     */
    virtual void OnParamChangeMsg(ParamID id) 
    {
        ivalid.SetEmpty();
    }
    //@}
};


