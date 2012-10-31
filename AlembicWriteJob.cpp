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


AlembicWriteJob::AlembicWriteJob(const std::string &in_FileName, const ObjectList &in_Selection, const std::vector<double> &in_Frames, Interface *i)
{
    mApplication = i;
	mMeshErrors = 0;
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
    std::string sceneFileName = "";
    sceneFileName.append( EC_MSTR_to_UTF8( mApplication->GetCurFilePath() ) );
    try
    {
        mArchive = CreateArchiveWithInfo(
			Alembic::AbcCoreHDF5::WriteArchive( true ), 
			mFileName.c_str(),
			getExporterName( "3DS Max " EC_QUOTE( crate_Max_Version ) ).c_str(),
			getExporterFileName( sceneFileName ).c_str(),
			Abc::ErrorHandler::kThrowPolicy);
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
	const bool bFlattenHierarchy = GetOption("flattenHierarchy");
	const bool bTransformCache = GetOption("transformCache");

	if(bTransformCache){
		for( int i = 0; i < mSelection.objectEntries.size(); i ++ ) {
			ObjectEntry *object = &( mSelection.objectEntries[i] );

			if (GetOption("exportSelected") && !object->entry.node->Selected())
				continue;

			int type = object->entry.type;
			if (type == OBTYPE_MESH || 
				type == OBTYPE_CAMERA || 
				(type == OBTYPE_DUMMY && !bFlattenHierarchy)|| 
				type == OBTYPE_POINTS || 
				type == OBTYPE_CURVES)
			{
				AlembicObjectPtr ptr;
				ptr.reset(new AlembicXForm(object->entry,this));            
				AddObject(ptr);
			}
		}
	}else{
		for( int i = 0; i < mSelection.objectEntries.size(); i ++ ) {
			ObjectEntry *object = &( mSelection.objectEntries[i] );

			if (GetOption("exportSelected") && !object->entry.node->Selected())
				continue;

			int type = object->entry.type;
			if (type == OBTYPE_MESH || (bParticleMesh && type == OBTYPE_POINTS) ) 
			{
				AlembicObjectPtr ptr;
				ptr.reset(new AlembicPolyMesh(object->entry,this));            
				AddObject(ptr);
			}
			else if (type == OBTYPE_CAMERA)
			{
				AlembicObjectPtr ptr;
				ptr.reset(new AlembicCamera(object->entry,this));
				AddObject(ptr);
			}
			else if (type == OBTYPE_DUMMY && !bFlattenHierarchy)
			{
				AlembicObjectPtr ptr;
				ptr.reset(new AlembicXForm(object->entry,this));            
				AddObject(ptr);
			}
			else if (type == OBTYPE_POINTS && !bParticleMesh)
			{
				AlembicObjectPtr ptr;
				ptr.reset(new AlembicPoints(object->entry,this));
				AddObject(ptr);
			}
			else if (type == OBTYPE_CURVES)
			{
				AlembicObjectPtr ptr;
				ptr.reset(new AlembicCurves(object->entry,this));
				AddObject(ptr);
			}
		}
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
