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
function RunList(uniqueString, sParentId, svgid, dataInFrontOf, parentOnMouseOver, x, y, w, h) {
	this.x = x;
	this.y = y;
	this.w = w;
	this.h = h;
	this.base = DataGrid;
	this.base(this.x, this.y, this.w, this.h);
	this.uniqueString = uniqueString;
	this.sParentId = sParentId;
	//this.gid = gid;
	this.svgid = svgid;
	this.dataInFrontOf = dataInFrontOf;
	this.parentOnMouseOver = parentOnMouseOver;
	this.gid = "g" + this.uniqueString + "List";
	this.nColumns = 2;
	this.colNames = [];
	this.colNames.push("name");
	this.colNames.push("status");
	this.colTitles = [];
	this.colTitles.push("Name");
	this.colTitles.push("Status");
	this.colWidth = [105];

	this.textFill = [];
	this.textFill.push("rgb(000,000,000)");
	//this.textFill.push("rgb(000,120,000)");
	this.textFill.push(function(d,i) {
		switch(d.value.status) {
			case "New":
				return "rgb(130,130,130)";
			case "Running":
				return "rgb(000,000,130)";
			case "Loading":
				return "rgb(000,000,130)";
			case "Complete":
				return "rgb(000,130,000)";
			case "Modified":
				return "rgb(130,000,000)";
			case "Error":
				return "rgb(130,000,000)";
			case "Saved":
				return "rgb(000,000,000)";
			default:
				return "rgb(000,000,000)";
		}
	});
	//
	this.NewButton    = new Button("New"   , this.sParentId, "buttonNew"    + this.uniqueString, this.dataInFrontOf,  30, this.h + 60, 50);
	this.RunButton    = new Button("Run"   , this.sParentId, "buttonRun"    + this.uniqueString, this.dataInFrontOf,  80, this.h + 60, 50);
	this.StopButton   = new Button("Stop"  , this.sParentId, "buttonStop"   + this.uniqueString, this.dataInFrontOf, 130, this.h + 60, 50);
	this.DeleteButton = new Button("Delete", this.sParentId, "buttonDelete" + this.uniqueString, this.dataInFrontOf, 180, this.h + 60, 50);
	this.SaveButton   = new Button("Save"  , this.sParentId, "buttonSave"   + this.uniqueString, this.dataInFrontOf, 270,         210, 50);
	this.ViewButton   = new Button("View"  , this.sParentId, "buttonView"   + this.uniqueString, this.dataInFrontOf,  80, this.h + 80, 50);
	//
	this.CouchDoc = "m_" + this.uniqueString + "List";
}

RunList.prototype = new DataGrid;

/////////////////////////////////////////////////////////////////////////////
// Required properties and functions in the parent class
//
RunList.prototype.CouchExe = ""; 

RunList.prototype.createNewData = function() {
	if (this.parent.createNewData)
		return this.parent.createNewData();
	else
		alert("RunList.parent.createNewData() has not been implemented!");
	return null;
}

RunList.prototype.setInputs     = function(bNothingSelected) {
	if (this.parent.setInputs)
		this.parent.setInputs(bNothingSelected);
	else
		alert("RunList.parent.setInputs() has not been implemented!");
}
RunList.prototype.saveData      = function(data2) {
	if (this.parent.saveData)
		return this.parent.saveData(data2);
	else
		alert("RunList.parent.saveData(data2) has not been implemented!");
	return null;
}
RunList.prototype.disableInputs = function(bDisabled) {
	if (this.parent.disableInputs)
		this.parent.disableInputs(bDisabled);
	else
		alert("RunList.parent.disableInputs(bDisabled) has not been implemented!");
}
RunList.prototype.checkValues   = function() {
	if (this.parent.checkValues)
		return this.parent.checkValues();
	else
		alert("RunList.parent.checkValues() has not been implemented!");
	return false;
}
RunList.prototype.viewNetwork = function() {
	if (this.parent.viewNetwork)
		return this.parent.viewNetwork();
	else
		alert("RunList.parent.viewNetwork() has not been implemented!");
	return false;
}
RunList.prototype.checkIncomplete = function(data) {
	if (this.parent.checkIncomplete)
		return this.parent.checkIncomplete(data);
	else
		alert("RunList.parent.checkIncomplete() has not been implemented!");
	return false;
}
RunList.prototype.checkDelete = function(data) {
	if (this.parent.checkDelete)
		return this.parent.checkDelete(data);
	else
		alert("RunList.parent.checkDelete() has not been implemented!");
	return false;
}

