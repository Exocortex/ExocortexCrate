#include "AlembicObject.h"
#include <xsi_ref.h>
#include <xsi_value.h>
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
   mOParent = mJob->GetTop();

   // find the parent
   std::string identifier = getIdentifierFromRef(GetRef(),mJob->GetOption(L"transformCache"));
   std::vector<std::string> parts;
   boost::split(parts, identifier, boost::is_any_of("/"));

   for(size_t i=1;i<parts.size();i++)
   {
	   int numChildren = mOParent.getNumChildren();
     // for(size_t j=0;j<numChildren;j++)
      //{
		 Alembic::Abc::ObjectHeader const* pChildHeader = mOParent.getChildHeader( parts[i] );
         if( pChildHeader != NULL )
         {
            mOParent = mOParent.getChild( parts[i] );
          //  break;
         }
     // }
   }

   mNumSamples = 0;
}

AlembicObject::~AlembicObject()
{
}
