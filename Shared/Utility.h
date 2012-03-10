/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "ILinkTMCtrl.h"

//=======================================================
// Forward declarations

INode* FindNodeRef(ReferenceTarget *rt);

//=======================================================
// Utility classes

/**
 * Sets a GraphicWindows into HitTest mode and restores 
 * the original state upon destruction. This is an idiom 
 * known as RAII, for resource allocation is initialization. 
 */
class HitTestModeRAII :
    MaxSDK::Util::Noncopyable
{
    GraphicsWindow* gw;
    DWORD savedLimits;

public:
    HitTestModeRAII(GraphicsWindow* x)
        : gw(x), savedLimits(x->getRndLimits())
    {
        gw->clearHitCode();
        gw->setRndLimits((savedLimits | GW_PICK) & ~GW_ILLUM);
    }

    ~HitTestModeRAII()
    {
        gw->setRndLimits(savedLimits);
    }
};

//=======================================================
// Utility functions, many are taken from various samples.

/**
 * Deletes a pointer variable and sets it to NULL. 
 * 
 * \param       x An argument of type T *&.
 */
template<typename T>
void DeleteAndNullify(T*& x)
{
    delete x;
    x = NULL;
}


/**
 * Returns the current time. 
 * 
 * \return      Returns a value of type TimeValue.
 */
TimeValue Now() 
{
    return GetCOREInterface()->GetTime();
}

/**
 * Get the first node pointing to a reference maker.
 * If "rm" is a node, returns it instead.
 * 
 * \param       rm An argument of type ReferenceMaker *.
 * \return      Returns a value of type INode *.
 */
INode* GetNodeRef(ReferenceMaker *rm) 
{
	if (rm->SuperClassID()==BASENODE_CLASS_ID) 
        return dynamic_cast<INode*>(rm);
	else 
        return rm->IsRefTarget() 
            ? FindNodeRef(dynamic_cast<ReferenceTarget*>(rm))
            : NULL;
}

/**
 * Finds the first node referencing a reference target.
 * 
 * \param       rt An argument of type ReferenceTarget *.
 * \return      Returns a value of type INode *.
 */
INode* FindNodeRef(ReferenceTarget *rt) {
	DependentIterator di(rt);
	INode *nd = NULL;	
	for (ReferenceMaker *rm = di.Next(); rm != NULL; rm = di.Next())
    {	
		nd = GetNodeRef(rm);
		if (nd) return nd;
	}
	return NULL;
}

/**
 * Return a pointer to a TriObject given an INode or NULL if no associated tri-object is found.
 * 
 * \param       node An argument of type INode *.
 * \param       t An argument of type const TimeValue.
 * \param       deleteIt An argument of type bool &.
 * \return      Returns a value of type TriObject *.
 */
TriObject* GetTriObjectFromNode(INode *node,const TimeValue t,bool &deleteIt)
{
	deleteIt = false;
	Object *obj = node->EvalWorldState(t).obj;
	if(obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID,0)))
    { 
		TriObject *tri = (TriObject *) obj->ConvertToType(0,Class_ID(TRIOBJ_CLASS_ID,0));
		// Note that the TriObject should only be deleted
		// if the pointer to it is not equal to the object
		// pointer that called ConvertToType()
		if (obj != reinterpret_cast<Object*>(tri))
            deleteIt = true;
		return tri;
	}
	else
		return NULL;
}

/**
 * Gets the rendered image aspect ratio.
 * 
 * \return      Returns a value of type float.
 */
float GetAspect() 
{
	return GetCOREInterface()->GetRendImageAspect();
}

/**
 * Gets the aperture width in millimeters.
 * 
 * \return      Returns a value of type float.
 */
float GetApertureWidth() 
{
	return GetCOREInterface()->GetRendApertureWidth();
}

/**
 * Gets the point targeted by a node. 
 * Returns FALSE is there is no target transformation matrix.
 * 
 * \param       t An argument of type TimeValue.
 * \param       inode An argument of type INode *.
 * \param       p An argument of type Point3 &.
 * \return      Returns a value of type BOOL.
 */
BOOL GetTargetPoint(TimeValue t, INode* inode, Point3& p) 
{
	Matrix3 tmat;
	if (inode->GetTargetTM(t,tmat)) {
		p = tmat.GetTrans();
		return TRUE;
	}
    else {
		return FALSE;
    }
}

/**
 * Gets the distance to a target, or 0.0f if there is no target.
 * 
 * \param       t An argument of type TimeValue.
 * \param       inode An argument of type INode *.
 * \return      Returns a value of type float.
 */
float GetTargetDistance(TimeValue t, INode* inode)
{
    Point3 pt;
    if (!GetTargetPoint(t, inode, pt))
        return 0.0f;
    return GetDistanceTo(inode->GetObjectTM(t), pt);
}

/**
 * Converts millimeter to field-of-view radians using the current aperture width.
 * 
 * \param       mm An argument of type float.
 * \param       width An argument of type float.
 * \return      Returns a value of type float.
 */
float MMtoFOV(float mm, float width = GetApertureWidth()) 
{
	return float(2.0f*atan(0.5f*width/mm));
}

/**
 * Converts field-of-view radians to millimeters using the current aperture width.
 * 
 * \param       fov An argument of type float.
 * \param       width An argument of type float.
 * \return      Returns a value of type float.
 */
float FOVtoMM(float fov, float width = GetApertureWidth())	
{
	return float((0.5f*width)/tan(fov/2.0f));
}

