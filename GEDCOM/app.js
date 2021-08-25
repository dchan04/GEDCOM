'use strict'

// C library API
const ffi = require('ffi');

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const cors = require("cors");
const formidable = require('formidable');
const fileUpload = require('express-fileupload');

app.use(cors());
app.use(fileUpload());


//MYSQL connection
const mysql = require('mysql');

let connection;


// Minimization
const fs = require('fs');
const JavaScriptObfuscator = require('javascript-obfuscator');

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = 3000;

// Send HTML at root, do not change
app.get('/',function(req,res){
  res.sendFile(path.join(__dirname+'/public/home.html'));
});

// Send Style, do not change
app.get('/style.css',function(req,res){
  //Feel free to change the contents of style.css to prettify your Web app
  res.sendFile(path.join(__dirname+'/public/style.css'));
});

// Send obfuscated JS, do not change
app.get('/index.js',function(req,res){
  fs.readFile(path.join(__dirname+'/public/index.js'), 'utf8', function(err, contents) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {compact: true, controlFlowFlattening: true});
    res.contentType('application/javascript');
    res.send(minimizedContents._obfuscatedCode);
  });
});

//Respond to POST requests that upload files to uploads/ directory
app.post('/upload', function(req, res) {
  if(!req.files) {
    return res.status(400).send('No files were uploaded.');
  }
 
  let uploadFile = req.files.uploadFile;

  // Use the mv() method to place the file somewhere on your server
	uploadFile.mv('uploads/' + uploadFile.name, function(err) {
    if(err) {
      return res.status(500).send(err);
    }
	let files = [];
	let obj;
	let jsonObj;
	
	fs.readdirSync('uploads/').forEach(file => {
			let fileN = 'uploads/' + file;
			obj = sharedLib.gedcomToJSON(fileN);
			jsonObj = JSON.parse(obj);
			//console.log("Error Messege:" + jsonObj.message);
			if (jsonObj.message == "OK") {
				if (files.indexOf(obj) < 0) {
					files.push(obj);
					//console.log("File Success");
				}
			}
			else {
				//console.log("Invalid File Found");
			}
	});
	res.send({files});
  });
});

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function(req , res){
  fs.stat('uploads/' + req.params.name, function(err, stat) {

    if(err == null) {
      res.sendFile(path.join(__dirname+'/uploads/' + req.params.name));
    } else {
      res.send('');
    }
  });
});

//******************** Your code goes here ******************** 
let sharedLib = ffi.Library('./sharedLib', {
  'gedcomToJSON': [ 'string', [ 'string' ] ],  //return type first, argument list second
  'createGEDCOMFile': [ 'string', [ 'string', 'string' ] ],
  'callAddIndividual': [ 'string', [ 'string', 'string' ] ],
  'getDescendantWrapper': [ 'string', [ 'string', 'string', 'string', 'int' ] ],
  'getAncestorsWrapper': [ 'string', [ 'string', 'string', 'string', 'int' ] ],
  'getListOfIndividuals': [ 'string', [ 'string' ] ],
});

//Sample endpoint
app.get('/createGedSummary', function(req , res){
  let files = [];
  let errMsg;
  let obj;
  let jsonObj;
  
  fs.readdirSync('uploads/').forEach(file => {
	    let fileN = 'uploads/' + file;
		obj = sharedLib.gedcomToJSON(fileN);
		jsonObj = JSON.parse(obj);
		//console.log("Error Messege:" + jsonObj.message);
		if (jsonObj.message == "OK") {
			 if (files.indexOf(obj) < 0) {
				files.push(obj);
				//console.log("File Success");
			 }
		}
		else {
			//console.log("Invalid File Found");
		}
  });
  res.send({files});
});
app.get('/create', function(req , res){
  let source = req.query.source;
  let obj = {"source":source, "gedcVersion":req.query.gedcVers, "encoding":req.query.encoding, "subName":req.query.submitterName ,"subAddress": req.query.submitterAddress};
  let objJSON = JSON.stringify(obj);
  //console.log(objJSON);
  let message = sharedLib.createGEDCOMFile(req.query.fileName, objJSON);
  //console.log(message);
  res.send({
	  msg: message
  });
});

app.get('/add', function(req , res){
  let individualObj = {"givenName":req.query.givenName, "surname":req.query.surname};
  let indiJSON = JSON.stringify(individualObj);
  //console.log(indiJSON);
  let msg = sharedLib.callAddIndividual(req.query.fileName, indiJSON);
  res.send({
	  foo: " "
  });
});

app.get('/getDescendants', function(req , res){
  let givenName = req.query.givenName;
  let surname = req.query.surname;
  let fileName = req.query.fileName;
  let maxGeneration = req.query.maxGen;
  if (maxGeneration == "") {
	  maxGeneration = '0';
  }
  let gList = JSON.parse(sharedLib.getDescendantWrapper(givenName, surname, fileName, maxGeneration));
  res.send({
	  gList
  });
});

