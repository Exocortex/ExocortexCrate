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
         AiMsgError("[ExocortexAlembic] Demo Mode: Cannot open sample indices higher than 75.");
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

typedef struct __map_key
{
   int vertexId;
   float uv_x;
   float uv_y;
} map_key;

class map_key_less: public std::binary_function<map_key, map_key, bool>
{
public:
   bool operator()(const map_key &p1, const map_key &p2) const
   {
      if (p1.vertexId < p2.vertexId)
         return true;
      else if (p1.vertexId > p2.vertexId)
         return false;

      else if (p1.uv_x < p2.uv_x)
         return true;
      else if (p1.uv_x > p2.uv_x)
         return false;

      return p1.uv_y < p2.uv_y;
   }
};

typedef std::map<map_key, int, map_key_less> map_map_key_to_int;    // a map from UVs to their respective indices!

AtArray *removeUvsDuplicate(Alembic::AbcGeom::IV2fGeomParam &uvParam, SampleInfo &sampleInfo,AtArray *uvsIdx, AtArray *faceIndices)
{
   const int nb_indices = faceIndices->nelements;
   Alembic::Abc::V2fArraySamplePtr abcUvs = uvParam.getExpandedValue(sampleInfo.floorIndex).getVals();
   map_map_key_to_int UVs_map;

   // fill the map and rewrite uvsIdx
   int new_idx = 0;
   for (int i = 0; i < nb_indices; ++i)
   {
      map_key mkey;
      mkey.vertexId = AiArrayGetUInt(faceIndices, i);
      unsigned int uvs_id = AiArrayGetUInt(uvsIdx, i);
      mkey.uv_x = abcUvs->get()[uvs_id].x;
      mkey.uv_y = abcUvs->get()[uvs_id].y;

      if (UVs_map.find(mkey) == UVs_map.end())
      {
         UVs_map[mkey] = new_idx;
         ++new_idx;
      }
      AiArraySetUInt(uvsIdx, i, UVs_map[mkey]);   // replace with the right index
   }

   // fill the UVs
   AtArray *uvs = AiArrayAllocate(UVs_map.size(), 1, AI_TYPE_POINT2);
   for (map_map_key_to_int::const_iterator beg = UVs_map.begin(); beg != UVs_map.end(); ++beg)
   {
      AtPoint2 pt;
      pt.x = beg->first.uv_x;
      pt.y = beg->first.uv_y;

      AiArraySetPnt2(uvs, beg->second, pt);
   }
   return uvs;
}