/**
 * Displays a dialog box that asks the user a simple yes or no question.
 * 
 * \param       str An argument of type const MSTR &.
 * \return      Returns a value of type BOOL.
 */
BOOL QueryUser(const MSTR& str)
{
    return MessageBox(GetCOREInterface()->GetMAXHWnd(), str, _M("Question"), MB_ICONQUESTION | MB_YESNO) == IDYES;
}

/**
 * Displays a simple dialog box with a message in it.
 * 
 * \param       str An argument of type const MSTR &.
 */
void MessageBox(const MSTR& str)
{
    MessageBox(GetCOREInterface()->GetMAXHWnd(), str, _M("Information"), MB_OK);
}

/**
 * Redraw all views at the specified time.
 * 
 * \param       t An argument of type TimeValue.
 */
void RedrawViews(TimeValue t = Now())
{
    GetCOREInterface()->RedrawViews(t);  
}

/**
 * Removes scaling from a node's transformation matrix, so that it appears always 
 * a constant size in the viewport (e.g. camera).
 * 
 * \param       t An argument of type TimeValue.
 * \param       inode An argument of type INode *.
 * \param       vpt An argument of type ViewExp *.
 * \param       tm An argument of type Matrix3 &.
 */
void GetUnscaledMatrix(TimeValue t, INode* inode, ViewExp* vpt, Matrix3& tm) 
{
    tm = inode->GetObjectTM(t);
    RemoveScaling(tm);
    float scaleFactor = vpt->NonScalingObjectSize() * vpt->GetVPWorldWidth(tm.GetTrans()) / 360.0f;
    if (scaleFactor!=(float)1.0)
	    tm.Scale(Point3(scaleFactor,scaleFactor,scaleFactor));
}

/**
 * Returns the distance between two positions represented as matrices.
 * 
 * \param       x An argument of type const Matrix3 &.
 * \param       y An argument of type const Matrix3 &.
 * \return      Returns a value of type float.
 */
float GetDistanceBetween(const Matrix3& x, const Matrix3& y)
{
    return Length(x.GetTrans() - y.GetTrans());    
}

/**
 * Returns the distance between two nodes.
 * 
 * \param       x An argument of type INode *.
 * \param       y An argument of type INode *.
 * \param       time An argument of type TimeValue.
 * \return      Returns a value of type float.
 */
float GetDistanceBetween(INode* x, INode* y, TimeValue time = Now()) 
{
    return GetDistanceBetween(x->GetObjectTM(time), y->GetObjectTM(time));
}

/**
 * Facilitates drawing to a GraphicsWindow 
 */
class DrawingContext
{
    GraphicsWindow* gw;

public:

    DrawingContext(GraphicsWindow* x)
        : gw(x)
    { }

    void DrawLine(const Point3& a, const Point3& b)
    {
        Point3 line[2 + 1] = { a, b, Point3() };
 	    gw->polyline( 2, line, NULL, NULL, TRUE, NULL );
    }    
};

/**
 * Rotates a node using the passed matrix
 * 
 * \param       src An argument of type INode *.
 * \param       mat An argument of type const Matrix3 &.
 * \param       time An argument of type TimeValue.
 */
void RotateToMatrix(INode* src, const Matrix3& mat, TimeValue time = Now())
{
    Matrix3 tm = src->GetObjectTM(Now());
    Quat q(tm); 
    Quat q2(mat);
    Quat q3 = q.Inverse() * q2; 
    src->Rotate(time, Identity(), q3, TRUE);
}


/**
 * Rotates a node so that it faces another node.
 * 
 * \param       src An argument of type INode *.
 * \param       target An argument of type INode *.
 * \param       time An argument of type TimeValue.
 */
void TurnNodeTowards(INode* src, INode* target, TimeValue time = Now())
{
    if (src == NULL || target == NULL)
        return;
    Matrix3 srcMat = src->GetObjectTM(time);
    Matrix3 targetMat = target->GetObjectTM(time);
    Point3 srcPoint = srcMat.GetTrans();
    Point3 targetPoint = targetMat.GetTrans();
    Matrix3 rotMat;
    rotMat.SetFromToUp(srcPoint, targetPoint, Point3::ZAxis);    
    RotateToMatrix(src, rotMat, time);
}

/**
 * Get the location associated with a particular node.
 * 
 * \param       node An argument of type INode *.
 * \param       time An argument of type TimeValue.
 * \return      Returns a value of type Point3.
 */
Point3 GetNodeLocation(INode* node, TimeValue time = Now())
{
    Matrix3 mat = node->GetObjectTM(time);
    return mat.GetTrans();
}

/**
 * Gets a vector from a point to a node.
 * 
 * \param       point An argument of type const Point3 &.
 * \param       target An argument of type INode *.
 * \param       time An argument of type TimeValue.
 * \return      Returns a value of type Point3.
 */
Point3 GetVectorTo(const Point3& point, INode* target, TimeValue time = Now())
{
    return GetNodeLocation(target, time) - point;
}

/**
 * Get distance from a point to a node. 
 * 
 * \param       point An argument of type const Point3 &.
 * \param       target An argument of type INode *.
 * \param       time An argument of type TimeValue.
 * \return      Returns a value of type float.
 */
float GetDistanceTo(const Point3& point, INode* target, TimeValue time = Now())
{
    return GetVectorTo(point, target, time).Length();
}

/**
 * Returns true if the point is equal to (0,0,0)
 *
 * \param       point An argument of type const Point3 &.
 * \return      Returns a value of type BOOL.
 */
BOOL IsZeroVector(const Point3& point)
{
    return point.Equals(Point3::Origin);
}
