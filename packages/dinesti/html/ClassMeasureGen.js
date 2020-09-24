// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function MeasureGen(nPanel, dataInFrontOf, parentOnMouseOver) {
	this.uniqueString = "Gen";
	this.nPanel = nPanel;
	this.dataInFrontOf = dataInFrontOf;
	d3.select("#g" + nPanel).append("g").attr("id","g" + nPanel + "i").style("visibility","hidden");
	this.GenList = new RunList(this.uniqueString, "g" + nPanel + "i", "svg" + nPanel, dataInFrontOf, parentOnMouseOver, 30, 40, 200, 300);
	this.GenList.SaveButton.setPosition({"x":320, "y":380});
	this.GenList.parent = this;
	this.GenList.CouchExe = "_measure";//Required by RunList()
}

MeasureGen.prototype.updateData = function(bFirstTime) {
	var m_this = this;
	if (bFirstTime)
		this.createInputs();
	this.GenList.updateData(bFirstTime, function() {
		m_this.addListeners();
	});
}

MeasureGen.prototype.createNewData = function() {
	var data = {};
	data.status = "New";
	data.statusOld = "";
	data.m_GenList = true;
	data.name = "M" + ("" + new Date().getTime()).substring(3);
	data.docFile_INP = {"docId":"","fileName":""};
	data.input_TSG = [{"a0":"","a1":"","a2":"","a3":"","a4":""}]
	data.Date = new Date();
	data.sensorList = null
	return data;
}

MeasureGen.prototype.createInputs = function() {
	var m_this = this;
	//
	var uniq = this.uniqueString;
	var difo = this.dataInFrontOf;
	var n = this.nPanel;
	var sid = "svg" + n;
	var x = 255;
	var y =  60;
	var h =  20;
	var i =   0;
	//
	this.labelName       = new Label      ("Name:"            , sid, "labelName"     + uniq, difo, x, y+h*i++,  50, 20);
	this.labelNet        = new Label      ("Network:"         , sid, "labelName"     + uniq, difo, x, y+h*i++,  50, 20);
	this.labelNode       = new Label      ("Node:"            , sid, "labelNode"     + uniq, difo, x, y+h*i++,  50, 20);
	this.labelType       = new Label      ("Type:"            , sid, "labelType"     + uniq, difo, x, y+h*i++,  50, 20);
	this.labelStrength   = new Label      ("Strength:"        , sid, "labelStrength" + uniq, difo, x, y+h*i++,  50, 20);
	this.labelStart      = new Label      ("Start (h:m):"     , sid, "labelStart"    + uniq, difo, x, y+h*i++,  50, 20);
	this.labelEnd        = new Label      ("End (h:m):"       , sid, "labelEnd"      + uniq, difo, x, y+h*i++,  50, 20);
	this.labelSensors    = new Label      ("Sensor locations:", sid, "labelSensors"  + uniq, difo, x, 210,  50, 20);
	this.labelFirst      = new Label      ("First (h:m):"     , sid, "labelFirst"    + uniq, difo, x, 305,  50, 20);
	this.labelLast       = new Label      ("Last (h:m):"      , sid, "labelLast"     + uniq, difo, x, 325,  50, 20);
	this.labelFreq       = new Label      ("Measure/hour:"    , sid, "labelPerHour"  + uniq, difo, x, 345,  50, 20);
	//
	var sid = "g" + n + "i";
	var x = 330;
	var i =   0;
	this.textName        = new Textbox    (                     sid, "textName"      + uniq, difo,   x, y+h*i++, 100);
	this.listNetwork     = new NetworkList(                     sid, "listNetwork"   + uniq, difo,   x, y+h*i++, 100+7.5);
	this.listNode        = new Dropdown   (                     sid, "listNode"      + uniq, difo,   x, y+h*i++, 100+7.5, true);
	this.listType        = new Dropdown   (                     sid, "listType"      + uniq, difo,   x, y+h*i++, 100+7.5, true);
	this.textStrength    = new Textbox    (                     sid, "textStrength"  + uniq, difo,   x, y+h*i++, 100);
	this.textStart       = new Timebox    (                     sid, "textStart"     + uniq, difo,   x, y+h*i++, 100);
	this.textEnd         = new Timebox    (                     sid, "textEnd"       + uniq, difo,   x, y+h*i++, 100);
	this.textSensors     = new Textarea   (                     sid, "textSensors"   + uniq, difo, 255, 230, 177, 55);
	this.textFirst       = new Timebox    (                     sid, "textFirst"     + uniq, difo,   x, 305, 100);
	this.textLast        = new Timebox    (                     sid, "textLast"      + uniq, difo,   x, 325, 100);
	this.textFreq        = new Textbox    (                     sid, "textFreq"      + uniq, difo,   x, 345, 100);
	//
	this.textStrength  .setPlaceHolderText("100"   );
	this.textStart     .setPlaceHolderText("0:00"  );
	this.textEnd       .setPlaceHolderText("hr:min");
	this.textSensors   .setPlaceHolderText("None"  );
	this.textFirst     .setPlaceHolderText("0:00"  );
	this.textLast      .setPlaceHolderText("hr:min");
	this.textFreq      .setPlaceHolderText("1"     );
	//
	this.textSensors.setWrap(false);
	this.textSensors.setResize(false);
	this.textSensors.setScroll(true);
	//
	this.d3grouppath = d3.select("#svg" + n).append("path").attr("id","pathInvInputs").attr("data-InFrontOf", difo).attr("d","M245,50 v325 h203 v-325 Z").attr("stroke","rgba(49,49,49,0.5)").attr("fill","none");
	//
	this.updateNetworkList();
	//
	this.listNode.updateData = function(network) {
		var uuid = network.getSelectedAttr("data-id");
		var fname = network.getSelectedAttr("data-jsonfile");
		if (uuid && fname) {
			Couch.GetFile(uuid, fname, function(data) {
				var list = (data && data.nodeList) ? data.nodeList : "<select></select>";
				list = list.replace(/m_listInjNode/g, m_this.listNode.sid);
				var div = document.createElement("div");
				div.innerHTML = list;
				var newList = div.firstChild;
				m_this.listNode.replaceList(newList);
				d3.select(div).remove();
				//m_this.addListenersForSelection("#" + m_this.listNode.sid);
				GlobalAddListeners(d3.select("#" + m_this.listNode.sid), m_this.GenList);
				var bHasSelection = m_this.GenList.hasSelection();
				var index = m_this.GenList.selectedIndex;
				var sVal = m_this.GenList.data[index].value.input_TSG[0].a0;
				m_this.listNode.selectValue(m_this.GenList && m_this.GenList.data && m_this.GenList.data[index] && bHasSelection ? sVal : "");
			});
		} else {
			m_this.listNode.updateList([]);
		}
	}
	//
	this.listType.updateList([{"text":"Mass","value":"MASS"},{"text":"Flow paced","value":"FLOWPACED"}]);
	//
}

