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
function Textbox(sParentId, uniqueString, dataInFrontOf, left, top, width, height) {
	this.left = left;
	this.top = top;
	this.width = width;
	this.height = height;
	this.bgColor = d3.rgb(255,255,255);
	this.bgColorDisabled = d3.rgb(170,170,170);
	this.fgColor = d3.rgb(0,0,0);
	this.fgColorDisabled = d3.rgb(90,90,90);
	this.fgColorError = d3.rgb(255,0,0);
	this.base = Control;
	this.base(sParentId, uniqueString, dataInFrontOf); // this calls the "create" method which has been overridden here
	//
	var m_min = null;
	var m_max = null;
	function setMin_private(val) {
		var t = typeof val;
		if (t == "number") {
			m_min = val;
		} else {
			var fval = parseFloat(val);
			m_min = (isNaN(fval)) ? null : fval;
		}
	}
	function setMax_private(val) {
		var t = typeof val;
		if (t == "number") {
			m_max = val;
		} else {
			var fval = parseFloat(val);
			m_max = (isNaN(fval)) ? null : fval;
		}
	}
	this.setMin = function(val) {
		setMin_private(val);
	}
	this.setMax = function(val) {
		setMax_private(val);
	}
	this.setRange = function(min, max) {
		if (min != null) setMin_private(min);
		if (max != null) setMax_private(max);
	}
	this.getMin = function() { return m_min; }
	this.getMax = function() { return m_max; }
}

Textbox.prototype = new Control;

