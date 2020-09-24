// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function GrabSample(nPanel, dataInFrontOf, parentOnMouseOver) {
	var uniqueString = "Grab";
//	this.CouchDoc = "m_GrabList";
//	this.CouchView = GlobalData.CouchDoc + "m_GrabList";
	this.nPanel = nPanel;
	this.dataInFrontOf = dataInFrontOf;
//	var close = new CloseButton(nPanel, dataInFrontOf);
	d3.select("#g" + nPanel).append("g").attr("id","g" + nPanel + "i").style("visibility","hidden");
	this.GrabList = new RunList(uniqueString, "g" + nPanel + "i", "svg" + nPanel, dataInFrontOf, parentOnMouseOver, 30, 40, 200, 300);
	this.GrabList.SaveButton.setPosition({"x":320,"y":300});
	this.GrabList.parent = this;
	this.GrabList.CouchExe = "_grab";//Required by RunList()
	this.results = null;
}

GrabSample.prototype.updateData = function(bFirstTime) {
	var m_this = this;
	if (bFirstTime)
		this.createInputs();
	this.GrabList.updateData(bFirstTime, function() {
		m_this.addListeners();
	});
	this.listGrabData.updateData();
}

// Static
GrabSample.cleanUpDeleted = function(uuid) {
	GrabSample.getScenariosList(uuid, function(data) {
		for (var i = 0; i < data.length; i++) {
			Couch.DeleteDoc(data[i].id, function(e,res) {
				var data = (res) ? res.data : [];
			});
		}
	});
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

GrabSample.prototype.createNewData = function() {
	var data = {};
	data.status = "New";
	data.statusOld = "";
	data.m_GrabList = true;
	data.name = "G" + ("" + new Date().getTime()).substring(3);
	data.sampleCount = "",
	data.sampleTime = "",
	data.docFile_INP = {"docId":"","fileName":"","wqmFile":""};
	//data.input_TSG = [{"a0":"","a1":null,"a2":null}];
	data.Date = new Date();
	return data;
}

GrabSample.prototype.createInputs = function() {
	var m_this = this;
	//
	this.labelGrabName     = new Label      ("Name:",              "svg" + this.nPanel      , "labelGrabName"    , this.dataInFrontOf, 255,  60,  50, 20);
	this.textName          = new Textbox    (                        "g" + this.nPanel + "i", "textGrabName"     , this.dataInFrontOf, 330,  60, 100);
	this.labelGrabNet      = new Label      ("Network:",           "svg" + this.nPanel      , "labelGrabName"    , this.dataInFrontOf, 255,  80,  50, 20);
	this.listNetwork       = new NetworkList(                        "g" + this.nPanel + "i", "listGrabNetwork"  , this.dataInFrontOf, 330,  80, 100+7.5);
	//
	this.labelGrabCount    = new Label      ("Sample count:"     , "svg" + this.nPanel      , "labelGrabCount"   , this.dataInFrontOf, 255, 100,  50, 20);
	this.textGrabCount     = new Textbox    (                        "g" + this.nPanel + "i", "textGrabCount"    , this.dataInFrontOf, 360, 100,  70);
	this.labelGrabTime     = new Label      ("Sample time (h:m):", "svg" + this.nPanel      , "labelGrabTime"    , this.dataInFrontOf, 255, 120,  50, 20);
	this.textGrabTime      = new Timebox    (                        "g" + this.nPanel + "i", "textGrabTime"     , this.dataInFrontOf, 360, 120,  70);
	//
	this.labelGrabData     = new Label      ("Scenarios:",         "svg" + this.nPanel      , "labelGrabData"    , this.dataInFrontOf, 255, 140,  50, 20);
	this.chooserGrabData   = new FileChooser(                        "g" + this.nPanel + "i", "chooserGrabData"  , this.dataInFrontOf, 253, 160);
	this.buttonGrabUpload  = new Button     ("Upload",               "g" + this.nPanel + "i", "buttonGrabUpload" , this.dataInFrontOf, 255, 180,  78, 20); 
	this.buttonGrabCreate  = new Button     ("Create new",           "g" + this.nPanel + "i", "buttonGrabCreate" , this.dataInFrontOf, 355, 180,  78, 20); 
	this.listGrabData      = new Dropdown   (                        "g" + this.nPanel + "i", "listGrabData"     , this.dataInFrontOf, 255, 200, 177, null, 4);
	this.buttonDelGrabData = new Button     ("Delete",               "g" + this.nPanel + "i", "buttonGrabDelData", this.dataInFrontOf, 255, 265,  50, 20);
	//
	this.buttonGrabUpload.parent = this;
	this.buttonGrabCreate.parent = this;
	this.buttonDelGrabData.parent = this;
	this.listGrabData.parent = this;
	//
	this.textGrabCount.setPlaceHolderText("3");
	this.textGrabTime .setPlaceHolderText("0:00");
	//
	this.d3grouppath = d3.select("#svg" + this.nPanel).append("path").attr("id","pathGrabInputs").attr("data-InFrontOf",this.dataInFrontOf)
		.attr("d","M245,50 v240 h203 v-240 Z")
		.attr("stroke","rgba(49,49,49,0.5)")
		.attr("fill","none")
		;
	//
	this.updateNetworkList();
	//
	this.chooserGrabData.onChange = function() {
		m_this.onChange(m_this.chooserGrabData.sid);
	}
	this.chooserGrabData.createNewData = function() {
		var data = {
			"Date"            : new Date(),
			"m_ScenariosList" : true,
			"fileName"        : this.getFileName(),
			"uuidGrab"        : m_this.GrabList.getSelectionUuid()
		};
		return data;
	}
	//
	this.listGrabData.updateData = function() {
		var uuid = m_this.GrabList.getSelectionUuid();
		GrabSample.getScenariosList(uuid, function(data) {
			m_this.listGrabData.updateList(data);
			m_this.onChange();
		});
	}
	this.listGrabData.setAttributes = function(sel) {
		sel.attr("data-rev",     function(d,i) { 
			return d.value.rev;      });
		sel.attr("data-date",    function(d,i) { 
			return d.value.date;     });
		sel.attr("data-grabId",  function(d,i) {
			return d.value.uuid;     });
		sel.attr("data-value",   function(d,i) {
			return d.id;             });
		sel.text(                function(d,i) { 
			return d.value.fileName; });
	}
	this.listGrabData.onClick = function() {
		m_this.onClick(m_this.listGrabData.sid);
	}
	this.listGrabData.onChange = function() {
		m_this.onChange(m_this.listGrabData.sid);
	}
	this.listGrabData.updateData();
	//
	this.buttonDelGrabData.onClick = function() {
		m_this.onClick(m_this.buttonDelGrabData.sid);
	}
}

// Static
GrabSample.getScenariosList = function(uuid, fCallback) {
	var query = (uuid == null) ? "" : "?key=\"" + uuid + "\"";
	Couch.GetView(Couch.ScenariosList + query, function(data) {
		fCallback(data.rows);
	});
}

GrabSample.prototype.updateNetworkList = function() {
	var selectedData = this.GrabList.getSelectedData();
	var uuid = (selectedData == null) ? "" : selectedData.value.docFile_INP.docId;
	this.listNetwork.updateData(uuid);
}

GrabSample.prototype.updateLabels = function() {
	this.labelGrabName .update();
	this.labelGrabCount.update();
	this.labelGrabTime .update();
	this.labelGrabNet  .update();
	this.labelGrabData .update();
}

GrabSample.prototype.viewNetwork = function() {
	var m_this = this;
	m_Waiting.show();
	var ids = "";
	var uuid = m_this.GrabList.getSelectionUuid();
	GrabSample.getScenariosList(uuid, function(list) {
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
					var line = lines[i].split("\t");
					var id   = line[0];
					var type = line[1];
					var c    = line[2];
					var t0   = line[3];
					var t1   = line[4];
					if (id.length == 0) continue;
					data.push({"id":id, "type":type, "c":c, "t0":t0, "t1":t1});
					if (ids.indexOf(id + " ") > -1) continue;
					ids += id + " ";
				}
				if (++ifile < nfiles) fLoop();
				else fContinue();
			});
		};
		fLoop();
	});

	var fContinue = function() {
		var uuid = m_this.GrabList.getSelectionUuid();
		Couch.GetDoc(uuid, function(data) {
			if (!data) return;
			var grabSampleResults = data.results;
			m_this.results = data.results;
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
				if (!data) return;
				var dotSize = data.dotSize;
				var drawTransform = data.drawTransform;
				var map = data.map;
				//var toggle = data.toggle;
				var json = data.jsonFile;
				Couch.GetDoc(uuid + "/" + json, function(data) {
					if (!data) return;
					data.m_GrabList = true;
					data.dotSize = dotSize;
					data.drawTransform = drawTransform;
					data.map = map;
					//data.toggle = toggle;
					data.uuid = uuid;
					data.Sensors = ids;
					data.grabSampleResults = grabSampleResults;
					var grabUuid = m_this.GrabList.getSelectionUuid();
					m_NetworkView.viewNetwork(data);
					//
					var results = grabSampleResults;
					var sel = m_NetworkView.getNodes();
					sel.attr("fill",function(d,i) {
						for (var i = 0; i < results.Nodes.length; i++) {
							if (results.Nodes[i] == d.id) return "red";
						}
						return "black";
					});
					sel.attr("fill-opacity",function(d,i) {
						for (var i = 0; i < results.Nodes.length; i++) {
							if (results.Nodes[i] == d.id) return 0.8;
						}
						return 0.2;
					});
					//
					d3.select("#gAll").style("visibility","");
				});
			});
		});
	};
}

