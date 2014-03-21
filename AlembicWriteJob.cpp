#include "stdafx.h"
#include "Alembic.h"

#include "AlembicWriteJob.h"
#include "ObjectList.h"

#include "SceneEnumProc.h"
#include "AlembicPolyMsh.h"
#include "AlembicXForm.h"
#include "AlembicCamera.h"
#include "AlembicPoints.h"
#include "AlembicCurves.h"
#include "CommonUtilities.h"
#include "CommonSubtreeMerge.h"


AlembicWriteJob::AlembicWriteJob(const std::string &in_FileName, std::map<std::string, bool>& objectsMap, const std::vector<double> &in_Frames, Interface *i)
{
    mApplication = i;
	mMeshErrors = 0;
    mFileName = in_FileName;
    mObjectsMap = objectsMap;

    for(int i=0; i < in_Frames.size(); i++)
    {
        mFrames.push_back(in_Frames[i]);
    }
}

AlembicWriteJob::~AlembicWriteJob()
{
}

void AlembicWriteJob::SetOption(const std::string & in_Name, const bool & in_Value)
{
    std::map<std::string,bool>::iterator it = mOptions.find(in_Name);
    if(it == mOptions.end())
    {
        mOptions.insert(std::pair<std::string,bool>(in_Name,in_Value));
    }
    else
    {
        it->second = in_Value;
    }
}

bool AlembicWriteJob::HasOption(const std::string & in_Name)
{
    std::map<std::string,bool>::iterator it = mOptions.find(in_Name);
    return it != mOptions.end();
}

bool AlembicWriteJob::GetOption(const std::string & in_Name)
{
    std::map<std::string,bool>::iterator it = mOptions.find(in_Name);
    if(it != mOptions.end())
    {
        return it->second;
    }
    return false;
}

struct PreProcessStackElement
{
   SceneNodePtr eNode;
   Abc::OObject oParent;

   PreProcessStackElement(SceneNodePtr enode, Abc::OObject parent):eNode(enode), oParent(parent)
   {}
};

