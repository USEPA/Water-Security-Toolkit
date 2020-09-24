// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

//
// Constructor
//
function EventView(sParentId) {
	var uniqueString = "EventView";
	this.uniqueString = uniqueString;
	this.sParentId = sParentId;
	this.gid = "g" + this.uniqueString;
	this.svgbgid = "svg" + this.uniqueString + "Bg";
	this.rectbgid = "rect" + this.uniqueString + "Bg";
	this.svgid = "svg" + this.uniqueString;
	this.rectid = "rect" + this.uniqueString;
	this.closerectid = "rectClose" + this.uniqueString;
	this.closetextid = "textClose" + this.uniqueString;
	this.closeid = "close" + this.uniqueString;
	this.data = {};
	this.local_save_data = {};
	//
	GlobalData.resizeHooks[uniqueString] = {"this": this, "function": this.resizeWindow};
	//
	//
	// Private properties
	//
	var m_training = false;
	var m_uuid = null;
	var m_uuids = null;
	//
	// Private functions
	//
	function setTraining_private(value) {
		m_training = value;
	}
	function setUuid_private(value) {
		m_uuid     = value ? value : ""  ;
	}
	function setUuids_private(value) {
		m_uuids    = value ? value : ""  ;
	}
	//
	function getTraining_private() {
		return m_training;
	}
	function getUuid_private() {
		return m_uuid;
	}
	function getUuids_private() {
		return m_uuids;
	}
	//
	// Privileged functions
	//
	this.setTraining = function(value) { 
		setTraining_private(value); 
	}
	this.setUuid     = function(value) {
		var old = getUuid_private();
		if (old == value) return;
		setUuid_private(value);
		this.GrabGrid.updateDataGrid([]);
	}
	this.setUuids    = function(value) {
		var old = getUuids_private();
		if (old == null) old = {};
		var b0 = (old.uuid   == value.uuid  ); 
		var b1 = (old.log    == value.log   );
		var b2 = (old.grab   == value.grab  );
		var b3 = (old.canary == value.canary);
		if (b0 && b1 && b2 && b3) return;
		setUuids_private(value);
		this.GrabGrid.updateDataGrid([]);
	}
	//
	this.getTraining = function() { 
		return getTraining_private();
	}
	this.getExe = function() {
		return this.getTraining() ? Couch.training : Couch.events;
	}
	this.getUuid     = function() {
		return getUuid_private();
	}
	this.getUuids    = function() {
		return getUuids_private();
	}
	//
	this.create();
}

EventView.COLOR_TEXT_BACKGROUND = d3.rgb(255,255,255);
EventView.COLOR_TEXT_ABNORMAL   = d3.rgb(255,182,193);

