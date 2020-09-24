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
function InputPopup(sParentId, uniqueString, left, top, width, height) {
	this.left = left;
	this.top = top;
	this.width = width;
	this.height = height;
	GlobalData.resizeHooks[uniqueString] = {"this": this, "function": this.resizeWindow};
	this.count = 0;
	this.base = Control;
	this.base(sParentId, uniqueString, null);
	//
	var m_count = 0;
	function incr_private() {
		return ++m_count;
	}
	this.incr = function() {
		return incr_private();
	}
}

InputPopup.prototype = new Control;

InputPopup.prototype.create = function() {
	this.d3parent = d3.select("#" + this.sParentId);
	this.d3g = this.d3parent.append("g")
		.attr("id", this.gid)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top", convertToPx(0))
		;
	this.d3bg = this.d3g.append("svg")
		.attr("id", "svg" + this.uniqueString)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top", convertToPx(0))
		.style("width",convertToPx(0))
		.style("",convertToPx(0))
		;
	this.d3rect = this.d3bg.append("rect")
		.attr("id", "rect1" + this.uniqueString)
		.attr("x", 0)
		.attr("y", 0)
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.5)
		.attr("width", 0)
		.attr("height", 0)
		;
	this.d3rect2 = this.d3bg.append("rect")
		.attr("id", "rect2" + this.uniqueString)
		.attr("x", 0)
		.attr("y", 0)
		//.attr("fill", d3.rgb(0,0,180))
		.attr("fill","url(#gradientBG)")
		.attr("fill-opacity", 1.0)
		.attr("width", 0)
		.attr("height", 0)
		;
	//
	this.addListeners();
	var gid = this.gid;
	var uniq = this.uniqueString;
	var w = this.width;
	var h = this.height;
	this.OkButton     = new Button  ("Ok"    , gid, uniq + "OkButton"    , null, 0, 0, 80, 20);
	this.CancelButton = new Button  ("Cancel", gid, uniq + "CancelButton", null, 0, 0, 80, 20);
	this.Textarea     = new Textarea(          gid, uniq + "Textarea"    , null, 0, 0,  w,  h);
	this.Textarea.registerResizeListener(this.uniqueString, this, this.handleResizeEvent);
	this.makeParentOf(this.OkButton, this.CancelButton);
	ResizeWindow();
}

InputPopup.prototype.show = function(bShow) {
	this.hide(false);
}

InputPopup.prototype.hide = function(bHide) {
	if (bHide == null || bHide) {
		this.d3g.style("visibility","hidden");
	} else {
		this.d3g.style("visibility","inherit");
	}
}

InputPopup.prototype.handleResizeEvent = function(sid) {
	var areaWidth = this.Textarea.getWidth();
	var areaHeight = this.Textarea.getHeight();
	this.d3rect2.attr("width", areaWidth + 10);
	this.d3rect2.attr("height", areaHeight + 31);
}

InputPopup.prototype.resizeWindow = function(w, h) {
	this.d3bg.style("width", convertToPx(w));
	this.d3bg.style("height", convertToPx(h));
	this.d3rect.attr("width", w);
	this.d3rect.attr("height", h);
	var areaWidth = this.Textarea.getWidth();
	var areaHeight = this.Textarea.getHeight();
	var x = (w - areaWidth) / 2;
	var y = (h - areaHeight) / 2;
	this.d3rect2.attr("x", x -  5);
	this.d3rect2.attr("y", y - 26);
	this.d3rect2.attr("width", areaWidth + 10);
	this.d3rect2.attr("height", areaHeight + 31);
	this.Textarea.setPosition(x, y);
	this.OkButton.setPosition(x + 83, y - 23);
	this.CancelButton.setPosition(x +  0, y - 23);
}

InputPopup.prototype.setText = function(val) {
	var input = this.getObject();
	input.textContent = val;
}

InputPopup.prototype.getText = function() {
	var input = this.getObject();
	var val = input.textContent;
	return val;
}

InputPopup.prototype.setValue = function(value) {
	this.Textarea.setValue(value);
}

InputPopup.prototype.onMouseDown = function(sid) {
	this.base.prototype.onMouseDown(sid);
}
InputPopup.prototype.onMouseUp = function(sid) {
	this.base.prototype.onMouseUp(sid);
}

InputPopup.prototype.onClick = function(sid) {
	switch (sid) {
		case this.OkButton.sid:
			var count = this.incr();
			var text = this.Textarea.getText();
			this.raiseEvent({"event": "ok", "count": count, "text": text});
			this.hide();
			break;
		case this.CancelButton.sid:
			this.hide();
			break;
	}
	this.base.prototype.onClick(sid);
}
