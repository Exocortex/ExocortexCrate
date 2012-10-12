from common import *

tasks = TaskList([Task("Import"), Task("Render")], genPath(iTestFile + ".ats"))

normals = True
uvs = True
clusters = True 
visibility = True 
standins = False
bboxes = False
attach = False
identifiers = ""

iDirectImport = getInput("directImport", "false");

path = ""

if iDirectImport != "true": 
	path = genPath(iObj + iAlembicFileExt)
else:
	path = iTestPath + iObj + iAlembicFileExt

result = Application.alembic_import(path , normals, uvs, clusters, visibility, standins, bboxes, attach, identifiers)
tasks.setStatus("Import", result == None)

if result == None: renderScene(Application, iObj)

tasks.writeResults()



#TODO 
# set up camera
# set frame
# render the frame