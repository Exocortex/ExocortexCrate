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
	result = "No error"
	try:
		conX = cmds.listConnections(name+".translate")
		if conX:
			# already receiving transformation from another node!
			conX = conX[0]
			if cmds.objectType(conX) == "ExocortexAlembicXform":
				attachTimeAndFile(conX, jobInfo, isConstant)
				return "Attached existing"
			else:
				return "Cannot attach Xform to " + name + ", it's attach to a node that is not an \"ExocortexAlembicXform\""

		newXform = cmds.createNode("ExocortexAlembicXform")
		cmds.setAttr(newXform+".identifier", identifier, type="string")
		cmds.connectAttr(newXform+".translate", 	name+".translate")
		cmds.connectAttr(newXform+".rotate",		name+".rotate")
		cmds.connectAttr(newXform+".scale", 		name+".scale")
		cmds.connectAttr(newXform+".outVisibility",	name+".visibility")

		attachTimeAndFile(newXform, jobInfo, isConstant)
	except:
		import traceback
		result = traceback.format_exc()

	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachXform")
	return result

def attachPolyMesh(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachPolyMesh")
	result = "No error"
	try:
		if not cmds.objExists(name) and cmds.objExists(name + "Deformed"):
			name = name + "Deformed"

		if cmds.objectType(name) != "mesh":
			cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachPolyMesh")
			return "Only mesh can be attached too!"

		polyObj = cmds.connectionInfo(name+".inMesh", sfd=True).split('.')[0]					# cmds.plugNode doesn't exist!
		if polyObj and cmds.objectType(polyObj) == "ExocortexAlembicPolyMeshDeform":	# it's already attached to a deform, simply change the file reference
			attachTimeAndFile(polyObj, jobInfo, isConstant)
			cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachPolyMesh")
			return "Connected existing"

		# create deformer, and attach time and file
		newDform = cmds.deformer(name, type="ExocortexAlembicPolyMeshDeform")[0]
		cmds.setAttr(newDform+".identifier", identifier, type="string")
		attachTimeAndFile(newDform, jobInfo, isConstant)

		if jobInfo.useFaceSets:
			cmds.ExocortexAlembic_createFaceSets(f=cmds.getAttr(jobInfo.filenode+".outFileName"), i=identifier, o=name)
	except:
		import traceback
		result = traceback.format_exc()

	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachPolyMesh")
	return result

def attachCamera(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachCamera")
	result = "No error"
	try:
		camObj = cmds.connectionInfo(name+".focalLength", sfd=True)
		if camObj and cmds.objectType(camObj) == "ExocortexAlembicCamera":
			attachTimeAndFile(camObj, jobInfo, isConstant)
			return "Attached existing"

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
		import traceback
		result = traceback.format_exc()

	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachCamera")
	return result

def attachCurves(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachCurves")
	result = "No error"
	try:
		curObj = cmds.connectionInfo(name+".visibility", sfd=True)
		if curObj and cmds.objectType(curObj) == "ExocortexAlembicCurvesDeform":
			attachTimeAndFile(curObj, jobInfo, isConstant)
			return "Attached existing"

		# create deformer, and attach time and file
		newDform = cmds.deformer(name, type="ExocortexAlembicCurvesDeform")[0]
		cmds.setAttr(newDform+".identifier", identifier, type="string")
		attachTimeAndFile(newDform, jobInfo, isConstant)

		# get curObj new "output" attribute connection
		if curObj and cmds.objectType(curObj) != "ExocortexAlembicCurves":
			originalCur = cmds.connectionInfo(curObj+".output", sfd=True).split('.')[0]

			cmds.delete(curObj)
			curObj = cmds.createNode("ExocortexAlembicCurves")
			attachTimeAndFile(curObj, jobInfo, isConstant)

		cmds.connectAttr(curObj+".outCurve", originalCur+".create")
		cmds.connectAttr(jobInfo.filenode+".outFileName", curObj+".fileName")
		cmds.setAttr(curObj+".identifier", identifier, type="string")
	except:
		import traceback
		result = traceback.format_exc()

	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachCurves")
	return result

def attachPoints(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachPoints")
	result = "No error"
	try:
		ptsObj = cmds.connectionInfo(name+".visibility", sfd=True)
		if ptsObj and cmds.objectType(ptsObj) == "ExocortexAlembicPoints":
			attachTimeAndFile(ptsObj, jobInfo, isConstant)
			return "Attached existing"

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
		import traceback
		result = traceback.format_exc()
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachPoints")
	return result

