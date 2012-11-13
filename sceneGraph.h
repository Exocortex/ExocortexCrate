#ifndef __MAYA_SCENE_GRAPH_H
#define __MAYA_SCENE_GRAPH_H

	#include "CommonSceneGraph.h"

	/*
	 * Build a scene graph base on the selected objects in the scene!
	 * @param dagPath - The dag node of the root
	 */
	exoNodePtr buildCommonSceneGraph(const MDagPath &dagPath);

#endif