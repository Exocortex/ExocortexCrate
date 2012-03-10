/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "ControlUtility.h"

/**
 * Provides default implementations of some of the basic Control
 * functions. 
 */
class ControlPlugin
    : public AnimatablePlugin<Control>
{
public:

	ControlPlugin(ClassDesc2* classDesc)
		: AnimatablePlugin(classDesc)
	{ }

    //================================================================================
    // Animatable overrides

    /// \name Animatable Overrides
    //@{
    /**
     * Calls the default AnimatablePlugin::BeginEditParams() implementation.
     * 
     * \see        Animatable::BeginEditParams
     * 
     * \param  ip An argument of type IObjParam *.
     * \param  flags An argument of type ULONG.
     * \param  prev An argument of type Animatable *.
     */
    virtual void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL) 
    {
        AnimatablePlugin::BeginEditParams(ip, flags, prev); 
    }

    /**
     * Calls the default AnimatablePlugin::EndEditParams() implementation,
     * but first assures that \c END_EDIT_REMOVEUI is or'd with the 
     * \c flags argument. 
     * 
     * \see        Animatable::EndEditParams()
     * 
     * \param  ip An argument of type IObjParam *.
     * \param  flags An argument of type ULONG.
     * \param  next An argument of type Animatable *.
     */
    virtual void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) 
    {
        // Assure that we set the parameter editing UI is removed
        // when no longer focusing on the object. 
        AnimatablePlugin::EndEditParams(ip, flags | END_EDIT_REMOVEUI, next); 
    }       
    //@}

    //================================================================================
    // Control overrides

	/// \name Control Overrides
	//@{
	/**
	 * Provides a default implementation of Control::Copy() which does nothing. 
	 * 
	 * \param  from An argument of type Control *.
	 */
	virtual void Copy(Control *from)
    {
    }

    /**
     * Provides a default implementation of Control::IsKeyable() that returns 
     * FALSE, indicating that the controller is not a key-frame controller. 
     * 
     * \return     Returns a value of type BOOL.
     */
    virtual BOOL IsKeyable() 
    {
        return FALSE;
    }
	//@}
};

//================================================================================
// A parameter base class for all parametric controllers

/**
 * This template class is used to create new base classes for different kinds 
 * of controller plug-ins based on the kinds of values that they manage. This is done
 * to provide type-safety and make it easier to implement the different kinds of plug-ins.
 */
template<typename GetAbsVal_T, typename GetRelVal_T, typename SetAbsVal_T, typename SetRelVal_T>
class ControlPluginTemplate : public ControlPlugin
{
public:

	ControlPluginTemplate(ClassDesc2* classDesc)
		: ControlPlugin(classDesc)
	{ }

    //============================================================================================
    // Control plug-in overrides 

    /// \name Control Overrides
    //@{
    /**
     * Provides a default implementation for Control::GetValue() based on the 
     * results of the pure virtual functions GetAbsoluteValue() and GetRelativeValue().
     * 
     * \param  t An argument of type TimeValue.
     * \param  val An argument of type void *.
     * \param  valid An argument of type Interval &.
     * \param  method An argument of type GetSetMethod.
     */
    virtual void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method=CTRL_ABSOLUTE)
    {
        if (method == CTRL_ABSOLUTE)
            GetAbsoluteValue(t, *reinterpret_cast<GetAbsVal_T*>(val), valid); else
            GetRelativeValue(t, *reinterpret_cast<GetRelVal_T*>(val), valid);
    }

    /**
     * Provides a default implementation for Control::SetValue() based on the 
     * results of the virtual functions SetAbsoluteValue() and SetRelativeValue().
     * 
     * \param  t An argument of type TimeValue.
     * \param  val An argument of type void *.
     * \param  commit An argument of type int.
     * \param  method An argument of type GetSetMethod.
     */
    virtual void SetValue(TimeValue t, void *val, int commit=1, GetSetMethod method=CTRL_ABSOLUTE)
    {
        if (method == CTRL_ABSOLUTE)
            SetAbsoluteValue(t, *reinterpret_cast<SetAbsVal_T*>(val), commit); else
            SetRelativeValue(t, *reinterpret_cast<SetRelVal_T*>(val), commit);
    }
    //@}

    //============================================================================================
    // New pure-virtual functions to override
    
    /// \name Pure Virtual Functions for All Controllers
    //@{
    /**
     * Classes that derive from ControlPluginTemplate can override this 
     * function to generate absolute values instead of having to implement
     * Control::GetValue() which is not type-safe. 
     *
     * \see        GetValue()
     * 
     * \param  t An argument of type TimeValue.
     * \param  val An argument of type GetAbsVal_T &.
     * \param  valid An argument of type Interval &.
     */
    virtual void GetAbsoluteValue(TimeValue t, GetAbsVal_T& val, Interval& valid) = 0;

    /**
     * Classes that derive from ControlPluginTemplate can override this 
     * function to generate relative values instead of having to implement
     * Control::GetValue() which is not type-safe. 
     *
     * \see        GetValue()
     * 
     * \param  t An argument of type TimeValue.
     * \param  val An argument of type GetRelVal_T &.
     * \param  valid An argument of type Interval &.
     */
    virtual void GetRelativeValue(TimeValue t, GetRelVal_T& val, Interval& valid) = 0;
    //@}

    /// \name Virtual Functions for Key-frame Controllers
    //@{
    /**
     * Allows the absolute value to be set at a specific time for a keyframe controller.
     * Does nothing by default. If you support this function, you should override
     * ControlPlugin::IsKeyable() to return TRUE.
     *
     * \see        SetValue()
     * 
     * \param  t An argument of type TimeValue.
     * \param  val An argument of type SetAbsVal_T &.
     * \param  commit An argument of type int.
     */
    virtual void SetAbsoluteValue(TimeValue t, SetAbsVal_T& val, int commit) { };
    /**
     * Allows the relative value to be set at a specific time for a keyframe controller.
     * Does nothing by default. If you support this function, you should override
     * ControlPlugin::IsKeyable() to return TRUE.
     *
     * \see        SetValue()
     * 
     * \param  t An argument of type TimeValue.
     * \param  val An argument of type SetRelVal_T &.
     * \param  commit An argument of type int.
     */
    virtual void SetRelativeValue(TimeValue t, SetRelVal_T& val, int commit) { };
    //@}
};

