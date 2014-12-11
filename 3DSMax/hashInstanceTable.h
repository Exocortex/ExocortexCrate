#ifndef __HASH_INSTANCE_TABLE_H
#define __HASH_INSTANCE_TABLE_H


bool InstanceMap_Exists(AbcU::Digest digest);
void InstanceMap_Add(AbcU::Digest digest, INode* node);
INode* InstanceMap_Get(AbcU::Digest digest);
void InstanceMap_Clear();


#endif 