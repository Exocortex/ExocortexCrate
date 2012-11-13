import maya.cmds as cmds
import _functions as fnt

""" Import module of Exocortex Crate """

def fillAlembicInfoList(filename):
	alembicInfos = []
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.fillAlembicInfoList")
	fnt.AlembicInfo.nbTransforms = 0
	for info in cmds.ExocortexAlembic_getInfo(f=filename):
		alembicInfos.append(fnt.AlembicInfo(info.split("|")))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.fillAlembicInfoList")
	return alembicInfos

def connectShapeAndReader(shape, reader):
	cmds.connectAttr(reader+".translate", 	shape+".translate")
	cmds.connectAttr(reader+".rotate", 		shape+".rotate")
	cmds.connectAttr(reader+".scale", 		shape+".scale")

def doPolyMesh(curObj, dynamictopology, doItParam, fileNode):
	""" import a polymesh object """
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.doPolyMesh")
	reader = ""
	topoReader = ""
	shape = fnt.alembicCreateNode(curObj.name, "mesh")
	cmds.sets(shape, e=True, forceElement="initialShadingGroup")
	if dynamictopology:
		reader = fnt.alembicCreateNode(curObj.name+".inMesh","ExocortexAlembicPolyMesh")
	elif not curObj.constant:
		reader = fnt.alembicCreateNode(curObj.name+".inMesh","ExocortexAlembicPolyMeshDeform")

	if reader == "":
		topoReader = cmds.createNode("ExocortexAlembicPolyMesh")
		fnt.alembicConnectAttr(topoReader+".outMesh", shape+".inMesh")
		fnt.alembicConnectAttr(fileNode+".outFileName", topoReader+".fileName")
		cmds.setAttr(topoReader+".identifier", curObj.identifier, type="string")
        cmds.setAttr(topoReader+".normals", doItParam[1])
        cmds.setAttr(topoReader+".uvs", doItParam[2])
        #if doItParam[3]:
        	#cmds.ExocortexAlembic_createFaceSets(o=shape, f=doItParam[0], i=curObj.identifier)
        if dynamictopology:
        	reader = topoReader

		if reader == "" and not curObj.constant:
			reader = cmds.deformer(shape, type="ExocortexAlembicPolyMeshDeform")[0]

	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.doPolyMesh")
	return reader, topoReader, shape

def doXform(curObj, alembicInfos, doItParam, fileNode):
	""" import an Xform """
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.doXform")
	reader = ""
	shape = ""
	if len(alembicInfos) == fnt.AlembicInfo.nbTransforms:
		locator = cmds.createNode("locator")
		shape = cmds.listRelatives(locator, p=True)[0]
		reader = cmds.createNode("ExocortexAlembicXform")
		connectShapeAndReader(shape, reader)
	else:
		cID = int(curObj.childIDs.split(".")[0])-1
		if cID >= 0:
			childObj = alembicInfos[cID]
			name = curObj.name
			if name != "front" and name != "top" and name != "side" and name != "persp":
				xform = cmds.listRelatives(childObj.object, p=True)
				shape 	= cmds.rename(xform, name, ignoreShape=True)
				reader 	= cmds.createNode("ExocortexAlembicXform")
				connectShapeAndReader(shape, reader)

	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.doXform")
	return reader, shape

def doGroup(curObj, alembicInfos, doItParam, fileNode):
	""" import an Xform with other Xforms as children """
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.doGroup")
	reader = ""
	shape = ""
	if len(alembicInfos) == fnt.AlembicInfo.nbTransforms:
		shape = cmds.listRelatives(cmds.createNode("locator"), p=True)[0]
		shape = cmds.rename(shape, curObj.name, ignoreShape=True)
		reader = cmds.createNode("ExocortexAlembicXform")
	else:
		shape = fnt.alembicCreateNode(curObj.name, "transform")
        reader = cmds.createNode("ExocortexAlembicXform")
	connectShapeAndReader(shape, reader)

	for scID in curObj.childIDs.split("."):
		childObj = alembicInfos[int(scID)-1]
		cObj = childObj.object
		if cObj == "" or cObj == shape:
			continue
		if childObj.type == "Xform" or childObj.type == "Group":
			cmds.parent(cObj, shape)
		else:
			cmds.parent(cObj, shape, shape=True)
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.doGroup")
	return reader, shape

def doIt(filename, importNormals=False, importUvs=True, importFaceSets=True):
	"""
	Creates the right nodes for each object in the Alembic file!
	"""
	gMainProgressBar = "MayaWindow|toolBar3|MainHelpLineLayout|helpLineFrame|formLayout16|mainProgressBar"
	doItParam = (filename, importNormals, importUvs, importFaceSets)

	# initialization!
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.doIt")
	cmds.ExocortexAlembic_profileReset()
	cmds.ExocortexAlembic_fileRefCount(i=filename)

	# fill the list
	alembicInfos = fillAlembicInfoList(filename)
	fileNode, timeControl = fnt.alembicTimeAndFileNode(filename)
	cmds.progressBar(gMainProgressBar, e=True, bp=True, ii=1, max=len(alembicInfos))

	# for each each, starting with the last one!
	for ii in xrange(len(alembicInfos)-1, -1, -1):
		if cmds.progressBar(gMainProgressBar, q=True, ic=True):
			print("Import interrupted by the user")
			break
		cmds.progressBar(gMainProgressBar, e=True, s=1)

		curObj = alembicInfos[ii]
		reader = ""
		topoReader = ""
		shape = ""

		# parse additional data options
		purepointcache = False
		dynamictopology = False
		hair = False
		for dt in curObj.data.split(";"):
			if dt == "purepointcache=1":
				purepointcache = True
			elif dt =="dynamictopology=1":
				dynamictopology = True
			elif dt =="hair=1":
				hair = True

		# import according to the type
		if curObj.type == "Xform":
			reader, shape = doXform(curObj, alembicInfos, doItParam, fileNode)
		elif curObj.type == "PolyMesh":
			reader, topoReader, shape = doPolyMesh(curObj, dynamictopology, doItParam, fileNode)
		elif curObj.type == "Group":
			reader, shape = doGroup(curObj, alembicInfos, doItParam, fileNode)
		elif curObj.type == "SubD":
			print("Type " + curObj.type + " not supported yet!")
		elif curObj.type == "Curves":
			print("Type " + curObj.type + " not supported yet!")
		elif curObj.type == "Points":
			print("Type " + curObj.type + " not supported yet!")
		elif curObj.type == "Camera":
			print("Type " + curObj.type + " not supported yet!")
		elif curObj.type == "Unknown":
			pass
		else:
			print("Invalid object type for object "+ curObj.name +"\n")
			continue

		# finalize
		cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.doIt:finalize")
		if reader != "":
			if curObj.constant:
				fnt.alembicConnectAttr(timeControl+".outTime", reader+".inTime")
			fnt.alembicConnectAttr(fileNode+".outFileName", reader+".fileName")
			cmds.setAttr(reader+".identifier", curObj.identifier, type="string")
		curObj.object = shape
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.doIt:finalize")

		# setup metadata if we have it
		#cmds.ExocortexAlembic_createMetaData(f=filename, i=curObj.identifier, o=shape)

	# finalization!
	cmds.progressBar(gMainProgressBar, e=True, endProgress=True)
	cmds.ExocortexAlembic_postImportPoints()
	cmds.ExocortexAlembic_fileRefCount(d=filename)
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.doIt")
	pass


