#include "stdafx.h"
#include "sceneGraph.h"
#include "Utility.h"


//Import methods, we won't need these until we update the importer
bool SceneNodeMax::replaceData(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAlembicPtr& nextFileNode)
{ 
   return false; 
}

bool SceneNodeMax::addChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode){ 
   return false; 
}

void SceneNodeMax::print()
{
   if(node){
      ESS_LOG_WARNING("MaxNode: "<<node->GetName());
   }
   else{
      ESS_LOG_WARNING("MaxNode: NULL");
   }
}

Imath::M44f SceneNodeMax::getGlobalTransFloat(double time)
{
   TimeValue ticks = GetTimeValueFromFrame(time);

   Matrix3 out = node->GetObjTMAfterWSM(ticks);

   if(this->bIsCameraTransform){
      // Cameras in Max are already pointing down the negative z-axis (as is expected from Alembic).
      // So we rotate it by 90 degrees so that it is pointing down the positive y-axis.
      Matrix3 rotation(TRUE);
      rotation.RotateX(-HALFPI);
      out = rotation * out;
   }

   // Convert the max transform to alembic
   Matrix3 alembicMatrix;
   ConvertMaxMatrixToAlembicMatrix(out, alembicMatrix);
   return Abc::M44f( 
      alembicMatrix.GetRow(0).x,  alembicMatrix.GetRow(0).y,  alembicMatrix.GetRow(0).z,  0,
      alembicMatrix.GetRow(1).x,  alembicMatrix.GetRow(1).y,  alembicMatrix.GetRow(1).z,  0,
      alembicMatrix.GetRow(2).x,  alembicMatrix.GetRow(2).y,  alembicMatrix.GetRow(2).z,  0,
      alembicMatrix.GetRow(3).x,  alembicMatrix.GetRow(3).y,  alembicMatrix.GetRow(3).z,  1);
}

Imath::M44d SceneNodeMax::getGlobalTransDouble(double time)
{
   TimeValue ticks = GetTimeValueFromFrame(time);

   Matrix3 out = node->GetObjTMAfterWSM(ticks);

   if(this->bIsCameraTransform){
      // Cameras in Max are already pointing down the negative z-axis (as is expected from Alembic).
      // So we rotate it by 90 degrees so that it is pointing down the positive y-axis.
      Matrix3 rotation(TRUE);
      rotation.RotateX(-HALFPI);
      out = rotation * out;
   }

   // Convert the max transform to alembic
   Matrix3 alembicMatrix;
   ConvertMaxMatrixToAlembicMatrix(out, alembicMatrix);
   return Abc::M44d( 
      alembicMatrix.GetRow(0).x,  alembicMatrix.GetRow(0).y,  alembicMatrix.GetRow(0).z,  0,
      alembicMatrix.GetRow(1).x,  alembicMatrix.GetRow(1).y,  alembicMatrix.GetRow(1).z,  0,
      alembicMatrix.GetRow(2).x,  alembicMatrix.GetRow(2).y,  alembicMatrix.GetRow(2).z,  0,
      alembicMatrix.GetRow(3).x,  alembicMatrix.GetRow(3).y,  alembicMatrix.GetRow(3).z,  1);
}

bool SceneNodeMax::getVisibility(double time)
{ 
   if(bMergedSubtreeNodeParent) return true;

   TimeValue ticks = GetTimeValueFromFrame(time);
   float flVisibility = node->GetLocalVisibility(ticks);
   return flVisibility > 0;
}


