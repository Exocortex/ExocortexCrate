#include "stdafx.h"
#include "AlembicModel.h"
#include "AlembicXform.h"

using namespace XSI;
using namespace MATH;


AlembicModel::AlembicModel(exoNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent)
: AlembicObject(eNode, in_Job, oParent)
{
   Primitive prim(GetRef());
  AbcG::OXform xform(GetMyParent(), eNode->name, GetJob()->GetAnimatedTs());
   if((bool)in_Job->GetOption(L"flattenHierarchy")){
      AddRef(prim.GetParent3DObject().GetKinematics().GetGlobal().GetRef());
   }
   else{
      AddRef(prim.GetParent3DObject().GetKinematics().GetLocal().GetRef());
   }

   // create the generic properties
   mOVisibility = CreateVisibilityProperty(xform,GetJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();
}

AlembicModel::~AlembicModel()
{
   // we have to clear this prior to destruction
   // this is a workaround for issue-171
   mOVisibility.reset();
}

Abc::OCompoundProperty AlembicModel::GetCompound()
{
   return mXformSchema;
}

XSI::CStatus AlembicModel::Save(double time)
{
   // access the model
   Primitive prim(GetRef());

   // store the transform
   SaveXformSample(GetRef(1),mXformSchema,mXformSample,time,
      GetJob()->GetOption("transformCache"),GetJob()->GetOption(L"globalSpace"),GetJob()->GetOption(L"flattenHierarchy"));

   // set the visibility
   Property visProp;
   prim.GetParent3DObject().GetPropertyFromName(L"Visibility",visProp);
   if(isRefAnimated(visProp.GetRef()) || mNumSamples == 0)
   {
      bool visibility = visProp.GetParameterValue(L"rendvis",time);
      mOVisibility.set(visibility ?AbcG::kVisibilityVisible :AbcG::kVisibilityHidden);
   }

   // store the metadata
   SaveMetaData(prim.GetParent3DObject().GetRef(),this);
   mNumSamples++;

   return CStatus::OK;
}

