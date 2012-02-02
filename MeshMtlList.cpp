#include "alembic.h"
#include "MeshMtlList.h"
#include <bitmap.h>
#include <imtl.h>
#include "mtldef.h"

void XMFreeAndZero(void **p) 
{
	if (p && *p) 
    {
		free(*p);
		*p = NULL;
	}
};

MeshMtlList::~MeshMtlList() 
{
	for (int i=0; i<Count(); i++) 
    {
		FreeMatRefs((*this)[i].sm);
		delete (*this)[i].sm;
	}
}


int MeshMtlList::FindMtl(Mtl *m) 
{
	for (int i=0; i<Count(); i++) 
    {
		if  ( (*this)[i].m == m ) 
        {
            return i;
        }
    }
	return -1;
}

static int IsStdMulti(Mtl *m) 
{
	return (m->ClassID()==Class_ID(MULTI_CLASS_ID,0))?1:0; 
}

int MeshMtlList::FindSName(char *name) 
{
	for (int i=0; i<Count(); i++) 
    {
		if  ( strcmp(name,(*this)[i].sm->name)==0) 
        {
            return i;
        }
    }
	return -1;
}

void MeshMtlList::AddMtl(Mtl *m) 
{
	if (m==NULL) 
    {
        return;
    }

	Interval v;
	m->Update(0,v);
	if (IsStdMulti(m)) 
    {
		for (int i=0; i<m->NumSubMtls(); i++) 
        {
			Mtl *sub = m->GetSubMtl(i);
			if (sub&&FindMtl(sub)<0) 
            {
				ReallyAddMtl(sub);
            }
		}
	}
	else if (FindMtl(m)<0) 
    {
	    ReallyAddMtl(m);    
	}
}

void MeshMtlList::FreeMatRefs(SMtl *m) 
{
	int k;
	if (m->appdata)
    {
        XMFreeAndZero(&m->appdata);
    }
	for (k=0; k<NMAPTYPES; k++) 
    {
		if (m->map[k]) 
        {
			FreeMapDataRefs(&m->map[k]->map);
			FreeMapDataRefs(&m->map[k]->mask);
			XMFreeAndZero((void **)&m->map[k]);
		}
	}
}

void MeshMtlList::ReallyAddMtl(Mtl *m) 
{
    // Do Something
	/*MEntry me;
	me.sm = NULL;
	me.m = m;
	SMtl *s = new SMtl;
	memset(s,0,sizeof(SMtl));
	me.sm = s;
	char buf[20];
	strncpy(buf,me.m->GetName(),16);
	if (strlen(buf)==0) strcpy(buf, "Matl");
	buf[16] = 0;
	strcpy(s->name,buf);
	int n=0;
	while (FindSName(s->name)>=0) 
    {
		IncrName(buf,s->name,++n); 
    }
	buf[16] = 0;
	ConvertMaxMtlToSMtl(s,m);
	Append(1,&me,20);*/
}	

void MeshMtlList::FreeMapDataRefs(MapData *md) 
{
	switch(md->kind) 
    {
		case 0:
			XMFreeAndZero(&md->p.tex.sxp_data);
			break;
		case 1:
			break;
	}	
}