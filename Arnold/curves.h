#ifndef _ARNOLD_ALEMBIC_CURVES_H_
#define _ARNOLD_ALEMBIC_CURVES_H_

#include "common.h"

AtNode *createCurvesNode(nodeData &nodata, userData *ud,
                         std::vector<float> &samples, int i);

#endif
