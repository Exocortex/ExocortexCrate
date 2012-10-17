


from common import *

tasks = TaskList([Task(iTestFile)], genPath(iTestFile + ".ats"))


iSceneToLoad = getInput("sceneToLoad");

Application.OpenScene(iTestPath + iSceneToLoad, False, False)


iNodesToSelect = getInput("nodesToSelect");

if iNodesToSelect != None: Application.SelectObj(iNodesToSelect)
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


exportStr = "filename=" + genPath(iObj + iAlembicFileExt) + ";" + objectsStr + transformsStr;
if iExportStrAppend != None: exportStr = exportStr + ";" + iExportStrAppend;

result = Application.alembic_export(exportStr)
tasks.setStatus(iTestFile, result == None)

tasks.writeResults()


