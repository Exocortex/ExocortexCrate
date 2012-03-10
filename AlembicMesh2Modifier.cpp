/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "AlembicMesh2Modifier.h"

// Static DLL handle
static HINSTANCE hDllInstance = 0; 

//==========================================================================
// Class descriptor implementation

int AlembicMesh2ModifierClassDesc::IsPublic() 
{ 
    return 1; 
} 
void* AlembicMesh2ModifierClassDesc::Create(BOOL loading) 
{ 
    return new AlembicMesh2Modifier(); 
} 
const MCHAR* AlembicMesh2ModifierClassDesc::ClassName() 
{ 
    return _M("Modifier tutorial"); 
} 
SClass_ID AlembicMesh2ModifierClassDesc::SuperClassID() 
{ 
    return OSM_CLASS_ID;
} 
Class_ID AlembicMesh2ModifierClassDesc::ClassID() 
{ 
    return MODIFIER_TUTORIAL_CLASS_ID; 
} 
const MCHAR* AlembicMesh2ModifierClassDesc::Category() 
{ 
    return SDK_TUTORIALS_CATEGORY; 
}  
const MCHAR* AlembicMesh2ModifierClassDesc::GetInternalName() 
{ 
    return _M("AlembicMesh2Modifier"); 
} 
HINSTANCE AlembicMesh2ModifierClassDesc::HInstance() 
{ 
    return hDllInstance; 
} 
ClassDesc2* AlembicMesh2ModifierClassDesc::GetInstance() 
{
    static AlembicMesh2ModifierClassDesc desc;
    return &desc;
}

//============================================================
// Define the static parameter block descriptor

static float BIGFLOAT = 999999.0f;

// This class derives from ParamBlockDesc2, but adds a number of convenience functions 
// for constructing parameter block descriptors incrementally.
// The ParamBlockDescUtil assumes the flags "P_AUTO_UI" and "P_AUTO_CREATE"

class AlembicMesh2ModifierParams
    : public ParamBlockDescUtil
{
private:
    //============================================================
    // Allows for a call back mechanism on GetValue / SetValue calls. You can check the value
    // being passed by PB2Value and to manipulate it as necessary.
    // Implement the check here for from/to crossover.  

    class BendPBAccessor : public PBAccessor
    {
    public:
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int /*tabIndex*/, TimeValue /*t*/)    // set from v
		{
			AlembicMesh2Modifier* u = dynamic_cast<AlembicMesh2Modifier*>(owner);
			if (u == NULL)
				return;

			float from;
			float to;

			switch (id)
			{
				case AlembicMesh2Modifier::FROM_PARAM_ID:				
					to = u->GetTo();
					from = v.f;
					if (from > to) 
						u->SetTo(from);
					break;

				case AlembicMesh2Modifier::TO_PARAM_ID:
					from = u->GetFrom();
					to = v.f;
					if (from > to) 
						u->SetFrom(to);
					break;
			}
		}
	};

    BendPBAccessor  bendPBAccessor;

public:        

    AlembicMesh2ModifierParams()
        : ParamBlockDescUtil(
				  AlembicMesh2Modifier::PBLOCK_ID
				, _M("AlembicMesh2ModifierParameters")
				, IDS_RB_PARAMETERS
				, AlembicMesh2ModifierClassDesc::GetInstance()
				, 0
				, 0 // Reference System ID for the Parameter Block
				, IDD_BENDPARAM
				, IDS_RB_PARAMETERS
				, NULL
				)
    {
        int id = AlembicMesh2Modifier::ANGLE_PARAM_ID;
        AddParam(id, _T("Angle"),	TYPE_FLOAT,	P_RESET_DEFAULT|P_ANIMATABLE, IDS_ANGLE, end);
		ParamOption(id, p_default,  0.0f, end);
		ParamOption(id, p_range,    -BIGFLOAT, BIGFLOAT, end);
		ParamOption(id, p_ui,       TYPE_SPINNER, EDITTYPE_FLOAT, IDC_ANGLE, IDC_ANGLESPINNER, 0.5f, end);

        id = AlembicMesh2Modifier::DIR_PARAM_ID;
        AddParam(id, _T("Direction"),	TYPE_FLOAT,	P_RESET_DEFAULT|P_ANIMATABLE, IDS_DIR, end);
		ParamOption(id, p_default,  0.0f, end);
		ParamOption(id, p_range,    -BIGFLOAT, BIGFLOAT, end);
		ParamOption(id, p_ui,       TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DIR, IDC_DIRSPINNER, 0.5f, end);

        id = AlembicMesh2Modifier::AXIS_PARAM_ID;
        AddParam(id, _T("Axis"), TYPE_INT, P_RESET_DEFAULT,	IDS_AXIS, end);
		ParamOption(id, p_default,  2, end);
		ParamOption(id, p_ui,       TYPE_RADIO, 3,IDC_X,IDC_Y,IDC_Z, 
						p_vals,     0,1,2, end); // Notice here how p_vals is passed in with p_ui to ParamOption. 

		id = AlembicMesh2Modifier::FROM_TO_PARAM_ID;
        AddParam(id, _T("FromTo"), TYPE_BOOL, P_RESET_DEFAULT, IDS_FROMTO, end);
        ParamOption(id, p_default,  FALSE, end);
		ParamOption(id, p_ui,       TYPE_SINGLECHEKBOX, IDC_BEND_AFFECTREGION, end);

        id = AlembicMesh2Modifier::FROM_PARAM_ID;
        AddParam(id, _T("From"), TYPE_FLOAT, P_RESET_DEFAULT|P_ANIMATABLE, IDS_FROM, end);
		ParamOption(id, p_default,  0.0f, end);
		ParamOption(id, p_range,    -BIGFLOAT, 0.0f, end);
		ParamOption(id, p_ui,       TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_BEND_FROM, IDC_BEND_FROMSPIN, SPIN_AUTOSCALE, end);
		ParamOption(id, p_accessor, &bendPBAccessor, end);

        id = AlembicMesh2Modifier::TO_PARAM_ID;
        AddParam(id, _T("To"), TYPE_FLOAT, P_RESET_DEFAULT|P_ANIMATABLE, IDS_TO, end);
		ParamOption(id, p_default,	0.0f, end);
		ParamOption(id, p_range, 	0.0f, BIGFLOAT, end);
		ParamOption(id, p_ui,		TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_BEND_TO, IDC_BEND_TOSPIN, SPIN_AUTOSCALE, end);
		ParamOption(id, p_accessor, &bendPBAccessor, end);

        // Prevent further changes to the parameter block descriptor
        SetFinished(); 
    }

    // Create a parameter block descriptor
    static AlembicMesh2ModifierParams* GetInstance()
    {
        static AlembicMesh2ModifierParams desc;
        return &desc;
    }
};

