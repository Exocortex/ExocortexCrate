#ifndef __MAYA_SCENE_GRAPH_H
#define __MAYA_SCENE_GRAPH_H

	#include "CommonSceneGraph.h"
	#include "CommonImport.h"

	class AlembicFileAndTimeControl;
	typedef boost::shared_ptr<AlembicFileAndTimeControl> AlembicFileAndTimeControlPtr;

	// Will also hold all the informations about the IJobString necessary!
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
		static AlembicFileAndTimeControlPtr createControl(const IJobStringParser& jobParams);
	};

	class SceneNodeMaya : public SceneNodeApp
	{
	private:
		AlembicFileAndTimeControlPtr fileAndTime;

		bool addXformChild(SceneNodeAlembicPtr fileNode, SceneNodeAppPtr& newAppNode);
		bool addCameraChild(SceneNodeAlembicPtr fileNode, SceneNodeAppPtr& newAppNode);
		bool addPolyMeshChild(SceneNodeAlembicPtr fileNode, SceneNodeAppPtr& newAppNode);
		bool addPointsChild(SceneNodeAlembicPtr fileNode, SceneNodeAppPtr& newAppNode);
	public:
		SceneNodeMaya(const AlembicFileAndTimeControlPtr alembicFileAndTimeControl = AlembicFileAndTimeControlPtr()): fileAndTime(alembicFileAndTimeControl)
		{}

		virtual bool replaceData(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAlembicPtr& nextFileNode);
		virtual bool addChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode);
		virtual void print(void);
	};

	/*
	 * Build a scene graph base on the selected objects in the scene!
	 * @param dagPath - The dag node of the root
	 * @param alembicFileAndTimeControl - an optional file and time controller!
	 */
	SceneNodeAppPtr buildMayaSceneGraph(const MDagPath &dagPath, const AlembicFileAndTimeControlPtr alembicFileAndTimeControl = AlembicFileAndTimeControlPtr());

#endif