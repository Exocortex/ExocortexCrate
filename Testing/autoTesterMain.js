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

nconf.defaults({
    "webServer":{"port": 3000},
    "appz":{
    	"max2010": "",
    	"max2011": "",
		"max2012": "C:/Program Files/Autodesk/3ds Max 2012/3dsmax.exe"
	}
});


var server_port = nconf.get('webServer:port');

var appz = nconf.get('appz');



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





var getTestFullPath = function(path){
	return [__dirname, '/Tests', path].join('');
};





var testPath = [__dirname, "/Tests"].join('');


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

readTestDir(testPath);


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

			atsname = atsname || ".ats";

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

							var crashStr = "POSSIBLE CRASH";
							//console.log("cec: "+errcode);
							if(errcode == -1){
								crashStr = "CRASH";
							}

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
			}
		}
		else{
			console.log("Error: not a directory or file.");
		}

	}

}



// var testList = [
// 	{testpath: "/SphereToMesh", scriptPath: scripts, scriptName: "sphereToMesh_Export.ms", inputs: {obj: "sphere", app: "max2012"} },
// 	{testpath: "/SphereToMesh", scriptPath: scripts, scriptName: "sphereToMesh_Import.ms", inputs: {obj: "sphere", app: "max2012"} },
// 	//{testpath: "/SphereToMesh", scriptPath: scripts, scriptName: "sphereToMesh_Import.ms", inputs: {obj: "sphere"} },//XSI
// 	//{testpath: "/SphereToMesh", scriptPath: scripts, scriptName: "sphereToMesh_Import.ms", inputs: {obj: "sphere"} },//Maya
// ]


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
	return test.app + "_" + name;
}

tmProto.testComplete = function(tDesc){

	buildTestResults(tDesc.testdir, this.results, this.getAtsName(), tDesc.errcode);

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
		if(script.indexOf('/') === -1 && scripts.indexOf('\\') === -1){//the script file is in the local directory
			scriptName = script; 
		}
		else{
			var pf = splitPathAndFile(script);
			scriptPath = pf.path;
			scriptName = pf.file;
		}

		var params = {
			app: list[i].inputs.app,
			exepath: appz[list[i].inputs.app],
			testdir: list[i].testpath,
			arguments: '',
			scriptPath: scriptPath,
			scriptName: scriptName, 
			runEnvVars: runEnv,
			completionContext: this,
			completionCallback: this.testComplete
		}

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

testMan.build(testList);
//testMan.print();

testMan.runAll();



app.get('/Results', function(req, res){

	//testMan.results

 	res.render(__dirname+'/testResults.html', {results: testMan.results});


});
   












