import traceback

import maya.cmds as cmds
import maya.OpenMayaMPx as apix
import _functions as fnt

""" Import module of Exocortex Crate """

############################################################################################################
# import classes
############################################################################################################
class IJobInfo:
	def __init__(self, _filename, useNormals=False, useUVs=True, useFaceSets=True, multi=False):
		fileTime = fnt.alembicTimeAndFileNode(_filename, multi)
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
	try:
		if reader != "":
			if not isConstant:
				fnt.alembicConnectAttr(jobInfo.timeCtrl+".outTime", reader+".inTime")
			cmds.connectAttr(jobInfo.filenode+".outFileName", reader+".fileName")
			cmds.setAttr(reader+".identifier", identifier, type="string")
	except:
		raise
	finally:
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.setupReaderAttribute")

def importXform(name, identifier, jobInfo, parentXform=None, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.importXform")

	# TODO: set isConstant properly elsewhere when there are no transforms but are
	# animated attributes
	isConstant = False
	try:
		shape  = fnt.alembicCreateNode(name, "transform", parentXform)
		reader = cmds.createNode("ExocortexAlembicXform")

		cmds.connectAttr(reader+".translate", 		shape+".translate")
		cmds.connectAttr(reader+".rotate", 			shape+".rotate")
		cmds.connectAttr(reader+".scale", 			shape+".scale")
		cmds.connectAttr(reader+".outVisibility",	shape+".visibility")

		setupReaderAttribute(reader, identifier, isConstant, jobInfo)
	except:
		return [traceback.format_exc()]
	finally:
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.importXform")
	return [shape, reader]

def importPolyMesh(name, identifier, jobInfo, parentXform=None, isConstant=False, useDynTopo=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.importPolyMesh")

	# TODO: set isConstant properly elsewhere when there are no transforms but are
	# animated attributes
	isConstant = False
	try:
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
			cmds.connectAttr(jobInfo.timeCtrl+".outTime", topoReader+".inTime")
			reader = topoReader
		elif not isConstant:
			reader = cmds.deformer(shape, type="ExocortexAlembicPolyMeshDeform")[0]
			setupReaderAttribute(reader, identifier, isConstant, jobInfo)

		#if not useDynTopo:
		#	setupReaderAttribute(topoReader, identifier, isConstant, jobInfo)

	except:
		return [traceback.format_exc()]
	finally:
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.importPolyMesh")
	return [shape, reader]

def importCamera(name, identifier, jobInfo, parentXform=None, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.importCamera")
	# TODO: set isConstant properly elsewhere when there are no transforms but are
	# animated attributes
	isConstant = False
	try:
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
	except:
		return [traceback.format_exc()]
	finally:
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.importCamera")
	return [shape, reader]

def importPoints(name, identifier, jobInfo, parentXform=None, isConstant=False):
	# TODO: set isConstant properly elsewhere when there are no transforms but are
	# animated attributes
	isConstant = False
	try:
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
	except:
		return [traceback.format_exc()]
	finally:
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.importPoints")
	return [shape, reader]

def importCurves(name, identifier, jobInfo, parentXform=None, isConstant=False, nbCurves=1):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.importCurves")
	# TODO: set isConstant properly elsewhere when there are no transforms but are
	# animated attributes
	isConstant = False
	try:
		topoReader = cmds.createNode("ExocortexAlembicCurves")
		cmds.connectAttr(jobInfo.filenode+".outFileName", topoReader+".fileName")
		cmds.connectAttr(jobInfo.timeCtrl+".outTime", topoReader+".inTime")
		cmds.setAttr(topoReader+".identifier", identifier, type="string")

		shape  = fnt.alembicCreateNode(name, "nurbsCurve", parentXform)
		cmds.connectAttr(topoReader+".outCurve[0]", shape+".create")
		for curve in xrange(1, nbCurves):
			shape  = fnt.alembicCreateNode(name + "_" + str(curve), "nurbsCurve", parentXform)
			cmds.connectAttr(topoReader+".outCurve[" + str(curve) + "]", shape+".create")

	except:
		return [traceback.format_exc()]
	finally:
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.importCurves")
	return [shape, topoReader]


