print("included common.ms");





struct exportStr (filename="", frame_in=0, frame_out=100, step=1, substep=1, normals=true, uvs=true, material_ids=true, bindpose=true, export_selected=true, flatten_hierarchy=true, automatic_instancing=true, transform_cache=false)

function alembicExport exportParams = (

	jobString = "filename=" + exportParams.filename
	jobString += ";in=" + (exportParams.frame_in as string) 
	jobString += ";out=" + (exportParams.frame_out as string)
	jobString += ";step=" + (exportParams.step as string)
	jobString += ";substep=" + (exportParams.substep as string)
	jobString += ";normals=" + (exportParams.normals as string)
	jobString += ";uvs=" + (exportParams.uvs as string)
	jobString += ";materialids=" + (exportParams.material_ids as string)
	jobString += ";bindpose=" + (exportParams.bindpose as string)
	jobString += ";exportselected=" + (exportParams.export_selected as string)
	jobString += ";flattenhierarchy=" + (exportParams.flatten_hierarchy as string)
	jobString += ";automaticinstancing=" + (exportParams.automatic_instancing as string)
	jobString += ";transformCache=" + (exportParams.transform_cache as string)

	return ExocortexAlembic.createExportJobs(jobString)

)


struct task (name="", status="UNKNOWN")





function setStatus tasks taskName val = (

	for t in tasks do (
		if t.name == taskName then (
			if val == true then t.status = "PASS"
			else t.status = "FAIL"
		) 
	)

)

function setStatusUnknown tasks taskName = (

	for t in tasks do (
		if t.name == taskName then (
			t.status = "UNKNOWN"
		) 
	)

)


function writeLog outFile value = (
	format "%\n" (">" + value) to:outFile
)

/*
testPath = getFilenamePath(getSourceFileName());
testFile = getFilenameFile(getSourceFileName());
*/



testPath = systemTools.getEnvVariable("testPath");
testFile = systemTools.getEnvVariable("testName");
app = systemTools.getEnvVariable("app");
obj = systemTools.getEnvVariable("obj");

outFile = createFile (app + "_" + testFile + ".ats")


function writeTaskNames tasks = (
	format "%" ("names=") to:outFile

	for i = 1 to tasks.count do (
		if (i != tasks.count) then format "%," tasks[i].name to:outFile
		else format "%" tasks[i].name to:outFile
	)

	format "%\n" "" to:outFile

	close outFile
	outFile = openFile (app + "_" + testFile + ".ats") mode:"a"
)

function writeTaskFile tasks = (

	for t in tasks do (
		format "%\n" (t.name + ":" + t.status) to:outFile
	)

	close outFile
)


/*
logsystem.quietmode = true
*/

/*
print test;
test = getThisScriptFilename();
print test; 
test = getSourceFileOffSet();
print test; 
test = getSourceFileLine();
print test; 

*/


/*
importNormals=true
importUVs=true
attachToExisting=true
importMaterialIDs=true
*/