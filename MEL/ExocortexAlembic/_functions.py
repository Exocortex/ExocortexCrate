import maya.mel as mel
import maya.cmds as cmds

""" Contains functions and data structures """

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


############################################################################################################
# progress bar
############################################################################################################

__gMainProgressBar = ""

def progressBar_init(_max):
	global __gMainProgressBar
	__gMainProgressBar = mel.eval('$tmp = $gMainProgressBar');
	cmds.progressBar(__gMainProgressBar, edit=True, isInterruptable=True, maxValue=_max)

def progressBar_start():
	global __gMainProgressBar
	cmds.progressBar(__gMainProgressBar, edit=True, beginProgress=True, step=1)

def progressBar_stop():
	global __gMainProgressBar
	cmds.progressBar(__gMainProgressBar, edit=True, endProgress=True)

def progressBar_incr(_step):
	global __gMainProgressBar
	cmds.progressBar(__gMainProgressBar, edit=True, step=_step)

def progressBar_isCancelled():
	global __gMainProgressBar
	if cmds.progressBar(__gMainProgressBar, query=True, isCancelled=True):
		return 1
	return 0


