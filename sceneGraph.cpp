#include "sceneGraph.h"

#include <xsi_iceattribute.h>
#include <xsi_primitive.h>
#include <xsi_geometry.h>
#include <xsi_application.h>
#include <xsi_model.h>

#include "CommonLog.h"

using namespace XSI;

exoNode::nodeTypeE getNodeType(X3DObject& xObj)
{
 
   //CString sceneRootName = Application().GetActiveSceneRoot().GetFullName();
   //if( xObj.GetFullName().IsEqualNoCase(sceneRootName) ){
   //   return exoNode::SCENE_ROOT;
   //}

   //ESS_LOG_WARNING("name: "<<xObj.GetName().GetAsciiString());
   //ESS_LOG_WARNING("type: "<<xObj.GetType().GetAsciiString());

   CString xObj_GetType = xObj.GetType();
   if(xObj_GetType.IsEqualNoCase(L"null")){
      return  exoNode::ITRANSFORM;
   }
   else if(xObj_GetType.IsEqualNoCase(L"camera"))
   {
      return exoNode::CAMERA;
   }
   else if(xObj_GetType.IsEqualNoCase(L"polymsh")){
      Property geomProp;
      xObj.GetPropertyFromName(L"geomapprox",geomProp);
      LONG subDivLevel = geomProp.GetParameterValue(L"gapproxmordrsl");
      if(subDivLevel > 0){
         return exoNode::SUBD;
      }
      else{
         return exoNode::POLYMESH;
      }
   }
   else if(xObj_GetType.IsEqualNoCase(L"surfmsh")){
      return exoNode::SURFACE;
   }
   else if(xObj_GetType.IsEqualNoCase(L"crvlist"))
   {
      return exoNode::CURVES;
   }
   else if(xObj_GetType.IsEqualNoCase(L"hair"))
   {
      return exoNode::CURVES;
   }
   else if(xObj_GetType.IsEqualNoCase(L"pointcloud"))
   {
      ICEAttribute strandPosition = xObj.GetActivePrimitive().GetGeometry().GetICEAttributeFromName(L"StrandPosition");
      if(strandPosition.IsDefined() && strandPosition.IsValid()){
         return exoNode::CURVES;
      }
      else{
         return exoNode::PARTICLES;
      }
   }

   return exoNode::UNKNOWN;
}

bool hasExtractableTransform( exoNode::nodeTypeE type )
{
   return 
      type == exoNode::CAMERA ||//the camera seems to already have a transform node
      type == exoNode::POLYMESH ||
      type == exoNode::SUBD ||
      type == exoNode::SURFACE ||
      type == exoNode::CURVES ||
      type == exoNode::PARTICLES;
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
   
   exoNodePtr exoRoot(new exoNode());
   exoRoot->name = xsiRoot.GetName().GetAsciiString();
   exoRoot->type = exoNode::SCENE_ROOT;
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

         exoNodePtr exoChild(new exoNode());

         exoNode::nodeTypeE type = getNodeType(child);
         

         if(!hasExtractableTransform(type))
         {
            exoChild->parent = eNode;
            exoChild->name = child.GetName().GetAsciiString();
            exoChild->type = type;
            exoChild->dccIdentifier = child.GetUniqueName().GetAsciiString();
         }
         else{
            //XSI Geometry nodes should split into two nodes: a transform node, and a pure geometry node
            exoNodePtr geoChild(new exoNode());

            exoChild->parent = eNode;
            exoChild->name = child.GetName().GetAsciiString();
            exoChild->name += "Xfo";
            exoChild->type = exoNode::ETRANSFORM;
            exoChild->dccIdentifier = child.GetUniqueName().GetAsciiString();

            geoChild->parent = exoChild;
            geoChild->name = child.GetName().GetAsciiString();
            geoChild->type = type;
            geoChild->dccIdentifier = child.GetUniqueName().GetAsciiString();

            exoChild->children.push_back(geoChild);
         }
 
         eNode->children.push_back(exoChild);

         sceneStack.push_back(stackElement(children[j], exoChild));
      }

   }

   return exoRoot;
}