/**
 * A base class for controller plug-ins that manage float values.
 */
class FloatControlPlugin
    : public ControlPluginTemplate<float, float, float, float> 
{ 
public:
	FloatControlPlugin(ClassDesc2* classDesc)
		: ControlPluginTemplate(classDesc)
	{ 
	}

    /**
     * Identifies the plug-in as a float controller plug-in.
     * 
     * \return     Returns a value of type SClass_ID.
     */
    virtual SClass_ID SuperClassID()
    {
        return CTRL_FLOAT_CLASS_ID;
    }
};

/**
 * A base class for controller plug-ins that manage Point3 values.
 */
class Point3ControlPlugin 
    : public ControlPluginTemplate<Point3, Point3, Point3, Point3> 
{ 
public:
	Point3ControlPlugin(ClassDesc2* classDesc)
		: ControlPluginTemplate(classDesc)
	{ 
	}

    /**
     * Identifies the plug-in as a point controller plug-in.
     * 
     * \return     Returns a value of type SClass_ID.
     */
    virtual SClass_ID SuperClassID()
    {
        return CTRL_POINT3_CLASS_ID;
    }
};

/**
 * A base class for controller plug-ins that manage Matrix3 values.
 */
class Matrix3ControlPlugin
    : public ControlPluginTemplate<Matrix3, Matrix3, SetXFormPacket, SetXFormPacket> 
{ 
public:
	Matrix3ControlPlugin(ClassDesc2* classDesc)
		: ControlPluginTemplate(classDesc)
	{ 
	}

    /**
     * Identifies the plug-in as a matrix controller plug-in.
     * 
     * \return     Returns a value of type SClass_ID.
     */
    virtual SClass_ID SuperClassID()
    {
        return CTRL_MATRIX3_CLASS_ID;
    }
};

/**
 * A base class for controller plug-ins that manage position.
 */
class PositionControlPlugin
    : public ControlPluginTemplate<Point3, Matrix3, Point3, Matrix3> 
{ 
public:
	PositionControlPlugin(ClassDesc2* classDesc)
		: ControlPluginTemplate(classDesc)
	{ 
	}

    /**
     * Identifies the plug-in as a position controller plug-in.
     * 
     * \return     Returns a value of type SClass_ID.
     */
    virtual SClass_ID SuperClassID()
    {
        return CTRL_POSITION_CLASS_ID;
    }
};

/**
 * A base class for controller plug-ins that manage rotation.
 */
class RotationControlPlugin 
    : public ControlPluginTemplate<Quat, Matrix3, Quat, Matrix3> 
{ 
public:
	RotationControlPlugin(ClassDesc2* classDesc)
		: ControlPluginTemplate(classDesc)
	{ 
	}

    /**
     * Identifies the plug-in as a rotation controller plug-in.
     * 
     * \return     Returns a value of type SClass_ID.
     */
    virtual SClass_ID SuperClassID()
    {
        return CTRL_ROTATION_CLASS_ID;
    }
};

/**
 * A base class for controller plug-ins that manage scale.
 */
class ScaleControlPlugin
    : public ControlPluginTemplate<ScaleValue, Matrix3, ScaleValue, Matrix3> 
{ 
public:
	ScaleControlPlugin(ClassDesc2* classDesc)
		: ControlPluginTemplate(classDesc)
	{ 
	}

    /**
     * Identifies the plug-in as a scale controller plug-in.
     * 
     * \return     Returns a value of type SClass_ID.
     */
    virtual SClass_ID SuperClassID()
    {
        return CTRL_SCALE_CLASS_ID;
    }
};
