import maya.cmds as cmds
import _functions as fnt

""" Import module of Exocortex Crate """

def fillAlembicInfoList(filename):
	alembicInfos = []
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.fillAlembicInfoList")
	AlembicInfo.nbTransforms = 0
	for info in cmds.ExocortexAlembic_getInfo(f=filename):
		alembicInfos.append(fnt.AlembicInfo(info.split("|")))
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.fillAlembicInfoList")
	return alembicInfos

def doIt(filename, importNormals=False, importUvs=True, importFaceSets=True):
	"""
	Creates the right nodes for each object in the Alembic file!
	"""
	gMainProgressBar = "MayaWindow|toolBar3|MainHelpLineLayout|helpLineFrame|formLayout16|mainProgressBar"

	# initialization!
	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.doIt")
	cmds.ExocortexAlembic_profileReset()
	cmds.ExocortexAlembic_fileRefCount(i=filename)

	# fill the lists
	alembicInfos = fillAlembicInfoList(filename)
	
	# time control/file node
	timeControl = "AlembicTimeControl"
	if not cmds.objExists(timeControl):
		timeControl = cmds.createNode("ExocortexAlembicTimeControl", name=timeControl)
		fnt.alembicConnectAttr("time1.outTime", timeControl+".inTime")
	fileNode = cmds.createNode("ExocortexAlembicFile")
	cmds.setAttr(fileNode+".fileName", filename, type="string")

	cmds.progressBar(gMainProgressBar, e=True, bp=True, ii=1, max=len(alembicInfos))

	# for each each, starting with the last one!
	for ii in xrange(len(alembicInfos)-1, -1, -1):
		if cmds.progressBar(gMainProgressBar, q=True, ic=True):
			print("Import interrupted by the user")
			break
		cmds.progressBar(gMainProgressBar, e=True, s=1)

		curObj = alembicInfos[ii]
		if curObj.identifier == "":
			continue
		parts = curObj.identifier.split("/")
		reader = ""
		topoReader = ""
		shape = ""

		# parse additional data options
		purepointcache = False
		dynamictopology = False
		hair = False
		data = curObj.data.split(";")
		for dt in data:
			if dt == "purepointcache=1":
        		purepointcache = True
      		else if dt =="dynamictopology=1":
        		dynamictopology = True
      		else if dt =="hair=1":
        		hair = True

        if curObj.type == "PolyMesh":
        	pass
        elif curObj.type == "Xform":
        	pass
        elif curObj.type == "Group":
        	pass
        elif curObj.type == "SubD":
        	pass
        elif curObj.type == "Curves":
        	pass
        elif curObj.type == "Points":
        	pass
        elif curObj.type == "Camera":
        	pass
        elif curObj.type == "Unknown":
        	pass
        else:
        	print("Invalid object type for object "+ curObj.name +"\n")
      		continue

      	# finalize
      	cmds.ExocortexAlembic_profileBegin(f="Python.ExocortexAlembic._import.doIt:finalize")
	    if ($reader != "")
	    {
	      if (curObj.constant)
	        fnt.alembicConnectAttr(timeControl+".outTime", reader+".inTime")
	      fnt.alembicConnectAttr($fileNode+".outFileName", $reader+".fileName")
	      cmds.setAttr($reader+".identifier", curObj.identifier, type="string")
	    }
	    curObj.object = shape
      	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.doIt:finalize")

	    # setup metadata if we have it
	    cmds.ExocortexAlembic_createMetaData(f=filename, i=curObj.identifier, o=shape)

	# finalization!
	cmds.progressBar(gMainProgressBar, e=True, endProgress=True)
  	cmds.ExocortexAlembic_postImportPoints()
	cmds.ExocortexAlembic_fileRefCount(d=filename)
	cmds.ExocortexAlembic_profileEnd(f="Python.ExocortexAlembic._import.doIt")
	pass

