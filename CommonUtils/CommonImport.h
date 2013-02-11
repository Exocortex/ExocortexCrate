#ifndef __COMMON_IMPORT_H
#define __COMMON_IMPORT_H

#include <string>
#include <vector>
#include <map>

#include "CommonSceneGraph.h"
#include "CommonAlembic.h"
#include "CommonAbcCache.h"

#include "CommonPBar.h"
#include "CommonRegex.h"

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
   bool selectShapes;
   bool skipUnattachedNodes;
   bool enableImportRootSelection;

   SearchReplace::ReplacePtr replacer;

	std::string filename;// = EC_MCHAR_to_UTF8( strPath );

   std::vector<std::string> nodesToImport;
   std::map<std::string, std::string> extraParameters;

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
      includeChildren(false),
      selectShapes(true),
      skipUnattachedNodes(false),
      enableImportRootSelection(false)
   {}

   bool parse(const std::string& jobString);

   std::string buildJobString();
};

SceneNodeAlembicPtr buildAlembicSceneGraph(AbcArchiveCache *pArchiveCache, AbcObjectCache *pRootObjectCache, int& nNumNodes, const IJobStringParser& jobParams, bool countMergableChildren=true, CommonProgressBar *pBar = 0);

// progress bar needs to be initialized before these functions are called!
bool ImportSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams, CommonProgressBar *pBar = 0);
bool AttachSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams, CommonProgressBar *pBar = 0);



#endif