#include "AlembicLicensing.h"
#include "AlembicWriteJob.h"
#include "AlembicObject.h"
#include "AlembicXform.h"
#include "AlembicCamera.h"
#include "AlembicPolyMesh.h"
#include "AlembicSubD.h"
#include "AlembicPoints.h"
#include "AlembicCurves.h"
#include "AlembicHair.h"
#include "CommonLog.h"
//#include "AlembicNurbs.h"

#include <maya/MAnimControl.h>
#include <maya/MProgressWindow.h>
#include <locale>



AlembicWriteJob::AlembicWriteJob
(
   const MString & in_FileName,
   const MObjectArray & in_Selection,
   const MDoubleArray & in_Frames
)
{
   // ensure to clear the isRefAnimated cache
   clearIsRefAnimatedCache();

   // todo... do we need to resolve this path?
   // mFileName = CUtils::ResolveTokenString(in_FileName,XSI::CTime(),false);
   mFileName = in_FileName;
   for(unsigned int i=0;i<in_Selection.length();i++)
      mSelection.append(in_Selection[i]);

   for(unsigned int i=0;i<in_Frames.length();i++)
      mFrames.push_back(in_Frames[i]);
}

AlembicWriteJob::~AlembicWriteJob()
{
}

void AlembicWriteJob::SetOption(const MString & in_Name, const MString & in_Value)
{
  ESS_PROFILE_SCOPE("AlembicWriteJob::SetOption");
   std::map<std::string,std::string>::iterator it = mOptions.find(in_Name.asChar());
   if(it == mOptions.end())
      mOptions.insert(std::pair<std::string,std::string>(in_Name.asChar(),in_Value.asChar()));
   else
      it->second = in_Value.asChar();
}

bool AlembicWriteJob::HasOption(const MString & in_Name)
{
   std::map<std::string,std::string>::iterator it = mOptions.find(in_Name.asChar());
   return it != mOptions.end();
}

MString AlembicWriteJob::GetOption(const MString & in_Name)
{
   std::map<std::string,std::string>::iterator it = mOptions.find(in_Name.asChar());
   if(it != mOptions.end())
      return it->second.c_str();
   return MString();
}

AlembicObjectPtr AlembicWriteJob::GetObject(const MObject & in_Ref)
{
  ESS_PROFILE_SCOPE("AlembicWriteJob::GetObject");

    std::string fullName(getFullNameFromRef(in_Ref).asChar());
	std::map<std::string, AlembicObjectPtr>::const_iterator it = mapObjects.find(fullName);
	if (it == mapObjects.end())
     return AlembicObjectPtr();
   return it->second;
}

bool AlembicWriteJob::AddObject(AlembicObjectPtr in_Obj)
{
  ESS_PROFILE_SCOPE("AlembicWriteJob::AddObject");
   if(!in_Obj)
      return false;
   const MObject &in_Ref = in_Obj->GetRef();
   if(in_Ref.isNull())
      return false;
   AlembicObjectPtr existing = GetObject(in_Ref);
   if(existing != NULL)
      return false;
   std::string fullName(getFullNameFromRef(in_Ref).asChar());
	 mapObjects[fullName] = in_Obj;
   return true;
}

