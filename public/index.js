// Put all onload AJAX calls here, and event listeners
$(document).ready(function () {
  // On page-load AJAX Example
  // Event listener form replacement example, building a Single-Page-App, no redirects if possible
  $.ajax({
    type: "get", //Request type
    dataType: "json", //Data type - we will use JSON for almost everything
    url: "/createGedSummary", //The server endpoint we are connecting to
    success: function (data) {
      let len = Object.keys(data.files).length;
      for (i = 0; i < len; i++) {
        let obj = JSON.parse(data.files[i]);
        if (obj.message == "OK") {
          $("#fileLogPanel").html(fileLogInfo2(obj));
          $("#fileList").append(addFileToList2(obj));
        }
      }
    },
    fail: function (error) {
      // Non-200 return, do something with error
      $("#statusPanel").append(error);
    },
  });

  //upload file
  $("#uploadForm").submit(function (e) {
    let fileInput = document.getElementById("uploadFile");
    let fileName = fileInput.files[0].name;

    $.ajax({
      url: "/upload",
      type: "POST",
      data: new FormData(this),
      processData: false,
      contentType: false,
      success: function (data) {
        let len = Object.keys(data.files).length;
        let obj = JSON.parse(data.files[len - 1]);
        if (obj.message == "OK") {
          $("#fileLogPanel").html(fileLogInfo2(obj));
          $("#fileList").append(addFileToList2(obj));
        }
        $("#statusPanel").append(" >" + fileName + "File has been uploaded!");
      },
      fail: function (error) {
        // Non-200 return, do something with error
        $("#statusPanel").append(error);
      },
    });
    e.preventDefault();
  });

  //create simple gedcom
  $("#createGedcomForm").submit(function (e) {
    e.preventDefault();
    $.ajax({
      url: "/create",
      type: "get",
      data: {
        fileName: "uploads/" + $("#fileNameCSG").val(),
        source: $("#sourceCSG").val(),
        gedcVers: $("#gedcVersionCSG").val(),
        encoding: $("#encodingCSG").val(),
        submitterName: $("#submitterNameCSG").val(),
        submitterAddress: $("#submitterAddressCSG").val() + " ",
      },
      success: function (data) {
        $("#fileLogPanel").html(fileLogInfoCreate());
        $("#fileList").append(addFileToListCreate());
        $("#statusPanel").append("Successfully created GEDCOM!<br>");
        $("#statusPanel").html(repositionStatusBar());
      },
      fail: function (error) {
        $("#statusPanel").append(error);
      },
    });
  });

  //add individual
  $("#addIndivForm").submit(function (e) {
    e.preventDefault();
    $.ajax({
      url: "/add",
      type: "get",
      data: {
        fileName: document.getElementById("fileList2").options[
          document.getElementById("fileList2").selectedIndex
        ].value,
        givenName: $("#givenNameAdd").val(),
        surname: $("#surnameAdd").val(),
      },
      success: function (data) {
        $("#fileLogPanel").html(updateFileLogContents());
        $("#statusPanel").append(
          "Added a new individual " +
          $("#givenNameAdd").val() +
          " " +
          $("#surnameAdd").val() +
          " to file " +
          document.getElementById("fileList2").options[
            document.getElementById("fileList2").selectedIndex
          ].value +
          "<br>"
        );
        $("#statusPanel").html(repositionStatusBar());
      },
      fail: function (error) {
        $("#statusPanel").append(error);
      },
    });
  });

  //get descendants
  $("#getDescendantsForm").submit(function (e) {
    e.preventDefault();
    $.ajax({
      url: "/getDescendants",
      type: "get",
      data: {
        fileName: document.getElementById("fileList3").options[
          document.getElementById("fileList3").selectedIndex
        ].value,
        givenName: $("#firstNameGetDesc").val(),
        surname: $("#lastNameGetDesc").val(),
        maxGen: $("#maxGenDesc").val(),
      },
      success: function (data) {
        console.log(data);
        $("#getDescendants").empty();
        getDesc(data);
      },
      fail: function (error) {
        $("#statusPanel").append(error);
      },
    });
  });

  //get ascendants
  $("#getAncestorsForm").submit(function (e) {
    e.preventDefault();
    $.ajax({
      url: "/getAncestors",
      type: "get",
      data: {
        fileName: document.getElementById("fileList4").options[
          document.getElementById("fileList4").selectedIndex
        ].value,
        givenName: $("#firstNameGetAns").val(),
        surname: $("#lastNameGetAns").val(),
        maxGen: $("#maxGenAns").val(),
      },
      success: function (data) {
        $("#getAncestors").empty();
        getAnces(data);
      },
      fail: function (error) {
        $("#statusPanel").append(error);
      },
    });
  });

  //File List button
  $("#fileList").change(function (e) {
    $("#gedcomViewPanel").empty();
    e.preventDefault();
    $.ajax({
      url: "/getIndividualList",
      type: "get",
      data: {
        fileName: document.getElementById("fileList").options[
          document.getElementById("fileList").selectedIndex
        ].value,
      },
      success: function (data) {
        gedFileIndividualInfo(data);
      },
      fail: function (error) {
        $("#statusPanel").append(error);
      },
    });
  });

  //database login button
  document.getElementById("dataBaseSubmit").onclick = function (e) {
    e.preventDefault();
    $.ajax({
      url: "/dataBaseLogin",
      type: "get",
      data: {
        userName: document.getElementById("userName").value,
        passWord: document.getElementById("passWord").value,
        dataBase: document.getElementById("dataBase").value,
      },
      success: function (data) {
        $("#exampleModal").modal("hide");
        $("#statusPanel").append("- Successfully connected to database<br>");
        $("#statusPanel").html(repositionStatusBar());
      },
      error: function () {
        $("#userName").val("");
        $("#passWord").val("");
        $("#dataBase").val("");
        alert("Data base login failed!");
      },
    });
  };

  /*
	document.getElementById('closeDB').onclick = function (e) { 
		e.preventDefault();
		  $.ajax({
			 url:'/dbClose',
			 type:'get',
			 data: {
		   },
		   success:function(data){
			   $('#statusPanel').append("- Success: Closed DB Connection<br>");
			   $('#statusPanel').html(repositionStatusBar());
		   },
		   error:function(error){
			  $('#statusPanel').append(error);
		   }
		 });
	};*/

  //Store all files
  document.getElementById("storeAllFiles").onclick = function (e) {
    e.preventDefault();

    //FILE table data
    let fName;
    let src;
    let gVersion;
    let encode;
    let submitterName;
    let submitterAddr;
    let numIndividuals;
    let numFamilies;

    //INDIVIDUAL table data
    let givenName = [];
    let surname = [];

    //temp variables to get file name
    let fNameReplace;
    let fileName;

    //console.log(document.getElementById("gedcomViewPanel").childNodes.length);
    for (
      i = 0; i < document.getElementById("fileLogPanel").childNodes.length; i++
    ) {
      //loop through all tables in the file log panel
      if (
        document.getElementById("fileLogPanel").childNodes[i].id != undefined
      ) {
        //skip undefined (for some reason, theres 1 undefined)
        fNameReplace = document.getElementById("fileLogPanel").childNodes[i].id;
        fileName = fNameReplace.replace(".gedTable", ".ged");
        //console.log(fileName);
        fName = fileName;
        src =
          document.getElementById("fileLogPanel").childNodes[i].rows[1].cells[1]
          .innerHTML;
        gVersion =
          document.getElementById("fileLogPanel").childNodes[i].rows[1].cells[2]
          .innerHTML;
        encode =
          document.getElementById("fileLogPanel").childNodes[i].rows[1].cells[3]
          .innerHTML;
        submitterName =
          document.getElementById("fileLogPanel").childNodes[i].rows[1].cells[4]
          .innerHTML;
        submitterAddr =
          document.getElementById("fileLogPanel").childNodes[i].rows[1].cells[5]
          .innerHTML;
        numIndividuals =
          document.getElementById("fileLogPanel").childNodes[i].rows[1].cells[6]
          .innerHTML;
        numFamilies =
          document.getElementById("fileLogPanel").childNodes[i].rows[1].cells[7]
          .innerHTML;
        //for (j = 0; j < document.getElementById("fileLogPanel").childNodes.length

        //store file data
        $.ajax({
          url: "/storeFile",
          type: "get",
          data: {
            fileNames: fName,
            source: src,
            gedcVersion: gVersion,
            encoding: encode,
            submName: submitterName,
            submAddr: submitterAddr,
            numIndiv: numIndividuals,
            numFam: numFamilies,
          },
          success: function (data) {},
          error: function (err) {
            console.log(
              "Error: File failed to be stored because you are not logged into a database"
            );
          },
        });
        //get individuals
        let gedFile = "uploads/" + fileName;
        $.ajax({
          url: "/getIndividualList",
          type: "get",
          data: {
            fileName: gedFile,
          },
          success: function (data) {
            if (Object.keys(data.iList).length == 0) {
              //no individuals. no need to push any data into INDIVIDUAL table.
              console.log("No Individuals in file");
            } else {
              //There are individuals
              for (j = 0; j < Object.keys(data.iList).length; j++) {
                givenName.push(data.iList[j].givenName);
                surname.push(data.iList[j].surname);
              }

              //Send individual data to INDIVIDUAL table
              $.ajax({
                type: "get",
                url: "/storeIndividuals",
                data: {
                  fName: data.file,
                  givenNames: givenName,
                  surnames: surname,
                },
                success: function (data) {
                  console.log("Store Individuals Success Test");
                },
                fail: function (error) {
                  // Non-200 return, do something with error
                  console.log("Store Individuals Success Failed");
                  $("#statusPanel").append(error);
                },
              });
            }
            //reset values. change/remove this later
            givenName = [];
            surname = [];
          },
          fail: function (error) {
            $("#statusPanel").append(error);
          },
        });
      }
    }
    $.ajax({
      url: "/dbStatus",
      type: "get",
      data: {},
      success: function (data) {
        $("#statusPanel").append(
          "- Database has " +
          data.numFiles +
          " files and " +
          data.numIndiv +
          " individuals<br>"
        );
        $("#statusPanel").html(repositionStatusBar());
      },
      error: function (error) {
        $("#statusPanel").append(error);
        $("#statusPanel").html(repositionStatusBar());
      },
    });
  };

  document.getElementById("clearAllData").onclick = function (e) {
    e.preventDefault();

    //clear all data
    $.ajax({
      url: "/clearAllData",
      type: "get",
      data: {},
      success: function (data) {},
      error: function (error) {
        $("#statusPanel").append(error);
        $("#statusPanel").html(repositionStatusBar());
      },
    });

    //display DB status
    $.ajax({
      url: "/dbStatus",
      type: "get",
      data: {},
      success: function (data) {
        $("#statusPanel").append(
          "- Database has " +
          data.numFiles +
          " files and " +
          data.numIndiv +
          " individuals<br>"
        );
        $("#statusPanel").html(repositionStatusBar());
      },
      error: function (error) {
        $("#statusPanel").append(error);
        $("#statusPanel").html(repositionStatusBar());
      },
    });
  };

  document.getElementById("dbStatus").onclick = function (e) {
    e.preventDefault();
    $.ajax({
      url: "/dbStatus",
      type: "get",
      data: {},
      success: function (data) {
        $("#statusPanel").append(
          "- Database has " +
          data.numFiles +
          " files and " +
          data.numIndiv +
          " individuals<br>"
        );
        $("#statusPanel").html(repositionStatusBar());
      },
      error: function (error) {
        $("#statusPanel").append(error);
        $("#statusPanel").html(repositionStatusBar());
      },
    });
  };

  document.getElementById("query1").onclick = function (e) {
    e.preventDefault();
    $.ajax({
      url: "/query1",
      type: "get",
      data: {},
      success: function (data) {
        let obj = data;
        $("resultPanel").empty();
        queryOneTable(obj);
      },
      error: function (error) {
        $("#statusPanel").append(error);
        $("resultPanel").empty();
        $("resultPanel").html(
          "Query 1 Error: You are not logged into a database"
        );
        $("#statusPanel").html(repositionStatusBar());
      },
    });
  };

  document.getElementById("query2").onclick = function (e) {
    e.preventDefault();
    $.ajax({
      url: "/query2",
      type: "get",
      data: {
        fileName: document.getElementById("fileList5").options[
          document.getElementById("fileList5").selectedIndex
        ].value,
      },
      success: function (data) {
        let obj = data;
        $("resultPanel").empty();
        queryOneTable(obj);
      },
      error: function (error) {
        $("#statusPanel").append(error);
        $("resultPanel").empty();
        $("resultPanel").html(
          "Query 2 Error: You are not logged into a database"
        );
        $("#statusPanel").html(repositionStatusBar());
      },
    });
  };

  document.getElementById("query3").onclick = function (e) {
    e.preventDefault();
    $.ajax({
      url: "/query3",
      type: "get",
      data: {
        numIndiv: document.getElementById("numIndivQ3").value,
      },
      success: function (data) {
        let obj = data;
        $("resultPanel").empty();
        displayFileTable(obj);
      },
      error: function (error) {
        $("#statusPanel").append(error);
        $("resultPanel").empty();
        $("resultPanel").html(
          "Query 3 Error: Either you are not logged into a database or User input is invalid"
        );
        $("#statusPanel").html(repositionStatusBar());
      },
    });
  };

  document.getElementById("query4").onclick = function (e) {
    e.preventDefault();
    $.ajax({
      url: "/query4",
      type: "get",
      data: {
        numFamilies: document.getElementById("numFamQ4").value,
      },
      success: function (data) {
        let obj = data;
        $("resultPanel").empty();
        displayFileTable(obj);
      },
      error: function (error) {
        $("#statusPanel").append(error);
        $("resultPanel").empty();
        $("resultPanel").html(
          "Query 4 Error: Either you are not logged into a database or User input is invalid"
        );
        $("#statusPanel").html(repositionStatusBar());
      },
    });
  };

  document.getElementById("query5").onclick = function (e) {
    e.preventDefault();
    $.ajax({
      url: "/query5",
      type: "get",
      data: {
        minInd: document.getElementById("minIndivQ5").value,
        maxInd: document.getElementById("maxIndivQ5").value,
        minFam: document.getElementById("minFamQ5").value,
        maxFam: document.getElementById("maxFamQ5").value,
      },
      success: function (data) {
        let obj = data;
        $("resultPanel").empty();
        displayFileTable(obj);
      },
      error: function (error) {
        $("#statusPanel").append(error);
        $("resultPanel").empty();
        $("resultPanel").html(
          "Query 5 Error: Either you are not logged into a database or User input is invalid"
        );
        $("#statusPanel").html(repositionStatusBar());
      },
    });
  };

  document.getElementById("query6").onclick = function (e) {
    e.preventDefault();
    $.ajax({
      url: "/query6",
      type: "get",
      data: {
        queryStatement: document.getElementById("selectFormQ6").value,
      },
      success: function (data) {
        let obj = data.rows;
        let columnKeys;
        //console.log(data.rows.length);
        $("resultPanel").empty();
        if (data.rows.length > 0) {
          columnKeys = Object.keys(obj[0]);
          executeSelectTable(obj, columnKeys);
        }
      },
      error: function (error) {
        $("#statusPanel").append(error);
        $("resultPanel").empty();
        $("resultPanel").html(
          "Execute Query Error: Either you are not logged into a database or select statement is Invalid"
        );
        $("#statusPanel").html(repositionStatusBar());
      },
    });
  };

  document.getElementById("helpButton").onclick = function () {
    $("resultPanel").empty();
    displayHelpTable();
  };

  //Clear status panel button
  document.getElementById("clearStatusButton").onclick = function () {
    $("#statusPanel").empty();
  };

  document.getElementById("fileList3").onchange = function () {
    $("#getDescendants").empty();
  };

  document.getElementById("fileList4").onchange = function () {
    $("#getAncestors").empty();
  };

  // Add smooth scrolling to all links
  $("a").on("click", function (event) {
    // Make sure this.hash has a value before overriding default behavior
    if (this.hash !== "") {
      // Prevent default anchor click behavior
      event.preventDefault();

      // Store hash
      var hash = this.hash;

      // Using jQuery's animate() method to add smooth page scroll
      // The optional number (800) specifies the number of milliseconds it takes to scroll to the specified area
      $("html, body").animate({
          scrollTop: $(hash).offset().top,
        },
        800,
        function () {
          // Add hash (#) to URL when done scrolling (default click behavior)
          window.location.hash = hash;
        }
      );
    } // End if
  });
});

