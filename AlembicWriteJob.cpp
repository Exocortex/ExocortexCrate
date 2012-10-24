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

#include <xsi_application.h>
#include <xsi_time.h>
#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_property.h>
#include <xsi_parameter.h>
#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_uitoolkit.h>
#include <xsi_geometry.h>
#include <xsi_iceattribute.h>
#include <xsi_model.h>
#include <xsi_utils.h>


using namespace XSI;
using namespace MATH;


AlembicWriteJob::AlembicWriteJob
(
   const CString & in_FileName,
   const std::vector<Selectee> & in_Selection,
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

AlembicObjectPtr AlembicWriteJob::GetObject(const XSI::CRef & in_Ref)
{
 	std::string key = std::string( in_Ref.GetAsText().GetAsciiString() );

	std::map<std::string,AlembicObjectPtr>::const_iterator it = mObjectsNames.find( key );

	if (it != mObjectsNames.end()) {
		return it->second;
	}
	return AlembicObjectPtr();
}

bool AlembicWriteJob::AddObjectIfDoesNotExist(AlembicObjectPtr in_Obj)
{
   if(!in_Obj)
      return false;
   if(!in_Obj->GetRef().IsValid())
      return false;
   AlembicObjectPtr existing = GetObject(in_Obj->GetRef());
   if(existing != NULL)
      return false;
   mObjects.push_back(in_Obj);
   mObjectsNames.insert( std::map<std::string,AlembicObjectPtr>::value_type( std::string( in_Obj->GetRef().GetAsText().GetAsciiString() ), in_Obj ) );
   return true;
}


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
            "Softimage Alembic Plugin",
            sceneFileName.GetAsciiString(),
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
      if(GetObject(xObj_GetActivePrimitive_GetRef) != NULL)
         continue;

      // push all models up the hierarchy
	  
	  // MHahn: we are going to disable this code for now, since it should probably be an additional 
	  // export option, and we would prefer not to make things more complicated. We we will require
	  // the user select the exact set of nodes that should be exported.

      //Model model(xObj.GetRef());
      //if(!model.IsValid())
      //   model = xObj.GetModel();
      //CRefArray modelRefs;
      //if((bool)GetOption(L"transformCache"))
      //{
      //   X3DObject parent = xObj.GetParent3DObject();
      //   while(parent.IsValid() && !parent.GetFullName().IsEqualNoCase( activeSceneRoot_GetFullName ))
      //   {
      //      modelRefs.Add(parent.GetActivePrimitive().GetRef());
      //      parent = parent.GetParent3DObject();
      //   }
      //}
      //else
      //{
      //   while(model.IsValid() && !model.GetFullName().IsEqualNoCase( activeSceneRoot_GetFullName ))
      //   {
      //      modelRefs.Add(model.GetActivePrimitive().GetRef());
      //      model = model.GetModel();
      //   }
      //}
      //for(LONG j=modelRefs.GetCount()-1;j>=0;j--)
      //{
      //   if(GetObject(modelRefs[j]) == NULL)
      //   {
      //      AlembicObjectPtr ptr;
      //      ptr.reset(new AlembicModel(modelRefs[j],this));
      //      AddObjectIfDoesNotExist(ptr);
      //   }
      //}

  
      if((bool)GetOption(L"transformCache"))
      {
         //always export all transforms whether the hierarchy is flattened or not

         AlembicObjectPtr ptr;
         ptr.reset(new AlembicModel(xObj_GetActivePrimitive_GetRef,this));
         AddObjectIfDoesNotExist(ptr);

         continue;// only do models (tranforms) if we perform pure transformcache
      }

      // take care of all other types
	  CString xObj_GetType = xObj.GetType();
      if(xObj_GetType.IsEqualNoCase(L"null") && ((bool)GetOption(L"flattenHierarchy") == false))
	  {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicModel(xObj_GetActivePrimitive_GetRef, this));
         AddObjectIfDoesNotExist(ptr);
	  }
      else if(xObj_GetType.IsEqualNoCase(L"camera"))
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicCamera(xObj_GetActivePrimitive_GetRef,this));
         AddObjectIfDoesNotExist(ptr);
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
            AddObjectIfDoesNotExist(ptr);
         }
         else
         {
            AlembicObjectPtr ptr;
            ptr.reset(new AlembicPolyMesh(xObj_GetActivePrimitive_GetRef,this));
            AddObjectIfDoesNotExist(ptr);
         }
      }
      else if(xObj_GetType.IsEqualNoCase(L"surfmsh"))
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicNurbs(xObj_GetActivePrimitive_GetRef,this));
         AddObjectIfDoesNotExist(ptr);
      }
      else if(xObj_GetType.IsEqualNoCase(L"crvlist"))
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicCurves(xObj_GetActivePrimitive_GetRef,this));
         AddObjectIfDoesNotExist(ptr);
      }
      else if(xObj_GetType.IsEqualNoCase(L"hair"))
      {
         AlembicObjectPtr ptr;
         ptr.reset(new AlembicCurves(xObj_GetActivePrimitive_GetRef,this));
         AddObjectIfDoesNotExist(ptr);
      }
      else if(xObj_GetType.IsEqualNoCase(L"pointcloud"))
      {
         AlembicObjectPtr ptr;
         ICEAttribute strandPosition = xObj.GetActivePrimitive().GetGeometry().GetICEAttributeFromName(L"StrandPosition");
         if(strandPosition.IsDefined() && strandPosition.IsValid())
            ptr.reset(new AlembicCurves(xObj_GetActivePrimitive_GetRef,this));
         else
            ptr.reset(new AlembicPoints(xObj_GetActivePrimitive_GetRef,this));
         AddObjectIfDoesNotExist(ptr);
      }
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