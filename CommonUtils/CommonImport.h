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

namespace XSI_XformTypes{
   enum xte{//Don't change the order! We write these out.
      UNKNOWN,
      XMODEL,
      XNULL
   };
};

namespace timeControlOptions{
   enum tco{
      NONE,
      SCENE_ROOT,
      ROOT_MODELS
   };
}

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
   bool stripMayaNamespaces;
   bool importCurvesAsStrands;
   bool useMultiFile;
   bool enableSubD;

   XSI_XformTypes::xte xformTypes;
   timeControlOptions::tco timeControl;

   SearchReplace::ReplacePtr replacer;

	std::string filename;// = EC_MCHAR_to_UTF8( strPath );

   std::vector<std::string> nodesToImport;
   std::map<std::string, std::string> extraParameters;

   bool includeChildren;
   bool replaceColonsWithUnderscores; //built-in option for XSI

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
      enableImportRootSelection(false),
      stripMayaNamespaces(false),
      importCurvesAsStrands(false),
      replaceColonsWithUnderscores(false),
	  useMultiFile(false),
      enableSubD(true)
   {}

   bool parse(const std::string& jobString);

   std::string buildJobString();

   bool paramIsSet(const std::string& param);
   void setParam(const std::string& param, bool bVal=true);
};

SceneNodeAlembicPtr buildAlembicSceneGraph(AbcArchiveCache *pArchiveCache, AbcObjectCache *pRootObjectCache, int& nNumNodes, const IJobStringParser& jobParams, bool countMergableChildren=true, CommonProgressBar *pBar = 0);

// progress bar needs to be initialized before these functions are called!
bool ImportSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams, CommonProgressBar *pBar = 0, std::list<SceneNodeAppPtr> *newNodes = 0);
bool AttachSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams, CommonProgressBar *pBar = 0, std::list<SceneNodeAppPtr> *newNodes = 0);
bool MergeSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams, CommonProgressBar *pBar = 0, std::list<SceneNodeAppPtr> *newNodes = 0);


void GetSampleRange(SceneNodeAlembicPtr fileRoot, std::size_t& oMinSample, std::size_t& oMaxSample, double& oMinTime, double& oMaxTime);

#endif