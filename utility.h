#ifndef _ARNOLD_ALEMBIC_UTILITY_H_
#define _ARNOLD_ALEMBIC_UTILITY_H_

#include "foundation.h"

struct SampleInfo
{
   Alembic::AbcCoreAbstract::index_t floorIndex;
   Alembic::AbcCoreAbstract::index_t ceilIndex;
   double alpha;
};

SampleInfo getSampleInfo
(
   double iFrame,
   Alembic::AbcCoreAbstract::TimeSamplingPtr iTime,
   size_t numSamps
);

#endif