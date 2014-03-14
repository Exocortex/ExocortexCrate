#include "stdafx.h"
#include "Alembic.h"
#include "AlembicObject.h"
#include "utility.h"

#pragma warning( disable: 4996 )


AlembicObject::AlembicObject (SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent )
{     
    //AddRef(in_Ref);
    mJob = in_Job;
    mOParent = oParent;
	bForever = false;
   mExoSceneNode = eNode;

   SceneNodeMaxPtr maxNode = reinterpret<SceneNode, SceneNodeMax>(eNode);
   mINode = maxNode->node;

	mNumSamples = 0;
}

AlembicObject::~AlembicObject()
{
}

AlembicWriteJob *AlembicObject::GetCurrentJob() 
{ 
    return mJob; 
}

//const SceneEntry &AlembicObject::GetRef(unsigned long index) 
//{ 
//    if (index < mRefs.size())
//    {
//        return *mRefs[index];
//    }
//
//    return *mRefs[0]; 
//}
//
//int AlembicObject::GetRefCount() 
//{ 
//    return static_cast<int>(mRefs.size()); 
//}
//
//void AlembicObject::AddRef(const SceneEntry & in_Ref) 
//{        
//    mRefs.push_back(&in_Ref); 
//}

Abc::OObject AlembicObject::GetOParent() 
{ 
    return mOParent; 
}

int AlembicObject::GetNumSamples() 
{ 
    return mNumSamples; 
}