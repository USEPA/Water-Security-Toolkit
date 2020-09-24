// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

//
// global constants
//
function GlobalData() {}
//
GlobalData.MOUSE_CLICK_LEFT    =   1;
GlobalData.MOUSE_CLICK_RIGHT   =   3;
GlobalData.MAX_PAGE_WIDTH      = 700;
GlobalData.MAX_PAGE_HEIGHT     = 500;
GlobalData.SUFFIX_MAX_LENGTH   =   1;
GlobalData.MENU_LEFT           =  10;
GlobalData.MENU_WIDTH          = 170;
GlobalData.MENU_HEIGHT         = 400;
GlobalData.NETWORK_VIEW_BORDER =  20;
GlobalData.PANEL_WIDTH         = 480;
GlobalData.PANEL_HEIGHT        = 420;//MENU_HEIGHT+20
//
GlobalData.x = 0;
GlobalData.y = 0;
//
GlobalData.nStrokeWidth = 1;
//
GlobalData.Days = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
GlobalData.Months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
//
// this is a dictionary of dictionaries that are called when the main browser window is resized. 
// the "key" to use for each new hook is probably gonna be the unique string used to create the html element.
// the associated "value" to that dictionary is another dictionary with the form:
// { "this": this, "function": this.resizeWindow}
// the resize window function has two arguments: w, h
GlobalData.resizeHooks = {};

// Static
GlobalData.getConfig = function(sConfig) {
	var bTraining = m_EventView ? m_EventView.getTraining() : true;
	switch (sConfig) {
		case "erd_dynamic_type":
			return GlobalData.config_erd_dynamic_type;
		case "sample_delay":
			return bTraining ? GlobalData.config_training_sample_delay : GlobalData.config_event_sample_delay;
		case "sample_count":
			return bTraining ? GlobalData.config_training_sample_count : GlobalData.config_event_sample_count;
	}
}

GlobalData.formatHttpDebugDate = function(date) {
	var sDate = "_";
	sDate += GlobalData.Days[date.getDay()] + "_";
	sDate += GlobalData.Months[date.getMonth()] + "_";
	sDate += ("" + (date.getDate()     )).lpad("0", 2) + "_";
	sDate += ("" + (date.getFullYear() )).lpad("0", 4) + "_";
	sDate += ("" + (date.getHours()    )).lpad("0", 2) + "_";
	sDate += ("" + (date.getMinutes()  )).lpad("0", 2) + "_";
	sDate += ("" + (date.getSeconds()  )).lpad("0", 2) + "_";
	return sDate;
}

GlobalData.formatClockDate = function(date) {
	var sDate = "";
	sDate += GlobalData.Days[date.getDay()] + " ";
	sDate += GlobalData.Months[date.getMonth()] + " ";
	sDate += ("" + (date.getDate()     )) + " ";
	sDate += ("" + (date.getFullYear() )).lpad("0", 4) + " ";
	if (GlobalData.config_clock_show_ampm) {
		var nHours = date.getHours();
		if (nHours > 12) nHours = nHours - 12;
		sDate += ("" + (nHours)) + ":";
	} else {
		sDate += ("" + (date.getHours())).lpad("0", 2) + ":";
	}
	sDate += ("" + (date.getMinutes()  )).lpad("0", 2);
	if (GlobalData.config_clock_show_seconds) {
		sDate += ":" + ("" + (date.getSeconds()  )).lpad("0", 2);
	}
	if (GlobalData.config_clock_show_ampm) {
		var nHours = date.getHours();
		if (nHours < 12) {
			sDate += "AM";
		} else {
			sDate += "PM";
		}
	}
	return sDate;
}

GlobalData.formatEventDate = function(date) {
	var sDate = "";
	sDate += GlobalData.Days[date.getDay()] + " ";
	sDate += GlobalData.Months[date.getMonth()] + " ";
	sDate += ("" + (date.getDate()     )) + " ";
	sDate += ("" + (date.getFullYear() )).lpad("0", 4) + " ";
	sDate += ("" + (date.getHours()    )) + ":";
	sDate += ("" + (date.getMinutes()  )).lpad("0", 2);
	//sDate += ("" + (date.getMinutes()  )).lpad("0", 2) + ":";
	//sDate += ("" + (date.getSeconds()  )).lpad("0", 2);
	return sDate;
}

