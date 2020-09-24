// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function Inversion(nPanel, dataInFrontOf, parentOnMouseOver) {
	var uniqueString = "Inversion";
	this.nPanel = nPanel;
	this.dataInFrontOf = dataInFrontOf;
//	var close = new CloseButton(nPanel, dataInFrontOf);
	d3.select("#g" + nPanel).append("g").attr("id","g" + nPanel + "i").style("visibility","hidden");
	this.InversionList = new RunList(uniqueString, "g" + nPanel + "i", "svg" + nPanel, dataInFrontOf, parentOnMouseOver, 30, 40, 200, 300);
	this.InversionList.SaveButton.setPosition({"x":320,"y":300});
	this.InversionList.parent = this;
	this.InversionList.CouchExe = "_inversion";//Required by RunList()
	this.results = null;
}

Inversion.prototype.updateData = function(bFirstTime) {
	var m_this = this;
	if (bFirstTime)
		this.createInputs();
	this.InversionList.updateData(bFirstTime, function() {
		m_this.addListeners();
	});
	this.listInvData.updateData();
}

// Static
Inversion.cleanUpDeleted = function(uuid) {
	Inversion.getMeasurementList(uuid, function(data) {
		for (var i = 0; i < data.length; i++) {
			Couch.DeleteDoc(data[i].id, function(e,res) {
				var data = (res) ? res.data : [];
			});
		}
	});
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

Inversion.prototype.createNewData = function() {
	var data = {};
	data.status = "New";
	data.statusOld = "";
	data.m_InversionList = true;
	data.name = "I" + ("" + new Date().getTime()).substring(3);
	data.docFile_INP = {"docId":"","fileName":""};
	data.input_GRAB = [{"a0":"","a1":null,"a2":null}];
	data.Date = new Date();
	return data;
}

Inversion.prototype.createInputs = function() {
	var m_this = this;
	//
	this.labelInvName     = new Label      ("Name:",             "svg" + this.nPanel      , "labelInvName"    , this.dataInFrontOf, 255,  60,  50, 20);
	this.textName         = new Textbox    (                       "g" + this.nPanel + "i", "textInvName"     , this.dataInFrontOf, 330,  60, 100);
	this.labelInvNet      = new Label      ("Network:",          "svg" + this.nPanel      , "labelInvName"    , this.dataInFrontOf, 255,  80,  50, 20);
	this.listNetwork      = new NetworkList(                       "g" + this.nPanel + "i", "listInvNetwork"  , this.dataInFrontOf, 330,  80, 100+7.5, true);
	this.labelInvData     = new Label      ("Measurement data:", "svg" + this.nPanel      , "labelInvData"    , this.dataInFrontOf, 255, 100,  50, 20);
	this.chooserInvData   = new FileChooser(                       "g" + this.nPanel + "i", "chooserInvData"  , this.dataInFrontOf, 253, 120);
	this.buttonInvUpload  = new Button     ("Upload",              "g" + this.nPanel + "i", "buttonInvUpload" , this.dataInFrontOf, 255, 140,  78, 20); 
	this.buttonInvCreate  = new Button     ("Create new",          "g" + this.nPanel + "i", "buttonInvCreate" , this.dataInFrontOf, 355, 140,  78, 20); 
	this.listInvData      = new Dropdown   (                       "g" + this.nPanel + "i", "listInvData"     , this.dataInFrontOf, 255, 160, 177, null, 7);
	this.buttonDelInvData = new Button     ("Delete",              "g" + this.nPanel + "i", "buttonDelInvData", this.dataInFrontOf, 255, 265,  50, 20);
	//
	this.buttonInvUpload.parent = this;
	this.buttonInvCreate.parent = this;
	this.buttonDelInvData.parent = this;
	this.listInvData.parent = this;
	//
	this.d3grouppath = d3.select("#svg" + this.nPanel).append("path").attr("id","pathInvInputs").attr("data-InFrontOf",this.dataInFrontOf)
		.attr("d","M245,50 v240 h203 v-240 Z")
		.attr("stroke","rgba(49,49,49,0.5)")
		.attr("fill","none")
		;
	//
	this.updateNetworkList();
	//
	this.chooserInvData.onChange = function() {
		m_this.onChange(m_this.chooserInvData.sid);
	}
	this.chooserInvData.createNewData = function() {
		var data = {
			"Date"          : new Date(),
			"m_MeasureList" : true,
			"fileName"      : this.getFileName(),
			"uuidInversion" : m_this.InversionList.getSelectionUuid()
		};
		return data;
	}
	//
	this.listInvData.updateData = function() {
		var uuid = m_this.InversionList.getSelectionUuid();
		Inversion.getMeasurementList(uuid, function(data) {
			m_this.listInvData.updateList(data);
			m_this.onChange();
		});
	}
	this.listInvData.setAttributes = function(sel) {
		sel.attr("data-rev",      function(d,i) { 
			return d.value.rev;      });
		sel.attr("data-date",     function(d,i) { 
			return d.value.date;     });
		sel.attr("data-invId",    function(d,i) {
			return d.value.uuid;     });
		sel.attr("data-value",    function(d,i) {
			return d.id;             });
		sel.text(                 function(d,i) { 
			return d.value.fileName;     });
	}
	this.listInvData.onClick = function() {
		m_this.onClick(m_this.listInvData.sid);
	}
	this.listInvData.onChange = function() {
		m_this.onChange(m_this.listInvData.sid);
	}
	this.listInvData.updateData();
	//
	this.buttonDelInvData.onClick = function() {
		m_this.onClick(m_this.buttonDelInvData.sid);
	}
}

// Static
Inversion.getMeasurementList = function(uuid, fCallback) {
	// uuid = inversion selected.
	var query = (uuid == null) ? "" : "?key=\"" + uuid + "\"";
	Couch.GetView(Couch.MeasureList + query, function(data) {
		fCallback(data.rows);
	});
}

Inversion.prototype.updateNetworkList = function() {
	var selectedData = this.InversionList.getSelectedData();
	var uuid = (selectedData == null) ? "" : selectedData.value.docFile_INP.docId;
	this.listNetwork.updateData(uuid);
}

Inversion.prototype.updateLabels = function() {
	this.labelInvName.update();
	this.labelInvNet .update();
	this.labelInvData.update();
}

Inversion.prototype.viewNetwork = function() {
	var m_this = this;
	var ids = [];
	var uuid = m_this.InversionList.getSelectionUuid();
	Inversion.staticViewNetwork(uuid, false);
}

// Static
Inversion.staticViewNetwork = function(uuid, bEvent) {
	m_Waiting.show();
	// use the Measure List file to set sensors
	var sensors = [];
	Inversion.getMeasurementList(uuid, function(list) {
		var data = [];
		var ifile = 0;
		var nfiles = list.length;
		var fLoop = function() {
			var uuid = list[ifile].id;
			var fileName = list[ifile].value.fileName; 
			Couch.GetFile(uuid, fileName, function(ignore_json, text) {
				var lines = text.split("\n");
				for (var i = 0; i < lines.length; i++) {
					if (lines[i].substring(0,1) == "#") continue;
					var line = lines[i].split(" ");
					var id   = line[0];
					var time = line[1];
					var c    = line[2];
					if (id.length == 0) continue;
					data.push({"id":id,"time":time,"c":c});
					var index = sensors.indexOf(id);
					if (index > -1) continue;
					sensors.push(id);
				}
				if (++ifile < nfiles) fLoop();
				else fContinue();
			});
		};
		fLoop();
	});

	// after the sensors have all been retrieved prepare for the network view
	var fContinue = function() {
		m_NetworkView.m_InversionNavigator.setId(uuid); // TODO - this isn't multi-network-view safe
		m_NetworkView.m_InversionGrid.setSensors(sensors); // TODO - this isn't multi-network-view safe
		m_NetworkView.m_InversionGrid.setId(uuid); // TODO - this isn't multi-network-view safe
		Couch.GetDoc(uuid, function(data) {
			if (!data) return;
			var inversionResults = data.results;
			var uuidInp = data.docFile_INP.docId;
			if (uuidInp.length == 0) {
				var data = m_NetworkView.m_data;
				data.Nodes = [];
				data.Links = [];
				data.SimList = [];
				m_NetworkView.viewNetwork(data);
				return;
			}
			Couch.GetDoc(uuidInp, function(data) {
				var dotSize = data.dotSize;
				var drawTransform = data.drawTransform;
				var map = data.map;
				//var toggle = data.toggle;
				var json = data.jsonFile;
				Couch.GetFile(uuidInp, json, function(data, text) {
					if (!data) return;
					data.m_InvList = true;
					data.dotSize = dotSize;
					data.drawTransform = drawTransform;
					data.map = map;
					//data.toggle = toggle;
					data.uuid = uuidInp;
					data.Sensors = sensors;
					data.inversionResults = inversionResults;
					data.selectionUuid = uuid;
					if (bEvent) data.m_Event = true;
					m_NetworkView.viewNetwork(data);
					//
					var sel = m_NetworkView.getNodes();
					Inversion.modifyNetwork(sel, inversionResults); // ???
				});
			});
		});
	};
}

// Static
Inversion.modifyNetwork = function(sel, results) {
	// sel = d3 selection of all the Junctions, Tanks, and Reservoirs
	sel.attr("fill", "red");
	sel.attr("fill-opacity", function(d,i) {
		if (results == null || results.ids[d.id] == null) return 0.0;
		var dVal = results.list[results.ids[d.id]].Objective;
		return dVal / results.max;
	});
	sel.attr("r", function(d,i) {
		if (d.Type == "Tank") return null;
		if (d.Type == "Reservoir") return null;
		var base = 5 * m_NetworkView.nStrokeWidth;
		if (results == null || results.ids[d.id] == null) return base * 0.0;
		var dVal = results.list[results.ids[d.id]].Objective;
		var dR = base * dVal / results.max;
		return dR;
	});
	sel.attr("d", function(d,i) {
		if (d.Type == "Junction") return null;
		if (results == null) return null;
		var index = results.ids[d.id];
		if (index == null) return "M 0 0";
		var base = m_NetworkView.nStrokeWidth;
		var dVal = results.list[index].Objective;
		var w = base * dVal / results.max;
		if (d.Type == "Tank"     ) return m_NetworkView.getTankPath     (d.x, d.y, w);
		if (d.Type == "Reservoir") return m_NetworkView.getReservoirPath(d.x, d.y, w);
		return null;
	});
}

Inversion.prototype.checkValues = function() {
	var data = this.InversionList.getSelectedData();
	if (data == null) return false;
	if (this.textName   .checkValue(data.value.name             )) return true;
	if (this.listNetwork.checkValue(data.value.docFile_INP.docId)) return true;
	return false;
}

Inversion.prototype.checkModified = function() {
	this.InversionList.checkModified();
}

Inversion.prototype.disableInputs = function(bDisabled) {
	this.labelInvName    .disable(bDisabled);
	this.textName        .disable(bDisabled);
	this.labelInvNet     .disable(bDisabled);
	this.listNetwork     .disable(bDisabled);
	this.labelInvData    .disable(bDisabled);
	this.chooserInvData  .disable(bDisabled);
	var bNoFile = !this.chooserInvData.hasFile();
	var bNoSelection = !this.InversionList.hasSelection();
	var bDisableUpload = bDisabled || bNoFile || bNoSelection;
	this.buttonInvUpload .disable(bDisableUpload);
	this.buttonInvCreate .disable(bDisabled);
	this.listInvData     .disable(bDisabled);
	this.buttonDelInvData.disable(bDisabled || !this.listInvData.hasSelection());
}

Inversion.prototype.saveData = function(data2) {
	var data = this.InversionList.getSelectedData();
	if (!data) return;
	data2.name = this.textName.getText();
	data2.docFile_INP = this.listNetwork.getInpInfo();
	data2.m_InversionList = true;
	data2.input_GRAB = [{"a0":"","a1":0,"a2":0}];
	return data2;
}

Inversion.prototype.setInputs = function(bNothingSelected) {
	this.textName   .setValue  (bNothingSelected ? "" : this.InversionList.data[this.InversionList.selectedIndex].value.name             );
	this.listNetwork.updateData(bNothingSelected ? "" : this.InversionList.data[this.InversionList.selectedIndex].value.docFile_INP.docId);
	this.listInvData.updateData();
}

Inversion.prototype.selectPrevious = function() {
	this.InversionList.selectRowPrevious();
}

Inversion.prototype.selectNext = function() {
	this.InversionList.selectRowNext();
}

Inversion.prototype.checkIncomplete = function(data) {
	// right now this is only the data passed in from the "view" call
	if (data == null) return true;
	if (data.name == null) return true;
	if (data.name.length == 0) return true;
	if (data.docFile_INP == null) return true;
	if (data.docFile_INP.docId == null) return true;
	if (data.docFile_INP.docId.length == 0) return true;
	if (data.docFile_INP.fileName == null) return true;
	if (data.docFile_INP.fileName.length == 0) return true;
	//if (this.listInvData.isEmpty()) return true;
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

//Inversion.prototype.onMouseMove = function(sid) {}
//Inversion.prototype.onMouseOver = function(sid) {}
//Inversion.prototype.onMouseOut = function(sid) {}
//Inversion.prototype.onMouseUp = function(sid) {}
//Inversion.prototype.onMouseDown = function(sid) {}
//Inversion.prototype.onMouseWheel = function(sid) {}
//Inversion.prototype.onKeyDown = function(sid) {}
//Inversion.prototype.onKeyPress = function(sid) {}
//Inversion.prototype.onKeyUp = function(sid) {}
//Inversion.prototype.onInput = function(sid) {}
//Inversion.prototype.onChange = function(sid) {}
//Inversion.prototype.onClick = function(sid) {}

Inversion.prototype.onDblClick = function(sid) {
	var sidPrefix = getPrefix2(sid);
	var id        = getSuffix2(sid);
	switch (sidPrefix) {
		case this.listInvData.sid:
			var uuid = this.listInvData.getSelectedAttr("data-value");
			var fileName = this.listInvData.getText();
			var url = GlobalData.CouchDb + uuid + "/" + fileName;
			window.open(url, uuid);
			break;
	}
}

Inversion.prototype.onMouseUp = function() {
	this.InversionList.onMouseUp();
}

Inversion.prototype.addListeners = function(bSkip) {
	//console.log("Inversion.addListeners");
	if (bSkip == null) bSkip = false;
	if (!bSkip) this.InversionList.addListeners();
	this.buttonInvUpload.addListeners();
	this.buttonInvCreate.addListeners();
	this.chooserInvData.addListeners();
	this.buttonDelInvData.addListeners();
}

Inversion.prototype.isVisible = function() {
	return this.InversionList.isVisible();
}

Inversion.prototype.handleInputPopupEvents = function(e) {
	if (e.event == null) return;
	switch (e.event) {
		case "ok":
			var data = this.chooserInvData.createNewData();
			data.fileName = "User" + e.count.toString().lpad("0", 4) + ".txt";
			data.text = e.text;
			Couch.createUniqueId(this, function(uuid) {
				Couch.setDoc(this, uuid, data, function(e,res) {
					Couch.Upload(res.data.id, data.fileName, res.data.rev, "text/plain", data.text, function(e,res) {
						var data = res.data;
					});
				});
			});
			break;
	}
}

Inversion.prototype.onClick = function(sid) {
	var m_this = this;
	switch(sid) {
		case this.buttonInvUpload.sid:
			this.chooserInvData.upload(m_Waiting);
			break;
		case this.buttonInvCreate.sid:
			if (this.InputPopup == null) {
				this.InputPopup = new InputPopup("gAll", "inputPopupCreateMessureFile", null, null, 300, 180);
				var str1 = "# this is a space-delimited text file with 3 columns:\n"
				var str2 = "# NodeName Time Concentration\n"
				this.InputPopup.setValue(str1 + str2);
				this.InputPopup.registerListener(this.uniqueString, this, this.handleInputPopupEvents);
			} else {
				this.InputPopup.show();
			}
		case this.listInvData.sid:
			this.buttonDelInvData.disable(!this.listInvData.hasSelection());
			break;
		case this.buttonDelInvData.sid:
			var uuid = this.listInvData.getSelectedValue();
			Couch.DeleteDoc(uuid);
			break;
		default:
			break;
	}
}

Inversion.prototype.onKeyDown = function() {
	this.InversionList.onKeyDown("");
}

Inversion.prototype.onInput = function(sid) {
	switch (sid) {
		case this.textName.sid:
			this.checkModified();
			break;
	}
}

Inversion.prototype.onChange = function(sid) {
	switch(sid) {
		case this.listNetwork.sid:
			this.checkModified();
			break;
		case this.chooserInvData.sid:
			this.buttonInvUpload.disable(!this.chooserInvData.hasFile());
			break;
		case this.listInvData.sid:
			this.buttonDelInvData.disable(!this.listInvData.hasSelection());
			break;
		default:
			this.buttonInvUpload.disable(!this.chooserInvData.hasFile() || !this.InversionList.hasSelection());
			this.buttonDelInvData.disable(!this.listInvData.hasSelection());
			break;
	}
}