//
// Public Member Methods
//
EventView.prototype.create = function() {
	var m_this = this;
	this.d3parent = d3.select("#" + this.sParentId);
	this.d3g =
	this.d3parent.append("g").attr("id", this.gid)
		.attr("class", this.uniqueString)
		.style("position","absolute")
		.style("top", convertToPx(0))
		.style("left", convertToPx(0))
		.style("visibility", "hidden")
		;
	this.d3svgbg = 
	d3.select("#" + this.gid).append("svg").attr("id", this.svgbgid)
		.attr("class", this.uniqueString)
		.style("position", "absolute")
		.style("top", convertToPx(0))
		.style("left", convertToPx(0))
		;
	this.d3rectbg = 
	d3.select("#" + this.svgbgid).append("rect").attr("id", this.rectbgid)
		.attr("class", this.uniqueString)
		.attr("x", 0)
		.attr("y", 0)
		.attr("fill", d3.rgb(0, 0, 0))
		.attr("fill-opacity", 0.5)
		;
	this.d3svg = 
	d3.select("#" + this.gid).append("svg").attr("id", this.svgid)
		.attr("class", this.uniqueString)
		.style("position", "absolute")
		.style("left", convertToPx(NETWORK_VIEW_BORDER))
		.style("top" , convertToPx(NETWORK_VIEW_BORDER))
		;
	this.d3rect =
	d3.select("#" + this.svgid).append("rect").attr("id", this.rectid)
		.attr("class", this.uniqueString)
		.attr("x", 0)
		.attr("y", 0)
		.attr("fill", d3.rgb(200, 200, 200))
		.attr("fill-opacity", 1.0)
		;
	////////////////////////////////////////////////////////////////////////////////
	var GridTextFill = function(d,i) {
		switch(d.value.status) {
			case "Abnormal":
				return "rgb(230,0,0)";
			default:
				return "rgb(0,0,0)";
		}
	}
	var GridRowFill = function(d,i) {
		// NOTE: instead of the "this" refering to the rect being acted upon
		//       its the DataGrid object.
		var bsel = (i == this.selectedIndex);
		switch(d.value.status) {
			case "Abnormal":
				return (bsel) ? "rgb(146,159,179)" : "rgb(255,182,193)";
			default:
				return (bsel) ? "rgb(146,159,179)" : "rgb(235,235,235)";
		}
	}
	////////////////
	this.d3CanaryTitle =
	d3.select("#" + this.svgid).append("text").attr("id", "CanaryTitle")
		.attr("x", 85)
		.attr("y", 30)
		.attr("font-size", 20)
		.text("Canary Data")
		;
	this.CanaryGrid = new DataGrid(NETWORK_VIEW_BORDER + 20, NETWORK_VIEW_BORDER + 40, 240, 480);
	this.CanaryGrid.sParentId = this.gid;
	this.CanaryGrid.uniqueString = "CanaryGrid"
	this.CanaryGrid.gid = "gCanary";
	this.CanaryGrid.parentOnMouseOver = this.gridOnMouseOver;
	this.CanaryGrid.bgFill = "rgba(150, 150, 150, 0.5)";
	this.CanaryGrid.nColumns = 4;
	//this.CanaryGrid.colNames = [];
	//this.CanaryGrid.colNames.push("time");
	//this.CanaryGrid.colNames.push("location");
	//this.CanaryGrid.colNames.push("status");
	this.CanaryGrid.colNames = function(d, irow, icol) {
		var bTraining = m_this.getTraining();
		switch (icol) {
			case 0:
				if (bTraining) {
					var nSeconds = d.value.time;
					var obj = GlobalData.convertSecondsToDays(nSeconds);
					return "Day " + (obj.days + 1);
				} else {
					var epoch = d.value.time;
					var datetime = new Date(epoch * 1000);
					var nMonth = datetime.getMonth();
					var sMonth = GlobalData.Months[nMonth];
					var nDate = datetime.getDate();
					return sMonth + " " + nDate;
				}
			case 1:
				if (bTraining) {
					var nSeconds = d.value.time;
					var obj = GlobalData.convertSecondsToDays(nSeconds);
					return obj.time;
				} else {
					var epoch = d.value.time;
					var datetime = new Date(epoch * 1000);
					var nHours = datetime.getHours();
					var nMinutes = datetime.getMinutes();
					var sMinutes = ("" + nMinutes).lpad("0", 2);
					return nHours + ":" + sMinutes;
				}
			case 2:
				return d.value["location"];
			case 3:
				return d.value["status"];
		}
	}
	this.CanaryGrid.colTitles = [];
	this.CanaryGrid.colTitles.push("Day");
	this.CanaryGrid.colTitles.push("Time");
	this.CanaryGrid.colTitles.push("Location");
	this.CanaryGrid.colTitles.push("Status");
	this.CanaryGrid.colWidth = [30, 35, 60];
	this.CanaryGrid.textFill = [];
	this.CanaryGrid.textFill.push(GridTextFill);
	this.CanaryGrid.textFill.push(GridTextFill);
	this.CanaryGrid.textFill.push(GridTextFill);
	this.CanaryGrid.textFill.push(GridTextFill);
	this.CanaryGrid.rowFill = GridRowFill;
	this.CanaryGrid.createDataGrid([]);
	this.CanaryGrid.registerListener(this.CanaryGrid.sid, this, this.handleGridChange);
	////////////////////////////////////////////////////////////////////
	this.d3GrabTitle =
	d3.select("#" + this.svgid).append("text").attr("id", "GrabTitle")
		.attr("x", 385)
		.attr("y", 30)
		.attr("font-size", 20)
		.text("Grab Data")
		;
	this.GrabGrid = new DataGrid(NETWORK_VIEW_BORDER + 300, NETWORK_VIEW_BORDER + 40, 260, 300);
	this.GrabGrid.sParentId = this.gid;
	this.GrabGrid.uniqueString = "GrabEventGrid"
	this.GrabGrid.gid = "gGrabEvent";
	this.GrabGrid.parentOnMouseOver = this.gridOnMouseOver;
	this.GrabGrid.bgFill = "rgba(150, 150, 150, 0.5)";
	this.GrabGrid.nColumns = 5;
	//this.GrabGrid.colNames = [];
	//this.GrabGrid.colNames.push("opt");
	//this.GrabGrid.colNames.push("location");
	//this.GrabGrid.colNames.push("time");
	//this.GrabGrid.colNames.push("status");
	this.GrabGrid.colNames = function(d, irow, icol) {
		var bTraining = m_this.getTraining();
		switch (icol) {
			case 0: // Opt
				if (d && d.value && d.value.Error) return d.value.Error;
				if (d.value.opt == null) return "";
				return d.value.opt;
			case 1: // Location
				if (d && d.value && d.value.Error) return "";
				if (isBlank(d.value.location)) return "-";
				if (isNull(d.value.location)) return "-";
				return d.value.location;
			case 2: // Day
				if (d && d.value && d.value.Error) return "";
				if (isBlank(d.value.time)) return "-";
				if (isNull(d.value.time)) return "-";
				if (bTraining) {
					var nSeconds = d.value.time;
					var obj = GlobalData.convertSecondsToDays(nSeconds);
					return "Day " + (obj.days + 1);
				} else {
					if (d.value.time_input_method == "time") return "-";
					var epoch = d.value.time;
					var datetime = new Date(epoch * 1000);
					var nMonth = datetime.getMonth();
					var sMonth = GlobalData.Months[nMonth];
					var nDate = datetime.getDate();
					return sMonth + " " + nDate;
				}
			case 3: // Time
				if (d && d.value && d.value.Error) return "";
				if (isBlank(d.value.time)) return "-";
				if (isNull(d.value.time)) return "-";
				if (bTraining) {
					var nSeconds = d.value.time;
					var obj = GlobalData.convertSecondsToDays(nSeconds);
					return obj.time;
				} else {
					var epoch = d.value.time;
					if (d.value.time_input_method == "time") {
						var obj = GlobalData.convertSecondsToDays(epoch);
						return obj.time;
					} else {
						var datetime = new Date(epoch * 1000);
						var nHours = datetime.getHours();
						var nMinutes = datetime.getMinutes();
						var sMinutes = ("" + nMinutes).lpad("0", 2);
						return nHours + ":" + sMinutes;
					}
				}
			case 4: // Status
				if (d && d.value && d.value.Error) return "";
				if (isBlank(d.value.status)) return "-";
				if (isNull(d.value.status)) return "-";
				return d.value.status;
		}
	}
	this.GrabGrid.colTitles = [];
	this.GrabGrid.colTitles.push("Opt.");
	this.GrabGrid.colTitles.push("Location");
	this.GrabGrid.colTitles.push("Day");
	this.GrabGrid.colTitles.push("Time");
	this.GrabGrid.colTitles.push("Status");
	this.GrabGrid.colWidth = [20, 60, 30, 35];
	this.GrabGrid.textFill = [];
	this.GrabGrid.textFill.push(GridTextFill);
	this.GrabGrid.textFill.push(GridTextFill);
	this.GrabGrid.textFill.push(GridTextFill);
	this.GrabGrid.textFill.push(GridTextFill);
	this.GrabGrid.textFill.push(GridTextFill);
	this.GrabGrid.rowFill = GridRowFill;
	this.GrabGrid.createDataGrid([]);
	this.GrabGrid.registerListener(this.GrabGrid.sid, this, this.handleGridChange);
	//
	////////////////////////////////////////////////////////////////////
	//
	this.d3grouppath = 
	d3.select("#" + this.svgid).append("path").attr("id", "pathEventInputs")
		.attr("d", "M600,50 v546 h300 v-546 Z")
		.attr("stroke", d3.rgb(49,49,49))
		.attr("stroke-opacity", 0.5)
		.attr("fill", "none")
		;
	this.d3group1path = 
	d3.select("#" + this.svgid).append("path").attr("id", "pathEventInputs1")
		.attr("d", "M610,165 v85 h280 v-85 Z")
		.attr("d", "M610,191 v85 h280 v-85 Z")
		.attr("stroke", d3.rgb(49,49,49))
		.attr("stroke-opacity", 0.5)
		.attr("fill", "none")
		.attr("stroke-opacity", 0)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.3)
		;
	this.d3group2path = 
	d3.select("#" + this.svgid).append("path").attr("id", "pathEventInputs2")
		.attr("d", "M610,253 v85 h280 v-85 Z")
		.attr("d", "M610,279 v85 h280 v-85 Z")
		.attr("stroke", d3.rgb(49,49,49))
		.attr("stroke-opacity", 0.5)
		.attr("fill", "none")
		.attr("stroke-opacity", 0)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.4)
		;
	this.d3group3path = 
	d3.select("#" + this.svgid).append("path").attr("id", "pathEventInputs3")
		.attr("d", "M610,341 v85 h280 v-85 Z")
		.attr("d", "M610,367 v85 h280 v-85 Z")
		.attr("stroke", d3.rgb(49,49,49))
		.attr("stroke-opacity", 0.5)
		.attr("fill", "none")
		.attr("stroke-opacity", 0)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		;
	this.d3group4path = 
	d3.select("#" + this.svgid).append("path").attr("id", "pathEventInputs4")
		.attr("d", "M610,429 v59 h280 v-59 Z")
		.attr("d", "M610,455 v59 h280 v-59 Z")
		.attr("stroke", d3.rgb(49,49,49))
		.attr("stroke-opacity", 0.5)
		.attr("fill", "none")
		.attr("stroke-opacity", 0)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.6)
		;
	////////////////////////////////////////////////////////////////////
	//
	this.suggestedText   = "Set Suggested: ";
	this.overrideText    = "Override Suggested: ";
	this.SuggestedButton = new SvgButton(this.suggestedText    , this, "EventViewButtonSuggested" , null, 650, 547, 240, 25, 13);
	this.OverrideButton  = new SvgButton(this.overrideText     , this, "EventViewButtonOverride"  , null, 650, 580, 240, 25, 13);
	this.CompleteButton  = new SvgButton("Wizard Complete"     , this, "EventViewButtonComplete"  , null,  40,   0, 150, 30, 15);
	this.NewButton       = new SvgButton("New"                 , this, "EventViewGrabButtonNew"   , null, 320,   0,  70, 25, 13);
	this.DeleteButton    = new SvgButton("Delete"              , this, "EventViewGrabButtonDelete", null, 395,   0,  70, 25, 13);
	this.ExportButton    = new SvgButton("Export to GIS"       , this, "EventViewGrabButtonExport", null, 470,   0, 110, 25, 13);
	this.InversionButton = new SvgButton("Perform Inversion...", this, "EventViewButtonInversion" , null, 320,   0, 260, 40, 20);
	//
	var dy = 2;
	var z = 1.3;
	var x = 615;
	var y = 100-dy;
	var w = 160;
	var h = z * 20;
	var g = 10;
	var i = 0;
	this.labelTeam        = new Label("Team name/#:"             , this.svgid, "labelTeam"       , null, x +  0, y + h * i++ + g * 0, w, h);
	this.labelLocation    = new Label("Location:"                , this.svgid, "labelLocation"   , null, x +  0, y + h * i++ + g * 0, w, h);
	this.labelDay         = new Label("Day:"                     , this.svgid, "labelDay"        , null, x +  0, y + h * i++ + g * 0, w, h);
	this.labelTime        = new Label("Time (h:m):"              , this.svgid, "labelTime"       , null, x +  0, y + h * i++ + g * 0, w, h);
	this.labelChlorine    = new Label("Chlorine Residual (mg/l):", this.svgid, "labelChlorine"   , null, x +  0, y + h * i++ + g * 1, w, h);
	this.labelChlorineMax = new Label("- upper limit:"           , this.svgid, "labelChlorineMax", null, x + 95, y + h * i++ + g * 1, w, h);
	this.labelChlorineMin = new Label("- lower limit:"           , this.svgid, "labelChlorineMin", null, x + 95, y + h * i++ + g * 1, w, h);
	this.labelpH          = new Label("pH:"                      , this.svgid, "labelpH"         , null, x +  0, y + h * i++ + g * 2, w, h);
	this.labelpHMax       = new Label("- upper limit:"           , this.svgid, "labelpHMax"      , null, x + 95, y + h * i++ + g * 2, w, h);
	this.labelpHMin       = new Label("- lower limit:"           , this.svgid, "labelpHMin"      , null, x + 95, y + h * i++ + g * 2, w, h);
	this.labelCond        = new Label("Conductivity (uS/cm):"    , this.svgid, "labelCond"       , null, x +  0, y + h * i++ + g * 3, w, h);
	this.labelCondMax     = new Label("- upper limit:"           , this.svgid, "labelCondMax"    , null, x + 95, y + h * i++ + g * 3, w, h);
	this.labelCondMin     = new Label("- lower limit:"           , this.svgid, "labelCondMin"    , null, x + 95, y + h * i++ + g * 3, w, h);
	this.labelTurb        = new Label("Turbidity (NTU):"         , this.svgid, "labelTurb"       , null, x +  0, y + h * i++ + g * 4, w, h);
	this.labelTurbMax     = new Label("- upper limit:"           , this.svgid, "labelTurbMax"    , null, x + 95, y + h * i++ + g * 4, w, h);
	//
	this.labelTeam       .enlarge(z);
	this.labelLocation   .enlarge(z);
	this.labelDay        .enlarge(z);
	this.labelTime       .enlarge(z);
	this.labelChlorine   .enlarge(z);
