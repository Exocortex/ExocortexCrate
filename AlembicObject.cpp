#include "Alembic.h"
#include "ObjectEntry.h"
#include "AlembicObject.h"
#include <boost/algorithm/string.hpp>
#include "utility.h"

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

#pragma warning( disable: 4996 )


AlembicObject::AlembicObject (const SceneEntry & in_Ref, AlembicWriteJob * in_Job )
{     
    AddRef(in_Ref);
    mJob = in_Job;
    mOParent = mJob->GetArchive().getTop();
	bForever = false;

	bool bFlatten = GetCurrentJob()->GetOption("flattenHierarchy");
	if(!bFlatten){
		// find the parent
		std::string identifier = getIdentifierFromRef(GetRef());
		std::vector<std::string> parts;
		boost::split(parts, identifier, boost::is_any_of("/"));

		for(size_t i=1;i<parts.size();i++)
		{
		  for(size_t j=0;j<mOParent.getNumChildren();j++)
		  {
			 Alembic::Abc::OObject child = mOParent.getChild(j);
			 if(child.getName() == parts[i])
			 {
				mOParent = child;
				break;
			 }
		  }
		}
	}

	mNumSamples = 0;
}

AlembicObject::~AlembicObject()
{
}

AlembicWriteJob *AlembicObject::GetCurrentJob() 
{ 
    return mJob; 
}

const SceneEntry &AlembicObject::GetRef(unsigned long index) 
{ 
    if (index < mRefs.size())
    {
        return *mRefs[index];
    }

    return *mRefs[0]; 
}

int AlembicObject::GetRefCount() 
{ 
    return static_cast<int>(mRefs.size()); 
}

void AlembicObject::AddRef(const SceneEntry & in_Ref) 
{        
    mRefs.push_back(&in_Ref); 
}

Alembic::Abc::OObject AlembicObject::GetOParent() 
{ 
    return mOParent; 
}

int AlembicObject::GetNumSamples() 
{ 
    return mNumSamples; 
}