app.get('/getAncestors', function(req , res){
  let givenName = req.query.givenName;
  let surname = req.query.surname;
  let fileName = req.query.fileName;
  let maxGeneration = req.query.maxGen;
  if (maxGeneration == "") {
	  maxGeneration = '0';
  }
  let gList = JSON.parse(sharedLib.getAncestorsWrapper(givenName, surname, fileName, maxGeneration));
  res.send({
	  gList
  });
});

app.get('/getIndividualList', function(req , res){
  //console.log(req.query.fileName);
  let iList = JSON.parse(sharedLib.getListOfIndividuals(req.query.fileName));
  res.send({
	  iList, file: req.query.fileName,
  });
});

//database connection
app.get('/dataBaseLogin', function(req , res){
	connection = mysql.createConnection({
		host : 'localhost',
		user : req.query.userName,
		password : req.query.passWord, 
		database : req.query.dataBase
	});
  connection.connect(function(err){ 
    if (err) {
		console.log(err);
        res.status(400).send('Fail');
	}
    else {
        //create File table
		connection.query("create table FILE (file_id int auto_increment,  file_Name varchar(60) not null, source varchar(250) not null, version varchar(10) not null, encoding varchar(10) not null, sub_name varchar(62) not null, sub_addr varchar(256), num_individials int, num_families int, primary key(file_id) )", function (err, rows, fields) {
			if (err) {
				console.log("table FILE already exists: . "+err);
			}
		});
		
		//create INDIVIDUAL table
		connection.query("create table INDIVIDUAL (ind_id int auto_increment, surname varchar(256) not null, given_name varchar(256) not null, sex varchar(1), fam_size int, source_file int, primary key(ind_id), foreign key (source_file) references FILE(file_id));", function (err, rows, fields) {
			if (err) {
				console.log("table INDIVIDUAL already exists: . "+err);
			}
		});
		res.status(200).send('Login Success');
    }
  });
});
/*
//database disconnection
app.get('/dbClose', function(req , res){
  connection.end();
  res.send('');
});*/

let i;
let numFam;
let numInd;
//storeFile
app.get('/storeFile', function(req , res){
	numFam = parseInt(req.query.numFam);
	numInd = parseInt(req.query.numIndiv);
	connection.query("insert into FILE (file_id, file_Name, source, version, encoding, sub_name, sub_addr, num_individials, num_families) values (null, '"
		+ req.query.fileNames + "', '" + req.query.source + "', '" + req.query.gedcVersion + "', '" + req.query.encoding + "', '" + req.query.submName +
		"', '" + req.query.submAddr + "', " + numInd + ", " + numFam + ");", function (err, rows, fields) {
			if (err) {
				//console.log("Test insert into FILE failed: . "+err);
				res.status(500).send('Fail');
			}
			else {
				res.send(req.query.fileNames);
			}
	});
	//res.send(req.query.fileNames);
});

//store all individuals in a file
app.get('/storeIndividuals', function(req , res){
	let fileName = req.query.fName.replace("uploads/", "");
	let sqlQuary = "select file_id from FILE where file_name = '" + fileName + "';";
	connection.query(sqlQuary, function (err, rows, fields) {
		if (err) {
			console.log("Something went wrong with storing individuals form a file: . "+err);
		}
		else { //getting file ID successful
			//console.log("ID: "+rows[0].file_id); 
			for (i = 0; i < req.query.givenNames.length; i++) {
				connection.query("insert into INDIVIDUAL (ind_id, surname, given_name, sex, fam_size, source_file)"
				+ "values (null, '" + req.query.surnames[i] + "', '" + req.query.givenNames[i] + "', null, 0, " + rows[0].file_id + ");", 
				function (err, rows, fields) {
					if (err) {
						//console.log("Test insert into INDIVIDUAL failed: . "+err);
					}
					else {
						//console.log("file name: "+fileName); 
					}
				});
			}
		}
	});
	res.send('');
});

app.get('/clearAllData', function(req , res){
	connection.query("delete from INDIVIDUAL;", function (err, rows, fields) {
			if (err) {
				res.status(500).send('Fail: You are not logged into a database');
			}
			else {
				
			}
	});
	connection.query("delete from FILE;", function (err, rows, fields) {
		if (err) {
			res.status(500).send('Fail: You are not logged into a database');
		}
		else {
			res.send('Success');
		}
	});
});
let numIndiv;
app.get('/dbStatus', function(req , res){

	connection.query("select count(*) as num_individuals from INDIVIDUAL;", function (err, rows, fields) {
				if (err) {
					res.status(500).send('Fail: You are not logged into a database');
				}
				else {
					numIndiv = rows[0].num_individuals;
				}
		});
	connection.query("select count(*) as num_files from FILE;", function (err, rows, fields) {
			if (err) {
				res.status(500).send('Fail: You are not logged into a database');
			}
			else {
				res.send({
						numFiles: rows[0].num_files, numIndiv
				});
			}
	});
});

