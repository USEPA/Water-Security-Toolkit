// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

//
//
//
function Timebox(sParentId, uniqueString, dataInFrontOf, left, top, width, height) {
	this.left = left;
	this.top = top;
	this.width = width;
	this.height = height;
	this.base = Textbox;
	this.base(sParentId, uniqueString, dataInFrontOf, left, top, width, height); 
}

Timebox.prototype = new Textbox;

Timebox.prototype.getTime = function() {
	return this.getText();
}

Timebox.prototype.getHours = function(def) {
	var sTime = this.getText();
	var t = typeof sTime;
	if (t == "string" && sTime.length == 0) return def;
	return convertTimeToHours(sTime);
}

Timebox.prototype.getSeconds = function(def) {
	var sTime = this.getText();
	var t = typeof sTime;
	if (t == "string" && sTime.length == 0) return def;
	return convertTimeToSeconds(sTime);
}

Timebox.prototype.setTime = function(value) {
	this.setValue(value, "");
}

Timebox.prototype.setSeconds = function(value, def) {
	if (def == null) def = "";
	var t = typeof value;
	if (value == 0 & t == "number") {
		this.setValue("0:00");
	} else if (isBlank(value) || isNull(value) || isNaN(value)) {
		this.setValue(def);
	} else if (value == null || value == "") {
		this.setValue(def);
	} else {
		var sTime = convertSecondsToTime(value);
		this.setValue(sTime);
	}
}

Timebox.prototype.checkTime = function(nOld) {
	var nNew = this.getSeconds();
	return (nOld != nNew);
}

