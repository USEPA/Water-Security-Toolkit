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
function Button(sText, sParentId, uniqueString, dataInFrontOf, left, top, width, height) {
	this.left = left;
	this.top = top;
	this.width = width;
	this.height = height;
	this.base = Control;
	this.base(sParentId, uniqueString, dataInFrontOf);
	this.create(sText);
}

Button.prototype = new Control;

Button.prototype.create = function(sText) {
	if (sText == null) return; // this prevents the base class call from actually happening.
	this.d3parent = d3.select("#" + this.sParentId);
	this.d3g = this.d3parent.append("g")
		.attr("id",this.gid)
		.attr("data-InFrontOf",this.dataInFrontOf)
		.style("position","absolute")
		.style("left",convertToPx(this.left))
		.style("top",convertToPx(this.top))
		;
	this.d3control = this.d3g.append("button")
		.attr("data-InFrontOf",this.dataInFrontOf)
		.attr("id",this.sid)
		.text(sText)
		;
	if (this.width)
		this.d3control.style("width",convertToPx(this.width));
	if (this.height)
		this.d3control.style("height",convertToPx(this.height));
	this.addListeners();
}

Button.prototype.setText = function(val) {
	var input = this.getObject();
	input.textContent = val;
}

Button.prototype.getText = function() {
	var input = this.getObject();
	var val = input.textContent;
	return val;
}

Button.prototype.setLeft = function(dVal) {
	this.d3g.style("left", convertToPx(dVal));
}

Button.prototype.setTop = function(dVal) {
	this.d3g.style("top", convertToPx(dVal));
}

Button.prototype.setWidth = function(dVal) {
	this.d3control.style("width", convertToPx(dVal));
}

Button.prototype.setPosition = function(p_or_x, null_or_y) {
	if (typeof(p_or_x) == "object" && typeof(null_or_y) == "undefined") {
		var p = p_or_x;
		this.setLeft(p.x);
		this.setTop(p.y);
	} else if ( typeof(p_or_x) == "number" && typeof(null_or_y) == "number") {
		var x = p_or_x;
		var y = null_or_y;
		this.setLeft(x);
		this.setTop(y);
	} else {
		alert("Error in Button.setPosition(p_or_x, null_or_y)!");
	}
}
