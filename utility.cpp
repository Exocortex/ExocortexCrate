#include "utility.h"

SampleInfo getSampleInfo
(
   double iFrame,
   Alembic::AbcCoreAbstract::TimeSamplingPtr iTime,
   size_t numSamps
)
{
   SampleInfo result;
   if (numSamps == 0)
      numSamps = 1;

   std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndex =
   iTime->getFloorIndex(iFrame, numSamps);

   result.floorIndex = floorIndex.first;
   result.ceilIndex = result.floorIndex;

   // check if we have a full license
   if(!HasFullLicense())
   {
      if(result.floorIndex > 75)
      {
         AiMsgWarning("[ExocortexAlembic] Demo Mode: Cannot open sample indices higher than 75.");
         result.floorIndex = 75;
         result.ceilIndex = 75;
         result.alpha = 0.0;
         return result;
      }
   }

   if (fabs(iFrame - floorIndex.second) < 0.0001) {
      result.alpha = 0.0f;
      return result;
   }

   std::pair<Alembic::AbcCoreAbstract::index_t, double> ceilIndex =
   iTime->getCeilIndex(iFrame, numSamps);

   if (fabs(iFrame - ceilIndex.second) < 0.0001) {
      result.floorIndex = ceilIndex.first;
      result.ceilIndex = result.floorIndex;
      result.alpha = 0.0f;
      return result;
   }

   if (result.floorIndex == ceilIndex.first) {
      result.alpha = 0.0f;
      return result;
   }

   result.ceilIndex = ceilIndex.first;

   result.alpha = (iFrame - floorIndex.second) / (ceilIndex.second - floorIndex.second);
   return result;
}