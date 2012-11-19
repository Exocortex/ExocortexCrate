#ifndef __XSI_SCENE_GRAPH_H
#define __XSI_SCENE_GRAPH_H

#include "CommonSceneGraph.h"


class SceneNodeXSI : public SceneNodeApp
{
public:

   XSI::CRef nodeRef;

   SceneNodeXSI(XSI::CRef ref):nodeRef(ref)
   {}

   virtual bool replaceData(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAlembicPtr& nextFileNode);
   virtual bool addChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode);
   virtual void print();
};

typedef boost::shared_ptr<SceneNodeXSI> SceneNodeXSIPtr;

SceneNodeXSIPtr buildCommonSceneGraph(XSI::CRef xsiRoot);

bool hasExtractableTransform( SceneNode::nodeTypeE type );

#endif