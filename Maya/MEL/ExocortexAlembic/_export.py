import maya.cmds as cmds

""" Export module of Exocortex Crate """

def doIt(filename, exInframe, exOutframe, exObjects=None, exStepframe=1, exSubstepframe=1, exTopology=3, exUVs=True, exFaceSets=True, exDynTopo=False, exGlobSpace=False, exWithoutHierarchy=False, exXformCache=False, exUseInitShadGrp=False, exUseOgawa=False):
	"""
	Set up the string parameter for ExocortexAlembic_export
	"""
	def doIt_listExportObjects(exObjects):
		objs = str(exObjects[0])
		for obj in exObjects[1:]:
			objs = objs + "," + str(obj)
		return objs

	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._export.doIt")
	if exObjects == None:
		exObjects = cmds.ls(sl=True)
	job = "in="+str(exInframe)+";out="+str(exOutframe)+";step="+str(exStepframe)+";substep="+str(exSubstepframe)+";filename="+filename+";objects="+(doIt_listExportObjects(exObjects))+";ogawa="+str(int(exUseOgawa))

	if exTopology == 1:
		job += ";purepointcache=1;dynamictopology=0;normals=0;uvs=0;facesets=0"
	else:
		job += ";purepointcache=0;uvs="+str(int(exUVs))
		job += ";facesets=" #+str(int(exFaceSets)) # move this in the if/else just below
		if exFaceSets:
			job += "1;useInitShadGrp=" + str(int(exUseInitShadGrp))
		else:
			job += "0"

		if exTopology == 2:
			job += ";normals=0;dynamictopology=0"
		else:
			job += ";normals=1;dynamictopology="+str(int(exDynTopo))

	job += ";globalspace="+str(int(exGlobSpace))
	job += ";withouthierarchy="+str(int(exWithoutHierarchy))
	job += ";transformcache="+str(int(exXformCache))

	cmds.ExocortexAlembic_export(j=job)
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._export.doIt")

