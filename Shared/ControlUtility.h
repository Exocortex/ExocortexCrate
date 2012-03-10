/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

// A utility template class, used for the construction of wrappers around 
// controls for type safe GetValue and SetValue functions. Helps with error 
// detection at compile-time and code completion. You shouldn't use this class directly
// but instead one of the classes derived from it below, such as FloatControlWrapper, etc.
template<typename GetAbs_T, typename GetRel_T, typename SetAbs_T, typename SetRel_T>
class ControlWrapper
{
private:

    Control* control;

public:
    
    ControlWrapper(Control* ctrl)
        : control(ctrl)
    { }

    Control* GetCtrl()
    {
        return ctrl;
    }

    void GetAbsoluteValue(GetAbs_T& val, TimeValue t = GetCOREInterface()->GetTime(), Interval& valid = FOREVER)
    {
        return control->GetValue(t, &val, valid, CTRL_ABSOLUTE);
    }

    void GetRelativeValue(GetRel_T& val, TimeValue t = GetCOREInterface()->GetTime(), Interval& valid = FOREVER)
    {
        return control->GetValue(t, &val, valid, CTRL_RELATIVE);
    }

    void SetAbsoluteValue(const SetAbs_T& val, int commit = 1, TimeValue t = GetCOREInterface()->GetTime())
    {
        return control->SetValue(t, const_cast<void*>(&val), commit, CTRL_ABSOLUTE);
    }

    void SetRelativeValue(const SetRel_T& val, int commit = 1, TimeValue t = GetCOREInterface()->GetTime())
    {
        return control->SetValue(t, const_cast<void*>(&val), commit, CTRL_RELATIVE);
    }
};
    
//=========================================================================================
// These functions are useful mnemonics for checking what kind of control
// we are looking at, without having to remember the class ID's 

BOOL IsFloatController(Control* ctrl) { 
    return ctrl->SuperClassID() == CTRL_FLOAT_CLASS_ID; 
} 

BOOL IsPoint3Controller(Control* ctrl) { 
    return ctrl->SuperClassID() == CTRL_POINT3_CLASS_ID; 
} 

BOOL IsMatrix3Controller(Control* ctrl) { 
    return ctrl->SuperClassID() == CTRL_MATRIX3_CLASS_ID; 
} 

BOOL IsPositionController(Control* ctrl) { 
    return ctrl->SuperClassID() == CTRL_POSITION_CLASS_ID; 
} 

BOOL IsRotationController(Control* ctrl) { 
    return ctrl->SuperClassID() == CTRL_ROTATION_CLASS_ID; 
} 

BOOL IsScaleController(Control* ctrl) { 
    return ctrl->SuperClassID() == CTRL_SCALE_CLASS_ID; 
} 

//===============================================================================
// The following are used as wrappers around controllers for 
// type-safe getting and setting of values. 

struct FloatControlWrapper 
    : public ControlWrapper<float, float, float, float> 
{ 
    FloatControlWrapper(Control* ctrl)
        : ControlWrapper(ctrl)
    { 
        DbgAssert(IsFloatController(ctrl));
    }
};

struct Point3ControlWrapper 
    : public ControlWrapper<Point3, Point3, Point3, Point3> 
{ 
    Point3ControlWrapper(Control* ctrl)
        : ControlWrapper(ctrl)
    { 
        DbgAssert(IsPoint3Controller(ctrl));
    }
};

struct Matrix3ControlWrapper 
    : public ControlWrapper<Matrix3, Matrix3, SetXFormPacket, SetXFormPacket> 
{ 
    Matrix3ControlWrapper(Control* ctrl)
        : ControlWrapper(ctrl)
    { 
        DbgAssert(IsMatrix3Controller(ctrl));
    }
};

struct PositionControlWrapper 
    : public ControlWrapper<Point3, Matrix3, Point3, Matrix3> 
{ 
    PositionControlWrapper(Control* ctrl)
        : ControlWrapper(ctrl)
    { 
        DbgAssert(IsPositionController(ctrl));
    }
};

struct RotationControlWrapper 
    : public ControlWrapper<Quat, Matrix3, Quat, Matrix3> 
{ 
    RotationControlWrapper(Control* ctrl)
        : ControlWrapper(ctrl)
    { 
        DbgAssert(IsRotationController(ctrl));
    }
};

struct ScaleControlWrapper 
    : public ControlWrapper<ScaleValue, Matrix3, ScaleValue, Matrix3> 
{ 
    ScaleControlWrapper(Control* ctrl)
        : ControlWrapper(ctrl)
    { 
        DbgAssert(IsScaleController(ctrl));
    }
};

//====================================================================================
// Sub-controller querying and replacement utility functions

// Finds a sub-controller
ISubTargetCtrl* FindSubCtrl(Control* ctrl, Control*& subCtrl)
{
	ISubTargetCtrl* assign = NULL;
    Control* child = NULL;
	subCtrl = NULL;
	for ( ISubTargetCtrl* next = GetSubTargetInterface(ctrl); next != NULL; next = GetSubTargetInterface(child)) {
		child = next->GetTMController();
		if (child == NULL)
			break;
		if (next->CanAssignTMController()) {
			assign = next;
			subCtrl = child;
		}
	}

	return assign;
}

// Replaces a lookat controller
bool ReplaceSubLookatController(Control* old)
{
	Control* child = NULL;
	ISubTargetCtrl* assign = FindSubCtrl(old, child);
	if (assign == NULL)
		return false;
	DbgAssert(assign->CanAssignTMController()); 
    DbgAssert(child != NULL);

	Control *tmc = NewDefaultMatrix3Controller();
	tmc->Copy(child); // doesn't copy rotation, only scale and position.
	assign->AssignTMController(tmc);

	return true;
}

// Removes the target controller
void ClearTargetController(INode* node)
{
	Control* old = node->GetTMController();

	if (!ReplaceSubLookatController(old)) {
		Control *tmc = NewDefaultMatrix3Controller();
		tmc->Copy(old); // doesn't copy rotation, only scale and position.
		node->SetTMController(tmc);
	}
}

// Replaces the sub PRS-controller
bool ReplaceSubPRSController(Control* old, INode* targNode)
{
	Control* child = NULL;
	ISubTargetCtrl* assign = FindSubCtrl(old, child);
	if (assign == NULL)
		return false;
	DbgAssert(assign->CanAssignTMController() && child != NULL);

	Control *laControl = CreateLookatControl();
	laControl->SetTarget(targNode);
	laControl->Copy(child);
	assign->AssignTMController(laControl);

	return true;
}

// Sets the target controller
void SetTargetController(INode* node, INode* targNode)
{
	Control* old = node->GetTMController();
	if (!ReplaceSubPRSController(old, targNode)) {
		// assign lookat controller
		Control *laControl= CreateLookatControl();
		laControl->SetTarget(targNode);
		laControl->Copy(old);
		node->SetTMController(laControl);
	}
}