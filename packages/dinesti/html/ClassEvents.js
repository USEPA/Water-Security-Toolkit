// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function Events(nPanel, dataInFrontOf, parentOnMouseOver) {
	var uniqueString = "Events";
	this.nPanel = nPanel;
	this.dataInFrontOf = dataInFrontOf;
	d3.select("#g" + nPanel).append("g").attr("id","g" + nPanel + "i").style("visibility","hidden");
	this.EventsList = new RunList(uniqueString, "g" + nPanel + "i", "svg" + nPanel, dataInFrontOf, parentOnMouseOver, 30, 40, 200, 300);
	this.EventsList.SaveButton.hide();
	this.EventsList.RunButton.hide();
	this.EventsList.ViewButton.hide();
	this.EventsList.StopButton.hide();
	this.EventsList.NewButton.hide();
	//this.EventsList.DeleteButton.move(150,0);
	this.EventsList.DeleteButton.move(30,10);
	this.EventsList.DeleteButton.hide();
	this.EventsList.parent = this;
	this.EventsList.CouchExe = "_events"; // Required by RunList()
	this.EventsList.colNames = ["date", "status"];
	this.EventsList.colTitles = ["Date", "Status"];
	this.EventsList.colWidth = [105];
}

Events.prototype.updateData = function(bFirstTime_or_uuid_or_data) {
	var m_this = this;
	var type = typeof(bFirstTime_or_uuid_or_data);
	var bFirstTime = (type == "boolean") ? bFirstTime_or_uuid_or_data : null;
	var uuid       = (type == "string" ) ? bFirstTime_or_uuid_or_data : null;
	var data       = (type == "object" ) ? bFirstTime_or_uuid_or_data : null;
	if (bFirstTime) {
		this.createInputs();
	}
	//
	this.EventsList.updateData(bFirstTime, function() {
		m_this.addListeners();
	});
	//
	var uuids = m_EventView ? m_EventView.getUuids() : null;
	if (uuids == null) return;
	if (uuid && uuids.uuid == uuid) {
		if (m_EventView.isVisible())
		m_EventView.updateData(uuid);
	}
	if (data && uuids.uuid == data._id) {
		if (m_EventView.isVisible())
			m_EventView.updateData(data);
	}
}

Events.prototype.updateGrabGrid = function(data) {
	if (m_EventView == null) return;
	if (m_EventView.isHidden()) return;
	if (m_EventView.getTraining()) return;
	var uuid = data._id;
	if (uuid == null) return;
	var uuids = m_EventView.getUuids();
	if (uuids.grab != uuid) return;
	m_EventView.updateGrabGrid(data);
}

Events.prototype.updateCanaryGrid = function(data) {
	if (m_EventView == null) return;
	if (m_EventView.isHidden()) return;
	if (m_EventView.getTraining()) return;
	var uuid = data._id;
	if (uuid == null) return;
	var uuids = m_EventView.getUuids();
	if (uuids.canary != uuid) return;
	m_EventView.updateCanaryGrid(data);
}

Events.prototype.updateLog = function(data) {
	var uuids = this.getUuids();
	if (uuids == null) return;
	if (data.eventId == uuids.uuid)
		this.textLog.setArray(data.log);
}

// Static
Events.getInversionListByEvent = function(uuid, fCallback) {
	var query = (uuid == null) ? "" : "?key=\"" + uuid + "\"";
	Couch.getView(this, Couch.InversionListByEvent + query, function(data) {
		fCallback(data ? data.rows : null);
	});
}

// Static
Events.getGrabListByEvent = function(uuid, fCallback) {
	var query = (uuid == null) ? "" : "?key=\"" + uuid + "\"";
	Couch.getView(this, Couch.GrabListByEvent + query, function(data) {
		fCallback(data ? data.rows : null);
	});
}

// Static
Events.getEventsGrabGridByEvent = function(uuid, fCallback) {
	var query = (uuid == null) ? "" : "?key=\"" + uuid + "\"";
	Couch.getView(this, Couch.EventsGrabGrid + query, function(data) {
		fCallback(data ? data.rows : null);
	});
}

