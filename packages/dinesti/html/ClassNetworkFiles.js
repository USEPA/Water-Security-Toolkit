// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function NetworkFiles(nPanel, dataInFrontOf, parentOnMouseOver) {
	var uniqueString = "NetworkFiles";
	this.nPanel = nPanel;
	this.dataInFrontOf = dataInFrontOf;
//	var close = new CloseButton(nPanel, dataInFrontOf);
	d3.select("#g" + nPanel).append("g").attr("id","g" + nPanel + "i").style("visibility","hidden");
	this.NetworkFiles = new RunList(uniqueString, "g" + nPanel + "i", "svg" + nPanel, dataInFrontOf, parentOnMouseOver, 30, 40, 200, 300);
	//this.NetworkFiles.SaveButton.setPosition({"x":305,"y":360});
	this.NetworkFiles.SaveButton.hide();
	this.NetworkFiles.RunButton.hide();
	this.NetworkFiles.StopButton.hide();
//	this.NetworkFiles.ViewButton.setPosition({"x":305,"y":360});
	this.NetworkFiles.ViewButton.move(0,-20);
	this.NetworkFiles.parent = this;
	this.NetworkFiles.CouchExe = null; // Couch.inp; // Required by RunList()
	this.NetworkFiles.CouchDoc = "m_InpList";
	this.NetworkFiles.colNames[0] = "name";
	//
	this.previewTransform = "translate(340,100) scale(1,-1) ";
	this.previewid = "gNetworkFilesPreview";
	this.pathid = "gNetworkFilesPreviewPath"
	this.d3preview = 
	d3.select("#svg" + this.nPanel).append("g").attr("id", this.previewid)
		.attr("transform", this.previewTransform)
		;
	this.d3path = 
	d3.select("#" + this.previewid).append("path").attr("id", this.pathid)
		.attr("d","M0,0")
		.attr("fill","none")
		.attr("stroke","black")
		;
}

NetworkFiles.prototype.updateData = function(bFirstTime) {
	var m_this = this;
	if (bFirstTime)
		this.createInputs();
	this.NetworkFiles.updateData(bFirstTime, function() {
		m_this.addListeners();
		UpdateNetworkLists();
	});
}