bool AlembicWriteJob::PreProcess()
{
    // check filenames
    if(mFileName.empty())
    {
        ESS_LOG_WARNING("[alembic] No filename specified.");
        return false;
    }

    //// check objects
    //if(mSelection.Count() == 0)
    //{
    //    ESS_LOG_WARNING("[alembic] No objects specified.");
    //    return false;
    //}

    // check frames
    if(mFrames.size() == 0)
    {
        ESS_LOG_WARNING("[alembic] No frames specified.");
        return false;
    }

    const bool bUseOgawa = (bool)GetOption("useOgawa");

    // init archive (use a locally scoped archive)
    std::string sceneFileName = "";
    sceneFileName.append( EC_MSTR_to_UTF8( mApplication->GetCurFilePath() ) );
    try
    {
       if(bUseOgawa){
        mArchive = CreateArchiveWithInfo(
            Alembic::AbcCoreOgawa::WriteArchive(),
			mFileName.c_str(),
			getExporterName( "3DS Max " EC_QUOTE( crate_Max_Version ) ).c_str(),
			getExporterFileName( sceneFileName ).c_str(),
			Abc::ErrorHandler::kThrowPolicy);
       }
       else{
        mArchive = CreateArchiveWithInfo(
			Alembic::AbcCoreHDF5::WriteArchive( true ),
			mFileName.c_str(),
			getExporterName( "3DS Max " EC_QUOTE( crate_Max_Version ) ).c_str(),
			getExporterFileName( sceneFileName ).c_str(),
			Abc::ErrorHandler::kThrowPolicy);
       }

    }
    catch(Alembic::Util::Exception& e)
    {
        std::string exc(e.what());
		    ESS_LOG_ERROR("[alembic] Error writing to file: "<<e.what());
        return false;
    }

    // get the frame rate
    mFrameRate = static_cast<float>(GetFrameRate());
    if(mFrameRate == 0.0f)
    {
        mFrameRate = 25.0f;
    }

    std::vector<AbcA::chrono_t> frames;
    for(LONG i=0;i<mFrames.size();i++)
    {
        frames.push_back(mFrames[i] / mFrameRate);
    }

    // create the sampling
    double timePerSample = 1.0 / mFrameRate;
    if(frames.size() > 1)
    {
		 if( ! HasAlembicWriterLicense() )
		 {
       if( HasAlembicInvalidLicense() ) {
          ESS_LOG_ERROR("[alembic] No license available and EXOCORTEX_ALEMBIC_NO_DEMO defined, aborting." );
          return false;
       }
			 if(frames.size() > 75)
			 {
				frames.resize(75);
				ESS_LOG_WARNING("[ExocortexAlembic] Writer license not found: Maximum exportable samplecount is 75!");
			 }
		 }
		 
	    double timePerCycle = frames[frames.size()-1] - frames[0];
        AbcA::TimeSamplingType samplingType((boost::uint32_t)frames.size(),timePerCycle);
        AbcA::TimeSampling sampling(samplingType,frames);
        mTs = mArchive.addTimeSampling(sampling);
    }
    else
    {
        AbcA::TimeSampling sampling(1.0,frames[0]);
        mTs = mArchive.addTimeSampling(sampling);
    }

    m_ArchiveBoxProp = AbcG::CreateOArchiveBounds(mArchive,mTs);

	

	const bool bParticleMesh = GetOption("exportParticlesAsMesh");
   bool bMergePolyMeshSubtree = GetOption("mergePolyMeshSubtree");

   bool bSelectParents = GetOption("includeParentNodes");/*|| !bFlattenHierarchy || bTransformCache*/
   const bool bSelectChildren = false;
   bool bTransformCache = GetOption("transformCache");
   const bool bFlattenHierarchy = GetOption("flattenHierarchy");

   if(bMergePolyMeshSubtree){
      bTransformCache = false;
      //bSelectParents = true;
   }

   bcsgSelection::types buildSelection = bcsgSelection::ALL;

   const bool bExportSelected = GetOption("exportSelected");
   const bool bObjectsParameterExists = GetOption("objectsParameterExists");
   if(bExportSelected){
      //copy max selection
      buildSelection = bcsgSelection::APP;
   }
   else if(bObjectsParameterExists){
      //select nothing when building, fill in later from parameter data
      buildSelection = bcsgSelection::NONE;
   }
   else{
      //select everything
   }

   int nNumNodes = 0;
   exoSceneRoot = buildCommonSceneGraph(nNumNodes, true, buildSelection);
   //WARNING ILM robot right crashes when printing
   //printSceneGraph(exoSceneRoot, false);


   if(bObjectsParameterExists){
      //Might be better to use refineSelection here, but call a function that sets up dccSelected flag first, then delete this function from codebase
      selectNodes(exoSceneRoot, mObjectsMap,  bSelectParents, bSelectChildren, !bTransformCache);

      bool bAllResolved = true;

      if(bObjectsParameterExists){
         for(SceneNode::SelectionT::iterator it = mObjectsMap.begin(); it != mObjectsMap.end(); it++){
            if(it->second == false){
               bAllResolved = false;
               ESS_LOG_ERROR("Could not resolve objects identifier: "<<it->first);
            }
         }
      }

      if(bAllResolved){
         removeUnselectedNodes(exoSceneRoot);
      }
      else{
         return false;
      }
   }
   else if(bExportSelected){
      refineSelection(exoSceneRoot, bSelectParents, bSelectChildren, !bTransformCache);
      removeUnselectedNodes(exoSceneRoot);
   }
   

   if(bMergePolyMeshSubtree){
      replacePolyMeshSubtree<SceneNodeMaxPtr, SceneNodeMax>(exoSceneRoot);
   }

   if(bFlattenHierarchy){
      nNumNodes = 0;
      flattenSceneGraph(exoSceneRoot, nNumNodes);
   }

   if(GetOption("renameConflictingNodes")){
      renameConflictingNodes(exoSceneRoot);
   }
   

   std::list<PreProcessStackElement> sceneStack;
   
   sceneStack.push_back(PreProcessStackElement(exoSceneRoot, mArchive.getTop()));

   try{

   while( !sceneStack.empty() )
   {

      PreProcessStackElement sElement = sceneStack.back();
      SceneNodePtr eNode = sElement.eNode;
      sceneStack.pop_back();
      
      Abc::OObject oParent = sElement.oParent;
      Abc::OObject oNewParent;

      AlembicObjectPtr pNewObject;

      if(eNode->type == SceneNode::SCENE_ROOT){
         //we do not want to export the Scene_Root (the alembic archive has one already)
      }
      else if(eNode->type == SceneNode::ITRANSFORM || eNode->type == SceneNode::ETRANSFORM){
         pNewObject.reset(new AlembicXForm(eNode, this, oParent));
      }
      else if(eNode->type == SceneNode::CAMERA){
         pNewObject.reset(new AlembicCamera(eNode, this, oParent));
      }
      else if(eNode->type == SceneNode::POLYMESH || eNode->type == SceneNode::POLYMESH_SUBTREE){
         pNewObject.reset(new AlembicPolyMesh(eNode, this, oParent));
      }
      //TODO: as far I recall we dont support SUBD. verify...
      //else if(eNode->type == SceneNode::SUBD){
      //   pNewObject.reset(new AlembicSubD(eNode, this, oParent));
      //}
      else if(eNode->type == SceneNode::CURVES){
         pNewObject.reset(new AlembicCurves(eNode, this, oParent));
      }
      else if(eNode->type == SceneNode::PARTICLES || eNode->type == SceneNode::PARTICLES_TP){
         if(bParticleMesh){
            pNewObject.reset(new AlembicPolyMesh(eNode, this, oParent));
         }
         else{
            pNewObject.reset(new AlembicPoints(eNode, this, oParent));
         }
      }
      //else{
      //   ESS_LOG_WARNING("Unknown type: not exporting "<<eNode->name);//Export as transform, and give warning?
      //}

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
         return false;
      }
   }

   }catch( std::exception& exp ){
      ESS_LOG_ERROR("An std::exception occured: "<<exp.what());
      return false;
   }catch(...){
      ESS_LOG_ERROR("Exception ecountered when exporting.");
   }

   if(mObjects.empty()){
      ESS_LOG_ERROR("No objects specified.");
      return false;
   }

   return true;
}

void AlembicWriteJob::AddObject(AlembicObjectPtr obj)
{
    mObjects.push_back(obj);
}

bool AlembicWriteJob::Process(double frame)
{
    bool result = false;

    for(size_t i=0;i<mFrames.size();i++)
    {
        // compare the frames
        if(abs(mFrames[i] - frame) > 0.001)
        {
            continue;
        }

        // run the export for all objects
        m_Archivebbox.makeEmpty();
        
        // run the export for all objects
        for(size_t j=0; j < mObjects.size(); j++)
        {
            bool status = mObjects[j]->Save(mFrames[i], i == (mFrames.size()-1) );
            if(status != true)
            {
                return status;
            }
            result = true;
        }

        // Set the archive bounds bounding box
        m_ArchiveBoxProp.set(m_Archivebbox);
    }

    return result;
}
