from common import *

tasks = TaskList([Task("Import"), Task("Render")], testPath + app + appVer + "_" + testFile + ".ats")

normals = True
uvs = True
clusters = True 
visibility = True 
standins = False
bboxes = False
attach = False
identifiers = ""

result = Application.alembic_import(testPath + app + appVer + "_" + obj + ".tabc", normals, uvs, clusters, visibility, standins, bboxes, attach, identifiers)
tasks.setStatus("Import", result == None)



if result == None:
	imageName = app + appVer + "_" + obj
	if genBaseline == "true": imageName += "_Baseline"
	else: imageName += "_Render"

	Application.SetValue("Passes.RenderOptions.OutputDir", testPath, "")
	Application.SetValue("Passes.Default_Pass.Main.Filename", imageName, "")
	Application.SetValue("Passes.Default_Pass.Main.Format", "jpg", "")
	#print "frameToRender: "+frameToRender
	Application.RenderPasses("Passes.Default_Pass", frameToRender, frameToRender)

	#right render includes the images existing and the comparison, but instead split them (we check for the image to exist here, and set to pass if it does)
	#tasks.setStatus("Render", True)





tasks.writeResults()



#TODO 
# set up camera
# set frame
# render the frame