#ifndef _ALEMBIC_XFORM_H_
#define _ALEMBIC_XFORM_H_

#include "AlembicObject.h"

void SaveXformSample(
   //XSI::CRef parentKineStateRef, 
   //XSI::CRef kineStateRef,
   SceneNodePtr node,
   AbcG::OXformSchema & schema, 
   AbcG::XformSample & sample, 
   double time, 
   bool xformCache, 
   bool globalSpace,
   bool flattenHierarchy
);

#endif