//	this.labelChlorineMax.enlarge(z);
//	this.labelChlorineMin.enlarge(z);
	this.labelpH         .enlarge(z);
//	this.labelpHMax      .enlarge(z);
//	this.labelpHMin      .enlarge(z);
	this.labelCond       .enlarge(z);
//	this.labelCondMax    .enlarge(z);
	this.labelTurb       .enlarge(z);
//	this.labelTurbMax    .enlarge(z);
	//
	var x = 800;
	var y = 100;
	var w = 100;
	var h = z * 20;
	var i = -1;
	this.textUuid        = new Textbox(this.gid, "textEventGrabUuid"       , null, x, y + h * i++ + g * 0, w);
	this.textTeam        = new Textbox(this.gid, "textEventGrabTeam"       , null, x, y + h * i++ + g * 0, w);
	this.textLocation    = new Textbox(this.gid, "textEventGrabLocation"   , null, x, y + h * i++ + g * 0, w);
	//
	this.listDay        = new Dropdown(this.gid, "listEventGrabDay"        , null, x-2, y+h*i+4, w+10, true);
	//
	this.textDay         = new Textbox(this.gid, "textEventGrabDay"        , null, x, y + h * i++ + g * 0, w);
	this.textTime        = new Timebox(this.gid, "textEventGrabTime"       , null, x, y + h * i++ + g * 0, w);
	this.textChlorine    = new Textbox(this.gid, "textEventGrabChlorine"   , null, x, y + h * i++ + g * 1, w);
	this.textChlorineMax = new Textbox(this.gid, "textEventGrabChlorineMax", null, x, y + h * i++ + g * 1, w);
	this.textChlorineMin = new Textbox(this.gid, "textEventGrabChlorineMin", null, x, y + h * i++ + g * 1, w);
	this.textpH          = new Textbox(this.gid, "textEventGrabpH"         , null, x, y + h * i++ + g * 2, w);
	this.textpHMax       = new Textbox(this.gid, "textEventGrabpHMax"      , null, x, y + h * i++ + g * 2, w);
	this.textpHMin       = new Textbox(this.gid, "textEventGrabpHMin"      , null, x, y + h * i++ + g * 2, w);
	this.textCond        = new Textbox(this.gid, "textEventGrabCond"       , null, x, y + h * i++ + g * 3, w);
	this.textCondMax     = new Textbox(this.gid, "textEventGrabCondMax"    , null, x, y + h * i++ + g * 3, w);
	this.textCondMin     = new Textbox(this.gid, "textEventGrabCondMin"    , null, x, y + h * i++ + g * 3, w);
	this.textTurb        = new Textbox(this.gid, "textEventGrabTurb"       , null, x, y + h * i++ + g * 4, w);
	this.textTurbMax     = new Textbox(this.gid, "textEventGrabTurbMax"    , null, x, y + h * i++ + g * 4, w);
	//
	this.textUuid.hide();
	//
	this.listDay.setAttributes = function(sel) {
		sel.attr("data-order", function(d,i) {
			return d.order; });
		sel.attr("data-index", function(d,i) {
			return d.index; });
		sel.attr("data-value", function(d,i) {
			return d.index; });
		sel.attr("data-year", function(d,i) {
			return d.year; });
		sel.attr("data-month", function(d,i) {
			return d.month; });
		sel.attr("data-date", function(d,i) {
			return d.date; });
		sel.attr("data-day", function(d,i) {
			return d.day; });
		sel.attr("data-inpstartday", function(d,i) {
			return d.inpStartDay; });
		sel.attr("data-inpday", function(d,i) {
			return d.inpDay; });
		sel.attr("data-currenttime", function(d,i) {
			return d.currentTime; });
		sel.attr("data-more", function(d,i) {
			return d.more; });
		sel.attr("data-range", function(d,i) {
			return d.range; });
		sel.attr("data-epoch", function(d,i) {
			return d.epoch; });
		sel.text(function(d,i) {
			return d.text; });
	}
	//
	//this.listDay.uniqueAttribute = "data-value";
	//
	this.listDay.updateData = function(epoch) {
		if (isNull(epoch) || isBlank(epoch)) {
			Clock.getServerTime(this, function(data) {
				this.updateDataWithTime(data.epoch);
				this.selectValue("0");
				this.updateDataAddMoreListeners();
			});
		} else {
			this.updateDataWithTime(epoch);
			this.selectUserData("epoch", "" + epoch);
			this.updateDataAddMoreListeners();
		}
	}
	this.listDay.updateDataWithTime = function(epoch, beforeRange, afterRange) {
		if (beforeRange == null) beforeRange = 10;
		if (afterRange == null) afterRange = 10;
		var list = [];
		if (epoch == null) return;
		var item = {}
		item.text = "More..."
		item.more = 1;
		item.range = beforeRange;
		list.push(item);
		for (var i = -beforeRange; i < 0; i++) {
			var item = this.updateDataWithTime2(epoch, i, i + beforeRange);
			list.push(item);
		}
		list.push({"index": 0, "order": beforeRange, "text": "", "currentTime": epoch});
		for (var i = 0; i < afterRange; i++) {
			var item = this.updateDataWithTime2(epoch, i, afterRange + 1 + i);
			list.push(item);
		}
		var item = {}
		item.text = "More..."
		item.more = 2;
		item.range = afterRange;
		list.push(item);
		this.updateList(list);
	}
	this.listDay.updateDataWithTime2 = function(epoch, i, order) {
		var item = {};
		item.order = order;
		item.index = i;
		item.day = i;
		item.currentTime = epoch;
		var date = new Date(epoch * 1000);
		var y = date.getFullYear();
		var m = date.getMonth();
		var n = date.getDate();
		var date = new Date(y, m, n, 0, 0, 0, 0);
		var epoch = parseInt( date.valueOf() / 1000 );
		item.epoch = epoch + 24 * 60 * 60 * i;
		item.date = "";
		var date = new Date(item.epoch * 1000);
		item.year = date.getFullYear();
		item.month = date.getMonth();
		item.date = date.getDate();
		item.day = date.getDay(); // the JSON days start with Sunday as zero
		var sDay = GlobalData.Days[item.day];
		var sMon = GlobalData.Months[item.month];
		item.text = sDay + " " + sMon + " " + item.date;
		item.inpStartDay = 1; // Starts on a Monday (TODO - put this in couch config doc)
		item.inpDay = (item.day < item.inpStartDay) ? item.day - item.inpStartDay + 7 : item.day - item.inpStartDay;
		return item;
	}
	this.listDay.updateDataAddMoreListeners = function() {
		var item1 = this.getItem(1);
		var item2 = this.getLastItem();
		if (item1 == null || item2 == null) return; 
		m_this.addListenersForSelectionForId("option", "#" + item1.id);
		m_this.addListenersForSelectionForId("option", "#" + item2.id);
		m_this.MoreId1 = item1.id;
		m_this.MoreId2 = item2.id;
	}
	//
	//this.textUuid.setValue("Initialized");
	this.textTeam       .enlarge(z);
	this.textLocation   .enlarge(z);
	//this.listDay      .enlarge(z);
	this.textDay        .enlarge(z);
	this.textTime       .enlarge(z);
	this.textChlorine   .enlarge(z);
	this.textChlorineMax.enlarge(z);
	this.textChlorineMin.enlarge(z);
	this.textpH         .enlarge(z);
	this.textpHMax      .enlarge(z);
	this.textpHMin      .enlarge(z);
	this.textCond       .enlarge(z);
	this.textCondMax    .enlarge(z);
	this.textCondMin    .enlarge(z);
	this.textTurb       .enlarge(z);
	this.textTurbMax    .enlarge(z);
	//
	this.textDay        .setRange(0);
	this.textChlorine   .setRange(0);
	this.textChlorineMax.setRange(0);
	this.textChlorineMin.setRange(0);
	this.textpH         .setRange(0,14);
	this.textpHMax      .setRange(0,14);
	this.textpHMin      .setRange(0,14);
	this.textCond       .setRange(0);
	this.textCondMax    .setRange(0);
	this.textCondMin    .setRange(0);
	this.textTurb       .setRange(0);
	this.textTurbMax    .setRange(0);
	//
	this.addListeners();
}
//
EventView.prototype.updateData = function(data, /*optional*/bShow) {
	var bTraining = this.getTraining();
	var bVisible = this.isVisible();
	var bShow = bShow || bVisible;
	var uuid = data._id;
	if (false) {
	} else if (data.status == "Initializing") {
		m_Waiting.show();
	} else if (data.status == "Inversion") {
		this.waiting = false;
		m_Waiting.show();
	} else if (data.status == "Locating") {
		this.waiting = false;
		m_Waiting.show();
	} else if (data.status == "Sampling") {
		m_Waiting.show();
	} else if (data.status == "Exporting") {
		this.waiting = false;
		m_Waiting.show();
	} else {
		//if (!this.waiting)
			m_Waiting.hide();
	}
	if (data.status == "Inverted" && data.view == "network") {
		Events.getInversionListByEvent(uuid, function(data) {
			var value = data[data.length - 1];
			Inversion.staticViewNetwork(value.id, true);
		});
	}
	//
	this.InversionButton.disable(data.alarm == null || data.alarm.time == null);
	this.addListeners();
}

