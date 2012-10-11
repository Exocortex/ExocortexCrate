


from common import *

tasks = TaskList([Task(iTestFile)], genPath(iTestFile + ".ats"))


Application.OpenScene(iTestPath+"export.scn", False, False)


iNodeToSelect = getInput("nodeToSelect");

if iNodeToSelect != None: Application.SelectObj(iNodeToSelect)
else: Application.SelectObj("*")


sel = Application.Selection;





objectsStr = "objects="
for i in range(0, len(sel)-1):
	objectsStr += sel[i].FullName
	objectsStr +=","
objectsStr += sel[len(sel)-1].FullName

transformsStr = ""
if iTransformsEO == "full":
     transformsStr += ";flattenHierarchy=false";
     transformsStr += ";globalspace=false";
elif iTransformsEO == "bake":
     transformsStr += ";flattenHierarchy=true";
     transformsStr += ";globalspace=true";
else: #iTransformsEO == "flat": 
     transformsStr += ";flattenHierarchy=true";
     transformsStr += ";globalspace=false";


exportStr = "filename=" + genPath(iObj + ".tabc") + ";" + objectsStr + transformsStr;
if iExportStrAppend != None: exportStr = exportStr + ";" + iExportStrAppend;

result = Application.alembic_export(exportStr)
tasks.setStatus(iTestFile, result == None)

tasks.writeResults()