MeshData SceneNodeMax::getMeshData(double time)
{
   MeshData meshData;

   TimeValue ticks = GetTimeValueFromFrame(time);
   meshData.obj = node->EvalWorldState(ticks).obj;

	if (meshData.obj->CanConvertToType(Class_ID(POLYOBJ_CLASS_ID, 0)))
	{
		meshData.polyObj = reinterpret_cast<PolyObject *>(meshData.obj->ConvertToType(ticks, Class_ID(POLYOBJ_CLASS_ID, 0)));
		meshData.polyMesh = &meshData.polyObj->GetMesh();
	}
	else if (meshData.obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
	{
		meshData.triObj = reinterpret_cast<TriObject *>(meshData.obj->ConvertToType(ticks, Class_ID(TRIOBJ_CLASS_ID, 0)));
		meshData.triMesh = &meshData.triObj->GetMesh();
	}

   return meshData;
}

Mtl* SceneNodeMax::getMtl()
{
   return node->GetMtl();
}

int SceneNodeMax::getMtlID()
{
   return -1;
}

SceneNode::nodeTypeE getNodeType(INode* node)
{
   //TODO: not sure if I need this...
	//ESS_CPP_EXCEPTION_REPORTING_START

   //TimeValue currentTime = pMaxInterface->GetTime();

   Object *obj = node->EvalWorldState(0).obj;
   SClass_ID superClassID = obj->SuperClassID();
   Class_ID classID = obj->ClassID();

#ifdef THINKING_PARTICLES
   if(obj->CanConvertToType(MATTERWAVES_CLASS_ID)){
     ESS_LOG_WARNING("We only support merged mesh export for thinking particles.");
     return SceneNode::PARTICLES_TP;
   }
#endif

   if (obj->IsParticleSystem()){
      return SceneNode::PARTICLES;
   }

	if (obj->IsShapeObject() == FALSE &&
        (obj->CanConvertToType(polyObjectClassID) || 
         obj->CanConvertToType(triObjectClassID))){
      return SceneNode::POLYMESH;
	}

	//if (node->IsTarget()) 
 //   {
	//	INode* ln = node->GetLookatNode();
	//	if (ln) 
 //       {
	//		Object *lobj = ln->EvalWorldState(time).obj;
	//		switch(lobj->SuperClassID()) 
 //           {
	//			//case LIGHT_CLASS_ID:  return SceneEntry(node, obj, OBTYPE_LTARGET, pFullname); break;
	//			case CAMERA_CLASS_ID: return SceneEntry(node, obj, OBTYPE_CTARGET, pFullname); break;
	//		}
	//	}
	//}

	switch (superClassID) 
    { 
		case HELPER_CLASS_ID:
			if (classID == Class_ID(DUMMY_CLASS_ID, 0))
            {
               return SceneNode::ITRANSFORM;
            }
			break;
		case LIGHT_CLASS_ID: 
            {
                /* LIGHT */
                break;
            }
		case CAMERA_CLASS_ID:
			if (classID == Class_ID(LOOKAT_CAM_CLASS_ID, 0) ||
                classID == Class_ID(SIMPLE_CAM_CLASS_ID, 0))
            {
               return SceneNode::CAMERA;
            }
			break;
        case SHAPE_CLASS_ID:
            if (obj->IsShapeObject() == TRUE)
            {
               return SceneNode::CURVES;
            }
            break;
	}

	//ESS_CPP_EXCEPTION_REPORTING_END

   //return SceneNode::ITRANSFORM;
   return SceneNode::UNKNOWN;
}


struct CSGStackElement
{
   INode* iNode;
   SceneNodePtr eNode;

   CSGStackElement(INode* node):iNode(node)
   {}
   CSGStackElement(INode* inode, SceneNodePtr enode):iNode(inode), eNode(enode)
   {}
};


SceneNodeMaxPtr createNodeMax(INode* pNode, SceneNode::nodeTypeE type, bool isCameraTransform, bcsgSelection::types selectionOption)
{
   SceneNodeMaxPtr sceneNode(new SceneNodeMax(pNode));
   
   sceneNode->name = EC_MCHAR_to_UTF8( pNode->GetName() );
   sceneNode->type = type;
   sceneNode->bIsCameraTransform = isCameraTransform;

   //TODO: not sure if I need to fill this in
   //sceneNode->dccIdentifier = xObj.GetFullName().GetAsciiString();

   if(selectionOption == bcsgSelection::ALL){
      sceneNode->selected = true;
   }
   else if(selectionOption == bcsgSelection::APP){
      sceneNode->selected = (pNode->Selected() == 1) ? true : false;
   }
   
   return sceneNode;
}


SceneNodeMaxPtr buildCommonSceneGraph(int& nNumNodes, bool bUnmergeNodes, bcsgSelection::types selectionOption)
{

   INode* pRootNode = GET_MAX_INTERFACE()->GetRootNode();

   ESS_PROFILE_FUNC();

   std::list<CSGStackElement> sceneStack;
   
   nNumNodes = 0;

   SceneNodeMaxPtr exoRoot = createNodeMax(pRootNode, SceneNode::SCENE_ROOT, false, selectionOption);

   
   for(int j=0; j<pRootNode->NumberOfChildren(); j++)
   {
      INode* pNode = pRootNode->GetChildNode(j);
      if(!pNode) continue;
      sceneStack.push_back(CSGStackElement(pNode, exoRoot));
   }


   while( !sceneStack.empty() )
   {

      CSGStackElement sElement = sceneStack.back();
      INode* pNode = sElement.iNode;
      SceneNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();
   
      nNumNodes++;

      SceneNodePtr newNode;

      SceneNode::nodeTypeE type = getNodeType(pNode);

      //if(bUnmergeNodes) { //export case (don't why we don't use it for import)
         if(isShapeNode(type) ){
            newNode = createNodeMax(pNode, SceneNode::ETRANSFORM, type == SceneNode::CAMERA, selectionOption);
            //newNode->name+="Xfo";
            SceneNodePtr geoNode = createNodeMax(pNode, type, false, selectionOption);
		      geoNode->name+="Shape";

            newNode->children.push_back(geoNode);
            geoNode->parent = newNode.get();
         } 
         else{
            newNode = createNodeMax(pNode, type, false, selectionOption);
         }
      //}
     // else{ //import 
     //    newNode = createNodeMax(xRef, type);//doesn't identify EXTRANFORMS because these are built-in to the shape node
     //    if(type == SceneNode::ITRANSFORM || type == SceneNode::NAMESPACE_TRANSFORM){
     //       //newNode->name+="Xfo";
     //    }
		   //else{
			  // //newNode->name+="Shape";
		   //}
     // }

      //if(bSelectAll){
      //   newNode->selected = true;
      //}

      eNode->children.push_back(newNode);
      newNode->parent = eNode.get();

      for(int j=0; j<pNode->NumberOfChildren(); j++)
      {
         INode* childNode = pNode->GetChildNode(j);
         if(!childNode) continue;
         sceneStack.push_back(CSGStackElement(childNode, newNode));
      }
   }

   return exoRoot;
}


