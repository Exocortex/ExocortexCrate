from common import *

iBase = os.environ["base"]
iPointCache = os.environ["pointCache"]

tasks = TaskList([Task("ImportBase"), Task("ImportPointCache"), Task("Render")], genPath(iTestFile + ".ats"))

normals = True
uvs = True
clusters = True 
visibility = True 
standins = False
bboxes = False
attach = False
identifiers = ""

result = Application.alembic_import(genPath(iBase + iAlembicFileExt), normals, uvs, clusters, visibility, standins, bboxes, attach, identifiers)
tasks.setStatus("ImportBase", result == None)

normals = False
attach = True

result = Application.alembic_import(genPath(iPointCache + iAlembicFileExt), normals, uvs, clusters, visibility, standins, bboxes, attach, identifiers)
tasks.setStatus("ImportPointCache", result == None)



renderScene(Application, iBase+"_"+iPointCache)

tasks.writeResults()



#TODO 
# set up camera
# set frame
# render the frame