function displayHelpTable() {
  let table = document.createElement("TABLE");
  let row = table.insertRow(0);
  let cell1 = row.insertCell(0);
  let cell2 = row.insertCell(1);
  let cell3 = row.insertCell(2);
  let cell4 = row.insertCell(3);
  let cell5 = row.insertCell(4);
  let cell6 = row.insertCell(5);
  let cell7 = row.insertCell(6);
  let cell8 = row.insertCell(7);
  let cell9 = row.insertCell(8);

  cell1.innerHTML = "<B>file_id</B>";
  cell2.innerHTML = "<B>file_Name</B>";
  cell3.innerHTML = "<B>source</B>";
  cell4.innerHTML = "<B>version</B>";
  cell5.innerHTML = "<B>encoding</B>";
  cell6.innerHTML = "<B>sub_name</B>";
  cell7.innerHTML = "<B>sub_addr</B>";
  cell8.innerHTML = "<B>num_individials</B>";
  cell9.innerHTML = "<B>num_families</B>";

  let row2 = table.insertRow(1);
  let cell10 = row2.insertCell(0);
  let cell11 = row2.insertCell(1);
  let cell12 = row2.insertCell(2);
  let cell13 = row2.insertCell(3);
  let cell14 = row2.insertCell(4);
  let cell15 = row2.insertCell(5);
  let cell16 = row2.insertCell(6);
  let cell17 = row2.insertCell(7);
  let cell18 = row2.insertCell(8);

  //fileId, fileName, source, version, encoding, subName, subAddr, numIndividual, numFamilies
  cell10.innerHTML = "int, auto_increment, primary key, can be null"; //file id
  cell11.innerHTML = "char, max length = 60, can't be null"; //file name
  cell12.innerHTML = "char, max length = 250, can't be null"; //source
  cell13.innerHTML = "char, max length = 10, can't be null"; //version
  cell14.innerHTML = "char, max length = 10, can't be null"; //encoding
  cell15.innerHTML = "char, max length = 62, can't be null"; //submitter name
  cell16.innerHTML = "char, max length = 256"; //submitter address
  cell17.innerHTML = "int"; //number of individuals
  cell18.innerHTML = "int"; //number of families

  let table2 = document.createElement("TABLE");
  let row3 = table2.insertRow(0);
  let cell1t2 = row3.insertCell(0);
  let cell2t2 = row3.insertCell(1);
  let cell3t2 = row3.insertCell(2);
  let cell4t2 = row3.insertCell(3);
  let cell5t2 = row3.insertCell(4);
  let cell6t2 = row3.insertCell(5);

  cell1t2.innerHTML = "<B>ind_id</B>";
  cell2t2.innerHTML = "<B>surname</B>";
  cell3t2.innerHTML = "<B>given_name</B>";
  cell4t2.innerHTML = "<B>sex</B>";
  cell5t2.innerHTML = "<B>fam_size</B>";
  cell6t2.innerHTML = "<B>source_file</B>";

  let row4 = table2.insertRow(1);
  let cell10t2 = row4.insertCell(0);
  let cell11t2 = row4.insertCell(1);
  let cell12t2 = row4.insertCell(2);
  let cell13t2 = row4.insertCell(3);
  let cell14t2 = row4.insertCell(4);
  let cell15t2 = row4.insertCell(5);

  cell10t2.innerHTML = "int, auto_increment, primary key, can be null";
  cell11t2.innerHTML = "char, max length = 256, can't be null";
  cell12t2.innerHTML = "char, max length = 256, can't be null";
  cell13t2.innerHTML = "char, max length = 1";
  cell14t2.innerHTML = "int";
  cell15t2.innerHTML = "int, foreign key referencing a file_id from table FILE";

  document.getElementById("resultPanel").appendChild(table);
  document.getElementById("resultPanel").appendChild(table2);
}

