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

def attachPolyMeshDeformer(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachPolyMeshDeformer")
	polyObj = (cmds.connectionInfo(name+".inMesh", sfd=True)[0]).split('.')[0]				# plugNode doesn't exist!
	if polyObj != None and cmds.objectType(polyObj) == "ExocortexAlembicPolyMeshDeform":	# it's already attached to a deform, simply change the file reference
		attachTimeAndFile(polyObj, jobInfo, isConstant)
		return

	# create deformer, and attach time and file
	newDform = cmds.deformer(name, type="ExocortexAlembicPolyMeshDeform")[0]
	cmds.setAttr(newDform+".identifier", identifier, type="string")
	attachTimeAndFile(newDform, jobInfo, isConstant)

  	# get polyObj new "output" attribute connection
  	if polyObj != None and cmds.objectType(polyObj) != "ExocortexAlembicPolyMesh":
  		originalMesh = (cmds.connectionInfo(polyObj+".output", sfd=True)[0]).split('.')[0]

  		cmds.delete(polyObj)
  		polyObj = cmds.createNode("ExocortexAlembicPolyMesh")
  		attachTimeAndFile(polyObj, jobInfo, isConstant)

  		cmds.connectAttr(polyObj+".outMesh", originalMesh+".inMesh")
  		cmds.setAttr(polyObj+".normals", jobInfo.useNormals)
		cmds.setAttr(polyObj+".uvs", jobInfo.useUVs)

  	if jobInfo.useFaceSets:
  		cmds.ExocortexAlembic_createFaceSets(f=cmds.getAttr(jobInfo.filenode+".outFileName"), i=identifier, o=name)

	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachPolyMeshDeformer")
	pass

def attachSubdivDeformer(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachSubdivDeformer")

	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachSubdivDeformer")
	pass

def attachPolyMesh(name, identifier, jobInfo, isConstant=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._attach.attachPolyMesh")
	nType = cmds.objectType(name)
	if nType == "mesh":
		attachPolyMeshDeformer(name, identifier, jobInfo, isConstant)
	elif nType == "subdiv":
		attachSubdivDeformer(name, identifier, jobInfo, isConstant)
	else:
		print("Cannot attach to a node of type " + str(nType))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._attach.attachPolyMesh")
	pass



