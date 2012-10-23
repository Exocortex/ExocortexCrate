#include "AlembicObject.h"
#include <xsi_ref.h>
#include <xsi_value.h>
#include <boost/algorithm/string.hpp>
#include <xsi_primitive.h>
#include <xsi_x3dobject.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

#pragma warning( disable: 4996 )


AlembicObject::AlembicObject
(
   const XSI::CRef & in_Ref,
   AlembicWriteJob * in_Job,
   Alembic::Abc::OObject oParent
)
{
   AddRef(in_Ref);
   mJob = in_Job;
   mMyParent = oParent;
   mNumSamples = 0;
}

AlembicObject::~AlembicObject()
{
}

std::string AlembicObject::GetXfoName()
{
   XSI::Primitive prim(GetRef());
   std::string modelName(prim.GetParent3DObject().GetName().GetAsciiString());
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
