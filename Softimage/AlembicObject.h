#ifndef _ALEMBIC_OBJECT_H_
#define _ALEMBIC_OBJECT_H_

#include "AlembicLicensing.h"
#include "AlembicMetaData.h"
#include "CommonSceneGraph.h"

class AlembicWriteJob;

class AlembicObject {
 private:
  XSI::CRefArray mRefs;
  AlembicWriteJob* mJob;
  Abc::OObject mMyParent;

 protected:
  int mNumSamples;

  enum refType {
    REF_NODE,
    REF_PRIMITIVE,
    REF_GLOBAL_TRANS,
    REF_PARENT_GLOBAL_TRANS
  };

 public:
  SceneNodePtr mExoSceneNode;

  AlembicObject(SceneNodePtr eNode, AlembicWriteJob* in_Job,
                Abc::OObject oParent);
  ~AlembicObject();

  AlembicWriteJob* GetJob() { return mJob; }
  const XSI::CRef& GetRef(ULONG index = 0) { return mRefs[index]; }
  ULONG GetRefCount() { return mRefs.GetCount(); }
  void AddRef(const XSI::CRef& in_Ref) { mRefs.Add(in_Ref); }
  Abc::OObject GetMyParent() { return mMyParent; }
  virtual Abc::OCompoundProperty GetCompound() = 0;
  int GetNumSamples() { return mNumSamples; }
  // std::string GetXfoName();

  virtual XSI::CStatus Save(double time) = 0;
};

typedef boost::shared_ptr<AlembicObject> AlembicObjectPtr;

class alembic_UD {
 public:
  alembic_UD(ULONG in_id);
  ~alembic_UD();

  double lastTime;

  std::vector<double> times;
  std::map<size_t, Abc::M44d> indexToMatrices;

  static void clearAll();

 private:
  ULONG id;
  static std::map<ULONG, alembic_UD*> gAlembicUDs;
};

#include "AlembicWriteJob.h"

#endif