function executeSelectTable(objArray, columnNames) {
  let table = document.createElement("TABLE");
  let columnNameCell;
  let columnRow = table.insertRow(0);
  for (j = 0; j < columnNames.length; j++) {
    columnNameCell = columnRow.insertCell(j);
    columnNameCell.innerHTML = "<B>" + columnNames[j] + "</B>";
  }

  for (i = 0; i < objArray.length; i++) {
    let row2 = table.insertRow(i + 1);
    let obj = objArray[i];
    for (k = 0; k < columnNames.length; k++) {
      let cell = row2.insertCell(k);
      cell.innerHTML = obj[columnNames[k]];
    }
  }
  document.getElementById("resultPanel").appendChild(table);
}

//displayFileTable
function displayFileTable(obj) {
  let table = document.createElement("TABLE");
  let row = table.insertRow(0);
  let cell1 = row.insertCell(0);
  let cell2 = row.insertCell(1);
  let cell3 = row.insertCell(2);
  let cell4 = row.insertCell(3);
  let cell5 = row.insertCell(4);
  let cell6 = row.insertCell(5);
  let cell7 = row.insertCell(6);
  let cell8 = row.insertCell(7);
  let cell9 = row.insertCell(8);

  cell1.innerHTML = "<B>file_id</B>";
  cell2.innerHTML = "<B>file_Name</B>";
  cell3.innerHTML = "<B>Source</B>";
  cell4.innerHTML = "<B>Version</B>";
  cell5.innerHTML = "<B>encoding</B>";
  cell6.innerHTML = "<B>Submitter name</B>";
  cell7.innerHTML = "<B>Submitter address</B>";
  cell8.innerHTML = "<B>Number of Individuals</B>";
  cell9.innerHTML = "<B>Number of Families</B>";

  for (i = 0; i < obj.fileId.length; i++) {
    let row2 = table.insertRow(i + 1);
    let cell10 = row2.insertCell(0);
    let cell11 = row2.insertCell(1);
    let cell12 = row2.insertCell(2);
    let cell13 = row2.insertCell(3);
    let cell14 = row2.insertCell(4);
    let cell15 = row2.insertCell(5);
    let cell16 = row2.insertCell(6);
    let cell17 = row2.insertCell(7);
    let cell18 = row2.insertCell(8);

    //fileId, fileName, source, version, encoding, subName, subAddr, numIndividual, numFamilies
    cell10.innerHTML = obj.fileId[i]; //file id
    cell11.innerHTML = obj.fileName[i]; //file name
    cell12.innerHTML = obj.source[i]; //source
    cell13.innerHTML = obj.version[i]; //version
    cell14.innerHTML = obj.encoding[i]; //encoding
    cell15.innerHTML = obj.subName[i]; //submitter name
    cell16.innerHTML = obj.subAddr[i]; //submitter address
    cell17.innerHTML = obj.numIndividual[i]; //number of individuals
    cell18.innerHTML = obj.numFamilies[i]; //number of families
  }
  document.getElementById("resultPanel").appendChild(table);
}

