// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

//
// base class for all controls.
// so far only the Button and Textbox and Textarea (because it is subclassed from the Textbox)
//
function Control(sParentId, uniqueString, dataInFrontOf) {
	this.gid = "g" + uniqueString;
	this.sParentId = sParentId;
	this.uniqueString = uniqueString;
	this.sid = "m_" + uniqueString;
	this.dataInFrontOf = dataInFrontOf;
	this.create()
	var m_this = this;
	//
	var m_listeners = [];//{}; // registered functions to call when events are raised
	//
	function registerListener_private(m_uniqueString, m_functionParent, m_function) {
		if (m_uniqueString == null) {
			if (m_this.uniqueString) m_uniqueString = m_this.uniqueString;
			if (m_this.sid         ) m_uniqueString = m_this.sid;
		}
		m_listeners[m_uniqueString] = {"this": m_functionParent, "function": m_function};
	}
	function unregisterListener_private(m_uniqueString) {
		delete m_listeners[m_uniqueString];
	}
	function raiseEvent_private(e) {
		for (var id in m_listeners) {
			var listener = m_listeners[id];
			listener.function.call(listener.this, e);
		}
	}
	//
	this.registerListener = function(m_uniqueString, m_functionParent, m_function) {
		registerListener_private(m_uniqueString, m_functionParent, m_function);
	}
	this.unregisterListener = function(m_uniqueString) {
		unregisterListener_private(m_uniqueString);
	}
	this.raiseEvent = function(e) {
		raiseEvent_private(e);
	}
}

Control.prototype.create = function() {
	this.addListeners();
}

// make sure you call this function with the ".call(this,[])" format.
// notice no argument list. using "arguments" keyword as a variable arg list.
Control.prototype.makeParentOf = function() {
	for (var i = 0; i < arguments.length; i++) {
		arguments[i].parent = this;
	}
}

Control.prototype.hide = function(bHide) {
	var s = (bHide == null || bHide) ? "hidden" : "inherit";
	this.d3g.style("visibility", s);
}

Control.prototype.show = function(bShow) {
	var s = (bShow == null || bShow) ? "inherit" : "hidden";
	this.d3g.style("visibility", s);
}

Control.prototype.disabled = function() {
	var input = this.getObject();
	return input.disabled;
}

Control.prototype.enabled = function() {
	var input = this.getObject();
	return !input.disabled;
}

Control.prototype.toggleDisabled = function() {
	var input = this.getObject();
	this.disable(!input.disabled);
}

Control.prototype.disable = function(bDisabled) {
	var input = this.getObject();
	input.disabled = (bDisabled == null) ? true : bDisabled;
}

Control.prototype.move = function(dx, dy) {
	var p = this.getPosition();
	if (dx == null) dx = 0;
	if (dy == null) dy = 0;
	if (dx == 0 && dy == 0) return;
	p.x = p.x + dx;
	p.y = p.y + dy;
	this.setPosition(p)
}

Control.prototype.isVisible = function() {
	return !this.isHidden();
	//var visibility = this.d3g.style("visibility");
	//return !(visibility == "hidden");
}

Control.prototype.isHidden = function() {
	var visibility = this.d3g.style("visibility");
	return (visibility == "hidden");
}

// "Get" functions

Control.prototype.getLeft = function() {
	var val = this.d3g.style("left");
	return parseFloat(val);
}

Control.prototype.getTop = function() {
	var val = this.d3g.style("top");
	return parseFloat(val);
}

Control.prototype.getWidth = function() {
	var val = this.d3g.style("width");
	return parseFloat(val);
}

Control.prototype.getHeight = function() {
	var val = this.d3g.style("height");
	return parseFloat(val);
//	var list = this.getObject();
//	return list.clientHeight;
}

Control.prototype.getPosition = function() {
	var x = this.getLeft();
	var y = this.getTop();
	return {"x": x, "y": y};
}

Control.prototype.getRect = function() {
	var w = this.getWidth();
	var h = this.getHeight();
	return {"w": w, "h": h};
}

Control.prototype.getObject = function() {
	return document.getElementById(this.sid);
}

Control.prototype.getValue = function(def) {
	var obj = this.getObject();
	return obj.value ? obj.value : def;
}

// "Set" functions

Control.prototype.setValue = function(value, def) {
	var bMissing = (value == null);
	var obj = this.getObject();
	obj.value = bMissing ? def : value;
}

Control.prototype.setPosition = function(p) {
	this.setLeft(p.x);
	this.setTop(p.y);
}

Control.prototype.setLeft = function(x) {
	this.d3g.style("left", x);
}

Control.prototype.setTop = function(y) {
	this.d3g.style("top", y);
}

