#ifndef __COMMON_IMPORT_H
#define __COMMON_IMPORT_H

#include <string>
#include <vector>
#include <map>

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
   bool selectShapes;

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
      selectShapes(true)
   {}

   bool parse(const std::string& jobString);

   std::string buildJobString();
};

class CommonProgressBar
{
public:
	inline void init(int range) { init(0, range, 1); }
	virtual void init(int min, int max, int incr) = 0;
	virtual void start(void) = 0;
	virtual void stop(void) = 0;
	virtual void incr(int step=1) = 0;
	virtual bool isCancelled(void) = 0;
    virtual void setCaption(std::string& caption){}
    virtual int getUpdateCount(){ return 20; }
};

SceneNodeAlembicPtr buildAlembicSceneGraph(AbcArchiveCache *pArchiveCache, AbcObjectCache *pRootObjectCache, int& nNumNodes, bool countMergableChildren=true);

// progress bar needs to be initialized before these functions are called!
bool ImportSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams, CommonProgressBar *pBar = 0);
bool AttachSceneFile(SceneNodeAlembicPtr fileRoot, SceneNodeAppPtr appRoot, const IJobStringParser& jobParams, CommonProgressBar *pBar = 0);



#endif