function queryOneTable(obj) {
  let table = document.createElement("TABLE");
  //console.log(table.id);
  let row = table.insertRow(0);
  let cell1 = row.insertCell(0);
  let cell2 = row.insertCell(1);
  let cell3 = row.insertCell(2);
  let cell4 = row.insertCell(3);
  let cell5 = row.insertCell(4);
  let cell6 = row.insertCell(5);

  cell1.innerHTML = "<B>ind_id</B>";
  cell2.innerHTML = "<B>Surname</B>";
  cell3.innerHTML = "<B>Given name</B>";
  cell4.innerHTML = "<B>Sex</B>";
  cell5.innerHTML = "<B>Family size</B>";
  cell6.innerHTML = "<B>Source file</B>";

  for (i = 0; i < obj.surname.length; i++) {
    let row2 = table.insertRow(i + 1);
    let cell9 = row2.insertCell(0);
    let cell10 = row2.insertCell(1);
    let cell11 = row2.insertCell(2);
    let cell12 = row2.insertCell(3);
    let cell13 = row2.insertCell(4);
    let cell14 = row2.insertCell(5);

    cell9.innerHTML = obj.id[i]; //ind_id
    cell10.innerHTML = obj.surname[i]; //surname
    cell11.innerHTML = obj.givenName[i]; //given name
    cell12.innerHTML = "NULL"; //sex
    cell13.innerHTML = "0"; //family size
    cell14.innerHTML = obj.source[i]; //source file
  }
  document.getElementById("resultPanel").appendChild(table);
}

