/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "TexturePlugin.h"

/**
 * Base class for 3D texture plug-ins.
 * Manages a XYZGen and uses it to provide many of the default 
 * implementations. 
 */
class Texture3DPlugin
    : public TexturePlugin<Tex3D>
{

public:

    Texture3DPlugin(ClassDesc2* classDesc)
        : TexturePlugin(classDesc)
    {
        RegisterReference(1, GetNewDefaultXYZGen());
        RegisterSubAnim(1, 1, _T("XYZGen"));
    }

    //=========================================================
    // TexMap overrides

    /// \name TexMap Overrides
    //@{
    /**
     * \see         TexMap::GetTheXYZGen()
     * 
     * \return      Returns a value of type XYZGen *.
     */
    XYZGen* GetTheXYZGen() 
    { 
        return GetReferenceAs<XYZGen*>(1);
    }

    /**
     * \see         TexMap::EvalColor()
     * 
     * \param       sc An argument of type ShadeContext &.
     * \return      Returns a value of type AColor.
     */
    AColor EvalColor(ShadeContext& sc)
    {
    	if (!sc.doMaps) return AColor(0.0f,0.0f,0.0f,0.0f);;
    	if (gbufID) sc.SetGBufferID(gbufID);
        return AColor(0.0f,0.0f,0.0f,0.0f);;
    }

    /**
     * \see         TexMap::EvalMono()
     * 
     * \param       sc An argument of type ShadeContext &.
     * \return      Returns a value of type float.
     */
    float EvalMono(ShadeContext& sc)
    {
	    if (!sc.doMaps) return 0.0f;
    	if (gbufID) sc.SetGBufferID(gbufID);
    	return Intens(EvalColor(sc));
    }

    /**
     * \see         TexMap::EvalNormalPerturb()
     * 
     * \param       sc An argument of type ShadeContext &.
     * \return      Returns a value of type Point3.
     */
    Point3 EvalNormalPerturb(ShadeContext& sc)
    {
	    if (!sc.doMaps) return Point3(0,0,0);
	    if (gbufID) sc.SetGBufferID(gbufID);
        return Point3(0,0,0);
    }

    /**
     * \see         TexMap::LocalRequirements()
     * 
     * \param       subMtlNum An argument of type int.
     * \return      Returns a value of type ULONG.
     */
    ULONG LocalRequirements(int subMtlNum)
    {
	    return GetTheXYZGen()->Requirements(subMtlNum); 
    }

	/**
	 * \see         TexMap::ReadSXPData()
	 * 
	 * \param       name An argument of type TCHAR *.
	 * \param       sxpdata An argument of type void *.
	 */
	void ReadSXPData(TCHAR *name, void *sxpdata) 
    { 
        // Do nothing
    }
    //@}
};
