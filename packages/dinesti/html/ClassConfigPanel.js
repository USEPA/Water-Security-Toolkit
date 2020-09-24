// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function ConfigPanel(nPanel, dataInFrontOf, parentOnMouseOver) {
	this.uniqueString = "ConfigPanel";
	this.nPanel = nPanel;
	this.dataInFrontOf = dataInFrontOf;
	this.gid = "g" + nPanel;
	this.giid = "g" + nPanel + "i";
	this.d3g = d3.select("#" + this.gid);
	this.d3gi = this.d3g.append("g").attr("id", this.giid).style("visibility", "hidden");
	this.Tabview = new Tabview(this, "ConfigTabView", this.dataInFrontOf, 20, 10, 450, 430);
	this.Tabview.nTabWidth = [];
	this.Tabview.nTabWidth.push(55);
	this.Tabview.nTabWidth.push(60);
	this.Tabview.nTabWidth.push(55);
	this.Tabview.nTabWidth.push(45);
	this.Tabview.nTabWidth.push(70);
	this.Tabview.nTabWidth.push(60);
	this.Tabview.sTabTitles = [];
	this.Tabview.sTabTitles.push("Events");
	this.Tabview.sTabTitles.push("Training");
	this.Tabview.sTabTitles.push("Clock");
	this.Tabview.sTabTitles.push("Log");
	this.Tabview.sTabTitles.push("Database");
	this.Tabview.sTabTitles.push("Server");
	this.Tabview.create();
	this.base = Control;
}

ConfigPanel.prototype = new Control;

ConfigPanel.prototype.updateData = function(bFirstTime, data) {
	var m_this = this;
	if (bFirstTime) {
		this.createInputs();
		Couch.getDoc(this, "gui", function(data) {
			var gui_data = data;
			Couch.getConfig(this, "", function(data) {
				gui_data.external = data.external;
				gui_data.httpd    = data.httpd   ; // bind address, port
				gui_data.couchdb  = data.couchdb ; // os process timeout
				this.storeData(gui_data);
			});
		});
	} else {
		var gui_data = data;
		Couch.getConfig(this, "", function(data) {
			gui_data.external = data.external;
			this.storeData(gui_data);
		});
	}
//
//	this.addListeners();
//	this.Tabview.addListeners();
//
//	this.Tabview.updateData(bFirstTime, function() {
//		m_this.addListeners();
//	});
}

ConfigPanel.staticStoreData = function(data) {
	ConfigPanel.staticStoreDataEvents  (data);
	ConfigPanel.staticStoreDataTraining(data);
	ConfigPanel.staticStoreDataClock   (data);
	ConfigPanel.staticStoreDataServer  (data);
}
ConfigPanel.prototype.storeData = function(data) {
	this.storeDataEvents  (data);
	this.storeDataTraining(data);
	this.storeDataClock   (data);
	this.storeDataDatabase(data);
	this.storeDataServer  (data);
}

ConfigPanel.staticStoreDataEvents = function(data) {
	GlobalData.config_erd_dynamic_type   = data.config_erd_dynamic_type;
	GlobalData.config_event_sample_delay = data.config_event_sample_delay;
	GlobalData.config_event_sample_count = data.config_event_sample_count;
}
ConfigPanel.prototype.storeDataEvents = function(data) {
	ConfigPanel.staticStoreDataEvents(data);
	this.showValuesEvents(data);
	var uuid = data.config_event_inp_uuid
	Couch.getDoc(this, uuid, function(data) {
		GlobalData.config_event_inp_data = data;
	});
}

ConfigPanel.staticStoreDataTraining = function(data) {
	GlobalData.config_erd_dynamic_type      = data.config_erd_dynamic_type;
	GlobalData.config_training_sample_delay = data.config_training_sample_delay;
	GlobalData.config_training_sample_count = data.config_training_sample_count;
}
ConfigPanel.prototype.storeDataTraining = function(data) {
	ConfigPanel.staticStoreDataTraining(data);
	this.showValuesTraining(data);
}

ConfigPanel.staticStoreDataClock = function(data) {
	GlobalData.config_clock_repeat       = data.config_clock_repeat;
	GlobalData.config_clock_interval     = data.config_clock_interval;
	GlobalData.config_clock_show_seconds = data.config_clock_show_seconds;
	GlobalData.config_clock_show_ampm    = data.config_clock_show_ampm;
	GlobalData.config_clock_testing      = data.config_clock_testing;
}
ConfigPanel.prototype.storeDataClock = function(data) {
	ConfigPanel.staticStoreDataClock(data);
	this.showValuesClock(data);
}

ConfigPanel.staticStoreDataDatabase = function(data) {
}
ConfigPanel.prototype.storeDataDatabase = function(data) {
	this.showValuesDatabase(data);
}

