#include "stdafx.h"
#include "hashInstanceTable.h"


//exists, add, and clear


std::map<AbcU::Digest, INode*> g_instanceMap;



bool InstanceMap_Exists(AbcU::Digest digest)
{
   return g_instanceMap.find(digest) != g_instanceMap.end();
}

void InstanceMap_Add(AbcU::Digest digest, INode* node)
{
   g_instanceMap[digest] = node;
}

INode* InstanceMap_Get(AbcU::Digest digest)
{
   std::map<AbcU::Digest, INode*>::iterator it = g_instanceMap.find(digest);
   if(it != g_instanceMap.end())
   {
      return it->second;
   }
   return NULL;
}

void InstanceMap_Clear()
{
   g_instanceMap.clear();
}