#include "stdafx.h"
#include "sceneGraph.h"

#include "CommonLog.h"

using namespace XSI;

SceneNode::nodeTypeE getNodeType(X3DObject& xObj)
{
 
   //CString sceneRootName = Application().GetActiveSceneRoot().GetFullName();
   //if( xObj.GetFullName().IsEqualNoCase(sceneRootName) ){
   //   return SceneNode::SCENE_ROOT;
   //}

   //ESS_LOG_WARNING("name: "<<xObj.GetName().GetAsciiString());
   //ESS_LOG_WARNING("type: "<<xObj.GetType().GetAsciiString());

   CString xObj_GetType = xObj.GetType();
   if(xObj_GetType.IsEqualNoCase(L"#model")){
      return SceneNode::ITRANSFORM;
   }
   if(xObj_GetType.IsEqualNoCase(L"null")){
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

   return SceneNode::UNKNOWN;
}

bool hasExtractableTransform( SceneNode::nodeTypeE type )
{
   return 
      type == SceneNode::CAMERA ||
      type == SceneNode::POLYMESH ||
      type == SceneNode::SUBD ||
      type == SceneNode::SURFACE ||
      type == SceneNode::CURVES ||
      type == SceneNode::PARTICLES;
}

exoNodePtr buildCommonSceneGraph(XSI::X3DObject xsiRoot)
{
   struct stackElement
   {
      XSI::CRef xNode;
      exoNodePtr eNode;

      stackElement(XSI::CRef xnode):xNode(xnode)
      {}
      stackElement(XSI::CRef xnode, exoNodePtr enode):xNode(xnode), eNode(enode)
      {}
   };

   std::list<stackElement> sceneStack;
   
   exoNodePtr exoRoot(new SceneNode());
   exoRoot->name = xsiRoot.GetName().GetAsciiString();
   exoRoot->type = SceneNode::SCENE_ROOT;
   exoRoot->dccIdentifier = xsiRoot.GetFullName().GetAsciiString();

   sceneStack.push_back(stackElement(xsiRoot, exoRoot));

   while( !sceneStack.empty() )
   {

      stackElement sElement = sceneStack.back();
      X3DObject xNode(sElement.xNode);
      exoNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();

      CRefArray children = xNode.GetChildren();

      for(LONG j=0;j<children.GetCount();j++)
      {
         X3DObject child(children[j]);
         if(!child.IsValid()) continue;

         exoNodePtr exoChild(new SceneNode());

         SceneNode::nodeTypeE type = getNodeType(child);
         

         if(!hasExtractableTransform(type))
         {
            exoChild->parent = eNode;
            exoChild->name = child.GetName().GetAsciiString();
            exoChild->type = type;
            exoChild->dccIdentifier = child.GetUniqueName().GetAsciiString();
         }
         else{
            //XSI shape nodes should split into two nodes: a transform node, and a pure shape node
            exoNodePtr geoChild(new SceneNode());

            exoChild->parent = eNode;
            exoChild->name = child.GetName().GetAsciiString();
            exoChild->type = SceneNode::ETRANSFORM;
            exoChild->dccIdentifier = child.GetFullName().GetAsciiString();

            geoChild->parent = exoChild;
            geoChild->name = child.GetName().GetAsciiString();
            geoChild->type = type;
            geoChild->dccIdentifier = child.GetFullName().GetAsciiString();

            exoChild->children.push_back(geoChild);
         }
 
         eNode->children.push_back(exoChild);

         sceneStack.push_back(stackElement(children[j], exoChild));
      }

   }

   return exoRoot;
}