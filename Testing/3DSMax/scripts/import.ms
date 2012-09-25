
includes = systemTools.getEnvVariable("includePath");
execute("include\"" + includes + "\common.ms\"");

tasks = #(task("import"), task("render"))
writeTaskNames tasks

/* load import.max if present. This is my was cofiguring custom camera settings */
try ( loadMaxFile "import.max" ) catch()

result = ExocortexAlembic.import (app + "_" + obj + ".tabc") true true true true 0
setStatus tasks "import" (result == 0)



select $*

sel = selection as array
for i = 1 to sel.count do (
	
	sel[i].wirecolor = [255, 0, 0]

)



if( result == 0 ) then (

	if genBaseline == "true" then (
		render frame:0 outputwidth:1024 outputheight:768 outputfile:(testPath + app + "_" + obj + "_Baseline.jpg") vfb:true
	)
	else (
		render frame:0 outputwidth:1024 outputheight:768 outputfile:(testPath + app + "_" + obj + "_Render.jpg") vfb:true
	)
)


writeTaskFile tasks


quitMax #noPrompt