/////////////////////////////////////////////////////////////////////////////

RunList.prototype.createButtons = function(sText, dLeft, dTop, dWidth) {
	d3.select("#" + this.sParentId).append("g").attr("id","g" + sText + this.uniqueString + "Button")
			.attr("data-InFrontOf",this.dataInFrontOf)
			.style("position","absolute")
			.style("left",convertToPx(dLeft))
			.style("top",convertToPx(dTop))
		.append("button")
			.attr("data-InFrontOf",this.dataInFrontOf)
			.attr("id","m_" + sText + this.uniqueString + "Button")
			.style("width",convertToPx(dWidth))
			.text(sText)
			;
	return document.getElementById("m_" + sText + this.uniqueString + "Button");
}

RunList.prototype.updateData = function(arg1, fCallback) {
	var m_this = this;
	var bFirstTime = null;
	if (typeof arg1 === "boolean") bFirstTime = arg1;
	var fNext = null;
	if (typeof arg1 === "function") fNext = arg1;
	Couch.GetView(this.CouchDoc, function(data) {
		if (data == null)
			m_this.data = [];
		else
			m_this.data = data.rows;
		if (bFirstTime) {
			m_this.disableInputs(true);//Parent
			m_this.createDataGrid();
		} else {
			m_this.updateDataGrid();
		}
		if (fCallback)
			fCallback();
	});
};

RunList.prototype.checkSelectionChanged = function() {
	var oldId = this.selectedDocId;
	var index = this.selectedIndex;
	var newId = "";
	if (index > -1 && index < this.data.length)
		newId = this.data[index].id;
	this.selectionChanged = (oldId != newId);
	this.selectedDocId = newId;
	return this.selectionChanged;
}

RunList.prototype.getStatus = function(index) {
		if (this.data == null)         return "";
		if (index <= -1)               return "";
		if (index >= this.data.length) return "";
		return this.data[index].value.status;
}

RunList.prototype.getSelectionStatus = function() {
	var index = this.selectedIndex;
	return this.getStatus(index);
}

RunList.prototype.getUuid = function(index) {
		if (this.data == null)         return "";
		if (index <= -1)               return "";
		if (index >= this.data.length) return "";
		return this.data[index].id;
}

RunList.prototype.getSelectionUuid = function() {
	var index = this.selectedIndex;
	return this.getUuid(index);
}

RunList.prototype.getPid = function(index) {
		if (this.data == null)         return "";
		if (index <= -1)               return "";
		if (index >= this.data.length) return "";
		var pid = this.data[index].value.pid;
		if (pid == null) return "";
		return pid;
}

RunList.prototype.getSelectionPid = function() {
	var index = this.selectedIndex;
	return this.getPid(index);
}

RunList.prototype.onMouseMove = function(sid) {
	DataGrid.prototype.onMouseMove.call(this,sid);
}

RunList.prototype.onMouseOver = function(sid) {
	DataGrid.prototype.onMouseOver.call(this,sid);
}

RunList.prototype.onMouseOut = function(sid) {
	DataGrid.prototype.onMouseOut.call(this,sid);
}

RunList.prototype.onMouseUp = function(sid) {
	if (sid) this.parent.onMouseUp(sid);
	switch(sid) {
		case this.NewButton.sid:
			if (this.onNew()) return;
			break;
		case this.RunButton.sid:
			if (this.onRun()) return;
			break;
		case this.StopButton.sid:
			if (this.onStop()) return;
			break;
		case this.DeleteButton.sid:
			if (this.onDelete()) return;
			break;
		case this.ViewButton.sid:
			if (this.onView()) return;
			break;
		case this.SaveButton.sid:
			if (this.onSave()) return;
			break;
		case null:
			DataGrid.prototype.onMouseUp.call(this,sid);
			break;
		default:
			DataGrid.prototype.onMouseUp.call(this,sid);
			break;
	}
}

