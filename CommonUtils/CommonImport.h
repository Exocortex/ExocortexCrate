#ifndef __COMMON_IMPORT_H
#define __COMMON_IMPORT_H

#include <string>
#include <vector>

#include "CommonSceneGraph.h"
#include "CommonAlembic.h"
#include "CommonAbcCache.h"


class IJobStringParser
{
public:

   bool importNormals;
   bool importUVs;
   bool importMaterialIds;
   bool importFacesets;
   bool importStandinProperties;
   bool importBoundingBoxes;//import boundng boxes instead of shape
   bool attachToExisting;
   bool failOnUnsupported;
   bool importVisibilityControllers;

	std::string filename;// = EC_MCHAR_to_UTF8( strPath );

   std::vector<std::string> nodesToImport;

   bool includeChildren;

   IJobStringParser():
      importNormals(false),
      importUVs(true),
      importMaterialIds(false),
      importFacesets(true),
      importStandinProperties(false),
      importBoundingBoxes(false),
      attachToExisting(false),
      failOnUnsupported(false),
      importVisibilityControllers(false),
      includeChildren(false)
   {}

   bool parse(const std::string& jobString);

   std::string buildJobString();
};


SceneNodeAlembicPtr buildAlembicSceneGraph(AbcArchiveCache *pArchiveCache, AbcObjectCache *pRootObjectCache, int& nNumNodes);

bool ImportSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams);
bool AttachSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams);



#endif