MeasureGen.prototype.updateNetworkList = function() {
	var data = this.GenList.getSelectedData();
	var uuid = (data == null) ? "" : data.value.docFile_INP.docId;
	this.listNetwork.updateData(uuid);
}

MeasureGen.prototype.updateLabels = function() {
	this.labelName      .update();
	this.labelNet       .update();
	this.labelNode      .update();
	this.labelType      .update();
	this.labelStrength  .update();
	this.labelStart     .update();
	this.labelEnd       .update();
	this.labelSensors   .update();
	this.labelFirst     .update();
	this.labelLast      .update();
	this.labelFreq      .update();
}

MeasureGen.prototype.viewNetwork = function() {
	var output = "";
	var data = this.GenList.getSelectedData();
	var uuid = data.id;
	Couch.getDoc(this, uuid, function(data) {
		var results = data ? data.results : null;
		var Concentrations = results ? results.Concentrations : {};
		for (var node in Concentrations) {
			var list = Concentrations[node];
			for (var i = 0; i < list.length; i++) {
				var sTime = "" + (results.TimeFirst + i * results.TimeStep);
				//output += node + "  " + sTime + "  " + list[i] + "\n";
				//output += node.lpad(" ", 15) + " " + sTime.lpad(" ", 8) + " " + list[i] + "\n";
				output += node + " \t\t " + sTime + " \t\t " + list[i] + "\n";
			}
			output += "\n";
		}
		if (this.ViewResultsPopup) {
			this.ViewResultsPopup.setValue(output);
			this.ViewResultsPopup.show();
		} else {
			this.ViewResultsPopup = new InputPopup("gAll", "inputPopupViewMeasureGenResults", null, null, 250, 500);
			this.ViewResultsPopup.setValue(output);
		}
		//this.InputPopup.registerListener(this.uniqueString, this, this.handleInputPopupEvents);
	});
}

