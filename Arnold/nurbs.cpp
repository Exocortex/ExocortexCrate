#include "stdafx.h"

#include "nurbs.h"

AtNode *createNurbsNode(nodeData &nodata, userData *ud,
                        std::vector<float> &samples, int i)
{
  ESS_LOG_INFO("ExocortexAlembicArnoldDSO: GetNode: INuPatch");
  AiMsgWarning(
      "[ExocortexAlembicArnold] This NURBS type is not YET implemented");
  return NULL;
}
