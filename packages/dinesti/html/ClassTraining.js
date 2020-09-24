// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function Training(nPanel, dataInFrontOf, parentOnMouseOver) {
	this.uniqueString = "Training";
	this.nPanel = nPanel;
	this.dataInFrontOf = dataInFrontOf;
	this.h = 300;
	d3.select("#g" + nPanel).append("g").attr("id","g" + nPanel + "i").style("visibility","hidden");
	this.TrainingList = new RunList(this.uniqueString, "g" + nPanel + "i", "svg" + nPanel, dataInFrontOf, parentOnMouseOver, 30, 40, 200, this.h);
	this.TrainingList.SaveButton.setPosition({"x":320,"y":270});
	this.TrainingList.RunButton.hide();
	this.TrainingList.ViewButton.hide();
	this.TrainingList.StopButton.hide();
	this.TrainingList.parent = this;
	this.TrainingList.CouchExe = "_training";//Required by RunList()
}

Training.prototype.updateData = function(bFirstTime_or_uuid_or_data) {
	var m_this = this;
	var type = typeof(bFirstTime_or_uuid_or_data);
	var bFirstTime = (type == "boolean") ? bFirstTime_or_uuid_or_data : null;
	var uuid       = (type == "string" ) ? bFirstTime_or_uuid_or_data : null;
	var data       = (type == "object" ) ? bFirstTime_or_uuid_or_data : null;
	if (bFirstTime) {
		this.createInputs();
	}
	//
	this.TrainingList.updateData(bFirstTime, function() {
		m_this.addListeners();
	});
	//
	if (uuid && m_EventView.getUuid() == uuid) {
		if (m_EventView.isVisible())
		m_EventView.updateData(uuid);
	}
	if (data && m_EventView.getUuid() == data._id) {
		if (m_EventView.isVisible())
			m_EventView.updateData(data);
	}
}

Training.prototype.updateGrabGrid = function(data) {
	if (m_EventView == null) return;
	if (m_EventView.isHidden()) return;
	if (!m_EventView.getTraining()) return;
	var uuid = data._id;
	if (uuid == null) return;
	var uuids = m_EventView.getUuids();
	if (uuids.grab != uuid) return;
	m_EventView.updateGrabGrid(data);
}

Training.prototype.updateCanaryGrid = function(data) {
	if (m_EventView == null) return;
	if (m_EventView.isHidden()) return;
	if (!m_EventView.getTraining()) return;
	var uuid = data._id;
	if (uuid == null) return;
	var uuids = m_EventView.getUuids();
	if (uuids.canary != uuid) return;
	m_EventView.updateCanaryGrid(data);
}

// Static
Training.cleanUpDeleted = function(uuid) {
	Events.deleteInversionListByEvent(uuid);
	Events.deleteGrabListByEvent(uuid);
	Events.deleteEventsGrabGridByEvent(uuid);
	Events.deleteEventsCanaryGridByEvent(uuid);
	Events.deleteEventsLogByEvent(uuid);
	Couch.getDoc(this, Couch.training + "?call=delete&uuid=" + uuid, function(data) {
		var data = data;
	});
}

