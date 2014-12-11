#ifndef _ARNOLD_ALEMBIC_POLY_MESH_H_
#define _ARNOLD_ALEMBIC_POLY_MESH_H_

  #include "common.h"

  AtNode *createPolyMeshNode(nodeData &nodata, userData * ud, std::vector<float> &samples, int i);
  AtNode *createSubDNode(nodeData &nodata, userData * ud, std::vector<float> &samples, int i);

#endif
