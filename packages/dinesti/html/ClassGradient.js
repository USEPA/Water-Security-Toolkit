// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function Gradient(uniqueString, sParentId, x1, y1, x2, y2) {
	this.uniqueString = uniqueString;
	this.sParentId = sParentId;
	this.x1 = x1;
	this.y1 = y1;
	this.x2 = x2;
	this.y2 = y2;
	this.Stops = [];
	this.defaultOpacity = 1.0;
	this.type = "linear";
	this.fill = "url(#" + this.uniqueString + ")";
}

Gradient.prototype.create = function() {
	this.d3_defs = d3.select("#" + this.sParentId).append("defs");
	this.d3_gradientLegend = this.d3_defs
		.append(this.type + "Gradient")
		.attr("id",this.uniqueString)
		.attr("x1",this.x1 + "%")
		.attr("y1",this.y1 + "%")
		.attr("x2",this.x2 + "%")
		.attr("y2",this.y2 + "%")
		;
	this.d3_gradientLegend
		.selectAll("." + this.type + "GradientStop_" + this.uniqueString)
		.data(this.Stops)
		.enter().append("stop")
		.attr("class", this.type + "GradientStop_" + this.uniqueString)
		.attr("offset",function(d,i) { return d.offset + "%"; })
		.style("stop-color",function(d,i) { return d.color; })
		.style("stop-opacity", function(d,i) { return d.opacity; })
		;

}

Gradient.prototype.addStop = function(nOffset, sColor, dOpacity) {
	if (dOpacity == null)
		dOpacity = this.defaultOpacity;
	this.Stops[this.Stops.length] = {"offset":nOffset,"color":sColor,"opacity":dOpacity}
	return this;
}

function RadialGradient(uniqueString, sParentId) {
	this.uniqueString = uniqueString;
	this.sParentId = sParentId;
	this.Stops = [];
	this.defaultOpacity = 1.0;
	this.type = "radial";
}

RadialGradient.prototype.create = function() {
	this.d3_defs = d3.select("#" + this.sParentId).append("defs");
	this.d3_gradientLegend = this.d3_defs
		.append("radialGradient")
		.attr("id", this.uniqueString)
		.attr("cx", "50%")
		.attr("cy", "50%")
		.attr("r", "50%")
		.attr("fx", "50%")
		.attr("fy", "50%")
		;
	this.d3_gradientLegend
		.selectAll("." + this.type + "GradientStop_" + this.uniqueString)
		.data(this.Stops)
		.enter().append("stop")
		.attr("class", "radialGradientStop_" + this.uniqueString)
		.attr("offset", function(d,i) { return d.offset + "%"; })
		.style("stop-color", function(d,i) { return d.color; })
		.style("stop-opacity", function(d,i) { return d.opacity; })
		;
}

RadialGradient.prototype.addStop = function(nOffset, sColor, dOpacity) {
	if (dOpacity == null)
		dOpacity = this.defaultOpacity;
	this.Stops[this.Stops.length] = {"offset":nOffset,"color":sColor,"opacity":dOpacity}
	return this;
}
