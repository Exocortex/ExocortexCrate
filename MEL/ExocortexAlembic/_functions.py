import maya.cmds as cmds

""" Contains functions and data structures """

class AlembicInfo:
	""" A structure to hold information about all objects in the file! """
	nbTransforms = 0
	def __init__(self, infoTokens):
		self.identifier = infoTokens[0]
		self.type = infoTokens[1]
		if self.type == "Xform":
			AlembicInfo.nbTransforms = AlembicInfo.nbTransforms + 1
		self.name = infoTokens[2]
		self.nbSamples = int(infoTokens[3])
		self.pID = int(infoTokens[4])
		self.childIDs = infoTokens[5]
		self.constant = bool(infoTokens[6])
		self.data = ""
		if len(infoTokens) > 7:
			self.data = infoTokens[7]
		self.object = ""

	def __str__(self):
		return "AI[i:" + str(self.identifier) + ", t:" + str(self.type) + ", n:" + str(self.name) + "]"

	def __repr__(self):
		return str(self)

############################################################################################################
# general functions needed to import and attach
############################################################################################################
def alembicTimeAndFileNode(filename):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._functions.alembicTimeAndFileNode")
	print("time control")
	timeControl = "AlembicTimeControl"
	if not cmds.objExists(timeControl):
		timeControl = cmds.createNode("ExocortexAlembicTimeControl", name=timeControl)
		alembicConnectAttr("time1.outTime", timeControl+".inTime")
	print("file node")
	fileNode = cmds.createNode("ExocortexAlembicFile")
	cmds.setAttr(fileNode+".fileName", filename, type="string")
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._functions.alembicTimeAndFileNode")
	return fileNode, timeControl

def alembicCreateNode(name, type, parentXform=None):
	""" create a node and make sure to return a full name and create namespaces if necessary! """
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._functions.createAlembicNode")

	# create namespaces if necessary!
	tokens = name.split(":")
	if len(tokens) > 1:
		# first one!
		accum = ":" + tokens[0]
		if not cmds.namespace(exists=accum):
			cmds.namespace(add=tokens[0])
		# rest...
		for nspace in tokens[1:-1]:
			curAccum = accum + ":" + nspace
			if not cmds.namespace(exists=curAccum):
				cmds.namespace(add=nspace, p=accum)
			accum = curAccum

	# create the node
	result = cmds.createNode(type, n=name, p=parentXform)
	#tokens = result.split("|")
	#if len(tokens) == 1:
	#	pp = cmds.listRelatives(result, p=True)
	#	if len(pp) > 0:
	#		result = pp[0] + "|" + result
	if parentXform != None:
		result = parentXform + "|" + result
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._functions.createAlembicNode")
	return result

def alembicConnectAttr(source, target):
	""" make sure nothing is connected the target and then connect the source to the target """
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._functions.alembicConnectIfUnconnected")
	currentSource = cmds.connectionInfo(target, sfd=True)
	if currentSource != "" and currentSource != source:
		cmds.disconnectAttr(currentSource, target)
	cmds.connectAttr(source, target)
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._functions.alembicConnectIfUnconnected")
	pass

def connectShapeAndReader(shape, reader):
	cmds.connectAttr(reader+".translate", 	shape+".translate")
	cmds.connectAttr(reader+".rotate", 		shape+".rotate")
	cmds.connectAttr(reader+".scale", 		shape+".scale")

############################################################################################################
# import functions
############################################################################################################
def setupReaderAttribute(reader, identifier, isConstant, fileTimeCtrl):
	if not isConstant:
		alembicConnectAttr(fileTimeCtrl[1]+".outTime", reader+".inTime")
	alembicConnectAttr(fileTimeCtrl[0]+".outFileName", reader+".fileName")
	cmds.setAttr(reader+".identifier", identifier, type="string")

def importXform(name, identifier, fileTimeCtrl, parentXform=None, isConstant=False):
	shape 	= alembicCreateNode(name, "transform", parentXform)
	reader 	= cmds.createNode("ExocortexAlembicXform")
	connectShapeAndReader(shape, reader)
	setupReaderAttribute(reader, identifier, isConstant, fileTimeCtrl)
	return shape

def importPolyMesh(name, identifier, fileTimeCtrl, parentXform=None, isConstant=False, useDynTopo=False, useFaceSets=False, useNormals=False, useUVs=False):
	reader = ""
	shape = alembicCreateNode(name, "mesh", parentXform)
	cmds.sets(shape, e=True, forceElement="initialShadingGroup")
	if useDynTopo:
		reader = alembicCreateNode(name+".inMesh","ExocortexAlembicPolyMesh")
	elif not isConstant:
		reader = alembicCreateNode(name+".inMesh","ExocortexAlembicPolyMeshDeform")

	if reader == "":
		topoReader = cmds.createNode("ExocortexAlembicPolyMesh")
		fnt.alembicConnectAttr(topoReader+".outMesh", shape+".inMesh")
		fnt.alembicConnectAttr(fileTimeCtrl[0]+".outFileName", topoReader+".fileName")
		cmds.setAttr(topoReader+".identifier", identifier, type="string")
        cmds.setAttr(topoReader+".normals", useNormals)
        cmds.setAttr(topoReader+".uvs", useUVs)
        if useFaceSets:
        	cmds.ExocortexAlembic_createFaceSets(o=shape, f=fileTimeCtrl[0], i=identifier)

		if useDynTopo:
			reader = topoReader
		elif not isConstant:
			reader = cmds.deformer(shape, type="ExocortexAlembicPolyMeshDeform")[0]

	setupReaderAttribute(reader, identifier, isConstant, fileTimeCtrl)
	return shape




