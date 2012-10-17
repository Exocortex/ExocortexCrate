

import os


def getInput(name, default=None):
	#print name + os.environ[name]
	try:
		return os.environ[name]
	except:
		return default

def getIntInput(name, default=None):
	#print name, int(os.environ[name])
	try:
		return int(os.environ[name])
	except:
		return default

#includePath = os.environ["includePath"];
iTestPath = os.environ["testPath"];
iTestFile = os.environ["testName"];
iApp = os.environ["app"];
iAppVer = os.environ["appVer"];
iGenBaseline = os.environ["genBaseline"];
iScript = os.environ["script"];


iObj = getInput("obj");

iTransformsEO = getInput("transformsEO");
iFrameToRender = getIntInput("frameToRender", 0);
iExportStrAppend = getInput("exportStr");
iAlembicFileExt = getInput("alembicFileExt", ".tabc");

def genPath(filename):
	return iTestPath + iApp + iAppVer + "_" + filename


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


def renderScene(app, name):
	imageName = iApp + iAppVer + "_" + name
	if iGenBaseline == "true": imageName += "_Baseline"
	else: imageName += "_Render"

	app.SetValue("Passes.RenderOptions.OutputDir", iTestPath, "")
	app.SetValue("Passes.Default_Pass.Main.Filename", imageName, "")
	app.SetValue("Passes.Default_Pass.Main.Format", "jpg", "")
	#print "iFrameToRender: "+iFrameToRender
	app.RenderPasses("Passes.Default_Pass", iFrameToRender, iFrameToRender)

	#right render includes the images existing and the comparison, but instead split them (we check for the image to exist here, and set to pass if it does)
	#tasks.setStatus("Render", True)