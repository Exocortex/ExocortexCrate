#ifndef __ALEMBIC_INTERMEDIATE_POLYMESH_H__
#define __ALEMBIC_INTERMEDIATE_POLYMESH_H__

//
#include "CommonMeshUtilities.h"
#include "CommonSceneGraph.h"

class CommonOptions
{
   typedef std::map<std::string, int>  optionMapT;
   optionMapT optionMap;
public:
   inline bool GetBoolOption(const std::string& name) const
   {
      optionMapT::const_iterator it = optionMap.find(name);
      if(it != optionMap.end()){
         return it->second != 0;
      }
      return false;
   }

   inline int GetIntOption(const std::string& name) const
   {
      optionMapT::const_iterator it = optionMap.find(name);
      if(it != optionMap.end()){
         return it->second;
      }
      return -1;
   }

   inline void AddOption(const std::string& name, int val)
   {
      optionMap[name] = val;
   }
};




typedef std::vector<AbcA::int32_t> facesetmap_vec;

struct FaceSetStruct
{
	std::string name;
	facesetmap_vec faceIds;
};


   

class CommonIntermediatePolyMesh
{
public:

   CommonIntermediatePolyMesh():bGeomApprox(0)
   {}

	Abc::Box3d bbox;

	std::vector<Abc::V3f> posVec;
	std::vector<AbcA::int32_t> mFaceCountVec;
	std::vector<AbcA::int32_t> mFaceIndicesVec;  

   std::vector<Abc::V3f> mVelocitiesVec;

	IndexedNormals mIndexedNormals;
	std::vector<IndexedUVs> mIndexedUVSet;
	
	//std::vector<Abc::uint32_t> mMatIdIndexVec;
   std::vector<FaceSetStruct> mFaceSets;

   std::vector<float> mUvOptionsVec;

   std::vector<Abc::V3f> mBindPoseVec;
   
   int bGeomApprox;

   //std::vector<float> mRadiusVec;

   virtual void Save(SceneNodePtr eNode, const Imath::M44f& transform44f, const CommonOptions& options, double time){}

	bool mergeWith(const CommonIntermediatePolyMesh& srcMesh);

   void clear();
};


#endif