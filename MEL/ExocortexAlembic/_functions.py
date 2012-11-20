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
	if parentXform != None:
		result = parentXform + "|" + result
	#print("alembicCreateNode(" + str(name) + ", " + str(type) + ", " + str(parentXform) + ") -> " + str(result))
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
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._functions.connectShapeAndReader")
	cmds.connectAttr(reader+".translate", 	shape+".translate")
	cmds.connectAttr(reader+".rotate", 		shape+".rotate")
	cmds.connectAttr(reader+".scale", 		shape+".scale")
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._functions.connectShapeAndReader")

############################################################################################################
# import classes
############################################################################################################
class IJobInfo:
	def __init__(self, _filename, useNormals=False, useUVs=True, useFaceSets=True):
		fileTime = alembicTimeAndFileNode(_filename)
		self.filename = _filename
		self.filenode = fileTime[0]
		self.timeCtrl = fileTime[1]
		self.useNormals = useNormals
		self.useUVs = useUVs
		self.useFaceSets = useFaceSets
		pass

############################################################################################################
# import functions
############################################################################################################
def setupReaderAttribute(reader, identifier, isConstant, jobInfo):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._functions.setupReaderAttribute")
	if reader != "":
		if not isConstant:
			alembicConnectAttr(jobInfo.timeCtrl+".outTime", reader+".inTime")
		alembicConnectAttr(jobInfo.filenode+".outFileName", reader+".fileName")
		cmds.setAttr(reader+".identifier", identifier, type="string")
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._functions.setupReaderAttribute")

def importXform(name, identifier, jobInfo, parentXform=None, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._functions.importXform")
	shape  = alembicCreateNode(name, "transform", parentXform)
	reader = cmds.createNode("ExocortexAlembicXform")
	connectShapeAndReader(shape, reader)
	setupReaderAttribute(reader, identifier, isConstant, jobInfo)
	#print("importXform(" + str(name) + ", " + str(identifier) + ", " + str(jobInfo) + ", " + str(parentXform) + ", " + str(isConstant) + ") -> " + str(shape))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._functions.importXform")
	return shape

def importPolyMesh(name, identifier, jobInfo, parentXform=None, isConstant=False, useDynTopo=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._functions.importPolyMesh")
	reader = ""
	shape  = alembicCreateNode(name, "mesh", parentXform)
	cmds.sets(shape, e=True, forceElement="initialShadingGroup")
	#if useDynTopo:
	#	reader = alembicCreateNode(name+".inMesh","ExocortexAlembicPolyMesh")
	#elif not isConstant:
	#	reader = alembicCreateNode(name+".inMesh","ExocortexAlembicPolyMeshDeform")

	#if reader == "":
	topoReader = cmds.createNode("ExocortexAlembicPolyMesh")
	cmds.connectAttr(topoReader+".outMesh", shape+".inMesh")
	cmds.connectAttr(jobInfo.filenode+".outFileName", topoReader+".fileName")
	cmds.setAttr(topoReader+".identifier", identifier, type="string")
	cmds.setAttr(topoReader+".normals", jobInfo.useNormals)
	cmds.setAttr(topoReader+".uvs", jobInfo.useUVs)
	if jobInfo.useFaceSets:
		cmds.ExocortexAlembic_createFaceSets(o=shape, f=jobInfo.filename, i=identifier)
	if useDynTopo:
		reader = topoReader
	elif not isConstant:
		reader = cmds.deformer(shape, type="ExocortexAlembicPolyMeshDeform")[0]

	setupReaderAttribute(reader, identifier, isConstant, jobInfo)
	#print("importPolyMesh(" + str(name) + ", " + str(identifier) + ", " + str(jobInfo) + ", " + str(parentXform) + ", " + str(isConstant) + ", " + str(useDynTopo) + ", " + str(useFaceSets) + ", " + str(useNormals) + ", " + str(useUVs) + ") -> " + str(shape))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._functions.importPolyMesh")
	return shape

def importCamera(name, identifier, jobInfo, parentXform=None, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._functions.importCamera")
	shape 	= alembicCreateNode(name, "camera", parentXform)
	reader 	= cmds.createNode("ExocortexAlembicCamera")

	cmds.connectAttr((reader+".focalLength"), (shape+".focalLength"))
	cmds.connectAttr((reader+".focusDistance"), (shape+".focusDistance"))
	cmds.connectAttr((reader+".lensSqueezeRatio"), (shape+".lensSqueezeRatio"))
	cmds.connectAttr((reader+".horizontalFilmAperture"), (shape+".horizontalFilmAperture"))
	cmds.connectAttr((reader+".verticalFilmAperture"), (shape+".verticalFilmAperture"))
	cmds.connectAttr((reader+".horizontalFilmOffset"), (shape+".horizontalFilmOffset"))
	cmds.connectAttr((reader+".verticalFilmOffset"), (shape+".verticalFilmOffset"))
	cmds.connectAttr((reader+".fStop"), (shape+".fStop"))
	cmds.connectAttr((reader+".shutterAngle"), (shape+".shutterAngle"))

	setupReaderAttribute(reader, identifier, isConstant, jobInfo)
	#print("importCamera(" + str(name) + ", " + str(identifier) + ", " + str(jobInfo) + ", " + str(parentXform) + ", " + str(isConstant) + ") -> " + str(shape))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._functions.importCamera")
	return shape

def importPoints(name, identifier, jobInfo, parentXform=None, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._functions.importPoints")
	shape  = alembicCreateNode(name, "particle", parentXform)
	reader = cmds.createNode("ExocortexAlembicPoints")

	cmds.addAttr(shape, ln="rgbPP", dt="vectorArray")
	cmds.addAttr(shape, ln="opacityPP", dt="doubleArray")
	cmds.addAttr(shape, ln="agePP", dt="doubleArray")
	cmds.addAttr(shape, ln="shapeInstanceIdPP", dt="doubleArray")
	cmds.addAttr(shape, ln="orientationPP", dt="vectorArray")
	cmds.connectAttr((reader+".output[0]"), (shape+".newParticles[0]"))
	cmds.connectAttr((jobInfo.timeCtrl+".outTime"), (shape+".currentTime"))
	cmds.setAttr(shape+".conserve", 0)

	setupReaderAttribute(reader, identifier, isConstant, jobInfo)
	#print("importPoints(" + str(name) + ", " + str(identifier) + ", " + str(jobInfo) + ", " + str(parentXform) + ", " + str(isConstant) + ") -> " + str(shape))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._functions.importPoints")
	return shape


