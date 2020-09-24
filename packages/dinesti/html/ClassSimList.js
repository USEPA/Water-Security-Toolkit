// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

//
// 
//
function SimList(nPanel,dataInFrontOf,parentOnMouseOver) {
	this.base = DataGrid;
	this.base(30, 40, 200, 300);
	this.nPanel = nPanel;
	this.dataInFrontOf = dataInFrontOf;
	this.parentOnMouseOver = parentOnMouseOver;
	this.sParentId = "g" + nPanel + "i";
	this.gid = "g" + nPanel + "s";
	this.uniqueString = "SimList";
	this.nColumns = 2;
	this.colNames = [];
	this.colNames.push("name");
	this.colNames.push("status");
	this.colTitles = [];
	this.colTitles.push("Name");
	this.colTitles.push("Status");
	this.colWidth = [105];
	this.textFill = [];
	this.textFill.push(d3.rgb(0,0,0));
	this.textFill.push(function(d,i){ 
		switch(d.value.status) {
			case "New":
				return d3.rgb(130,130,130);
			case "Running":
				return d3.rgb(0,0,130);
			case "Complete":
				return d3.rgb(0,130,0);
			case "Modified":
				return d3.rgb(130,0,0);
			case "Saved":
				return d3.rgb(0,0,0);
			case "Error":
				return d3.rgb(130,0,0);
			default:
				return d3.rgb(0,0,0);
		}
	});
	//
	d3.select("#g" + panelIndex).append("g").attr("id","g" + panelIndex + "i").style("visibility","hidden");
	//
	this.createInputs();
	this.disableInputs(true);
}

SimList.prototype = new DataGrid;

SimList.prototype.createInputs = function() {
	var m_this = this;
	//
	this.createButtons("New"   ,  30, 360, 50);
	this.createButtons("Run"   ,  80, 360, 50);
	this.createButtons("Stop"  , 130, 360, 50);
	this.createButtons("Delete", 180, 360, 50);
	this.createButtons("View"  ,  80, 380, 50);
	//
	var difo = this.dataInFrontOf;
	var sid = "svg" + this.nPanel;
	var x = 255;
	var y =  60;
	var w =  50;
	var h =  20;
	var i =   0;
	this.labelName     = new Label      ("Name:"           , sid, "labelSimName"    , difo, x, y+h*i++, w, h);
	this.labelNetwork  = new Label      ("Network:"        , sid, "labelSimNetwork" , difo, x, y+h*i++, w, h);
	this.labelNode     = new Label      ("Event node:"     , sid, "labelSimNode"    , difo, x, y+h*i++, w, h);
	this.labelType     = new Label      ("Event type:"     , sid, "labelSimType"    , difo, x, y+h*i++, w, h);
	this.labelStrength = new Label      ("Ev strength:"    , sid, "labelSimStrength", difo, x, y+h*i++, w, h);
	this.labelStart    = new Label      ("Ev start (h:m):" , sid, "labelSimStart"   , difo, x, y+h*i++, w, h);
	this.labelEnd      = new Label      ("Ev end (h:m):"   , sid, "labelSimEnd"     , difo, x, y+h*i++, w, h);
	//
	var gid = "g" + this.nPanel + "i";
	var x = 330;
	var y =  60;
	var w = 100;
	var i =   0;
	this.textName      = new Textbox    (gid, "textSimName"     , difo, x, y+h*i++, w);
	this.listNetwork   = new NetworkList(gid, "listSimNetwork"  , difo, x, y+h*i++, w+7.5);
	this.listInjNode   = new Dropdown   (gid, "listInjNode"     , difo, x, y+h*i++, w+7.5, true);
	this.listInjType   = new Dropdown   (gid, "listInjType"     , difo, x, y+h*i++, w+7.5, true);
	this.textStrength  = new Textbox    (gid, "textSimStrength" , difo, x, y+h*i++, w);
	this.textStart     = new Timebox    (gid, "textSimStart"    , difo, x, y+h*i++, w);
	this.textEnd       = new Timebox    (gid, "textSimEnd"      , difo, x, y+h*i++, w);
	//
	this.textStrength.setPlaceHolderText("100");
	this.textStart   .setPlaceHolderText("0:00");
	this.textEnd     .setPlaceHolderText("hr:min");
	//
	this.d3grouppath = d3.select("#svg" + this.nPanel).append("path").attr("id","pathSimInputs").attr("data-InFrontOf",difo).attr("d","M245,50 v165 h203 v-165 Z").attr("stroke","rgba(49,49,49,0.5)").attr("fill","none");
	//
	this.createButtons("Save", 320, 220, 50);
	//
	this.updateNetworkList();
	//
	this.listInjNode.updateData = function(network) {
		var uuid = network.getSelectedAttr("data-id");
		var fname = network.getSelectedAttr("data-jsonfile");
		if (uuid && fname) {
			Couch.GetFile(uuid, fname, function(data) {
				var list = (data && data.nodeList) ? data.nodeList : "<select></select>";
				var div = document.createElement("div");
				div.innerHTML = list;
				var newList = div.firstChild;
				m_this.listInjNode.replaceList(newList);
				d3.select(div).remove();
				m_this.addListenersForSelection("#" + m_this.listInjNode.sid);
				var bHasSelection = m_this.hasSelection();
				var index = m_this.selectedIndex;
				var sVal = m_this.data[index].value.input_TSG[0].a0;
				m_this.listInjNode.selectValue(m_this.data && m_this.data[index] && bHasSelection ? sVal : "");
			});
		} else {
			m_this.listInjNode.updateList([]);
		}
	}
	//
	this.listInjType.updateList([{"text":"Mass","value":"MASS"},{"text":"Flow paced","value":"FLOWPACED"}]);
}

