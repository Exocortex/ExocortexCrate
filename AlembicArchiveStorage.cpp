#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include <boost/algorithm/string.hpp>
#pragma warning( disable: 4996 )

struct AlembicArchiveInfo
{
   Alembic::Abc::IArchive * archive;
   int refCount;

   AlembicArchiveInfo()
   {
      archive = NULL;
      refCount = 0;
   }
};
std::map<std::string,AlembicArchiveInfo> gArchives;

std::string resolvePath(std::string path)
{
	return path;
}

Alembic::Abc::IArchive * getArchiveFromID(std::string path)
{
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
   {
      // check if the file exists
      FILE * file = fopen(resolvedPath.c_str(),"rb");
      if(file != NULL)
      {
         fclose(file);
         addArchive(new Alembic::Abc::IArchive( Alembic::AbcCoreHDF5::ReadArchive(), resolvedPath));
         return getArchiveFromID(resolvedPath);
      }
      return NULL;
   }
   return it->second.archive;
}

std::string addArchive(Alembic::Abc::IArchive * archive)
{
   AlembicArchiveInfo info;
   info.archive = archive;
   gArchives.insert(std::pair<std::string,AlembicArchiveInfo>(archive->getName(),info));
   return archive->getName().c_str();
}

void deleteArchive(std::string path)
{
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return;
   it->second.archive->reset();
   delete(it->second.archive);
   gArchives.erase(it);
}

void deleteAllArchives()
{
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   for(it = gArchives.begin(); it != gArchives.end(); it++)
   {
      it->second.archive->reset();
      delete(it->second.archive);
   }
   gArchives.clear();
}

Alembic::Abc::IObject getObjectFromArchive(std::string path, std::string identifier)
{
   Alembic::Abc::IArchive * archive = getArchiveFromID(path);
   if(archive == NULL)
      return Alembic::Abc::IObject();

   // split the path
   std::vector<std::string> parts;
   boost::split(parts, identifier, boost::is_any_of("/"));

   // recurse to find it
   Alembic::Abc::IObject obj = archive->getTop();
   for(size_t i=1;i<parts.size();i++)
   {
      Alembic::Abc::IObject child(obj,parts[i]);
      obj = child;
      if(!obj)
      {
         obj = Alembic::Abc::IObject();
         break;
      }
   }

   return obj;
}

int addRefArchive(std::string path)
{
   if(path.empty())
      return -1;
   std::string resolvedPath = resolvePath(path);

   // call get archive to ensure to create it!
   getArchiveFromID(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return -1;
   it->second.refCount++;
   return it->second.refCount;
}

int delRefArchive(std::string path)
{
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return -1;
   it->second.refCount--;
   if(it->second.refCount==0)
   {
      deleteArchive(resolvedPath);
      return 0;
   }
   return it->second.refCount;
}

int getRefArchive(std::string path)
{
   std::string resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath);
   if(it == gArchives.end())
      return -1;
   return it->second.refCount;
}