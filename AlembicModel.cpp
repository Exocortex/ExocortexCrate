#include "stdafx.h"
#include "AlembicModel.h"
#include "AlembicXform.h"

using namespace XSI;
using namespace MATH;


AlembicModel::AlembicModel(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent)
: AlembicObject(eNode, in_Job, oParent)
{
   AbcG::OXform xform(GetMyParent(), eNode->name, GetJob()->GetAnimatedTs());

   // create the generic properties
   mOVisibility = CreateVisibilityProperty(xform,GetJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();

   XSI::CRef parentGlobalTransRef;

   WeakSceneNodePtr parent = mExoSceneNode->parent;

   while(parent){
      if(parent->selected){
         break;
      }
      parent = parent->parent;
   }

   if(parent){
      XSI::CRef nodeRef;
      nodeRef.Set(mExoSceneNode->parent->dccIdentifier.c_str());
      XSI::X3DObject xObj(nodeRef);
      parentGlobalTransRef = xObj.GetKinematics().GetGlobal().GetRef();
   }

   AddRef(parentGlobalTransRef);//global transform of parent - ref 3
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
   Primitive prim(GetRef(REF_PRIMITIVE));

   // store the transform
   SaveXformSample(GetRef(REF_PARENT_GLOBAL_TRANS), GetRef(REF_GLOBAL_TRANS),mXformSchema, mXformSample, time,
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
   SaveMetaData(GetRef(REF_NODE),this);
   mNumSamples++;

   return CStatus::OK;
}

