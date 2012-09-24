
var childp = require('child_process');


var TestDesc = function(params){

	for(var v in params){
		if(!params.hasOwnProperty(v)) continue;
		this[v] = params[v];
	}
}

// --- Status:
// Not Started
// AllPassed
// Failed

var pdProto = TestDesc.prototype;
pdProto.toString = function(){
	return [
	'exepath: ', this.exepath,
	'testdir: ', this.testdir,
	'arguments: ', this.arguments,
	'maxfile: ', this.maxfile,
	'dispname: ', this.dispName
	].join(' ');
}


pdProto.execute = function(){

	var script = this.scriptPath + this.scriptName;

	var runStr = ["\"", this.exepath,"\"" , ' ', '-U MAXScript', this.arguments, ' ', script].join('');

	console.log('runStr: '+runStr);
	console.log('wdir: '+this.testdir);

	var envVar = process.env;
	
	//copy over custom environmental variables
    for(var v in this.runEnvVars){
		if(!this.runEnvVars.hasOwnProperty(v)) continue;
		envVar[v] = this.runEnvVars[v];
	}
	envVar["testPath"] = this.testdir+"/";
	envVar["testName"] = this.scriptName.substring(0, this.scriptName.length-3);

	//console.log(envVar);

	var that = this;
	this.child = childp.exec(runStr, {cwd: this.testdir, env:envVar }, function (error, stdout, stderr) {

		if (error !== null) {
			if( error.signal == 'SIGTERM'){
				
			}
			else if(error.code != 0) {

			}
			console.log('errcode: '+error.code);
			that.errcode = error.code;
		}


		console.log('stdout: '+stdout);
		console.log('stderr: '+stderr);
		console.log('status: '+envVar.status);
		that.pid = 0;

		that.completionCallback.call(that.completionContext, that);
	});

	this.pid = this.child.pid;
	this.status = 'Running';

	console.log('Starting test...');

	return true;
}

exports.TestDesc = TestDesc;
