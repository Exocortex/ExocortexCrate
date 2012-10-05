var express = require('express');
var fs = require('fs');
//var url = require('url');
//var querystring = require('querystring');
var util = require('util');
var nconf = require('nconf');
var childp = require('child_process');



var testsMod = require('./testsMod');
var TestDesc = testsMod.TestDesc;

nconf.argv().env().file({file: __dirname+'/config.json'});

// nconf.defaults({
//     "webServer":{"port": 3000},
//     "appz":{
//     	"max2010": "",
//     	"max2011": "",
// 		"max2012": "C:/Program Files/Autodesk/3ds Max 2012/3dsmax.exe"
// 	}
// });


var server_port = nconf.get('webServer:port');

var globals = nconf.get('globals');




var app = express.createServer();

app.configure('development', function(){
   // have everything in the public directory available to requesters as static data
   // this means that one can request by name anything in the public directory
   // such as images, css, etc...
   //app.use(express.static(__dirname + '/public'));
   app.use(express.errorHandler({ dumpExceptions: true, showStack: true }));
   app.use(express.bodyParser());
});


app.register('.html', require('ejs'));
app.set('view engine', 'ejs');
app.set("view options", { layout: false })


// app.get('/scripts/main.js', function(req, res){
//  fs.readFile( "plan.txt", 'utf8', function( err, mainData ) {
//        res.send(mainData );
//        });
// });

app.get('/404', function(req, res) {
  	res.send('NOT FOUND '+req.url);
});

app.listen(server_port);
console.log( "listening on port: "+server_port);




var testPath = [__dirname, "/Tests"].join('');

if( nconf.get('testFolderPath') !== ""){
	testPath = nconf.get('testFolderPath');
	console.log('Reading tests at '+testPath);
}


var getTestFullPath = function(path){
	return [testPath, path].join('');
};



var testList = [];

var readTestDir = function(path, list){

	//console.log('Reading test dir: '+path);

	var files = fs.readdirSync(path);

	var len = files.length;
	//console.log("numFiles: "+len);
	for(var i=0; i<len; i++){

		var filePath = [path, '/', files[i]].join('');

		//console.log("reading file: "+filePath);

		var stats = fs.lstatSync(filePath);
		if(stats.isDirectory()){
			readTestDir(filePath, list);
		}
		else if(stats.isFile()){
			if(files[i] === 'testParams.json'){ //we have found a folder that contains a test

				//console.log("reading "+filePath)

				var data = fs.readFileSync(filePath);

				if(data.length > 0 ){
					//console.log("data: \n"+data);

	          		var aJSON = JSON.parse(data);

	          		var slen = aJSON.tasks.length;
	          		for(s=0; s<slen; s++){
	          			var script = aJSON.tasks[s];
	          			script.testpath = path;
	          			testList.push(script);
	          		}

				}
			}
		}
		else{
			console.log("Error: not a directory or file");
		}

	}

}


var cleanTestDir = function(path){

	//console.log('Reading test dir: '+path);

	var files = fs.readdirSync(path);

	var len = files.length;
	//console.log("numFiles: "+len);
	for(var i=0; i<len; i++){

		var filePath = [path, '/', files[i]].join('');

		//console.log("reading file: "+filePath);

		var stats = fs.lstatSync(filePath);
		if(stats.isDirectory()){
			cleanTestDir(filePath);
		}
		else if(stats.isFile()){
			if(	files[i].search(".ats") != -1 || 
				files[i].search("_Render") != -1 || 
				files[i].search(".tabc") != -1 
				){ //we have found a folder that contains a test

				var res = fs.unlinkSync(filePath);
				//console.log("deleted " + path + " = " + res);
			}
		}
		else{
			console.log("Error: not a directory or file");
		}

	}

}






