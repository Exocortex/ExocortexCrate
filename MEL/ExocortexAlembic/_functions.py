import maya.mel as mel
import maya.cmds as cmds

""" Contains functions and data structures """

############################################################################################################
# general functions needed to import and attach
############################################################################################################
def alembicTimeAndFileNode(filename, multi=False):
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._functions.alembicTimeAndFileNode")
	#print("time control")
	timeControl = "AlembicTimeControl"
	if not cmds.objExists(timeControl):
		timeControl = cmds.createNode("ExocortexAlembicTimeControl", name=timeControl)
		alembicConnectAttr("time1.outTime", timeControl+".inTime")
	#print("file node")
	fileNode = cmds.createNode("ExocortexAlembicFile")
	cmds.setAttr(fileNode+".fileName", filename, type="string")
	cmds.connectAttr(timeControl+".outTime", fileNode+".inTime")
	if multi:
		cmds.setAttr(fileNode+".multiFiles", 1);
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._functions.alembicTimeAndFileNode")
	return fileNode, timeControl

def createNamespaces(namespaces):
	#print("createNamespaces("+ str(namespaces) + ")")
	if len(namespaces) > 1:
		# first one!
		accum = ":" + namespaces[0]
		if not cmds.namespace(exists=accum):
			cmds.namespace(add=namespaces[0])
		# rest ...
		for nspace in namespaces[1:-1]:
			curAccum = accum + ":" + nspace
			if not cmds.namespace(exists=curAccum):
				cmds.namespace(add=nspace, p=accum)
			accum = curAccum
	pass

def subCreateNode(names, type, parentXform):
	#print("subCreateNode(" + str(names) + ", " + type + ", " + str(parentXform) + ")")
	accum = None
	for name in names[:-1]:
		result = name
		if accum != None:
			result = accum + '|' + result
		if not cmds.objExists(result):
			createNamespaces(name.split(':'))
			#print("create " + name + ", child of: " + str(accum))
			result = cmds.createNode("transform", n=name, p=accum)

		if accum == None:
			accum = result
		else:
			accum = accum + "|" + result.split('|')[-1]

	name = names[-1]
	if parentXform != None and len(parentXform) > 0:
		accum = parentXform
	createNamespaces(name.split(':'))
	#print("create " + name + ", child of: " + str(accum))
	result = cmds.createNode(type, n=name, p=accum)

	if accum != None:
		result = accum + "|" + result.split("|")[-1]
	#print("--> " + result)
	return result

def alembicCreateNode(name, type, parentXform=None):
	""" create a node and make sure to return a full name and create namespaces if necessary! """
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._functions.createAlembicNode")
	#print("alembicCreateNode(" + str(name) + ", " + str(type) + ", " + str(parentXform) + ")")
	result = subCreateNode(name.split('|'), type, parentXform)
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
# poly mesh functions
############################################################################################################

def alembicPolyMeshToSubdiv(mesh=None):
	if mesh == None:	# if None... refer to the selection list
		sel = cmds.ls(sl=True)
		if len(sel) != 1:
			if len(sel) > 1:
				return ("Only one transform or mesh should be selected")
			else:
				return ("Need to select one transform or mesh")
		mesh = sel[0];

	xform = cmds.objectType(mesh)	# here xform is the type of mesh. below, xform becomes mesh's transform
	if xform == "mesh":
		xform = cmds.listRelatives(mesh, p=True)[0]
	elif xform == "transform":
		xform = mesh;
		mesh = cmds.listRelatives(xform, s=True)[0]
	else:
		return ("Type " + xform + " not supported")

	try:
		newX = cmds.createNode("transform", n=(xform+"_SubD"))
		sub  = cmds.createNode("subdiv", n=(mesh+"_SubD"), p=newX)
		poly = cmds.createNode("polyToSubdiv")

		cmds.connectAttr(xform+".translate", newX+".translate")
		cmds.connectAttr(xform+".rotate", newX+".rotate")
		cmds.connectAttr(xform+".scale", newX+".scale")
		cmds.connectAttr(xform+".rotateOrder", newX+".rotateOrder")
		cmds.connectAttr(xform+".shear", newX+".shear")
		cmds.connectAttr(xform+".rotateAxis", newX+".rotateAxis")

		cmds.connectAttr(poly+".outSubdiv", sub+".create")
		cmds.connectAttr(mesh+".outMesh", poly+".inMesh")

		cmd.setAttr(sub+".dispResolution", 3);
	except Exception as ex:
		return str(ex.args)
	return ""

############################################################################################################
# progress bar
############################################################################################################

__gMainProgressBar = ""

def progressBar_init(_max):
	global __gMainProgressBar
	try:
		__gMainProgressBar = mel.eval('$tmp = $gMainProgressBar')
		if __gMainProgressBar != "":
			cmds.progressBar(__gMainProgressBar, edit=True, isInterruptable=True, maxValue=_max)
	except:
		__gMainProgressBar = ""

def progressBar_start():
	global __gMainProgressBar
	if __gMainProgressBar != "":
		cmds.progressBar(__gMainProgressBar, edit=True, beginProgress=True, step=1)

def progressBar_stop():
	global __gMainProgressBar
	if __gMainProgressBar != "":
		cmds.progressBar(__gMainProgressBar, edit=True, endProgress=True)

def progressBar_incr(_step=20):
	global __gMainProgressBar
	if __gMainProgressBar != "":
		cmds.progressBar(__gMainProgressBar, edit=True, step=_step)

def progressBar_isCancelled():
	global __gMainProgressBar
	if __gMainProgressBar == "":
		return 0
	if cmds.progressBar(__gMainProgressBar, query=True, isCancelled=True):
		return 1
	return 0


