#include "stdafx.h"
#include "Alembic.h"

#include "SceneEnumProc.h"
#include "AlembicXform.h"
#include "Utility.h"
#include "AlembicMetadataUtils.h"
#include "AlembicPropertyUtils.h"


AlembicXForm::AlembicXForm(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent) : AlembicObject(eNode, in_Job, oParent), customAttributes("User Properties")
{
   const bool bRename = in_Job->GetOption("renameConflictingNodes");

   std::string uniqueName;
   if(bRename){
      uniqueName = getUniqueName(oParent.getFullName(), eNode->name);
   }
   else{
      uniqueName = eNode->name;
   }

   AbcG::OXform xform(GetOParent(), uniqueName, GetCurrentJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();

   mOVisibility = CreateVisibilityProperty(xform, GetCurrentJob()->GetAnimatedTs());

   if(!bMergedSubtreeNodeParent){
      Abc::OCompoundProperty userProperties = mXformSchema.getUserProperties();
      customAttributes.defineCustomAttributes(mMaxNode, userProperties, mXformSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs());
   }
}

AlembicXForm::~AlembicXForm()
{
    // we have to clear this prior to destruction this is a workaround for issue-171
    mOVisibility.reset();
}

bool AlembicXForm::Save(double time, bool bLastFrame)
{
    ESS_PROFILE_FUNC();

   const bool bTransCache = GetCurrentJob()->GetOption("transformCache");
   bool bGlobalSpace = GetCurrentJob()->GetOption("globalSpace");
   const bool bFlatten = GetCurrentJob()->GetOption("flattenHierarchy");


   if(this->mExoSceneNode->type == SceneNode::ITRANSFORM){

      // Set the bounding box to be used to draw the dummy object on import
      TimeValue ticks = GetTimeValueFromFrame(time);
      Object *obj = mMaxNode->EvalWorldState(ticks).obj;
      DummyObject *pDummyObject = static_cast<DummyObject*>(obj);
      Box3 maxBox = pDummyObject->GetBox();

      //Abc::V3d minpoint(maxBox.pmin.x, maxBox.pmin.y, maxBox.pmin.z);      //Abc::V3d maxpoint(maxBox.pmax.x, maxBox.pmax.y, maxBox.pmax.z);

      Abc::V3d minpoint( ConvertMaxPointToAlembicPoint4(maxBox.pmin) );
      Abc::V3d maxpoint( ConvertMaxPointToAlembicPoint4(maxBox.pmax) );

      // max point -> alembic point: (x, y, z) -> (x, z, -y)
      // thus, since Zmin < zMax, then -Zmin > -Zmax. So we need to swap the z components
      std::swap(minpoint.z, maxpoint.z);

      mXformSchema.getChildBoundsProperty().set( Abc::Box3d(minpoint, maxpoint) );

   }

   //const bool bFirstFrame = mNumSamples == 0;

   if(!bMergedSubtreeNodeParent){
      customAttributes.exportCustomAttributes(mMaxNode, time);
   }

   //SaveUserProperties(mExoSceneNode, mXformSchema, GetCurrentJob()->GetAnimatedTs(), time, bFirstFrame);

   if(bFlatten){//a hack needed to handle namespace xforms
      mXformSample.setMatrix(mExoSceneNode->getGlobalTransDouble(time));
      mXformSchema.set(mXformSample);
   }
   else{
      Imath::M44d parentGlobalTransInv = mExoSceneNode->parent->getGlobalTransDouble(time).invert();
      Imath::M44d transform = mExoSceneNode->getGlobalTransDouble(time) * parentGlobalTransInv;
      mXformSample.setMatrix(transform);
      mXformSchema.set(mXformSample);
   }

   bool bVisibility = mExoSceneNode->getVisibility(time);
   mOVisibility.set(bVisibility ?AbcG::kVisibilityVisible :AbcG::kVisibilityHidden);

   if(mXformSchema.getNumSamples() == 0)
   {
      SaveMetaData(mMaxNode, this);
   }

   return true;
}

Abc::OCompoundProperty AlembicXForm::GetCompound()
{
    return mXformSchema;
}