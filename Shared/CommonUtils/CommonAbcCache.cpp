#include "CommonAbcCache.h"
#include "CommonAlembic.h"
#include "CommonMeshUtilities.h"
#include "CommonUtilities.h"

AbcObjectCache::AbcObjectCache(Alembic::Abc::IObject& objToCache)
    : obj(objToCache),
      isMeshTopoDynamic(false),
      isMeshPointCache(false),
      fullName(objToCache.getFullName())
{
  ESS_PROFILE_SCOPE("AbcObjectCache::AbcObjectCache");

  BasicSchemaData bsd;
  getBasicSchemaDataFromObject(objToCache, bsd);
  isConstant = bsd.isConstant;
  numSamples = bsd.nbSamples;
  bool isMesh = true;
  if (bsd.type == bsd.__POLYMESH || !(isMesh = (bsd.type != bsd.__SUBDIV))) {
    bool isTopoDyn = false;
    extractMeshInfo(&objToCache, isMesh, isMeshPointCache, isTopoDyn);
    if (!isConstant) isMeshTopoDynamic = isTopoDyn;
  }
}

AbcObjectCache::~AbcObjectCache() {}
IXformPtr AbcObjectCache::getXform()
{
  if (!pObjXform && obj.valid() && AbcG::IXform::matches(obj.getMetaData())) {
    pObjXform = IXformPtr(new AbcG::IXform(obj, Abc::kWrapExisting));

    // iXformVec.resize(numSamples);
    // for(int i=0; i<iXformVec.size(); i++){
    //   AbcG::XformSample sample;
    //   pObjXform->getSchema().get(sample, i);
    //   iXformVec[i] = sample.getMatrix();
    //}
  }
  return pObjXform;
}

Abc::M44d AbcObjectCache::getXformMatrix(int index)
{
  if (iXformMap.find(index) == iXformMap.end()) {
    AbcG::XformSample sample;
    getXform();
    pObjXform->getSchema().get(sample, index);
    Abc::M44d mat = sample.getMatrix();
    iXformMap[index] = mat;
    return mat;
  }
  return iXformMap[index];
}

AbcObjectCache* addObjectToCache(AbcArchiveCache* fullNameToObjectCache,
                                 Abc::IObject& obj,
                                 std::string parentIdentifier,
                                 CommonProgressBar* pBar)
{
  ESS_PROFILE_SCOPE("addObjectToCache");
  AbcObjectCache objectCache(obj);
  objectCache.parentIdentifier = parentIdentifier;
  for (int i = 0; i < obj.getNumChildren(); i++) {
    if (pBar) {
      pBar->incr(1);
      if (!(i % 20) && pBar->isCancelled()) return 0;
    }

    Abc::IObject child = obj.getChild(i);
    AbcObjectCache* childObjectCache = addObjectToCache(
        fullNameToObjectCache, child, objectCache.fullName, pBar);
    if (childObjectCache == 0) return 0;

    objectCache.childIdentifiers.push_back(childObjectCache->fullName);
  }
  fullNameToObjectCache->insert(
      AbcArchiveCache::value_type(objectCache.fullName, objectCache));
  return &(fullNameToObjectCache->find(objectCache.fullName)->second);
}

bool s_hasRunOnceRun = false;

void runonce()
{
#ifdef _MSC_VER
  if (!s_hasRunOnceRun) {
    _setmaxstdio(2048);
    s_hasRunOnceRun = true;
  }
#endif
}

bool createAbcArchiveCache(Abc::IArchive* pArchive,
                           AbcArchiveCache* fullNameToObjectCache,
                           CommonProgressBar* pBar)
{
  ESS_PROFILE_SCOPE("createAbcArchiveCache");
  EC_LOG_INFO("Creating AbcArchiveCache for archive: " << pArchive->getName());

  runonce();

  Abc::IObject top = pArchive->getTop();
  return addObjectToCache(fullNameToObjectCache, top, "", pBar) != 0;
}