GrabSample.prototype.checkValues = function() {
	var data = this.GrabList.getSelectedData();
	if (data == null) return false;
	if (this.textName     .checkValue(data.value.name             )) return true;
	if (this.textGrabCount.checkFloat(data.value.sampleCount      )) return true;
	if (this.textGrabTime .checkTime (data.value.sampleTime       )) return true;
	if (this.listNetwork  .checkValue(data.value.docFile_INP.docId)) return true;
	return false;
}

GrabSample.prototype.checkModified = function() {
	this.GrabList.checkModified();
}

GrabSample.prototype.disableInputs = function(bDisabled) {
	this.labelGrabName    .disable(bDisabled);
	this.textName         .disable(bDisabled);
	this.labelGrabNet     .disable(bDisabled);
	this.listNetwork      .disable(bDisabled);
	this.labelGrabCount   .disable(bDisabled);
	this.textGrabCount    .disable(bDisabled);
	this.labelGrabTime    .disable(bDisabled);
	this.textGrabTime     .disable(bDisabled);
	this.labelGrabData    .disable(bDisabled);
	this.chooserGrabData  .disable(bDisabled);
	var bNoFile = !this.chooserGrabData.hasFile();
	var bNoSelection = !this.GrabList.hasSelection();
	var bDisableUpload = bDisabled || bNoFile || bNoSelection;
	this.buttonGrabUpload .disable(bDisableUpload);
	this.buttonGrabCreate .disable(bDisabled);
	this.listGrabData     .disable(bDisabled);
	this.buttonDelGrabData.disable(bDisabled || !this.listGrabData.hasSelection());
}