// - can call with the entire test folder as path to gather all test results
//OR
// - can call with a single test folder to gather results of a single test
var buildTestResults = function(path, testResults, atsname, errcode){
	//console.log('Reading test dir: '+path);

	var files = fs.readdirSync(path);

	var len = files.length;
	//console.log("numFiles: "+len);
	for(var i=0; i<len; i++){

		var filePath = [path, '/', files[i]].join('');

		//console.log("reading file: "+filePath);

		var stats = fs.lstatSync(filePath);
		if(stats.isDirectory()){
			buildTestResults(filePath, testResults);
		}
		else if(stats.isFile()){

			var bReturn = false;
			if(atsname !== undefined){
				bReturn = true;
			}

			atsname = atsname || ".ats";

			//console.log("Searching for ats file: "+atsname);

			if( files[i].search(atsname) !== -1){ //we have found a folder that contains a test

				var testName = path.substring(__dirname.length + 6);

				var result = testResults[testName];
				if(result === undefined){
					result = {
						tasks:{}
					};
					testResults[testName] = result;
				}
				console.log('READING ats file: ' + filePath);

				var data = fs.readFileSync(filePath, 'ascii');

				if(data.length > 0){
					console.log(data + "\n");

					var tokens =  data.split('\n');

					if(tokens.length == 0){
						//result.tasks will be empty
						console.log("no data in ats file:" + filePath);
					}
					else{

						var namesToken = tokens[0].split("=");
						if(namesToken.length > 1 && namesToken[0] == "names"){

							var names = namesToken[1].split(",");

							var crashStr = "INCOMPLETE";
							//console.log("cec: "+errcode);
							// if(errcode == -1){
							// 	crashStr = "CRASH";
							// }

							for(var n=0; n<names.length; n++){
								result.tasks[names[n].trim()] = {status: crashStr};
							}

							//console.log("result: "+JSON.stringify(result)+"\n");

							for(var t=1; t<tokens.length-1; t++){
								var tokenTask = tokens[t].split(':');
								if(tokenTask.length == 2){

									var task = result.tasks[tokenTask[0]];
									if( task == undefined ){
										result.tasks[tokenTask[0].trim()] = {status: " --- UNREPORTED --- " + tokenTask[1].trim()};
									}
									else{
										task.status = tokenTask[1].trim();
									}

								}
							}

							//console.log("result: "+JSON.stringify(result)+"\n");
						}
						else{
							console.log("task names token not found: token[0] = "+tokens[0] + "   split = " + JSON.stringify(namesToken));
						}


					}
				}

				if(bReturn){
					return result;
				}
			}
		}
		else{
			console.log("Error: not a directory or file.");
		}

	}

}




var equalFiles = function(bpath, rpath){



	try{

	var bfd = fs.readFileSync(bpath);
	var rfd = fs.readFileSync(rpath);

	}
	 catch(err){

	 	console.log("bfd: "+bpath);
	 	console.log("rfd: "+rpath);
	 	return false;
	}

	if(bfd.length != rfd.length){
		console.log("lengths not equal");
		return false;
	}
	else{

		for(var i=0; i<bfd.length; i++){

			if(bfd[i] != rfd[i]){
				console.log("byte not equal");
				return false;
			}

		}

		return true;
	}


}


var TestManager = function(maxpath, includePath){

	this.maxpath = maxpath;
	this.includePath = includePath;
	this.tests = [];
	this.currTestIndex = 0;
	this.results = {};
};

var tmProto = TestManager.prototype;

tmProto.getAtsName = function(){
	var test = this.tests[this.currTestIndex];
	var name = test.scriptName.replace(".ms", ".ats");
	name = test.scriptName.replace(".py", ".ats");
	return test.app + test.appVer + "_" + name;
}

tmProto.getRenderedImagePath = function(){
	var test = this.tests[this.currTestIndex];
	return [test.testdir, "/", test.app, test.appVer, "_", test.obj, "_Render.0.jpg"].join('');
}

tmProto.getBaselineImagePath = function(){
	var test = this.tests[this.currTestIndex];
	return [test.testdir, "/", test.app, test.appVer, "_", test.obj, "_Baseline.0.jpg"].join('');
}

