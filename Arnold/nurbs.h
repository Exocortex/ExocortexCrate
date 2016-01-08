#ifndef _ARNOLD_ALEMBIC_NURBS_H_
#define _ARNOLD_ALEMBIC_NURBS_H_

#include "common.h"

AtNode *createNurbsNode(nodeData &nodata, userData *ud,
                        std::vector<float> &samples, int i);

#endif
