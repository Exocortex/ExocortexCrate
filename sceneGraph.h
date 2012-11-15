#ifndef __XSI_SCENE_GRAPH_H
#define __XSI_SCENE_GRAPH_H

#include "CommonSceneGraph.h"


class SceneNodeXSI : public SceneNodeApp
{
public:

   XSI::CRef nodeRef;

   SceneNodeXSI(XSI::CRef ref):nodeRef(ref)
   {}

   virtual SceneNodeClass::typeE getClassType();
   virtual bool replaceData(SceneNodePtr fileNode, const IJobStringParser& jobParams);
   virtual bool addChild(SceneNodePtr fileNode, const IJobStringParser& jobParams, SceneNodePtr newAppNode);
   virtual void print();
};


SceneNodePtr buildCommonSceneGraph(XSI::CRef xsiRoot);

bool hasExtractableTransform( SceneNode::nodeTypeE type );

#endif