SimList.prototype.createButtons = function(sText, dLeft, dTop, dWidth) {
	d3.select("#" + this.sParentId).append("g").attr("id","g" + this.nPanel + sText + "SimButton")
			.attr("data-InFrontOf",this.dataInFrontOf)
			.style("position","absolute")
			.style("left",convertToPx(dLeft))
			.style("top",convertToPx(dTop))
		.append("button")
			.attr("data-InFrontOf",this.dataInFrontOf)
			.attr("id","m_" + sText + "SimButton")
			.style("width",convertToPx(dWidth))
			.text(sText)
			;
}

SimList.prototype.updateData = function(arg1) {
	var m_this = this;
	var bFirstTime = null;
	if (typeof arg1 === "boolean") bFirstTime = arg1;
	var fNext = null;
	if (typeof arg1 === "function") fNext = arg1;
	Couch.GetView(Couch.SimListHide, function(data) {
		m_this.data = data ? data.rows : [];
		if (bFirstTime) {
			m_this.createDataGrid();
		} else {
			m_this.updateDataGrid();
		}
	});
};

SimList.prototype.updateNetworkList = function() {
	var selectedData = this.getSelectedData();
	var uuid = (selectedData == null) ? "" : selectedData.value.docFile_INP.docId;
	this.listNetwork.updateData(uuid);
}

SimList.prototype.updateLabels = function() {
	this.labelName    .update();
	this.labelNetwork .update();
	this.labelNode    .update();
	this.labelType    .update();
	this.labelStrength.update();
	this.labelStart   .update();
	this.labelEnd     .update();
}

SimList.prototype.checkSelectionChanged = function() {
	var oldId = this.selectedDocId;
	var index = this.selectedIndex;
	var newId = "";
	if (index > -1 && index < this.data.length)
		newId = this.data[index].id;
	this.selectionChanged = (oldId != newId);
	this.selectedDocId = newId;
	return this.selectionChanged;
}

