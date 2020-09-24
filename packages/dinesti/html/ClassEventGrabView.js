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
function EventGrabView(sParentId) {
	var uniqueString = "EventGrabView";
	this.uniqueString = uniqueString;
	this.sParentId = sParentId;
	this.gid = "g" + this.uniqueString;
	this.svgbgid = "svg" + this.uniqueString + "Bg";
	this.rectbgid = "rect" + this.uniqueString + "Bg";
	this.svgid = "svg" + this.uniqueString;
	this.rectid = "rect" + this.uniqueString;
	//
	GlobalData.resizeHooks[uniqueString] = {"this": this, "function": this.resizeWindow};
	//
	//
	// Private properties
	//
	var m_uuids = null;
	//
	// Private functions
	//
	function setUuids_private(value) { m_uuids = value ? value : ""; }
	//
	function getUuids_private() { return m_uuids; }
	//
	// Privileged functions
	//
	this.setUuids = function(value) { setUuids_private(value); }
	//
	this.getUuids = function() { return getUuids_private(); }
	//
	this.create();
}
//
// Public Member Methods
//
EventGrabView.prototype.create = function() {
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
		.attr("fill-opacity", 0.0)
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
	this.d3Title =
	d3.select("#" + this.svgid).append("text").attr("id", "GrabSampleOptimizationTitle")
		.attr("x", 300)
		.attr("y", 230)
		.attr("font-size", 30)
		.text("Grab Sample Optimization")
		;
	////////////////////////////////////////////////////////////////////////////////
	var L1 = NETWORK_VIEW_BORDER + 20;
	this.CompleteButton = new SvgButton("Wizard Complete"        , this, "EventGrabButtonComplete", null,  L1, 0, 150, 30, 15);
	this.SkipButton     = new SvgButton("Skip optimization..."   , this, "EventGrabButtonSkip"    , null, 340, 0, 150, 30, 15);
	this.PerformButton  = new SvgButton("Perform optimization...", this, "EventGrabButtonPerform" , null, 640, 0, 150, 30, 15);
	//
	var z = 1.5;
	var dy = 5;
	var x = 300;
	var y = 330 - dy;
	var h = 40;
	var w = 220;
	var i = 0;
	//
	this.labelDay   = new Label("Sample day:"              , this.svgid, "labelDay"  , null, x, 0, w, h);
	this.labelTime  = new Label("Sample time (h:m):"       , this.svgid, "labelTeam" , null, x, 0, w, h);
	this.labelCount = new Label("Number of sampling teams:", this.svgid, "labelCount", null, x, 0, w, h);
	//
	this.labelDay   .enlarge(z);
	this.labelTime  .enlarge(z);
	this.labelCount .enlarge(z);
	//
	var x = x + w + 30;
	var w = 100;
	//
	this.listDay   = new Dropdown(this.gid, "listEventGrabViewDay"  , null, 0, 0, w+6, false);
	this.textDay   = new Textbox (this.gid, "textEventGrabViewDay"  , null, 0, 0, w);
	this.textTime  = new Timebox (this.gid, "textEventGrabViewTime" , null, 0, 0, w);
	this.textCount = new Textbox (this.gid, "textEventGrabViewCount", null, 0, 0, w);
	//
	this.textDay   .enlarge(z);
	this.textTime  .enlarge(z);
	this.textCount .enlarge(z);
	//
	this.listDay.setAttributes = function(sel) {
		sel.attr("data-epoch", function(d,i) {
			return d.epoch; });
		sel.attr("data-value", function(d,i) {
			return d.index;        });
		sel.text(              function(d,i) {
			var date = new Date(d.epoch * 1000);
			var day = date.getDay();
			var day = GlobalData.Days[day];
			var mon = date.getMonth();
			var mon = GlobalData.Months[mon];
			var num = date.getDate();
			var h   = date.getHours();
			return day + " " + mon + " " + num; });
	}
	//
	//this.listDay.uniqueAttribute = "data-value";
	//
	this.listDay.updateData = function(epoch) {
		if (epoch == null) {
			Clock.getServerTime(this, function(data) {
				this.updateDataWithTime(data.epoch);
			});
		} else {
			this.updateDataWithTime(epoch);
		}
	}
	this.listDay.updateDataWithTime = function(epoch) {
		var datetime = new Date(epoch * 1000);
		var year = datetime.getFullYear();
		var month = datetime.getMonth();
		var number = datetime.getDate();
		var date = new Date(year, month, number, 0, 0, 0, 0);
		var currentTime = epoch;
		epoch = parseInt(date.valueOf() / 1000.0);
		var list = [];
		if (epoch == null) return;
		for (var i = -2; i <= 10; i++) {
			list.push({"index": i, "epoch": epoch + 24 * 60 * 60 * i});
		}
		this.updateList(list);
		this.selectValue("0");
	}
	//
	this.d3grouppath = 
	d3.select("#" + this.svgid).append("path").attr("id", "pathEventInputs")
		.attr("d", "M220,160 v300 h500 v-300 Z")
		.attr("stroke", d3.rgb(49,49,49))
		//.attr("stroke", "none")
		.attr("stroke-opacity", 0.5)
		.attr("fill", "none")
		;
	this.d3grouppath2 = 
	d3.select("#" + this.svgid).append("path").attr("id", "path2EventInputs")
		.attr("d", "M280,280 v130 h380 v-130 Z")
		.attr("stroke", d3.rgb(49,49,49))
		//.attr("stroke", "none")
		.attr("stroke-opacity", 0.5)
		.attr("fill", "none")
		;
	//
	this.addListeners();
}

