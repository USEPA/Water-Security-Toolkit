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
function SvgButton(sText, parent, uniqueString, dataInFrontOf, left, top, width, height, rx, ry) {
	this.base = Control;
	this.base(parent.gid, uniqueString, dataInFrontOf);
	//
	var m_this = this;
	if (rx == null) rx = 0;
	if (ry == null) ry = rx;
	//
	// Private properties
	//
	//var m_parent = parent;
	var m_left = left
	var m_top = top
	var m_width = width
	var m_height = height
	var m_rx = rx;
	var m_ry = ry;
	var m_disabled = false;
	var m_toggle = false;
	//
	// Private functions
	//
	//function setParent_private(value) { m_parent = value;}
	function setLeft_private    (value) { m_left     = value; m_this.resize();}
	function setTop_private     (value) { m_top      = value; m_this.resize();}
	function setWidth_private   (value) { m_width    = value; m_this.resize();}
	function setHeight_private  (value) { m_height   = value; m_this.resize();}
	function setRx_private      (value) { m_rx       = value; m_this.resize();}
	function setRy_private      (value) { m_ry       = value; m_this.resize();}
	function setDisabled_private(value) { m_disabled = value; }
	function setToggle_private  (value) { m_toggle   = value; }
	//
	//function getParent_private() { return m_parent; }
	function getLeft_private    () { return m_left    ; }
	function getTop_private     () { return m_top     ; }
	function getWidth_private   () { return m_width   ; }
	function getHeight_private  () { return m_height  ; }
	function getRx_private      () { return m_rx      ; }
	function getRy_private      () { return m_ry      ; }
	function getDisabled_private() { return m_disabled; }
	function getToggle_private  () { return m_toggle  ; }
	//
	// Privileged functions
	//
	//this.setParent = function(value) { setParent_private(value); }
	this.setLeft     = function(value) { setLeft_private    (value); }
	this.setTop      = function(value) { setTop_private     (value); }
	this.setWidth    = function(value) { setWidth_private   (value); }
	this.setHeight   = function(value) { setHeight_private  (value); }
	this.setRx       = function(value) { setRx_private      (value); }
	this.setRy       = function(value) { setRy_private      (value); }
	this.setToggle   = function(value) { setToggle_private  (value); }
	//
	this.bHover = false;
	this.toggle = function(toggle) {
		if (toggle == null)
			toggle = !this.getToggle();
		this.setToggle(toggle);
		if (toggle) {
			this.d3rect.attr("fill", this.GradientHover.fill);
			//this.d3cover.attr("cursor", "auto");
		} else {
			this.d3rect.attr("fill", this.fillBg);
			this.d3rect.attr("fill-opacity", 0.5);
			this.d3cover.attr("cursor", "pointer");
		}
	}
	this.enable = function(value) {
		var bEnabled = (value == null) ? true : value;
		this.disable(!bEnabled);
	}
	this.disable = function(value) {
		var bDisabled = (value == null) ? true : value;
		setDisabled_private(bDisabled);
		if (bDisabled) {
			this.d3rect.attr("fill-opacity", 0.5);
			this.d3rect.attr("fill", this.fillBgDisabled);
			this.d3text.attr("fill", this.fillTextDisabled);
			this.d3cover.attr("cursor", "auto");
		} else {
			if (!this.bHover)
				this.d3rect.attr("fill", this.fillBg);
			this.d3text.attr("fill", this.fillText);
			this.d3cover.attr("cursor", "pointer");
		}
	}
	//
	//this.getParent = function() { return getParent_private(); }
	this.getLeft    = function() { return getLeft_private    (); }
	this.getTop     = function() { return getTop_private     (); }
	this.getWidth   = function() { return getWidth_private   (); }
	this.getHeight  = function() { return getHeight_private  (); }
	this.getRx      = function() { return getRx_private      (); }
	this.getRy      = function() { return getRy_private      (); }
	//
	this.isDisabled  = function() { return getDisabled_private(); }
	this.Disabled    = function() { return getDisabled_private(); }
	this.disabled    = function() { return getDisabled_private(); }
	this.getDisabled = function() { return getDisabled_private(); }
	this.getToggle   = function() { return getToggle_private  (); }
	//
	this.svgid   = "svg"   + uniqueString;
	this.rectid  = "rect"  + uniqueString;
	this.textid  = "text"  + uniqueString;
	this.coverid = "cover" + uniqueString;
	//
	this.parent = parent;
	this.d3parent = d3.select("#" + parent.gid);
	//
	this.fillBg = SvgButton.DEFAULT_FILLBG;
	this.fillBgDisabled = SvgButton.DEFAULT_FILLBG_DISABLED;
	this.fillText = SvgButton.DEFAULT_FILLTEXT;
	this.fillTextDisabled = SvgButton.DEFAULT_FILLTEXT_DISABLED;
	//
	this.create(sText);
}

