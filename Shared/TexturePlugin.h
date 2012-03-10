/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "MtlBasePlugin.h"

/**
 * A base-class for both 2D and 3D textures. It manages ParamDlgs
 * for both UVGen and XYZGen. 
 */
template<typename Base_T>
class TexturePlugin
    : public MtlBasePlugin<Base_T>
{
    // Note: these fields may get deleted (i.e. invalidated) at some arbitrary 
    // point by 3ds Max. They are only used in DlgSetThing.
    // This is slightly inefficient, because both all maps whether they are 
    // 2D or 3D will have access to both. This is inconsequential, and putting 
    // them here, reduce complexity in those classes. Besides, if you wanted 
    // you can now create a texture map that is both 2D and 3D.
    ParamDlg* dlgUVGen;
    ParamDlg* dlgXYZGen;
    
public:

    //===========================================================================
    // Constructor

    TexturePlugin(ClassDesc2* classDesc) 
        : MtlBasePlugin(classDesc), dlgUVGen(NULL), dlgXYZGen(NULL)
    {
    }

    //===========================================================================
    // MtlBase overrides

    /// \name MtlBase Overrides
    //@{
    virtual ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) 
    {
        IAutoMParamDlg* masterDlg = 
            dynamic_cast<IAutoMParamDlg*>(MtlBasePlugin::CreateParamDlg(hwMtlEdit, imp));
        DbgAssert(masterDlg != NULL);

        if (GetTheUVGen() != NULL)
        {
            dlgUVGen = GetTheUVGen()->CreateParamDlg(hwMtlEdit, imp);
            masterDlg->AddDlg(dlgUVGen);
        }

        if (GetTheXYZGen() != NULL)
        {
            dlgXYZGen = GetTheXYZGen()->CreateParamDlg(hwMtlEdit, imp);
            masterDlg->AddDlg(dlgXYZGen);
        }

	    return masterDlg;
    }  
    //@}

    //===========================================================================
    // Texmap overrides

    /// \name Texmap Overrides
    //@{
    /**
     * \see         Texmap::SetDlgThing()
     * 
     * \param       dlg An argument of type ParamDlg *.
     * \return      Returns a value of type BOOL.
     */
    virtual BOOL SetDlgThing(ParamDlg* dlg)
    {
	    if (dlg == dlgUVGen)
        {
		    dlgUVGen->SetThing(GetTheUVGen());
    	    return TRUE;
        }
	    else if (dlg == dlgXYZGen)
        {
            dlgXYZGen->SetThing(GetTheXYZGen());
    	    return TRUE;
        }
		else 
        {
            return FALSE;
        }
    }

    /**
     * Resets the validity interval and calls reset on the UVGen 
     * and XYZGen
     * 
     * \see         MtlBasePlugin::Reset() 
     */
    virtual void Reset()
    {
        if (GetTheUVGen() != NULL)
            GetTheUVGen()->Reset();
        if (GetTheXYZGen() != NULL)
            GetTheXYZGen()->Reset();
        MtlBasePlugin::Reset();
    }
    //@}
};
    