//display gedcomViewPanel's list of individuals
function gedFileIndividualInfo(objList) {
  if (Object.keys(objList.iList).length == 0) {
    document.getElementById("gedcomViewPanel").innerHTML = "No Individuals<br>";
  } else {
    for (i = 0; i < Object.keys(objList.iList).length; i++) {
      let table = document.createElement("TABLE");
      let row = table.insertRow(0);
      let cell1 = row.insertCell(0);
      let cell2 = row.insertCell(1);
      let cell3 = row.insertCell(2);
      let cell4 = row.insertCell(3);
      cell1.innerHTML = "<B>Given name</B>";
      cell2.innerHTML = "<B>Surname</B>";
      cell3.innerHTML = "<B>Sex</B>";
      cell4.innerHTML = "<B>Family size</B>";

      let row2 = table.insertRow(1);
      let cell5 = row2.insertCell(0);
      let cell6 = row2.insertCell(1);
      let cell7 = row2.insertCell(2);
      let cell8 = row2.insertCell(3);
      cell5.innerHTML = objList.iList[i].givenName;
      cell6.innerHTML = objList.iList[i].surname;
      cell7.innerHTML = "N/A";
      cell8.innerHTML = "N/A";
      document.getElementById("gedcomViewPanel").appendChild(table);
    }
  }
}

