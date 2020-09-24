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
function VerifyPopup(sParentId, uniqueString, left, top, width, height) {
	this.left = left;
	this.top = top;
	this.width = width;
	this.height = height;
	GlobalData.resizeHooks[uniqueString] = {"this": this, "function": this.resizeWindow};
	m_KeydownVoting.registerVoter(uniqueString, this, this.isHidden);
	this.svgbgid = "svgbg" + uniqueString;
	this.rectbgid = "rectbg" + this.uniqueString;
	this.gwid = "gw" + uniqueString;
	this.svgid = "svg" + uniqueString;
	this.rectid = "rect" + this.uniqueString;
	this.borderid = "border" + this.uniqueString;
	this.base = Control;
	this.base(sParentId, uniqueString, null);
}

VerifyPopup.prototype = new Control;

VerifyPopup.prototype.create = function() {
	this.d3parent = d3.select("#" + this.sParentId);
	this.d3g = this.d3parent.append("g")
		.attr("id", this.gid)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top", convertToPx(0))
	//	.style("visibility", "hidden")
		;
	this.d3svgbg = this.d3g.append("svg")
		.attr("id", this.svgbgid)
		.style("position", "absolute")
		.style("width", convertToPx(0)) // arbitrary
		.style("height", convertToPx(0)) // arbitrary
		;
	this.d3rectbg = this.d3svgbg.append("rect")
		.attr("id", this.rectbgid)
		.attr("x", 0)
		.attr("y", 0)
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.5)
		.attr("width", 0) // arbitrary
		.attr("height", 0) // arbitrary
		;
	this.d3gw = this.d3svgbg.append("g")
		.attr("id", this.gwid)
		.style("position", "absolute")
		;
	this.d3svg = this.d3gw.append("svg")
		.attr("id", this.svgid)
		.style("position", "absolute")
		.attr("x", 0) // arbitrary
		.attr("y", 0) // arbitrary
		;
	this.d3rect = this.d3svg.append("rect")
		.attr("id", this.rectid)
		.attr("x", 0)
		.attr("y", 0)
		.attr("width", 0) // arbitrary
		.attr("height", 0) // arbitrary
		.attr("fill", "url(#gradientBG)")
		;
	this.d3border = this.d3svg.append("rect")
		.attr("id", this.borderid)
		.attr("x", 2)
		.attr("y", 2)
		.attr("width", 0) // arbitrary
		.attr("height", 0) // arbitrary
		.attr("fill", "none")
		.attr("stroke", d3.rgb(100,100,100))
		.attr("stroke", d3.rgb(0,0,0))
		.attr("stroke", d3.rgb(255,255,255))
		.attr("stroke-width", 1.5)
		.attr("stroke-width", 4)
		.attr("stroke-width", 2)
		.attr("stroke-opacity", 0.6)
		;
//
// in my attempt to create a cool border i tried to move the rect in 3 px to
// make room for the border since it was being cut off by the svg boundaries
//
//	this.d3rect = this.d3svg.append("rect")
//		.attr("id", this.rectid)
//		.attr("x", 3)
//		.attr("y", 3)
//		.attr("width", 0) // arbitrary
//		.attr("height", 0) // arbitrary
//		.attr("fill", "url(#gradientBG)")
//		.attr("stroke", d3.rgb(255,255,255))
//		.attr("stroke-width", 2)
//		.attr("stroke-opacity", 0.6)
//		;
//	this.d3border = this.d3svg.append("rect")
//		.attr("id", this.borderid)
//		.attr("fill", "none")
//		;
	//
	this.addListeners();
	var pid = this.gid;
	var uniq = this.uniqueString;
	var w = this.width;
	var h = this.height;
	this.OkButton     = new Button  ("Ok"    , pid, uniq + "OkButton"    , null,  0,  0,  70, 20);
	this.CancelButton = new Button  ("Cancel", pid, uniq + "CancelButton", null,  0,  0,  70, 20);
	var pid = this.svgid;
	this.label        = new Label   ("label1", pid, uniq + "Label"       , null, 20, 40, 300, 20);
	this.makeParentOf(this.OkButton, this.CancelButton);
	ResizeWindow();
}

VerifyPopup.prototype.show = function(bShow) {
	this.hide(false);
}

VerifyPopup.prototype.hide = function(bHide) {
	if (bHide == null || bHide) {
		this.d3g.style("visibility","hidden");
	} else {
		this.d3g.style("visibility","inherit");
	}	
}

VerifyPopup.prototype.handleResizeEvent = function(sid) {
	this.d3rect.attr("width", areaWidth + 10);
	this.d3rect.attr("height", areaHeight + 31);
}

VerifyPopup.prototype.resizeWindow = function(w, h) {
	this.d3svgbg.style("width", convertToPx(w));
	this.d3svgbg.style("height", convertToPx(h));
	this.d3rectbg.attr("width", w);
	this.d3rectbg.attr("height", h);
	var x = (w - this.width) / 2;
	var y = (h - this.height) / 2;
	this.d3svg.attr("x", x);
	this.d3svg.attr("y", y);
	this.d3rect  .attr("width" , this.width );
	this.d3rect  .attr("height", this.height);
	//this.d3rect  .attr("width",  (this.width  > 6) ? this.width  - 6 : 0);
	//this.d3rect  .attr("height", (this.height > 6) ? this.height - 6 : 0);
	this.d3border.attr("width",  (this.width  > 4) ? this.width  - 4 : 0);
	this.d3border.attr("height", (this.height > 4) ? this.height - 4 : 0);
	//
	this.OkButton    .setPosition(x + this.width - 160, y + this.height - 30);
	this.CancelButton.setPosition(x + this.width -  80, y + this.height - 30);
}

VerifyPopup.prototype.setText = function(val) {
	this.label.setText(val);
	this.label.update2();
}

VerifyPopup.prototype.getText = function() {
	return this.label.getText();
}

VerifyPopup.prototype.onMouseDown = function(sid) {
	this.base.prototype.onMouseDown(sid);
}
VerifyPopup.prototype.onMouseUp = function(sid) {
	this.base.prototype.onMouseUp(sid);
}

VerifyPopup.prototype.onClick = function(sid) {
	switch (sid) {
		case this.OkButton.sid:
			this.raiseEvent({"event": "ok"});
			this.hide();
			break;
		case this.CancelButton.sid:
			this.hide();
			break;
	}
	this.base.prototype.onClick(sid);
}