GrabSample.prototype.saveData = function(data2) {
	var data = this.GrabList.getSelectedData();
	if (!data) return;
	data2.name = this.textName.getText();
	data2.sampleCount = this.textGrabCount.getFloat();
	data2.sampleTime = this.textGrabTime.getSeconds();
	data2.docFile_INP = this.listNetwork.getInpInfo();
	data2.m_GrabList = true;
	//data2.input_TSG = [{"a0":"","a1":"","a2":"","a3":"","a4":""}];
	return data2;
}

GrabSample.prototype.setInputs = function(bNothingSelected) {
	this.textName     .setValue  (bNothingSelected ? "" : this.GrabList.data[this.GrabList.selectedIndex].value.name                 );
	this.textGrabCount.setValue  (bNothingSelected ? "" : this.GrabList.data[this.GrabList.selectedIndex].value.sampleCount      , "");
	this.textGrabTime .setSeconds(bNothingSelected ? "" : this.GrabList.data[this.GrabList.selectedIndex].value.sampleTime       , "");
	this.listNetwork  .updateData(bNothingSelected ? "" : this.GrabList.data[this.GrabList.selectedIndex].value.docFile_INP.docId    );
	this.listGrabData .updateData();
}

GrabSample.prototype.selectPrevious = function() {
	this.GrabList.selectRowPrevious();
}

GrabSample.prototype.selectNext = function() {
	this.GrabList.selectRowNext();
}

