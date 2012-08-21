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

typedef std::pair<float, float> pairff;      // a pair of floats, to represent a UV coordinate

class pairff_less: public std::binary_function<pairff, pairff, bool>
{
public:
   bool operator()(const pairff &p1, const pairff &p2)
   {
      if (p1.first == p2.first)
         return p1.second < p2.second;
      return p1.first < p2.first;
   }
};

typedef std::map<pairff, int, pairff_less> map_ff_int;    // a map from UVs to their respective indices!

bool removeUvsDuplicate(Alembic::AbcGeom::IV2fGeomParam &uvParam, SampleInfo &sampleInfo, AtArray *uvs, AtArray *uvsIdx)
{
   const Alembic::Abc::V2fArraySamplePtr    abcUvs   = uvParam.getExpandedValue(sampleInfo.floorIndex).getVals();
   const Alembic::Abc::UInt32ArraySamplePtr abcUvIdx = uvParam.getExpandedValue(sampleInfo.floorIndex).getIndices();
   
   // fill the MAP!
   const int nb_uvs = abcUvs->size();
   map_ff_int UVs;
   for (int i = 0; i < nb_uvs; ++i)
   {
      const pairff curUV(abcUvs->get()[i].x, abcUvs->get()[i].y); // the current pair!
      if (UVs.find(curUV) == UVs.end())   // not found, add it!
      {
         UVs[curUV] = -1;  // not putting the right index now, because cannot know where it will be exactly!
      }
   }

   // reindex the map and fill the UV array
   {
      int new_idx = 0;
      uvs = AiArrayAllocate(UVs.size(), 1, AI_TYPE_POINT2); // uvs point
      AtPoint2 in_pt;
      for (map_ff_int::iterator beg = UVs.begin(); beg != UVs.end(); ++beg, ++new_idx)
      {
         beg->second = new_idx;     // new index
         in_pt.x = beg->first.first;
         in_pt.y = beg->first.second;
         AiArraySetPnt2(uvs, new_idx, in_pt);
      }
   }

   // fill the index array
   {
      const int nb_idx = abcUvIdx->size();
      uvsIdx = AiArrayAllocate(nb_idx, 1, AI_TYPE_UINT);
      for (int i = 0; i < nb_idx; ++i)
      {
         const int curIdx = abcUvIdx->get()[i]; // current index
         const pairff curUV(abcUvs->get()[curIdx].x, abcUvs->get()[curIdx].y);   // get the current UV
         AiArraySetUInt(uvsIdx, i, UVs[curUV]); // assign replace the curIdx with the non-replicated index
      }
   }
   return true;
}