GlobalData.formatLogDate = function(date) {
	var sDate = "";
	sDate += ("" + (date.getFullYear()    )).lpad("0", 4) + "-";
	sDate += ("" + (date.getMonth() + 1   )).lpad("0", 2) + "-";
	sDate += ("" + (date.getDate()        )).lpad("0", 2) + " ";
	sDate += ("" + (date.getHours()       )).lpad("0", 2) + ":";
	sDate += ("" + (date.getMinutes()     )).lpad("0", 2) + ":";
	sDate += ("" + (date.getSeconds()     )).lpad("0", 2) + ".";
	sDate += ("" + (date.getMilliseconds())).lpad("0", 3);
	return sDate;
}

GlobalData.formatNewGrabDate = function(date) {
	var sDate = "";
	sDate += ("" + (date.getFullYear()    )).lpad("0", 4) + "";
	sDate += ("" + (date.getMonth() + 1   )).lpad("0", 2) + "";
	sDate += ("" + (date.getDate()        )).lpad("0", 2) + "-";
	sDate += ("" + (date.getHours()       )).lpad("0", 2) + "";
	sDate += ("" + (date.getMinutes()     )).lpad("0", 2) + "";
	sDate += ("" + (date.getSeconds()     )).lpad("0", 2) + "-";
	sDate += ("" + (date.getMilliseconds())).lpad("0", 3);
	return sDate;
}

function convertSecondsToTime(seconds) {
	if (isBlank(seconds)) return "";
	if (isNull (seconds)) return "";
	if (isNaN  (seconds)) return "";
	var hours = parseInt(seconds / 3600);
	var leftover = seconds - hours * 3600;
	var minutes = parseInt(leftover / 60);
	if (minutes < 10) minutes = "0" + minutes;
	return hours + ":" + minutes;
}

function convertTimeToSeconds(time) {
	if (time == null) return null;
	var len = time.length;
	var n = time.indexOf(":");
	//if (n < 0) return null;
	if (n < 0) {
		var s1 = time;
		var s2 = "0";
	} else {
		var s1 = time.substring(0, n);
		var s2 = time.substring(n + 1);
	}
	var hours   = parseInt(s1);
	var minutes = parseInt(s2);
	if (s1.length == 0) hours = 0;
	if (s2.length == 0) minutes = 0;
	return hours * 3600 + minutes * 60;
}

function convertTimeToHours(time) {
	if (time == null) return null;
	var len = time.length;
	var n = time.indexOf(":");
	//if (n < 0) return null;
	if (n < 0) {
		var s1 = time;
		var s2 = "0";
	} else {
		var s1 = time.substring(0, n);
		var s2 = time.substring(n + 1);
	}
	var hours   = parseInt(s1);
	var minutes = parseInt(s2);
	if (s1.length == 0) hours = 0;
	if (s2.length == 0) minutes = 0;
	return hours + minutes / 60;
}

GlobalData.convertSecondsToDays = function(seconds) {
	var type = typeof seconds;
	if (type == "string" && seconds == "") seconds = null;
	if (seconds == null) return {};
	var total_seconds = parseInt(seconds);
	var days = parseInt(total_seconds / 86400);
	var seconds_from_hours = total_seconds - 86400 * days;
	var time = convertSecondsToTime(seconds_from_hours);
	return {"days": days, "time": time, "day": days + 1, "seconds_from_hours": seconds_from_hours, "total_seconds": total_seconds, "seconds": seconds};
}

function isNotBlank(val) {
	return !isBlank(val);
}

function isBlank(val) {
	var t = typeof val;
	return (t == "string" && val.length == 0);
}

function isNotZero(val) {
	return !isZero(val);
}

function isZero(val) {
	var t = typeof val;
	return (t == "number" && val == 0);
}

function isNumeric(val) {
	if (isZero (val)) return true;
	if (isNull (val)) return false;
	if (isBlank(val)) return false;
	if (isNaN  (val)) return false;
	var t = typeof val;
	return (t == "number");
}

