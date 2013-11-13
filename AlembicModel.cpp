#include "stdafx.h"
#include "AlembicModel.h"
#include "AlembicXform.h"
#include "sceneGraph.h"
#include "CommonUtilities.h"

using namespace XSI;
using namespace MATH;


AlembicModel::AlembicModel(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent)
: AlembicObject(eNode, in_Job, oParent)
{
   //AbcA::ObjectWriterPtr parent = GetObjectWriterPtr( GetMyParent() );
   //AbcA::ObjectWriterPtr child = parent->getChild(eNode->name);
   //if(child != NULL){
   //   ESS_LOG_WARNING("duplicate name");
   //}

   const bool bRename = in_Job->GetOption("renameConflictingNodes");

   std::string uniqueName;
   if(bRename){
      uniqueName = getUniqueName(GetMyParent().getFullName(), eNode->name);
   }
   else{
      uniqueName = eNode->name;
   }
      
   AbcG::OXform xform(GetMyParent(), uniqueName, GetJob()->GetAnimatedTs());

   eNode->name = uniqueName; //need to this so that the correct parent is retrived later

   // create the generic properties
   mOVisibility = CreateVisibilityProperty(xform,GetJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();

   XSI::CRef parentGlobalTransRef;

   SceneNode* parent = mExoSceneNode->parent;

   while(parent){
      if(parent->selected){
         break;
      }
      parent = parent->parent;
   }

   if(parent){
      XSI::CRef nodeRef;
      nodeRef.Set(parent->dccIdentifier.c_str());
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

   const bool bTransCache = GetJob()->GetOption("transformCache");
   bool bGlobalSpace = GetJob()->GetOption(L"globalSpace");
   const bool bFlatten = GetJob()->GetOption(L"flattenHierarchy");

   if(bFlatten && mExoSceneNode->type == SceneNode::NAMESPACE_TRANSFORM){
      bGlobalSpace = true;
   }

   for(std::list<SceneNodePtr>::iterator it=mExoSceneNode->children.begin(); it != mExoSceneNode->children.end(); it++){
      if((*it)->type == SceneNode::CAMERA){
         //ESS_LOG_WARNING("Disabling global space export for camera");
         bGlobalSpace = false;
         break;
      }
   }

   // store the transform
   SaveXformSample(mExoSceneNode, mXformSchema, mXformSample, time, bTransCache, bGlobalSpace, bFlatten);
   
   if(mNumSamples == 0){
      if(!mXformXSINodeType.valid()){
         mXformXSINodeType = Abc::OUcharProperty(mXformSchema.getArbGeomParams(), ".xsiNodeType", mXformSchema.getMetaData(), GetJob()->GetAnimatedTs() );
      }

      if(mExoSceneNode->type == SceneNode::NAMESPACE_TRANSFORM){
         mXformXSINodeType.set(XSI_XformTypes::XMODEL);
      }
      else{
         mXformXSINodeType.set(XSI_XformTypes::XNULL);
      }
   }


   //// set the visibility
   //Property visProp;
   //prim.GetParent3DObject().GetPropertyFromName(L"Visibility",visProp);
   //if(isRefAnimated(visProp.GetRef()) || mNumSamples == 0)
   //{
   //   bool visibility = visProp.GetParameterValue(L"rendvis",time);
   //   mOVisibility.set(visibility ?AbcG::kVisibilityVisible :AbcG::kVisibilityHidden);
   //}

   bool bVisibility = mExoSceneNode->getVisibility(time);

   mOVisibility.set(bVisibility ?AbcG::kVisibilityVisible :AbcG::kVisibilityHidden);

   // store the metadata
   SaveMetaData(GetRef(REF_NODE),this);
   mNumSamples++;

   return CStatus::OK;
}

