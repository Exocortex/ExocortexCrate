import maya.cmds as cmds
import _functions as fnt

""" Import module of Exocortex Crate """

############################################################################################################
# import classes
############################################################################################################
class IJobInfo:
	def __init__(self, _filename, useNormals=False, useUVs=True, useFaceSets=True):
		fileTime = fnt.alembicTimeAndFileNode(_filename)
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
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.setupReaderAttribute")
	if reader != "":
		if not isConstant:
			fnt.alembicConnectAttr(jobInfo.timeCtrl+".outTime", reader+".inTime")
		fnt.alembicConnectAttr(jobInfo.filenode+".outFileName", reader+".fileName")
		cmds.setAttr(reader+".identifier", identifier, type="string")
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.setupReaderAttribute")

def importXform(name, identifier, jobInfo, parentXform=None, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.importXform")
	shape  = fnt.alembicCreateNode(name, "transform", parentXform)
	reader = cmds.createNode("ExocortexAlembicXform")

	cmds.connectAttr(reader+".translate", 	shape+".translate")
	cmds.connectAttr(reader+".rotate", 		shape+".rotate")
	cmds.connectAttr(reader+".scale", 		shape+".scale")

	setupReaderAttribute(reader, identifier, isConstant, jobInfo)
	#print("importXform(" + str(name) + ", " + str(identifier) + ", " + str(jobInfo) + ", " + str(parentXform) + ", " + str(isConstant) + ") -> " + str(shape))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.importXform")
	return shape

def importPolyMesh(name, identifier, jobInfo, parentXform=None, isConstant=False, useDynTopo=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.importPolyMesh")
	reader = ""
	shape  = fnt.alembicCreateNode(name, "mesh", parentXform)
	cmds.sets(shape, e=True, forceElement="initialShadingGroup")

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
	#print("importPolyMesh(" + str(name) + ", " + str(identifier) + ", " + str(parentXform) + ") -> " + str(shape))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.importPolyMesh")
	return shape

def importCamera(name, identifier, jobInfo, parentXform=None, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.importCamera")
	shape 	= fnt.alembicCreateNode(name, "camera", parentXform)
	reader 	= cmds.createNode("ExocortexAlembicCamera")

	cmds.connectAttr(reader+".focalLength", shape+".focalLength")
	cmds.connectAttr(reader+".focusDistance", shape+".focusDistance")
	cmds.connectAttr(reader+".lensSqueezeRatio", shape+".lensSqueezeRatio")
	cmds.connectAttr(reader+".horizontalFilmAperture", shape+".horizontalFilmAperture")
	cmds.connectAttr(reader+".verticalFilmAperture", shape+".verticalFilmAperture")
	cmds.connectAttr(reader+".horizontalFilmOffset", shape+".horizontalFilmOffset")
	cmds.connectAttr(reader+".verticalFilmOffset", shape+".verticalFilmOffset")
	cmds.connectAttr(reader+".fStop", shape+".fStop")
	cmds.connectAttr(reader+".shutterAngle", shape+".shutterAngle")

	setupReaderAttribute(reader, identifier, isConstant, jobInfo)
	#print("importCamera(" + str(name) + ", " + str(identifier) + ", " + str(jobInfo) + ", " + str(parentXform) + ", " + str(isConstant) + ") -> " + str(shape))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.importCamera")
	return shape

def importPoints(name, identifier, jobInfo, parentXform=None, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.importPoints")
	shape  = fnt.alembicCreateNode(name, "particle", parentXform)
	reader = cmds.createNode("ExocortexAlembicPoints")

	cmds.addAttr(shape, ln="rgbPP", dt="vectorArray")
	cmds.addAttr(shape, ln="opacityPP", dt="doubleArray")
	cmds.addAttr(shape, ln="agePP", dt="doubleArray")
	cmds.addAttr(shape, ln="shapeInstanceIdPP", dt="doubleArray")
	cmds.addAttr(shape, ln="orientationPP", dt="vectorArray")
	cmds.connectAttr(reader+".output[0]", shape+".newParticles[0]")
	cmds.connectAttr(jobInfo.timeCtrl+".outTime", shape+".currentTime")
	cmds.setAttr(shape+".conserve", 0)

	setupReaderAttribute(reader, identifier, isConstant, jobInfo)
	#print("importPoints(" + str(name) + ", " + str(identifier) + ", " + str(jobInfo) + ", " + str(parentXform) + ", " + str(isConstant) + ") -> " + str(shape))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.importPoints")
	return shape

def importCurves(name, identifier, jobInfo, parentXform=None, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.importCurves")
	shape  = fnt.alembicCreateNode(name, "nurbsCurve", parentXform)
	reader = cmds.deformer(shape, type="ExocortexAlembicCurvesDeform")[0]

	topoReader = cmds.createNode("ExocortexAlembicCurves")
	cmds.connectAttr(topoReader+".outCurve", shape+".create")
	cmds.connectAttr(jobInfo.filenode+".outFileName", topoReader+".fileName")
	cmds.setAttr(topoReader+".identifier", identifier, type="string")

	setupReaderAttribute(reader, identifier, isConstant, jobInfo)
	#print("importCurves(" + str(name) + ", " + str(identifier) + ", " + str(jobInfo) + ", " + str(parentXform) + ", " + str(isConstant) + ") -> " + str(shape))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.importCurves")
	return shape


