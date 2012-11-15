#ifndef __MAYA_SCENE_GRAPH_H
#define __MAYA_SCENE_GRAPH_H

	#include "CommonSceneGraph.h"
	#include "CommonImport.h"

	class AlembicFileAndTimeControl;
	typedef boost::shared_ptr<AlembicFileAndTimeControl> AlembicFileAndTimeControlPtr;

	class AlembicFileAndTimeControl
	{
	private:
		MString var;

		AlembicFileAndTimeControl(const MString &variable): var(variable) {}
	public:
		~AlembicFileAndTimeControl(void);

		const MString &variable(void) const
		{
			return var;
		}
		static AlembicFileAndTimeControlPtr createControl(const std::string &filename);
	};

	class SceneNodeMaya : public SceneNodeApp
	{
	private:
		AlembicFileAndTimeControlPtr fileAndTime;
	public:
		SceneNodeMaya(const AlembicFileAndTimeControlPtr alembicFileAndTimeControl = AlembicFileAndTimeControlPtr()): fileAndTime(alembicFileAndTimeControl)
		{}

		virtual bool replaceData(SceneNodePtr fileNode, const IJobStringParser& jobParams);
		virtual bool addChild(SceneNodePtr fileNode, const IJobStringParser& jobParams, SceneNodePtr newAppNode);
	};

	/*
	 * Build a scene graph base on the selected objects in the scene!
	 * @param dagPath - The dag node of the root
	 * @param alembicFileAndTimeControl - an optional file and time controller!
	 */
	SceneNodePtr buildMayaSceneGraph(const MDagPath &dagPath, const AlembicFileAndTimeControlPtr alembicFileAndTimeControl = AlembicFileAndTimeControlPtr());

#endif