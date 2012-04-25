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
//#include "AlembicNurbs.h"

#include <maya/MAnimControl.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

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
   MString refString = getFullNameFromRef(in_Ref);
   for(size_t i=0;i<mObjects.size();i++)
   {
      if(getFullNameFromRef(mObjects[i]->GetRef()) == refString)
         return mObjects[i];
   }
   return AlembicObjectPtr();
}

bool AlembicWriteJob::AddObject(AlembicObjectPtr in_Obj)
{
   if(!in_Obj)
      return false;
   if(in_Obj->GetRef().isNull())
      return false;
   AlembicObjectPtr existing = GetObject(in_Obj->GetRef());
   if(existing != NULL)
      return false;
   mObjects.push_back(in_Obj);
   return true;
}


MStatus AlembicWriteJob::PreProcess()
{
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
            "Softimage Alembic Plugin",
            sceneFileName.asChar(),
            Alembic::Abc::ErrorHandler::kThrowPolicy);
   }
   catch(Alembic::Util::Exception& e)
   {
      MString exc(e.what());
      MGlobal::displayError("[ExocortexAlembic] Error writing to file '"+mFileName+"' ("+exc+"). Do you still have it opened?");
      return MStatus::kInvalidParameter;
   }

   // get the frame rate
   mFrameRate = MAnimControl::currentTime().value() / MAnimControl::currentTime().as(MTime::kSeconds);
   std::vector<Alembic::AbcCoreAbstract::chrono_t> frames;
   for(LONG i=0;i<mFrames.size();i++)
      frames.push_back(mFrames[i] / mFrameRate);

   // create the sampling
   double timePerSample = 1.0 / mFrameRate;
   if(frames.size() > 1)
   {
      if(!HasFullLicense())
      {
         if(frames.size() > 75)
         {
            frames.resize(75);
            EC_LOG_WARNING("[ExocortexAlembic] Demo Mode: Maximum exportable samplecount is 75!");
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

   Alembic::Abc::OBox3dProperty boxProp = Alembic::AbcGeom::CreateOArchiveBounds(mArchive,mTs);

   // create object for each
   for(unsigned int i=0;i<mSelection.length();i++)
   {
      MObject mObj = mSelection[i];
      if(GetObject(mObj) != NULL)
         continue;

      // take care of all other types
      MString mType = mObj.apiTypeStr();
      if(mType == "kTransform")
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicXform(mObj,this));
         AddObject(ptr);
      }
      else if(mType == "kCamera")
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicCamera(mObj,this));
         AddObject(ptr);
      }
      else if(mType == "kMesh")
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicPolyMesh(mObj,this));
         AddObject(ptr);
      }
      else if(mType == "kSubdiv")
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicSubD(mObj,this));
         AddObject(ptr);
      }
      else if(mType == "kNurbsCurve")
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicCurves(mObj,this));
         AddObject(ptr);
      }
      else if(mType == "kParticle")
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicPoints(mObj,this));
         AddObject(ptr);
      }
      else if(mType == "kPfxHair")
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicHair(mObj,this));
         AddObject(ptr);
      }
      else
      {
         MGlobal::displayInfo("unsupported type: "+mType);
      }
   }

   return MStatus::kSuccess;
}

MStatus AlembicWriteJob::Process(double frame)
{
   MStatus result = MStatus::kSuccess;

   for(size_t i=0;i<mFrames.size();i++)
   {
      // compare the frames
      if(abs(mFrames[i] - frame) > 0.001)
         continue;

      // run the export for all objects
      for(size_t j=0;j<mObjects.size();j++)
      {
         MStatus status = mObjects[j]->Save(mFrames[i]);
         if(status != MStatus::kSuccess)
            return status;
         result = status;
      }
   }

   return result;
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

MStatus AlembicExportCommand::doIt(const MArgList & args)
{
   MStatus status = MS::kFailure;

   MTime oldCurTime = MAnimControl::currentTime();
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
         else if(valuePair[0].toLowerCase() == "filename")
            filename = valuePair[1];
         else if(valuePair[0].toLowerCase() == "objects")
         {
            // try to find each object
            MStringArray objectStrings;
            valuePair[1].split(',',objectStrings);
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
                  objects.append(objRef);

                  // check all of the shapes below
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
         else
         {
            MGlobal::displayWarning("[ExocortexAlembic] Skipping invalid token: "+tokens[j]);
            continue;
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
         // TODO: show a file dialog
         //CComAPIHandler toolkit;
         //toolkit.CreateInstance("XSI.UIToolkit");
         //CComAPIHandler filebrowser(toolkit.GetProperty("FileBrowser"));
         //filebrowser.PutProperty("InitialDirectory",Application().GetActiveProject().GetPath());
         //filebrowser.PutProperty("Filter","Alembic Files(*.abc)|*.abc||");
         //CValue returnVal;
         //filebrowser.Call("ShowSave",returnVal);
         //filename = filebrowser.GetProperty("FilePathName").GetAsText();
         //if(filename.IsEmpty())
         //{
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

   // TODO: see if we can have a progress bar
   //ProgressBar prog;
   //prog = Application().GetUIToolkit().GetProgressBar();
   //prog.PutCaption(L"Exporting "+CString(jobCount)+L" object's frames...");
   //prog.PutMinimum(0);
   //prog.PutMaximum(jobCount);
   //prog.PutValue(0);
   //prog.PutCancelEnabled(true);
   //prog.PutVisible(true);
	
   // now, let's run through all frames, and process the jobs
   double frameRate = MAnimControl::currentTime().value() / MAnimControl::currentTime().as(MTime::kSeconds);
   for(double frame = minFrame; frame<=maxFrame; frame += maxSteps / maxSubsteps)
   {
      MAnimControl::setCurrentTime(MTime(frame/frameRate,MTime::kSeconds));

      bool canceled = false;
      for(size_t i=0;i<jobPtrs.size();i++)
      {
         MStatus status = jobPtrs[i]->Process(frame);
         if(status == MStatus::kSuccess)
         {
            // TODO: increment progress bar
            // prog.Increment((LONG)jobPtrs[i]->GetNbObjects());
         }
         else
         {
            MGlobal::displayError("[ExocortexAlembic] Job aborted :"+jobPtrs[i]->GetFileName());
            for(size_t k=0;k<jobPtrs.size();k++)
               delete(jobPtrs[k]);
            return status;
         }

         // TODO: if progressbar canceled, cancel export
         //if(prog.IsCancelPressed())
         //{
         //   canceled = true;
         //   break;
         //}
      }
      if(canceled)
         break;
   }

   // TODO: hide progressbar
   //prog.PutVisible(false);

   // delete all jobs
   for(size_t k=0;k<jobPtrs.size();k++)
      delete(jobPtrs[k]);

   // remove all known archives
   deleteAllArchives();

   return status;
}