function isNotNull(val) {
	return !isNull(val);
}

function isNull(val) {
	var t = typeof val;
	if (t == "undefined") return true;
	if (val == null) return true;
	return false;
}

function isSingleSpace(val) {
	var t = typeof val;
	if (t == "string") {
		if (val.length == 1) {
			if (val == " ") return true;
		}
	}
	return false;
}

function addSlash(s) {
	s = s.trim();
	if (s.endsWith("/")) return s;
	return s + "/";
}

function convertToPx(val) { // to do the inverse just use parseInt()
	if (val == null) return "";
	return val + "px";
}

function SigFig(n, val) {
	if (n < 1) return val;
	var max = Math.pow(10, n - 1);
	if (val >= max) return Math.floor(val);
	if (val == 0) return val.toFixed(n - 1);
	var log10 = Math.log(val) / Math.log(10);
	var left = Math.floor(log10) + 1;
	return val.toFixed(n - left);
}

// not used.
function getNumDigits(sVal) {
	var index = sVal.indexOf(".");
	if (index < 0) return null;
	return {"before": index, "after": sVal.len - index - 1};
}

GlobalData.setAttributesNetworkList = function(sel) {
		sel.attr("data-rev",      function(d,i) { 
			return (d && d.value) ? d.value.rev      : null; });
		sel.attr("data-date",     function(d,i) { 
			return (d && d.value) ? d.key            : null; });
		sel.attr("data-id",       function(d,i) {
			return (d && d.value) ? d.id             : null; });
		sel.attr("data-value",    function(d,i) {
			return (d && d.value) ? d.id             : null; });
		sel.attr("data-jsonfile", function(d,i) { 
			return (d && d.value) ? d.value.jsonFile : null; });
		sel.attr("data-wqmfile",  function(d,i) { 
			return (d && d.value) ? d.value.wqmFile  : null; });
		sel.attr("data-filename", function(d,i) {
			return (d && d.value) ? d.value.fileName : null; });
		sel.attr("data-duration", function(d,i) {
			return (d && d.value && d.value.TimeData) ? d.value.TimeData.Duration   : null; });
		sel.attr("data-step",     function(d,i) {
			return (d && d.value && d.value.TimeData) ? d.value.TimeData.ReportStep : null; });
		sel.text(                 function(d,i) { 
			return (d && d.value) ? d.value.name     : null; });
}

function GlobalAddListeners(selection, m_this) {
	//console.log("delete line 56 in GlobalData.js. replace with Control???");
	selection
		.on("mousemove" , function() { m_this.onMouseMove (this.id); })
		.on("mouseover" , function() { m_this.onMouseOver (this.id); })
		.on("mouseout"  , function() { m_this.onMouseOut  (this.id); })
		.on("mouseup"   , function() { m_this.onMouseUp   (this.id); })
		.on("mousedown" , function() { m_this.onMouseDown (this.id); })
//		.on("mousewheel", function() { m_this.onMouseWheel(this.id); })
		.on("keydown"   , function() { m_this.onKeyDown   (this.id); })
		.on("keypress"  , function() { m_this.onKeyPress  (this.id); })
		.on("keyup"     , function() { m_this.onKeyUp     (this.id); })
		.on("input"     , function() { m_this.onInput     (this.id); })
		.on("change"    , function() { m_this.onChange    (this.id); })
		.on("click"     , function() { m_this.onClick     (this.id); })
		.on("dblclick"  , function() { m_this.onDblClick  (this.id); })
		;
}

// i had to create my own unique id creator since the one built in to couch 
// was causing some weird errors if it contained and "E" like it thought the
// string was actually a number or something
function createUniqueId() {
	raiseError();
	var uuid = "";
	//              1234567890123456789012345
	var sLetters = "ABCDFGHIJKLMNOPQRSTUVWXYZ";
	var nTime = new Date().getTime();
	var sTime = "" + nTime;
	for (var i=0; i < sTime.length; i++) {
		var n = parseInt(sTime.substring(i,i+1));
		uuid = uuid + sLetters.substring(n,n+1);
	}
	var dRandom = Math.random();
	var nRandom = Math.floor(dRandom * 1e16);
	var sRandom = "" + nRandom;
	for (var i=0; i < sRandom.length; i++) {
		var n = parseInt(sRandom.substring(i,i+1));
		uuid = uuid + sLetters.substring(n,n+1);
	}
	return uuid;
}

