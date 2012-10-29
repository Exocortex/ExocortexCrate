#include "stdafx.h"
#include "AlembicLicensing.h"
#include "AlembicWriteJob.h"
#include "AlembicObject.h"
#include "AlembicXform.h"
#include "AlembicCamera.h"
#include "AlembicPolyMsh.h"
#include "AlembicCurves.h"
#include "AlembicPoints.h"
#include "AlembicModel.h"
#include "AlembicSubD.h"
#include "AlembicNurbs.h"
#include "Utility.h"
#include "sceneGraph.h"

using namespace XSI;
using namespace MATH;


AlembicWriteJob::AlembicWriteJob
(
   const CString & in_FileName,
   const std::vector<std::string>& in_Selection,
   const CDoubleArray & in_Frames
)
{
   // ensure to clear the isRefAnimated cache
   clearIsRefAnimatedCache();

   mFileName = CUtils::ResolveTokenString(in_FileName,XSI::CTime(),false);
   mSelection = in_Selection;

   for(LONG i=0;i<in_Frames.GetCount();i++)
      mFrames.push_back(in_Frames[i]);
}

AlembicWriteJob::~AlembicWriteJob()
{
}

void AlembicWriteJob::SetOption(const CString & in_Name, const CValue & in_Value)
{
   std::map<XSI::CString,XSI::CValue>::iterator it = mOptions.find(in_Name);
   if(it == mOptions.end())
      mOptions.insert(std::pair<XSI::CString,XSI::CValue>(in_Name,in_Value));
   else
      it->second = in_Value;
}

bool AlembicWriteJob::HasOption(const CString & in_Name)
{
   std::map<XSI::CString,XSI::CValue>::iterator it = mOptions.find(in_Name);
   return it != mOptions.end();
}

CValue AlembicWriteJob::GetOption(const CString & in_Name)
{
   std::map<XSI::CString,XSI::CValue>::iterator it = mOptions.find(in_Name);
   if(it != mOptions.end())
      return it->second;
   return CValue(false);
}

//AlembicObjectPtr AlembicWriteJob::GetObject(const XSI::CRef & in_Ref)
//{
// 	std::string key = std::string( in_Ref.GetAsText().GetAsciiString() );
//
//	std::map<std::string,AlembicObjectPtr>::const_iterator it = mObjectsNames.find( key );
//
//	if (it != mObjectsNames.end()) {
//		return it->second;
//	}
//	return AlembicObjectPtr();
//}

bool AlembicWriteJob::AddObject(AlembicObjectPtr in_Obj)
{
   if(!in_Obj)
      return false;
   if(!in_Obj->GetRef().IsValid())
      return false;
   mObjects.push_back(in_Obj);
   return true;
}


struct PreProcessStackElement
{
   exoNodePtr eNode;
   Abc::OObject oParent;

   PreProcessStackElement(exoNodePtr enode, Abc::OObject parent):eNode(enode), oParent(parent)
   {}
};