EventView.prototype.updateGrabGrid = function(data) {
	if (data == null) data = {"grab": []};
	this.ExportButton.disable(data.grab.length == 0);
	var bDisable = this.GrabGrid.isSelected();
	//
	this.GrabGrid.updateDataGrid(data.grab);
	//this.addListeners(); // TODO - is this needed? it causes a long delay
	//
	var index = this.GrabGrid.selectedIndex;
	var bDisable = !(index > -1 && index < data.grab.length);
	if (bDisable) {
		this.textTeam    .disable(bDisable);
		this.textLocation.disable(bDisable);
		this.listDay     .disable(bDisable);
		this.textDay     .disable(bDisable);
		this.textTime    .disable(bDisable);
	} else {
		this.disableInputs(data.grab[index].value);
	}
	var bTraining = this.getTraining();
	this.disableMeasurements(bDisable || bTraining);
	this.disableLimits(bDisable || bTraining);
	//
	// set the local data object
	//
	if (bDisable) {
		this.data = {};
	} else {
		this.data = data.grab[index].value;
		delete this.data.save;
		this.data.save = this.cloneGrabData(this.data);
	}	
	this.setButtonText();
	this.disableInputs();
	this.showValues();
//	if (isNotNull(data.index) && data.index > -1) {
//		console.log("EventView.updateGrabGrid(select_index)=" + data.index)	
//		this.GrabGrid.selectRow(data.index);
//	} else {
//		console.log("EventView.updateGrabGrid(deselect)")	
//		this.GrabGrid.deselectAllRows();
//	}
}

EventView.prototype.updateCanaryGrid = function(data) {
	if (data == null) data = {"canary": []};
	this.CanaryGrid.updateDataGrid(data.canary);
	this.addListeners();
	this.CanaryGrid.makeRowVisible(data.canary.length + 1);
}

EventView.prototype.updateLabels = function() {
	this.labelTeam       .update2();
	this.labelLocation   .update2();
	this.labelTime       .update2();
	this.labelChlorine   .update2();
	this.labelChlorineMax.update2();
	this.labelChlorineMin.update2();
	this.labelpH         .update2();
	this.labelpHMax      .update2();
	this.labelpHMin      .update2();
	this.labelCond       .update2();
	this.labelCondMax    .update2();
	this.labelCondMin    .update2();
	this.labelTurb       .update2();
	this.labelTurbMax    .update2();
}

EventView.prototype.disableAll = function(bDisable) {
	//this.disableInputs(bDisable);
	this.disableInputs_old(bDisable);
	this.disableMeasurements(bDisable);
	this.disableLimits(bDisable);
}

EventView.prototype.disableInputs = function(data) {
	if (data == null) data = this.data;
	var bDisable = this.GrabGrid.isSelected();
	//this.textTeam.disable(bDisable);
	var bOptimal = (data.opt == "*");
	var bOptimal = false;
	this.textTeam    .disable(bDisable || bOptimal);
	this.textLocation.disable(bDisable || bOptimal);
	this.listDay     .disable(bDisable || bOptimal);
	this.textDay     .disable(bDisable || bOptimal);
	this.textTime    .disable(bDisable || bOptimal);
}

EventView.prototype.disableInputs_old = function(bDisable) {
	this.textTeam    .disable(bDisable);
	this.textLocation.disable(bDisable);
	this.listDay     .disable(bDisable);
	this.textDay     .disable(bDisable);
	this.textTime    .disable(bDisable);
}

EventView.prototype.disableMeasurements = function(bDisable) {
	this.textChlorine   .disable(bDisable);
	this.textpH         .disable(bDisable);
	this.textCond       .disable(bDisable);
	this.textTurb       .disable(bDisable);
}

EventView.prototype.disableLimits = function(bDisable) {
	this.textChlorineMax.disable(bDisable);
	this.textChlorineMin.disable(bDisable);
	this.textpHMax      .disable(bDisable);
	this.textpHMin      .disable(bDisable);
	this.textCondMax    .disable(bDisable);
	this.textCondMin    .disable(bDisable);
	this.textTurbMax    .disable(bDisable);
}

