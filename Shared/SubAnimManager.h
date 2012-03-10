/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "ReferenceManager.h"

//=========================================================
// The template parameter Base_T is used as the Base class
// 
// The SubAnimManager manages sub-anims. The sub-anims are all assumed
// to be strong references registered with the ReferenceManager. 
// The number of sub-anims is assumed to be 
// fixed at construction. In order to manage the sub-animatables automatically 
// they must be register using RegisterSubAnim after they have been registered
// as Reference using RegisterReference.
//
// The main advantage of this class, is that it provides default implementations
// of many of the sub-animatable related functions from the Animatable base class.
// It also provides a large amount of contract checking.

/**
 * Implements the sub-animatable related functions for a plug-in. The sub-animatables are 
 * assumed to already be registered with the ReferenceManager as strong references. 
 * Sub-animatables must first be registered by calling ReferenceManager::RegisterReference()
 * then they are registered by called SubAnimmanager::RegisterSubAnim()
 * 
 * \see        Animatable
 */
template<typename Base_T>
class SubAnimManager : 
    public ReferenceManager<Base_T>
{
private:

    /// Used to store information about each sub-animatable
    struct SubAnimInfo
    {
        SubAnimInfo(int refIndex, 
            MCHAR* name, BOOL canDelete, BOOL canCopy, BOOL canAssign, BOOL canMakeUnique
            ) : 
                m_refIndex(refIndex), m_name(name), m_canDelete(canDelete)
                ,m_canCopy(canCopy), m_canAssign(canAssign), m_canMakeUnique(canMakeUnique)
        { }

        int m_refIndex;
        MCHAR* m_name;
        BOOL m_canDelete;
        BOOL m_canCopy;
        BOOL m_canAssign;
        BOOL m_canMakeUnique;
    };
    
    /// The internal array of structures describing each sub-animatables.
    MaxSDK::Array<SubAnimInfo> m_subAnims;
    
    /**
     * Used to retrieve information about a sub-animatable.
     * 
     * \param  n An argument of type int.
     * \return     Returns a value of type SubAnimManager<Base_T>::SubAnimInfo &.
     * \see        Related API elements
     */
    SubAnimInfo& GetSubAnimInfo(int n) 
    {
        DbgAssert(IsValidSubAnimIndex(n));
        return m_subAnims[n];
    }

public:

    /// \name Animatable Overrides
    //@{
    /**
     * Returns the number of sub-animatables. 
     * 
     * \return     Returns a value of type int.
     * \see        Animatables::NumSubs()
     */
    virtual int NumSubs() 
    { 
        return (int)m_subAnims.length();
    }  
    
	/**
	 * Returns nth sub-animatable.
	 * 
	 * \param  n An argument of type int.
	 * \return     Returns a value of type Animatable *.
	 * \see        Animatable::SubAnim()
	 */
	virtual Animatable* SubAnim(int n) 
    { 
        DbgAssert(IsValidSubAnimIndex(n));
        if (!IsValidSubAnimIndex(n))
            return NULL;
        int refNum = SubAnimToRefNum(n);
        return GetReferenceAs<Animatable*>(refNum);
    }
    
    /**
     * Returns the name of the nth sub-animatable.
     * 
     * \param  i An argument of type int.
     * \return     Returns a value of type MSTR.
     * \see        Animatable::SubAnimName()
     */
    virtual MSTR SubAnimName(int i) 
    {
        return GetSubAnimInfo(i).m_name;
    }
    
    /**
     * Returns TRUE if the sub-animatable can be copied and pasted in the track view
     * 
     * \param  n An argument of type int.
     * \return     Returns a value of type BOOL.
     * \see        Animatable::CanCopyAnim()
     */
    virtual BOOL CanCopyAnim(int n)
    {
        return GetSubAnimInfo(n).m_canCopy;
    }
    
    /**
     * Returns FALSE if the sub-animatable prohibits being made unique from the UI
     * 
     * \param  n An argument of type int.
     * \return     Returns a value of type BOOL.
     * \see        Animatable::CanMakeUnique()
     */
    virtual BOOL CanMakeUnique(int n)
    {
        return GetSubAnimInfo(n).m_canMakeUnique;
    }
   
    /**
     * Returns true if the specified sub-animatable can be reassigned. 
     * 
     * \param  n An argument of type int.
     * \return     Returns a value of type BOOL.
     * \see        Animatable::CanAssignController()
     */
    virtual BOOL CanAssignController(int n)
    {
        return GetSubAnimInfo(n).m_canAssign;
    }
    
    /**
     * Called by 3ds Max when the controller is reassigned.
     * 
     * \param  control An argument of type Animatable *.
     * \param  subAnim An argument of type int.
     * \return     Returns a value of type BOOL.
     * \see        Animatable::AssignController()
     */
    virtual BOOL AssignController(Animatable* control, int subAnim)
    {
        DbgAssert(CanAssignController(subAnim) && "Cannot assign the specified sub-anim");

        // Convert the control into a reference target
        ReferenceTarget* ref = dynamic_cast<ReferenceTarget*>(control);

        // Because the sub-animatable is a reference we ask 3ds Max to replace the reference
        ReplaceReference(SubAnimToRefNum(subAnim), ref);

        // Finally we notify the dependents of the change to a controller
        NotifyDependents(FOREVER, 0, REFMSG_CONTROLREF_CHANGE);
        return TRUE;
    }
    
    /**
     * Returns TRUE if the sub-animatable can be deleted by the UI or programmatically 
     * 
     * \param  n An argument of type int.
     * \return     Returns a value of type BOOL.
     * \see        Animatable::CanDeleteSubAnim()
     */
    virtual BOOL CanDeleteSubAnim(int n)
    {
        return GetSubAnimInfo(n).m_canDelete;
    }

    /**
     * Delete the nth sub-animatable.
     * 
     * \param  n An argument of type int.
     * \see        Animatable::DeleteSubAnim()
     */
    virtual void DeleteSubAnim(int n)
    {
        DbgAssert(CanDeleteSubAnim(n) && "Cannot delete the specified sub-anim");

        // Because the sub-animatable is a reference we ask 3ds Max to delete the reference
        DeleteReference(SubAnimToRefNum(n));
    }

    /**
     * Used for copying and pasting in the track view. It converts an anim index to a reference index or returns 
     * -1 if there is no correspondence. If a client does not wish an anim to be copied or pasted then it can 
     * return -1 even if there is a corresponding reference number.
     * 
     * \param  subNum An argument of type int.
     * \return     Returns a value of type int.
     * \see        Animatable::SubNumToRefNum()
     */
    virtual int SubNumToRefNum(int subNum)
    {
        if (!CanCopyAnim(subNum)) return -1;
        return SubAnimToRefNum(subNum);
    }
    //@}

    /// \name Functions introduced by SubAnimManager
    //@{
    /**
     * Associates a sub-anim index with a reference index. 
     * Only call this after registering all references
     * 
     * \param  nSubAnimIndex An argument of type int.
     * \param  nRefIndex An argument of type int, the index of the reference which is used as a sub-animatable..
     * \param  name An argument of type char *.
     * \param  canDelete An argument of type BOOL.
     * \param  canCopy An argument of type BOOL.
     * \param  canAssign An argument of type BOOL.
     * \param  canMakeUnique An argument of type BOOL.
     */
    void RegisterSubAnim(int nSubAnimIndex, int nRefIndex, MCHAR* name, BOOL canDelete = FALSE, BOOL canCopy = TRUE, BOOL canAssign = TRUE, BOOL canMakeUnique = TRUE)
    {
        // Make sure that the reference index is valid.
        DbgAssert(IsValidReferenceIndex(nRefIndex));

        // Technically we could guess what nSubAnimIndex is, but this way avoids confusion
        DbgAssert(nSubAnimIndex == NumSubs() && "Can only add sub-anims to the last place");
        
        // Make sure that we are not mapping to an already used place
#ifdef _DEBUG
        for (int i=0; i < NumSubs(); ++i) 
            DbgAssert(m_subAnims[i].m_refIndex != nRefIndex && "The reference index is already used");
#endif

        // Create a new subAnimInfo structure and copy it into the list
        SubAnimInfo subAnim(nRefIndex, name, canDelete, canCopy, canAssign, canMakeUnique);        
        m_subAnims.append(subAnim);
    }
    
    /**
     * Will return true if the number is not a valid sub-animatable index
     * 
     * \param  n An argument of type int.
     * \return     Returns a value of type BOOL.
     * \see        Related API elements
     */
    BOOL IsValidSubAnimIndex(int n)
    {
        return n >= 0 && n < NumSubs();
    }

    /**
     * Converts a sub-animatable index into a reference index. Unlike the override of 
     * Animatable::SubNumToRefNum() it does not return -1 if copying and pasting is 
     * disallowed.
     * 
     * \param  n An argument of type int.
     * \return     Returns a value of type int.
     * \see        SubNumToRefNum()
     */
    int SubAnimToRefNum(int n)
    {
        return GetSubAnimInfo(n).m_refIndex;
    }

    /**
     * Converts a reference number to sub-animatable index.
     * 
     * \param  n An argument of type int.
     * \return     Returns a value of type int.
     * \see        Related API elements
     */
    int RefNumToSubAnim(int n)
    {
        // Not blazingly fast, but simple. Should be sufficient because it is only 
        // the number of sub-anims should be minimal.
        for (int i=0; i < NumSubs(); ++i) 
            if (SubAnimToRefNum(i) == n) 
                return i;
        return -1;
    }    
    //@}
};

