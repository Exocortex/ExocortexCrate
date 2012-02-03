#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "Foundation.h"

class SceneEntry;

struct SampleInfo
{
   Alembic::AbcCoreAbstract::index_t floorIndex;
   Alembic::AbcCoreAbstract::index_t ceilIndex;
   double alpha;
};

SampleInfo getSampleInfo(double iFrame,Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps);
std::string getIdentifierFromRef(const SceneEntry &in_Ref);




#endif  // _FOUNDATION_H_