// this prevents the list box and the text area from passing their scroll onto the main window
// once the list or box has reached the end (top or bottom) of its scroll.
// currently this works for list boxes.
// the code for text areas is in the ClassTextarea.js
function StopScroll(e, list) {
	if (list.stopScroll) return list.stopScroll(e);
	e.cancelBubble = true;
	return;
	//
	var obj = list.getObject();
	var rows = obj.rows ? obj.rows : 0;
	var count = list.getLineCount ? list.getLineCount() : obj.childElementCount;
	var size = obj.size ? obj.size : rows;
	if (obj.scrollTop == 0 && getWheelDelta(e) > 0)
		return cancelEvent(e);
	var a = obj.scrollTop / ((obj.clientHeight+1) / size);
	var b = count - size;
	if ((a == b || b < 0) && getWheelDelta(e) < 0)
		return cancelEvent(e);
	return;
}

function HasAncestorId(element, id) {
	if (element.id == id) return true;
	if (element.parentElement == null) return false;
	if (HasAncestorId(element.parentElement, id)) return true;
}

function clone(obj) {
	if (obj == null) return null;
	var target = {};
	for (var i in obj) {
		if (obj.hasOwnProperty(i)) {
			target[i] = obj[i];
 		}
 	}
 	return target;
 }

function modified(obj1, obj2) {
	for (var prop in obj1) {
		if (obj1.hasOwnProperty(prop) && obj2.hasOwnProperty(prop)) {
			if (obj1[prop] != obj2[prop]) return true;
 		}
 	}
	for (var prop in obj2) {
		if (obj1.hasOwnProperty(prop) && obj2.hasOwnProperty(prop)) {
			if (obj1[prop] != obj2[prop]) return true;
 		}
 	}
 	return false;
 }

GlobalData.hidePanel = function(nPanel) {
}

function SetTrue(value) {
	value = true;
	return value;
}

//

// this extends the String object to work like the python object 
// where {0} and {1} get replaced by the arguments in the format 
// function call
if (typeof String.prototype.format != "function") {
	String.prototype.format = function() {
		var args = arguments;
		return this.replace(/{(\d+)}/g, function(match, number) { 
			return typeof args[number] != "undefined"
				? args[number]
				: match
				;
		});
	}
}

if (typeof String.prototype.startsWith != "function") {
	String.prototype.startsWith = function(str) {		
		//return this.indexOf(str) === 0;
		//return this.slice(0, str.length) === str;
		return this.lastIndexOf(str, 0) === 0;
	}
}

if (typeof String.prototype.endsWith != "function") {
	String.prototype.endsWith = function(str) {		
		//return this.indexOf(str) === 0;
		return this.slice(-str.length) === str;
		//return this.indexOf(str, this.length-str.length) === this.length-str.length
	}
}

//pads left
if (typeof String.prototype.lpad != "function") {
	String.prototype.lpad = function(padString, length) {
		var str = this;
		while (str.length < length)
			str = padString + str;
		return str;
	}
} 

//pads right
if (typeof String.prototype.rpad != "function") {
	String.prototype.rpad = function(padString, length) {
		var str = this;
		while (str.length < length)
			str = str + padString;
		return str;
	}
}

//trimming space from both side of the string
if (typeof String.prototype.trim != "function") {
	String.prototype.trim = function() {
		return this.replace(/^\s+|\s+$/g,"");
	}
 }
 
//trimming space from left side of the string
if (typeof String.prototype.ltrim != "function") {
	String.prototype.ltrim = function() {
		return this.replace(/^\s+/,"");
	}
}

//trimming space from right side of the string
if (typeof String.prototype.rtrim != "function") {
	String.prototype.rtrim = function() {
		return this.replace(/\s+$/,"");
	}
}

//String.prototype.