///////////////////////////////////////////////////////

EventGrabView.prototype.updateLabels = function() {
	this.labelDay  .update2();
	this.labelTime .update2();
	this.labelCount.update2();
}

EventGrabView.prototype.resizeWindow = function(w, h) {
	this.d3svgbg.style("width" , convertToPx(w));
	this.d3svgbg.style("height", convertToPx(h));
	this.d3rectbg.attr("width" , w);
	this.d3rectbg.attr("height", h);
	this.d3svg.style("width",  convertToPx(w - 2 * NETWORK_VIEW_BORDER));
	this.d3svg.style("height", convertToPx(h - 2 * NETWORK_VIEW_BORDER));
	this.d3rect.attr("width",  w - 2 * NETWORK_VIEW_BORDER);
	this.d3rect.attr("height", h - 2 * NETWORK_VIEW_BORDER);
	//
	this.CompleteButton.setTop(h - NETWORK_VIEW_BORDER - this.CompleteButton.getHeight() - 10);
	this.SkipButton    .setTop(h - NETWORK_VIEW_BORDER - this.SkipButton.getHeight()     - 10);
	this.PerformButton .setTop(h - NETWORK_VIEW_BORDER - this.PerformButton.getHeight()  - 10);
	//
	var L1 = this.CompleteButton.getLeft();
	var R1 = L1 + this.CompleteButton.getWidth();
	var R3 = w - NETWORK_VIEW_BORDER - 10;
	var L3 = R3 - this.SkipButton.getWidth();
	var L2 = R1 + (L3 - R1) / 2 - this.PerformButton.getWidth() / 2;
	//
	this.CompleteButton.setLeft(L1);
	this.SkipButton    .setLeft(L2);
	this.PerformButton .setLeft(L3);
	//
	var d = "M" + (w/2 - 250) + "," + (h/2 - 180) + " v300 h505 v-300 Z";
	var d2 = "M" + (w/2 - 190) + "," + (h/2 - 80) + " v130 h385 v-130 Z";
	this.d3grouppath.attr("d", d);
	this.d3grouppath2.attr("d", d2);
	this.d3Title.attr("x", w / 2 - 170);
	this.d3Title.attr("y", h / 2 - 120);
	this.labelDay  .setLeft(w / 2 - 170);
	this.labelTime .setLeft(w / 2 - 170);
	this.labelCount.setLeft(w / 2 - 170);
	this.listDay   .setLeft(w / 2 + 90);
	this.textDay   .setLeft(w / 2 + 90);
	this.textTime  .setLeft(w / 2 + 90);
	this.textCount .setLeft(w / 2 + 90);
	this.labelDay  .setTop (h / 2 - 40);
	this.labelTime .setTop (h / 2 - 10);
	this.labelCount.setTop (h / 2 + 30);
	this.listDay   .setTop (h / 2 - 40);
	this.textDay   .setTop (h / 2 - 40);
	this.textTime  .setTop (h / 2 - 10);
	this.textCount .setTop (h / 2 + 30);
	//
}

///////////////////////////////////////////////////////

EventGrabView.prototype.show = function() {
	var bTraining = m_EventView.getTraining();
	if (bTraining) {
		this.listDay.hide();
		this.textDay.show();
	} else {
		this.listDay.show();
		this.textDay.hide();
	}
	var nDelay = GlobalData.getConfig("sample_delay");
	var uuids = this.getUuids();
	Clock.getServerTime(this, function(data) {
		var date = new Date(data.time + 1000 * nDelay); // milliseconds
		var nTime = 3600 * date.getHours() + 60 * date.getMinutes() + date.getSeconds();
		Couch.getDoc(this, uuids.uuid, function(data) {
			if (bTraining) {
				var seconds = parseInt(data.sampleTime) + nDelay;
				var obj = GlobalData.convertSecondsToDays(seconds);
				this.textDay.setValue(obj.day, "");
				this.textTime.setSeconds(obj.seconds_from_hours, "");
			} else {
				var epoch = date.valueOf() / 1000.0;
//				this.listDay.updateData(parseInt(epoch));
				this.listDay.updateData(epoch);
				this.textTime.setSeconds(nTime);
			}
			var nCount = GlobalData.getConfig("sample_count");
			this.textCount.setValue(nCount);
			//data.time = time;
			data.view = "grab";
			Couch.setDoc(this, data._id, data, function(e,res) {
				ResizeWindow();
				this.addListeners();
				this.d3g.style("visibility", "inherit");
			});
		});
	});
}