Textbox.prototype.create = function() {
	var sel = d3.select("#" + this.gid);
	if (sel[0][0])
		sel.remove();
	this.d3parent = d3.select("#" + this.sParentId);
	this.d3g = this.d3parent.append("g")
		.attr("id", this.gid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.style("position", "absolute")
		.style("left", convertToPx(this.left))
		.style("top", convertToPx(this.top))
		;
	this.d3text = this.d3g.append("input")
		.attr("class", "dinesti")
		.attr("id", this.sid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("type", "text")
		.style("width", convertToPx(this.width))
		;
	if (this.height != null) this.d3text.style("height", convertToPx(this.height));
	this.addListeners();
}

Textbox.prototype.setArray = function(val) {
	var obj = this.getObject();
	var newVal = "";
	if (val != null && val.length > 0)
		newVal = val.join("\n");
	obj.value = newVal;
}

Textbox.prototype.getArray = function() {
	var obj = this.getObject();
	var sVal = obj.value;
	if (sVal == null) return null;
	if (sVal.length == 0) return null;
	return sVal.split("\n");
}

Textbox.prototype.getText = function() {
	var obj = this.getObject();
	if (obj.value == null) return "";/////?????
	var sVal = obj.value;
	return sVal;
}

Textbox.prototype.getFloat = function(def) {	
	var sVal = this.getText();
	var t = typeof sVal;
	if (isBlank(sVal)) {
		this.d3text.style("color", this.fgColor);
		return def;
	}
	var fVal = parseFloat(sVal);
//	var bNAN = isNaN(fVal);
//	var bOOR = this.checkRange(fVal);
//	if (bNAN || bOOR) {
	var bOutOfRange = this.checkRange(fVal);
	if (bOutOfRange) {
		this.d3text.style("color", this.fgColorError);
		return def;
	}
	this.d3text.style("color", this.fgColor);
	return fVal;
}

Textbox.prototype.getInt = function(def) {
	var sVal = this.getText();
	var fVal = parseInt(sVal);
	var bNAN = isNaN(fVal);
	var bOOR = this.checkRange(fVal);
	if (bNAN || bOOR) {
		this.d3text.style("color", this.fgColorError);
		return def;
	}
	this.d3text.style("color", this.fgColor);
	return fVal;
}

Textbox.prototype.checkRange = function(val) {
	var t = typeof this.getMin();
	if (t == "number" && val < this.getMin()) return true;
	if (t == "number" && isNaN(val)         ) return true;
	var t = typeof this.getMax();
	if (t == "number" && val > this.getMax()) return true;
	if (t == "number" && isNaN(val)         ) return true;
	return false;
}

Textbox.prototype.onBlurSetDefault = function(def) {
	if (def == null) def = "";
	var sVal = this.getText();
	var fVal = parseFloat(sVal);
	var bNAN = isNaN(fVal);
	var bOOR = this.checkRange(fVal);
	var obj = this.getObject();
	if (isBlank(sVal)) {
		//do nothing
	} else if (bNAN || bOOR) {
		obj.value = def;
	}
	this.d3text.style("color", this.fgColor);
}

Textbox.prototype.checkArray = function(val) {
	var input = this.getObject();
	var newVal = (input.value == null) ? null : (input.value.length == 0 ? null : input.value);
	var oldVal = (val == null) ? val : val.join("\n");
	return (oldVal != newVal);
}

Textbox.prototype.checkValue = function(val) {
	var input = this.getObject();
	var newVal = input.value;
	var oldVal = val;
	return (oldVal != newVal);
}

Textbox.prototype.checkFloat = function(val) {
	var input = this.getObject();
	var newVal = parseFloat(input.value);
	var oldVal = parseFloat(val);
	if (isNaN(newVal) && isNaN(oldVal))
		return false;
	return (oldVal != newVal);
}

Textbox.prototype.isEmpty = function() {
	return !this.checkValue("");
}

Textbox.prototype.setLeft = function(val) {
	this.left = val;
	this.d3g.style("left", convertToPx(val));
}

Textbox.prototype.setTop = function(val) {
	this.top = val;
	this.d3g.style("top", convertToPx(val));
}

Textbox.prototype.setWidth = function(val) {
	this.width = val;
	this.d3text.style("width", convertToPx(val));
}

Textbox.prototype.setHeight = function(val) {
	this.height = val;
	this.d3text.style("height", convertToPx(val));
}

Textbox.prototype.setPosition = function(p_or_x, null_or_y) {
	if (typeof(p_or_x) == "object" && typeof(null_or_y) == "undefined") {
		var p = p_or_x;
		this.setLeft(p.x);
		this.setTop(p.y);
	} else if (typeof(p_or_x) == "number" && typeof(null_or_y) == "number") {
		var x = p_or_x;
		var y = null_or_y;
		this.setLeft(x);
		this.setTop(y);
	} else {
		alert("Error in Textbox.setPosition(p_or_x, null_or_y)!");
	}
}

Textbox.prototype.getObject = function() {
	return document.getElementById(this.sid);
}

Textbox.prototype.setBgColor = function(color) {
	this.bgColor = color;
	var input = this.getObject();
	if (input.disabled) return;
	//this.d3text.style("background"      , color);
	this.d3text.style("background-color", color);
	this.d3text.style("border-color"    , color);
}

Textbox.prototype.setBgColorDisabled = function(color) {
	this.bgColorDisabled = color;
	var input = this.getObject();
	if (!input.disabled) return;
	//this.d3text.style("background"      , color);
	this.d3text.style("background-color", color);
	this.d3text.style("border-color"    , color);
}

Textbox.prototype.disable = function(bDisabled) {
	if (bDisabled == null) bDisabled = true;
	var input = this.getObject();
	input.disabled = bDisabled;
	if (bDisabled) {
		var bg = d3.select("#" + input.id).style("background");
		var bg = d3.select("#" + input.id).style("background-color");
		var bg = d3.select("#" + input.id).style("border-color");
		d3.select("#" + input.id).style("background-color", this.bgColorDisabled);
		d3.select("#" + input.id).style("border-color"    , this.bgColorDisabled);
		this.d3text.style("color", this.fgColorDisabled);
	} else {
		d3.select("#" + input.id).style("background-color", this.bgColor);
		d3.select("#" + input.id).style("border-color"    , this.bgColor);
		this.d3text.style("color", this.fgColor);
	}
}

Textbox.prototype.enlarge = function(z) {
	var n = this.d3text.style("font-size");
	var n = parseInt(n);
	this.d3text.style("font-size", convertToPx(n * z));
	var n = this.d3text.style("font-size");
}

Textbox.prototype.setPlaceHolderText = function(value) {
	this.d3text.attr("placeholder", value);
}

Textbox.prototype.getPlaceHolderText = function() {
	return this.d3text.attr("placeholder");
}

Textbox.prototype.getPlaceHolderInt = function() {
	var s = this.getPlaceHolderText();
	return parseInt(s);
}

Textbox.prototype.getPlaceHolderFloat = function() {
	var s = this.getPlaceHolderText();
	return parseFloat(s);
}
