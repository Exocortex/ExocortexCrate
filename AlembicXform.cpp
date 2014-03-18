#include "stdafx.h"
#include "Alembic.h"

#include "SceneEnumProc.h"
#include "AlembicXform.h"
#include "Utility.h"
#include "AlembicMetadataUtils.h"
#include "AlembicPropertyUtils.h"

void GetObjectMatrix(TimeValue ticks, INode *node, Matrix3 &out, bool bFlattenHierarchy)
{
    ESS_PROFILE_FUNC();
    out = node->GetObjTMAfterWSM(ticks);
	
	if(bFlattenHierarchy) return;

    INode *pModelTransformParent = GetParentModelTransformNode(node);

	if (pModelTransformParent)
    {
        Matrix3 modelParentTM = pModelTransformParent->GetObjTMAfterWSM(ticks);
        out = out * Inverse(modelParentTM);
    }
}

void SaveXformSample(const SceneEntry &in_Ref, AbcG::OXformSchema &schema, AbcG::XformSample &sample, double time, bool bFlattenHierarchy)
{
    ESS_PROFILE_FUNC();
    //Note: this code is extremely slow! 

	// check if the transform is animated
    //if(schema.getNumSamples() > 0)
    //{
    //    if (!CheckIfNodeIsAnimated(in_Ref.node))
    //    {
    //        // No need to save transform after first frame for non-animated objects. 
    //        return;
    //    }
    //}

    // JSS : To validate, I am currently assuming that the ObjectTM is what we are seeking. This may be wrong. 
    // Model transform
    TimeValue ticks = GetTimeValueFromFrame(time);

    Matrix3 transformation;
    GetObjectMatrix(ticks, in_Ref.node, transformation, bFlattenHierarchy);

    // Convert the max transform to alembic
    Matrix3 alembicMatrix;
    ConvertMaxMatrixToAlembicMatrix(transformation, alembicMatrix);
    Abc::M44d iMatrix( alembicMatrix.GetRow(0).x,  alembicMatrix.GetRow(0).y,  alembicMatrix.GetRow(0).z,  0,
                                alembicMatrix.GetRow(1).x,  alembicMatrix.GetRow(1).y,  alembicMatrix.GetRow(1).z,  0,
                                alembicMatrix.GetRow(2).x,  alembicMatrix.GetRow(2).y,  alembicMatrix.GetRow(2).z,  0,
                                alembicMatrix.GetRow(3).x,  alembicMatrix.GetRow(3).y,  alembicMatrix.GetRow(3).z,  1);

    // save the sample
    sample.setMatrix(iMatrix);
    schema.set(sample);
}

void SaveCameraXformSample(const SceneEntry &in_Ref, AbcG::OXformSchema &schema, AbcG::XformSample &sample, double time, bool bFlatten)
{
   // check if the transform is animated
    //if(schema.getNumSamples() > 0)
    //{
    //    if (!CheckIfNodeIsAnimated(in_Ref.node))
    //    {
    //        // No need to save transform after first frame for non-animated objects. 
    //        return;
    //    }
    //}

   // Model transform
    TimeValue ticks = GetTimeValueFromFrame(time);
    
    Matrix3 transformation;
    GetObjectMatrix(ticks, in_Ref.node, transformation, bFlatten);

    // Cameras in Max are already pointing down the negative z-axis (as is expected from Alembic).
    // So we rotate it by 90 degrees so that it is pointing down the positive y-axis.
    Matrix3 rotation(TRUE);
    rotation.RotateX(-HALFPI);
    transformation = rotation * transformation;

    // Convert the max transform to alembic
    Matrix3 alembicMatrix;
    ConvertMaxMatrixToAlembicMatrix(transformation, alembicMatrix);
    Abc::M44d iMatrix( alembicMatrix.GetRow(0).x,  alembicMatrix.GetRow(0).y,  alembicMatrix.GetRow(0).z,  0,
                                alembicMatrix.GetRow(1).x,  alembicMatrix.GetRow(1).y,  alembicMatrix.GetRow(1).z,  0,
                                alembicMatrix.GetRow(2).x,  alembicMatrix.GetRow(2).y,  alembicMatrix.GetRow(2).z,  0,
                                alembicMatrix.GetRow(3).x,  alembicMatrix.GetRow(3).y,  alembicMatrix.GetRow(3).z,  1);

    // save the sample
    sample.setMatrix(iMatrix);
    schema.set(sample);
}

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
      customAttributes.defineCustomAttributes(mINode, userProperties, mXformSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs());
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

   //TODO: reimplement bounding box export

   // // Set the bounding box to be used to draw the dummy object on import
   // DummyObject *pDummyObject = static_cast<DummyObject*>(GetRef().obj);
   // Box3 maxBox = pDummyObject->GetBox();

   ////Abc::V3d minpoint(maxBox.pmin.x, maxBox.pmin.y, maxBox.pmin.z);   ////Abc::V3d maxpoint(maxBox.pmax.x, maxBox.pmax.y, maxBox.pmax.z);

   //Abc::V3d minpoint( ConvertMaxPointToAlembicPoint4(maxBox.pmin) );
   //Abc::V3d maxpoint( ConvertMaxPointToAlembicPoint4(maxBox.pmax) );
   //
   //// max point -> alembic point: (x, y, z) -> (x, z, -y)
   //// thus, since Zmin < zMax, then -Zmin > -Zmax. So we need to swap the z components
   //std::swap(minpoint.z, maxpoint.z);

   //mXformSchema.getChildBoundsProperty().set( Abc::Box3d(minpoint, maxpoint) );



   //const bool bFirstFrame = mNumSamples == 0;

   if(!bMergedSubtreeNodeParent){
      customAttributes.exportCustomAttributes(mINode, time);
   }

   //SaveUserProperties(mExoSceneNode, mXformSchema, GetCurrentJob()->GetAnimatedTs(), time, bFirstFrame);

   // Store the transformation
   //SaveXformSample(GetRef(), mXformSchema, mXformSample, time, false);

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
      SaveMetaData(mINode, this);
   }

   return true;
}

Abc::OCompoundProperty AlembicXForm::GetCompound()
{
    return mXformSchema;
}