EventGrabView.prototype.hide = function() {
	this.d3g.style("visibility", "hidden");
}

EventGrabView.prototype.isVisible = function() {
	var sVisible = this.d3g.style("visibility");
	return !(sVisible == "hidden");
}

EventGrabView.prototype.validateInput = function() {
	var b1 = this.validateTime();
	var b2 = this.validateCount();
	var bDisable = !b1 || !b2;
	this.PerformButton.disable(bDisable);
}

EventGrabView.prototype.validateCount = function() {
	var count = this.textCount.getInt();
	if (count == null) return false;
	if (count < 1) return false;
	return true;
}

EventGrabView.prototype.validateTime = function() {
	var time = this.getSampleTime();
	if (time == null) return false;
	return true;
}

EventGrabView.prototype.getSampleTime = function() {
	var seconds = this.textTime.getSeconds();
	if (seconds == null) return null;
	if (seconds < 0) return null;
	if (m_EventView.getTraining()) {
		var day = this.textDay.getInt();
		if (day == null) return null;
		if (day < 1) return null;
		return 24 * 60 * 60 * (day - 1) + seconds;
	} else {
		var epoch = this.listDay.getSelectedUserDataInt("epoch");
		return epoch + seconds;
	}
}

///////////////////////////////////////////////////////

EventGrabView.prototype.onComplete = function() {
	var uuids = this.getUuids();
	Events.log(null, uuids.log, ["Complete (grab sample view)", ""]);
	m_EventView.hide();
	m_NetworkView.hide();
	this.hide();
}

EventGrabView.prototype.onPerform = function() {
	m_Waiting.show();
	m_EventView.waiting = true;
	var uuids = this.getUuids();
	Events.log(null, uuids.log, "Perform grab sample optimization");
	Couch.getDoc(this, uuids.uuid, function(data) {
		data.view = "data";
		data.statusOld = data.status;
		data.status = "Locating";
		var day = this.textDay.getInt();
		var seconds  = this.textTime.getSeconds();
		// TODO - figure out the actual sample time
		data.sampleTime  = this.getSampleTime();
		data.sampleCount = this.textCount.getInt();
		var list = []
		list.push("    Sample Count = " + data.sampleCount);
		list.push("    Sample Time  = " + data.sampleTime);
		Events.log(null, uuids.log, list);
		Couch.setDoc(this, uuids.uuid, data, function(data) {
			Couch.getDoc(this, m_EventView.getExe() + "?call=locate&uuid=" + uuids.uuid, function(data) {
				var data = data ? data : {};
			});
		});
	});
	m_NetworkView.hide();
	this.hide();
}

EventGrabView.prototype.onSkip = function() {
	var uuids = this.getUuids();
	Events.log(null, uuids.log, "Skip grab sample optimization");
	Couch.getDoc(this, uuids.uuid, function(data) {
		data.view = "data";
		Couch.setDoc(this, uuids.uuid, data, function(data) {
			m_NetworkView.hide();
			this.hide();
		});
	});
}

///////////////////////////////////////////////////////

EventGrabView.prototype.onMouseMove = function(sid) {}
EventGrabView.prototype.onMouseOver = function(sid) {}
EventGrabView.prototype.onMouseOut = function(sid) {}
EventGrabView.prototype.onMouseUp = function(sid) {}
EventGrabView.prototype.onMouseDown = function(sid) {}
//EventGrabView.prototype.onMouseWheel = function(sid) {}
EventGrabView.prototype.onKeyDown = function(sid) {}
EventGrabView.prototype.onKeyPress = function(sid) {}
EventGrabView.prototype.onKeyUp = function(sid) {}
EventGrabView.prototype.onInput = function(sid) {
	switch (sid) {
		case this.listDay.sid:
		case this.textDay.sid:
		case this.textTime.sid:
		case this.textCount.sid:
			this.validateInput();
			return;
	}
}

EventGrabView.prototype.onChange = function(sid) {
	switch (sid) {
		case this.listDay.sid:
		case this.textDay.sid:
		case this.textTime.sid:
		case this.textCount.sid:
			return;
	}
}

EventGrabView.prototype.onClick = function(sid) {
	switch (sid) {
		case this.CompleteButton.sid:
			this.onComplete();
			return;
		case this.SkipButton.sid:
			this.onSkip();
			return;
		case this.PerformButton.sid:
			this.onPerform();
			return;
	}
}

EventGrabView.prototype.onDblClick = function(sid) {}

EventGrabView.prototype.onBlur = function(sid) {
	switch (sid) {
		case this.listDay.sid:
		case this.textDay.sid:
		case this.textTime.sid:
		case this.textCount.sid:
			return;
	}
}

///////////////////////////////////////////////////////

EventGrabView.prototype.addListeners = function() {
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
	this.CompleteButton.addListeners();
	this.SkipButton    .addListeners();
	this.PerformButton .addListeners();
}

EventGrabView.prototype.addListenersForSelection = function(sSelector) {
	var m_this = this;
	var d3g = d3.select("#" + this.gid);
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