SimList.prototype.viewNetwork = function(networkView, bInitialize) {
	var m_this = this;
	m_Waiting.show();
	var index = this.selectedIndex;
	var uuid = this.data[index].id;
	var nodeId = this.data[index];
	var nodeId = this.data[index].value.selectedNode;
	if (bInitialize) networkView.m_NodeGraph.initialize();
	networkView.m_NodeGraph.setNodeId(nodeId);
	networkView.m_NodeGraph.setSimId(uuid);
	networkView.m_NodeGraph.getErdData(function(data) {
		var uuid = m_this.data[index].value.docFile_INP.docId;
		Couch.GetDoc(uuid, function(data) {
			if (m_NetworkView.isVisible()) {
				m_Waiting.hide();
			} else {
				var dotSize = data.dotSize;
				var drawTransform = data.drawTransform;
				var map = data.map;
				//var toggle = data.toggle;
				var jsonFile = data.jsonFile;
				Couch.GetFile(uuid, jsonFile, function(data) {
					data.m_SimList = true;
					data.dotSize = dotSize;
					data.drawTransform = drawTransform;
					data.map = map;
					//data.toggle = toggle;
					data.uuid = uuid;
					networkView.viewNetwork(data);
					m_Waiting.hide();
				});
			}
		});
	});
}	

