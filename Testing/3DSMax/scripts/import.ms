
includes = systemTools.getEnvVariable("includePath");
execute("include\"" + includes + "\common.ms\"");

tasks = #(task("import"), task("render"))
writeTaskNames tasks

/* load import.max if present. This is my was cofiguring custom camera settings */
try ( loadMaxFile "import.max" ) catch()

result = ExocortexAlembic.import (app + "_" + obj + ".abc") true true true true 0
setStatus tasks "import" (result == 0)


if( result == 0 ) then (
	render frame:0 outputwidth:1024 outputheight:768 outputfile:(testPath + app + "_" + obj + "_Render.jpg") vfb:true
)


writeTaskFile tasks


quitMax #noPrompt




