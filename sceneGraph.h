#ifndef __XSI_SCENE_GRAPH_H
#define __XSI_SCENE_GRAPH_H

#include "CommonSceneGraph.h"


class SceneNodeXSI : public SceneNodeApp
{
public:

   XSI::CRef nodeRef;

   SceneNodeXSI(XSI::CRef ref):nodeRef(ref)
   {}

   virtual bool replaceData(SceneNodePtr fileNode, const IJobStringParser& jobParams);
   virtual bool addChild(SceneNodePtr fileNode, const IJobStringParser& jobParams, SceneNodePtr newAppNode);
};


SceneNodePtr buildCommonSceneGraph(XSI::CRef xsiRoot);

bool hasExtractableTransform( SceneNode::nodeTypeE type );

#endif