// Static
Events.getEventsCanaryGridByEvent = function(uuid, fCallback) {
	var query = (uuid == null) ? "" : "?key=\"" + uuid + "\"";
	Couch.getView(this, Couch.EventsCanaryGrid + query, function(data) {
		fCallback(data ? data.rows : null);
	});
}

// Static
Events.getEventsLogByEvent = function(uuid, fCallback) {
	var query = (uuid == null) ? "" : "?key=\"" + uuid + "\"";
	Couch.getView(this, Couch.EventsLog + query, function(data) {
		fCallback(data ? data.rows : null);
	});
}

// Static
Events.deleteInversionListByEvent = function(uuid) {
	Events.getInversionListByEvent(uuid, function(data) {
		for (var irow = 0; irow < data.length; irow++) {
			var deleteId = data[irow].id;
			Couch.DeleteDoc(deleteId, function(e,res) {
				var data = (res) ? res.data : [];
			});
		}
	});
}

// Static
Events.deleteGrabListByEvent = function(uuid) {
	Events.getGrabListByEvent(uuid, function(data) {
		for (var irow = 0; irow < data.length; irow++) {
			var deleteId = data[irow].id;
			Couch.DeleteDoc(deleteId, function(e,res) {
				var data = (res) ? res.data : [];
			});
		}
	});
}

// Static
Events.deleteEventsGrabGridByEvent = function(uuid) {
	Events.getEventsGrabGridByEvent(uuid, function(data) {
		for (var irow = 0; irow < data.length; irow++) {
			var deleteId = data[irow].id;
			Couch.DeleteDoc(deleteId, function(e,res) {
				var data = (res) ? res.data : [];
			});
		}
	});
}

// Static
Events.deleteEventsCanaryGridByEvent = function(uuid) {
	Events.getEventsCanaryGridByEvent(uuid, function(data) {
		for (var irow = 0; irow < data.length; irow++) {
			var deleteId = data[irow].id;
			Couch.DeleteDoc(deleteId, function(e,res) {
				var data = (res) ? res.data : [];
			});
		}
	});
}

// Static
Events.deleteEventsLogByEvent = function(uuid) {
	Events.getEventsLogByEvent(uuid, function(data) {
		for (var irow = 0; irow < data.length; irow++) {
			var deleteId = data[irow].id;
			Couch.DeleteDoc(deleteId, function(e,res) {
				var data = (res) ? res.data : [];
			});
		}
	});
}

