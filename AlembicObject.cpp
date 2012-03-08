#include "AlembicObject.h"
#include <boost/algorithm/string.hpp>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

AlembicObject::AlembicObject
(
   const MObject & in_Ref,
   AlembicWriteJob * in_Job
)
{
   AddRef(in_Ref);
   mJob = in_Job;

   // find the parent
   if(mParent == NULL)
   {
      MFnDagNode dag(in_Ref);
      for(unsigned int i=0;i<dag.parentCount();i++)
      {
         MObject parentRef = dag.parent(i);
         if(!parentRef.isNull())
         {
            mParent = mJob->GetObject(parentRef);
            if(mParent)
               break;
         }
      }
   }

   mNumSamples = 0;
}

AlembicObject::~AlembicObject()
{
}

Alembic::Abc::OObject AlembicObject::GetParentObject()
{
   if(mParent)
      return mParent->GetObject();
   return mJob->GetArchive().getTop();
}


unsigned int gRefIdMax = 0;
std::map<unsigned int,AlembicObjectNode*> gNodes;

AlembicObjectNode::AlembicObjectNode()
{
   mRefId = gRefIdMax;
   gRefIdMax++;
   gNodes.insert(std::pair<unsigned int,AlembicObjectNode*>(mRefId,this));
}

AlembicObjectNode::~AlembicObjectNode()
{
   gNodes.erase(gNodes.find(mRefId));
}

void preDestructAllNodes()
{
   std::map<unsigned int,AlembicObjectNode*>::iterator it;
   for(it = gNodes.begin(); it != gNodes.end(); it++)
      it->second->PreDestruction();
}