MStatus AlembicWriteJob::PreProcess()
{
  ESS_PROFILE_SCOPE("AlembicWriteJob::PreProcess");
   // check filenames
   if(mFileName.length() == 0)
   {
      MGlobal::displayError("[ExocortexAlembic] No filename specified.");
      return MStatus::kInvalidParameter;
   }

   // check objects
   if(mSelection.length() == 0)
   {
      MGlobal::displayError("[ExocortexAlembic] No objects specified.");
      return MStatus::kInvalidParameter;
   }

   // check frames
   if(mFrames.size() == 0)
   {
      MGlobal::displayError("[ExocortexAlembic] No frames specified.");
      return MStatus::kInvalidParameter;
   }

   // check if the file is currently in use
   if(getRefArchive(mFileName) > 0)
   {
      MGlobal::displayError("[ExocortexAlembic] Error writing to file '"+mFileName+"'. File currently in use.");
      return MStatus::kInvalidParameter;
   }

   // init archive (use a locally scoped archive)
   // TODO: determine how to access the current maya scene path
   //MString sceneFileName = "Exported from: "+Application().GetActiveProject().GetActiveScene().GetParameterValue("FileName").GetAsText();
   MString sceneFileName = "Exported from Maya.";
   try
   {
      mArchive = CreateArchiveWithInfo(
            Alembic::AbcCoreHDF5::WriteArchive(),
            mFileName.asChar(),
            "Maya Alembic Plugin",
            sceneFileName.asChar(),
            Abc::ErrorHandler::kThrowPolicy);
   }
   catch(AbcU::Exception& e)
   {
      MString exc(e.what());
      MGlobal::displayError("[ExocortexAlembic] Error writing to file '"+mFileName+"' ("+exc+"). Do you still have it opened?");
      return MStatus::kInvalidParameter;
   }

   // get the frame rate
   mFrameRate = MAnimControl::currentTime().value() / MAnimControl::currentTime().as(MTime::kSeconds);
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
            EC_LOG_WARNING("[ExocortexAlembic] Demo Mode: Maximum exportable samplecount is 75!");
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
   Abc::OBox3dProperty boxProp = AbcG::CreateOArchiveBounds(mArchive,mTs);

   // create object for each
   MProgressWindow::reserve();
   MProgressWindow::setTitle("Alembic Export: Listing objects");
   MProgressWindow::setInterruptable(true);
   MProgressWindow::setProgressRange(0, mSelection.length());
   MProgressWindow::setProgress(0);

   MProgressWindow::startProgress();
   int interrupt = 20;
   bool processStopped = false;
   for(unsigned int i=0;i<mSelection.length(); ++i, --interrupt)
   {
      if (interrupt == 0)
      {
        interrupt = 20;
        if (MProgressWindow::isCancelled())
        {
          processStopped = true;
          break;
        }
      }
      MProgressWindow::advanceProgress(1);
      MObject mObj = mSelection[i];
      if(GetObject(mObj) != NULL)
         continue;

      // take care of all other types
      AlembicObjectPtr ptr;
      switch(mObj.apiType())
      {
      case MFn::kCamera:
        ptr.reset(new AlembicCamera(mObj,this));
        break;
      case MFn::kMesh:
        ptr.reset(new AlembicPolyMesh(mObj,this));
        break;
      case MFn::kSubdiv:
        ptr.reset(new AlembicSubD(mObj,this));
        break;
      case MFn::kNurbsCurve:
        ptr.reset(new AlembicCurves(mObj,this));
        break;
      case MFn::kParticle:
        ptr.reset(new AlembicPoints(mObj,this));
        break;
      case MFn::kPfxHair:
        ptr.reset(new AlembicHair(mObj,this));
        break;
      default:  // NO BREAK... DON'T ADD ONE!!!!
        MGlobal::displayInfo("[ExocortexAlembic] Exporting "+MString(mObj.apiTypeStr())+" node as kTransform.\n");  // NO BREAK, it's normal!
      case MFn::kTransform:
        ptr.reset(new AlembicXform(mObj,this));
        break;
      }
      AddObject(ptr);
   }
   MProgressWindow::endProgress();
   return processStopped ? MStatus::kEndOfFile : MStatus::kSuccess;
}

MStatus AlembicWriteJob::Process(double frame)
{
   ESS_PROFILE_SCOPE("AlembicWriteJob::Process");
   MProgressWindow::reserve();
   MProgressWindow::setTitle("Alembic Export: <each frame/each object>");
   MProgressWindow::setInterruptable(true);
   MProgressWindow::setProgressRange(0, mFrames.size() * mapObjects.size());
   MProgressWindow::setProgress(0);

   MProgressWindow::startProgress();
   for(size_t i=0; i<mFrames.size(); ++i)
   {
      // compare the frames
      if(abs(mFrames[i] - frame) > 0.001)
         continue;

      // run the export for all objects
      int interrupt = 20;
	    for (std::map<std::string, AlembicObjectPtr>::iterator it = mapObjects.begin(); it != mapObjects.end(); ++it, --interrupt)
	    {
         if (interrupt == 0)
         {
           interrupt = 20;
           if (MProgressWindow::isCancelled())
             break;
         }
         MProgressWindow::advanceProgress(1);
         MStatus status = it->second->Save(mFrames[i]);
         if(status != MStatus::kSuccess)
            return status;
      }
   }
   MProgressWindow::endProgress();
   return MStatus::kSuccess;
}

AlembicExportCommand::AlembicExportCommand()
{
}