ConfigPanel.staticStoreDataServer = function(data) {
}
ConfigPanel.prototype.storeDataServer = function(data) {
	ConfigPanel.staticStoreDataServer(data);
	this.showValuesServer(data);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////

ConfigPanel.prototype.createInputs = function() {
	var difo = this.dataInFrontOf;
	var uniq = this.uniqueString;
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	if (true) {
		var itab = 0;
		var svgid = this.Tabview.getTabId1(itab);
		var gid   = this.Tabview.getTabId2(itab);
		var x =  10;
		var y =  23;
		var w = 125;
		var h =  20;
		var i =   0;
		this.label11 = new Label("Network"                 , svgid, "labelEventNetwork"   + uniq, difo, x, y+h*i++, w, h);
		this.label12 = new Label("Network start day"       , svgid, "labelEventStartDay"  + uniq, difo, x, y+h*i++, w, h);
		this.label13 = new Label("Network start time (h:m)", svgid, "labelEventStartTime" + uniq, difo, x, y+h*i++, w, h);
		this.label14 = new Label("Network repeat (h:m)"    , svgid, "labelEventRepeat"    + uniq, difo, x, y+h*i++, w, h);
		this.label15 = new Label("Horizon (h:m)"           , svgid, "labelEventHorizon"   + uniq, difo, x, y+h*i++, w, h);
		this.label16 = new Label("Sample count"            , svgid, "labelEventCount"     + uniq, difo, x, y+h*i++, w, h);
		this.label17 = new Label("Sample delay (h:m)"      , svgid, "labelEventDelay"     + uniq, difo, x, y+h*i++, w, h);
		this.label18 = new Label("Sample limits"           , svgid, "labelEventLimits"    + uniq, difo, x+0, 5+y+h*i++, w, h);
		this.label1A = new Label("Chlorine residual max (mg/l)", svgid, "labelEventClMax"     + uniq, difo, x+9, 5+y+h*i++, w, h);
		this.label19 = new Label("Chlorine residual min (mg/l)", svgid, "labelEventClMin"     + uniq, difo, x+9, 5+y+h*i++, w, h);
		this.label1C = new Label("pH max"                      , svgid, "labelEventpHMax"     + uniq, difo, x+9, 5+y+h*i++, w, h);
		this.label1B = new Label("pH min"                      , svgid, "labelEventpHMin"     + uniq, difo, x+9, 5+y+h*i++, w, h);
		this.label1E = new Label("Conductivity max (uS/cm)"    , svgid, "labelEventCondMax"   + uniq, difo, x+9, 5+y+h*i++, w, h);
		this.label1D = new Label("Conductivity min (uS/cm)"    , svgid, "labelEventCondMin"   + uniq, difo, x+9, 5+y+h*i++, w, h);
		this.label1G = new Label("Turbidity max (NTU)"         , svgid, "labelEventTurbMax"   + uniq, difo, x+9, 5+y+h*i++, w, h);
		//
		var x = 165;//145;
		var y =  20;
		var w = 100;
		var h =  20;
		var i =   0;
		this.listNetwork        = new NetworkList(gid, "listNetwork"               + uniq, difo, x, y+h*i++, w + 7.5); // 7.5 is to compensate for the fact that
		this.listEventStartDay  = new DateList   (gid, "listEventNetworkStartDay"  + uniq, difo, x, y+h*i++, w + 7.5); // textboxes are wider than specified
		this.textEventStartTime = new Timebox    (gid, "textEventNetworkStartTime" + uniq, difo, x, y+h*i++, w);
		this.textEventRepeat    = new Timebox    (gid, "textEventNetworkRepeat"    + uniq, difo, x, y+h*i++, w);
		this.textEventHorizon   = new Timebox    (gid, "textEventNetworkHorizon"   + uniq, difo, x, y+h*i++, w);
		this.textEventCount     = new Textbox    (gid, "textEventSampleCount"      + uniq, difo, x, y+h*i++, w);
		this.textEventDelay     = new Timebox    (gid, "textEventSampleDelay"      + uniq, difo, x, y+h*i++, w);
		this.textEventClMax     = new Timebox    (gid, "textEventClMax"            + uniq, difo, x, 25+y+h*i++, w);
		this.textEventClMin     = new Timebox    (gid, "textEventClMin"            + uniq, difo, x, 25+y+h*i++, w);
		this.textEventPhMax     = new Timebox    (gid, "textEventPhMax"            + uniq, difo, x, 25+y+h*i++, w);
		this.textEventPhMin     = new Timebox    (gid, "textEventPhMin"            + uniq, difo, x, 25+y+h*i++, w);
		this.textEventCondMax   = new Timebox    (gid, "textEventCondMax"          + uniq, difo, x, 25+y+h*i++, w);
		this.textEventCondMin   = new Timebox    (gid, "textEventCondMin"          + uniq, difo, x, 25+y+h*i++, w);
		this.textEventTurbMax   = new Timebox    (gid, "textEventTurbMax"          + uniq, difo, x, 25+y+h*i++, w);
		//
		this.SaveEventsButton   = new Button("Save Events", gid, "buttonSaveEvents" + uniq, difo, 320, 333, 95, 20);
	}
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	if (true) {
		var itab = 1;
		var svgid = this.Tabview.getTabId1(itab);
		var gid   = this.Tabview.getTabId2(itab);
		var x =  10;
		var y =  23;
		var w = 125;
		var h =  20;
		var i =   0;
		this.label21 = new Label("Horizon (h:m)"        , svgid, "labelTrainingHorizon"     + uniq, difo, x, y+h*i++, w, h);
		this.label22 = new Label("Sample count"         , svgid, "labelTrainingSampleCount" + uniq, difo, x, y+h*i++, w, h);
		this.label23 = new Label("Sample delay (h:m)"   , svgid, "labelTrainingSampleDelay" + uniq, difo, x, y+h*i++, w, h);
		this.label24 = new Label("Measurements per hour", svgid, "labelTrainingMeasures"    + uniq, difo, x, y+h*i++, w, h);
		//
		var x = 145;
		var y =  20;
		var w = 100;
		var h =  20;
		var i =   0;
		this.textTrainingHorizon  = new Timebox(gid, "textTrainingHorizon"  + uniq, difo, x, y+h*i++, w);
		this.textTrainingCount    = new Textbox(gid, "textTrainingCount"    + uniq, difo, x, y+h*i++, w);
		this.textTrainingDelay    = new Timebox(gid, "textTrainingDelay"    + uniq, difo, x, y+h*i++, w);
		this.textTrainingMeasures = new Textbox(gid, "textTrainingMeasures" + uniq, difo, x, y+h*i++, w);
		//
		this.SaveTrainingButton   = new Button("Save Training", gid, "buttonSaveTraining" + uniq, difo, 320, 333, 95, 20);
	}
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	if (true) {
		var itab = 2;
		var svgid = this.Tabview.getTabId1(itab);
		var gid   = this.Tabview.getTabId2(itab);
		var x =  10;
		var y =  23;
		var w =  90;
		var h =  20;
		var i =   0;
		this.label31 = new Label("Update repeatedly"   , svgid, "labelClockUpdateRepeat"    + uniq, difo, x, y+h*i++, w, h);
		this.label32 = new Label("Update interval (ms)", svgid, "labelClockUpdateInternval" + uniq, difo, x, y+h*i++, w, h);
		this.label33 = new Label("Show seconds"        , svgid, "labelClockShowSeconds"     + uniq, difo, x, y+h*i++, w, h);
		this.label34 = new Label("Show AM/PM"          , svgid, "labelClockShowAmPm"        + uniq, difo, x, y+h*i++, w, h);
		this.label35 = new Label("Testing"             , svgid, "labelClockTesting"         + uniq, difo, x, y+h*i++, w, h);
		this.label36 = new Label("Test date"           , svgid, "labelClockTestDate"        + uniq, difo, x, y+h*i++, w, h);
		this.label37 = new Label("Test time"           , svgid, "labelClockTestTime"        + uniq, difo, x, y+h*i++, w, h);
		//
		var x = 145;
		var y =  20;
		var w =  90;
		var h =  20;
		var i =   0;
		this.checkUpdateRepeat  = new Checkbox(gid, "checkClockUpdateRepeat"  + uniq, difo, x, y+h*i++ + 1);
		this.textUpdateInterval = new Textbox (gid, "textClockUpdateInterval" + uniq, difo, x, y+h*i++,  w);
		this.checkShowSeconds   = new Checkbox(gid, "checkClockShowSeconds"   + uniq, difo, x, y+h*i++ + 1);
		this.checkShowAmPm      = new Checkbox(gid, "checkClockShowAmPm"      + uniq, difo, x, y+h*i++ + 1);
		this.checkClockTesting  = new Checkbox(gid, "checkClockTesting"       + uniq, difo, x, y+h*i++ + 1);
		this.listTestDate       = new DateList(gid, "listClockTestDate"       + uniq, difo, x, y+h*i++,  w + 7.5); // textboxes are wider than specified
		this.textTestTime       = new Timebox (gid, "textClockTestTime"       + uniq, difo, x, y+h*i++,  w);
		//
		this.SaveClockButton = new Button("Save Clock", gid, "buttonSaveClock" + uniq, difo, 320, 333, 95, 20);
	}
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	if (true) {
		var itab = 3;
		var svgid = this.Tabview.getTabId1(itab);
		var gid   = this.Tabview.getTabId2(itab);
		var x =  10;
		var y =  23;
		var w = 131;
		var h =  20;
		this.label41 = new Label("Character count to retrieve", svgid, "labelLogCount" + uniq, difo, x, y, w, h);	
		//
		this.textLogCharacterCount = new Textbox(gid, "textLogCharacterCount" + uniq, difo, x + w + 20, y - 3, 60);
		this.textLogCharacterCount.setPlaceHolderText("50000");
		this.textLogCharacterCount.setMin(1);
		this.textLogCharacterCount.setMax(1000000);
		this.LogRetrieveButton = new Button("Retrieve", gid, "buttonLogRetrieve" + uniq, difo, x + w + 20 + 60 + 15, y - 3, 60, 20);
		this.textLog = new Textarea(gid, "textLog" + uniq, difo, x, 60, 400, 290);
		this.textLog.setWrap(false);
		this.textLog.setScroll(true);
	}
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	if (true) {
		var itab = 4;
		var svgid = this.Tabview.getTabId1(itab);
		var gid   = this.Tabview.getTabId2(itab);
		var x =  20;
		var y =  20;
		var w = 170;
		var h =  20;
		var i =   0;
		//
		this.DatabaseOrphansButton   = new Button("Delete any orphaned docs"   , gid, "buttonDbDelOrphans"    + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseUnknownButton   = new Button("Delete any unknown docs"    , gid, "buttonDbDelUnknown"    + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseGisButton       = new Button("Delete old GIS docs"        , gid, "buttonDbDelGis"        + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseSvgButton       = new Button("Delete old SVG/PNG docs"    , gid, "buttonDbDelSvg"        + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseInpButton       = new Button("Delete all inp docs"        , gid, "buttonDbDelInp"        + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseSimButton       = new Button("Delete all simulation docs" , gid, "buttonDbDelSim"        + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseMesGenButton    = new Button("Delete all measuregen docs" , gid, "buttonDbDelMesGen"     + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseImpactButton    = new Button("Delete all impact docs"     , gid, "buttonDbDelImpact"     + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseInversionButton = new Button("Delete all inversion docs"  , gid, "buttonDbDelInversion"  + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseGrabButton      = new Button("Delete all grab sample docs", gid, "buttonDbDelGrab"       + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseTrainingButton  = new Button("Delete all training docs"   , gid, "buttonDbDelTraining"   + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseEventsButton    = new Button("Delete all event docs"      , gid, "buttonDbDelEvents"     + uniq, difo, x, y+h*i++, w, 20);
		this.DatabaseAllButton       = new Button("Delete all docs"            , gid, "buttonDbDelAll"        + uniq, difo, x, y+h*i++, w, 20);
		//
		var x = 200;
		var y =  23;
		var w =  15;
		var i =   0;
		this.LockDbOrphansButton     = new LockButton(svgid, "buttonLockDbOrphans"   + uniq, difo, x, y+h*i++, w);
		this.LockDbUnknownButton     = new LockButton(svgid, "buttonLockDbUnknown"   + uniq, difo, x, y+h*i++, w);
		this.LockDbGisButton         = new LockButton(svgid, "buttonLockDbGis"       + uniq, difo, x, y+h*i++, w);
		this.LockDbSvgButton         = new LockButton(svgid, "buttonLockDbSvg"       + uniq, difo, x, y+h*i++, w);
		this.LockDbInpButton         = new LockButton(svgid, "buttonLockDbInp"       + uniq, difo, x, y+h*i++, w);
		this.LockDbSimButton         = new LockButton(svgid, "buttonLockDbSim"       + uniq, difo, x, y+h*i++, w);
		this.LockDbMesGenButton      = new LockButton(svgid, "buttonLockDbMesGen"    + uniq, difo, x, y+h*i++, w);
		this.LockDbImpactButton      = new LockButton(svgid, "buttonLockDbImpact"    + uniq, difo, x, y+h*i++, w);
		this.LockDbInversionButton   = new LockButton(svgid, "buttonLockDbInversion" + uniq, difo, x, y+h*i++, w);
		this.LockDbGrabButton        = new LockButton(svgid, "buttonLockDbGrab"      + uniq, difo, x, y+h*i++, w);
		this.LockDbTrainingButton    = new LockButton(svgid, "buttonLockDbTraining"  + uniq, difo, x, y+h*i++, w);
		this.LockDbEventsButton      = new LockButton(svgid, "buttonLockDbEvents"    + uniq, difo, x, y+h*i++, w);
		this.LockDbAllButton         = new LockButton(svgid, "buttonLockDbAll"       + uniq, difo, x, y+h*i++, w);
		//
		this.LockDbOrphansButton  .registerListener(null, this, this.handleLockButtons);
		this.LockDbUnknownButton  .registerListener(null, this, this.handleLockButtons);
		this.LockDbGisButton      .registerListener(null, this, this.handleLockButtons);
		this.LockDbSvgButton      .registerListener(null, this, this.handleLockButtons);
		this.LockDbInpButton      .registerListener(null, this, this.handleLockButtons);
		this.LockDbSimButton      .registerListener(null, this, this.handleLockButtons);
		this.LockDbMesGenButton   .registerListener(null, this, this.handleLockButtons);
		this.LockDbImpactButton   .registerListener(null, this, this.handleLockButtons);
		this.LockDbInversionButton.registerListener(null, this, this.handleLockButtons);
		this.LockDbGrabButton     .registerListener(null, this, this.handleLockButtons);
		this.LockDbTrainingButton .registerListener(null, this, this.handleLockButtons);
		this.LockDbEventsButton   .registerListener(null, this, this.handleLockButtons);
		this.LockDbAllButton      .registerListener(null, this, this.handleLockButtons);
		//
		var x = 225;
		var y =  21;
		var w =  30;
		var i =   0;
		this.labelDbOrphans   = new Label("", svgid, "labelDbOrphans"   + uniq, difo, x, y+h*i++, w, h);
		this.labelDbUnknown   = new Label("", svgid, "labelDbUnknown"   + uniq, difo, x, y+h*i++, w, h);
		this.labelDbGis       = new Label("", svgid, "labelDbGis"       + uniq, difo, x, y+h*i++, w, h);
		this.labelDbSvg       = new Label("", svgid, "labelDbSvg"       + uniq, difo, x, y+h*i++, w, h);
		this.labelDbInp       = new Label("", svgid, "labelDbInp"       + uniq, difo, x, y+h*i++, w, h);
		this.labelDbSim       = new Label("", svgid, "labelDbSim"       + uniq, difo, x, y+h*i++, w, h);
		this.labelDbMesGen    = new Label("", svgid, "labelDbMesGen"    + uniq, difo, x, y+h*i++, w, h);
		this.labelDbImpact    = new Label("", svgid, "labelDbImpact"    + uniq, difo, x, y+h*i++, w, h);
		this.labelDbInversion = new Label("", svgid, "labelDbInversion" + uniq, difo, x, y+h*i++, w, h);
		this.labelDbGrab      = new Label("", svgid, "labelDbGrab"      + uniq, difo, x, y+h*i++, w, h);
		this.labelDbTraining  = new Label("", svgid, "labelDbTraining"  + uniq, difo, x, y+h*i++, w, h);
		this.labelDbEvents    = new Label("", svgid, "labelDbEvents"    + uniq, difo, x, y+h*i++, w, h);
		this.labelDbAll       = new Label("", svgid, "labelDbAll"       + uniq, difo, x, y+h*i++, w, h);
		//
		this.DatabaseCompactButton   = new Button("Compact database", gid, "buttonDatabaseCompact" + uniq, difo,  40, 290, 130, 20);
		this.DatabaseCanaryButton    = new Button("Clean up canary" , gid, "buttonDatabaseCanary"  + uniq, difo,  40, 315, 130, 20);
		//
		this.labelDbCompact   = new Label("", svgid, "labelDbCompact"   + uniq, difo, x - 45, 10 + y+h*i++, w, h);
		this.labelDbCanary    = new Label("", svgid, "labelDbCanary"    + uniq, difo, x - 45, 15 + y+h*i++, w, h);
		//
		this.RefreshButton           = new Button("Refresh"         , gid, "buttonRefresh"         + uniq, difo,  320, 333, 95, 20);
		//
		this.LockDatabaseButton      = new LockButton(svgid, "buttonLockDatabase" + uniq, difo, 403,   5, 15);
		//
		this.LockDatabaseButton.Lock();
		//
		this.onLockDatabase(true);
		this.LockDatabaseButton.registerListener(this.LockDatabaseButton.sid, this, this.handleLockButtons);
	}
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	if (true) {
		var itab = 5;
		var svgid = this.Tabview.getTabId1(itab);
		var gid   = this.Tabview.getTabId2(itab);
		var x =  10;
		var y =  23;
		var w =  90;
		var h =  20;
		var i =   0;
		this.label61 = new Label("WST location"      , svgid, "labelServerWstLoc"      + uniq, difo, x, y+h*i++, w, h);
		this.label62 = new Label("GIS location"      , svgid, "labelServerGisLoc"      + uniq, difo, x, y+h*i++, w, h);
		this.label63 = new Label("INP executable"    , svgid, "labelServerInpExe"      + uniq, difo, x, y+h*i++, w, h);
		this.label64 = new Label("ERD executable"    , svgid, "labelServerErdExe"      + uniq, difo, x, y+h*i++, w, h);
		this.label65 = new Label("Externals location", svgid, "labelServerExternLoc"   + uniq, difo, x, y+h*i++, w, h);
		this.label66 = new Label("Externals list"    , svgid, "labelServerExternList"  + uniq, difo, x, y+h*i++, w, h);
		this.label67 = new Label("Bind address"      , svgid, "labelServerBindAddress" + uniq, difo, x, y+h*i++, w, h);
		this.label68 = new Label("Port"              , svgid, "labelServerPort"        + uniq, difo, x, y+h*i++, w, h);
		this.label69 = new Label("Timeout (ms)"      , svgid, "labelServerTimeout"     + uniq, difo, x, y+h*i++, w, h);
		this.label6A = new Label("Save temp files"   , svgid, "labelSaveTempFiles"     + uniq, difo, x, y+h*i++, w, h);
		//
		var x = 110;
		var y =  20;
		var w =  90;
		var h =  20;
		var i =   0;
		this.textWstLoc       = new Textbox (gid, "textServerWstLoc"      + uniq, difo, x, y+h*i++, 280);
		this.textGisLoc       = new Textbox (gid, "textServerGisLoc"      + uniq, difo, x, y+h*i++, 280);
		this.textInpExe       = new Textbox (gid, "textServerInpExe"      + uniq, difo, x, y+h*i++, 280);
		this.textErdExe       = new Textbox (gid, "textServerErdExe"      + uniq, difo, x, y+h*i++, 280);
		this.textExternLoc    = new Textbox (gid, "textServerExternLoc"   + uniq, difo, x, y+h*i++, 280);
		this.textExternals    = new Textbox (gid, "textServerExternals"   + uniq, difo, x, y+h*i++, 280);
		this.textBindAddress  = new Textbox (gid, "textServerBindAddress" + uniq, difo, x, y+h*i++,   w);
		this.textPort         = new Textbox (gid, "textServerPort"        + uniq, difo, x, y+h*i++,   w);
		this.textTimeout      = new Textbox (gid, "textServerTimeout"     + uniq, difo, x, y+h*i++,   w);
		this.checkSaveTemp    = new Checkbox(gid,"checkServerSaveTemp"    + uniq, difo, x, y+h*i++ + 1 );
		//
		this.LockExternalsButton   = new LockButton(svgid, "buttonLockExternals"   + uniq, difo, 400, 125, 15);
		this.LockBindAddressButton = new LockButton(svgid, "buttonLockBindAddress" + uniq, difo, 210, 145, 15);
		this.LockPortButton        = new LockButton(svgid, "buttonLockPort"        + uniq, difo, 210, 165, 15);
		this.LockServerButton      = new LockButton(svgid, "buttonLockServer"      + uniq, difo, 403,   5, 15);
		//
		this.LockExternalsButton  .Lock();
		this.LockBindAddressButton.Lock();
		this.LockPortButton       .Lock();
		this.LockServerButton     .Lock();
		//
		this.RestartButton    = new Button("Restart server", gid, "buttonRestart"    + uniq, difo,  10, 333, 95, 20);
		this.SaveServerButton = new Button("Save Server"   , gid, "buttonSaveServer" + uniq, difo, 320, 333, 95, 20);
		//
		this.onLockServer(true);
		this.LockExternalsButton  .registerListener(this.LockServerButton.sid     , this, this.handleLockButtons);
		this.LockBindAddressButton.registerListener(this.LockBindAddressButton.sid, this, this.handleLockButtons);
		this.LockPortButton       .registerListener(this.LockPortButton.sid       , this, this.handleLockButtons);
		this.LockServerButton     .registerListener(this.LockServerButton.sid     , this, this.handleLockButtons);
	}
}

ConfigPanel.prototype.handleLockButtons = function(e) {
	var sid = e.source.sid;
	switch (sid) {
		case this.LockDbOrphansButton.sid:
			this.onLockDbOrphans(e.locked);
			break;
		case this.LockDbUnknownButton.sid:
			this.onLockDbUnknown(e.locked);
			break;
		case this.LockDbGisButton.sid:
			this.onLockDbGis(e.locked);
			break;
		case this.LockDbSvgButton.sid:
			this.onLockDbSvg(e.locked);
			break;
		case this.LockDbInpButton.sid:
			this.onLockDbInp(e.locked);
			break;
		case this.LockDbSimButton.sid:
			this.onLockDbSim(e.locked);
			break;
		case this.LockDbMesGenButton.sid:
			this.onLockDbMesGen(e.locked);
			break;
		case this.LockDbImpactButton.sid:
			this.onLockDbImpact(e.locked);
			break;
		case this.LockDbInversionButton.sid:
			this.onLockDbInversion(e.locked);
			break;
		case this.LockDbGrabButton.sid:
			this.onLockDbGrab(e.locked);
			break;
		case this.LockDbTrainingButton.sid:
			this.onLockDbTraining(e.locked);
			break;
		case this.LockDbEventsButton.sid:
			this.onLockDbEvents(e.locked);
			break;
		case this.LockDbAllButton.sid:
			this.onLockDbAll(e.locked);
			break;
		//
		case this.LockDatabaseButton.sid:
			this.onLockDatabase(e.locked);
			break;
		//
		case this.LockExternalsButton.sid:
			this.textExternals.toggleDisabled();
			break;
		case this.LockBindAddressButton.sid:
			this.textBindAddress.toggleDisabled();
			break;
		case this.LockPortButton.sid:
			this.textPort.toggleDisabled();
			break;
		case this.LockServerButton.sid:
			this.onLockServer(e.locked);
			break;
	}
}

ConfigPanel.prototype.onLockDbOrphans = function(bLocked) {
	this.DatabaseOrphansButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbUnknown = function(bLocked) {
	this.DatabaseUnknownButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbGis = function(bLocked) {
	this.DatabaseGisButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbSvg = function(bLocked) {
	this.DatabaseSvgButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbInp = function(bLocked) {
	this.DatabaseInpButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbSim = function(bLocked) {
	this.DatabaseSimButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbMesGen = function(bLocked) {
	this.DatabaseMesGenButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbImpact = function(bLocked) {
	this.DatabaseImpactButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbInversion = function(bLocked) {
	this.DatabaseInversionButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbGrab = function(bLocked) {
	this.DatabaseGrabButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbTraining = function(bLocked) {
	this.DatabaseTrainingButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbEvents = function(bLocked) {
	this.DatabaseEventsButton.disable(bLocked);
}
ConfigPanel.prototype.onLockDbAll = function(bLocked) {
	this.DatabaseAllButton.disable(bLocked);
}

ConfigPanel.prototype.onLockDatabase = function(bLocked) {
	if (bLocked) {
		this.onLockDbOrphans(bLocked);
		this.onLockDbUnknown(bLocked);
		this.onLockDbGis(bLocked);
		this.onLockDbSvg(bLocked);
		this.onLockDbInp(bLocked);
		this.onLockDbSim(bLocked);
		this.onLockDbMesGen(bLocked);
		this.onLockDbImpact(bLocked);
		this.onLockDbInversion(bLocked);
		this.onLockDbGrab(bLocked);
		this.onLockDbTraining(bLocked);
		this.onLockDbEvents(bLocked);
		this.onLockDbAll(bLocked);
	}
	//
	var bDisabled = bLocked;
	this.DatabaseCompactButton.disable(bDisabled);
	this.DatabaseCanaryButton .disable(bDisabled);
	this.RefreshButton        .disable(bDisabled);
	//
	this.LockDbOrphansButton  .disable(bDisabled);
	this.LockDbUnknownButton  .disable(bDisabled);
	this.LockDbGisButton      .disable(bDisabled);
	this.LockDbSvgButton      .disable(bDisabled);
	this.LockDbInpButton      .disable(bDisabled);
	this.LockDbSimButton      .disable(bDisabled);
	this.LockDbMesGenButton   .disable(bDisabled);
	this.LockDbImpactButton   .disable(bDisabled);
	this.LockDbInversionButton.disable(bDisabled);
	this.LockDbGrabButton     .disable(bDisabled);
	this.LockDbTrainingButton .disable(bDisabled);
	this.LockDbEventsButton   .disable(bDisabled);
	this.LockDbAllButton      .disable(bDisabled);
	//
	this.labelDbOrphans  .disable(bDisabled);
	this.labelDbUnknown  .disable(bDisabled);
	this.labelDbGis      .disable(bDisabled);
	this.labelDbSvg      .disable(bDisabled);
	this.labelDbInp      .disable(bDisabled);
	this.labelDbSim      .disable(bDisabled);
	this.labelDbMesGen   .disable(bDisabled);
	this.labelDbImpact   .disable(bDisabled);
	this.labelDbInversion.disable(bDisabled);
	this.labelDbGrab     .disable(bDisabled);
	this.labelDbTraining .disable(bDisabled);
	this.labelDbEvents   .disable(bDisabled);
	this.labelDbAll      .disable(bDisabled);
	this.labelDbCompact  .disable(bDisabled);
	this.labelDbCanary   .disable(bDisabled);
	//
	if (bLocked) {
		this.LockDbOrphansButton  .Lock();
		this.LockDbUnknownButton  .Lock();
		this.LockDbGisButton      .Lock();
		this.LockDbSvgButton      .Lock();
		this.LockDbInpButton      .Lock();
		this.LockDbSimButton      .Lock();
		this.LockDbMesGenButton   .Lock();
		this.LockDbImpactButton   .Lock();
		this.LockDbInversionButton.Lock();
		this.LockDbGrabButton     .Lock();
		this.LockDbTrainingButton .Lock();
		this.LockDbEventsButton   .Lock();
		this.LockDbAllButton      .Lock();
	}
}

ConfigPanel.prototype.onLockServer = function(bLocked) {
	var bDisabled = bLocked;
	this.textWstLoc           .disable(bDisabled);
	this.textGisLoc           .disable(bDisabled);
	this.textInpExe           .disable(bDisabled);
	this.textErdExe           .disable(bDisabled);
	this.textExternLoc        .disable(bDisabled);
	if (bDisabled) this.textExternals  .disable(bDisabled);
	if (bDisabled) this.textBindAddress.disable(bDisabled);
	if (bDisabled) this.textPort       .disable(bDisabled);
	this.textTimeout          .disable(bDisabled);
	this.checkSaveTemp        .disable(bDisabled);
	this.LockExternalsButton  .disable(bDisabled);
	this.LockBindAddressButton.disable(bDisabled);
	this.LockPortButton       .disable(bDisabled);
	this.label61              .disable(bDisabled);
	this.label62              .disable(bDisabled);
	this.label63              .disable(bDisabled);
	this.label64              .disable(bDisabled);
	this.label65              .disable(bDisabled);
	this.label66              .disable(bDisabled);
	this.label67              .disable(bDisabled);
	this.label68              .disable(bDisabled);
	this.label69              .disable(bDisabled);
	this.label6A              .disable(bDisabled);
	this.LockExternalsButton  .Lock();
	this.LockBindAddressButton.Lock();
	this.LockPortButton       .Lock();
	this.SaveServerButton     .disable(bDisabled);
	this.RestartButton        .disable(bDisabled);
}

ConfigPanel.prototype.updateNetworkList = function() {
	Couch.getDoc(this, "gui", function(data) {
		this.listNetwork.updateData(data.config_event_inp_uuid);
	});
}

ConfigPanel.prototype.updateLabels = function() {
	if (this.label11) this.label11.update();
	if (this.label12) this.label12.update();
	if (this.label13) this.label13.update();
	if (this.label14) this.label14.update();
	if (this.label15) this.label15.update();
	if (this.label16) this.label16.update();
	if (this.label17) this.label17.update();
	if (this.label18) this.label18.update();
	if (this.label19) this.label19.update();
	if (this.label1A) this.label1A.update();
	if (this.label1B) this.label1B.update();
	if (this.label1C) this.label1C.update();
	if (this.label1D) this.label1D.update();
	if (this.label1E) this.label1E.update();
	if (this.label1F) this.label1F.update();
	if (this.label1G) this.label1G.update();
	//
	if (this.label21) this.label21.update();
	if (this.label22) this.label22.update();
	if (this.label23) this.label23.update();
	if (this.label24) this.label24.update();
	if (this.label25) this.label25.update();
	if (this.label26) this.label26.update();
	if (this.label27) this.label27.update();
	if (this.label28) this.label28.update();
	if (this.label29) this.label29.update();
	if (this.label2A) this.label2A.update();
	if (this.label2B) this.label2B.update();
	if (this.label2C) this.label3C.update();
	//
	if (this.label31) this.label31.update();
	if (this.label32) this.label32.update();
	if (this.label33) this.label33.update();
	if (this.label34) this.label34.update();
	if (this.label35) this.label35.update();
	if (this.label36) this.label36.update();
	if (this.label37) this.label37.update();
	if (this.label38) this.label38.update();
	if (this.label39) this.label39.update();
	if (this.label3A) this.label3A.update();
	if (this.label3B) this.label3B.update();
	if (this.label3C) this.label3C.update();
	//
	if (this.label41) this.label41.update();
	if (this.label42) this.label42.update();
	if (this.label43) this.label43.update();
	if (this.label44) this.label44.update();
	if (this.label45) this.label45.update();
	if (this.label46) this.label46.update();
	if (this.label47) this.label47.update();
	if (this.label48) this.label48.update();
	if (this.label49) this.label49.update();
	if (this.label4A) this.label4A.update();
	if (this.label4B) this.label4B.update();
	if (this.label4C) this.label4C.update();
	//
	if (this.label51) this.label51.update();
	if (this.label52) this.label52.update();
	if (this.label53) this.label53.update();
	if (this.label54) this.label54.update();
	if (this.label55) this.label55.update();
	if (this.label56) this.label56.update();
	if (this.label57) this.label57.update();
	if (this.label58) this.label58.update();
	if (this.label59) this.label59.update();
	if (this.label5A) this.label5A.update();
	if (this.label5B) this.label5B.update();
	if (this.label5C) this.label5C.update();
	//
	if (this.label61) this.label61.update();
	if (this.label62) this.label62.update();
	if (this.label63) this.label63.update();
	if (this.label64) this.label64.update();
	if (this.label65) this.label65.update();
	if (this.label66) this.label66.update();
	if (this.label67) this.label67.update();
	if (this.label68) this.label68.update();
	if (this.label69) this.label69.update();
	if (this.label6A) this.label6A.update();
	if (this.label6B) this.label6B.update();
	if (this.label6C) this.label6C.update();
	//
	if (this.label71) this.label71.update();
	if (this.label72) this.label72.update();
	if (this.label73) this.label73.update();
	if (this.label74) this.label74.update();
	if (this.label75) this.label75.update();
	if (this.label76) this.label76.update();
	if (this.label77) this.label77.update();
	if (this.label78) this.label78.update();
	if (this.label79) this.label79.update();
	if (this.label7A) this.label7A.update();
	if (this.label7B) this.label7B.update();
	if (this.label7C) this.label7C.update();
	//
	if (this.label81) this.label81.update();
	if (this.label82) this.label82.update();
	if (this.label83) this.label83.update();
	if (this.label84) this.label84.update();
	if (this.label85) this.label85.update();
	if (this.label86) this.label86.update();
	if (this.label87) this.label87.update();
	if (this.label88) this.label88.update();
	if (this.label89) this.label89.update();
	if (this.label8A) this.label8A.update();
	if (this.label8B) this.label8B.update();
	if (this.label8C) this.label8C.update();
	//
	this.labelDbOrphans   .update();
	this.labelDbUnknown   .update();
	this.labelDbGis       .update();
	this.labelDbSvg       .update();
	this.labelDbInp       .update();
	this.labelDbSim       .update();
	this.labelDbMesGen    .update();
	this.labelDbImpact    .update();
	this.labelDbInversion .update();
	this.labelDbGrab      .update();
	this.labelDbTraining  .update();
	this.labelDbEvents    .update();
	this.labelDbAll       .update();
	this.labelDbCompact   .update();
	this.labelDbCanary    .update();
}

ConfigPanel.prototype.showValues = function(data) {
	this.showValuesEvents  (data);
	this.showValuesTraining(data);
	this.showValuesClock   (data);
	this.showValuesDatabase(data);
	this.showValuesServer  (data);
}

ConfigPanel.prototype.showValuesEvents = function(data) {
	this.listNetwork         .updateData(data.config_event_inp_uuid                   );
	this.listEventStartDay   .updateData(data.config_event_epoch_start                );
	var nSeconds = isNumeric(data.config_event_epoch_start) ? this.listEventStartDay.getSecondsForTime() : null;
	this.textEventStartTime  .setSeconds(nSeconds                                 , "");
	this.textEventRepeat     .setSeconds(data.config_event_epoch_repeat           , "");
	this.textEventHorizon    .setSeconds(data.config_event_horizon_hours   * 3600 , "");
	this.textEventCount      .setValue  (data.config_event_sample_count           , "");
	this.textEventDelay      .setSeconds(data.config_event_sample_delay           , "");
	this.textEventClMax      .setValue  (data.config_event_cl_max                 , "");
	this.textEventClMin      .setValue  (data.config_event_cl_min                 , "");
	this.textEventPhMax      .setValue  (data.config_event_ph_max                 , "");
	this.textEventPhMin      .setValue  (data.config_event_ph_min                 , "");
	this.textEventCondMax    .setValue  (data.config_event_cond_max               , "");
	this.textEventCondMin    .setValue  (data.config_event_cond_min               , "");
	this.textEventTurbMax    .setValue  (data.config_event_turb_max               , "");
}

ConfigPanel.prototype.showValuesTraining = function(data) {
	this.textTrainingHorizon .setSeconds(data.config_training_horizon_hours * 3600, "");
	this.textTrainingCount   .setValue  (data.config_training_sample_count        , "");
	this.textTrainingDelay   .setSeconds(data.config_training_sample_delay        , "");
	this.textTrainingMeasures.setValue  (data.config_training_mph                 , "");
}

ConfigPanel.prototype.showValuesClock = function(data) {
	this.checkUpdateRepeat   .setValue  (data.config_clock_repeat                     );
	this.textUpdateInterval  .setValue  (data.config_clock_interval               , "");
	this.checkShowSeconds    .setValue  (data.config_clock_show_seconds               );
	this.checkShowAmPm       .setValue  (data.config_clock_show_ampm                  );
	this.checkClockTesting   .setValue  (data.config_clock_testing                    );
	this.listTestDate        .updateData(data.config_clock_test_start                 );
	var nSeconds = isNumeric(data.config_clock_test_start) ? this.listTestDate.getSecondsForTime() : null;
	this.textTestTime        .setSeconds(nSeconds                                 , "");
	this.onClockTest();
}

ConfigPanel.prototype.showValuesDatabase = function(data) {
	this.onDeleteOrphansCount();
	this.onDeleteUnknownCount();
	this.onDeleteGisCount();
	this.onDeleteSvgCount();
	this.onDeleteInpCount();
	this.onDeleteSimCount();
	this.onDeleteMesGenCount();
	this.onDeleteImpactCount();
	this.onDeleteInversionCount();
	this.onDeleteGrabCount();
	this.onDeleteTrainingCount();
	this.onDeleteEventsCount();
	this.onDeleteAllCount();
	this.onDeleteCompactSize();
	this.onDeleteCanaryCount();
}

ConfigPanel.prototype.showValuesServer = function(data) {
	this.textWstLoc.setValue(data.config_install_dir, "");
	this.textGisLoc.setValue(data.config_gis_dir    , "");
	this.textInpExe.setValue(data.config_inp_exe    , "");
	this.textErdExe.setValue(data.config_erd_exe    , "");
	//
	var path = "";
	var sExternals = "";
	for (var external in data.external) {
		if (path.length == 0) {
			var path = data.external[external];
			var path = path.split("/_" + external + ".py");
			var path = path[0];
			var path = path.split("python ");
			var path = path[1] + "/";
		}
		if (sExternals.length > 0) sExternals += ", ";
		sExternals += external;
	}
	this.textExternLoc.setValue(path);
	this.textExternals.setValue(sExternals);
	//
	if (data.httpd) {
		this.textBindAddress.setValue(data.httpd.bind_address        );
		this.textPort       .setValue(data.httpd.port                );
	}
	if (data.couchdb) {
		this.textTimeout.setValue(data.couchdb.os_process_timeout);
	}
	this.checkSaveTemp.setValue(data.config_server_save_temp, "");
}

ConfigPanel.prototype.show = function() {
	this.updateLabels();
	this.Tabview.show();
}

ConfigPanel.prototype.hide = function() {
	this.Tabview.hide();
}

ConfigPanel.prototype.onSaveEvents = function () {
	Couch.getDoc(this, "gui", function(data) {
		var nDay  = this.listEventStartDay .getSecondsForDay();
		var nTime = this.textEventStartTime.getSeconds();
		//
		if (isNumeric(nDay) && isNumeric(nTime))
			var nStart = nDay + nTime;
		else if (isNumeric(nDay))
			var nStart = nDay;
		else if (isNumeric(nTime))
			var nStart = nTime;
		else 
			var nStart = null;
		//
		data.config_event_epoch_start      = nStart;
		data.config_event_inp_uuid         = this.listNetwork         .getValue();
		data.config_event_epoch_repeat     = this.textEventRepeat     .getSeconds();
		data.config_event_horizon_hours    = this.textEventHorizon    .getHours();
		data.config_event_sample_count     = this.textEventCount      .getInt();
		data.config_event_sample_delay     = this.textEventDelay      .getSeconds();
		data.config_event_cl_max           = this.textEventClMax      .getFloat();
		data.config_event_cl_min           = this.textEventClMin      .getFloat();
		data.config_event_ph_max           = this.textEventPhMax      .getFloat();
		data.config_event_ph_min           = this.textEventPhMin      .getFloat();
		data.config_event_cond_max         = this.textEventCondMax    .getFloat();
		data.config_event_cond_min         = this.textEventCondMin    .getFloat();
		data.config_event_turb_max         = this.textEventTurbMax    .getFloat();
		//
		Couch.setDoc(this, "gui", data);
		this.storeDataEvents(data);
	});
}

ConfigPanel.prototype.onSaveTraining = function () {
	Couch.getDoc(this, "gui", function(data) {
		data.config_training_horizon_hours = this.textTrainingHorizon .getHours();
		data.config_training_sample_count  = this.textTrainingCount   .getInt();
		data.config_training_sample_delay  = this.textTrainingDelay   .getSeconds();
		data.config_training_mph           = this.textTrainingMeasures.getInt();
		Couch.setDoc(this, "gui", data);
		this.storeDataTraining(data);
	});
}

ConfigPanel.prototype.onSaveClock = function () {
	Couch.getDoc(this, "gui", function(data) {
		data.config_clock_repeat           = this.checkUpdateRepeat   .getValue();
		data.config_clock_interval         = this.textUpdateInterval  .getInt();
		data.config_clock_show_seconds     = this.checkShowSeconds    .getValue();
		data.config_clock_show_ampm        = this.checkShowAmPm       .getValue();
		data.config_clock_testing          = this.checkClockTesting   .getValue();
		//
		var nDay  = this.listTestDate.getSecondsForDay();
		var nTime = this.textTestTime.getSeconds();
		//
		if (isNumeric(nDay) && isNumeric(nTime))
			var nStart = nDay + nTime;
		else if (isNumeric(nDay))
			var nStart = nDay;
		else if (isNumeric(nTime))
			var nStart = nTime;
		else 
			var nStart = null;
		//
		data.config_clock_test_start       = nStart;
		//
		GlobalData.config_clock_update_now = true;
		if (data.config_clock_repeat) GlobalData.config_clock_update_now = true;
		//
		Couch.setDoc(this, "gui", data);
		this.storeDataClock(data);
	});
}

ConfigPanel.prototype.onClockTest = function() {
	var bDisable = this.checkClockTesting.getValue();
	this.label36.disable(!bDisable);
	this.label37.disable(!bDisable);
	this.listTestDate.disable(!bDisable);
	this.textTestTime.disable(!bDisable);
}

ConfigPanel.prototype.onDeleteOrphans = function(bDelete, callback) {
	if (bDelete == null) bDelete = true;
	var t_ids = {};
	var e_ids = {};
	var i_ids = {};
	var g_ids = {};
	var delete_me = [];
	Couch.getView(this, Couch.TrainingList, function(data) {
		var rows = data && data.rows ? data.rows : [];
		for (var i = 0; i < rows.length; i++) {
			var row = rows[i];
			var id = row.id;
			t_ids[id] = {};
		}
		console.log("training", t_ids);
		Couch.getView(this, Couch.EventsList, function(data) {
			var rows = data && data.rows ? data.rows : [];
			for (var i = 0; i < rows.length; i++) {
				var row = rows[i];
				var id = row.id;
				e_ids[id] = {};
			}
			console.log("events", e_ids);
			Couch.getView(this, Couch.InversionListByEvent, function(data) {
				var rows = data && data.rows ? data.rows : [];
				for (var i = 0; i < rows.length; i++) {
					var row = rows[i];
					var id = row.id;
					var key = row.key;
					i_ids[id] = true;
					if (!t_ids[key] && !e_ids[key]) delete_me.push({"notes": "orphan-inversion", "id": id});
				}
				console.log("inversion", i_ids);
				Couch.getView(this, Couch.GrabListByEvent, function(data) {
					var rows = data && data.rows ? data.rows : [];
					for (var i = 0; i < rows.length; i++) {
						var row = rows[i];
						var id = row.id;
						var key = row.key;
						g_ids[id] = true;
						if (!t_ids[key] && !e_ids[key]) delete_me.push({"notes": "orphan-grabsample", "id": id});
					}
					console.log("grabsample", g_ids);
					Couch.getView(this, Couch.EventsLog, function(data) {
						var rows = data && data.rows ? data.rows : [];
						console.log("log", rows);
						for (var i = 0; i < rows.length; i++) {
							var row = rows[i];
							var id = row.id;
							var key = row.key;
							if (!t_ids[key] && !e_ids[key]) delete_me.push({"notes": "orphan-eventlog", "id": id});
						}
						Couch.getView(this, Couch.EventsCanaryGrid, function(data) {
							var rows = data && data.rows ? data.rows : [];
							console.log("canarygrid", rows);
							for (var i = 0; i < rows.length; i++) {
								var row = rows[i];
								var id = row.id;
								var key = row.key;
								if (!t_ids[key] && !e_ids[key]) delete_me.push({"notes": "orphan-canarygrid", "id": id});
							}
							Couch.getView(this, Couch.EventsGrabGrid, function(data) {
								var rows = data && data.rows ? data.rows : [];
								console.log("grabgrid", rows);
								for (var i = 0; i < rows.length; i++) {
									var row = rows[i];
									var id = row.id;
									var key = row.key;
									if (!t_ids[key] && !e_ids[key]) delete_me.push({"notes": "orphan-grabgrid", "id": id});
								}
								Couch.getView(this, Couch.MeasureList, function(data) {
									var rows = data && data.rows ? data.rows : [];
									console.log("measure", rows);
									for (var i = 0; i < rows.length; i++) {
										var row = rows[i];
										var id = row.id;
										var key = row.key;
										if (!i_ids[key]) delete_me.push({"notes": "orphan-measure", "id": id});
									}
									Couch.getView(this, Couch.ScenariosList, function(data) {
										var rows = data && data.rows ? data.rows : [];
										console.log("scenarios", rows);
										for (var i = 0; i < rows.length; i++) {
											var row = rows[i];
											var id = row.id;
											var key = row.key;
											if (!g_ids[key]) delete_me.push({"notes": "orphan-scenarios", "id": id});
										}
										//
										// delete all orphans
										//
										if (bDelete) {
											for (var i = 0; i < delete_me.length; i++) {
												console.log(i, "deleting", delete_me[i].notes, delete_me[i].id);
												Couch.DeleteDoc(delete_me[i].id);
											}
										} else {
											if (callback) callback.call(this, delete_me.length);
										}
									});
								});
							});
						});
					});
				});
			});
		});
	});
}

ConfigPanel.prototype.onDeleteUnknown = function() {
	Couch.deleteEntireView(Couch.UnknownList);
}

ConfigPanel.prototype.onDeleteGis = function() {
	Couch.deleteEntireView(Couch.GisList);
}

ConfigPanel.prototype.onDeleteSvg = function() {
	Couch.deleteEntireView(Couch.SvgList);
}

ConfigPanel.prototype.onDeleteInp = function() {
	Couch.deleteEntireView(Couch.InpList);
}

ConfigPanel.prototype.onDeleteSim = function() {
	Couch.deleteEntireView(Couch.SimList);
}

ConfigPanel.prototype.onDeleteMesGen = function() {
	Couch.deleteEntireView(Couch.GenList);
}

ConfigPanel.prototype.onDeleteImpact = function() {
	Couch.deleteEntireView(Couch.ImpactList);
}

ConfigPanel.prototype.onDeleteInversion = function() {
	Couch.deleteEntireView(Couch.InversionList);
}

ConfigPanel.prototype.onDeleteGrab = function() {
	Couch.deleteEntireView(Couch.GrabList);
}

ConfigPanel.prototype.onDeleteTraining = function() {
	Couch.deleteEntireView(Couch.TrainingList);
}

ConfigPanel.prototype.onDeleteEvents = function() {
	Couch.deleteEntireView(Couch.EventsList);
}

ConfigPanel.prototype.onDeleteAll = function() {
	Couch.deleteEntireView(Couch.DeleteAll);
}


ConfigPanel.prototype.onDeleteOrphansCount = function() {
	this.onDeleteOrphans(false, function(count){
		this.labelDbOrphans.setText(count);
	});
}
ConfigPanel.prototype.onDeleteUnknownCount = function() {
	Couch.getViewRowCount(this, Couch.UnknownList, function(count) {
		this.labelDbUnknown.setText(count);
	});
}
ConfigPanel.prototype.onDeleteGisCount = function() {
	Couch.getViewRowCount(this, Couch.GisList, function(count) {
		this.labelDbGis.setText(count);
	});
}
ConfigPanel.prototype.onDeleteSvgCount = function() {
	Couch.getViewRowCount(this, Couch.SvgList, function(count) {
		this.labelDbSvg.setText(count);
	});
}
ConfigPanel.prototype.onDeleteInpCount = function() {
	Couch.getViewRowCount(this, Couch.InpList, function(count) {
		this.labelDbInp.setText(count);
	});
}
ConfigPanel.prototype.onDeleteSimCount = function() {
	Couch.getViewRowCount(this, Couch.SimList, function(count) {
		this.labelDbSim.setText(count);
	});
}
ConfigPanel.prototype.onDeleteMesGenCount = function() {
	Couch.getViewRowCount(this, Couch.GenList, function(count) {
		this.labelDbMesGen.setText(count);
	});
}
ConfigPanel.prototype.onDeleteImpactCount = function() {
	Couch.getViewRowCount(this, Couch.ImpactList, function(count) {
		this.labelDbImpact.setText(count);
	});
}
ConfigPanel.prototype.onDeleteInversionCount = function() {
	Couch.getViewRowCount(this, Couch.InversionList, function(count) {
		if (count > 0) count = "" + count + "  (x 2)";
		this.labelDbInversion.setText(count);
	});
}
ConfigPanel.prototype.onDeleteGrabCount = function() {
	Couch.getViewRowCount(this, Couch.GrabList, function(count) {
		if (count > 0) count = "" + count + "  (x 2)";
		this.labelDbGrab.setText(count);
	});
}
ConfigPanel.prototype.onDeleteTrainingCount = function() {
	Couch.getViewRowCount(this, Couch.TrainingList, function(count) {
		if (count > 0) count = "" + count + "  (x 4)";
		this.labelDbTraining.setText(count);
	});
}
ConfigPanel.prototype.onDeleteEventsCount = function() {
	Couch.getViewRowCount(this, Couch.EventsList, function(count) {
		this.labelDbEvents.setText(count);
	});
}
ConfigPanel.prototype.onDeleteAllCount = function() {
	Couch.getViewRowCount(this, Couch.DeleteAll, function(count) {
		this.labelDbAll.setText(count);
	});
}

ConfigPanel.prototype.onDeleteCompact = function() {
	Couch.compact();
}

ConfigPanel.prototype.onDeleteCompactSize = function() {
	Couch.getDoc(this, "", function(data) {
		var size = data.disk_size;
		var unit = "";
		if (false) {
		} else if (size > 1024 * 1024 * 1024 * 1024) {
			size = size / 1024 / 1024 / 1024 / 1024;
			unit = "TB";
		} else if (size > 1024 * 1024 * 1024) {
			size = size / 1024 / 1024 / 1024;
			unit = "GB";
		} else if (size > 1024 * 1024) {
			size = size / 1024 / 1024;
			unit = "MB";
		} else if (size > 1024) {
			size = size / 1024;
			unit = "kb";
		} else if (true) {
			size = size;
			unit = "bytes";
		}
		this.labelDbCompact.setText("" + size.toFixed(1) + " " + unit);
	});
}

ConfigPanel.prototype.onDeleteCanary = function() {
	Couch.getDoc(this, Couch.canary + "?call=compact", function(data) {
		console.log("results = ", data);
	});
}

ConfigPanel.prototype.onDeleteCanaryCount = function() {
	Couch.getDoc(this, Couch.canary + "?call=count", function(data) {
		this.labelDbCanary.setText("" + data.count + " rows");
	});
}

ConfigPanel.prototype.onRefresh = function() {
	this.showValuesDatabase();
}

ConfigPanel.prototype.onLogRetrieve = function() {
	var def = this.textLogCharacterCount.getPlaceHolderInt();
	var nCount = this.textLogCharacterCount.getInt(def);
	Couch.getLog(this, nCount, function(data) {
		this.textLog.setValue(data);
	});
}

ConfigPanel.prototype.onSaveServer = function() {
	Couch.getDoc(this, "gui", function(data) { 
		data.config_install_dir      = addSlash(this.textWstLoc.getText());
		data.config_gis_dir          = addSlash(this.textGisLoc.getText());
		data.config_inp_exe          = this.textInpExe.getText();
		data.config_erd_exe          = this.textErdExe.getText();
		data.config_server_save_temp = this.checkSaveTemp.getValue();
		Couch.setDoc(this, "gui", data);
	});
	//
	if (this.textBindAddress.enabled())
		Couch.setConfig(this, "httpd/bind_address"        , this.textBindAddress.getText());
	if (this.textPort.enabled())
		Couch.setConfig(this, "httpd/port"                , this.textPort       .getText());
	if (true)
		Couch.setConfig(this, "couchdb/os_process_timeout", this.textTimeout    .getText());
	//
	var sPath = "python " + addSlash(this.textExternLoc.getText());
	var list = this.textExternals.getText();
	var list = list.replace(/\s+/g, "");
	var list = list.split(",");
	for (var i = 0; i < list.length; i++) {
		var key = list[i];
		if (key.length == 0) continue;
		var value = sPath + "_" + key + ".py";
		//erd,events,gis,grab,impact,inp,inversion,kill,measure,svg,tevasim,time,training,uuid
		Couch.setConfig(this, "external/" + key, value);
		if (this.textExternals.enabled())
			Couch.setConfig(this, "httpd_db_handlers/_" + key, "{couch_httpd_external, handle_external_req, <<\"" + key + "\">>}");
	}
	this.storeDataServer(data);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigPanel.prototype.onClick = function(sid) {
	switch (sid) {
		case this.SaveEventsButton.sid:
			this.onSaveEvents();
			break;
		case this.SaveTrainingButton.sid:
			this.onSaveTraining();
			break;
		case this.SaveClockButton.sid:
			this.onSaveClock();
			break;
		case this.checkClockTesting.sid:
			this.onClockTest();
			break;
			//
		case this.DatabaseUnknownButton.sid:
			this.onDeleteUnknown();
			break;
		case this.DatabaseOrphansButton.sid:
			this.onDeleteOrphans();
			break;
		case this.DatabaseGisButton.sid:
			this.onDeleteGis();
			break;
		case this.DatabaseSvgButton.sid:
			this.onDeleteSvg();
			break;
		case this.DatabaseInpButton.sid:
			this.onDeleteInp();
			break;
		case this.DatabaseSimButton.sid:
			this.onDeleteSim();
			break;
		case this.DatabaseMesGenButton.sid:
			this.onDeleteMesGen();
			break;
		case this.DatabaseImpactButton.sid:
			this.onDeleteImpact();
			break;
		case this.DatabaseInversionButton.sid:
			this.onDeleteInversion();
			break;
		case this.DatabaseGrabButton.sid:
			this.onDeleteGrab();
			break;
		case this.DatabaseTrainingButton.sid:
			this.onDeleteTraining();
			break;
		case this.DatabaseEventsButton.sid:
			this.onDeleteEvents();
			break;
		case this.DatabaseAllButton.sid:
			this.onDeleteAll();
			break;
			//
		case this.DatabaseCompactButton.sid:
			this.onDeleteCompact();
			break;
		case this.DatabaseCanaryButton.sid:
			this.onDeleteCanary();
			break;
		case this.RefreshButton.sid:
			this.onRefresh();
			//
		case this.LogRetrieveButton.sid:
			this.onLogRetrieve();
			break;
			//
		case this.SaveServerButton.sid:
			this.onSaveServer();
			break;
		case this.RestartButton.sid:
			Couch.restart();
			break;
	}
}
ConfigPanel.prototype.addListeners = function() {
	this.base.prototype       .addListeners.call(this);
	this.Tabview              .addListeners();
	//
	this.LockDbOrphansButton  .addListeners();
	this.LockDbUnknownButton  .addListeners();
	this.LockDbGisButton      .addListeners();
	this.LockDbSvgButton      .addListeners();
	this.LockDbInpButton      .addListeners();
	this.LockDbSimButton      .addListeners();
	this.LockDbMesGenButton   .addListeners();
	this.LockDbImpactButton   .addListeners();
	this.LockDbInversionButton.addListeners();
	this.LockDbGrabButton     .addListeners();
	this.LockDbTrainingButton .addListeners();
	this.LockDbEventsButton   .addListeners();
	this.LockDbAllButton      .addListeners();
	//
	this.LockDatabaseButton   .addListeners();
	this.LockExternalsButton  .addListeners();
	this.LockBindAddressButton.addListeners();
	this.LockPortButton       .addListeners();
	this.LockServerButton     .addListeners();
	//
	this.listEventStartDay    .addListeners();
	//
	this.addListener(this.SaveEventsButton       );
	this.addListener(this.SaveTrainingButton     );
	//
	this.addListener(this.checkClockTesting      );
	this.addListener(this.SaveClockButton        );
	//
	this.addListener(this.DatabaseOrphansButton  );
	this.addListener(this.DatabaseGisButton      );
	this.addListener(this.DatabaseSvgButton      );
	this.addListener(this.DatabaseUnknownButton  );
	this.addListener(this.DatabaseMesGenButton   );
	//
	this.addListener(this.DatabaseEventsButton   );
	this.addListener(this.DatabaseTrainingButton );
	this.addListener(this.DatabaseGrabButton     );
	this.addListener(this.DatabaseInversionButton);
	this.addListener(this.DatabaseImpactButton   );
	this.addListener(this.DatabaseSimButton      );
	this.addListener(this.DatabaseInpButton      );
	this.addListener(this.DatabaseAllButton      );
	//
	this.addListener(this.DatabaseCompactButton  );
	this.addListener(this.DatabaseCanaryButton   );
	this.addListener(this.RefreshButton          );
	//
	this.addListener(this.LogRetrieveButton      );
	//
	this.addListener(this.SaveServerButton       );
	this.addListener(this.RestartButton          );
}
