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
function Checkbox(sParentId, uniqueString, dataInFrontOf, left, top, width) {
	this.left = left;
	this.top = top;
	this.width = width;
	this.base = Control;
	this.base(sParentId, uniqueString, dataInFrontOf); // this calls the "create" method which has been overridden here
}

Checkbox.prototype = new Control;

Checkbox.prototype.create = function() {
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
		.attr("type", "checkbox")
		;
	this.addListeners();
}

Checkbox.prototype.setValue = function(val, def) {
	if (def == null) def = false;
	if (val == null) val = def;
	var obj = this.getObject();
	obj.checked = val;
}

Checkbox.prototype.getValue = function() {
	var obj = this.getObject();
	return obj.checked;
}

Checkbox.prototype.setLeft = function(val) {
	this.left = val;
	this.d3g.style("left", convertToPx(val));
}

Checkbox.prototype.setTop = function(val) {
	this.top = val;
	this.d3g.style("top", convertToPx(val));
}

Checkbox.prototype.setPosition = function(p_or_x, null_or_y) {
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

Checkbox.prototype.disable = function(bDisabled) {
	var input = this.getObject();
	input.disabled = bDisabled;
}