tmProto.testComplete = function(tDesc){

	var result = buildTestResults(tDesc.testdir, this.results, this.getAtsName(), tDesc.errcode);

	if(result !== undefined){

		var rt = result.tasks["render"];
		if( rt == undefined){
			rt = result.tasks["Render"];
		}
		if(rt !== undefined){

			if( equalFiles(this.getBaselineImagePath(), this.getRenderedImagePath()) ){
				rt.status = 'PASS';
			}
			else{
				rt.status = "FAIL";
			}
		}
	}


	this.currTestIndex++;
	if(this.currTestIndex < this.tests.length){

		// var path = [this.tests[this.currTestIndex], getAtsName()].join('');

		// var res = fs.unlinkSync(path);

		// console.log("unlink: "+res);

		this.tests[this.currTestIndex].execute();
	}
	else{
		// console.log(JSON.stringify(this.results) + "\n");

		// var results = this.results;

		// for(var name in results){
		// 	if(!results.hasOwnProperty(name)) continue;

		// 	console.log("name: "+name);

		// 	var tasks = results[name].tasks;

		// 	for(var t in tasks){
		// 		console.log(t + ":" + tasks[t].status + "\n");
		// 	}

		// }

	}
}

var splitPathAndFile = function(pathfile){

	var index = pathfile.lastIndexOf('/');
	var indexb = pathfile.lastIndexOf('\\');
	if(indexb > index){
		index = indexb;
	}

	return{
	 	path: __dirname + pathfile.substring(0, index+1), // "/3DSMax/pathfiles/"
	 	file: pathfile.substring(index+1)
	}
}


var resolveGlobal = function(inputStr, globalTable){

	if (inputStr.substring(0,2) === "g_"){

		var globalName = inputStr.substring(2);
		return globalTable[globalName];
	} 
	else{
		return inputStr;
	}

}

tmProto.build = function(list){

	for(i=0; i<list.length; i++){

		var runEnv ={
			includePath: this.includePath,
			genBaseline: 'true'
		};
		for(var v in list[i].inputs){
			if(!list[i].inputs.hasOwnProperty(v)) continue;
			runEnv[v] = list[i].inputs[v];
		}

		var script = list[i].script;
		var scriptPath = '';
		var scriptName = '';
		if(script.indexOf('/') === -1 && script.indexOf('\\') === -1){//the script file is in the local directory
			scriptName = script; 
		}
		else{
			var pf = splitPathAndFile(script);
			scriptPath = pf.path;
			scriptName = pf.file;
		}

		var testAppSettings = globals.app[list[i].inputs.app];
		var testAppVersionSettings = testAppSettings[list[i].inputs.appVer];

		//console.log(JSON.stringify(testAppSettings));
		//console.log(JSON.stringify(testAppVersionSettings));
		
		var params = {
			genBaseline: false,
			app: list[i].inputs.app,
			appVer: list[i].inputs.appVer,
			obj: list[i].inputs.obj,
			exepath: testAppVersionSettings.path,//"C:/Program Files/Autodesk/Softimage 2013/Application/bin/XSI.exe",
			testdir: list[i].testpath,
			arguments: '',//-silent',
			scriptPath: scriptPath,
			scriptName: scriptName, 
			runEnvVars: runEnv,
			completionContext: this,
			completionCallback: this.testComplete
		}

		//XSI alembic_import method doesn't like back slashes
		params.testdir.replace('\\', '/');

		this.tests.push( new TestDesc(params) );
	}
}

tmProto.runAll = function(){
	if(this.currTestIndex < this.tests.length){
		this.tests[this.currTestIndex].execute();
	}
}

tmProto.print = function(){
	for(i=0; i<this.tests.length; i++){

		console.log(this.tests[i]);
	}
}

var testMan = new TestManager("", __dirname+"/3DSMax/includes");




app.get('/Results', function(req, res){

	//testMan.results

 	res.render(__dirname+'/testResults.html', {results: testMan.results});


});
   


cleanTestDir(testPath);
readTestDir(testPath);
testMan.build(testList);
testMan.runAll();





// var bpath = [__dirname, '/max2012_Sphere001_Baseline.jpg'].join('');
// var rpath = [__dirname, '/max2012_Sphere001_Render.jpg'].join('');

// if( equalFiles(bpath, rpath) ){

// 	console.log('files are equal');

// }
// else{

// 	console.log('files are not equal');
// }