///////////////////////////////////////////////////////

EventView.prototype.gridOnMouseOver  = function() {} // for "InFrontOf"

EventView.prototype.handleGridChange = function(e) {
	if (e.event == null) return;
	var iOld = e.source.selectedIndexOld;
	var iNew = e.source.selectedIndex;
	var bDisable = (iNew == -1);
	switch (e.source.sid) {
		case this.CanaryGrid.sid:
			return;
		case this.GrabGrid.sid:
			this.DeleteButton   .disable(bDisable);
			this.SuggestedButton.disable(bDisable);
			this.OverrideButton .disable(bDisable);
			if (!bDisable) this.CanaryGrid.deselectAllRows();
			if (iNew == iOld) return;
			//this.onBlurAll();
			this.changeSelection(iOld, iNew);
			e.source.selectedIndexOld = iNew; // TODO - is this right?
			return;
	}
}
//EventView.prototype.onCanaryGridChange = function(e) {}
//EventView.prototype.onGrabGridChange = function(e) {}

EventView.prototype.queueKeyDown = function(keyCode) {
	this.CanaryGrid.onKeyDown("", keyCode);
	this.GrabGrid.onKeyDown("");
}

EventView.prototype.changeSelection = function(iOld, iNew) {
	var uuids = this.getUuids();
	Couch.getDoc(this, uuids.grab, function(data) {
		var bModified = false;
		if (iOld > -1 && iOld < data.grab.length) {
			bModified = this.setData(data.grab[iOld]);
		}
		if (iNew > -1 && iNew < data.grab.length) {
			this.local_save_data = clone(data.grab[iNew].value);
			if (!this.getTraining()) this.listDay.updateData(data.grab[iNew].value.time);
		}
		if (bModified) {
			data.grab[iOld].value.status = "";
			data.grab[iOld].value.suggested = "";
			data.grab[iOld].value.override = "";
			data.index = iNew;
			//console.log("EventView.changeSelection(set_index)=" + iNew);
			Events.log(null, uuids.log, "Change selection");
			this.logData();
			Couch.setDoc(this, uuids.grab, data);
		} else {
			//console.log("EventView.changeSelection(use_index)=" + iNew);
			data.index = iNew;
			this.updateGrabGrid(data);
		}
	});
}

EventView.prototype.logData = function(data) {
	if (data == null) data = this.data;
	var uuids = this.getUuids();
	var list = [];
	list.push("    modified = " + data.modified);
	list.push("    unique id = " + data.uuid);
	var b1 = isNotBlank(data.chlorine);
	var b2 = isNotBlank(data.ph);
	var b3 = isNotBlank(data.cond);
	var b4 = isNotBlank(data.turb);
	if (b1 || b2 || b3 || b4) {
		list.push("    suggested = " + data.suggested);
	}
	this.createLogData(list, "status"      , data.status  );
	this.createLogData(list, "team"        , data.team    );
	this.createLogData(list, "location"    , data.location);
	this.createLogData(list, "time"        , data.time    );
	this.createLogData(list, "chlorine"    , data.chlorine, data.chlorinemin, data.chlorinemax);
	this.createLogData(list, "pH"          , data.ph      , data.phmin      , data.phmax      );
	this.createLogData(list, "conductivity", data.cond    , data.condmin    , data.condmax    );
	this.createLogData(list, "turbitity"   , data.turb    , 0               , data.turbmax    );
	Events.log(null, uuids.log, list);
}

EventView.prototype.createLogData = function(list, s, val, vMin, vMax) {
	if (isBlank(val)) return list;
	var msg1 = (vMin == null && vMax == null) ? "" : " (" + vMin + "-" + vMax + ")";
	var msg2 = "";
	if (val < vMin) msg2 = " low";
	if (val > vMax) msg2 = " high";
	var item = "    " + s + " = " + val + msg1 + msg2;
	list.push(item);
	return list;
}

//
// use this to transfer the cached data into the object that is going to be sent to CouchDB in a .setDoc()
//
EventView.prototype.setData = function(data) {
	var bModified = this.cacheValues();
	var bSame = !bModified;
	if (bSame) return false;
	data.value.uuid        = this.data.uuid       ;
	data.value.team        = this.data.team       ;
	data.value.location    = this.data.location   ;
	data.value.time        = this.data.time       ;
	//
	data.value.time_input_method = this.data.time_input_method;
	//
	data.value.chlorine    = this.data.chlorine   ;
	data.value.chlorinemax = this.data.chlorinemax;
	data.value.chlorinemin = this.data.chlorinemin;
	data.value.ph          = this.data.ph         ;
	data.value.phmax       = this.data.phmax      ;
	data.value.phmin       = this.data.phmin      ;
	data.value.cond        = this.data.cond       ;
	data.value.condmax     = this.data.condmax    ;
	data.value.condmin     = this.data.condmin    ;
	data.value.turb        = this.data.turb       ;
	data.value.turbmax     = this.data.turbmax    ;
//	data.value.status      = this.data.status     ;
//	data.value.suggested   = this.data.suggested  ;
//	data.value.override    = this.data.override   ;
//	data.value.overridden  = this.data.overridden ;
	delete data.value.save;
	data.value.save = this.cloneGrabData(data.value);
	delete data.value.save.modified;
	return true;
}

EventView.prototype.cloneGrabData = function(data) {
	var save = clone(data);
	delete save.status;
	delete save.overridden;
	delete save.override;
	delete save.suggested;
	return save;
}

EventView.prototype.onBlurAll = function() {
	var data = this.local_save_data;
	if (data == null) data = {};
	this.textDay        .onBlurSetDefault();
	this.textChlorine   .onBlurSetDefault(data.chlorine);
	this.textChlorineMax.onBlurSetDefault(data.chlorinemax);
	this.textChlorineMin.onBlurSetDefault(data.chlorinemin);
	this.textpH         .onBlurSetDefault(data.ph);
	this.textpHMax      .onBlurSetDefault(data.phmax);
	this.textpHMin      .onBlurSetDefault(data.phmin);
	this.textCond       .onBlurSetDefault(data.cond);
	this.textCondMax    .onBlurSetDefault(data.condmax);
	this.textCondMin    .onBlurSetDefault(data.condmin);
	this.textTurb       .onBlurSetDefault(data.turb);
	this.textTurbMax    .onBlurSetDefault(data.turbmax);
	this.cacheValues();
	this.setButtonText();
	this.local_save_data = (this.data.save == null) ? {} : this.data.save;
}

EventView.prototype.checkModified = function(data) {
	if (data == null) data = this.data;
	if (data.save == null) return false;
	delete data.save.modified;
	var bSameSum = true;
	for (var key in this.data) {
		if (key == "save") continue;
		if (key == "time_input_method") continue;
		//var bSkip = !data.save[key];
		var bSkip = (data.save[key] == null);
		if (key == "time") bSkip = false;
		if (bSkip) continue;
		var valOld = data.save[key]
		var valNew = data[key];
		var bSame = (valOld == valNew);
		var _old = data.save[key]
		var _new = data[key];
		var tOld = typeof _old;
		var tNew = typeof _new;
		var bSame = (_old == _new && tOld == tNew);
		if (!bSame) {
			console.log("< *** MODIFIED *** > \"" + key + "\" = (" + valOld + ", " + valNew + ")");
		}
		//if (!bSame) return true;
		bSameSum = bSameSum && bSame;
	}
	return !bSameSum;
	return false;
}

