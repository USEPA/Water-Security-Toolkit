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
function Textarea(sParentId, uniqueString, dataInFrontOf, left, top, width, height) {
	var m_resizeListeners = {}; // registered functions to call when resizing text area.
	//
	function registerResizeListener_private(m_uniqueString, m_this, m_function) {
		m_resizeListeners[m_uniqueString] = {"this": m_this, "function": m_function};
	}
	function unregisterResizeListener_private(m_uniqueString) {
		delete m_resizeListeners[m_uniqueString];
	}
	function raiseResizeEvent_private(sid) {
		for (id in m_resizeListeners) {
			var listener = m_resizeListeners[id];
			listener.function.call(listener.this, sid);
		}
	}
	//
	this.registerResizeListener = function(m_uniqueString, m_this, m_function) {
		registerResizeListener_private(m_uniqueString, m_this, m_function);
	}
	this.unregisterResizeListener = function(m_uniqueString) {
		unregisterResizeListener_private(m_uniqueString);
	}
	this.raiseResizeEvent = function(sid) {
		raiseResizeEvent_private(sid);
	}
	//
	this.bResizing = false;
	//
	this.base = Textbox;
	this.base(sParentId, uniqueString, dataInFrontOf, left, top, width, height); // this calls the base class "create" method which we override here.
}

Textarea.prototype = new Textbox;

Textarea.prototype.create = function() {
	var sel = d3.select("#" + this.gid);
	if (sel[0][0])
		sel.remove();
	this.d3parent = d3.select("#" + this.sParentId);
	this.d3g = this.d3parent.append("g")
		.attr("id",this.gid)
		.attr("data-InFrontOf",this.dataInFrontOf)
		.style("position","absolute")
		.style("left",convertToPx(this.left))
		.style("top",convertToPx(this.top))
		;
	this.d3text = this.d3g.append("textarea")
		.attr("id",this.sid)
		.attr("data-InFrontOf",this.dataInFrontOf)
		.style("width",convertToPx(this.width))
//		.style("height",convertToPx(this.height))
		;
	this.setWrap(false);

	for (var irow = 0; irow <= 1000; irow++) {
		this.d3text.attr("rows", irow);
		var h = this.getHeight();
		var obj = this.getObject();
		var h = obj.clientHeight;
		if (h >= this.height) {
			this.setHeight(this.height);
			break;
		}
	}
	
	this.addListeners();
}

Textarea.prototype.stopScroll = function(e) {
	e.cancelBubble = true;
	return;
	var delta = getWheelDelta(e);
	var deltaX = getWheelDeltaX(e);
	var obj = this.getObject();
	var bTop = (obj.scrollTop == 0);	
	var bBottom = (obj.scrollHeight - obj.scrollTop == obj.clientHeight);
	var bLeft = (obj.scrollLeft == 0);
	var horiScrollDiff = obj.scrollWidth - obj.scrollLeft;
	var bRight = (horiScrollDiff == obj.clientWidth);
	var bRight1 = (horiScrollDiff - 1 == obj.clientWidth);
	if (bTop && delta > 0) {
		e.wheelDeltaY = 0;
		if (deltaX == 0) e.preventDefault();//return cancelEvent(e);
	}
	if (bBottom && delta < 0) {
		e.wheelDeltaY = 0;
		if (deltaX == 0) e.preventDefault();//return cancelEvent(e);
	}
	if (bLeft && deltaX > 0) {
		e.wheelDeltaX = 0; 
		if (delta == 0) e.preventDefault();//return cancelEvent(e);
	}
	if (bRight && deltaX < 0) {
		e.wheelDeltaX = 0;
		if (delta == 0) e.preventDefault();//return cancelEvent(e);
	}
	if (bRight1 && deltaX < 0) {
		e.wheelDeltaX = 0; 
		if (delta == 0) e.preventDefault();//return cancelEvent(e);
	}
	return;
}

Textarea.prototype.onMouseMove = function(sid) {
	var x = this.getLeft();
	var y = this.getTop();
	var w = this.getWidth();
	var h = this.getHeight();
	x = d3.event.clientX - x;
	y = d3.event.clientY - y;	
	this.bResizing = ((x > 0.8 * w) || (y > 0.8 * h));
	if (this.bResizing) this.raiseResizeEvent(this.sid);
	this.base.prototype.onMouseMove(sid);
}

Textarea.prototype.getLineCount = function() {
	var s = this.getValue();
	if (s == null) return 0;
	var a = s.split("\n");
	return a.length;
}

Textarea.prototype.setRect = function(w, h) {
	this.setWidth(w);
	this.setHeight(h);
}

Textarea.prototype.setWrap = function(bVal) {
	if (bVal) {
		this.d3text.attr("wrap", "on")
	} else {
		this.d3text.attr("wrap", "off")
	}
}

Textarea.prototype.setResize = function(bVal) {
	if (bVal) {
		this.d3text.style("resize", null);
	} else {
		this.d3text.style("resize", "none");
	}
}

Textarea.prototype.setScroll = function(bVal) {
	if (bVal) {
		this.d3text.style("overflow", "auto");
		this.d3text.style("overflow-x", "auto");
		this.d3text.style("overflow-y", "auto");
	} else {
		this.d3text.style("overflow", "hidden");
		this.d3text.style("overflow-x", "hidden");
		this.d3text.style("overflow-y", "hidden");
	}
}