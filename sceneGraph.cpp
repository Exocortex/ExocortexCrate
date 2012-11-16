#include "stdafx.h"
#include "sceneGraph.h"

#include "CommonLog.h"
#include "CommonImport.h"
#include "AlembicImport.h"

using namespace XSI;


bool SceneNodeXSI::replaceData(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAlembicPtr& nextFileNode)
{
   if(jobParams.attachToExisting){
      return false;
   }
   
   SceneNodePtr returnNode;
   bool bSuccess = createNodes(this, fileNode, jobParams, returnNode);
   nextFileNode = reinterpret<SceneNode, SceneNodeAlembic>(returnNode);
   return bSuccess;
}

bool SceneNodeXSI::addChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode)
{
   if(jobParams.attachToExisting){
      return false;
   }

   SceneNodePtr returnNode;
   bool bSuccess = createNodes(this, fileNode, jobParams, returnNode);
   newAppNode = reinterpret<SceneNode, SceneNodeApp>(returnNode);
   return bSuccess;
}

void SceneNodeXSI::print()
{
   ESS_LOG_WARNING("XSINodeObjectCRef: "<<nodeRef.GetAsText().GetAsciiString());
}


SceneNode::nodeTypeE getNodeType(X3DObject& xObj)
{
 
   //CString sceneRootName = Application().GetActiveSceneRoot().GetFullName();
   //if( xObj.GetFullName().IsEqualNoCase(sceneRootName) ){
   //   return SceneNode::SCENE_ROOT;
   //}

   CString xObj_GetType = xObj.GetType();
   if(xObj_GetType.IsEqualNoCase(L"#model")){
      return SceneNode::ITRANSFORM;
   }
   else if(xObj_GetType.IsEqualNoCase(L"null")){
      return SceneNode::ITRANSFORM;
   }
   else if(xObj_GetType.IsEqualNoCase(L"bone")){
      return SceneNode::ITRANSFORM;
   }
   else if(xObj_GetType.IsEqualNoCase(L"camera"))
   {
      return SceneNode::CAMERA;
   }
   else if(xObj_GetType.IsEqualNoCase(L"polymsh")){
      Property geomProp;
      xObj.GetPropertyFromName(L"geomapprox",geomProp);
      LONG subDivLevel = geomProp.GetParameterValue(L"gapproxmordrsl");
      if(subDivLevel > 0){
         return SceneNode::SUBD;
      }
      else{
         return SceneNode::POLYMESH;
      }
   }
   else if(xObj_GetType.IsEqualNoCase(L"surfmsh")){
      return SceneNode::SURFACE;
   }
   else if(xObj_GetType.IsEqualNoCase(L"crvlist"))
   {
      return SceneNode::CURVES;
   }
   else if(xObj_GetType.IsEqualNoCase(L"hair"))
   {
      return SceneNode::CURVES;
   }
   else if(xObj_GetType.IsEqualNoCase(L"pointcloud"))
   {
      ICEAttribute strandPosition = xObj.GetActivePrimitive().GetGeometry().GetICEAttributeFromName(L"StrandPosition");
      if(strandPosition.IsDefined() && strandPosition.IsValid()){
         return SceneNode::CURVES;
      }
      else{
         return SceneNode::PARTICLES;
      }
   }
   
   //ESS_LOG_WARNING("Unknown type of XSI node, name: "<<xObj.GetName().GetAsciiString()<<", type: "<<xObj.GetType().GetAsciiString());
   //
   //if(xObj_GetType.IsEqualNoCase(L"light")){
   //   
   //}
   //else if(xObj_GetType.IsEqualNoCase(L"sphere")){

   //}
   //else if(xObj_GetType.IsEqualNoCase(L"cube")){

   //}
   //else if(xObj_GetType.IsEqualNoCase(L"tetrahedron")){

   //}
   //else if(xObj_GetType.IsEqualNoCase(L"icosahedron")){

   //}
   //else if(xObj_GetType.IsEqualNoCase(L"CameraInterest")){

   //}
   //else if(xObj_GetType.IsEqualNoCase(L"eff")){

   //}

   return SceneNode::ITRANSFORM;
   //return SceneNode::UNKNOWN;
}

struct CSGStackElement
{
   XSI::CRef xNode;
   SceneNodePtr eNode;

   CSGStackElement(XSI::CRef xnode):xNode(xnode)
   {}
   CSGStackElement(XSI::CRef xnode, SceneNodePtr enode):xNode(xnode), eNode(enode)
   {}
};


SceneNodeXSIPtr buildCommonSceneGraph(XSI::CRef xsiRoot)
{
   X3DObject xsiRootObj(xsiRoot);

   std::list<CSGStackElement> sceneStack;
   
   SceneNodeXSIPtr exoRoot(new SceneNodeXSI(xsiRoot));
   exoRoot->name = xsiRootObj.GetName().GetAsciiString();
   exoRoot->type = SceneNode::SCENE_ROOT;
   exoRoot->dccIdentifier = xsiRootObj.GetFullName().GetAsciiString();

   sceneStack.push_back(CSGStackElement(xsiRoot, exoRoot));

   while( !sceneStack.empty() )
   {

      CSGStackElement sElement = sceneStack.back();
      X3DObject xNode(sElement.xNode);
      SceneNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();

      CRefArray children = xNode.GetChildren();

      for(LONG j=0;j<children.GetCount();j++)
      {
         X3DObject child(children[j]);
         if(!child.IsValid()) continue;

         SceneNode::nodeTypeE type = getNodeType(child);
         
         SceneNodePtr exoChild(new SceneNodeXSI(child.GetRef()));

         if(!hasExtractableTransform(type))
         {
            SceneNodePtr exoChild(new SceneNodeXSI(child.GetRef()));
            exoChild->parent = eNode.get();
            exoChild->name = child.GetName().GetAsciiString();
            exoChild->type = type;
            exoChild->dccIdentifier = child.GetUniqueName().GetAsciiString();
         }
         else{
            //XSI shape nodes should split into two nodes: a transform node, and a pure shape node
            SceneNodePtr geoChild(new SceneNodeXSI(child.GetRef()));
            geoChild->parent = exoChild.get();
            geoChild->name = child.GetName().GetAsciiString();
            geoChild->type = type;
            geoChild->dccIdentifier = child.GetFullName().GetAsciiString();

            
            exoChild->parent = eNode.get();
            exoChild->name = child.GetName().GetAsciiString();
            exoChild->name += "Xfo";
            exoChild->type = SceneNode::ETRANSFORM;
            exoChild->dccIdentifier = child.GetFullName().GetAsciiString();
            exoChild->children.push_back(geoChild);
         }

         eNode->children.push_back(exoChild);
         sceneStack.push_back(CSGStackElement(children[j], exoChild));
      }
   }

   return exoRoot;
}