//
// use this to transfer the text in the input fields to the cache storage
//
EventView.prototype.cacheValues = function() {
	//
	// Day-Time
	//
	if (this.getTraining()) {
		var day = this.textDay.getFloat();
		var time = this.textTime.getSeconds();
		if (isNotNull(day) && isNotNull(time)) {
			this.data.time = 24*60*60 * (day - 1) + time;
			this.data.time_input_method = "both"
		} else if (isNotNull(day)) {
			this.data.time = 24*60*60 * (day - 1);
			this.data.time_input_method = "day"
		} else if (isNotNull(time)) {
			this.data.time = time;
			this.data.time_input_method = "time"
		} else {
			this.data.time = "";
			delete this.data.time_input_method;
		}
	} else {
		var inpStartDay  = this.listDay.getSelectedUserDataInt("inpstartday" );
		var inpDay       = this.listDay.getSelectedUserDataInt("inpday"      );
		var day          = this.listDay.getSelectedUserDataInt("day"         );
		var date         = this.listDay.getSelectedUserDataInt("date"        );
		var month        = this.listDay.getSelectedUserDataInt("month"       );
		var year         = this.listDay.getSelectedUserDataInt("year"        );
		var currentTime  = this.listDay.getSelectedUserDataInt("currenttime" );
		var epoch        = this.listDay.getSelectedUserDataInt("epoch"       );
		var time = this.textTime.getSeconds();
		if (isNotNull(day) && isNotNull(time)) {
			this.data.time = epoch + time;
			this.data.time_input_method = "both"
		} else if (isNotNull(day)) {
			this.data.time = epoch;
			this.data.time_input_method = "day"
		} else if (isNotNull(time)) {
			this.data.time = time;
			this.data.time_input_method = "time"
		} else {
			this.data.time = "";
			delete this.data.time_input_method;
		}
	}
	//
	this.data.uuid        = this.textUuid       .getValue("");
	this.data.team        = this.textTeam       .getValue("");
	this.data.location    = this.textLocation   .getValue("");
	this.data.chlorine    = this.textChlorine   .getFloat("");
	this.data.chlorinemax = this.textChlorineMax.getFloat("");
	this.data.chlorinemin = this.textChlorineMin.getFloat("");
	this.data.ph          = this.textpH         .getFloat("");
	this.data.phmax       = this.textpHMax      .getFloat("");
	this.data.phmin       = this.textpHMin      .getFloat("");
	this.data.cond        = this.textCond       .getFloat("");
	this.data.condmax     = this.textCondMax    .getFloat("");
	this.data.condmin     = this.textCondMin    .getFloat("");
	this.data.turb        = this.textTurb       .getFloat("");
	this.data.turbmax     = this.textTurbMax    .getFloat("");
	//
	this.data.modified    = this.checkModified();
	//
	this.setButtonText();
	//
	return this.data.modified;
}

EventView.prototype.setButtonText = function() {
	var bTraining = this.getTraining();
	if (bTraining) {
		if (this.data.modified) {
			this.setButton(this.SuggestedButton, this.suggestedText, null, true, true);
			this.setButton(this.OverrideButton , this.overrideText , null, true, false);
		} else {
			var suggestion = (this.data.suggested == "Abnormal");
			var blank      = (this.data.suggested == null || this.data.suggested == "");
			this.data.suggested = this.setButton(this.SuggestedButton, this.suggestedText, suggestion, blank, true);
			this.data.override  = this.setButton(this.OverrideButton , this.overrideText , suggestion, blank, false);
		}
	} else {
		var suggestion = this.checkSuggestion();
		this.data.suggested = this.setButton(this.SuggestedButton, this.suggestedText, suggestion, blank, true);
		this.data.override  = this.setButton(this.OverrideButton , this.overrideText , suggestion, blank, false);
	}
}

EventView.prototype.setButton = function(button, pretext, suggestion, blank, suggested) {
	if (blank) {
		var text = suggested ? "Set Suggested" : "Override Suggested";
		var color = EventView.COLOR_TEXT_BACKGROUND;
		button.setText(text);
		button.setTextColor(color);
		return "";
	}
	if (suggested) {
		var text = suggestion ? "Abnormal" : "Background";
		var color = suggestion ? EventView.COLOR_TEXT_ABNORMAL : EventView.COLOR_TEXT_BACKGROUND;
	} else {
		var text = suggestion ? "Background" : "Abnormal";
		var color = suggestion ? EventView.COLOR_TEXT_BACKGROUND : EventView.COLOR_TEXT_ABNORMAL;
	}
	button.setText(pretext + text);
	button.setTextColor(color);
	return text;
}

EventView.prototype.checkSuggestion = function() {
	var suggestion1 = this.checkChlorine1();
	var suggestion2 = this.checkChlorine();
	var suggestion3 = this.checkPh();
	var suggestion4 = this.checkCond();
	var suggestion5 = this.checkTurb();
	if (suggestion1) return true; // "Abnormal"
	if (suggestion2 && suggestion3) return true;
	if (suggestion2 && suggestion4) return true;
	if (suggestion2 && suggestion5) return true;
	if (suggestion3 && suggestion4) return true;
	if (suggestion3 && suggestion5) return true;
	if (suggestion4 && suggestion5) return true;
	return false; // "Background"
}

EventView.prototype.checkChlorine1 = function() {
	var suggestion = this.checkSuggestionOne(this.data.chlorine, 0.1, null, this.textChlorine, this.textChlorineMin, this.textChlorineMax);
	return suggestion;
}

EventView.prototype.checkChlorine = function() {
	var suggestion = this.checkSuggestionOne(this.data.chlorine, this.data.chlorinemin, this.data.chlorinemax, this.textChlorine, this.textChlorineMin, this.textChlorineMax);
	return suggestion;
}

EventView.prototype.checkPh = function() {
	var suggestion = this.checkSuggestionOne(this.data.ph, this.data.phmin, this.data.phmax, this.textpH, this.textpHMin, this.textpHMax);
	return suggestion;
}

EventView.prototype.checkCond = function() {
	var suggestion = this.checkSuggestionOne(this.data.cond, this.data.condmin, this.data.condmax, this.textCond, this.textCondMin, this.textCondMax);
	return suggestion;
}

EventView.prototype.checkTurb = function() {
	var suggestion = this.checkSuggestionOne(this.data.turb, null, this.data.turbmax, this.textTurb, null, this.textTurbMax);
	return suggestion;
}

EventView.prototype.checkSuggestionOne = function(val, min, max, text, textmin, textmax) {
	//var value = (val != "" && val != null && min != "" && min != null && val < min);
	var t = typeof(val);
	var bBlank1 = (t == "string" && val == "");
	var t = typeof(min);
	var bBlank2 = (t == "string" && min == "");
	var t = typeof(max);
	var bBlank3 = (t == "string" && min == "");
	var value1 = (!bBlank1 && !bBlank2 && val < min);
	var suggestion1 = this.checkTextboxColor(textmin, value1);
	var value = (val != "" && val != null && max != "" && max != null && val > max);
	var suggestion2 = this.checkTextboxColor(textmax, value);
	return this.checkTextboxColor(text, suggestion1 || suggestion2);
}

EventView.prototype.checkTextboxColor = function(textbox, value) {
	if (textbox == null) return false;
	if (value)
		textbox.setBgColor(d3.rgb(255,182,193));
	else
		textbox.setBgColor(d3.rgb(255,255,255));
	return value;
}

