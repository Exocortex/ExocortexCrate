#ifndef _SCENEGRAPH_H_
#define _SCENEGRAPH_H_

#include "CommonSceneGraph.h"
#include "CommonImport.h"

class INode;
class SceneNodeMax : public SceneNodeApp
{
public:

   INode *node;
   bool bMergedSubtreeNodeParent;
   bool bIsCameraTransform;

   SceneNodeMax(INode *n):node(n), bMergedSubtreeNodeParent(false), bIsCameraTransform(false)
   {}

   SceneNodeMax(const SceneNodeMax& n, bool mergedSubtreeNodeParent):
      node(n.node), bMergedSubtreeNodeParent(mergedSubtreeNodeParent), bIsCameraTransform(false)
   {}

   //Import methods, we won't need these until we update the importer
   virtual bool replaceData(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAlembicPtr& nextFileNode);
   virtual bool addChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode);
   virtual void print();
   
   virtual Imath::M44f getGlobalTransFloat(double time);
   virtual Imath::M44d getGlobalTransDouble(double time);
   virtual bool getVisibility(double time);
};

typedef boost::shared_ptr<SceneNodeMax> SceneNodeMaxPtr;

SceneNodeMaxPtr buildCommonSceneGraph(int& nNumNodes, bool bUnmergeNodes, bool bSelectAll);





#endif