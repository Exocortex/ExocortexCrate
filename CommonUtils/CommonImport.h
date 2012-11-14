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





class SceneNodeAlembic;
class SceneNodeApp : public SceneNode
{
public:

   virtual bool replaceData(SceneNodePtr node, const IJobStringParser& jobParams);
   SceneNodePtr attachChild(SceneNodePtr node, const IJobStringParser& jobParams);
   //the method can check if its node can be merged with one of its children (for XSI and MAX)
   //the the merged childs selected flag will be set to false, so that we know to skip over it
};

class SceneNodeAlembic : public SceneNode
{
public:
   Abc::IObject iObj;

   SceneNodeAlembic(Abc::IObject& obj):iObj(obj)
   {}

   virtual Abc::IObject getObject();

   //all alembic nodes will start out as selected
   //will set the selected flag to false if the node was merged
};




SceneNodePtr buildCommonSceneGraph(AbcArchiveCache *pArchiveCache, AbcObjectCache *pRootObjectCache, int& nNumNodes);



bool ImportSceneFile(const IJobStringParser& jobParams, SceneNodePtr fileRoot);
bool AttachSceneFile(const IJobStringParser& jobParams, SceneNodePtr fileRoot, SceneNodePtr appRoot);



#endif