#include "Alembic.h"
#include "AlembicWriteJob.h"
#include <gup.h>
#include <maxapi.h>
#include "ObjectList.h"
#include "ObjectEntry.h"
#include "SceneEnumProc.h"
#include "AlembicPolyMsh.h"
#include "AlembicXForm.h"
#include "AlembicCamera.h"
#include "AlembicPoints.h"
#include "AlembicCurves.h"



namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

AlembicWriteJob::AlembicWriteJob(const std::string &in_FileName, const ObjectList &in_Selection, const std::vector<double> &in_Frames, Interface *i)
{
    mApplication = i;
    mFileName = in_FileName;
    mSelection = in_Selection;

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

bool AlembicWriteJob::PreProcess()
{
    // check filenames
    if(mFileName.empty())
    {
        ESS_LOG_WARNING("[alembic] No filename specified.");
        return false;
    }

    // check objects
    if(mSelection.Count() == 0)
    {
        ESS_LOG_WARNING("[alembic] No objects specified.");
        return false;
    }

    // check frames
    if(mFrames.size() == 0)
    {
        ESS_LOG_WARNING("[alembic] No frames specified.");
        return false;
    }

    // init archive (use a locally scoped archive)
    std::string sceneFileName = "Exported from: ";
    sceneFileName.append(mApplication->GetCurFilePath());
    try
    {
        mArchive = CreateArchiveWithInfo(Alembic::AbcCoreHDF5::WriteArchive(), mFileName.c_str(), "Max Alembic Plugin", sceneFileName.c_str(), Alembic::Abc::ErrorHandler::kThrowPolicy);
    }
    catch(Alembic::Util::Exception& e)
    {
        std::string exc(e.what());
        ESS_LOG_ERROR("[alembic] Error writing to file. Do you still have it opened?");
        return false;
    }

    // get the frame rate
    mFrameRate = static_cast<float>(GetFrameRate());
    if(mFrameRate == 0.0f)
    {
        mFrameRate = 25.0f;
    }

    std::vector<Alembic::AbcCoreAbstract::chrono_t> frames;
    for(LONG i=0;i<mFrames.size();i++)
    {
        frames.push_back(mFrames[i] / mFrameRate);
    }

    // create the sampling
    double timePerSample = 1.0 / mFrameRate;
    if(frames.size() > 1)
    {
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
    for (ObjectEntry *object = mSelection.head; object != NULL; object = object->next) 
    {
        // Only export selected objects if told
        if (GetOption("exportSelected") && !object->entry->node->Selected())
            continue;

        int type = object->entry->type;
		if (type == OBTYPE_MESH) 
        {
            AlembicObjectPtr ptr;
            ptr.reset(new AlembicPolyMesh(*object->entry,this));            
            AddObject(ptr);
		}
        else if (type == OBTYPE_CAMERA)
        {
            AlembicObjectPtr ptr;
            ptr.reset(new AlembicCamera(*object->entry,this));
            AddObject(ptr);
        }
        else if (type == OBTYPE_DUMMY)
        {
            AlembicObjectPtr ptr;
            ptr.reset(new AlembicXForm(*object->entry,this));            
            AddObject(ptr);
        }
        else if (type == OBTYPE_POINTS)
        {
            AlembicObjectPtr ptr;
            ptr.reset(new AlembicPoints(*object->entry,this));
            AddObject(ptr);
        }
        else if (type == OBTYPE_CURVES)
        {
            AlembicObjectPtr ptr;
            ptr.reset(new AlembicCurves(*object->entry,this));
            AddObject(ptr);
        }

        // push all models up the hierarchy
        /*
        Model model(xObj.GetRef());
        if(!model.IsValid())
            model = xObj.GetModel();
        CRefArray modelRefs;
        while(model.IsValid() && !model.GetFullName().IsEqualNoCase(Application().GetActiveSceneRoot().GetFullName()))
        {
            modelRefs.Add(model.GetActivePrimitive().GetRef());
            model = model.GetModel();
        }
        for(LONG j=modelRefs.GetCount()-1;j>=0;j--)
        {
            if(GetRefObject(modelRefs[j]) == NULL)
            {
                AlembicObjectPtr ptr;
                ptr.reset(new AlembicModel(modelRefs[j],this));
                AddObject(ptr);
            }
        }
        */

        /*
        // take care of all other types
        else if(xObj.GetType().IsEqualNoCase(L"polymsh"))
        {
            Property geomProp;
            xObj.GetPropertyFromName(L"geomapprox",geomProp);
            LONG subDivLevel = geomProp.GetParameterValue(L"gapproxmordrsl");
            if(subDivLevel > 0)
            {
                AlembicObjectPtr ptr;
                ptr.reset(new AlembicSubD(xObj.GetActivePrimitive().GetRef(),this));
                AddObject(ptr);
            }
            else
            {
                AlembicObjectPtr ptr;
                ptr.reset(new AlembicPolyMesh(xObj.GetActivePrimitive().GetRef(),this));
                AddObject(ptr);
            }
        }
        else if(xObj.GetType().IsEqualNoCase(L"crvlist"))
        {
            AlembicObjectPtr ptr;
            ptr.reset(new AlembicCurves(xObj.GetActivePrimitive().GetRef(),this));
            AddObject(ptr);
        }
        else if(xObj.GetType().IsEqualNoCase(L"hair"))
        {
            AlembicObjectPtr ptr;
            ptr.reset(new AlembicCurves(xObj.GetActivePrimitive().GetRef(),this));
            AddObject(ptr);
        }
        else if(xObj.GetType().IsEqualNoCase(L"pointcloud"))
        {
            AlembicObjectPtr ptr;
            ICEAttribute strandPosition = xObj.GetActivePrimitive().GetGeometry().GetICEAttributeFromName(L"StrandPosition");
            if(strandPosition.IsDefined() && strandPosition.IsValid())
            {
                ptr.reset(new AlembicCurves(xObj.GetActivePrimitive().GetRef(),this));
            }
            else
            {
                ptr.reset(new AlembicPoints(xObj.GetActivePrimitive().GetRef(),this));
            }
            AddObject(ptr);
        }
        */
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
        for(size_t j=0; j < mObjects.size(); j++)
        {
            bool status = mObjects[j]->Save(mFrames[i]);
            if(status != true)
            {
                return status;
            }
            result = true;
        }
    }

    return result;
}
