import traceback

import maya.cmds as cmds
import _functions as fnt

"""  Attach to existing """

def attachTimeAndFile(node, jobInfo, isConstant=False):
	connAttr = cmds.connectionInfo(node+".inTime", sfd=True)
	if connAttr == "" and not isConstant:
		cmds.connectAttr(jobInfo.timeCtrl+".outTime", node+".inTime")

	node = node + ".fileName"	# compute once, used 2-3 times
	connAttr = cmds.connectionInfo(node, sfd=True)
	if connAttr != None and connAttr != "":
		cmds.disconnectAttr(connAttr, node)
	cmds.connectAttr(jobInfo.filenode+".outFileName", node)
	pass

def attachXform(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachXform")
	try:
		conX = cmds.listConnections(name+".translate")
		if conX:
			# already receiving transformation from another node!
			conX = conX[0]
			if cmds.objectType(conX) == "ExocortexAlembicXform":
				attachTimeAndFile(conX, jobInfo, isConstant)
				return [conX]
			else:
				return ["!", "Cannot attach Xform to " + name + ", it's attach to a node that is not an \"ExocortexAlembicXform\""]

		newXform = cmds.createNode("ExocortexAlembicXform")
		cmds.setAttr(newXform+".identifier", identifier, type="string")
		cmds.connectAttr(newXform+".translate", 	name+".translate")
		cmds.connectAttr(newXform+".rotate",		name+".rotate")
		cmds.connectAttr(newXform+".scale", 		name+".scale")
		cmds.connectAttr(newXform+".outVisibility",	name+".visibility")

		attachTimeAndFile(newXform, jobInfo, isConstant)
	except:
		return ["!", traceback.format_exc()]
	finally:
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachXform")
	return [newXform]

def attachPolyMesh(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachPolyMesh")
	try:
		if cmds.objectType(name) != "mesh":
			return ["!", "Only mesh can be attached too!"]

		conX = cmds.listConnections(name, d=False, type="ExocortexAlembicPolyMeshDeform")
		if conX: # it's already attached to a deform, simply change the file reference
			polyObj = conX[0]
			attachTimeAndFile(polyObj, jobInfo, isConstant)
			return [polyObj]

		# create deformer, and attach time and file
		newDform = cmds.deformer(name, type="ExocortexAlembicPolyMeshDeform")[0]
		cmds.setAttr(newDform+".identifier", identifier, type="string")
		attachTimeAndFile(newDform, jobInfo, isConstant)

		if jobInfo.useFaceSets:
			cmds.ExocortexAlembic_createFaceSets(f=cmds.getAttr(jobInfo.filenode+".outFileName"), i=identifier, o=name)
	except:
		return ["!", traceback.format_exc()]
	finally:
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachPolyMesh")
	return [newDform]

def attachCamera(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachCamera")
	try:
		conX = cmds.listConnections(name, d=False, type="ExocortexAlembicCamera")
		if conX:
			camObj = conX[0]
			attachTimeAndFile(camObj, jobInfo, isConstant)
			return [camObj]

		reader = cmds.createNode("ExocortexAlembicCamera")
		cmds.connectAttr(reader+".focalLength", name+".focalLength")
		cmds.connectAttr(reader+".focusDistance", name+".focusDistance")
		cmds.connectAttr(reader+".lensSqueezeRatio", name+".lensSqueezeRatio")
		cmds.connectAttr(reader+".horizontalFilmAperture", name+".horizontalFilmAperture")
		cmds.connectAttr(reader+".verticalFilmAperture", name+".verticalFilmAperture")
		cmds.connectAttr(reader+".horizontalFilmOffset", name+".horizontalFilmOffset")
		cmds.connectAttr(reader+".verticalFilmOffset", name+".verticalFilmOffset")
		cmds.connectAttr(reader+".fStop", name+".fStop")
		cmds.connectAttr(reader+".shutterAngle", name+".shutterAngle")

		attachTimeAndFile(reader, jobInfo, isConstant)
	except:
		return ["!", traceback.format_exc()]
	finally:
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachCamera")
	return [reader]

def attachCurves(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachCurves")
	try:
		conX = (cmds.listConnections(name+".create", d=False, type="ExocortexAlembicCurvesDeform") or
				cmds.listConnections(name+".create", d=False, type="ExocortexAlembicCurves"))
		if conX:
			curObj = conX[0]
			attachTimeAndFile(curObj, jobInfo, isConstant)
			return [curObj]

		# create deformer, and attach time and file
		newDform = cmds.deformer(name, type="ExocortexAlembicCurvesDeform")[0]
		cmds.setAttr(newDform+".identifier", identifier, type="string")
		attachTimeAndFile(newDform, jobInfo, isConstant)

		# get curObj new "output" attribute connection
		conX = cmds.listConnections(name+".create", d=False, type="ExocortexAlembicCurvesDeform")
		if conX:
			curObj = conX[0]
			originalCur = cmds.connectionInfo(curObj+".output", sfd=True).split('.')[0]

			cmds.delete(curObj)
			curObj = cmds.createNode("ExocortexAlembicCurves")
			attachTimeAndFile(curObj, jobInfo, isConstant)

			cmds.connectAttr(curObj+".outCurve", originalCur+".create")
			cmds.connectAttr(jobInfo.filenode+".outFileName", curObj+".fileName")
			cmds.setAttr(curObj+".identifier", identifier, type="string")
	except:
		return ["!", traceback.format_exc()]
	finally:
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachCurves")
	return [curObj]

def attachPoints(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachPoints")
	try:
		conX = cmds.listConnections(name, d=False, type="ExocortexAlembicPoints")
		if conX:
			ptsObj = conX[0]
			attachTimeAndFile(ptsObj, jobInfo, isConstant)
			return [ptsObj]

		reader = cmds.createNode("ExocortexAlembicPoints")
		cmds.addAttr(name, ln="rgbPP", dt="vectorArray")
		cmds.addAttr(name, ln="opacityPP", dt="doubleArray")
		cmds.addAttr(name, ln="agePP", dt="doubleArray")
		cmds.addAttr(name, ln="shapeInstanceIdPP", dt="doubleArray")
		cmds.addAttr(name, ln="orientationPP", dt="vectorArray")
		cmds.connectAttr(reader+".output[0]", name+".newParticles[0]")
		cmds.connectAttr(jobInfo.timeCtrl+".outTime", name+".currentTime")
		cmds.setAttr(name+".conserve", 0)

		attachTimeAndFile(reader, jobInfo, isConstant)
	except:
		return ["!", traceback.format_exc()]
	finally:
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachPoints")
	return [reader]

