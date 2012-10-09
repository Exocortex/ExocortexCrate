

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

includePath = os.environ["includePath"];
testPath = os.environ["testPath"];
testFile = os.environ["testName"];
app = os.environ["app"];
appVer = os.environ["appVer"];
genBaseline = os.environ["genBaseline"];


obj = os.environ["obj"];

transformsEO = getInput("transformsEO");
frameToRender = getIntInput("frameToRender", 0);
exportStrAppend = getInput("exportStr");

if exportStrAppend != None : print "exportStr was found."


#transformsEO
#try:
#	transformsEO = os.environ["transformsEO"]
#except:
#	transformsEO = None


#frameToRender = 0
#try:
#	frameToRender = os.environ["frameToRender"]
#except:
#	frameToRender = None

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