EventView.prototype.showValues = function(data) {
	if (data == null) {
		data = this.data;
	} else {
		this.data = data;
	}
	// Day-Time
	if (this.getTraining()) {
		var obj = GlobalData.convertSecondsToDays(data.time);
		if (data.time_input_method == "day") {
			this.textDay.setValue(obj.day, "");
			this.textTime.setSeconds("");
		} else if (data.time_input_method == "time") {
			this.textDay.setValue("");
			this.textTime.setSeconds(obj.total_seconds, "");
		} else {
			this.textDay.setValue(obj.day, "");
			this.textTime.setSeconds(obj.seconds_from_hours, "");
		}
	} else {
		if (isNull(data.time) || isBlank(data.time)) { 
			this.listDay.selectValue("0");
			this.textTime.setSeconds("");
		} else {
			var datetime = new Date(data.time * 1000);
			var year = datetime.getFullYear();
			var month = datetime.getMonth();
			var number = datetime.getDate();
			var date = new Date(year, month, number, 0, 0, 0, 0);
			if (data.time_input_method == "day") {
				this.listDay.selectUserData("epoch", "" + parseInt(date.getTime() / 1000));
				this.textTime.setSeconds("");
			} else if (data.time_input_method == "time") {
				this.listDay.selectValue("0");
				this.textTime.setSeconds(data.time, "");
			} else {
				this.listDay.selectUserData("epoch", "" + parseInt(date.getTime() / 1000));
				var hours = datetime.getHours();
				var minutes = datetime.getMinutes();
				this.textTime.setSeconds(3600 * hours + 60 * minutes, "");
			}
		}
	}
	//
	this.textUuid       .setValue(data.uuid       , "");
	this.textTeam       .setValue(data.team       , "");
	this.textLocation   .setValue(data.location   , "");
	this.textChlorine   .setValue(data.chlorine   , "");
	this.textChlorineMax.setValue(data.chlorinemax, "");
	this.textChlorineMin.setValue(data.chlorinemin, "");
	this.textpH         .setValue(data.ph         , "");
	this.textpHMax      .setValue(data.phmax      , "");
	this.textpHMin      .setValue(data.phmin      , "");
	this.textCond       .setValue(data.cond       , "");
	this.textCondMax    .setValue(data.condmax    , "");
	this.textCondMin    .setValue(data.condmin    , "");
	this.textTurb       .setValue(data.turb       , "");
	this.textTurbMax    .setValue(data.turbmax    , "");
}

EventView.prototype.clearValues = function() {
	this.listDay.updateList([]);
	this.showValues({});
}

///////////////////////////////////////////////////////

EventView.prototype.resizeWindow = function(w, h) {
	this.d3svgbg.style("width" , convertToPx(w));
	this.d3svgbg.style("height", convertToPx(h));
	this.d3rectbg.attr("width" , w);
	this.d3rectbg.attr("height", h);
	this.d3svg.style("width",  convertToPx(w - 2 * NETWORK_VIEW_BORDER));
	this.d3svg.style("height", convertToPx(h - 2 * NETWORK_VIEW_BORDER));
	this.d3rect.attr("width",  w - 2 * NETWORK_VIEW_BORDER);
	this.d3rect.attr("height", h - 2 * NETWORK_VIEW_BORDER);
	//
	var y = this.CanaryGrid.y;
	var w2 = this.CanaryGrid.w;
	var unknown = 20;
	var h2 = h - y - NETWORK_VIEW_BORDER - unknown - 80;
	this.CanaryGrid.resize(w2, h2);
	//
	var y = this.GrabGrid.y;
	var w3 = this.GrabGrid.w;
	var unknown = 20;
	var h3 = h - y - NETWORK_VIEW_BORDER - unknown - 130;
	this.GrabGrid.resize(w3, h3);
	//
	this.CompleteButton .setTop(h - NETWORK_VIEW_BORDER - this.CompleteButton.getHeight()  - 10);
	this.NewButton      .setTop(h - NETWORK_VIEW_BORDER - this.NewButton.getHeight()       - 90);
	this.DeleteButton   .setTop(h - NETWORK_VIEW_BORDER - this.DeleteButton.getHeight()    - 90);
	this.ExportButton   .setTop(h - NETWORK_VIEW_BORDER - this.ExportButton.getHeight()    - 90);
	this.InversionButton.setTop(h - NETWORK_VIEW_BORDER - this.InversionButton.getHeight() - 10);
}

///////////////////////////////////////////////////////

EventView.prototype.show = function(view, startTime) {
	if (this.getTraining()) {
		this.textDay.show();
		this.listDay.hide();
	} else {
		this.textDay.hide();
		this.listDay.show();
	}
	ResizeWindow();
	if (view == "grab") m_EventGrabView.show();
	this.d3g.style("visibility", "inherit");
}

EventView.prototype.hide = function() {
	this.CanaryGrid.updateDataGrid([]);
	this.GrabGrid.updateDataGrid([]);
	this.d3g.style("visibility", "hidden");
}

EventView.prototype.isVisible = function() {
	var sVisible = this.d3g.style("visibility");
	return !(sVisible == "hidden");
}

EventView.prototype.isHidden = function() {
	return !this.isVisible();
}
///////////////////////////////////////////////////////

EventView.prototype.onNew = function() {
	var index = this.GrabGrid.selectedIndex;
	var uuids = this.getUuids();
	Couch.getDoc(this, uuids.grab, function(data) {
		if (index > -1) this.setData(data.grab[index]);
		Couch.setDoc(this, uuids.grab, data, function(e,res) {
			this.CanaryGrid.deselectAllRows();
			var url = Couch.events + "?call=newgrab&uuid=" + uuids.uuid;
			Couch.getDoc(this, url, function(data){
				var data = (data) ? data : null;
			})
		});
	});
}

EventView.prototype.onDelete = function() {
	var index = this.GrabGrid.selectedIndex;
	if (index == -1) return;
	var uuids = this.getUuids();
	Couch.getDoc(this, uuids.grab, function(data) {
		data.grab.splice(index, 1);
		this.logData();
		Couch.setDoc(this, uuids.grab, data);
	});
}

EventView.prototype.onExport = function() {
	m_Waiting.show()
	var uuids = this.getUuids();
	var index = this.GrabGrid.selectedIndex;
	// Get the training/events document
	Couch.getDoc(this, uuids.uuid, function(data) {
		data.statusOld = data.status;
		data.status = "Exporting";
		// save the new status to the training/events document
		Couch.setDoc(this, uuids.uuid, data, function(res,e) {
			// get the grab document
			Couch.getDoc(this, uuids.grab, function(data) {
				var bModified = false;
				if (index > -1 && index < data.grab.length) {
					bModified = this.setData(data.grab[index]);
				}
				if (bModified) {
					Couch.setDoc(this, uuids.grab, data, function(res,e) {
						this.callServerGis(uuids);
					});
				} else {
					this.callServerGis(uuids);
				}
			});
		});
	});
}

EventView.prototype.callServerGis = function(uuids) {
	Couch.getDoc(this, Couch.gis + "?uuid=" + uuids.uuid, function(data) { // TODO - uuids.uuid or uuids.grab?
		if (data.error) {
			alert(data.reason);
			m_Waiting.hide()
			return;
		}
		// uuid is the id of the export.kmz file
		var url = GlobalData.CouchDb + data.uuid + "/" + data.fileName;
		window.open(url, "_self");
		m_Waiting.hide()
	});
}

EventView.prototype.onInversion = function() {
	m_Waiting.show()
	var uuids = this.getUuids();
	Couch.getDoc(this, uuids.uuid, function(data) { 
		data.statusOld = data.status;
		data.status = "Inversion";
		data.grabIndex = this.GrabGrid.selectedIndex;
		Couch.setDoc(this, uuids.uuid, data, function(e,res) {
			Couch.getDoc(this, this.getExe() + "?call=inversion&uuid=" + uuids.uuid, function(data) {
				var data = data ? data : null;
			});
		});
	});
}

EventView.prototype.onSuggested = function() {
	this.data.overridden = false
	if (!this.getTraining()) {
		this.saveStatusKnown(this.data.suggested);
	} else if (this.data.modified || this.data.suggested == "") {
		this.saveStatusUnknown();
	} else {
		this.saveStatusKnown(this.data.suggested);
	}
}

EventView.prototype.onOverride = function() {
	this.data.overridden = true
	if (!this.getTraining()) {
		this.saveStatusKnown(this.data.override);
	} else if (this.data.modified || this.data.override == "") {
		this.saveStatusUnknown();
	} else {
		this.saveStatusKnown(this.data.override);
	}
}

