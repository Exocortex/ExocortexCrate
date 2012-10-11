


from common import *

tasks = TaskList([Task("Export")], testPath + app + appVer + "_" + testFile + ".ats")


Application.OpenScene(testPath+"export.scn", False, False)


Application.SelectObj("*")


sel = Application.Selection;


objectsStr = "objects="
for i in range(0, len(sel)-1):
	objectsStr += sel(i).Name
	objectsStr +=","
objectsStr += sel(len(sel)-1).Name

transformsStr = ""
if transformsEO == "flat": 
     transformsStr += ";flattenHierarchy=true";
     transformsStr += ";globalspace=false";
elif transformsEO == "full":
     transformsStr += ";flattenHierarchy=false";
     transformsStr += ";globalspace=false";
elif transformsEO == "bake":
     transformsStr += ";flattenHierarchy=true";
     transformsStr += ";globalspace=true";



exportStr = "filename=" + testPath + app + appVer + "_" +  obj + ".tabc;" + objectsStr + transformsStr;
if exportStrAppend != None: exportStr = exportStr + ";" + exportStrAppend;

result = Application.alembic_export(exportStr)
tasks.setStatus("Export", result == None)

tasks.writeResults()