SvgButton.DEFAULT_FILLBG = d3.rgb(0,0,0);
SvgButton.DEFAULT_FILLBG_DISABLED = d3.rgb(30,30,30);
SvgButton.DEFAULT_FILLTEXT = d3.rgb(255,255,255);
SvgButton.DEFAULT_FILLTEXT_DISABLED = d3.rgb(150,150,150);

SvgButton.prototype = new Control;

SvgButton.prototype.create = function(sText) {
	if (sText == null) return; // this prevents the base class call from actually happening.
	this.d3g = 
	this.d3parent.append("g").attr("id", this.gid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.style("position","absolute")
		;
	this.d3svg =
	this.d3g.append("svg").attr("id", this.svgid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.style("position","absolute")
		.style("left", convertToPx(0))
		.style("top", convertToPx(0))
		;
	//
	var getcolor = d3.interpolate(d3.rgb(65,105,225), d3.rgb(100,100,100));
	var getcolor = d3.interpolate(d3.rgb(99,184,255), d3.rgb(100,100,100));
	var getcolor = d3.interpolate(d3.rgb(95,108,128), d3.rgb(100,100,100));
	this.GradientDown = new Gradient("linearGradient_" + this.uniqueString + "_GradientDown", this.svgid, 0, 0, 0, 100);
	this.GradientDown.addStop(  0, getcolor(0.0), 1.0);
	this.GradientDown.addStop( 40, getcolor(0.4), 1.0);
	this.GradientDown.addStop( 60, getcolor(0.6), 1.0);
	this.GradientDown.addStop(100, getcolor(1.0), 1.0);
	this.GradientDown.create();
	//
	var getcolor = d3.interpolate(d3.rgb(25, 25,112), d3.rgb(60,60,60));
	var getcolor = d3.interpolate(d3.rgb(24,116,205), d3.rgb(60,60,60));
	this.GradientHover = new Gradient("linearGradient_" + this.uniqueString + "_GradientHover", this.svgid, 0, 0, 0, 100);
	this.GradientHover.addStop(  0, getcolor(0.0), 1.0);
	this.GradientHover.addStop( 40, getcolor(0.4), 1.0);
	this.GradientHover.addStop( 60, getcolor(0.6), 1.0);
	this.GradientHover.addStop(100, getcolor(1.0), 1.0);
	this.GradientHover.create();
	//
	this.d3rect =
	this.d3svg.append("rect").attr("id", this.rectid)
		.attr("class", this.uniqueString)
		.attr("left", 0)
		.attr("top", 0)
		.attr("fill", this.fillBg)
		.attr("fill-opacity", 0.5)
		;
	this.d3text = 
	this.d3svg.append("text").attr("id", this.textid)
		.attr("class", this.uniqueString)
		.attr("fill", this.fillText)
		.attr("fill-opacity", 1.0)
		.text(sText)
		;
	this.d3cover = 
	this.d3svg.append("rect").attr("id", this.coverid)
		.attr("class", this.uniqueString)
		.attr("left", 0)
		.attr("top", 0)
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	this.sid = this.d3cover.attr("id");
	this.resize();
	this.addListeners();
}

SvgButton.prototype.resize = function(_w, _h) {
	if (_w != null) this.setWidth(_w);
	if (_h != null) this.setHeight(_h);
	var l = this.getLeft();
	var t = this.getTop();
	var w = this.getWidth();
	var h = this.getHeight();
	var rx = this.getRx();
	var ry = this.getRy();
	var box = this.d3text[0][0].getBBox();
	this.d3g
		.style("left",convertToPx(l))
		.style("top",convertToPx(t))
		;
	this.d3svg
		.style("width", convertToPx(w))
		.style("height", convertToPx(h))
		;
	this.d3rect
		.attr("width", w)
		.attr("height", h)
		.attr("rx", rx)
		.attr("ry", ry)
		;
	this.d3text
		.attr("x", w/2 - box.width/2)
		.attr("y", h/2 + box.height/4)
		;
	this.d3cover
		.attr("width", w)
		.attr("height", h)
		.attr("rx", rx)
		.attr("ry", ry)
		;
}

SvgButton.prototype.setText = function(value) {
	this.d3text.text(value);
	this.resize();
}

SvgButton.prototype.getText = function() {
	return this.d3text.text();
}

SvgButton.prototype.setTextColor = function(value) {
	this.fillText = value;
	if (this.disabled()) return;
	this.d3text.attr("fill", value);
}

SvgButton.prototype.getTextColor = function() {
	return this.d3text.attr("fill");
}

SvgButton.prototype.setPosition = function(p_or_x, null_or_y) {
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

SvgButton.prototype.setRect = function(p_or_w, null_or_h) {
	if (typeof(p_or_w) == "object" && typeof(null_or_h) == "undefined") {
		var p = p_or_w;
		this.setLeft(p.w);
		this.setTop(p.h);
	} else if ( typeof(p_or_w) == "number" && typeof(null_or_h) == "number") {
		var w = p_or_w;
		var h = null_or_h;
		this.setLeft(w);
		this.setTop(h);
	} else {
		alert("Error in Button.setRect(p_or_w, null_or_h)!");
	}
}

SvgButton.prototype.onMouseMove = function(sid) {
	if (sid == this.sid) {
	}
	Control.prototype.onMouseMove.call(this, sid); // call base class function
}

SvgButton.prototype.onMouseOver = function(sid) {
	if (sid == this.sid) {
		if (this.getToggle()) {
			this.d3cover.attr("cursor", "auto");
		} else if (this.isDisabled()) {
			this.d3rect.attr("fill", this.fillBgDisabled);
		} else {
			this.bHover = true;
			this.d3rect.attr("fill", this.GradientHover.fill);
			this.d3rect.attr("fill-opacity", 0.8);
		}
	}
	Control.prototype.onMouseOver.call(this, sid); // call base class function
}

SvgButton.prototype.onMouseOut = function(sid) {
	if (sid == this.sid) {
		this.bHover = false;
		if (this.getToggle()) {
			this.d3cover.attr("cursor", "auto");
		} else if (this.isDisabled()) {
			this.d3rect.attr("fill-opacity", 0.5);
		} else {
			this.d3rect.attr("fill", this.fillBg);
			this.d3rect.attr("fill-opacity", 0.5);
		}
	}
	Control.prototype.onMouseOut.call(this, sid); // call base class function
}

SvgButton.prototype.onMouseUp = function(sid) {
	if (sid == this.sid) {
		if (this.getToggle()) {
		} else if (this.isDisabled()) {
			this.d3rect.attr("fill-opacity", 0.5);
			this.d3rect.attr("fill", this.fillBgDisabled);
		} else {
			this.d3rect.attr("fill-opacity", 0.8);
			this.d3rect.attr("fill", this.GradientHover.fill);
		}
	}
	Control.prototype.onMouseUp.call(this, sid); // call base class function
}

SvgButton.prototype.onMouseDown = function(sid) {
	if (sid == this.sid) {
		if (this.getToggle()) {
		} else if (this.isDisabled()) {
			this.d3rect.attr("fill-opacity", 0.5);
			this.d3rect.attr("fill", this.fillBgDisabled);
		} else {
			this.d3rect.attr("fill-opacity", 0.8);
			this.d3rect.attr("fill", this.GradientDown.fill);
		}
	}
	Control.prototype.onMouseDown.call(this, sid); // call base class function
}

SvgButton.prototype.onClick = function(sid) {
	if (sid == this.sid) {
		if (this.getToggle()) return;
		if (this.isDisabled()) return;
	}
	Control.prototype.onClick.call(this, sid); // call base class function
}