Training.cleanUpSaved = function(uuids) {
	Events.deleteInversionListByEvent(uuids.uuid);
	Events.deleteGrabListByEvent(uuids.uuid);
	Couch.getDoc(this, uuids.grab, function(data) {
		data.grab = [];
		data.index = -1
		Couch.setDoc(this, uuids.grab, data);
	});
	Couch.getDoc(this, uuids.canary, function(data) {
		data.canary = [];
		data.index = -1
		Couch.setDoc(this, uuids.canary, data);
	});
	Couch.getDoc(this, Couch.training + "?call=save&uuid=" + uuids.uuid, function(data) {
		var data = data;
	});
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

Training.prototype.createNewData = function() {
	var data = {};
	data.status = "New";
	data.statusOld = "";
	data.m_TrainingList = true;
	data.name = "T" + ("" + new Date().getTime()).substring(3);
	data.scenario = "",
	data.sensors = "",
	data.Date = new Date();
	// used to figure out how many uuids to create and where to put them.
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
	return data;
}

Training.prototype.createInputs = function() {
	var m_this = this;
	//
	this.labelName         = new Label      ("Name:"    , "svg" + this.nPanel      , "labelTrainingName"       , this.dataInFrontOf, 255,  60,  50, 20);
	this.labelScenario     = new Label      ("Scenario:", "svg" + this.nPanel      , "labelTrainingScenario"   , this.dataInFrontOf, 255,  80,  50, 20);
	this.labelSensors      = new Label      ("Sensors:" , "svg" + this.nPanel      , "labelTrainingSensors"    , this.dataInFrontOf, 255, 100,  50, 20);
	//
	this.textName          = new Textbox    (               "g" + this.nPanel + "i", "textTrainingName"        , this.dataInFrontOf, 330,  60, 100);
	this.listScenario      = new Dropdown   (               "g" + this.nPanel + "i", "listTrainingScenario"    , this.dataInFrontOf, 330,  80, 100+7.5, true, null);
	this.textSensors       = new Textarea   (               "g" + this.nPanel + "i", "textTrainingSensors"     , this.dataInFrontOf, 255, 120, 177, 120);
	//
	this.textSensors.setPlaceHolderText("None");
	this.textSensors.setWrap(false);
	this.textSensors.setResize(false);
	this.textSensors.setScroll(true);
	//
	this.InitializeButton = new Button("Initialize",   "g" + this.nPanel + "i", "buttonInitializeTraining", this.dataInFrontOf,  80, this.h + 60, 100);
	this.ContinueButton   = new Button("Continue"  ,   "g" + this.nPanel + "i", "buttonContinueTraining"  , this.dataInFrontOf,  80, this.h + 80, 100);
	this.RunAllButton     = new Button("Run All"   ,   "g" + this.nPanel + "i", "buttonRunAllTraining"    , this.dataInFrontOf,  80, this.h + 100, 100);
	//
	this.RunAllButton.hide();
	//
	this.d3grouppath = d3.select("#svg" + this.nPanel).append("path").attr("id","pathTrainingInputs").attr("data-InFrontOf",this.dataInFrontOf)
		.attr("d","M245,50 v300 h183 v-300 Z")
		.attr("d","M245,50 v300 h203 v-300 Z")
		.attr("d","M245,50 v210 h203 v-210 Z")
		.attr("stroke","rgba(49,49,49,0.5)")
		.attr("fill","none")
		;
	//
	this.listScenario.setAttributes = function(sel) {
		sel.attr("data-date",    function(d,i) {
			return d.key;            });
		sel.attr("data-id",      function(d,i) {
			return d.id;             });
		sel.attr("data-value",   function(d,i) {
			return d.id;             });
		sel.text(                function(d,i) {
			return d.value.name; });
	}
	this.listScenario.uniqueAttribute = "data-id";
	// we must use m_this since this is a function called by another class
	this.listScenario.updateData = function() {
		Couch.getView(this, Couch.SimListPlus, function(data) {
			var rows = data ? data.rows : [];
			var selectedData = m_this.getSelectedData();
			var uuid = (selectedData) ? selectedData.value.scenario : "";
			m_this.listScenario.updateList(rows);
			m_this.listScenario.selectValue(uuid);
		});
	}
	this.listScenario.updateData();
}

Training.prototype.updateLabels = function() {
	this.labelName.update();
	this.labelScenario .update();
	this.labelSensors.update();
}

//Training.prototype.viewNetwork = function() {}

Training.prototype.checkValues = function() {
	var data = this.getSelectedData();
	if (data == null) return false;
	if (this.textName    .checkValue(data.value.name    )) return true;
	if (this.listScenario.checkValue(data.value.scenario)) return true;
	if (this.textSensors .checkValue(data.value.sensors )) return true;
	return false;
}

Training.prototype.checkModified = function() {
	this.TrainingList.checkModified();
}

Training.prototype.disableInputs = function(bDisabled) {
	this.labelName       .disable(bDisabled);
	this.textName        .disable(bDisabled);
	this.labelScenario   .disable(bDisabled);
	this.listScenario    .disable(bDisabled);
	this.labelSensors    .disable(bDisabled);
	this.textSensors     .disable(bDisabled);
}

Training.prototype.saveData = function(data2) {
	var data = this.getSelectedData();
	if (!data) return;
	data2.name = this.textName.getText();
	var scenario = this.listScenario.getValue();
	data2.scenario = scenario;
	var sensors = this.textSensors.getText();
	data2.sensors = sensors;
	data2.m_TrainingList = true;
	data2.logId = data.value.logId;
	data2.grabId = data.value.grabId;
	data2.canaryId = data.value.canaryId;
	//data2.delay = 1800;
	//data2.sampleCount = 3;
	var uuids = this.getUuids(data);
	Training.cleanUpSaved(uuids);
	Events.log(null, data2.logId, "Saved");
	return data2;
}

Training.prototype.setInputs = function(bNothingSelected) {
	this.textName.setValue(bNothingSelected ? "" : this.TrainingList.data[this.TrainingList.selectedIndex].value.name);
	this.listScenario.updateData();
	this.textSensors.setValue(bNothingSelected ? "" : this.TrainingList.data[this.TrainingList.selectedIndex].value.sensors);
}

Training.prototype.selectPrevious = function() {
	this.TrainingList.selectRowPrevious();
}

Training.prototype.selectNext = function() {
	this.TrainingList.selectRowNext();
}

Training.prototype.getSelectedData = function() {
	return this.TrainingList.getSelectedData();
}

Training.prototype.getUuid = function() {
	var data = this.getSelectedData();
	return (data) ? data.id : null;
}

Training.prototype.getUuids = function(data) {
	if (data == null) data = this.getSelectedData();
	if (data == null) return null;
	var uuids = {};
	uuids.uuid = data.id;
	uuids.grab = data.value.grabId;
	uuids.canary = data.value.canaryId;
	uuids.log = data.value.logId;
	return uuids;
}

Training.prototype.getStatus = function() {
	var data = this.getSelectedData();
	return (data) ? data.value.status : null;
}

Training.prototype.checkIncomplete = function(data) {
	if (data == null) return true;
	if (data.name == null) return true;
	if (data.name.length == 0) return true;
	if (data.scenario == null) return true;
	if (data.scenario.length == 0) return true;
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

//Training.prototype.onMouseMove = function(sid) {}
//Training.prototype.onMouseOver = function(sid) {}
//Training.prototype.onMouseOut = function(sid) {}
//Training.prototype.onMouseUp = function(sid) {}
//Training.prototype.onMouseDown = function(sid) {}
//Training.prototype.onMouseWheel = function(sid) {}
//Training.prototype.onKeyDown = function(sid) {}
//Training.prototype.onKeyPress = function(sid) {}
//Training.prototype.onKeyUp = function(sid) {}
//Training.prototype.onInput = function(sid) {}
//Training.prototype.onChange = function(sid) {}
//Training.prototype.onClick = function(sid) {}
//Training.prototype.onDblClick = function(sid) {}

Training.prototype.onInitialize = function() {
	m_Waiting.show();
	var uuid = this.getUuid();
	if (uuid == null) return;
	var uuids = this.getUuids();
	if (uuids == null || uuids.uuid == null) return;
	var uuid = uuids.uuid;
	ResizeWindow();
	Events.log(null, uuids.log, "Initializing");
	m_EventView.setTraining(true);
	m_EventView.setUuid(uuid);
	m_EventView.setUuids(uuids);
	m_EventView.clearValues();
	m_EventView.InversionButton.disable(true);
	m_EventView.updateGrabGrid(null);
	m_EventView.updateCanaryGrid(null);
	m_EventGrabView.setUuids(uuids);
	Couch.setValue(this, uuid, "status", "Initializing", function(e,res) {
		Couch.getDoc(this, Couch.training + "?call=init&uuid=" + uuid, function(data) {
			var data = data ? data : {};
			m_EventView.show(data.view);
		});
	});
}

Training.prototype.onContinue = function() {
	m_Waiting.show();
	var uuids = this.getUuids();
	if (uuids == null || uuids.uuid == null) return;
	ResizeWindow();
	Events.log(null, uuids.log, "Continue...");
	m_EventView.setTraining(true);
	m_EventView.setUuid(uuids.uuid);
	m_EventView.setUuids(uuids);
	m_EventView.clearValues();
	m_EventView.InversionButton.disable(true);
	m_EventView.updateGrabGrid(null);
	m_EventView.updateCanaryGrid(null);
	m_EventGrabView.setUuids(uuids);
	Couch.getDoc(this, uuids.canary, function(data) {
		m_EventView.updateCanaryGrid(data);
		Couch.getDoc(this, uuids.grab, function(data) {
			m_EventView.updateGrabGrid(data);
			Couch.getDoc(this, uuids.uuid, function(data) {
				m_EventView.updateData(data, true);
				m_EventView.show(data.view);
			});
		});
	});
}

Training.prototype.onRunAll = function() {
	var uuid = this.getUuid();
	if (uuid == null) return;
	var uuids = this.getUuids();
	if (uuids == null || uuids.uuid == null) return;
	var uuid = uuids.uuid;
	ResizeWindow();
	Events.log(null, uuids.log, "Run All");
	m_EventView.setTraining(true);
	m_EventView.setUuid(uuid);
	m_EventView.setUuids(uuids);
	m_EventView.clearValues();
	m_EventView.InversionButton.disable(true);
	m_EventGrabView.setUuids(uuids);
	Couch.setValue(this, uuid, "status", "Initializing", function(e,res) {
		Couch.getDoc(this, Couch.training + "?call=all&uuid=" + uuid, function(data) {
			var data = data ? data : {};
		});
	});
}

Training.prototype.onMouseUp = function(sid) {
	this.TrainingList.onMouseUp(); // this is for the DataGrid scroll bar to know when scrolling is done
	var m_this = this;
	switch (sid) {
		case this.InitializeButton.sid:
			this.onInitialize();
			return;
		case this.ContinueButton.sid:
			this.onContinue();
			return;
		case this.RunAllButton.sid:
			this.onRunAll();
			return;
	}
}

Training.prototype.addListeners = function(bSkip) {
	if (bSkip == null) bSkip = false;
	if (!bSkip) this.TrainingList.addListeners();

// these two calls are only needed if i dont a call to this.onMouseUp in the RunList.
// they override the listeners created in the 'create' function of the RunList
//	this.InitializeButton.addListeners();
//	this.ContinueButton.addListeners();

//	this.buttonUploadData.addListeners();
}

Training.prototype.isVisible = function() {
	return this.TrainingList.isVisible();
}

Training.prototype.onClick = function(sid) {
	var m_this = this;
	switch(sid) {
		default:
			break;
	}
}

Training.prototype.onKeyDown = function() {
	this.TrainingList.onKeyDown("");
}

Training.prototype.onInput = function(sid) {
	switch (sid) {
		case this.textName.sid:
		case this.textSensors.sid:
			this.checkModified();
			break;
	}
}

Training.prototype.onChange = function(sid) {
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
		this.InitializeButton.disable(bNoneSelected || !bSaved || bIncomplete)
		this.ContinueButton  .disable(bNoneSelected || bNotStarted);	
	}	
	//
	switch (sid) {
		case this.listScenario.sid:
			this.checkModified();
			break;
		default:
			break;
	}
}
