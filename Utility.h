#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "Foundation.h"
#include <xsi_ref.h>

struct SampleInfo
{
   Alembic::AbcCoreAbstract::index_t floorIndex;
   Alembic::AbcCoreAbstract::index_t ceilIndex;
   double alpha;
};

SampleInfo getSampleInfo(double iFrame,Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps);




#endif  // _FOUNDATION_H_