RunList.prototype.onNew = function(m_this, fCallback) {
	if (this.parent.onNew) {
		var bContinue = this.parent.onNew();
		if (!bContinue) {
			if (fCallback) fCallback.call(m_this);
			return;
		}
	}
	var data = this.createNewData();//Parent
	if (data == null) return true;
	var ids = data.create_auxilary_docs;
	var count = ids ? data.create_auxilary_docs.length + 1 : 1;
	Couch.createUniqueIds(this, count, function(uuids) {
		var obj = {};
		obj["uuid"] = uuids[0];
		for (var i in ids) {
			var uuid = uuids[parseInt(i) + 1];
			var key = ids[i].name;
			obj[key] = uuid;
			data[key + "Id"] = uuid;
			ids[i].data.eventId = uuids[0];
			Couch.setDoc(null, uuid, ids[i].data, function(e,res) { ; });
		}
		if (data.create_auxilary_docs) delete data.create_auxilary_docs;
		uuids = obj;
		Couch.setDoc(this, uuids.uuid, data, function(e,res) {
			this.selectLastRow();
			if (fCallback) fCallback.call(m_this, uuids);
		});
	});
}

RunList.prototype.onRun = function() {
	if (this.CouchExe.length == 0) return true;
	var data = this.getSelectedData();
	if (!data) return true;
	var uuid = data.id;
	Couch.setValue(this, uuid, "status", "Running", function(data) {
		Couch.getDoc(this, this.CouchExe + "?call=run&uuid=" + uuid, function(data) {
			var data = (data) ? data : []; // start process server-side
		});
	});
}

RunList.prototype.onStop = function() {
	var m_this = this;
	var data = m_this.getSelectedData();
	if (!data) return true;
	var uuid = data.id;
	var pid = data.value.pid;
	Couch.GetDoc(Couch.kill + "?pid=" + pid, function(data) {
		Couch.SetValue(uuid, "status", "Stopped", function(e,res) {
			var data = (res && res.data) ? res.data : [];
		});
	});
}

RunList.prototype.onDelete = function() {
	if (!this.getSelectedData()) return true;
	if (this.VerifyDelete == null) {
		this.VerifyDelete = new VerifyPopup("gAll", "verifyPopupDeleteSelection" + this.uniqueString, null, null, 300, 100);
		this.VerifyDelete.setText("Are you sure you want to delete the selected item?");
		this.VerifyDelete.registerListener(this.uniqueString, this, this.handleVerifyPopupEvents);
	} else {
		this.VerifyDelete.show();
	}
	return true;
}

RunList.prototype.handleVerifyPopupEvents = function(e) {
	switch (e.event) {
		case "ok":
			this.onDeleteVerified();
			break;
	}
}

RunList.prototype.onDeleteVerified = function() {
	var data = this.getSelectedData();
	if (!data) return true;
	var uuid = data.id;
	var url1 = this.CouchExe + "?call=delete&uuid=" + uuid;
	var url = this.CouchExe ? url1 : uuid;
	Couch.getDoc(this, url, function(data) {
		Couch.deleteDoc(this, uuid, function(e,res) {
			this.updateData();
			this.disableInputs(this.hasSelection());//Parent
		});
	});
}

RunList.prototype.onView = function() {
	var m_this = this;
	var data = m_this.getSelectedData();
	if (!data) return true;
	this.viewNetwork();
}

RunList.prototype.onSave = function() {
	var m_this = this;
	var data = m_this.getSelectedData();
	if (!data) return true;
	var uuid = data.id;
	var rev  = data.value.rev;
	var data2 = {}; //create new data object to PUT into the current simulations couchdb document
	data2._id = data.id;
	data2._rev = data.value.rev;
	data2.Date = data.key;
	data2.status = "Saved";
	///////////////////////////////
	data2 = this.saveData(data2);// Parent
	///////////////////////////////
	Couch.SetDoc(uuid, data2, function(e,res) {
	//		Couch.GetDoc(m_this.CouchExe + "?call=rename&uuid=" + uuid, function(data) {
			m_this.updateData();
	//	});
	});
}

RunList.prototype.onMouseDown = function(sid) {
	DataGrid.prototype.onMouseDown.call(this,sid);
}
RunList.prototype.onMouseWheel = function(sid_or_e) {
	DataGrid.prototype.onMouseWheel.call(this,sid_or_e);
}
RunList.prototype.onKeyDown = function(sid) {
	DataGrid.prototype.onKeyDown.call(this,sid);
}
RunList.prototype.onKeyPress = function(sid) {
	DataGrid.prototype.onKeyPress.call(this,sid);
}
RunList.prototype.onKeyUp = function(sid) {
	DataGrid.prototype.onKeyUp.call(this,sid);
}
RunList.prototype.onInput = function(sid) {
	this.parent.onInput(sid);
	DataGrid.prototype.onInput.call(this,sid);
}