app.get('/query1', function(req , res){
	let row;
	let id = [];
	let surname = [];
	let givenName = [];
	let source = [];
	connection.query("select * from INDIVIDUAL order by surname;", function (err, rows, fields) {
				if (err) {
					res.status(500).send('Fail: You are not logged into a database');
				}
				else {
					for (row of rows) {
						id.push(row.ind_id);
						surname.push(row.surname);
						givenName.push(row.given_name);
						source.push(row.source_file);
					}
					res.send({id, surname, givenName, source});
				}
	});
});

let sourceId;
app.get('/query2', function(req , res){
	let row;
	let id = [];
	let surname = [];
	let givenName = [];
	let source = [];
	let fName = req.query.fileName.replace("uploads/", "");
	connection.query("select file_id from FILE where file_Name = '" + fName + "';", function (err, rows, fields) {
		if (err){
			res.status(500).send('Fail: You are not logged into a database');
		}
		else {
			if (rows.length > 0) { //If SQL returns something
				sourceId = rows[0].file_id;
				connection.query("select * from INDIVIDUAL where source_file = " + sourceId + ";", function (err, rows, fields) {
					if (err) {
						res.status(500).send('Fail: You are not logged into a database');
					}
					else {
						for (row of rows) {
							id.push(row.ind_id);
							surname.push(row.surname);
							givenName.push(row.given_name);
							source.push(row.source_file);
						}
						res.send({id, surname, givenName, source});
					}
				});
			}
			else {
				res.send({id, surname, givenName, source});
			}
		}
	});
});


app.get('/query3', function(req , res){
	let row;
	let fileId = [];
	let fileName = [];
	let source = [];
	let version = [];
	let encoding = [];
	let subName = [];
	let subAddr = [];
	let numIndividual = [];
	let numFamilies = [];
	connection.query("select * from FILE where num_individials = " + req.query.numIndiv + ";", function (err, rows, fields) {
		if (err) {
			res.status(500).send('Fail: You are not logged into a database');
		}
		else {
			for (row of rows) {
				fileId.push(row.file_id);
				fileName.push(row.file_Name);
				source.push(row.source);
				version.push(row.version);
				encoding.push(row.encoding);
				subName.push(row.sub_name);
				subAddr.push(row.sub_addr);
				numIndividual.push(row.num_individials);
				numFamilies.push(row.num_families);
			}
			res.send({fileId, fileName, source, version, encoding, subName, subAddr, numIndividual, numFamilies});
		}
	});
});

app.get('/query4', function(req , res){
	let row;
	let fileId = [];
	let fileName = [];
	let source = [];
	let version = [];
	let encoding = [];
	let subName = [];
	let subAddr = [];
	let numIndividual = [];
	let numFamilies = [];
	connection.query("select * from FILE where num_families = " + req.query.numFamilies + ";", function (err, rows, fields) {
		if (err) {
			res.status(500).send('Fail: You are not logged into a database');
		}
		else {
			for (row of rows) {
				fileId.push(row.file_id);
				fileName.push(row.file_Name);
				source.push(row.source);
				version.push(row.version);
				encoding.push(row.encoding);
				subName.push(row.sub_name);
				subAddr.push(row.sub_addr);
				numIndividual.push(row.num_individials);
				numFamilies.push(row.num_families);
			}
			res.send({fileId, fileName, source, version, encoding, subName, subAddr, numIndividual, numFamilies});
		}
	});
});


app.get('/query5', function(req , res){
	let row;
	let fileId = [];
	let fileName = [];
	let source = [];
	let version = [];
	let encoding = [];
	let subName = [];
	let subAddr = [];
	let numIndividual = [];
	let numFamilies = [];
	
	connection.query("select * from FILE where num_individials >= " + req.query.minInd + " and num_individials <= " + req.query.maxInd + " and num_families >= " + req.query.minFam + " and num_families <= " + req.query.maxFam + ";", function (err, rows, fields) {
		if (err) {
			res.status(500).send('Fail: You are not logged into a database');
		}
		else {
			for (row of rows) {
				fileId.push(row.file_id);
				fileName.push(row.file_Name);
				source.push(row.source);
				version.push(row.version);
				encoding.push(row.encoding);
				subName.push(row.sub_name);
				subAddr.push(row.sub_addr);
				numIndividual.push(row.num_individials);
				numFamilies.push(row.num_families);
			}
			res.send({fileId, fileName, source, version, encoding, subName, subAddr, numIndividual, numFamilies});
		}
	});
});

app.get('/query6', function(req , res){
	connection.query("select " + req.query.queryStatement, function (err, rows, fields) {
		if (err) {
			res.status(500).send('Fail: You are not logged into a database');
		}
		else {
			res.send({rows});
		}
	});
});
app.listen(portNum);
console.log('Running app at localhost: ' + portNum);