EventView.prototype.saveStatusUnknown = function() {
	var uuids = this.getUuids();
	var index = this.GrabGrid.selectedIndex;
	if (index < 0) return;
	this.cacheValues();
	Couch.getDoc(this, uuids.uuid, function(data) {
		data.statusOld = data.status;
		data.status = "Sampling";
		//data.grabIndex = index;
		Couch.setDoc(this, uuids.uuid, data);
	});
	Couch.getDoc(this, uuids.grab, function(data) {
		data.grab[index].value.overridden = this.data.overridden;
		this.setData(data.grab[index]);
		data.grab[index].value.suggested  = "";
		data.grab[index].value.override   = "";
		data.grab[index].value.status     = "";
		data.index = index;
		//console.log("EventView.saveStatusUnknown(set_index)=" + index);
		this.logData();
		Couch.setDoc(this, uuids.grab, data, function(res,e) {
			Couch.getDoc(this, this.getExe() + "?call=sample&uuid=" + uuids.uuid, function(data) {
				var data = data ? data : null;
			});
		});
	});
}
EventView.prototype.saveStatusKnown = function(status) {
	var uuids = this.getUuids();
	var time_type = typeof this.data.time;
	if (time_type == "string") status = "Error: Time";
	if (this.data.location == "") status = "Error: Location";
	this.data.status = status;
	var index = this.GrabGrid.selectedIndex;

	Couch.getDoc(this, uuids.grab, function(data) {
		this.setData(data.grab[index]);
		data.grab[index].value.status = this.data.status;
		if (this.data.status == "Abnormal") {
			Couch.getDoc(this, uuids.uuid, function(data) {
				if (data.alarm == null) {
					if (time_type == "number") {
						data.alarm = {"time" : this.data.time, "sensors": [this.data.location]};
						Couch.setDoc(this, uuids.uuid, data);
					}
				}
			});
		}
		this.logData();
		data.index = index;
		Couch.setDoc(this, uuids.grab, data);
	});
}

///////////////////////////////////////////////////////

EventView.prototype.onMouseMove = function(sid) {
	this.CanaryGrid.onMouseMove(sid);
	this.GrabGrid  .onMouseMove(sid);
}
EventView.prototype.onMouseOver = function(sid) {
	this.CanaryGrid.onMouseOver(sid);
	this.GrabGrid  .onMouseOver(sid);
}
EventView.prototype.onMouseOut = function(sid) {
	this.CanaryGrid.onMouseOut(sid);
	this.GrabGrid  .onMouseOut(sid);
}
EventView.prototype.onMouseUp = function(sid) {
	this.CanaryGrid.onMouseUp(sid);
	this.GrabGrid  .onMouseUp(sid);
}
EventView.prototype.onMouseDown = function(sid) {
	this.CanaryGrid.onMouseDown(sid);
	this.GrabGrid  .onMouseDown(sid);
}
//EventView.prototype.onMouseWheel = function(sid) {}
EventView.prototype.onKeyDown = function(sid) {}
EventView.prototype.onKeyPress = function(sid) {}
EventView.prototype.onKeyUp = function(sid) {}
EventView.prototype.onInput = function(sid) {
	switch (sid) {
		case this.textTeam       .sid:
		case this.textLocation   .sid:
		case this.textDay        .sid:
		case this.textTime       .sid:
		case this.textChlorine   .sid:
		case this.textChlorineMax.sid:
		case this.textChlorineMin.sid:
		case this.textpH         .sid:
		case this.textpHMax      .sid:
		case this.textpHMin      .sid:
		case this.textCond       .sid:
		case this.textCondMin    .sid:
		case this.textCondMax    .sid:
		case this.textTurb       .sid:
		case this.textTurbMax    .sid:
			this.cacheValues();
			return;
	}
}

EventView.prototype.onChange = function(sid) {
	switch (sid) {
		case this.listDay.sid:
			var nMore = this.listDay.getSelectedUserDataInt("more");
			var item = {};
			if (nMore == 1) {
				var len = this.listDay.getLength();
				var beforeRange = this.listDay.getUserDataInt(1, "range");
				var afterRange = this.listDay.getUserDataInt(len - 1, "range");
				var epoch = this.listDay.getUserDataFromValue("0", "currentTime");
				this.listDay.updateDataWithTime(parseInt(epoch), beforeRange + 5, afterRange);
				this.listDay.selectValue("0");
			} else if (nMore == 2) {
				var len = this.listDay.getLength();
				var beforeRange = this.listDay.getUserDataInt(1, "range");
				var afterRange = this.listDay.getUserDataInt(len - 1, "range");
				var epoch = this.listDay.getUserDataFromValue("0", "currentTime");
				this.listDay.updateDataWithTime(parseInt(epoch), beforeRange, afterRange + 5);
				this.listDay.selectValue("0");
			}
			this.cacheValues();
			return;
	}
}

EventView.prototype.onClick = function(sid) {
	var uuids = this.getUuids();	
	switch (sid) {
		case this.SuggestedButton.sid:
			Events.log(null, uuids.log, "Set Suggested");
			this.onSuggested();
			return;
		case this.OverrideButton.sid:
			Events.log(null, uuids.log, "Override suggested");
			this.onOverride();
			return;
		case this.CompleteButton.sid:
			Events.log(null, uuids.log, ["Complete (data view)", ""]);
			this.GrabGrid.updateDataGrid([]);
			this.CanaryGrid.updateDataGrid([]);
			this.hide();
			return;
		case this.NewButton.sid:
			Events.log(null, uuids.log, "New grab sample");
			this.onNew();
			return;
		case this.DeleteButton.sid:
			Events.log(null, uuids.log, "Delete grab sample");
			this.onDelete();
			return;
		case this.ExportButton.sid:
			this.waiting = true;
			Events.log(null, uuids.log, "Export grab sample data");
			this.onExport();
			return;
		case this.InversionButton.sid:
			this.waiting = true;
			Events.log(null, uuids.log, "Inversion");
			this.onInversion();
	}
	var obj = document.getElementById(sid);
	var parent = obj ? obj.parentElement : null;
	var pid = parent ? parent.id : null;
	switch (pid) {
		case this.CanaryGrid.sid:
			this.CanaryGrid.onClick(sid);
			this.GrabGrid.deselectAllRows();
			return;
		case this.GrabGrid.sid:
			this.GrabGrid.onClick(sid);
			this.CanaryGrid.deselectAllRows();
			//var bDisable = this.GrabGrid.isSelected();
			//this.disableInputs(bDisable);
			return;
	}
}

EventView.prototype.onDblClick = function(sid) {}

EventView.prototype.onBlur = function(sid) {
	switch (sid) {
		case this.textTeam       .sid:
		case this.textLocation   .sid:
		case this.textDay        .sid:
		case this.textTime       .sid:
		case this.textChlorine   .sid:
		case this.textChlorineMax.sid:
		case this.textChlorineMin.sid:
		case this.textpH         .sid:
		case this.textpHMax      .sid:
		case this.textpHMin      .sid:
		case this.textCond       .sid:
		case this.textCondMin    .sid:
		case this.textCondMax    .sid:
		case this.textTurb       .sid:
		case this.textTurbMax    .sid:
			this.onBlurAll();
			return;
	}
}

///////////////////////////////////////////////////////

EventView.prototype.addListeners = function() {
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
	//
	//this.CanaryGrid.addListeners();
	//this.GrabGrid.addListeners();
	this.SuggestedButton.addListeners();
	this.OverrideButton .addListeners();
	this.CompleteButton .addListeners();
	this.NewButton      .addListeners();
	this.DeleteButton   .addListeners();
	this.ExportButton   .addListeners();
	this.InversionButton.addListeners();
}

EventView.prototype.addListenersForSelection = function(sSelector) {
	this.addListenersForSelectionForId(sSelector, "#" + this.gid);
}

EventView.prototype.addListenersForSelectionForId = function(sSelector, sid) {
	var m_this = this;
	var d3g = d3.select(sid);
	var sel = d3g.selectAll(sSelector);
	sel	
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
		.on("blur"      , function() { m_this.onBlur      (this.id); })
		;
}


