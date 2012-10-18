
includes = systemTools.getEnvVariable("includePath");
execute("include\"" + includes + "\common.ms\"");


tasks = #(task("export"))
writeTaskNames tasks

execute("loadMaxFile \"" + iFrameworkRoot + iSceneToLoad + "\"")

select $*

exportParams = exportStr (app + appVer + "_" + obj + ".tabc")
result = alembicExport exportParams
setStatus tasks "export" (result == 0)

writeTaskFile tasks



/*quitMax #noPrompt*/