function updateFileLogContents() {
  let fName = document
    .getElementById("fileList2")
    .options[document.getElementById("fileList2").selectedIndex].value.replace(
      "uploads/",
      ""
    );
  console.log(fName);
  let fileNameId = fName + "Table";
  let currentCount =
    document.getElementById(fileNameId).rows[1].cells[6].innerHTML;
  //console.log(document.getElementById(fileNameId).id);
  //console.log(currentCount);
  let newCount = parseInt(currentCount) + 1;
  console.log(newCount);
  let tableID = document.getElementById(fileNameId);
  tableID.rows[1].cells[6].innerHTML = newCount;
  console.log(document.getElementById(fileNameId).rows[1].cells[6].innerHTML);
}

function addFileToListCreate() {
  let fName = "uploads/" + document.getElementById("fileNameCSG").value;
  //add to list #1 in gedcom view panel
  let newOption = document.createElement("option");
  newOption.id = document.getElementById("fileNameCSG").value;
  newOption.value = fName;
  newOption.innerHTML = document.getElementById("fileNameCSG").value;
  document.getElementById("fileList").options.add(newOption);

  //add to list #2 in add individuals
  let newOption2 = document.createElement("option");
  newOption2.value = fName;
  newOption2.innerHTML = document.getElementById("fileNameCSG").value;
  document.getElementById("fileList2").options.add(newOption2);

  //add to list #3 in add individuals
  let newOption3 = document.createElement("option");
  newOption3.value = fName;
  newOption3.innerHTML = document.getElementById("fileNameCSG").value;
  document.getElementById("fileList3").options.add(newOption3);

  //add to list #4 in add individuals
  let newOption4 = document.createElement("option");
  newOption4.value = fName;
  newOption4.innerHTML = document.getElementById("fileNameCSG").value;
  document.getElementById("fileList4").options.add(newOption4);

  //add to list #4 in add individuals
  let newOption5 = document.createElement("option");
  newOption5.value = fName;
  newOption5.innerHTML = document.getElementById("fileNameCSG").value;
  document.getElementById("fileList5").options.add(newOption5);
}
//display gedcom header's information
function fileLogInfoCreate() {
  let table = document.createElement("TABLE");
  table.setAttribute(
    "id",
    document.getElementById("fileNameCSG").value + "Table"
  );
  let row = table.insertRow(0);
  let cell1 = row.insertCell(0);
  let cell2 = row.insertCell(1);
  let cell3 = row.insertCell(2);
  let cell4 = row.insertCell(3);
  let cell5 = row.insertCell(4);
  let cell6 = row.insertCell(5);
  let cell7 = row.insertCell(6);
  let cell8 = row.insertCell(7);
  cell1.innerHTML = "<B>File name (click to download)</B>";
  cell2.innerHTML = "<B>Source</B>";
  cell3.innerHTML = "<B>GEDC version</B>";
  cell4.innerHTML = "<B>encoding</B>";
  cell5.innerHTML = "<B>Submitter name</B>";
  cell6.innerHTML = "<B>Submitter address</B>";
  cell7.innerHTML = "<B>Number of individuals</B>";
  cell8.innerHTML = "<B>Number of families</B>";

  let row2 = table.insertRow(1);
  let cell9 = row2.insertCell(0); //file download link
  let cell10 = row2.insertCell(1); //source
  let cell11 = row2.insertCell(2); //gedc version
  let cell12 = row2.insertCell(3); //encoding
  let cell13 = row2.insertCell(4); //submitter name
  let cell14 = row2.insertCell(5); //submitter address
  let cell15 = row2.insertCell(6); //num individuals
  let cell16 = row2.insertCell(7); //num families

  let fName = "/uploads/" + document.getElementById("fileNameCSG").value;
  cell9.innerHTML =
    '<a href="' +
    fName +
    '">' +
    document.getElementById("fileNameCSG").value +
    "</a>";
  cell10.innerHTML = "PAF";
  cell11.innerHTML = "5.5";
  cell12.innerHTML = "ANSEL";
  cell13.innerHTML = document.getElementById("submitterNameCSG").value;
  cell14.innerHTML = document.getElementById("submitterAddressCSG").value;
  cell15.innerHTML = "0";
  cell16.innerHTML = "0";
  document.getElementById("filelogPanel").appendChild(table);
}