//============================================================
// DLL Main functions 

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved) { 
    // NOTE: Do not call managed code from here.
    // You should do any initialization in LibInitialize
	hDllInstance = hinstDLL; 
	switch(fdwReason) { 
		case DLL_PROCESS_ATTACH: break; 
		case DLL_THREAD_ATTACH: break; 
		case DLL_THREAD_DETACH: break; 
		case DLL_PROCESS_DETACH: break; 
	} 
	return(TRUE); 
} 
__declspec( dllexport ) const TCHAR * LibDescription() 
{ 
	return _T("Modifier tutorial plug-in project."); 
} 
__declspec( dllexport ) int LibNumberClasses() 
{ 
	return 1;  
} 
__declspec( dllexport ) ClassDesc2* LibClassDesc(int i) 
{ 
    return AlembicMesh2ModifierClassDesc::GetInstance();
} 
__declspec( dllexport ) ULONG LibVersion() 
{ 
	return VERSION_3DSMAX; 
} 
__declspec( dllexport ) ULONG CanAutoDefer() 
{ 
	return 1; 
} 
__declspec( dllexport ) ULONG LibInitialize() 
{ 
    AlembicMesh2ModifierParams::GetInstance();
    return 1;
}
__declspec( dllexport ) ULONG LibShutdown() 
{ 
    return 1;
}

//==========================================================================================================
// AlembicMesh2Modifier implementations

AlembicMesh2Modifier::AlembicMesh2Modifier() : ModifierPlugin<Modifier>(AlembicMesh2ModifierClassDesc::GetInstance())
{
    // Ask the class descriptor to make the parameter blocks
    // This will trigger 3ds Max to call ReplaceReference with the 
    // constructed parameter block 
    AlembicMesh2ModifierClassDesc::GetInstance()->MakeAutoParamBlocks(this);
}

RefTargetHandle AlembicMesh2Modifier::Clone(RemapDir &remap) 
{        
    AlembicMesh2Modifier* r = new AlembicMesh2Modifier();
    BaseClone(this, r, remap);
    return r;
}

Deformer* AlembicMesh2Modifier::GetDeformer(TimeValue t, ModContext &mc, Matrix3& mat, Matrix3& invmat) 
{
    deformer = AlembicMesh2Deformer(t, mc, GetAngle(t), GetDirection(t),
        GetAxis(t), GetFrom(t), GetTo(t), GetFromTo(t), mat, invmat);
    return &deformer;
}

float AlembicMesh2Modifier::GetAngle(TimeValue t)
{
    return GetParameter<float>(ANGLE_PARAM_ID, t);
}

float AlembicMesh2Modifier::GetDirection(TimeValue t)
{
    return GetParameter<float>(DIR_PARAM_ID, t);
}

int AlembicMesh2Modifier::GetAxis(TimeValue t)
{
    return GetParameter<int>(AXIS_PARAM_ID, t);
}

float AlembicMesh2Modifier::GetFrom(TimeValue t)
{
    return GetParameter<float>(FROM_PARAM_ID, t);
}

float AlembicMesh2Modifier::GetTo(TimeValue t)
{
    return GetParameter<float>(TO_PARAM_ID, t);
}

void AlembicMesh2Modifier::SetFrom(float x, TimeValue t)
{
    SetParameter(FROM_PARAM_ID, x, t);
}

void AlembicMesh2Modifier::SetTo(float x, TimeValue t)
{
    SetParameter(TO_PARAM_ID, x, t);
}

BOOL AlembicMesh2Modifier::GetFromTo(TimeValue t)
{
    return GetParameter<BOOL>(FROM_TO_PARAM_ID, t);
}