Control.prototype.focus = function() {
	var obj = this.getObject();
	obj.focus();
	var len = obj.value.length;
	if (obj.selectionStart)
		obj.selectionStart = 0;
	if (obj.selectionEnd)
		obj.selectionEnd = len;
}

// Events

Control.prototype.onMouseMove = function(sid) {
	if (this.parent && this.parent.onMouseMove) this.parent.onMouseMove(sid);
}
Control.prototype.onMouseOver = function(sid) {
	if (this.parent && this.parent.onMouseOver) this.parent.onMouseOver(sid);
}
Control.prototype.onMouseOut = function(sid) {
	if (this.parent && this.parent.onMouseOut) this.parent.onMouseOut(sid);
}
Control.prototype.onMouseUp = function(sid) {
	if (this.parent && this.parent.onMouseUp) this.parent.onMouseUp(sid);
}
Control.prototype.onMouseDown = function(sid) {
	if (this.parent && this.parent.onMouseDown) this.parent.onMouseDown(sid);
}
//Button.prototype.onMouseWheel = function(sid) {
//	if (this.parent && this.parent.onMouseWheel) this.parent.onMouseWheel(sid);
//}
Control.prototype.onKeyDown = function(sid) {
	if (this.parent && this.parent.onKeyDown) this.parent.onKeyDown(sid);
}
Control.prototype.onKeyPress = function(sid) {
	if (this.parent && this.parent.onKeyPress) this.parent.onKeyPress(sid);
}
Control.prototype.onKeyUp = function(sid) {
	if (this.parent && this.parent.onKeyUp) this.parent.onKeyUp(sid);
}
Control.prototype.onInput = function(sid) {
	if (this.parent && this.parent.onInput) this.parent.onInput(sid);
}
Control.prototype.onChange = function(sid) {
	if (this.parent && this.parent.onChange) this.parent.onChange(sid);
}
Control.prototype.onClick = function(sid) {
	if (this.parent && this.parent.onClick) this.parent.onClick(sid);
}
Control.prototype.onDblClick = function(sid) {
	if (this.parent && this.parent.onDblClick) this.parent.onDblClick(sid);
}
Control.prototype.onBlur = function(sid) {
	if (this.parent && this.parent.onBlur) this.parent.onBlur(sid);
}

Control.prototype.addListener = function(child) {
	this.addListenersForSelectionForId("#" + child.gid  , "#" + this.gid);
	this.addListenersForSelectionForId("#" + child.svgid, "#" + this.gid);
	this.addListenersForSelectionForId("#" + child.sid  , "#" + this.gid);
}

Control.prototype.addListeners = function() {
	this.addListenersForSelection("g"        )
	this.addListenersForSelection("svg"      )
	this.addListenersForSelection("rect"     )//svg
	this.addListenersForSelection("polygon"  )//svg
	this.addListenersForSelection("path"     )//svg
	this.addListenersForSelection("circle"   )//svg
	this.addListenersForSelection("ellipse"  )//svg
	this.addListenersForSelection("text"     )//svg
	this.addListenersForSelection("input"    )//form
	this.addListenersForSelection("select"   )//form
	this.addListenersForSelection("option"   )//form
	this.addListenersForSelection("button"   )//form
	this.addListenersForSelection("textarea" )//form
}

Control.prototype.addListenersForSelection = function(sSelector) {
	//this.addListenersForSelectionForId(sSelector, "#" + this.svgid);
	this.addListenersForSelectionForId(sSelector, "#" + this.gid);
}

Control.prototype.addListenersForSelectionForId = function(sSelector, sid) {
	var m_this = this;
	var d3g = d3.select(sid);
	var sel = d3g.selectAll(sSelector);
	sel
		.on("mousemove" , function() { 
			m_this.onMouseMove (this.id); })
		.on("mouseover" , function() { 
			m_this.onMouseOver (this.id); })
		.on("mouseout"  , function() { 
			m_this.onMouseOut  (this.id); })
		.on("mouseup"   , function() { 
			m_this.onMouseUp   (this.id); })
		.on("mousedown" , function() { 
			m_this.onMouseDown (this.id); })
		//.on("mousewheel", function() { 
		//	m_this.onMouseWheel(this.id); })
		.on("keydown"   , function() { 
			m_this.onKeyDown   (this.id); })
		.on("keypress"  , function() { 
			m_this.onKeyPress  (this.id); })
		.on("keyup"     , function() { 
			m_this.onKeyUp     (this.id); })
		.on("input"     , function() { 
			m_this.onInput     (this.id); })
		.on("change"    , function() { 
			m_this.onChange    (this.id); })
		.on("click"     , function() { 	
			m_this.onClick     (this.id); })
		.on("dblclick"  , function() { 
			m_this.onDblClick  (this.id); })
		.on("blur"  , function() { 
			m_this.onBlur      (this.id); })
		;
}
