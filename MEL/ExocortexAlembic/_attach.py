import maya.cmds as cmds
import _functions as fnt

"""  Attach to existing """

def attachTimeAndFile(node, jobInfo, isConstant=False):
	connAttr = cmds.connectionInfo(node+".inTime", sfd=True)
	if connAttr == "" and not isConstant:
		cmds.connectAttr(jobInfo.timeCtrl+".outTime", node+".inTime")

	node = node + ".fileName"	# compute once, used 2-3 times
	connAttr = cmds.connectionInfo(node, sfd=True)
	if connAttr != "":
		cmds.disconnectAttr(connAttr, node)
	cmds.connectAttr(jobInfo.filenode+".outFileName", node)
	pass

def attachXform(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachXform")
	conX = cmds.listConnections(name+".translate")
	if conX != None and len(conX) > 0:
		# already receiving transformation from another node!
		conX = conX[0]
		if cmds.objectType(conX) == "ExocortexAlembicXform":
			conX = conX + ".fileName"	# compute once, used 2-3 times!
			fileN = cmds.connectionInfo(conX, sfd=True)
			if fileN != None and fileN != "":
				cmds.disconnectAttr(fileN, conX)
			cmds.connectAttr(jobInfo.filenode+".outFileName", conX)
		else:
			print("Cannot attach Xform to " + name + ", it's attach to a node that is not an \"ExocortexAlembicXform\"")
		return

	newXform = cmds.createNode("ExocortexAlembicXform")
	cmds.setAttr(newXform+".identifier", identifier, type="string")
	cmds.connectAttr(newXform+".translate", name+".translate")
	cmds.connectAttr(newXform+".rotate",	name+".rotate")
	cmds.connectAttr(newXform+".scale", 	name+".scale")

	attachTimeAndFile(newXform, jobInfo, isConstant)
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachXform")
	pass

def attachPolyMesh(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachPolyMesh")
	if cmds.objectType(name) != "mesh":
		print("Only mesh can be attached too!")
		cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachPolyMesh")
		return

	polyObj = cmds.connectionInfo(name+".inMesh", sfd=True).split('.')[0]					# cmds.plugNode doesn't exist!
	if polyObj != None and cmds.objectType(polyObj) == "ExocortexAlembicPolyMeshDeform":	# it's already attached to a deform, simply change the file reference
		attachTimeAndFile(polyObj, jobInfo, isConstant)
		return

	# create deformer, and attach time and file
	newDform = cmds.deformer(name, type="ExocortexAlembicPolyMeshDeform")[0]
	cmds.setAttr(newDform+".identifier", identifier, type="string")
	attachTimeAndFile(newDform, jobInfo, isConstant)

  	# get polyObj new "output" attribute connection
  	if polyObj != None and cmds.objectType(polyObj) != "ExocortexAlembicPolyMesh":
  		originalMesh = cmds.connectionInfo(polyObj+".output", sfd=True).split('.')[0]

  		cmds.delete(polyObj)
  		polyObj = cmds.createNode("ExocortexAlembicPolyMesh")
  		attachTimeAndFile(polyObj, jobInfo, isConstant)

  		cmds.connectAttr(polyObj+".outMesh", originalMesh+".inMesh")
  		cmds.setAttr(polyObj+".normals", jobInfo.useNormals)
		cmds.setAttr(polyObj+".uvs", jobInfo.useUVs)

  	if jobInfo.useFaceSets:
  		cmds.ExocortexAlembic_createFaceSets(f=cmds.getAttr(jobInfo.filenode+".outFileName"), i=identifier, o=name)

	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachPolyMesh")
	pass

def attachCamera(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachCamera")
	camObj = cmds.connectionInfo(name+".focalLength", sfd=True)
	if camObj != None and cmds.objectType(camObj) == "ExocortexAlembicCamera":
		attachTimeAndFile(camObj, jobInfo, isConstant)
		return

	reader = cmds.createNode("ExocortexAlembicPoints")
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
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachCamera")
	pass

def attachCurves(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachCurves")
	curObj = cmds.connectionInfo(name+".focalLength", sfd=True)
	if curObj != None and cmds.objectType(curObj) == "ExocortexAlembicCurvesDeform":
		attachTimeAndFile(curObj, jobInfo, isConstant)
		return

	# create deformer, and attach time and file
	newDform = cmds.deformer(name, type="ExocortexAlembicCurvesDeform")[0]
	cmds.setAttr(newDform+".identifier", identifier, type="string")
	attachTimeAndFile(newDform, jobInfo, isConstant)

  	# get curObj new "output" attribute connection
  	if curObj != None and cmds.objectType(curObj) != "ExocortexAlembicCurves":
  		originalCur = cmds.connectionInfo(curObj+".output", sfd=True).split('.')[0]

  		cmds.delete(curObj)
  		curObj = cmds.createNode("ExocortexAlembicCurves")
  		attachTimeAndFile(curObj, jobInfo, isConstant)

		cmds.connectAttr(curObj+".outCurve", originalCur+".create")
		cmds.connectAttr(jobInfo.filenode+".outFileName", curObj+".fileName")
		cmds.setAttr(curObj+".identifier", identifier, type="string")

	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachCurves")
	pass

def attachPoints(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachPoints")
	ptsObj = cmds.connectionInfo(name+".focalLength", sfd=True)
	if ptsObj != None and cmds.objectType(ptsObj) == "ExocortexAlembicPoints":
		attachTimeAndFile(ptsObj, jobInfo, isConstant)
		return

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
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachPoints")
	pass