MeasureGen.prototype.checkValues = function() {
	var data = this.GenList.getSelectedData();
	if (data == null) return false;
	if (this.textName      .checkValue(data.value.name             )) return true;
	if (this.listNetwork   .checkValue(data.value.docFile_INP.docId)) return true;
	if (this.listNode      .checkValue(data.value.input_TSG[0].a0  )) return true;
	if (this.listType      .checkValue(data.value.input_TSG[0].a1  )) return true;
	if (this.textStrength  .checkFloat(data.value.input_TSG[0].a2  )) return true;
	if (this.textStart     .checkTime (data.value.input_TSG[0].a3  )) return true;
	if (this.textEnd       .checkTime (data.value.input_TSG[0].a4  )) return true;
	if (this.textSensors   .checkArray(data.value.sensorList       )) return true;
	if (this.textFirst     .checkTime (data.value.sensorStart      )) return true;
	if (this.textLast      .checkTime (data.value.sensorStop       )) return true;
	if (this.textFreq      .checkFloat(data.value.sensorPerHour    )) return true;
	return false;
}

MeasureGen.prototype.checkModified = function() {
	this.GenList.checkModified();
}

MeasureGen.prototype.disableInputs = function(bDisabled) {
	this.labelName    .disable(bDisabled);
	this.textName     .disable(bDisabled);
	this.labelNet     .disable(bDisabled);
	this.listNetwork  .disable(bDisabled);
	this.labelNode    .disable(bDisabled);
	this.listNode     .disable(bDisabled);
	this.labelType    .disable(bDisabled);
	this.listType     .disable(bDisabled);
	this.labelStrength.disable(bDisabled);
	this.textStrength .disable(bDisabled);
	this.labelStart   .disable(bDisabled);
	this.textStart    .disable(bDisabled);
	this.labelEnd     .disable(bDisabled);
	this.textEnd      .disable(bDisabled);
	this.labelSensors .disable(bDisabled);
	this.textSensors  .disable(bDisabled);
	this.labelFirst   .disable(bDisabled);
	this.textFirst    .disable(bDisabled);
	this.labelLast    .disable(bDisabled);
	this.textLast     .disable(bDisabled);
	this.labelFreq    .disable(bDisabled);
	this.textFreq     .disable(bDisabled);
}

MeasureGen.prototype.saveData = function(data2) {
	var data = this.GenList.getSelectedData();
	if (!data) return;
	data2.name = this.textName.getText();
	data2.docFile_INP = this.listNetwork.getInpInfo();
	data2.m_GenList = true;
	var a0 = this.listNode.getText();
	var a1 = this.listType.getValue();
	var a2 = this.textStrength.getFloat();
	var a3 = this.textStart.getSeconds();
	var a4 = this.textEnd.getSeconds();
	data2.input_TSG = [{"a0":a0,"a1":a1,"a2":a2,"a3":a3,"a4":a4}];
	data2.sensorList = this.textSensors.getArray();
	data2.sensorStart = this.textFirst .getSeconds();
	data2.sensorStop = this.textLast.getSeconds();
	data2.sensorPerHour = this.textFreq.getInt();
	return data2;
}

