
####### Common Code Begin #############
import os

includePath = os.environ["includePath"];
testPath = os.environ["testPath"];
testFile = os.environ["testName"];
app = os.environ["app"];
appVer = os.environ["appVer"];
obj = os.environ["obj"];
genBaseline = os.environ["genBaseline"];


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



tasks = TaskList([Task("Export")], testPath + app + appVer + "_" + testFile + ".ats")


Application.OpenScene(testPath+"sphere.scn", False, False)


Application.SelectObj("*")


sel = Application.Selection;


objectsStr = "objects="
for i in range(0, len(sel)-1):
	objectsStr += sel(i).Name
	objectsStr +=","
objectsStr += sel(len(sel)-1).Name


result = Application.alembic_export("filename=" + testPath + app + appVer + "_" + obj + ".tabc;"+objectsStr)
tasks.setStatus("Export", result == None)

tasks.writeResults()


