#ifndef _MESH_MTL_LIST_H_
#define _MESH_MTL_LIST_H_
#include <Tab.h>
#include "mtldef.h"

struct SMtl;
class Mtl;

struct MEntry 
{ 
    SMtl *sm; 
    Mtl *m; 
};

class MeshMtlList: public Tab<MEntry> 
{
public:
	void AddMtl(Mtl *m);
	void ReallyAddMtl(Mtl *m);
	int FindMtl(Mtl *m);
	int FindSName(char *nam);
	~MeshMtlList();

private:
    void FreeMatRefs(SMtl *m);
    void FreeMapDataRefs(MapData *md);
};

#endif // _MESH_MTL_LIST_H_