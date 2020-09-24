// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function Impact(nPanel, dataInFrontOf, parentOnMouseOver) {
	var uniqueString = "Impact";
	this.nPanel = nPanel;
	this.dataInFrontOf = dataInFrontOf;
	//var close = new CloseButton(nPanel, dataInFrontOf);
	d3.select("#g" + nPanel).append("g").attr("id","g" + nPanel + "i").style("visibility","hidden");
	this.ImpactList = new RunList(uniqueString, "g" + nPanel + "i", "svg" + nPanel, dataInFrontOf, parentOnMouseOver, 30, 40, 200, 300);
	this.ImpactList.SaveButton.setPosition({"x":320, "y":390});
	this.ImpactList.parent = this;
	this.ImpactList.CouchExe = "_impact";//Required by RunList()
	this.results = null;
}

Impact.prototype.updateData = function(bFirstTime) {
	var m_this = this;
	if (bFirstTime)
		this.createInputs();
	this.ImpactList.updateData(bFirstTime, function() {
		m_this.addListeners();
	});
}

// Static
Impact.cleanUpDeleted = function(uuid) {
	var m_this = this;
	// TODO - delete temp erd file here?
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

Impact.prototype.createNewData = function() {
	var data = {};
	data.status = "New";
	data.statusOld = "";
	data.m_ImpactList = true;
	data.name = "I" + ("" + new Date().getTime()).substring(3);
	data.docFile_INP = {"docId":"","fileName":""};
	data.input_TSG = [{"a0":"","a1":"","a2":"","a3":"","a4":""}]
	data.Date = new Date();
	data.sensors = null
	data.injections = null
	data.assets = null
	return data;
}

Impact.prototype.createInputs = function() {
	var m_this = this;
	//
	this.labelName       = new Label      ("Name:",                "svg" + this.nPanel      , "labelImpactName"      , this.dataInFrontOf, 255,  60,  50, 20);
	this.textName        = new Textbox    (                          "g" + this.nPanel + "i", "textImpactName"       , this.dataInFrontOf, 330,  60, 100    );
	this.labelNet        = new Label      ("Network:",             "svg" + this.nPanel      , "labelImpactName"      , this.dataInFrontOf, 255,  80,  50, 20);
	this.listNetwork     = new NetworkList(                          "g" + this.nPanel + "i", "listImpactNetwork"    , this.dataInFrontOf, 330,  80, 100+7.5);
	this.labelType       = new Label      ("Event type:",          "svg" + this.nPanel      , "labelImpactType"      , this.dataInFrontOf, 255, 100,  50, 20);
	this.listType        = new Dropdown   (                          "g" + this.nPanel + "i", "listImpactType"       , this.dataInFrontOf, 330, 100, 100+7.5, true);
	this.labelStrength   = new Label      ("Ev strength:",         "svg" + this.nPanel      , "labelImpactStrength"  , this.dataInFrontOf, 255, 120,  50, 20);
	this.textStrength    = new Textbox    (                          "g" + this.nPanel + "i", "textStrength"         , this.dataInFrontOf, 330, 120, 100    );
	this.labelStart      = new Label      ("Ev start (h:m):",      "svg" + this.nPanel      , "labelImpactStart"     , this.dataInFrontOf, 255, 140,  50, 20);
	this.textStart       = new Timebox    (                          "g" + this.nPanel + "i", "textStart"            , this.dataInFrontOf, 330, 140, 100    );
	this.labelEnd        = new Label      ("Ev end (h:m):",        "svg" + this.nPanel      , "labelImpactEnd"       , this.dataInFrontOf, 255, 160,  50, 20);
	this.textEnd         = new Timebox    (                          "g" + this.nPanel + "i", "textEnd"              , this.dataInFrontOf, 330, 160, 100    );
	this.labelStop       = new Label      ("Sim end (h:m):",       "svg" + this.nPanel      , "labelImpactStop"      , this.dataInFrontOf, 255, 180,  50, 20);
	this.textStop        = new Timebox    (                          "g" + this.nPanel + "i", "textImpactStop"       , this.dataInFrontOf, 330, 180, 100    );
	this.labelInjections = new Label      ("Event locations:",     "svg" + this.nPanel      , "labelImpactInjections", this.dataInFrontOf, 255, 206,  50, 20);
	this.textInjections  = new Textarea   (                          "g" + this.nPanel + "i", "textImpactInjections" , this.dataInFrontOf, 255, 226, 177, 30);
	this.labelSensors    = new Label      ("Sensor locations:",    "svg" + this.nPanel      , "labelImpactSensors"   , this.dataInFrontOf, 255, 263,  50, 20);
	this.textSensors     = new Textarea   (                          "g" + this.nPanel + "i", "textImpactSensors"    , this.dataInFrontOf, 255, 283, 177, 30);
	this.labelAssets     = new Label      ("Asset locations:",     "svg" + this.nPanel      , "labelImpactAssets"    , this.dataInFrontOf, 255, 320,  50, 20);
	this.textAssets      = new Textarea   (                          "g" + this.nPanel + "i", "textImpactAssets"     , this.dataInFrontOf, 255, 340, 177, 30);
	//
	this.textStrength  .setPlaceHolderText("100"   );
	this.textStart     .setPlaceHolderText("0:00"  );
	this.textEnd       .setPlaceHolderText("hr:min");
	this.textInjections.setPlaceHolderText("All"   );
	this.textSensors   .setPlaceHolderText("None"  );
	this.textAssets    .setPlaceHolderText("All"   );
	this.textStop      .setPlaceHolderText("hr:min");
	//
	this.textInjections.setWrap(false);
	this.textInjections.setResize(false);
	this.textInjections.setScroll(true);
	//
	this.textSensors.setWrap(false);
	this.textSensors.setResize(false);
	this.textSensors.setScroll(true);
	//
	this.textAssets.setWrap(false);
	this.textAssets.setResize(false);
	this.textAssets.setScroll(true);
	//
	this.d3grouppath = d3.select("#svg" + this.nPanel).append("path").attr("id","pathInvInputs").attr("data-InFrontOf",this.dataInFrontOf).attr("d","M245,50 v335 h203 v-335 Z").attr("stroke","rgba(49,49,49,0.5)").attr("fill","none");
	//
	this.updateNetworkList();
	//
	this.listType.updateList([{"text":"Mass","value":"MASS"},{"text":"Flow paced","value":"FLOWPACED"}]);
	//
}

Impact.prototype.updateNetworkList = function() {
	var selectedData = this.ImpactList.getSelectedData();
	var uuid = (selectedData == null) ? "" : selectedData.value.docFile_INP.docId;
	this.listNetwork.updateData(uuid);
}

Impact.prototype.updateLabels = function() {
	this.labelName      .update();
	this.labelNet       .update();
	this.labelType      .update();
	this.labelStrength  .update();
	this.labelStart     .update();
	this.labelEnd       .update();
	this.labelInjections.update();
	this.labelSensors   .update();
	this.labelAssets    .update();
	this.labelStop      .update();
}

Impact.prototype.viewNetwork = function() {
	var m_this = this;
	m_Waiting.show();
	var ids = "";
	var uuid = m_this.ImpactList.getSelectionUuid();
	Couch.GetDoc(uuid, function(data) {
		var Sensors = data.sensors;
		m_NetworkView.m_ImpactView.update(data);
		var uuid = data.docFile_INP.docId;
		if (uuid.length == 0) {
			var data = m_NetworkView.m_data;
			data.Nodes = [];
			data.Links = [];
			data.SimList = [];
			m_NetworkView.viewNetwork(data);
			return;
		}
		Couch.GetDoc(uuid, function(data) {
			var dotSize = data.dotSize;
			var drawTransform = data.drawTransform;
			var map = data.map;
			//var toggle = data.toggle;
			var json = data.jsonFile;
			Couch.GetFile(uuid, json, function(data, text) {
				if (!data) return;
				data.m_ImpactList = true;
				data.dotSize = dotSize;
				data.drawTransform = drawTransform;
				data.map = map;
				//data.toggle = toggle;
				data.uuid = uuid;
				data.Sensors = Sensors;
				m_NetworkView.viewNetwork(data);
			});
		});
	});
}

Impact.prototype.checkValues = function() {
	var data = this.ImpactList.getSelectedData();
	if (data == null) return false;
	if (this.textName      .checkValue(data.value.name             )) return true;
	if (this.listNetwork   .checkValue(data.value.docFile_INP.docId)) return true;
	if (this.listType      .checkValue(data.value.input_TSG[0].a1  )) return true;
	if (this.textStrength  .checkFloat(data.value.input_TSG[0].a2  )) return true;
	if (this.textStart     .checkTime (data.value.input_TSG[0].a3  )) return true;
	if (this.textEnd       .checkTime (data.value.input_TSG[0].a4  )) return true;
	if (this.textInjections.checkArray(data.value.injections       )) return true;
	if (this.textSensors   .checkArray(data.value.sensors          )) return true;
	if (this.textAssets    .checkArray(data.value.assets           )) return true;
	if (this.textStop      .checkTime (data.value.stopTime         )) return true;
	return false;
}

Impact.prototype.checkModified = function() {
	this.ImpactList.checkModified();
}

Impact.prototype.disableInputs = function(bDisabled) {
	this.labelName       .disable(bDisabled);
	this.textName        .disable(bDisabled);
	this.labelNet        .disable(bDisabled);
	this.listNetwork     .disable(bDisabled);
	this.labelInjections .disable(bDisabled);
	this.labelType       .disable(bDisabled);
	this.listType        .disable(bDisabled);
	this.labelStrength   .disable(bDisabled);
	this.textStrength    .disable(bDisabled);
	this.labelStart      .disable(bDisabled);
	this.textStart       .disable(bDisabled);
	this.labelEnd        .disable(bDisabled);
	this.textEnd         .disable(bDisabled);
	this.textInjections  .disable(bDisabled);
	this.labelSensors    .disable(bDisabled);
	this.textSensors     .disable(bDisabled);
	this.labelAssets     .disable(bDisabled);
	this.textAssets      .disable(bDisabled);
	this.labelStop       .disable(bDisabled);
	this.textStop        .disable(bDisabled);
}

Impact.prototype.saveData = function(data2) {
	var data = this.ImpactList.getSelectedData();
	if (!data) return;
	data2.name = this.textName.getText();
	data2.docFile_INP = this.listNetwork.getInpInfo();
	data2.m_ImpactList = true;
	var a1 = this.listType.getValue();
	var a2 = this.textStrength.getFloat();
	var a3 = this.textStart.getSeconds();
	var a4 = this.textEnd.getSeconds();
	data2.input_TSG = [{"a0":null,"a1":a1,"a2":a2,"a3":a3,"a4":a4}];
	data2.injections = this.textInjections.getArray();
	data2.sensors = this.textSensors.getArray();
	data2.assets = this.textAssets.getArray();
	data2.stopTime = this.textStop.getSeconds();
	return data2;
}

Impact.prototype.setInputs = function(bNothingSelected) {
	this.textName      .setValue   (bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.name                 );
	//is.listNetwork   .updateData (bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.docFile_INP.docId    );
	this.listNetwork   .selectValue(bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.docFile_INP.docId    );
	this.listType      .selectValue(bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.input_TSG[0].a1      );
	this.textStrength  .setValue   (bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.input_TSG[0].a2  , "");
	this.textStart     .setSeconds (bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.input_TSG[0].a3      );
	this.textEnd       .setSeconds (bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.input_TSG[0].a4      );
	this.textInjections.setArray   (bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.injections           );
	this.textSensors   .setArray   (bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.sensors              );
	this.textAssets    .setArray   (bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.assets               );
	this.textStop      .setSeconds (bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.stopTime             );
	var duration = this.listNetwork.getSelectedDuration();
	this.textEnd.setPlaceHolderText(duration == null ? "hr:min" : convertSecondsToTime(duration));
}

Impact.prototype.selectPrevious = function() {
	this.ImpactList.selectRowPrevious();
}

Impact.prototype.selectNext = function() {
	this.ImpactList.selectRowNext();
}

Impact.prototype.checkIncomplete = function(data) {
	// right now this is only the data passed in from the "view" call
	if (data == null) return true;
	if (data.name == null) return true;
	if (data.name.length == 0) return true;
	if (data.docFile_INP == null) return true;
	if (data.docFile_INP.docId == null) return true;
	if (data.docFile_INP.docId.length == 0) return true;
	if (data.docFile_INP.fileName == null) return true;
	if (data.docFile_INP.fileName.length == 0) return true;
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

//Impact.prototype.onMouseMove = function(sid) {}
//Impact.prototype.onMouseOver = function(sid) {}
//Impact.prototype.onMouseOut = function(sid) {}
//Impact.prototype.onMouseUp = function(sid) {}
//Impact.prototype.onMouseDown = function(sid) {}
//Impact.prototype.onMouseWheel = function(sid) {}
//Impact.prototype.onKeyDown = function(sid) {}
//Impact.prototype.onKeyPress = function(sid) {}
//Impact.prototype.onKeyUp = function(sid) {}
//Impact.prototype.onInput = function(sid) {}
//Impact.prototype.onChange = function(sid) {}
//Impact.prototype.onClick = function(sid) {}
//Impact.prototype.onBlur = function(sid) {}

Impact.prototype.onDblClick = function(sid) {
	var sidPrefix = getPrefix2(sid);
	var id        = getSuffix2(sid);
	switch (sidPrefix) {
	}
}

Impact.prototype.onMouseUp = function() {
	this.ImpactList.onMouseUp();
}

Impact.prototype.addListeners = function(bSkip) {
	if (bSkip == null) bSkip = false;
	if (!bSkip) this.ImpactList.addListeners();
}

Impact.prototype.isVisible = function() {
	return this.ImpactList.isVisible();
}

// this must be registered with text file upload controls so we know when the user is done entering data
Impact.prototype.handleInputPopupEvents = function(e) {
	if (e.event == null) return;
	switch (e.event) {
		case "ok":
			break;
	}
}

Impact.prototype.onKeyDown = function() {
	this.ImpactList.onKeyDown("");
}

Impact.prototype.onInput = function(sid) {
	switch (sid) {
		case this.textName.sid:
		case this.textStrength.sid:
		case this.textStart.sid:
		case this.textEnd.sid:
		case this.textInjections.sid:
		case this.textSensors.sid:
		case this.textAssets.sid:
		case this.textStop.sid:
			this.checkModified();
			break;
	}
}

Impact.prototype.onChange = function(sid) {
	switch(sid) {
		case this.listNetwork.sid:
			this.checkModified();
			var duration = this.listNetwork.getSelectedDuration();
			this.textEnd.setPlaceHolderText(duration == null ? "hr:min" : convertSecondsToTime(duration));
			break;
		case this.listType.sid:
			this.checkModified();
			break;
	}
}
