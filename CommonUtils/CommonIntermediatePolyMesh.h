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

   inline void SetOption(const std::string& name, int val)
   {
      optionMap[name] = val;
   }

   inline void SetOption(const std::string& name, bool val)
   {
      if(val){
         optionMap[name] = 1;
      }
      else{
         optionMap[name] = 0;
      }
   }

   inline void Copy(std::map<std::string, bool> map)
   {
      for( std::map<std::string, bool>::iterator it=map.begin(); it != map.end(); it++){
         SetOption(it->first, it->second);
      }
   }
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
	


   std::vector<float> mUvOptionsVec;      ////TODO: merge?

   std::vector<Abc::V3f> mBindPoseVec;    ////TODO: merge?
   
   int bGeomApprox;                       ////TODO: merge?

   //std::vector<float> mRadiusVec;

   virtual void Save(SceneNodePtr eNode, const Imath::M44f& transform44f, const CommonOptions& options, double time) = 0;

	virtual bool mergeWith(const CommonIntermediatePolyMesh& srcMesh);

   virtual void clear() = 0;
};


#endif