// Static
Events.cleanUpDeleted = function(uuid) {
	Events.deleteInversionListByEvent(uuid);
	Events.deleteGrabListByEvent(uuid);
	Events.deleteEventsGrabGridByEvent(uuid);
	Events.deleteEventsCanaryGridByEvent(uuid);
	Events.deleteEventsLogByEvent(uuid);
	Couch.getDoc(this, Couch.events + "?call=delete&uuid=" + uuid, function(data) {
		var data = data ? data : null;
	});
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

Events.prototype.createNewData = function() {	
	var data = {};
	data.status = "New";
	data.statusOld = "";
	data.m_EventsList = true;
	data.scenario = "";
	data.sensors = "";
	//
	data.create_auxilary_docs = [];
	var data2 = {};
	data2.m_EventsLog = true;
	data2.log = [];
	data.create_auxilary_docs.push({"name": "log", "data": data2});
	var data2 = {};
	data2.m_EventsGrabGrid = true;
	data2.grab = [];
	data.create_auxilary_docs.push({"name": "grab", "data": data2});
	var data2 = {};
	data2.m_EventsCanaryGrid = true;
	data2.canary = [];
	data.create_auxilary_docs.push({"name": "canary", "data": data2});
	//
	var date = new Date();
	data.Date = date;
//	var sDateTime = date.toJSON();
//	var i1    = sDateTime.indexOf("T");
//	var i2    = sDateTime.indexOf(".");
//	var sDate = sDateTime.substring(0, i1);
//	var sTime = sDateTime.substring(i1 + 1, i2);
//	data.dateTime = sDate + " " + sTime;
	//
	data.dateTime  = "";
	data.dateTime += ""  + ("" + (date.getFullYear() + 0)).lpad("0", 2);
	data.dateTime += "-" + ("" + (date.getMonth()    + 1)).lpad("0", 2);
	data.dateTime += "-" + ("" + (date.getDate()     + 0)).lpad("0", 2);
	data.dateTime += " " + ("" + (date.getHours()    + 0)).lpad("0", 2);
	data.dateTime += ":" + ("" + (date.getMinutes()  + 0)).lpad("0", 2);
	data.dateTime += ":" + ("" + (date.getSeconds()  + 0)).lpad("0", 2);
	//
	data.name = "";
	data.name += ("" + (date.getFullYear() + 0)).lpad("0", 2);
	data.name += ("" + (date.getMonth()    + 1)).lpad("0", 2);
	data.name += ("" + (date.getDate()     + 0)).lpad("0", 2);
	data.name += ("" + (date.getHours()    + 0)).lpad("0", 2);
	data.name += ("" + (date.getMinutes()  + 0)).lpad("0", 2);
	data.name += ("" + (date.getSeconds()  + 0)).lpad("0", 2);
	//
	var wqmFile  = GlobalData.config_event_inp_data.wqmFile;
	var docId    = GlobalData.config_event_inp_data._id
	var fileName = GlobalData.config_event_inp_data.fileName;
	var name     = GlobalData.config_event_inp_data.name;
	var docFile_INP = {"wqmFile": wqmFile, "docId": docId, "fileName": fileName, "name": name};
	data.docFile_INP = docFile_INP;
	return data;
}

Events.prototype.createInputs = function() {
	var m_this = this;
	//
	this.ContinueButton = new Button("Continue", "g" + this.nPanel + "i", "buttonContinueEvent", this.dataInFrontOf,  80, 300 + 60, 100);
	this.DownloadButton = new Button("Download", "g" + this.nPanel + "i", "buttonDownloadEvent", this.dataInFrontOf,  80, 300 + 80, 100);
	//this.ContinueButton.move(0,-20);
	//
	this.textLog = new Textarea("g" + this.nPanel + "i", "textareaEventLog", this.dataInFrontOf, 240, 50, 215, 305);
	this.textLog.disable();
	this.textLog.setBgColorDisabled(d3.rgb(233,233,233));
}

Events.prototype.updateLabels = function() {}
Events.prototype.checkValues = function() {}
Events.prototype.checkModified = function() {}
Events.prototype.disableInputs = function(bDisabled) {}
Events.prototype.saveData = function(data2) {}

Events.prototype.setInputs = function(bNothingSelected) {
	if (bNothingSelected) {
		this.textLog.setValue("");
		return;
	}
	var uuid = this.EventsList.data[this.EventsList.selectedIndex].value.logId;
	if (uuid == null)
		uuid = this.EventsList.data[this.EventsList.selectedIndex].id;
	Couch.getDoc(this, uuid, function(data){
		this.textLog.setArray(data.log);
	})
}

Events.prototype.selectPrevious = function() {
	this.EventsList.selectRowPrevious();
}

Events.prototype.selectNext = function() {
	this.EventsList.selectRowNext();
}

Events.prototype.getSelectedData = function() {
	return this.EventsList.getSelectedData();
}

Events.prototype.getUuid = function() {
	var data = this.getSelectedData();
	return (data) ? data.id : null;
}

Events.prototype.getUuids = function() {
	var data = this.getSelectedData();
	if (data == null) return null;
	var uuids = {};
	uuids.uuid = data.id;
	uuids.grab = data.value.grabId;
	uuids.canary = data.value.canaryId;
	uuids.log = data.value.logId;
	return uuids;
}

Events.prototype.getStatus = function() {
	var data = this.getSelectedData();
	return (data) ? data.value.status : null;
}

Events.prototype.checkIncomplete = function(data) {
	if (data == null) return true;
	if (data.name == null) return true;
	if (data.name.length == 0) return true;
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

//Events.prototype.onMouseMove = function(sid) {}
//Events.prototype.onMouseOver = function(sid) {}
//Events.prototype.onMouseOut = function(sid) {}
//Events.prototype.onMouseUp = function(sid) {}
//Events.prototype.onMouseDown = function(sid) {}
//Events.prototype.onMouseWheel = function(sid) {}
//Events.prototype.onKeyDown = function(sid) {}
//Events.prototype.onKeyPress = function(sid) {}
//Events.prototype.onKeyUp = function(sid) {}
//Events.prototype.onInput = function(sid) {}
//Events.prototype.onChange = function(sid) {}
//Events.prototype.onClick = function(sid) {}
//Events.prototype.onDblClick = function(sid) {}

// Static properties
Events.logQueue = {};
Events.logProcId = null;


Events.log = function(m_this, log_uuid, s, fCallback) { // TODO - remove m_this and fCallback
	var t = typeof(s);
	var count = 1;
	if (t == "object") {
		count = s.length;
	}
	var sLog = [];
	for (var i = 0; i < count; i++) {
		if (t == "string") {
			sLog.push(s);
		} else {
			sLog.push(s[i]);
		}
	}
	Couch.getDoc(m_this, Couch.events + "?call=log&uuid=" + log_uuid + "&log=" + JSON.stringify(sLog));
}

// Static method
Events.log_old = function(m_this, log_uuid, s, fCallback) { // TODO - remove m_this and fCallback
	if (log_uuid == null) return;
	//
	if (Events.logProcId) clearTimeout(Events.logProcId)
	//
	var t = typeof(s);
	var count = 1;
	if (t == "object") {
		count = s.length;
	}
	for (var i = 0; i < count; i++) {
		var sDate = GlobalData.formatLogDate(new Date()) + "> ";
		if (t == "string") {
			sDate += s;
		} else {
			sDate += s[i];
		}
		var log = Events.logQueue[log_uuid];
		if (log == null) {
			Events.logQueue[log_uuid] = [];
			var log = Events.logQueue[log_uuid];
		}
		log.push(sDate);
	}
	Events.logProcId = setTimeout(function() {
		Events.logWrite();
	}, 200);
}

// Static method
Events.logWrite = function() {
	for (var log_uuid in Events.logQueue) {
		var queue = clone(Events.logQueue[log_uuid]);
		delete Events.logQueue[log_uuid];
		Couch.getDoc(this, log_uuid, function(data) {
			if (data.log == null) data.log = [];
			for (var i = 0; queue[i] != null; i++) {
				data.log.push(queue[i]);
			}
			Couch.setDoc(this, log_uuid, data, function(e,res) {
				var data = (res) ? res.data : null;
			});
		});
	}
}


// Static method
Events.log_old2 = function(m_this, log_uuid, s, fCallback) {	
	if (log_uuid == null) return;
	//
	var t = typeof(s);
	var count = 1;
	if (t == "object") {
		count = s.length;
	}
	//
	Couch.getDoc(m_this, log_uuid, function(data) {
		if (data.log == null) data.log = [];
		for (var i = 0; i < count; i++) {
			var sDate = GlobalData.formatLogDate(new Date()) + "> ";
			if (t == "string") {
				data.log.push(sDate + s);
			} else {
				data.log.push(sDate + s[i]);
			}
		}
		Couch.setDoc(m_this, log_uuid, data, function(e,res) {
			if (e.target.status == 409) {
				Events.log(m_this, log_uuid, s, fCallback);
				return;
			}
			var data = (res) ? res.data : null;
			if (fCallback) fCallback.call(m_this);
		});
	});
}

Events.prototype.onEvent = function() {
	this.EventsList.onNew(this, function(uuids) {
		Events.log(null, uuids.log, "Start ------------------------------");
		this.onStart(uuids);
	});
}

Events.prototype.onStart = function(uuids) {
	m_Waiting.show();
	if (uuids == null) uuids = this.getUuids();
	var uuid = uuids.uuid;
	if (uuid == null) return;
	ResizeWindow();
	m_EventView.setTraining(false);
	m_EventView.setUuids(uuids);
	m_EventView.clearValues();
	m_EventView.InversionButton.disable(true);	
	m_EventGrabView.setUuids(uuids);
	Couch.setValue(this, uuid, "status", "Initializing", function(e,res) {
		var url = Couch.events + "?call=init&uuid=" + uuid;
		Couch.getDoc(this, url, function(data) {
			m_EventView.show(data.view, data.startTime);
		});
	});
}

Events.prototype.onContinue = function(uuids) {
	m_Waiting.show();
	if (uuids == null) uuids = this.getUuids();
	var uuid = uuids.uuid;
	if (uuid == null) return;
	ResizeWindow();
	Events.log(null, uuids.log, "Continue ---------------------------");
	m_EventView.setTraining(false);
	m_EventView.setUuids(uuids);
	m_EventView.clearValues();
	m_EventView.InversionButton.disable(true);
	m_EventGrabView.setUuids(uuids);
	Couch.getDoc(this, uuids.canary, function(data) {
		m_EventView.updateCanaryGrid(data);
		Couch.getDoc(this, uuids.grab, function(data) {
			m_EventView.updateGrabGrid(data);
			Couch.getDoc(this, uuids.uuid, function(data) {
				m_EventView.updateData(data, true);
				m_EventView.show(data.view, data.startTime);
			});
		});
	});
}

Events.prototype.onDownload = function(uuids) {
	m_Waiting.show();
	if (uuids == null) uuids = this.getUuids();
	var uuid = uuids.uuid;
	if (uuid == null) return;
	var url = Couch.events + "?call=download&uuid=" + uuid;
	Couch.getDoc(this, url, function(data) {
		var url = GlobalData.CouchDb + data.uuid + "/" + data.fileName;
		window.open(url, "_self");
		m_Waiting.hide()
	});
}

Events.prototype.onMouseUp = function(sid) {
	this.EventsList.onMouseUp(); // this is for the DataGrid scroll bar to know when scrolling is done
	var m_this = this;
	switch (sid) {
		case this.ContinueButton.sid:
			this.onContinue();
			return;
		case this.DownloadButton.sid:
			this.onDownload();
			return;
	}
}

Events.prototype.addListeners = function(bSkip) {
	if (bSkip == null) bSkip = false;
	if (!bSkip) this.EventsList.addListeners();
	//
	// these two calls are only needed if i dont make a call to this.onMouseUp in the RunList.
	// they override the listeners created in the 'create' function of the RunList
	//
	this.textLog.addListeners();
}

Events.prototype.isVisible = function() {
	return this.EventsList.isVisible();
}

Events.prototype.onClick = function(sid) {}

Events.prototype.onKeyDown = function(sid) {
	this.EventsList.onKeyDown("");
	//
	switch (sid) {
		case this.textLog.sid:
			break;
	}
}

Events.prototype.onKeyUp = function(sid) {
	switch (sid) {
		case this.textLog.sid:
			break;
	}
}

Events.prototype.onKeyPress = function(sid) {
	switch (sid) {
		case this.textLog.sid:
			break;
	}
}

Events.prototype.onInput = function(sid) {}

Events.prototype.onChange = function(sid) {
	if (sid == null) {
		var status = this.getStatus();
		var bNew          = (status == "New"         );
		var bModified     = (status == "Modified"    );
		var bSaved        = (status == "Saved"       );
		var bInitializing = (status == "Initializing");
		var bInitialized  = (status == "Initialized" );
		var bAlarm        = (status == "Alarm"       );
		var bAlerted      = (status == "Alerted"     );
		var bInversion    = (status == "Inversion"   );
		var bInverted     = (status == "Inverted"    );
		var bLocating     = (status == "Locating"    );
		var bLocated      = (status == "Located"     );
		var bSampling     = (status == "Sampling"    );
		var bSampled      = (status == "Sampled"      );
		var bNotStarted   = bNew || bModified || bSaved;
		var bNoneSelected = (status == null);
		var bSelected     = !bNoneSelected;
		//
		var data = this.getSelectedData();
		var bIncomplete = this.checkIncomplete(data ? data.value : null);
		//
		this.ContinueButton.disable(bNoneSelected || bNotStarted);
		this.DownloadButton.disable(bNoneSelected || bNotStarted);
	}	
	//
	switch (sid) {
		default:
			break;
	}
}
