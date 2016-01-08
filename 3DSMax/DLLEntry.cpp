#include "Alembic.h"
#include "AlembicCameraModifier.h"
#include "AlembicDefinitions.h"
#include "AlembicNames.h"
#include "AlembicNurbsModifier.h"
#include "stdafx.h"

HINSTANCE hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
  if (fdwReason == DLL_PROCESS_ATTACH) {
    hInstance = hinstDLL;
    DisableThreadLibraryCalls(hInstance);
  }

  return (TRUE);
}

#define MAX_DLL_EXPORT extern "C" __declspec(dllexport)

MAX_DLL_EXPORT const TCHAR* LibDescription()
{
  return _T("Exocortex Alembic for 3DS Max");
}

typedef ClassDesc2* ClassDescPtr;

ClassDesc2** getClassDescs(int& numClassDescs)
{
  static ClassDescPtr s_classDescs[] = {
      GetAlembicXformControllerClassDesc(),
      GetAlembicVisibilityControllerClassDesc(),
      GetAlembicParticlesClassDesc(),
      GetAlembicMeshTopoModifierClassDesc(),
      GetAlembicMeshGeomModifierClassDesc(),
      GetAlembicMeshNormalsModifierClassDesc(),
      GetAlembicMeshUVWModifierClassDesc(),
      GetEmptySplineObjectClassDesc(),
      GetEmptyPolyLineObjectClassDesc(),
      GetAlembicSplineGeomModifierClassDesc(),
      GetAlembicSplineTopoModifierClassDesc(),
      GetAlembicFloatControllerClassDesc(),
      GetAlembicCameraModifierClassDesc(),
      GetAlembicNurbsModifierClassDesc()};
  numClassDescs = sizeof(s_classDescs) / sizeof(ClassDescPtr);
  return s_classDescs;
};

// TODO: Must change this number when adding a new class
MAX_DLL_EXPORT int LibNumberClasses()
{
  int numClassDescs;
  getClassDescs(numClassDescs);
  return numClassDescs;
}

MAX_DLL_EXPORT ClassDesc* LibClassDesc(int i)
{
  int numClassDescs;
  ClassDesc2** ppClassDescs = getClassDescs(numClassDescs);
  if (i < numClassDescs) {
    return ppClassDescs[i];
  }
  return NULL;
}

MAX_DLL_EXPORT ULONG LibVersion() { return VERSION_3DSMAX; }
// Let the plug-in register itself for deferred loading
MAX_DLL_EXPORT ULONG CanAutoDefer() { return 0; }
