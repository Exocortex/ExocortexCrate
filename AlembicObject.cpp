#include "stdafx.h"
#include "AlembicObject.h"

#pragma warning( disable: 4996 )


AlembicObject::AlembicObject
(
   exoNodePtr eNode,
   AlembicWriteJob * in_Job,
   Abc::OObject oParent
)
{
   mExoSceneNode = eNode;

   XSI::CRef nodeRef;
   nodeRef.Set(mExoSceneNode->dccIdentifier.c_str());
   XSI::X3DObject xObj(nodeRef);
   AddRef(nodeRef);//main node - ref 0

   AddRef(xObj.GetActivePrimitive().GetRef());//active primitive - ref 1

   AddRef(xObj.GetKinematics().GetGlobal().GetRef());//global transform - ref 2

   mJob = in_Job;
   mMyParent = oParent;
   mNumSamples = 0;
}

AlembicObject::~AlembicObject()
{
}

std::string AlembicObject::GetXfoName()
{
   XSI::X3DObject node(GetRef());
   std::string modelName(node.GetName().GetAsciiString());
   return std::string(modelName+"Xfo");
}

std::map<ULONG,alembic_UD*> alembic_UD::gAlembicUDs;

alembic_UD::alembic_UD(ULONG in_id)
{
   id = in_id;
   lastFloor = 0;

   std::map<ULONG,alembic_UD*>::iterator it = gAlembicUDs.find(id);
   if(it == gAlembicUDs.end())
      gAlembicUDs.insert(std::pair<ULONG,alembic_UD*>(id,this));
   else
      it->second = this;
}

alembic_UD::~alembic_UD()
{
   std::map<ULONG,alembic_UD*>::iterator it = gAlembicUDs.find(id);
   if(it == gAlembicUDs.end())
      return;
   gAlembicUDs.erase(it);
}

void alembic_UD::clearAll()
{
   std::map<ULONG,alembic_UD*>::iterator it = gAlembicUDs.begin();
   for(;it!=gAlembicUDs.end();it++)
   {
      it->second->times.clear();
      it->second->matrices.clear();
   }
}