AlembicExportCommand::~AlembicExportCommand()
{
}

MSyntax AlembicExportCommand::createSyntax()
{
   MSyntax syntax;
   syntax.addFlag("-h", "-help");
   syntax.addFlag("-j", "-jobArg", MSyntax::kString);
   syntax.makeFlagMultiUse("-j");
   syntax.enableQuery(false);
   syntax.enableEdit(false);

   return syntax;
}


void* AlembicExportCommand::creator()
{
   return new AlembicExportCommand();
}

static void restoreOldTime(MTime &start, MTime &end, MTime &curr, MTime &minT, MTime &maxT)
{
  const double st = start.as(start.unit()),
               en = end.as(end.unit()),
               cu = curr.as(curr.unit());

  MString inst = "playbackOptions -min ";
  inst += st;
  inst += " -max ";
  inst += en;
  inst += ";\ncurrentTime ";
  inst += cu;
  inst += ";";

  MGlobal::executeCommand(inst);
}

MStatus AlembicExportCommand::doIt(const MArgList & args)
{
   ESS_PROFILE_SCOPE("AlembicExportCommand::doIt");

   MStatus status = MS::kFailure;

   MTime currentAnimStartTime = MAnimControl::animationStartTime(),
         currentAnimEndTime   = MAnimControl::animationEndTime(),
         oldCurTime           = MAnimControl::currentTime(),
         curMinTime           = MAnimControl::minTime(),
         curMaxTime           = MAnimControl::maxTime();
   MArgParser argData(syntax(), args, &status);

   if (argData.isFlagSet("help"))
   {
      // TODO: implement help for this command
      //MGlobal::displayInfo(util::getHelpText());
      return MS::kSuccess;
   }

   unsigned int jobCount = argData.numberOfFlagUses("jobArg");
   MStringArray jobStrings;
   if (jobCount == 0)
   {
      // TODO: display dialog
      MGlobal::displayError("[ExocortexAlembic] No jobs specified.");
      return status;
   }
   else
   {
      // get all of the jobstrings
      for(unsigned int i=0;i<jobCount;i++)
      {
         MArgList jobArgList;
         argData.getFlagArgumentList("jobArg", i, jobArgList);
         jobStrings.append(jobArgList.asString(0));
      }
   }

   // create a vector to store the jobs
   std::vector<AlembicWriteJob*> jobPtrs;
   double minFrame = 1000000.0;
   double maxFrame = -1000000.0;
   double maxSteps = 1;
   double maxSubsteps = 1;

   // for each job, check the arguments
   for(unsigned int i=0;i<jobStrings.length();i++)
   {
      double frameIn = 1.0;
      double frameOut = 1.0;
      double frameSteps = 1.0;
      double frameSubSteps = 1.0;
      MString filename;
      bool purepointcache = false;
      bool normals = true;
      bool uvs = true;
      bool facesets = true;
	    bool bindpose = true;
      bool dynamictopology = false;
      bool globalspace = false;
      bool withouthierarchy = false;
      bool transformcache = false;
      bool useInitShadGrp = false;
      MStringArray objectStrings;
      MObjectArray objects;

      // process all tokens of the job
      MStringArray tokens;
      jobStrings[i].split(';',tokens);
      for(unsigned int j=0;j<tokens.length();j++)
      {
         MStringArray valuePair;
         tokens[j].split('=',valuePair);
         if(valuePair.length()!=2)
         {
            MGlobal::displayWarning("[ExocortexAlembic] Skipping invalid token: "+tokens[j]);
            continue;
         }

         if(valuePair[0].toLowerCase() == "in")
            frameIn = valuePair[1].asDouble();
         else if(valuePair[0].toLowerCase() == "out")
            frameOut = valuePair[1].asDouble();
         else if(valuePair[0].toLowerCase() == "step")
            frameSteps = valuePair[1].asDouble();
         else if(valuePair[0].toLowerCase() == "substep")
            frameSubSteps = valuePair[1].asDouble();
         else if(valuePair[0].toLowerCase() == "normals")
            normals = valuePair[1].asInt() != 0;
         else if(valuePair[0].toLowerCase() == "uvs")
            uvs = valuePair[1].asInt() != 0;
         else if(valuePair[0].toLowerCase() == "facesets")
            facesets = valuePair[1].asInt() != 0;
		     else if(valuePair[0].toLowerCase() == "bindpose")
            bindpose = valuePair[1].asInt() != 0;
		     else if(valuePair[0].toLowerCase() == "purepointcache")
            purepointcache = valuePair[1].asInt() != 0;
		     else if(valuePair[0].toLowerCase() == "dynamictopology")
            dynamictopology = valuePair[1].asInt() != 0;
		     else if(valuePair[0].toLowerCase() == "globalspace")
            globalspace = valuePair[1].asInt() != 0;
		     else if(valuePair[0].toLowerCase() == "withouthierarchy")
            withouthierarchy = valuePair[1].asInt() != 0;
		     else if(valuePair[0].toLowerCase() == "transformcache")
            transformcache = valuePair[1].asInt() != 0;
         else if(valuePair[0].toLowerCase() == "filename")
            filename = valuePair[1];
         else if(valuePair[0].toLowerCase() == "objects")
         {
            // try to find each object
            valuePair[1].split(',',objectStrings);
         }
         else if(valuePair[0].toLowerCase() == "useinitshadgrp")
            useInitShadGrp = valuePair[1].asInt() != 0;
         else
         {
            MGlobal::displayWarning("[ExocortexAlembic] Skipping invalid token: "+tokens[j]);
            continue;
         }
      }

      // now check the object strings
      for(unsigned int k=0;k<objectStrings.length();k++)
      {
         MSelectionList sl;
         MString objectString = objectStrings[k];
         sl.add(objectString);
         MDagPath dag;
         for(unsigned int l=0;l<sl.length();l++)
         {
            sl.getDagPath(l,dag);
            MObject objRef = dag.node();
            if(objRef.isNull())
            {
               MGlobal::displayWarning("[ExocortexAlembic] Skipping object '"+objectStrings[k]+"', not found.");
               break;
            }

            // get all parents
            MObjectArray parents;
            MString typeStr = dag.node().apiTypeStr();

            // check if this is a camera
            bool isCamera = false;
            for(unsigned int m=0;m<dag.childCount();m++)
            {
               MFnDagNode child(dag.child(m));
               MString cameraTypeStr = child.object().apiTypeStr();
               if(cameraTypeStr == "kCamera")
               {
                  isCamera = true;
                  break;
               }
            }

            if(typeStr == "kTransform" && !isCamera && !globalspace && !withouthierarchy)
            {
               MDagPath ppath = dag;
               while(!ppath.node().isNull() && ppath.length() > 0 && ppath.isValid())
               {
                  parents.append(ppath.node());
                  MStatus status = ppath.pop();
                  if(status != MStatus::kSuccess)
                     break;
               }
            }
            else
            {
               parents.append(dag.node());
            }

            // push all parents in
            while(parents.length() > 0)
            {
               bool found = false;
               for(unsigned int m=0;m<objects.length();m++)
               {
                  if(objects[m] == parents[parents.length()-1])
                  {
                     found = true;
                     break;
                  }
               }
               if(!found)
                  objects.append(parents[parents.length()-1]);
               parents.remove(parents.length()-1);
            }

            // check all of the shapes below
            if(!transformcache)
            {
               sl.getDagPath(l,dag);
               for(unsigned int m=0;m<dag.childCount();m++)
               {
                  MFnDagNode child(dag.child(m));
                  if(child.isIntermediateObject())
                     continue;
                  objects.append(child.object());
               }
            }
         }
      }

      // check if we have incompatible subframes
      if(maxSubsteps > 1.0 && frameSubSteps > 1.0)
      {
         if(maxSubsteps > frameSubSteps)
         {
            double part = maxSubsteps / frameSubSteps;
            if(abs(part - floor(part)) > 0.001)
            {
               MString frameSubStepsStr,maxSubstepsStr;
               frameSubStepsStr.set(frameSubSteps);
               maxSubstepsStr.set(maxSubsteps);
               MGlobal::displayError("[ExocortexAlembic] You cannot combine substeps "+frameSubStepsStr+" and "+maxSubstepsStr+" in one export. Aborting.");
               return MStatus::kInvalidParameter;
            }
         }
         else if(frameSubSteps > maxSubsteps )
         {
            double part = frameSubSteps / maxSubsteps;
            if(abs(part - floor(part)) > 0.001)
            {
               MString frameSubStepsStr,maxSubstepsStr;
               frameSubStepsStr.set(frameSubSteps);
               maxSubstepsStr.set(maxSubsteps);
               MGlobal::displayError("[ExocortexAlembic] You cannot combine substeps "+frameSubStepsStr+" and "+maxSubstepsStr+" in one export. Aborting.");
               return MStatus::kInvalidParameter;
            }
         }
      }

      // remember the min and max values for the frames
      if(frameIn < minFrame) minFrame = frameIn;
      if(frameOut > maxFrame) maxFrame = frameOut;
      if(frameSteps > maxSteps) maxSteps = frameSteps;
      if(frameSteps > 1.0) frameSubSteps = 1.0;
      if(frameSubSteps > maxSubsteps) maxSubsteps = frameSubSteps;

      // check if we have a filename
      if(filename.length() == 0)
      {
         MGlobal::displayError("[ExocortexAlembic] No filename specified.");
         for(size_t k=0;k<jobPtrs.size();k++)
            delete(jobPtrs[k]);
         return MStatus::kFailure;
      }

      // construct the frames
      MDoubleArray frames;
      for(double frame=frameIn; frame<=frameOut; frame+=frameSteps / frameSubSteps)
         frames.append(frame);

      AlembicWriteJob * job = new AlembicWriteJob(filename,objects,frames);
      job->SetOption("exportNormals",normals ? "1" : "0");
      job->SetOption("exportUVs",uvs ? "1" : "0");
      job->SetOption("exportFaceSets",facesets ? "1" : "0");
      job->SetOption("exportInitShadGrp",useInitShadGrp ? "1" : "0");
	    job->SetOption("exportBindPose",bindpose ? "1" : "0");
      job->SetOption("exportPurePointCache",purepointcache ? "1" : "0");
      job->SetOption("exportDynamicTopology",dynamictopology ? "1" : "0");
      job->SetOption("indexedNormals","1");
      job->SetOption("indexedUVs","1");
      job->SetOption("exportInGlobalSpace",globalspace ? "1" : "0");

      // check if the job is satifsied
      if(job->PreProcess() != MStatus::kSuccess)
      {
         MGlobal::displayError("[ExocortexAlembic] Job skipped. Not satisfied.");
         delete(job);
         continue;
      }

      // push the job to our registry
      MGlobal::displayInfo("[ExocortexAlembic] Using WriteJob:"+jobStrings[i]);
      jobPtrs.push_back(job);
   }

   // compute the job count
   unsigned int jobFrameCount = 0;
   for(size_t i=0;i<jobPtrs.size();i++)
      jobFrameCount += (unsigned int)jobPtrs[i]->GetNbObjects() * (unsigned int)jobPtrs[i]->GetFrames().size();

   // now, let's run through all frames, and process the jobs
   const double frameRate = MAnimControl::currentTime().value() / MAnimControl::currentTime().as(MTime::kSeconds);
   const double incrSteps = maxSteps / maxSubsteps;
   double prevFrame = minFrame - incrSteps;
   for(double frame = minFrame; frame<=maxFrame; frame += incrSteps, prevFrame += incrSteps)
   {
      MAnimControl::setCurrentTime(MTime(prevFrame/frameRate,MTime::kSeconds));
      MAnimControl::setAnimationEndTime( MTime(frame/frameRate,MTime::kSeconds) );
      MAnimControl::playForward();  // this way, it forces Maya to play exactly one frame! and particles are updated!

      bool canceled = false;
      for(size_t i=0;i<jobPtrs.size();i++)
      {
         MStatus status = jobPtrs[i]->Process(frame);
         if(status != MStatus::kSuccess)
         {
            MGlobal::displayError("[ExocortexAlembic] Job aborted :"+jobPtrs[i]->GetFileName());
            for(size_t k=0;k<jobPtrs.size();k++)
               delete(jobPtrs[k]);
            restoreOldTime(currentAnimStartTime, currentAnimEndTime, oldCurTime, curMinTime, curMaxTime);
            return status;
         }

      }
      if(canceled)
         break;
   }

   // restore the animation start/end time and the current time!
   restoreOldTime(currentAnimStartTime, currentAnimEndTime, oldCurTime, curMinTime, curMaxTime);

   // delete all jobs
   for(size_t k=0;k<jobPtrs.size();k++)
      delete(jobPtrs[k]);

   // remove all known archives
   deleteAllArchives();

   return status;
}
