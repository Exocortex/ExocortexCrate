#include "stdafx.h"
#include "AlembicObject.h"


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

Abc::OObject AlembicObject::GetParentObject()
{
   if(mParent)
      return mParent->GetObject();
   return mJob->GetArchive().getTop();
}

MString AlembicObject::GetUniqueName(const MString & in_Name)
{
   Abc::OObject parent = GetParentObject();
   bool unique = false;
   MString name = in_Name;
   unsigned int index = 0;
   while(!unique)
   {
      unique = true;
      for(size_t i=0;i<parent.getNumChildren();i++)
      {
         MString childName = parent.getChildHeader(i).getName().c_str();
         if(childName == name)
         {
            index++;
            MString indexString;
            indexString.set((double)index);
            name = in_Name + indexString;
            unique = false;
            break;
         }
      }
   }
   return name;
}

unsigned int gRefIdMax = 0;
std::map<unsigned int,AlembicObjectNode*> gNodes;
std::map<unsigned int,AlembicObjectDeformNode*> gDeformNodes;
std::map<unsigned int,AlembicObjectEmitterNode*> gEmitterNodes;
std::map<unsigned int,AlembicObjectLocatorNode*> gLocatorNodes;

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

AlembicObjectDeformNode::AlembicObjectDeformNode()
{
   mRefId = gRefIdMax;
   gRefIdMax++;
   gDeformNodes.insert(std::pair<unsigned int,AlembicObjectDeformNode*>(mRefId,this));
}

AlembicObjectDeformNode::~AlembicObjectDeformNode()
{
   gDeformNodes.erase(gDeformNodes.find(mRefId));
}

AlembicObjectEmitterNode::AlembicObjectEmitterNode()
{
   mRefId = gRefIdMax;
   gRefIdMax++;
   gEmitterNodes.insert(std::pair<unsigned int,AlembicObjectEmitterNode*>(mRefId,this));
}

AlembicObjectEmitterNode::~AlembicObjectEmitterNode()
{
   gEmitterNodes.erase(gEmitterNodes.find(mRefId));
}

AlembicObjectLocatorNode::AlembicObjectLocatorNode()
{
   mRefId = gRefIdMax;
   gRefIdMax++;
   gLocatorNodes.insert(std::pair<unsigned int,AlembicObjectLocatorNode*>(mRefId,this));
}

AlembicObjectLocatorNode::~AlembicObjectLocatorNode()
{
   gLocatorNodes.erase(gLocatorNodes.find(mRefId));
}

void preDestructAllNodes()
{
   std::map<unsigned int,AlembicObjectNode*>::iterator it;
   for(it = gNodes.begin(); it != gNodes.end(); it++)
      it->second->PreDestruction();
   std::map<unsigned int,AlembicObjectDeformNode*>::iterator itDeform;
   for(itDeform = gDeformNodes.begin(); itDeform != gDeformNodes.end(); itDeform++)
      itDeform->second->PreDestruction();
   std::map<unsigned int,AlembicObjectEmitterNode*>::iterator itEmitter;
   for(itEmitter = gEmitterNodes.begin(); itEmitter != gEmitterNodes.end(); itEmitter++)
      itEmitter->second->PreDestruction();
   std::map<unsigned int,AlembicObjectLocatorNode*>::iterator itLocator;
   for(itLocator = gLocatorNodes.begin(); itLocator != gLocatorNodes.end(); itLocator++)
      itLocator->second->PreDestruction();
}
