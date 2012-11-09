#ifndef __XSI_SCENE_GRAPH_H
#define __XSI_SCENE_GRAPH_H

#include "CommonSceneGraph.h"

SceneNodePtr buildCommonSceneGraph(XSI::X3DObject xsiRoot);

bool hasExtractableTransform( SceneNode::nodeTypeE type );

#endif