CStatus AlembicWriteJob::PreProcess()
{
   // check filenames
   if(mFileName.IsEmpty())
   {
      Application().LogMessage(L"[ExocortexAlembic] No filename specified.",siErrorMsg);
      return CStatus::InvalidArgument;
   }

   // check objects
   if(mSelection.size() == 0)
   {
      Application().LogMessage(L"[ExocortexAlembic] No objects specified.",siErrorMsg);
      return CStatus::InvalidArgument;
   }

   // check frames
   if(mFrames.size() == 0)
   {
      Application().LogMessage(L"[ExocortexAlembic] No frames specified.",siErrorMsg);
      return CStatus::InvalidArgument;
   }

   // check if the file is currently in use
   if(getRefArchive(mFileName) > 0)
   {
      Application().LogMessage(L"[ExocortexAlembic] Error writing to file '"+mFileName+L"'. File currently in use.",siErrorMsg);
      return CStatus::InvalidArgument;
   }

   // init archive (use a locally scoped archive)
   CString sceneFileName = L"Exported from: "+Application().GetActiveProject().GetActiveScene().GetParameterValue(L"FileName").GetAsText();
   try
   {
      mArchive = CreateArchiveWithInfo(
		  Alembic::AbcCoreHDF5::WriteArchive(),
            mFileName.GetAsciiString(),
            getExporterName( "Softimage " EC_QUOTE( crate_Softimage_Version ) ).c_str(),
			getExporterFileName( sceneFileName.GetAsciiString() ).c_str(),
            Abc::ErrorHandler::kThrowPolicy);
   }
   catch(Alembic::Util::Exception& e)
   {
      CString exc(e.what());
      Application().LogMessage(L"[ExocortexAlembic] Error writing to file '"+mFileName+L"' ("+exc+L"). Do you still have it opened?",siErrorMsg);
      return CStatus::InvalidArgument;
   }

   mTop = mArchive.getTop();

   // get the frame rate
   mFrameRate = 25.0;
   CValue returnVal;
   CValueArray args(1);
   args[0] = L"PlayControl.Rate";
   Application().ExecuteCommand(L"GetValue",args,returnVal);
   mFrameRate = returnVal;
   if(mFrameRate == 0.0)
      mFrameRate = 25.0;

   std::vector<AbcA::chrono_t> frames;
   for(LONG i=0;i<mFrames.size();i++)
      frames.push_back(mFrames[i] / mFrameRate);

   // create the sampling
   double timePerSample = 1.0 / mFrameRate;
   if(frames.size() > 1)
   {
      if( ! HasAlembicWriterLicense() )
      {
         if(frames.size() > 75)
         {
            frames.resize(75);
            EC_LOG_WARNING("[ExocortexAlembic] Writer license not found: Maximum exportable samplecount is 75!");
         }
      }

      double timePerCycle = frames[frames.size()-1] - frames[0];
	  AbcA::TimeSamplingType samplingType((Abc::uint32_t)frames.size(),timePerCycle);
      AbcA::TimeSampling sampling(samplingType,frames);
      mTs = mArchive.addTimeSampling(sampling);
   }
   else
   {
      AbcA::TimeSampling sampling(1.0,frames[0]);
      mTs = mArchive.addTimeSampling(sampling);
   }

   const bool bSelectParents = true;
   const bool bSelectChildren = false;
   const bool bTransformCache = (bool)GetOption(L"transformCache");
   const bool bFlattenHierarchy = (bool)GetOption(L"flattenHierarchy");

   //TODO: eventually this should be a replaced with an equivalent virtual method, and the exporter will be shared
   exoNodePtr exoSceneRoot = buildCommonSceneGraph(Application().GetActiveSceneRoot());
   
   std::map<std::string, bool> selectionMap;

   for(int i=0; i<mSelection.size(); i++){
      XSI::CRef nodeRef;
      nodeRef.Set(mSelection[i].c_str());
      XSI::X3DObject xObj(nodeRef);
      selectionMap[xObj.GetFullName().GetAsciiString()] = true;
   }
   
   selectNodes(exoSceneRoot, selectionMap, !bFlattenHierarchy || bTransformCache, bSelectChildren, !bTransformCache);

   //::printSceneGraph(exoSceneRoot);


   //return CStatus::OK;

   std::list<PreProcessStackElement> sceneStack;
   
   sceneStack.push_back(PreProcessStackElement(exoSceneRoot, GetTop()));

   while( !sceneStack.empty() )
   {

      PreProcessStackElement sElement = sceneStack.back();
      exoNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();
      
      Abc::OObject oParent = sElement.oParent;
      Abc::OObject oNewParent;

      AlembicObjectPtr pNewObject;

      if(eNode->selected)
      {
         if(eNode->type == SceneNode::SCENE_ROOT){
            //we do not want to export the Scene_Root (the alembic archive has one already)
         }
         else if(eNode->type == SceneNode::ITRANSFORM || eNode->type == SceneNode::ETRANSFORM){
            pNewObject.reset(new AlembicModel(eNode, this, oParent));
         }
         else if(eNode->type == SceneNode::CAMERA){
            pNewObject.reset(new AlembicCamera(eNode, this, oParent));
         }
         else if(eNode->type == SceneNode::POLYMESH){
            pNewObject.reset(new AlembicPolyMesh(eNode, this, oParent));
         }
         else if(eNode->type == SceneNode::SUBD){
            pNewObject.reset(new AlembicSubD(eNode, this, oParent));
         }
         else if(eNode->type == SceneNode::SURFACE){
            pNewObject.reset(new AlembicNurbs(eNode, this, oParent));
         }
         else if(eNode->type == SceneNode::CURVES){
            pNewObject.reset(new AlembicCurves(eNode, this, oParent));
         }
         else if(eNode->type == SceneNode::PARTICLES){
            pNewObject.reset(new AlembicPoints(eNode, this, oParent));
         }
         else{
            ESS_LOG_WARNING("Unknown type: not exporting "<<eNode->name);//Export as transform, and give warning?
         }
      }

      if(pNewObject){
         //add the AlembicObject to export list if it is not being skipped
         AddObject(pNewObject);
      }

      if(pNewObject){
         oNewParent = oParent.getChild(eNode->name);
      }
      else{
         //if we skip node A, we parent node A's children to the parent of A
         oNewParent = oParent;
      }

      if(oNewParent.valid()){
         for( std::list<exoNodePtr>::iterator it = eNode->children.begin(); it != eNode->children.end(); it++){

            if( !bFlattenHierarchy || (bFlattenHierarchy && eNode->type == SceneNode::ETRANSFORM && hasExtractableTransform((*it)->type)) ){
               //If flattening the hierarchy, we want to attach each external transform to its corresponding geometry node.
               //All internal transforms should be skipped. Geometry nodes will never have children (If and XSI geonode is parented
               //to another geonode, each will be parented to its extracted transform node, and one node will be parented to the 
               //transform of the other.
               sceneStack.push_back(PreProcessStackElement(*it, oNewParent));
            }
            else{
               //if we skip node A, we parent node A's children to the parent of A
               sceneStack.push_back(PreProcessStackElement(*it, oParent));
            }
         }
      }
      else{
         ESS_LOG_ERROR("Do not have refernce to parent.");
         return CStatus::Fail;
      }
   }



#if 0

   for(LONG i=0; i<mSelection.size(); i++)
   {
      mSelection[i].nNodeDepth = getNodeDepthFromRef(mSelection[i].cRef);
   }
   std::sort(mSelection.begin(), mSelection.end());

   Abc::OBox3dProperty boxProp =AbcG::CreateOArchiveBounds(mArchive,mTs);

   CString activeSceneRoot_GetFullName = Application().GetActiveSceneRoot().GetFullName();
   // create object for each
   for(LONG i=0;i<mSelection.size();i++)
   {
	   X3DObject xObj(mSelection[i].cRef);
	   CRef xObj_GetActivePrimitive_GetRef = xObj.GetActivePrimitive().GetRef();
      //if(GetObject(xObj_GetActivePrimitive_GetRef) != NULL) continue;

      // push all models up the hierarchy
	  
	  // MHahn: we are going to disable this code for now, since it should probably be an additional 
	  // export option, and we would prefer not to make things more complicated. We we will require
	  // the user select the exact set of nodes that should be exported.

      //Model model(xObj.GetRef());
      //if(!model.IsValid())
      //   model = xObj.GetModel();
      CRefArray modelRefs;
      //if((bool)GetOption(L"transformCache"))
      if((bool)GetOption(L"flattenHierarchy") == false)
      {
         X3DObject parent = xObj.GetParent3DObject();
         while(parent.IsValid() && !parent.GetFullName().IsEqualNoCase( activeSceneRoot_GetFullName ))
         {
            modelRefs.Add(parent.GetActivePrimitive().GetRef());
            parent = parent.GetParent3DObject();
         }
      }
      //else
      //{
      //   while(model.IsValid() && !model.GetFullName().IsEqualNoCase( activeSceneRoot_GetFullName ))
      //   {
      //      modelRefs.Add(model.GetActivePrimitive().GetRef());
      //      model = model.GetModel();
      //   }
      //}
      for(LONG j=modelRefs.GetCount()-1;j>=0;j--)
      {
         //if(GetObject(modelRefs[j]) == NULL)
         {
            AlembicObjectPtr ptr;
            ptr.reset(new AlembicModel(modelRefs[j],this));
            //AddObjectIfDoesNotExist(ptr);
         }
      }

  
      if((bool)GetOption(L"transformCache"))
      {
         //always export all transforms whether the hierarchy is flattened or not

         AlembicObjectPtr ptr;
         ptr.reset(new AlembicModel(xObj_GetActivePrimitive_GetRef,this));
         //AddObjectIfDoesNotExist(ptr);

         continue;// only do models (tranforms) if we perform pure transformcache
      }

      // take care of all other types
	  CString xObj_GetType = xObj.GetType();
      if(xObj_GetType.IsEqualNoCase(L"null") && ((bool)GetOption(L"flattenHierarchy") == false))
	  {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicModel(xObj_GetActivePrimitive_GetRef, this));
         //AddObjectIfDoesNotExist(ptr);
	  }
      else if(xObj_GetType.IsEqualNoCase(L"camera"))
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicCamera(xObj_GetActivePrimitive_GetRef,this));
         //AddObjectIfDoesNotExist(ptr);
      }
      else if(xObj_GetType.IsEqualNoCase(L"polymsh"))
      {
         Property geomProp;
         xObj.GetPropertyFromName(L"geomapprox",geomProp);
         LONG subDivLevel = geomProp.GetParameterValue(L"gapproxmordrsl");
         if(subDivLevel > 0 && GetOption(L"geomApproxSubD") )
         {
            AlembicObjectPtr ptr;
            ptr.reset(new AlembicSubD(xObj_GetActivePrimitive_GetRef,this));
            //AddObjectIfDoesNotExist(ptr);
         }
         else
         {
            AlembicObjectPtr ptr;
            ptr.reset(new AlembicPolyMesh(xObj_GetActivePrimitive_GetRef,this));
            //AddObjectIfDoesNotExist(ptr);
         }
      }
      else if(xObj_GetType.IsEqualNoCase(L"surfmsh"))
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicNurbs(xObj_GetActivePrimitive_GetRef,this));
         //AddObjectIfDoesNotExist(ptr);
      }
      else if(xObj_GetType.IsEqualNoCase(L"crvlist"))
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicCurves(xObj_GetActivePrimitive_GetRef,this));
         //AddObjectIfDoesNotExist(ptr);
      }
      else if(xObj_GetType.IsEqualNoCase(L"hair"))
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicCurves(xObj_GetActivePrimitive_GetRef,this));
         //AddObjectIfDoesNotExist(ptr);
      }
      else if(xObj_GetType.IsEqualNoCase(L"pointcloud"))
      {
         AlembicObjectPtr ptr;
         ICEAttribute strandPosition = xObj.GetActivePrimitive().GetGeometry().GetICEAttributeFromName(L"StrandPosition");
         if(strandPosition.IsDefined() && strandPosition.IsValid())
            ptr.reset(new AlembicCurves(xObj_GetActivePrimitive_GetRef,this));
         else
            ptr.reset(new AlembicPoints(xObj_GetActivePrimitive_GetRef,this));
         //AddObjectIfDoesNotExist(ptr);
      }
   }

#endif

   return CStatus::OK;
}

CStatus AlembicWriteJob::Process(double frame)
{
   CStatus result = CStatus::False;

   for(size_t i=0;i<mFrames.size();i++)
   {
      // compare the frames
      if(abs(mFrames[i] - frame) > 0.001)
         continue;

      // run the export for all objects
      for(size_t j=0;j<mObjects.size();j++)
      {
         CStatus status = mObjects[j]->Save(mFrames[i]);
         if(status != CStatus::OK)
            return status;
         result = CStatus::OK;
      }
   }

   return result;
}