// Static
NetworkFiles.cleanUpDeleted = function(uuid) {
	UpdateNetworkLists();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

NetworkFiles.prototype.createNewData = function() {
	this.chooser.open();
	return data;
}

NetworkFiles.prototype.onNew = function() {
	this.chooser.enable();
	this.chooser.open();
	this.chooser.disable();	
	return false;
}

NetworkFiles.prototype.createInputs = function() {
	var m_this = this;
	this.chooser = new FileChooser("g" + this.nPanel + "i", "chooserNetworkFiles", this.dataInFrontOf, 140, 382);
	this.chooser.setWidth(80);
	this.chooser.setOpacity(0.6);
	this.chooser.setOpacity(0.0);
	this.chooser.disable();
	//
	this.labelTextDuration       = "Duration: ";
	this.labelTextStepSize       = "Step size: ";
	this.labelTextJunctionCount  = "Junction count: ";
	this.labelTextReservoirCount = "Reservoir count: ";
	this.labelTextTankCount      = "Tank count: ";
	this.labelTextPumpCount      = "Pump count: ";
	this.labelTextValveCount     = "Valve count: ";
	this.labelTextPipeCount      = "Pipe count: ";
	//
	var x1 = 270;
	var y = 248;
	var w = 100;
	var h = 14;
	this.labelDuration       = new Label(this.labelTextDuration      , "svg" + this.nPanel, "labelFileDuration"      , this.dataInFrontOf, x1, y+h*0, w, h);
	this.labelStepSize       = new Label(this.labelTextStepSize      , "svg" + this.nPanel, "labelFileStepSize"      , this.dataInFrontOf, x1, y+h*1, w, h);
	this.labelJunctionCount  = new Label(this.labelTextJunctionCount , "svg" + this.nPanel, "labelFileJunctionCount" , this.dataInFrontOf, x1, y+h*2, w, h);
	this.labelReservoirCount = new Label(this.labelTextReservoirCount, "svg" + this.nPanel, "labelFileReservoirCount", this.dataInFrontOf, x1, y+h*3, w, h);
	this.labelTankCount      = new Label(this.labelTextTankCount     , "svg" + this.nPanel, "labelFileTankCount"     , this.dataInFrontOf, x1, y+h*4, w, h);
	this.labelPumpCount      = new Label(this.labelTextPumpCount     , "svg" + this.nPanel, "labelFilePumpCount"     , this.dataInFrontOf, x1, y+h*5, w, h);
	this.labelValveCount     = new Label(this.labelTextValveCount    , "svg" + this.nPanel, "labelFileValveCount"    , this.dataInFrontOf, x1, y+h*6, w, h);
	this.labelPipeCount      = new Label(this.labelTextPipeCount     , "svg" + this.nPanel, "labelFilePipeCount"     , this.dataInFrontOf, x1, y+h*7, w, h);
	//
	var x2 = x1 + 100;
	var w = 90;
	this.textDuration       = new Label(this.labelTextDuration      , "svg" + this.nPanel, "labelFileDuration2"      , this.dataInFrontOf, x2, y+h*0, w, h);
	this.textStepSize       = new Label(this.labelTextStepSize      , "svg" + this.nPanel, "labelFileStepSize2"      , this.dataInFrontOf, x2, y+h*1, w, h);
	this.textJunctionCount  = new Label(this.labelTextJunctionCount , "svg" + this.nPanel, "labelFileJunctionCount2" , this.dataInFrontOf, x2, y+h*2, w, h);
	this.textReservoirCount = new Label(this.labelTextReservoirCount, "svg" + this.nPanel, "labelFileReservoirCount2", this.dataInFrontOf, x2, y+h*3, w, h);
	this.textTankCount      = new Label(this.labelTextTankCount     , "svg" + this.nPanel, "labelFileTankCount2"     , this.dataInFrontOf, x2, y+h*4, w, h);
	this.textPumpCount      = new Label(this.labelTextPumpCount     , "svg" + this.nPanel, "labelFilePumpCount2"     , this.dataInFrontOf, x2, y+h*5, w, h);
	this.textValveCount     = new Label(this.labelTextValveCount    , "svg" + this.nPanel, "labelFileValveCount2"    , this.dataInFrontOf, x2, y+h*6, w, h);
	this.textPipeCount      = new Label(this.labelTextPipeCount     , "svg" + this.nPanel, "labelFilePipeCount2"     , this.dataInFrontOf, x2, y+h*7, w, h);
	//
	//this.textName = new Textbox("g" + this.nPanel + "i", "textImpactName", this.dataInFrontOf, 330,  60, 100);
	//this.textSensors = new Textarea("g" + this.nPanel + "i", "textNetworkFilesSensors", this.dataInFrontOf, 255, 255, 177, 30);
	//this.d3grouppath = d3.select("#svg" + this.nPanel).append("path").attr("id","pathNetworkFilesInputs").attr("data-InFrontOf",this.dataInFrontOf).attr("d","M245,50 v305 h203 v-305 Z").attr("stroke","rgba(49,49,49,0.5)").attr("fill","none");
}

NetworkFiles.prototype.updateLabels = function() {
	this.labelDuration      .update2();
	this.labelStepSize      .update2();
	this.labelJunctionCount .update2();
	this.labelReservoirCount.update2();
	this.labelTankCount     .update2();
	this.labelPumpCount     .update2();
	this.labelValveCount    .update2();
	this.labelPipeCount     .update2();
	//
	this.textDuration      .update2();
	this.textStepSize      .update2();
	this.textJunctionCount .update2();
	this.textReservoirCount.update2();
	this.textTankCount     .update2();
	this.textPumpCount     .update2();
	this.textValveCount    .update2();
	this.textPipeCount     .update2();
}

NetworkFiles.prototype.viewNetwork = function() {
	m_Waiting.show();
	var uuid = this.NetworkFiles.getSelectionUuid();
	Couch.getDoc(this, uuid, function(data) {
		var dotSize = data.dotSize;
		var drawTransform = data.drawTransform;
		var map = data.map;
		//var toggle = data.toggle;
		var json = data.jsonFile;
		Couch.getFile(this, uuid, json, function(data, text) {
			if (!data) return;
			data.m_InpList = true;
			data.dotSize = dotSize;
			data.drawTransform = drawTransform;
			data.map = map;
			//data.toggle = toggle;
			data.uuid = uuid;
			//data.Sensors = Sensors;
			m_NetworkView.viewNetwork(data);
		});
	});
}

NetworkFiles.prototype.checkValues = function() {
	var data = this.NetworkFiles.getSelectedData();
	if (data == null) return false;
	//if (this.textName.checkValue(data.value.name)) return true;
	//if (this.textSensors.checkValue(data.value.sensors)) return true;
	return false;
}

NetworkFiles.prototype.checkModified = function() {
	this.NetworkFiles.checkModified();
}

NetworkFiles.prototype.disableInputs = function(bDisabled) {
	//this.textName.disable(bDisabled);
	//this.textSensors.disable(bDisabled);
}

NetworkFiles.prototype.saveData = function(data2) {
	var data = this.NetworkFiles.getSelectedData();
	if (!data) return;
	//data2.name = this.textName.getText();
	//data2.sensors = this.textSensors.getText();
	return data2;
}

NetworkFiles.prototype.setInputs = function(bNothingSelected) {
	var data = bNothingSelected ? null : this.NetworkFiles.data[this.NetworkFiles.selectedIndex].value;
	if (data && data.ObjectCount == null) data.ObjectCount = {};
	//this.textName.setValue(bNothingSelected ? "" : this.NetworkFiles.data[this.NetworkFiles.selectedIndex].value.name);
	//this.textSensors.setArray(bNothingSelected ? "" : this.ImpactList.data[this.ImpactList.selectedIndex].value.sensors);
	var duration = (data && data.TimeData) ? convertSecondsToTime(data.TimeData.Duration) : "";
	var step     = (data && data.TimeData) ? convertSecondsToTime(data.TimeData.WaterQualityStep) : "";
	this.textDuration      .setText(bNothingSelected ? "" : duration, "");
	this.textStepSize      .setText(bNothingSelected ? "" : step, "");
	this.textJunctionCount .setText(bNothingSelected ? "" : data.ObjectCount.Junctions, "");
	this.textReservoirCount.setText(bNothingSelected ? "" : data.ObjectCount.Reservoirs, "");
	this.textTankCount     .setText(bNothingSelected ? "" : data.ObjectCount.Tanks, "");
	this.textPumpCount     .setText(bNothingSelected ? "" : data.ObjectCount.Pumps, "");
	this.textValveCount    .setText(bNothingSelected ? "" : data.ObjectCount.Valves, "");
	this.textPipeCount     .setText(bNothingSelected ? "" : data.ObjectCount.Pipes, "");
}

NetworkFiles.prototype.selectPrevious = function() {
	this.NetworkFiles.selectRowPrevious();
}

NetworkFiles.prototype.selectNext = function() {
	this.NetworkFiles.selectRowNext();
}

NetworkFiles.prototype.checkIncomplete = function(data) {
	// right now this is only the data passed in from the "view" call
	if (data == null) return true;
	//if (data.name == null) return true;
	//if (data.name.length == 0) return true;
	//if (data.docFile_INP == null) return true;
	//if (data.docFile_INP.docId == null) return true;
	//if (data.docFile_INP.docId.length == 0) return true;
	//if (data.docFile_INP.fileName == null) return true;
	//if (data.docFile_INP.fileName.length == 0) return true;
	return false;
}

NetworkFiles.prototype.getSelectedDataFromDb = function(callback) {
	this.NetworkFiles.disable();
	var uuid = this.NetworkFiles.getSelectionUuid();
	if (uuid.length == 0) {
		callback.call(this, {"blank":true});
		return;
	}
	Couch.getDoc(this, uuid, function(data) {
		if (!data) {
			callback.call(this, {"error":true});
			return;
		}
		var dotSize = data.dotSize;
		var drawTransform = data.drawTransform;
		var map = data.map;
		var toggle = data.toggle;
		var fname = data.jsonFile;
		if (fname == null) {
			callback.call(this, {"error":true});
			return;
		}
		Couch.getFile(this, uuid, fname, function(data) {
			if (!data) {
				callback.call(this, {"error":true});
				return;
			}
			data.m_InpList = true;
			data.dotSize = dotSize;
			data.drawTransform = drawTransform;
			data.map = map;
			data.toggle = toggle;
			data.uuid = uuid;
			if (callback) callback.call(this, data);
		});
	});
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

//NetworkFiles.prototype.onMouseMove = function(sid) {}
//NetworkFiles.prototype.onMouseOver = function(sid) {}
//NetworkFiles.prototype.onMouseOut = function(sid) {}
NetworkFiles.prototype.onMouseUp = function() {
	this.NetworkFiles.onMouseUp();
}
//NetworkFiles.prototype.onMouseDown = function(sid) {}
//NetworkFiles.prototype.onMouseWheel = function(sid) {}
NetworkFiles.prototype.onKeyDown = function() {
	this.NetworkFiles.onKeyDown("");
}
//NetworkFiles.prototype.onKeyPress = function(sid) {}
//NetworkFiles.prototype.onKeyUp = function(sid) {}
NetworkFiles.prototype.onInput = function(sid) {
	switch (sid) {
		case "":
		//case this.textName.sid:
		//case this.textSensors.sid:
			this.checkModified();
			break;
	}
}
NetworkFiles.prototype.onChange = function(sid) {
	var m_this = this;
	var pathError = "M50,45 L48,45 L45,65 L55,65 L52,45 L50,45 M47,40 L53,40 L53,34 L47,34 L47,40";
	var hourglass = "M50,50 L45,45 L45,40 L55,40 L55,45 L50,50 L45,55 L45,60 L55,60 L55,55 L50,50 M45,42 h10 M45,58 h10";
	if (sid == null) {
		var uuid = this.NetworkFiles.getSelectionUuid();
		if (this.uuid == uuid) return;
		this.uuid = uuid;
		this.d3path.attr("d","M0,0");
		this.d3path.attr("d", hourglass);
		var cx = 50;
		var cy = 50;
		var range = 100;
		var w = Math.max(0.5, range / 100);
		this.d3path.attr("stroke-width", w);
		var scale = 100 / range;
		this.d3preview
			.attr("transform", this.previewTransform
				+ " scale(" + scale + ") "
				+ " translate(" + -cx + "," + -cy + ") "
			)
			;
		this.getSelectedDataFromDb(function(data) {
			if (data.blank) {
				this.d3path.attr("d","M0,0");
			} else if (data.error) {
				this.d3path.attr("d", pathError);
			} else if (data) {
				var path = m_NetworkView.getPipesPath(data);
				this.d3path.attr("d", path);
				if (data.Bounds && data.Center) {
					var range = data.Bounds.range.max;
					var w = Math.max(0.5, range / 100);
					this.d3path.attr("stroke-width", w);
					var scale = 100 / range;
					this.d3preview
						.attr("transform", 
							this.previewTransform 
							+ " scale(" + scale + ") "
							+ " translate(" + -data.Center.x + "," + -data.Center.y + ") "
						)
						;
				}
			}
			this.NetworkFiles.enable();
		});
		return;
	}
	switch (sid) {
		case this.chooser.sid:
			var obj = this.chooser.getObject();
			Couch.createUniqueId(this, function(uuid) {
				var uploadFile = m_this.chooser.getFile();
				if (uploadFile == null) return;
				var data = {
					"encryption"         : 0,
					"name"               : m_this.chooser.getFileName(),
					"fileName"           : m_this.chooser.getFileName(),
					"m_InpList"          : true,
					"m_NetworkFilesList" : true,
					"sensors"            : null,
					"status"             : "New",
					"statusOld"          : "",
					"Date"               : new Date()
					};
				Couch.setDoc(this, uuid, data, function(e,res) {
					var rev = res.data.rev;
					var formData;
					try {
						formData = new FormData();
					} catch (e) {
						m_Waiting.hide();
						alert("FormData object not supported!");
						return;
					}
					this.chooser.uploadTo(this, uuid, rev, null, function(e,res) {
//						Couch.get   (this, GlobalData.CouchInpExe + "?uuid=" + uuid + "&fileName=" + uploadFile.name, function(data) {
						Couch.getDoc(this, Couch.inp + "?uuid=" + uuid + "&fileName=" + uploadFile.name, function(data) {
							this.chooser.recreate();
							//this.chooser.setOpacity(0.0);
							//this.chooser.disable();
						});
					});
				});
			});
			break;
	}
}
//NetworkFiles.prototype.onClick = function(sid) {}
//NetworkFiles.prototype.onDblClick = function(sid) {}

NetworkFiles.prototype.addListeners = function(bSkip) {
	if (bSkip == null) bSkip = false;
	if (!bSkip) this.NetworkFiles.addListeners();
}

NetworkFiles.prototype.isVisible = function() {
	return this.NetworkFiles.isVisible();
}
