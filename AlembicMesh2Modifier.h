/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "resource.h"
#include "Shared/Common.h"
#include "macrorec.h"
#include "Shared/ModifierPlugin.h"
#include "AlembicMesh2Deformer.h"

const Class_ID MODIFIER_TUTORIAL_CLASS_ID = Class_ID(0x25725b80, 0x251b3ee9);

//============================================================
// Class descriptor declaration

/**
 * The class descriptor for AlembicMesh2Modifier.
 */
class AlembicMesh2ModifierClassDesc
    : public ClassDesc2 
{
public: 
    //---------------------------------------
    // ClassDesc2 overrides 

    virtual int IsPublic();
    virtual void* Create(BOOL loading = FALSE);
    virtual const MCHAR* ClassName();
    virtual SClass_ID SuperClassID();
    virtual Class_ID ClassID();
    virtual const MCHAR* Category();
    virtual const MCHAR* GetInternalName();
    virtual HINSTANCE HInstance();

    //---------------------------------------
    // Returns a singleton instance of the class descriptor
    static ClassDesc2* GetInstance();
}; 

//============================================================
// The plug-in definition

/**
 * This plug-in demonstrates a bend modifier which is a re-implementation
 * of the standard 3ds Max bend modifier. The following concepts are 
 * demonstrated: 
 * \li Implementing a plug-in derived from Modifier
 * \li Using a Deformer to implement a plug-in.
 */
class AlembicMesh2Modifier 
    : public ModifierPlugin<Modifier>
{
private:

	//============================================================
	// Member fields

	AlembicMesh2Deformer deformer;

public:
	//============================================================
	// Parameter Block Identifier
	static const BlockID        PBLOCK_ID = 0;

    //============================================================
    // Parameter identifiers 
    
    static const ParamID ANGLE_PARAM_ID = 0;
    static const ParamID DIR_PARAM_ID = 1;
    static const ParamID AXIS_PARAM_ID = 2;
    static const ParamID FROM_TO_PARAM_ID = 3;
    static const ParamID FROM_PARAM_ID = 4;
    static const ParamID TO_PARAM_ID = 5;

	AlembicMesh2Modifier();

	//============================================================
    // RefMaker overrides 

	virtual RefTargetHandle Clone(RemapDir &remap);

    //============================================================
    // ModifierPlugin overrides 

    virtual Deformer* GetDeformer(TimeValue t, ModContext &mc, Matrix3& mat, Matrix3& invmat);

    //=========================================================================
    // BendModifier specific functions for accessing the parameters.

    float GetAngle(TimeValue t);
    float GetDirection(TimeValue t = Now());
    int GetAxis(TimeValue t = Now());
    float GetFrom(TimeValue t = Now());
    float GetTo(TimeValue t = Now());
    void SetFrom(float x, TimeValue t = Now());
    void SetTo(float x, TimeValue t = Now());
    BOOL GetFromTo(TimeValue t = Now());
};

//======================================================================
