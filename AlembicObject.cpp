#include "AlembicObject.h"
#include <xsi_ref.h>
#include <boost/algorithm/string.hpp>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

#pragma warning( disable: 4996 )


AlembicObject::AlembicObject
(
   const XSI::CRef & in_Ref,
   AlembicWriteJob * in_Job
)
{
   AddRef(in_Ref);
   mJob = in_Job;
   mOParent = mJob->GetArchive().getTop();

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

   mNumSamples = 0;
}

AlembicObject::~AlembicObject()
{
}
