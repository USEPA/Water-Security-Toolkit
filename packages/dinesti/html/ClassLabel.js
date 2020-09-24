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
function Label(sText, sParentId, uniqueString, dataInFrontOf, left, top, width, height) {
	this.sParentId = sParentId;
	this.uniqueString = uniqueString;
	this.sid = "m_" + uniqueString;
	this.dataInFrontOf = dataInFrontOf;
	this.left = left;
	this.top = top;
	this.width = width;
	this.height = height;
	this.colorText = Label.DEFAULT_COLOR_TEXT;
	this.colorTextDisabled = Label.DEFAULT_COLOR_TEXT_DISABLED;
	this.create(sText);
}

Label.DEFAULT_COLOR_TEXT = "rgba(0,0,0,1.0)";
Label.DEFAULT_COLOR_TEXT_DISABLED = "rgba(60,60,60,0.5)";

Label.prototype.create = function(sText) {
	this.d3parent = d3.select("#" + this.sParentId);
	this.d3text = this.d3parent.append("text")
		.attr("id", this.sid)
		.attr("class", "input_label")
		.attr("data-InFrontOf", this.dataInFrontOf)
		.style("width", convertToPx(this.width))
		.attr("x", this.left)
		.attr("y", this.top)
		.text(sText)
		;
	this.d3rect = this.d3parent.append("rect")
		.attr("id", this.sid + "_cover")
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("x", this.left)
		.attr("y", this.top)
		.attr("width", this.width)
		.attr("height", this.height)
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.0)
		;
	this.addListeners();
}

Label.prototype.update = function() {
	var text = this.getObject();
	var box = text.getBBox();
	this.d3text.attr("y", this.top + box.height);
}

Label.prototype.update2 = function() {
	var text = this.getObject();
	var box = text.getBBox();
	this.d3rect.attr("y", this.top - box.height);
}

Label.prototype.setLeft = function(val) {
	this.left = val;
	this.d3text.attr("x", val);
	this.d3rect.attr("x", val);
}

Label.prototype.setTop = function(val) {
	this.top = val;
	this.d3text.attr("y", val);
	this.d3rect.attr("y", val);
}

Label.prototype.setText = function(val, def) {
	if (def == null) def = "";
	var input = this.getObject();
	//input.value = val; // this didnt seem to work
	if (val == null)
		input.textContent = def;
	else
		input.textContent = val;
}

Label.prototype.getText = function() {
	var input = this.getObject();
	var val = input.value;
	return val;
}

Label.prototype.getObject = function() {
	return document.getElementById(this.sid);
}

Label.prototype.disable = function(bDisabled) {
	if (bDisabled == null || bDisabled) {
		this.d3text.attr("fill", this.colorTextDisabled);
	} else {
		this.d3text.attr("fill", this.colorText);		
	}
}

Label.prototype.enlarge = function(z) {
	var n = this.d3text.style("font-size");
	var n = parseInt(n);
	//console.log(n);
	this.d3text.style("font-size", convertToPx(n * z));
	var n = this.d3text.style("font-size");
	//console.log(n);	
}

Label.prototype.hide = function(bHide) {
	if (bHide == null) bHide = true;
	if (bHide) {
		this.d3text.style("visibility", "hidden");
		this.d3rect.style("visibility", "hidden");
	} else {
		this.d3text.style("visibility", "inherit");
		this.d3rect.style("visibility", "inherit");
	}
}

Label.prototype.show = function(bShow) {
	this.hide(!bShow);
}
//

Label.prototype.onMouseMove = function(sid) {;}
Label.prototype.onMouseOver = function(sid) {;}
Label.prototype.onMouseOut = function(sid) {;}
Label.prototype.onMouseUp = function(sid) {;}
Label.prototype.onMouseDown = function(sid) {;}
//Label.prototype.onMouseWheel = function(sid) {;}
Label.prototype.onKeyDown = function(sid) {;}
Label.prototype.onKeyPress = function(sid) {;}
Label.prototype.onKeyUp = function(sid) {;}
Label.prototype.onInput = function(sid) {;}
Label.prototype.onChange = function(sid) {;}
Label.prototype.onClick = function(sid) {;}
Label.prototype.onDblClick = function(sid) {;}

//

Label.prototype.addListeners = function() {
	this.addListenersForSelection("g"      )
	this.addListenersForSelection("svg"    )
	this.addListenersForSelection("rect"   )//svg
	this.addListenersForSelection("polygon")//svg
	this.addListenersForSelection("path"   )//svg
	this.addListenersForSelection("circle" )//svg
	this.addListenersForSelection("text"   )//svg
	this.addListenersForSelection("input"  )//form
	this.addListenersForSelection("select" )//form
	this.addListenersForSelection("option" )//form
	this.addListenersForSelection("button" )//form
}

Label.prototype.addListenersForSelection = function(sSelector) {
	var m_this = this;
	var d3g = d3.select("#" + this.gid);
	var sel = d3g.selectAll(sSelector)
		.on("mousemove" , function() { m_this.onMouseMove (this.id); })
		.on("mouseover" , function() { m_this.onMouseOver (this.id); })
		.on("mouseout"  , function() { m_this.onMouseOut  (this.id); })
		.on("mouseup"   , function() { m_this.onMouseUp   (this.id); })
		.on("mousedown" , function() { m_this.onMouseDown (this.id); })
		//.on("mousewheel", function() { m_this.onMouseWheel(this.id); })
		.on("keydown"   , function() { m_this.onKeyDown   (this.id); })
		.on("keypress"  , function() { m_this.onKeyPress  (this.id); })
		.on("keyup"     , function() { m_this.onKeyUp     (this.id); })
		.on("input"     , function() { m_this.onInput     (this.id); })
		.on("change"    , function() { m_this.onChange    (this.id); })
		.on("click"     , function() { m_this.onClick     (this.id); })
		.on("dblclick"  , function() { m_this.onDblClick  (this.id); })
		;
}
