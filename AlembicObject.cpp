#include "AlembicObject.h"
#include <xsi_ref.h>
#include <xsi_value.h>
#include <boost/algorithm/string.hpp>

#pragma warning( disable: 4996 )


AlembicObject::AlembicObject
(
   const XSI::CRef & in_Ref,
   AlembicWriteJob * in_Job
)
{
   AddRef(in_Ref);
   mJob = in_Job;
   mOParent = mJob->GetTop();

   bool bFlatten = mJob->GetOption("flattenHierarchy");
   if(!bFlatten){

      // find the parent
      std::string identifier = getIdentifierFromRef(GetRef(), true);//mJob->GetOption(L"transformCache"));
      std::vector<std::string> parts;
      boost::split(parts, identifier, boost::is_any_of("/"));

      for(size_t i=1;i<parts.size();i++)
      {
         int numChildren = (int) mOParent.getNumChildren();
         for(size_t j=0;j<numChildren;j++)
         {
            Abc::ObjectHeader const* pChildHeader = mOParent.getChildHeader( parts[i] );
            if( pChildHeader != NULL )
            {
               mOParent = mOParent.getChild( parts[i] );
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