function addFileToList2(obj) {
  if (obj.message == "OK") {
    let fName = obj.fileName.replace("uploads/", "");
    //add to list #1 in gedcom view panel
    let newOption = document.createElement("option");
    newOption.id = obj.fileName;
    newOption.value = obj.fileName;
    newOption.innerHTML = fName;
    document.getElementById("fileList").options.add(newOption);

    //add to list #2 in add individuals
    let newOption2 = document.createElement("option");
    newOption2.value = obj.fileName;
    newOption2.innerHTML = fName;
    document.getElementById("fileList2").options.add(newOption2);

    //add to list #3 in add individuals
    let newOption3 = document.createElement("option");
    newOption3.value = obj.fileName;
    newOption3.innerHTML = fName;
    document.getElementById("fileList3").options.add(newOption3);

    //add to list #4 in add individuals
    let newOption4 = document.createElement("option");
    newOption4.value = obj.fileName;
    newOption4.innerHTML = fName;
    document.getElementById("fileList4").options.add(newOption4);

    //add to list #4 in add individuals
    let newOption5 = document.createElement("option");
    newOption5.value = obj.fileName;
    newOption5.innerHTML = fName;
    document.getElementById("fileList5").options.add(newOption5);
  }
}
//display gedcom header's information
function fileLogInfo2(obj) {
  if (obj.message == "OK") {
    let fName = obj.fileName.replace("uploads/", "");

    let table = document.createElement("TABLE");
    table.setAttribute("id", fName + "Table");
    let row = table.insertRow(0);
    let cell1 = row.insertCell(0);
    let cell2 = row.insertCell(1);
    let cell3 = row.insertCell(2);
    let cell4 = row.insertCell(3);
    let cell5 = row.insertCell(4);
    let cell6 = row.insertCell(5);
    let cell7 = row.insertCell(6);
    let cell8 = row.insertCell(7);
    cell1.innerHTML = "<B>File name (click to download)</B>";
    cell2.innerHTML = "<B>Source</B>";
    cell3.innerHTML = "<B>GEDC version</B>";
    cell4.innerHTML = "<B>encoding</B>";
    cell5.innerHTML = "<B>Submitter name</B>";
    cell6.innerHTML = "<B>Submitter address</B>";
    cell7.innerHTML = "<B>Number of individuals</B>";
    cell8.innerHTML = "<B>Number of families</B>";

    let row2 = table.insertRow(1);
    let cell9 = row2.insertCell(0);
    let cell10 = row2.insertCell(1);
    let cell11 = row2.insertCell(2);
    let cell12 = row2.insertCell(3);
    let cell13 = row2.insertCell(4);
    let cell14 = row2.insertCell(5);
    let cell15 = row2.insertCell(6);
    let cell16 = row2.insertCell(7);

    cell9.innerHTML = '<a href="' + obj.fileName + '">' + fName + "</a>";
    cell10.innerHTML = obj.source;
    cell11.innerHTML = obj.GEDCversion;
    cell12.innerHTML = obj.encoding;
    cell13.innerHTML = obj.subName;
    cell14.innerHTML = obj.subAddress;
    cell15.innerHTML = obj.numIndividuals;
    cell16.innerHTML = obj.numFamilies;
    document.getElementById("filelogPanel").appendChild(table);
  }
}

