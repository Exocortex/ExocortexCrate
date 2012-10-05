
####### Common Code Begin #############
import os

includePath = os.environ["includePath"]
testPath = os.environ["testPath"]
testFile = os.environ["testName"]
app = os.environ["app"]
appVer = os.environ["appVer"]
obj = os.environ["obj"]
genBaseline = os.environ["genBaseline"]
frameToRender = int(os.environ["frameToRender"])


class Task:
	def __init__(self, name, status="UNKNOWN"):
		self.name = name
		self.status = status


class TaskList:
	def __init__(self, array, path):
		self.data = array
		self.path = path
		self.file = open(path, 'w')
		self.writeNames()

	def setStatus(self, name, val):
		for x in self.data:
			if x.name == name:
				if val == True: x.status = "PASS"
				else: x.status = "FAIL"
			
	def printAll(self):
		for x in self.data:
			print( x.name + ":" + x.status )

	def writeNames(self):
		self.file.write("names=")

		for i in range(0, len(self.data)-1):
			self.file.write(self.data[i].name+",")
		self.file.write( self.data[len(self.data)-1].name )

		self.file.write("\n")
		self.file.close()
		self.file = open(self.path, 'a')

	def writeResults(self):

		for x in self.data:
			self.file.write(x.name + ":" + x.status + "\n")

		self.file.close()

####### Common Code End ##################


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
	Application.RenderPasses("Passes.Default_Pass", frameToRender, frameToRender)

	#right render includes the images existing and the comparison, but instead split them (we check for the image to exist here, and set to pass if it does)
	#tasks.setStatus("Render", True)





tasks.writeResults()



#TODO 
# set up camera
# set frame
# render the frame