MeasureGen.prototype.setInputs = function(bNothingSelected) {
	this.textName      .setValue   (bNothingSelected ? "" : this.GenList.data[this.GenList.selectedIndex].value.name                 );
//	this.listNetwork   .updateData (bNothingSelected ? "" : this.GenList.data[this.GenList.selectedIndex].value.docFile_INP.docId    );
	this.listNetwork   .selectValue(bNothingSelected ? "" : this.GenList.data[this.GenList.selectedIndex].value.docFile_INP.docId    );
	this.listNode      .updateData (this.listNetwork);
	this.listType      .selectValue(bNothingSelected ? "" : this.GenList.data[this.GenList.selectedIndex].value.input_TSG[0].a1      );
	this.textStrength  .setValue   (bNothingSelected ? "" : this.GenList.data[this.GenList.selectedIndex].value.input_TSG[0].a2  , "");
	this.textStart     .setSeconds (bNothingSelected ? "" : this.GenList.data[this.GenList.selectedIndex].value.input_TSG[0].a3      );
	this.textEnd       .setSeconds (bNothingSelected ? "" : this.GenList.data[this.GenList.selectedIndex].value.input_TSG[0].a4      );
	this.textSensors   .setArray   (bNothingSelected ? "" : this.GenList.data[this.GenList.selectedIndex].value.sensorList           );
	this.textFirst     .setSeconds (bNothingSelected ? "" : this.GenList.data[this.GenList.selectedIndex].value.sensorStart          );
	this.textLast      .setSeconds (bNothingSelected ? "" : this.GenList.data[this.GenList.selectedIndex].value.sensorStop           );
	this.textFreq      .setValue   (bNothingSelected ? "" : this.GenList.data[this.GenList.selectedIndex].value.sensorPerHour    , "");
	var duration = this.listNetwork.getSelectedDuration();
	this.textEnd.setPlaceHolderText(duration == null ? "hr:min" : convertSecondsToTime(duration));
	this.textLast.setPlaceHolderText(duration == null ? "hr:min" : convertSecondsToTime(duration));
}

MeasureGen.prototype.selectPrevious = function() {
	this.GenList.selectRowPrevious();
}

MeasureGen.prototype.selectNext = function() {
	this.GenList.selectRowNext();
}

MeasureGen.prototype.checkIncomplete = function(data) {
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
	if (data.sensorList == null) return true;
	if (data.sensorList.length == 0) return true;
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

//MeasureGen.prototype.onMouseMove = function(sid) {}
//MeasureGen.prototype.onMouseOver = function(sid) {}
//MeasureGen.prototype.onMouseOut = function(sid) {}
//MeasureGen.prototype.onMouseUp = function(sid) {}
//MeasureGen.prototype.onMouseDown = function(sid) {}
//MeasureGen.prototype.onMouseWheel = function(sid) {}
//MeasureGen.prototype.onKeyDown = function(sid) {}
//MeasureGen.prototype.onKeyPress = function(sid) {}
//MeasureGen.prototype.onKeyUp = function(sid) {}
//MeasureGen.prototype.onInput = function(sid) {}
//MeasureGen.prototype.onChange = function(sid) {}
//MeasureGen.prototype.onClick = function(sid) {}
//MeasureGen.prototype.onDblClick = function(sid) {}

MeasureGen.prototype.onMouseUp = function() {
	this.GenList.onMouseUp();
}

MeasureGen.prototype.addListeners = function(bSkip) {
	if (bSkip == null) bSkip = false;
	if (!bSkip) this.GenList.addListeners();
}

MeasureGen.prototype.isVisible = function() {
	return this.GenList.isVisible();
}

// this must be registered with text file upload controls so we know when the user is done entering data
MeasureGen.prototype.handleInputPopupEvents = function(e) {
	if (e.event == null) return;
	switch (e.event) {
		case "ok":
			break;
	}
}

MeasureGen.prototype.onKeyDown = function() {
	this.GenList.onKeyDown("");
}

MeasureGen.prototype.onInput = function(sid) {
	switch (sid) {
		case this.textName.sid:
		case this.textStrength.sid:
		case this.textStart.sid:
		case this.textEnd.sid:
		case this.textSensors.sid:
		case this.textFirst .sid:
		case this.textLast.sid:
		case this.textFreq.sid:
			this.checkModified();
			break;
	}
}

MeasureGen.prototype.onChange = function(sid) {
	switch(sid) {
		case this.listNetwork.sid:
			this.checkModified();
			this.listNode.updateData(this.listNetwork);
			var duration = this.listNetwork.getSelectedDuration();
			this.textEnd.setPlaceHolderText(duration == null ? "hr:min" : convertSecondsToTime(duration));
			this.textLast.setPlaceHolderText(duration == null ? "hr:min" : convertSecondsToTime(duration));
			break;
		case this.listNode.sid:
		case this.listType.sid:
			this.checkModified();
			break;
	}
}