function getAnces(list) {
  let table = document.createElement("TABLE");
  if (Object.keys(list.gList).length == 0) {
    document.getElementById("getAncestors").innerHTML = "No Ancestors<br>";
  } else {
    for (i = 0; i < Object.keys(list.gList).length; i++) {
      let row = table.insertRow(i);
      let ilist = list.gList[i];
      console.log(Object.keys(ilist).length);
      for (j = 0; j < Object.keys(ilist).length; j++) {
        let cell1 = row.insertCell(j);
        cell1.innerHTML = ilist[j].givenName + " " + ilist[j].surname;
      }
    }
    document.getElementById("getAncestors").appendChild(table);
  }
}

function getDesc(list) {
  let table = document.createElement("TABLE");
  if (Object.keys(list.gList).length == 0) {
    document.getElementById("getDescendants").innerHTML = "No Descendants<br>";
  } else {
    for (i = 0; i < Object.keys(list.gList).length; i++) {
      let row = table.insertRow(i);
      let ilist = list.gList[i];
      console.log(Object.keys(ilist).length);
      for (j = 0; j < Object.keys(ilist).length; j++) {
        let cell1 = row.insertCell(j);
        cell1.innerHTML = ilist[j].givenName + " " + ilist[j].surname;
      }
    }
    document.getElementById("getDescendants").appendChild(table);
  }
}

function repositionStatusBar() {
  let objDiv = document.getElementById("statusPanel"); //reposition status panel scrollbar to the bottom
  objDiv.scrollTop = objDiv.scrollHeight;
}

jQuery(document).ready(function ($) {
  jQuery(window).scroll(function () {
    if ($(window).scrollTop() >= 700) {
      $("#nav_bar").fadeIn();
    } else {
      $("#nav_bar").fadeOut();
    }
  });
});