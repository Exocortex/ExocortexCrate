#include "dataUniqueness.h"

// --------------------------------------------------------------------------------------------------
typedef struct __uv_map_key
{
   int vertexId;
   float uv_x;
   float uv_y;
} uv_map_key;

class uv_map_key_less: public std::binary_function<uv_map_key, uv_map_key, bool>
{
public:
   bool operator()(const uv_map_key &p1, const uv_map_key &p2) const
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

typedef std::map<uv_map_key, int, uv_map_key_less> uv_map_map_key_to_int;    // a map from UVs to their respective indices!

AtArray *removeUvsDuplicate(Alembic::AbcGeom::IV2fGeomParam &uvParam, SampleInfo &sampleInfo, AtArray *uvsIdx, AtArray *faceIndices)
{
   const int nb_indices = faceIndices->nelements;
   Alembic::Abc::V2fArraySamplePtr abcUvs = uvParam.getExpandedValue(sampleInfo.floorIndex).getVals();
   uv_map_map_key_to_int UVs_map;

   // fill the map and rewrite uvsIdx
   int new_idx = 0;
   for (int i = 0; i < nb_indices; ++i)
   {
      uv_map_key mkey;
      mkey.vertexId = AiArrayGetUInt(faceIndices, i);
      unsigned int uvs_id = AiArrayGetUInt(uvsIdx, i);
      const Alembic::Abc::V2fArraySamplePtr::value_type::value_type &UV = abcUvs->get()[uvs_id];
      mkey.uv_x = UV.x;
      mkey.uv_y = UV.y;

      if (UVs_map.find(mkey) == UVs_map.end())
      {
         UVs_map[mkey] = new_idx;
         AiArraySetUInt(uvsIdx, i, new_idx);
         ++new_idx;
      }
      else
        AiArraySetUInt(uvsIdx, i, UVs_map[mkey]);   // replace with the right index
   }

   // fill the UVs
   AtArray *uvs = AiArrayAllocate((AtUInt32)UVs_map.size(), 1, AI_TYPE_POINT2);
   for (uv_map_map_key_to_int::const_iterator beg = UVs_map.begin(); beg != UVs_map.end(); ++beg)
   {
      AtPoint2 pt;
      pt.x = beg->first.uv_x;
      pt.y = beg->first.uv_y;

      AiArraySetPnt2(uvs, beg->second, pt);
   }
   return uvs;
}

// --------------------------------------------------------------------------------------------------
typedef struct __n_map_key
{
   int vertexId;
   float n_x;
   float n_y;
   float n_z;
} n_map_key;

class n_map_key_less: public std::binary_function<n_map_key, n_map_key, bool>
{
public:
   bool operator()(const n_map_key &p1, const n_map_key &p2) const
   {
      if (p1.vertexId < p2.vertexId)
         return true;
      else if (p1.vertexId > p2.vertexId)
         return false;

      else if (p1.n_x < p2.n_x)
         return true;
      else if (p1.n_x > p2.n_x)
         return false;

      else if (p1.n_y < p2.n_y)
         return true;
      else if (p1.n_y > p2.n_y)
         return false;

      return p1.n_z < p2.n_z;
   }
};

typedef std::map<n_map_key, int, n_map_key_less> n_map_map_key_to_int;    // a map from UVs to their respective indices!

static AtArray *fillNormals(const n_map_map_key_to_int &Ns_map)
{
   AtArray *ns = AiArrayAllocate((AtUInt32)Ns_map.size(), 1, AI_TYPE_VECTOR);
   for (n_map_map_key_to_int::const_iterator beg = Ns_map.begin(); beg != Ns_map.end(); ++beg)
   {
      AtVector norm;
      norm.x = beg->first.n_x;
      norm.y = beg->first.n_y;
      norm.z = beg->first.n_z;

      AiArraySetVec(ns, beg->second, norm);
   }
   return ns;
}

AtArray *removeNormalsDuplicate(Alembic::Abc::N3fArraySamplePtr &abcN, SampleInfo &sampleInfo, AtArray *nIdx, AtArray *faceIndices)
{
   const int nb_indices = faceIndices->nelements;
   n_map_map_key_to_int Ns_map;

   // fill the map and rewrite uvsIdx
   int new_idx = 0;
   for (int i = 0; i < nb_indices; ++i)
   {
      n_map_key mkey;
      mkey.vertexId = AiArrayGetUInt(faceIndices, i);
      unsigned int n_id = AiArrayGetUInt(nIdx, i);
      const Alembic::Abc::N3fArraySamplePtr::value_type::value_type &N1 = abcN->get()[n_id];
      mkey.n_x = N1.x;
      mkey.n_y = N1.y;
      mkey.n_z = N1.z;

      if (Ns_map.find(mkey) == Ns_map.end())
      {
         Ns_map[mkey] = new_idx;
         AiArraySetUInt(nIdx, i, new_idx);
         ++new_idx;
      }
      else
        AiArraySetUInt(nIdx, i, Ns_map[mkey]);   // replace with the right index
   }

   // fill the Ns
   return fillNormals(Ns_map);
}

AtArray *removeNormalsDuplicateDynTopology(Alembic::Abc::N3fArraySamplePtr &abcN1, Alembic::Abc::N3fArraySamplePtr &abcN2, const float alpha,
                                           SampleInfo &sampleInfo, AtArray *nIdx, AtArray *faceIndices)
{
   const int nb_indices = faceIndices->nelements;
   const float beta = 1.0f - alpha;
   n_map_map_key_to_int Ns_map;

   // fill the map and rewrite uvsIdx
   int new_idx = 0;
   for (int i = 0; i < nb_indices; ++i)
   {
      n_map_key mkey;
      mkey.vertexId = AiArrayGetUInt(faceIndices, i);
      unsigned int n_id = AiArrayGetUInt(nIdx, i);
      const Alembic::Abc::N3fArraySamplePtr::value_type::value_type &N1 = abcN1->get()[n_id],
                                                                    &N2 = abcN2->get()[n_id];
      mkey.n_x = N1.x * beta + N2.x * alpha;
      mkey.n_y = N1.y * beta + N2.y * alpha;
      mkey.n_z = N1.z * beta + N2.z * alpha;

      if (Ns_map.find(mkey) == Ns_map.end())
      {
         Ns_map[mkey] = new_idx;
         AiArraySetUInt(nIdx, i, new_idx);
         ++new_idx;
      }
      else
        AiArraySetUInt(nIdx, i, Ns_map[mkey]);   // replace with the right index
   }

   // fill the Ns
   return fillNormals(Ns_map);
}