RunList.prototype.checkModified = function() {
	var m_this = this;
	var bModified = this.checkValues();//Parent
	this.SaveButton.disable(!bModified);
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

RunList.prototype.onChange = function(sid) {
	var m_this = this;
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
		this.parent.onChange(sid);
	} else {
		var bNothingSelected = !this.hasSelection();
		var data = this.getSelectedData();
		var uuid = (data) ? data.id : "";
		Couch.getValue(this, uuid, "status", function(status) {
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
			this.ViewButton  .disable(bNothingSelected ||  bIsSaved   ||  bIsStopped  || bIsModified || bIsRunning || bIsNew || bIsUndone || bIsError);
			this.RunButton   .disable(bNothingSelected ||  bIsRunning ||  bIsModified || bIsIncomplete);
			this.StopButton  .disable(bNothingSelected || !bIsRunning || !bIsStoppable);
			this.DeleteButton.disable(bNothingSelected ||  bIsRunning );
			this.SaveButton  .disable(bNothingSelected || !bIsModified);
			/////////////////////////////////////////////////////
			this.disableInputs(bIsRunning || bNothingSelected);// Parent
			/////////////////////////////////////////////////////
		});
		//
		var bBlank = !this.hasSelection();
		var uuid = (!bBlank) ? this.getSelectedData().id : "";
		Couch.getValue(this, uuid, "status", function(status) {
			var bModified = m_this.checkValues();//Parent
			if (!bModified || bSelectionChanged || (bModified && status == "Saved")) {
				///////////////////////////////////
				this.setInputs(bNothingSelected);// Parent
				///////////////////////////////////
			}
		});
		//
		DataGrid.prototype.onChange.call(this,sid);
		this.parent.onChange(sid);
	}
}

RunList.prototype.onClick = function(sid) {
	//this.parent.onClick(sid);
	DataGrid.prototype.onClick.call(this,sid);
}
RunList.prototype.onDblClick = function(sid) {
	DataGrid.prototype.onDblClick.call(this,sid);
}
RunList.prototype.onBlur = function(sid) {
	DataGrid.prototype.onBlur.call(this,sid);
}

RunList.prototype.addListeners = function() {
	this.addListenersForSelection("g"      )
	this.addListenersForSelection("svg"    )
	this.addListenersForSelection("rect"   )//svg
	this.addListenersForSelection("polygon")//svg
	this.addListenersForSelection("path"   )//svg
	this.addListenersForSelection("circle" )//svg
	this.addListenersForSelection("text"   )//svg
	this.addListenersForSelection("input"  )//form
	this.addListenersForSelection("select" )//form
	this.addListenersForSelection("option" )//form
	this.addListenersForSelection("button" )//form
	this.addListenersForSelection("textarea" )//form
	if (this.parent && this.parent.addListeners) this.parent.addListeners(true);
}

RunList.prototype.addListenersForSelection = function(sSelector) {
	//this.addListenersForSelectionForId(sSelector, "#" + this.svgid);
	this.addListenersForSelectionForId(sSelector, "#" + this.sParentId);
}

RunList.prototype.addListenersForSelectionForId = function(sSelector, sid) {
	var m_this = this;
	var d3g = d3.select(sid);
	var sel = d3g.selectAll(sSelector);
	sel
		.on("mousemove" , function() { 
			m_this.onMouseMove (this.id); })
		.on("mouseover" , function() { 
			m_this.onMouseOver (this.id); })
		.on("mouseout"  , function() { 
			m_this.onMouseOut  (this.id); })
		.on("mouseup"   , function() { 
			m_this.onMouseUp   (this.id); })
		.on("mousedown" , function() { 
			m_this.onMouseDown (this.id); })
		//.on("mousewheel", function() { 
		//	m_this.onMouseWheel(this.id); })
		.on("keydown"   , function() { 
			m_this.onKeyDown   (this.id); })
		.on("keypress"  , function() { 
			m_this.onKeyPress  (this.id); })
		.on("keyup"     , function() { 
			m_this.onKeyUp     (this.id); })
		.on("input"     , function() { 
			m_this.onInput     (this.id); })
		.on("change"    , function() { 
			m_this.onChange    (this.id); })
		.on("click"     , function() { 
			m_this.onClick     (this.id); })
		.on("dblclick"  , function() {
			 m_this.onDblClick (this.id); })
		.on("blur"      , function() {
			 m_this.onBlur     (this.id); })
		;
}
