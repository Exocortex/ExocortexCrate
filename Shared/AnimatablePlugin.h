/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "SubAnimManager.h"

/**
 * A class template for use as a base class for plug-ins that derive from 
 * ReferenceTarget. An instantiation of this
 * class template will derive ultimately from the the template parameter Base_T. 
 * It is required that Base_T is a class that derives from ReferenceTarget. 
 * This class assumes that the plug-in has a single parameter block as reference 0 
 * which is created automatically by the ClassDescriptor. This is done by 
 * passing the flag P_AUTO_CONSTRUCT to the parameter block descriptor (ParamBlockDesc2)
 * or by constructing the parameter blocks using ParamBlockDesc2.
 * This class is similar to SimpleMod, SimpleMod2, SimpleObj, and SimpleObj2. 
 */
template<typename Base_T>
class AnimatablePlugin
    : public SubAnimManager<Base_T>    
{
private:

    /// Identifies the main parameter block sub-anim
    static const int PARAM_BLOCK_SUBANIM_INDEX = 0;

    /// A cached pointer to class descriptor 
	ClassDesc2* m_classDesc;

	/// Default construction is not allowed. 
	AnimatablePlugin()
	{ }

public:

    /**
     * Creates an animatable plug-in which is associated with a particular class descriptor. 
     * 
     * \param  classDesc An argument of type ClassDesc2 *.
     * \see        Related API elements
     */
    AnimatablePlugin(ClassDesc2* classDesc)
        : m_classDesc(classDesc)
    {
		assert(m_classDesc);

        // Register a NULL parameter-block reference with the ReferenceManager base class
        RegisterReference(GetParamBlockRefIndex(), NULL);

        // Register the parameter block as a sub-animatable with the SubAnimManager base
        // class
        RegisterSubAnim(PARAM_BLOCK_SUBANIM_INDEX, GetParamBlockRefIndex(), "Parameters");
    }

    /// \name Reference Maker Overrides
    //@{
    /**
     * The override of NotifyRefChanged handles parameter changes by calling the virtual 
     * function OnParamChangeMsg() and handles deletions by calling OnRefDeleted(). 
     * REFMSG_GET_PARAM_DIM, and REFMSG_GET_PARAM_NAME. 
     * 
     * \param  changeInt An argument of type Interval.
     * \param  hTarget An argument of type RefTargetHandle.
     * \param  partID An argument of type PartID &.
     * \param  message An argument of type RefMessage.
     * \return     Returns a value of type RefResult.
     * \see        ReferenceMaker::NotifyRefChanged()
     */
    virtual RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message ) 
   	{
        RefResult result = REF_DONTCARE;

	    switch (message) 
        {
            // Reference message representing a change in one of the targets 
            case REFMSG_CHANGE: 
            {   
                // If the target is a parameter block, then we do handle it 
                IParamBlock2* pb = dynamic_cast<IParamBlock2*>(hTarget);
                if (pb != NULL)
                    OnParamChangeMsg(pb->LastNotifyParamID());
			    result = REF_SUCCEED; 
                break;
            }
            // Reference message representing a request for the dimension of a parameter
		    case REFMSG_GET_PARAM_DIM: 
            {
			    GetParamDim *gpd = (GetParamDim*)partID;
			    gpd->dim = GetMainParamBlock()->GetParamDimension(GetMainParamBlock()->IndextoID(gpd->index));			
			    result = REF_HALT; 
                break;
            }
            // Reference message representing a request for the name of a parameter
            case REFMSG_GET_PARAM_NAME: 
            {
                ::GetParamName *gpn = (::GetParamName*)partID;                
			    gpn->name = GetMainParamBlock()->GetLocalName(GetMainParamBlock()->IndextoID(gpn->index));			
			    result = REF_HALT; 
                break;
            }
            // Reference message informing us of a deletion. 
            case REFMSG_TARGET_DELETED:
            {
                OnRefDeleted(hTarget);
			    result = ReferenceManager::NotifyRefChanged(changeInt, hTarget, partID, message);
                break;
            }
            default: 
                result = ReferenceManager::NotifyRefChanged(changeInt, hTarget, partID, message);
                break;
		}

        return result;
	}
    //@}

    /// \name Animatable Overrides
    //@{    
    /**
     * Returns the number of parameter blocks. 
     * 
     * \return     Returns a value of type int.
     * \see        Animatable::NumParamBlocks()
     */
    virtual int NumParamBlocks() 
    {
        return 1;
    }

    /**
     * Returns the nth parameter block. 
     *
     * \param  n An argument of type int.
     * \return     Returns a value of type IParamBlock2 *.
     * \see        Animatable::GetParamBlock()
     */
    virtual IParamBlock2* GetParamBlock(int n) 
    {
        if (n != 0) return NULL;
        return GetMainParamBlock();
    }    

    /**
     * Returns a parameter block with the specified id.  
     * 
     * \param  id An argument of type short.
     * \return     Returns a value of type IParamBlock2 *.
     * \see        Animatable::GetParamBlockByID()
     */
    virtual IParamBlock2* GetParamBlockByID(short id) 
    {
        if (id != 0) return NULL;
        return GetMainParamBlock();
    }

    /**
     * Called when the roll-out panel is about to be shown.
     * 
     * \param  ip An argument of type IObjParam *.
     * \param  flags An argument of type ULONG.
     * \param  prev An argument of type Animatable *.
     * \see        Animatable::BeginEditParams()
     */
    virtual void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) 
    {
        GetClassDesc()->BeginEditParams(ip, this, flags, prev);
    }

    /**
     * Called when the roll-out panel is about to be closed. 
     * 
     * \param  ip An argument of type IObjParam *.
     * \param  flags An argument of type ULONG.
     * \param  next An argument of type Animatable *.
     * \see        Animatable::EndEditParams()
     */
    virtual void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) 
    {
        ip->ClearPickMode(); // prevents crash on undo, while pickmode is active.
        GetClassDesc()->EndEditParams(ip, this, flags, next);
    }   

    /**
     * Returns the class ID.
     * 
     * \return     Returns a value of type Class_ID.
     * \see        Animatable::ClassID()
     */
    virtual Class_ID ClassID() 
    { 
        return GetClassDesc()->ClassID(); 
    }		

	/**
	 * Returns the super-class name.
	 * 
	 * \return     Returns a value of type SClass_ID.
	 * \see        Animatable::SuperClassID()
	 */
	virtual SClass_ID SuperClassID() 
    { 
        return GetClassDesc()->SuperClassID(); 
    }

	/**
	 * Returns the class name.
	 * 
	 * \param  s An argument of type MSTR &.
	 * \see        Animatable::GetClassName()
	 */
	virtual void GetClassName(MSTR& s) 
    {
        s = GetClassDesc()->ClassName();
    }    
    //@}
   
    /// \name BaseObject Overrides
    //@{
    /**
     * An animatable plug-in may or may not derive from a BaseObject, 
     * but if it does it must override this function. It is placed 
     * 
     * \param  id An argument of type int.
     * \return     Returns a value of type int.
     * \see        BaseObject::GetParamBlockIndex()
     */
    virtual int GetParamBlockIndex(int id)
	{
        return id == 0 ? 0 : -1;
	}
    //@}

    /// \name Utility Functions
    //@{
    /**
     * Returns a parameter block.     
     * 
     * \return     Returns a value of type IParamBlock2 *.
     */
    IParamBlock2* GetMainParamBlock() 
    {
        IParamBlock2* r = GetReferenceAs<IParamBlock2*>(GetParamBlockRefIndex());
        
        // The pblock variable is expected to be set by 3ds Max. This will happen through
        // a call to the ReferenceMaker::SetReference() method on the plug-in.        
        // The class descriptor will tell 3ds Max to create the parameter blocks 
        // automatically. 
        DbgAssert(r != NULL && "Parameter block was not set correctly");

        return r; 
    }
    
    
    /**
     * Retrieves parameters. 
     * 
     * \tparam     T the type of the parameter to retrieve.
     * \param  id An argument of type ParamID.
     * \param  t An argument of type TimeValue.
     * \return     Returns a value of type T.
     */
    template<typename T>
    T GetParameter(ParamID id, TimeValue t = Now())
    {
        T r = T(); // Initialized, but should always work.
		IParamBlock2* pb = GetMainParamBlock();
		// Should never ever be NULL
		if (pb)
		{			
			pb->GetValue(id, t, r, FOREVER);
		}
        return r;
    }
    
    /**
     * Sets the value of a particular parameter.
     * 
     * \tparam     T the type of the parameter to set.
     * \param  id An argument of type ParamID.
     * \param  x An argument of type T.
     * \param  t An argument of type TimeValue.
     */
    template<typename T>
    void SetParameter(ParamID id, T x, TimeValue t = Now())
    {
        GetMainParamBlock()->SetValue(id, t, x);
		InvalidateUI();
    }
    
    /**
     * Forces the parameter block descriptor to update the roll-out UI.
     */
    void InvalidateUI() 
    {
    	GetParamBlockDesc()->InvalidateUI();
    }

    /**
     * Returns the parameter block descriptor.
     * 
     * \return     Returns a value of type ParamBlockDesc2 *.
     */
    ParamBlockDesc2* GetParamBlockDesc()
    {
        return GetMainParamBlock()->GetDesc();
    }
    
    /**
     * Resets all parameters to their default values.
     * 
     * \param  updateUI An argument of type BOOL.
     * \param  callSetHandlers An argument of type BOOL.
     */
    void ResetParams(BOOL updateUI = TRUE, BOOL callSetHandlers = TRUE)
    {
        GetClassDesc()->Reset(this, updateUI, callSetHandlers);
    }

	/**
	 * Provides access to the class descriptor passed to the constructor.
	 * 
	 * \return     Returns a value of type ClassDesc2 *.
	 */
	ClassDesc2* GetClassDesc() 
	{
		assert(m_classDesc != NULL);
		return m_classDesc;
	}

	/**
	 * Returns the reference index of the parameter block. 
	 * 
	 * \return     Returns a value of type int.
	 */
	int GetParamBlockRefIndex()
	{
		return GetClassDesc()->GetParamBlockDesc(0)->ref_no;
	}
    //@}

    /// \name Virtual Functions
    //@{
    /**
     * Override if you want to respond to reference messages notifying the plug-in of changes to
     * parameters.
     * 
     * \param  id An argument of type ParamID.
     */
    virtual void OnParamChangeMsg(ParamID id) 
    {
        // Does nothing by default.
    }

    /**
     * Override if you want to respond to reference messages notifying the plug-in of deletions
     * to observed plug-ins.
     * 
     * \param  ref An argument of type ReferenceTarget*.
     */
    virtual void OnRefDeleted(ReferenceTarget* ref) 
    {
        // Does nothing by default.
    }
    //@}
};