SimList.prototype.checkIncomplete = function(data) {
	// right now this is only the data passed in from the "view" call
	if (data == null) return true;
	if (data.name == null) return true;
	if (data.name.length == 0) return true;
	if (data.docFile_INP == null) return true;
	if (data.docFile_INP.docId == null) return true;
	if (data.docFile_INP.docId.length == 0) return true;
	if (data.docFile_INP.fileName == null) return true;
	if (data.docFile_INP.fileName.length == 0) return true;
	if (data.input_TSG == null) return true;
	if (data.input_TSG.length == 0) return true;
	if (data.input_TSG[0].a0 == null) return true;
	if (data.input_TSG[0].a0.length == 0) return true;
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
SimList.prototype.onMouseMove = function(sid) {
	DataGrid.prototype.onMouseMove.call(this,sid);
}

SimList.prototype.onMouseOver = function(sid) {
	DataGrid.prototype.onMouseOver.call(this,sid);
}

SimList.prototype.onMouseOut = function(sid) {
	DataGrid.prototype.onMouseOut.call(this,sid);
}

SimList.prototype.onMouseUp = function(sid) {
	m_NetworkView.onMouseUp();
	var m_this = this;
	switch(sid) {
		case "m_NewSimButton":
			var data = {
				"status": "New",
				"statusOld": "",
				"m_SimList": true,
				"name": "S" + ("" + new Date().getTime()).substring(3),
				"docFile_INP": {"docId":"","fileName":""},
				"input_TSG": [{"a0":"","a1":"","a2":null,"a3":null,"a4":null}],
				"Date": new Date()
				};
			Couch.createUniqueId(this, function(uuid) {
				Couch.setDoc(this, uuid, data, function(e,res) {
					this.selectLastRow();
				});
			});
			break;
		case "m_RunSimButton":
			if (!m_this.getSelectedData()) return;
			var index = this.selectedIndex;
			var uuid = this.data[index].id;
			Couch.setValue(this, uuid, "status", "Running", function(data) {
				Couch.getDoc(this, Couch.tevasim + "?call=run&uuid=" + uuid, function(data) {
					var results = data;
				});
			});
			break;
		case "m_StopSimButton":
			if (!m_this.getSelectedData()) return;
			var index = this.selectedIndex;
			var uuid = this.data[index].id;
			var pid = this.data[index].value.pid;
			Couch.GetDoc(Couch.kill + "?pid=" + pid, function(data) {
				Couch.SetValue(uuid, "status", "Stopped", function(e,res) {
					var data = (res && res.data) ? res.data : [];
				});
			});
			break;
		case "m_DeleteSimButton":
			if (!m_this.getSelectedData()) return;
			if (this.VerifyDelete == null) {
				this.VerifyDelete = new VerifyPopup("gAll", "verifyPopupDeleteSim", null, null, 320, 100);
				this.VerifyDelete.setText("Are you sure you want to delete the selected simulation?");
				this.VerifyDelete.registerListener(this.uniqueString, this, this.handleVerifyPopupEvents);
			} else {
				this.VerifyDelete.show();
			}
			break;
		case "m_ViewSimButton":
			if (!m_this.getSelectedData()) return;
			this.viewNetwork(m_NetworkView, true);
			break;
		case "m_SaveSimButton":
			var m_this = this;
			var data = this.getSelectedData();
			if (!data) return;
			var uuid = data.id;
			var rev  = data.value.rev;
			var data2 = {}; //create new data object to PUT into the current simulations couchdb document
			data2._id = data.id;
			data2._rev = data.value.rev;
			data2.Date = data.key;
/*?*/		data2.statusOld = data.value.status;
			data2.status = "Saved";
			data2.name = this.textName.getText();
			data2.docFile_INP = this.listNetwork.getInpInfo();
			var a0 = this.listInjNode.getText();
			var a1 = this.listInjType.getValue();
			var a2 = this.textStrength.getFloat();
			var a3 = this.textStart.getSeconds();
			var a4 = this.textEnd.getSeconds();
			data2.input_TSG = [{"a0":a0,"a1":a1,"a2":a2,"a3":a3,"a4":a4}];
			data2.selectedNode = a0;
			data2.m_SimList = true;
			Couch.setDoc(this, uuid, data2, function(data) { ; });
			break;
		default: // program will come here if sid == null
			DataGrid.prototype.onMouseUp.call(this,sid);
	}
}

SimList.prototype.onDelete = function() {
	var uuid = this.getSelectedData().id;
	Couch.deleteDoc(this, uuid, function(e,res) {
		this.updateData();
		this.disableInputs(this.hasSelection());
	});
}

SimList.prototype.handleVerifyPopupEvents = function(e) {
	if (e.event == null) return;
	switch (e.event) {
		case "ok":
			this.onDelete();
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
SimList.prototype.onMouseDown = function(sid) {
	DataGrid.prototype.onMouseDown.call(this,sid);
}
SimList.prototype.onMouseWheel = function(sid) {
	DataGrid.prototype.onMouseWheel.call(this,sid);
}
SimList.prototype.onKeyDown = function(sid) {
	DataGrid.prototype.onKeyDown.call(this,sid);
}
SimList.prototype.onKeyPress = function(sid) {
	DataGrid.prototype.onKeyPress.call(this,sid);
}
SimList.prototype.onKeyUp = function(sid) {
	DataGrid.prototype.onKeyUp.call(this,sid);
}
SimList.prototype.onInput = function(sid) {
	switch (sid) {
		case this.textName.sid:
		case this.textStrength.sid:
		case this.textStart.sid:
		case this.textEnd.sid:
			this.checkModified();
			break;
	}
	DataGrid.prototype.onInput.call(this,sid);
}

SimList.prototype.checkValues = function() {
	var data = this.getSelectedData();
	if (data == null) return false;
	if (this.textName    .checkValue(data.value.name             )) return true;
	if (this.listNetwork .checkValue(data.value.docFile_INP.docId)) return true;
	if (this.listInjNode .checkValue(data.value.input_TSG[0].a0  )) return true;
	if (this.listInjType .checkValue(data.value.input_TSG[0].a1  )) return true;
	if (this.textStrength.checkFloat(data.value.input_TSG[0].a2  )) return true;
	if (this.textStart   .checkTime (data.value.input_TSG[0].a3  )) return true;
	if (this.textEnd     .checkTime (data.value.input_TSG[0].a4  )) return true;	
	return false;
}

SimList.prototype.checkModified = function() {
	var m_this = this;
	var bModified = this.checkValues();
	document.getElementById("m_SaveSimButton").disabled = !bModified;
	if (bModified) {
		var index = this.selectedIndex;
		var uuid = this.data[index].id;
		Couch.GetDoc(uuid, function(data) {
			if (!(data.status == "Modified")) {
				data.statusOld = data.status;
				data.status = "Modified";
				Couch.SetDoc(data._id, data, null);
			}
		});
	} else {
		var index = this.selectedIndex;
		var uuid = this.data[index].id;
		Couch.GetDoc(uuid, function(data) {
			if (data.status == "Modified") {
				Couch.SetValue(data._id, "status", data.statusOld, null);
			}
		});
	}
}

SimList.prototype.disableInputs = function(bDisabled) {
	this.labelName    .disable(bDisabled);
	this.labelNetwork .disable(bDisabled);
	this.labelNode    .disable(bDisabled);
	this.labelType    .disable(bDisabled);
	this.labelStrength.disable(bDisabled);
	this.labelStart   .disable(bDisabled);
	this.labelEnd     .disable(bDisabled);
	this.textName     .disable(bDisabled);
	this.listNetwork  .disable(bDisabled);
	this.listInjNode  .disable(bDisabled);
	this.listInjType  .disable(bDisabled);
	this.textStrength .disable(bDisabled);
	this.textStart    .disable(bDisabled);
	this.textEnd      .disable(bDisabled);
}

SimList.prototype.onChange = function(sid) {
	var m_this = this;
	//
	if (m_TrainingList && m_TrainingList.listScenario) m_TrainingList.listScenario.updateData();
	//	
	var bSelectionChanged = this.checkSelectionChanged();
	if (bSelectionChanged) {
		var index = this.selectedIndexOld;
		if (index > -1 && index < this.data.length) {
			var uuid = this.data[index].id;
			Couch.GetDoc(uuid, function(data) {
				var status = data.status;
				var statusOld = data.statusOld;
				var uuid = data._id;
				if (status == "Modified") {
					var status = (statusOld == "Complete" || statusOld == "Stopped" || statusOld == "New") ? statusOld : "Undone";
					Couch.SetValue(uuid, "status", status);
				}
			});
		}
		var index = this.selectedIndex;
		if (index > -1 && index < this.data.length) {
			var uuid = this.data[index].id;			
			Couch.GetDoc(uuid, function(data) {
				if (data.status == "Undone" || data.status == "Modified") {
					var uuid = data._id;
					Couch.SetValue(uuid, "status", data.statusOld);
				}
			});
		}
	}
	//	
	if (sid != null) {
		switch(sid) {
			case this.listNetwork.sid:
				this.checkModified();
				this.listInjNode.updateData(m_this.listNetwork);
				var duration = this.listNetwork.getSelectedDuration();
				this.textEnd.setPlaceHolderText(duration == null ? "hr:min" : convertSecondsToTime(duration));
				break;
			case this.listInjNode.sid:
				this.checkModified();
				break;
			case this.listInjType.sid:
				this.checkModified();
				break;
			default:
				//should i watch for the text boxes here also?
				//they are pretty much taken care of in "onInput"
				break;
		}
	} else {

		var bNothingSelected = !this.hasSelection();
		var data = this.getSelectedData();
		var uuid = (data) ? data.id : "";
		Couch.GetValue(uuid, "status", function(status) {
			var data = {"value": {"status": status} };
			var bIsNew        = (data) ? (data.value.status == "New"     ) : false;
			var bIsModified   = (data) ? (data.value.status == "Modified") : false;
			var bIsSaved      = (data) ? (data.value.status == "Saved"   ) : false;
			var bIsUndone     = (data) ? (data.value.status == "Undone"  ) : false;
			var bIsStopped    = (data) ? (data.value.status == "Stopped" ) : false;
			var bIsRunning    = (data) ? (data.value.status == "Running" ) : false;
			var bIsError      = (data) ? (data.value.status == "Error"   ) : false;
			var data = m_this.getSelectedData();
			var bIsIncomplete = (data) ? m_this.checkIncomplete(data.value) : true;
			var bIsStoppable  = (data) ? (data.value.pid != null) : false;
			document.getElementById("m_ViewSimButton"  ).disabled = (bNothingSelected ||  bIsSaved   ||  bIsStopped  || bIsModified || bIsRunning || bIsNew || bIsUndone || bIsError);
			document.getElementById("m_RunSimButton"   ).disabled = (bNothingSelected ||  bIsRunning ||  bIsModified || bIsIncomplete);
			document.getElementById("m_StopSimButton"  ).disabled = (bNothingSelected || !bIsRunning || !bIsStoppable);
			document.getElementById("m_DeleteSimButton").disabled = (bNothingSelected ||  bIsRunning );
			document.getElementById("m_SaveSimButton"  ).disabled = (bNothingSelected || !bIsModified);
			m_this.disableInputs(bNothingSelected || bIsRunning);
		});
		//
		var bBlank = !this.hasSelection();
		var uuid = (!bBlank) ? this.getSelectedData().id : "";
		Couch.GetValue(uuid, "status", function(status) {
			var bModified = m_this.checkValues();
			if (!bModified || bSelectionChanged || (bModified && status == "Saved")) {
				m_this.textName    .setValue  (bBlank ? "" : m_this.data[m_this.selectedIndex].value.name               );
				m_this.textStrength.setValue  (bBlank ? "" : m_this.data[m_this.selectedIndex].value.input_TSG[0].a2, "");
				m_this.textStart   .setSeconds(bBlank ? "" : m_this.data[m_this.selectedIndex].value.input_TSG[0].a3, "");
				m_this.textEnd     .setSeconds(bBlank ? "" : m_this.data[m_this.selectedIndex].value.input_TSG[0].a4, "");
				//var bChanged = m_this.listNetwork.selectValue(bBlank ? "" : m_this.data[m_this.selectedIndex].value.docFile_INP.docId);
				//if (bChanged) m_this.listInjNode.updateData(m_this.listNetwork);
				m_this.listNetwork.selectValue(bBlank ? "" : m_this.data[m_this.selectedIndex].value.docFile_INP.docId  );
				m_this.listInjNode.updateData(m_this.listNetwork);
				m_this.listInjType.selectValue(bBlank ? "" : m_this.data[m_this.selectedIndex].value.input_TSG[0].a1    );
				var duration = m_this.listNetwork.getSelectedDuration();
				m_this.textEnd.setPlaceHolderText(duration == null ? "hr:min" : convertSecondsToTime(duration));
			}
		});
		//
		DataGrid.prototype.onChange.call(this,sid);
	}
}
SimList.prototype.onClick = function(sid) {
	DataGrid.prototype.onClick.call(this,sid);
}
SimList.prototype.onDblClick = function(sid) {
	DataGrid.prototype.onDblClick.call(this,sid);
}

SimList.prototype.addListeners = function() {
	this.addListenersForSelection("g"      )
	this.addListenersForSelection("svg"    )
	this.addListenersForSelection("rect"   )//svg
	this.addListenersForSelection("polygon")//svg
	this.addListenersForSelection("path"   )//svg
	this.addListenersForSelection("circle" )//svg
	this.addListenersForSelection("text"   )//svg
	this.addListenersForSelection("input"  )//form
	this.addListenersForSelection("select" )//form
	//this.addListenersForSelection("option" )//form
	this.addListenersForSelection("button" )//form
}

SimList.prototype.addListenersForSelection = function(sSelector) {
	var m_this = this;
	var d3g = d3.select("#" + this.sParentId);
	d3g.selectAll(sSelector)
		.on("mousemove" , function() { m_this.onMouseMove (this.id); })
		.on("mouseover" , function() { m_this.onMouseOver (this.id); })
		.on("mouseout"  , function() { m_this.onMouseOut  (this.id); })
		.on("mouseup"   , function() { m_this.onMouseUp   (this.id); })
		.on("mousedown" , function() { m_this.onMouseDown (this.id); })
		//.on("mousewheel", function() { m_this.onMouseWheel(this.id); })
		.on("keydown"   , function() { m_this.onKeyDown   (this.id); })
		.on("keypress"  , function() { m_this.onKeyPress  (this.id); })
		.on("keyup"     , function() { m_this.onKeyUp     (this.id); })
		.on("input"     , function() { m_this.onInput     (this.id); })
		.on("change"    , function() { m_this.onChange    (this.id); })
		.on("click"     , function() { m_this.onClick     (this.id); })
		.on("dblclick"  , function() { m_this.onDblClick  (this.id); })
		;
}
