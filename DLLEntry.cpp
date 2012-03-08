#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "iparamm2.h"

extern ClassDesc* GetAlembicXFormCtrlClassDesc();
extern ClassDesc* GetAlembicPolyMeshModifierDesc();
extern ClassDesc* GetAlembicCameraModifierDesc();
extern ClassDesc* GetAlembicVisCtrlClassDesc();
extern ClassDesc* GetAlembicSimpleParticleClassDesc();

HINSTANCE hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
   if( fdwReason == DLL_PROCESS_ATTACH )
   {
      hInstance = hinstDLL;
      DisableThreadLibraryCalls(hInstance);
   }

   return (TRUE);
}

#define MAX_DLL_EXPORT extern "C" __declspec( dllexport )

MAX_DLL_EXPORT const TCHAR* LibDescription()
{
   return "Exocortex Alembic for 3DS Max";
}

//TODO: Must change this number when adding a new class
MAX_DLL_EXPORT int LibNumberClasses()
{
   return 5;
}

MAX_DLL_EXPORT ClassDesc* LibClassDesc(int i)
{
    switch(i) 
    {
    case 0: return GetAlembicXFormCtrlClassDesc();
    case 1: return GetAlembicPolyMeshModifierDesc();
    case 2: return GetAlembicCameraModifierDesc();
    case 3: return GetAlembicVisCtrlClassDesc();
    case 4: return GetAlembicSimpleParticleClassDesc();

    default: return 0;
    }
}

MAX_DLL_EXPORT ULONG LibVersion()
{
   return VERSION_3DSMAX;
}

// Let the plug-in register itself for deferred loading
MAX_DLL_EXPORT ULONG CanAutoDefer()
{
   return 0;
}
