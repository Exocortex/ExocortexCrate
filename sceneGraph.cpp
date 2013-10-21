#include "stdafx.h"
#include "sceneGraph.h"

#include "CommonLog.h"
#include "CommonImport.h"
#include "AlembicImport.h"

using namespace XSI;


bool SceneNodeXSI::replaceData(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAlembicPtr& nextFileNode)
{
   ESS_PROFILE_FUNC();

   //if(!jobParams.attachToExisting){
   //   return false;
   //}
  
   

   SceneNodePtr returnNode;
   bool bSuccess = createNodes(this, fileNode, jobParams, returnNode, true);
   //nextFileNode = reinterpret<SceneNode, SceneNodeAlembic>(returnNode);
   SceneNodeAlembicPtr shapeFileNode = reinterpret<SceneNode, SceneNodeAlembic>(returnNode);

   //delete the merged shape from the tree. this is done so that we will not look at the merged node when attach to next node to attach to

   //for(SceneChildIterator it = shapeFileNode->children.begin(), SceneChildIterator endIt = shapeFileNode->children.end(); it != endIt; it++){
   //   SceneNodeAlembicPtr childFileNode = reinterpret<SceneNode, SceneNodeAlembic>(*it);
   //   
   //   if(!childFileNode->isSupported()) continue;

   //   fileNode->children.push_back(childFileNode);
   //   childFileNode->parent = fileNode.get();
   //}

   //fileNode->children.erase(shapeFileNode);

   nextFileNode = fileNode;

   return bSuccess;
}

bool SceneNodeXSI::addChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode)
{
   ESS_PROFILE_FUNC();

   //if(jobParams.attachToExisting){
   //   return false;
   //}

   SceneNodePtr returnNode;
   bool bSuccess = createNodes(this, fileNode, jobParams, returnNode, false);
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
      return SceneNode::NAMESPACE_TRANSFORM;
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
   CSGStackElement(XSI::CRef xnode, SceneNodePtr node):xNode(xnode), eNode(node)
   {}
};

SceneNodeXSIPtr createNodeXSI(CRef& ref, SceneNode::nodeTypeE type)
{
   X3DObject xObj(ref);
   
   SceneNodeXSIPtr sceneNode(new SceneNodeXSI(ref));
   
   sceneNode->name = xObj.GetName().GetAsciiString();
   sceneNode->type = type;
   sceneNode->dccIdentifier = xObj.GetFullName().GetAsciiString();
   
   return sceneNode;
}

SceneNodeXSIPtr buildCommonSceneGraph(XSI::CRef xsiRoot, int& nNumNodes, bool bUnmergeNodes)
{
   ESS_PROFILE_FUNC();

   std::list<CSGStackElement> sceneStack;
   
   nNumNodes = 0;

   SceneNodeXSIPtr exoRoot = createNodeXSI(xsiRoot, SceneNode::SCENE_ROOT);

   X3DObject xRoot(xsiRoot);
   CRefArray children = xRoot.GetChildren();
   for(LONG j=0; j<children.GetCount(); j++)
   {
      X3DObject child(children[j]);
      if(!child.IsValid()) continue;
      sceneStack.push_back(CSGStackElement(children[j], exoRoot));
   }


   while( !sceneStack.empty() )
   {

      CSGStackElement sElement = sceneStack.back();
      CRef xRef = sElement.xNode;
      X3DObject xNode(xRef);
      SceneNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();
   
      nNumNodes++;

      SceneNodePtr newNode;

      SceneNode::nodeTypeE type = getNodeType(xNode);

      if(bUnmergeNodes) { //export case (don't why we don't use it for import)
         if(hasExtractableTransform(type) ){
            newNode = createNodeXSI(xRef, SceneNode::ETRANSFORM);
            //newNode->name+="Xfo";
            SceneNodePtr geoNode = createNodeXSI(xRef, type);
		      geoNode->name+="Shape";

            newNode->children.push_back(geoNode);
            geoNode->parent = newNode.get();
         } 
         else{
            newNode = createNodeXSI(xRef, type);
         }
      }
      else{ //import 
         newNode = createNodeXSI(xRef, type);//doesn't identify EXTRANFORMS because these are built-in to the shape node
         if(type == SceneNode::ITRANSFORM || type == SceneNode::NAMESPACE_TRANSFORM){
            //newNode->name+="Xfo";
         }
		   else{
			   //newNode->name+="Shape";
		   }
      }

      eNode->children.push_back(newNode);
      newNode->parent = eNode.get();

      CRefArray children = xNode.GetChildren();
      for(LONG j=0;j<children.GetCount();j++)
      {
         X3DObject child(children[j]);
         if(!child.IsValid()) continue;

         sceneStack.push_back(CSGStackElement(children[j], newNode));
      }
   }

   return exoRoot;
}


XSIProgressBar::XSIProgressBar()
{
   prog = Application().GetUIToolkit().GetProgressBar();
}

void XSIProgressBar::init(int min, int max, int incr)
{
   prog.PutMinimum(0);
   prog.PutMaximum(max);
   prog.PutValue(0);
   prog.PutCancelEnabled(true);
   nProgress = 0;
}

void XSIProgressBar::start(void)
{
   prog.PutVisible(true);
}

void XSIProgressBar::stop(void)
{
   prog.PutVisible(false);
}

void XSIProgressBar::incr(int step)
{
   prog.Increment(step);
}

bool XSIProgressBar::isCancelled(void)
{
   return prog.IsCancelPressed();
}

void XSIProgressBar::setCaption(std::string& caption)
{
   prog.PutCaption(CString(caption.c_str()));

	if(!Application().IsInteractive()){
      std::stringstream progStr;
      progStr<<std::setw(3)<<(prog.GetValue() * 100 / prog.GetMaximum())<<" - "<<caption;
      Application().LogMessage( CString(progStr.str().c_str()) );
	}
}


XSI_XformTypes::xte getXformType(AbcG::IXform& obj)
{
   Abc::ICompoundProperty arbGeom = obj.getSchema().getArbGeomParams();

   if(!arbGeom.valid()){
      return XSI_XformTypes::UNKNOWN;
   }

   if ( arbGeom.getPropertyHeader( ".xsiNodeType" ) != NULL ){

      Abc::IUcharProperty types = Abc::IUcharProperty( arbGeom, ".xsiNodeType" );
      if(types.valid() && types.getNumSamples() != 0){
         return (XSI_XformTypes::xte)types.getValue(0);
      }
   }

   return XSI_XformTypes::UNKNOWN;

}

XSI::CRef findTimeControlDccIdentifier(SceneNodeAlembicPtr fileRoot, XSI::CRef importRoot, XSI_XformTypes::xte in_xte)
{
   XSI::CRef ref;

   SceneNodeAlembic* currNode = fileRoot.get();

   while( true )
   {
      XSI_XformTypes::xte xte = XSI_XformTypes::UNKNOWN;
      Abc::IObject iObj = currNode->getObject();
      if(AbcG::IXform::matches(iObj.getMetaData()))
      {
         AbcG::IXform xform(iObj, Abc::kWrapExisting);
         xte = getXformType(xform);
         if(xte == XSI_XformTypes::UNKNOWN && currNode->type == SceneNode::ITRANSFORM){
            xte = in_xte;
         }
      }
      
      if( currNode->parent == NULL ){
         ref = importRoot;
         break;
      }
      if (currNode->bIsDirectChild && xte == XSI_XformTypes::XMODEL){
         ref.Set(currNode->dccIdentifier.c_str());
         break;
      }

      currNode = (SceneNodeAlembic*) currNode->parent;
   }

   
   
   return ref;
}