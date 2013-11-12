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
#include "CommonSubtreeMerge.h"

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
   if(!in_Obj->GetRef().IsValid() && in_Obj->mExoSceneNode->type != SceneNode::POLYMESH_SUBTREE)
      return false;
   mObjects.push_back(in_Obj);
   return true;
}


struct PreProcessStackElement
{
   SceneNodePtr eNode;
   Abc::OObject oParent;

   PreProcessStackElement(SceneNodePtr enode, Abc::OObject parent):eNode(enode), oParent(parent)
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

   bool bUseOgawa = (bool)GetOption(L"useOgawa");

   // init archive (use a locally scoped archive)
   CString sceneFileName = L"Exported from: "+Application().GetActiveProject().GetActiveScene().GetParameterValue(L"FileName").GetAsText();
   try
   {
      if(bUseOgawa){
         mArchive = CreateArchiveWithInfo(
            Alembic::AbcCoreOgawa::WriteArchive(),
               mFileName.GetAsciiString(),
               getExporterName( "Softimage " EC_QUOTE( crate_Softimage_Version ) ).c_str(),
			   getExporterFileName( sceneFileName.GetAsciiString() ).c_str(),
               Abc::ErrorHandler::kThrowPolicy);
      }
      else{
         mArchive = CreateArchiveWithInfo(
            Alembic::AbcCoreHDF5::WriteArchive( true ),
               mFileName.GetAsciiString(),
               getExporterName( "Softimage " EC_QUOTE( crate_Softimage_Version ) ).c_str(),
			   getExporterFileName( sceneFileName.GetAsciiString() ).c_str(),
               Abc::ErrorHandler::kThrowPolicy);
      }
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
       if( HasAlembicInvalidLicense() ) {
          ESS_LOG_ERROR("[alembic] No license available and EXOCORTEX_ALEMBIC_NO_DEMO defined, aborting." );
          return CStatus::Fail;
       }
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
   
   bool bMergePolyMeshSubtree = true;

   bool bSelectParents = (bool)GetOption(L"includeParentNodes");
   const bool bSelectChildren = false;
   bool bTransformCache = (bool)GetOption(L"transformCache");
   const bool bFlattenHierarchy = (bool)GetOption(L"flattenHierarchy");

   if(bMergePolyMeshSubtree){
      bTransformCache = false;
      //bSelectParents = true;
   }

   int nNumNodes = 0;
   exoSceneRoot = buildCommonSceneGraph(Application().GetActiveSceneRoot(), nNumNodes, true);

   std::map<std::string, bool> selectionMap;

   for(int i=0; i<mSelection.size(); i++){
      XSI::CRef nodeRef;
      nodeRef.Set(mSelection[i].c_str());
      XSI::X3DObject xObj(nodeRef);
      selectionMap[xObj.GetFullName().GetAsciiString()] = true;
   }
   
   selectNodes(exoSceneRoot, selectionMap, /*!bFlattenHierarchy || bTransformCache*/ bSelectParents, bSelectChildren, !bTransformCache);
   removeUnselectedNodes(exoSceneRoot);

   if(bMergePolyMeshSubtree){
      replacePolyMeshSubtree<SceneNodeXSIPtr, SceneNodeXSI>(exoSceneRoot);
   }

   if(bFlattenHierarchy){
      nNumNodes = 0;
      flattenSceneGraph(exoSceneRoot, nNumNodes);
   }


   //return CStatus::OK;

   std::list<PreProcessStackElement> sceneStack;
   
   sceneStack.push_back(PreProcessStackElement(exoSceneRoot, GetTop()));

   try{

   while( !sceneStack.empty() )
   {

      PreProcessStackElement sElement = sceneStack.back();
      SceneNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();
      
      Abc::OObject oParent = sElement.oParent;
      Abc::OObject oNewParent;

      AlembicObjectPtr pNewObject;


      //if(eNode->selected)
      {
         if(eNode->type == SceneNode::SCENE_ROOT){
            //we do not want to export the Scene_Root (the alembic archive has one already)
         }
         else if(eNode->type == SceneNode::ITRANSFORM || eNode->type == SceneNode::ETRANSFORM || eNode->type == SceneNode::NAMESPACE_TRANSFORM){
            pNewObject.reset(new AlembicModel(eNode, this, oParent));
         }
         else if(eNode->type == SceneNode::CAMERA){
            pNewObject.reset(new AlembicCamera(eNode, this, oParent));
         }
         else if(eNode->type == SceneNode::POLYMESH || eNode->type == SceneNode::POLYMESH_SUBTREE){
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
      else{ //this case should be unecessary
         //if we skip node A, we parent node A's children to the parent of A
         oNewParent = oParent;
      }

      if(oNewParent.valid()){
         for( std::list<SceneNodePtr>::iterator it = eNode->children.begin(); it != eNode->children.end(); it++){
            sceneStack.push_back(PreProcessStackElement(*it, oNewParent));
         }
      }
      else{
         ESS_LOG_ERROR("Do not have refernce to parent.");
         return CStatus::Fail;
      }
   }

   }catch( std::exception& exp ){
      ESS_LOG_ERROR("An std::exception occured: "<<exp.what());
      return CStatus::Fail;
   }catch(...){
      ESS_LOG_ERROR("Exception ecountered when exporting.");
   }

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
