#ifndef _ALEMBIC_POINTSUTILS_H_
#define _ALEMBIC_POINTSUTILS_H_

#include "AlembicMax.h"

Mesh* getParticleSystemRenderMesh(TimeValue ticks, Object* obj, INode* node, BOOL& bNeedDelete);

#endif
