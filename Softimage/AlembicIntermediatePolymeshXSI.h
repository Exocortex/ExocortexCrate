#ifndef _ALEMBIC_INTERMEDIATE_POLYMESH_XSI_H_
#define _ALEMBIC_INTERMEDIATE_POLYMESH_XSI_H_

#include "CommonIntermediatePolyMesh.h"

typedef std::vector<AbcA::int32_t> facesetmap_vec;

struct FaceSetStruct {
  facesetmap_vec faceIds;
};

typedef std::map<std::string, FaceSetStruct> FaceSetMap;

class IntermediatePolyMeshXSI : public CommonIntermediatePolyMesh {
 public:
  FaceSetMap mFaceSets;

  virtual void Save(SceneNodePtr eNode, const Imath::M44f& transform44f,
                    const CommonOptions& options, double time);
  virtual bool mergeWith(const CommonIntermediatePolyMesh& srcMesh);
  virtual void clear();
};

#endif