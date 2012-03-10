/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

/**
 * Instantiations of this class template can be used as the base-class 
 * for plug-ins that derive from ReferenceMaker but that 
 * manage arbitrary but fixed number of reference targets. The plug-in type is passed 
 * as the template parameter, and a template instantiation will derive from it.
 * 
 * \see        ReferenceMaker
 */
template<typename Base_T>
class ReferenceManager 
    : public Base_T
{
private:

    /**
     * Stores information about each reference managed by ReferenceManager.
     */
    struct RefInfo
    {
        ReferenceTarget* m_target;
        BOOL m_isWeak;
        BOOL m_isPersisted;

        RefInfo(ReferenceTarget* target, BOOL isWeak, BOOL isPersisted)
            : m_target(target), m_isWeak(isWeak), m_isPersisted(isPersisted)
        {
        }
    };

    /** 
     * The array of reference targets. 
     * \remarks The template class "RefMgr" is not 
     * used because it may be deprecated in versions of the 3ds Max SDK after 2010.
     * The class IRefTargContainer is not used because it does not support node targets,
     * and was intended for use with dynamically changing lists of references.
     */
    MaxSDK::Array<RefInfo> m_refs;

public:

    ReferenceManager()
        : Base_T()
    { }
    
    ~ReferenceManager() 
    {
        // Informs 3ds Max that it is safe to delete the all references from and to this object
        // This will delete the reference connections but will not directly delete memory to objects.
        // Once any objects have no more strong references, they will be deleted. 
        // This should not be strictly necessary if all plug-ins and components behave 
        // as they should, but in this case being paranoid has little cost. 
        DeleteAllRefs();
    }

    //============================================================
    // ReferenceMaker / ReferenceTarget overloads
    
    /// \name ReferenceMaker Overrides
    //@{
    /**
     * Returns the number of references.
     * \return     Returns a value of type int.
     */
    virtual int NumRefs() 
    {
        return (int)m_refs.length();
    }

    /**
     * Returns the nth reference.
     *
     * \see        ReferenceMaker::GetReference()
     * 
     * \param  i An argument of type int.
     * \return     Returns a value of type RefTargetHandle.
     */
    virtual RefTargetHandle GetReference(int i) 
    {
        DbgAssert(IsValidReferenceIndex(i));
        if (!IsValidReferenceIndex(i))
            return NULL;
        return m_refs[i].m_target;
    }

	/**
	 * Set the nth reference. 
     * \note       Do not call this function directly, instead use ReplaceReference. 
	 * \see        ReferenceMaker::SetReference()
	 * 
	 * \param  i An argument of type int.
	 * \param  rtarg An argument of type RefTargetHandle.
	 */
	virtual void SetReference(int i, RefTargetHandle rtarg) 
    { 
        DbgAssert(IsValidReferenceIndex(i));
        if (!IsValidReferenceIndex(i))
            return;
        m_refs[i].m_target = rtarg;
    }

    /**
     * Called by 3ds Max in response to a change in an observed ReferenceTarget. 
     * 
     * \see        ReferenceMaker::NotifyRefChanged()
     * 
     * \param  changeInt An argument of type Interval.
     * \param  hTarget An argument of type RefTargetHandle.
     * \param  partID An argument of type PartID &.
     * \param  message An argument of type RefMessage.
     * \return     Returns a value of type RefResult.
     */
    virtual RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message ) 
   	{
	    switch (message) 
        {
        case REFMSG_TARGET_DELETED:            
            if (hTarget != NULL)
            {
                int n = GetReferenceIndex(hTarget);
                DbgAssert(n >= 0 && "Internal error, reference could not be found");
                if (n >= 0)
                    m_refs[n].m_target = NULL;
        	    return REF_SUCCEED;
            }
        }
        return REF_SUCCEED;
    }

    /**
     * Indicate whether the nth reference is a strong reference or not. 
     * \see        ReferenceMaker::IsRealDependency()
     * 
     * \param  rtarg An argument of type ReferenceTarget *.
     * \return     TRUE if a strong reference, FALSE if a weak reference.
     */
    virtual BOOL IsRealDependency(ReferenceTarget* rtarg)
    {
        int n = GetReferenceIndex(rtarg);
        if (n < 0)
            return FALSE;
        return !m_refs[n].m_isWeak;
    }

    /**
     * Called by 3ds Max for weak references, to see if a plug-in wishes for the 
     * reference links to be saved to file. 
     * \see        ReferenceMaker::ShouldPersistWeakRef
     * 
     * \param  rtarg An argument of type ReferenceTarget *.
     * \return     Returns a value of type BOOL.
     */
    virtual BOOL ShouldPersistWeakRef(ReferenceTarget* rtarg)
    {
        int n = GetReferenceIndex(rtarg);
        if (n < 0)
            return TRUE;
        return m_refs[n].m_isPersisted;
    }
    //@}

    /// \name ReferenceTarget Overrides
    //@{
    /**
     * This default implementation of ReferenceTarget::BaseClone() will clone all objects that have strong references 
     *
     * \see        ReferenceTarget::BaseClone()
     * 
     * \param  from An argument of type ReferenceTarget *.
     * \param  to An argument of type ReferenceTarget *.
     * \param  remap An argument of type RemapDir &.
     */
    virtual void BaseClone(ReferenceTarget* from, ReferenceTarget* to, RemapDir& remap) 
    {
        if (!from || !to || from == to)
            return;

        Base_T::BaseClone(from, to, remap);

        int numRefs = NumRefs();
        for (int i=0; i < numRefs; ++i)
        {
            ReferenceTarget* fromTarget = from->GetReference(i);

            if (from->IsRealDependency(fromTarget))
                to->ReplaceReference(i, remap.CloneRef(fromTarget));
            else
            {
                // We do not clone weak references, just copy them
                // however, the weakly referenced item may have been cloned
                // so we have to look for it.
                RefTargetHandle newTarget = remap.FindMapping( fromTarget );
                if (newTarget)
                    fromTarget = newTarget;
                to->ReplaceReference(i, fromTarget);       
            }
        }
    }

    //@}
    //========================================================================
    // New reference related methods
        
    /// \name ReferenceManager Specific Methods
    //@{
    /**
     * Registers a reference with the reference manager as the specified index. 
     * The n parameter is not strictly necessary, because it should always be equal to the number 
     * of references. This is the contract of using this function. By requiring it though, we 
     * help plug-in users be explicit about what the indexes of their references are.
     *
     * \note Do not call RegisterReference after construction. All fully constructed 
     * instances of a plug-in must have the same number of references if they want to 
     * use ReferenceManager. 
     * 
     * \param  n An argument of type int.
     * \param  ref An argument of type ReferenceTarget *.
     * \param  isWeak An argument of type BOOL.
     * \param  isPersisted An argument of type BOOL.
     * \return     Returns a value of type RefResult.
     */
    RefResult RegisterReference(int n, ReferenceTarget* ref, BOOL isWeak = FALSE, BOOL isPersisted = TRUE) 
    {
		/*
        DbgAssert(n == NumRefs() && "Can only register a reference in the last available place");
		*/
		while (n >= NumRefs())
		{
			m_refs.append(RefInfo(NULL, false, false));
		}

        // Add a new RefInfo structure to the array.
        m_refs[n] = RefInfo(NULL, isWeak, isPersisted);
        
        DbgAssert(isWeak || isPersisted == TRUE && "Strong references are always persisted automatically");

        // We have to call ReplaceReference to set the reference so that 3ds Max can 
        // track the reference correctly. This will also check that result is not a circular reference
        RefResult result = ReplaceReference(n, ref);
        
        return result;           
    }    

    /**
     * Casts the nth reference to a specific type.
     * 
     * \tparam     T the type to cast the nth reference to.
     * \param  n An argument of type int.
     * \return     Returns a value of type T.
     */
    template<typename T>
    T GetReferenceAs(int n) 
    {
		RefTargetHandle ref = GetReference(n);
		return dynamic_cast<T>(ref);
    }

    /**
     * Indicates whether \c n is a valid reference index.
     * 
     * \param  n An argument of type int.
     * \return     \c true if \c n is a valid reference index, \c false otherwise.
     */
    bool IsValidReferenceIndex(int n)
    {
        return n >= 0 && n < NumRefs();
    }

    /**
     * Given a ReferenceTarget, returns its reference index.
     * In the case of multiple references to the same item,  
     * the lowest index is given.      
     * Returns -1 if the ReferenceTarget is not observed.
     * 
     * \param  ref An argument of type ReferenceTarget *.
     * \return     Returns a value of type int.
     */
    int GetReferenceIndex(ReferenceTarget* ref) 
    {
        for (int i=0; i < NumRefs(); ++i)
            if (GetReference(i) == ref)
                return i;
        return -1;
    }
    
    /**
     * Returns TRUE if the nth index is a weak reference,
     * or FALSE otherwise.
     * 
     * \param  n An int indicating the reference index.
     * \return     Returns a value of type BOOL.
     */
    BOOL IsWeakRef(int n)
    {
        DbgAssert(IsValidReferenceIndex(n));
        return m_refs[n].m_isWeak;
    }
    //@}
};