GrabSample.prototype.checkIncomplete = function(data) {
	// right now this is only the data passed in from the "view" call
	if (data == null) return true;
	if (data.name == null) return true;
	if (data.name.length == 0) return true;
	if (data.docFile_INP == null) return true;
	if (data.docFile_INP.docId == null) return true;
	if (data.docFile_INP.docId.length == 0) return true;
	if (data.docFile_INP.fileName == null) return true;
	if (data.docFile_INP.fileName.length == 0) return true;
	//if (this.listGrabData.isEmpty()) return true;
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

//GrabSample.prototype.onMouseMove = function(sid) {}
//GrabSample.prototype.onMouseOver = function(sid) {}
//GrabSample.prototype.onMouseOut = function(sid) {}
//GrabSample.prototype.onMouseUp = function(sid) {}
//GrabSample.prototype.onMouseDown = function(sid) {}
//GrabSample.prototype.onMouseWheel = function(sid) {}
//GrabSample.prototype.onKeyDown = function(sid) {}
//GrabSample.prototype.onKeyPress = function(sid) {}
//GrabSample.prototype.onKeyUp = function(sid) {}
//GrabSample.prototype.onInput = function(sid) {}
//GrabSample.prototype.onChange = function(sid) {}
//GrabSample.prototype.onClick = function(sid) {}
GrabSample.prototype.onDblClick_hide = function(sid) {
	var process = function() {
		var above = 0;
		var below = 0;
		for (var i = 0; i < 1e7; i++) {
			if (Math.random() * 2 > 1) {
				above++;
			} else {
				below++;
			}
		}
	}
	//
	var test1 = function() {
		var start = new Date().getTime();
		for (var i = 0; i < 100; i++) {
			process();
		}
		alert("time = " + (new Date().getTime() - start));
	}
	//
	var test2 = function() {
		var start = new Date().getTime();
		var i = 0;
		var imax = 100;
		var busy = false;
		var processor = setInterval(function() {
			if (!busy) {
				busy = true;
				process()
				if (i++ == imax) {
					clearInterval(processor);
					alert("time = " + (new Date().getTime() - start));
				}
				busy = false;
			}
		}, 50);
	}
	//
	test2();
}

GrabSample.prototype.onDblClick = function(sid) {
	var sidPrefix = getPrefix2(sid);
	var id        = getSuffix2(sid);
	switch (sidPrefix) {
		case this.listGrabData.sid:
			var uuid = this.listGrabData.getSelectedAttr("data-value");
			var fileName = this.listGrabData.getText();
			window.open(GlobalData.CouchDb + uuid + "/" + fileName, uuid);
			break;
	}
}

GrabSample.prototype.onMouseUp = function() {
	this.GrabList.onMouseUp();
}

GrabSample.prototype.addListeners = function(bSkip) {
	if (bSkip == null) bSkip = false;
	if (!bSkip) this.GrabList.addListeners();
	this.buttonGrabUpload.addListeners();
	this.buttonGrabCreate.addListeners();
	this.chooserGrabData.addListeners();
	this.buttonDelGrabData.addListeners();
}

GrabSample.prototype.isVisible = function() {
	return this.GrabList.isVisible();
}

GrabSample.prototype.handleInputPopupEvents = function(e) {
	if (e.event == null) return;
	switch (e.event) {
		case "ok":
			var data = this.chooserGrabData.createNewData();
			data.fileName = "User" + e.count.toString().lpad("0", 4) + ".tsg";
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

GrabSample.prototype.onClick = function(sid) {
	var m_this = this;
	switch(sid) {
		case this.buttonGrabUpload.sid:
			this.chooserGrabData.upload(m_Waiting);
			break;
		case this.buttonGrabCreate.sid:
			if (this.InputPopup == null) {
				this.InputPopup = new InputPopup("gAll", "inputPopupCreateGrabFile", null, null, 300, 180);
				var str1 = "# this is a space-delimited text file with 5 columns:\n"
				var str2 = "# NodeName InjectionType Strength StartTime EndTime\n"
				this.InputPopup.setValue(str1 + str2);
				this.InputPopup.registerListener(this.uniqueString, this, this.handleInputPopupEvents);
			} else {
				this.InputPopup.show();
			}
		case this.listGrabData.sid:
			this.buttonDelGrabData.disable(!this.listGrabData.hasSelection());
			break;
		case this.buttonDelGrabData.sid:
			var uuid = this.listGrabData.getSelectedValue();
			Couch.DeleteDoc(uuid);
			break;
		default:
			break;
	}
}

GrabSample.prototype.onKeyDown = function() {
	this.GrabList.onKeyDown("");
}

GrabSample.prototype.onInput = function(sid) {
	switch (sid) {
		case this.textName.sid:
		case this.textGrabCount.sid:
		case this.textGrabTime.sid:
			this.checkModified();
			break;
	}
}

GrabSample.prototype.onChange = function(sid) {
	switch(sid) {
		case this.listNetwork.sid:
			this.checkModified();
			break;
		case this.chooserGrabData.sid:
			this.buttonGrabUpload.disable(!this.chooserGrabData.hasFile());
			break;
		case this.listGrabData.sid:
			this.buttonDelGrabData.disable(!this.listGrabData.hasSelection());
			break;
		default:
			this.buttonGrabUpload.disable(!this.chooserGrabData.hasFile() || !this.GrabList.hasSelection());
			this.buttonDelGrabData.disable(!this.listGrabData.hasSelection());
			break;
	}
}
