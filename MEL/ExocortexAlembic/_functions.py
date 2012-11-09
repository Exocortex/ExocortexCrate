import maya.cmds as cmds

""" Contains functions and data structures """

class AlembicInfo:
	""" A structure to hold information about all objects in the file! """
	nbTransforms = 0
	def __init__(self, infoTokens):
		self.identifier = cmds.ExocortexAlembic_getNodeFromIdentifier(i=infoTokens[0])
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

def alembicCreateNode(name, type):
	""" create a node and make sure to return a full name and create namespaces if necessary! """
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.createAlembicNode")

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
	result = cmds.createNode(type, n=name)
	tokens = result.split("|")
	if len(tokens) == 1:
		pp = cmds.listRelatives(result, p=True)
		if len(pp) > 0:
			result = pp[0] + "|" + result

	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.createAlembicNode")
	return result

def alembicConnectAttr(source, target):
	""" make sure nothing is connected the target and then connect the source to the target """
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.alembicConnectIfUnconnected")
	currentSource = cmds.connectionInfo("pSphereShape1.inMesh", sfd=True)
	if currentSource != "" and currentSource != source:
		cmds.disconnectAttr(currentSource, target)
	cmds.connectAttr(source, target)
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.alembicConnectIfUnconnected")
	pass
