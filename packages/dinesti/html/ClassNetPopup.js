// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function NetPopup(uniqueString, networkView, /*optional*/x, /*optional*/y, /*optional*/text) {
	this.uniqueString = uniqueString; // "NodePopup";
	this.parent = networkView;
	this.nid = networkView.nid
	this.x = (x == null) ? 0 : x;
	this.y = (y == null) ? 0 : y;
	this.sParentId = networkView.gid;
	this.gid = "g" + this.uniqueString + "_" + this.nid;
	this.svgid = "svg" + this.uniqueString + "_" + this.nid;
	this.polygonid = "polygon" + this.uniqueString + "_" + this.nid;
	this.coverid = "cover" + this.uniqueString + "_" + this.nid;
	this.bExists = true;
	this.sBorder = "rgba(0,0,0,0.4)";
	this.nBorderWidth = .5;
	this.sBgFill = "rgba(255,255,255,0.4)";
	this.sTextFill = "black"
	this.sPoints = "0,0";
	this.delta = {"x": 5, "y": -25};
	//
	this.create((text == null) ? "" : text);
	this.redraw(this.x, this.y, text);
}

NetPopup.prototype.create = function(/*optional*/sText) {
	if (sText == null) {
		sText = "";
		this.x = -10000;
		this.y = -10000;
	}
	//
	d3.select("#" + this.sParentId).selectAll("." + this.uniqueString + "_" + this.nid).remove();
	//
	this.d3g = d3.select("#" + this.sParentId)
		.append("g")
		.attr("id",this.gid)
		.attr("class",this.uniqueString + "_" + this.nid)
		.style("position","absolute")
		.style("left",convertToPx(this.x-2))
		.style("top",convertToPx(this.y-2))
		;
	this.d3svg = d3.select("#" + this.gid)
		.append("svg")
		.attr("id", this.svgid)
		.attr("class",this.uniqueString + "_" + this.nid)
		.attr("width",80)
		.attr("height",34)
		.attr("fill","rgba(0,0,0,0)")
		;
	this.d3polygon = d3.select("#" + this.svgid)
		.append("polygon")
		.attr("id", this.polygonid)
		.attr("class",this.uniqueString + "_" + this.nid)
		.attr("fill",this.sBgFill)
		.attr("stroke", this.sBorder)
		.attr("stroke-width", this.nBorderWidth)
		.attr("points", this.sPoints)
		;
	this.d3text = d3.select("#" + this.svgid)
		.append("text")
		.attr("id",this.uniqueString + "Text" + "_" + this.nid)
		.attr("class",this.uniqueString + "_" + this.nid)
		.attr("fill", this.sTextFill)
		.attr("x",5)
		.attr("x",9)
		.attr("y",16.5)
		.attr("y",20.5)
		.text(sText)
		;
	this.d3cover = d3.select("#" + this.svgid)
		.append("rect")
		.attr("id", this.coverid)
		.attr("class", this.uniqueString + "_" + this.nid)
		.attr("fill", "rgba(0,0,0,0.0)")
		.attr("x",8)
		.attr("y",8)
		.attr("width",20)
		.attr("height",16)
		;
	this.addListeners();
}

NetPopup.prototype.setText = function(text) {
	d3.select("#" + this.uniqueString + "Text" + "_" + this.nid).text(text);
	var obj = document.getElementById(this.uniqueString + "Text" + "_" + this.nid);
	var box = obj.getBBox();
	var w = box.width;
	this.sPoints = "6,6 " + (w+13) + ",6 " + (w+13) + ",26 8,26  0,29 6,24 6,6";
	d3.select("#" + this.polygonid).attr("points", this.sPoints);
	d3.select("#" + this.svgid).attr("width", w + 15)
	d3.select("#" + this.coverid).attr("width", w + 3);
}

NetPopup.prototype.moveX = function(newX) {
	this.x = newX;
	d3.select("#" + this.gid).style("left",convertToPx(this.x-6));
}

NetPopup.prototype.moveY = function(newY) {
	this.y = newY;
	d3.select("#" + this.gid).style("top",convertToPx(this.y-6));
}

NetPopup.prototype.move = function(newX, newY, /*optional*/ bTransition) {
	this.x = newX;
	this.y = newY;
	if (bTransition = null || !bTransition) {
		this.moveX(newX);
		this.moveY(newY);
	} else {
		d3.select("#" + this.gid)
			.transition()
			.duration(500)
			.style("left",convertToPx(this.x-6))
			.style("top",convertToPx(this.y-6))
			;
	}
}

NetPopup.prototype.redraw = function(x, y, text, /*Optional*/ sType, /*optional*/ bShow, /**/ bTransition) {
	sType = (sType == null) ? "" : sType;
	bShow = (bShow == null) ? false : bShow;
	bTransition = (bTransition == null) ? false : bTransition;
	var newX = x + this.delta.x;
	var newY = y + this.delta.y;
	this.move(newX, newY, bTransition);
	this.setText(text);
	var sBgFill = this.sBgFill;
	var sFgFill = "black";
	var sCursor = "url(grey.cur),auto";
	switch (sType) {
		case "Tank":
			sBgFill = "rgba(000,000,150,0.4)";
			sFgFill = "rgba(255,255,255,1.0)";
			sCursor = "url(blue.cur),auto";
			break;
		case "Reservoir":
			sBgFill = "rgba(000,150,000,0.4)";
			sFgFill = "rgba(255,255,255,1.0)";
			sCursor = "url(green.cur),auto";
			break;
		case "Pump":
			sBgFill = "rgba(150,150,000,0.4)";
			sFgFill = "rgba(255,255,255,1.0)";
			sCursor = "url(yellow.cur),auto";
			break;
		case "Valve":
			sBgFill = "rgba(000,150,150,0.4)";
			sFgFill = "rgba(255,255,255,1.0)";
			sCursor = "url(cyan.cur),auto";
			break;
		case "ToolTip":
			sBgFill = "rgba(255,255,255,1.0)";
			sFgFill = "rgba(0,0,0,1.0)";
			break;
	}
	//this.d3polygon.attr("cursor", sCursor);
	this.d3polygon.attr("fill",   sBgFill);
	this.d3text   .attr("fill",   sFgFill);
	if (bShow) this.show();
}

NetPopup.prototype.hide = function(bHide) {
	if (bHide == null || bHide)
		d3.select("#" + this.gid).style("visibility","hidden");
	else
		d3.select("#" + this.gid).style("visibility","");
}

NetPopup.prototype.show = function(bShow) {
	if (bShow == null || bShow)
		d3.select("#" + this.gid).style("visibility","");
	else
		d3.select("#" + this.gid).style("visibility","hidden");
}

NetPopup.prototype.onMouseMove = function(sid) {
	this.hide();
	var obj = document.elementFromPoint(d3.event.clientX,d3.event.clientY);
	this.show();
	if (obj == null) return;
	if (obj.id.length > 0)
		if (this.parent)
			this.parent.onMouseMove(obj.id);
		else
			onMouseMove(obj.id);
	else
		console.log("NetPopup error!");
}
NetPopup.prototype.onMouseOver = function(sid) {
	this.hide();
	var obj = document.elementFromPoint(d3.event.clientX,d3.event.clientY);
	this.show();
	if (obj == null) return;
	if (obj.id.length > 0)
		if (this.parent)
			this.parent.onMouseOver(obj.id);
		else
			onMouseOver(obj.id);
	else
		console.log("NetPopup error!");
}
NetPopup.prototype.onMouseOut = function(sid) {
	this.hide();
	var obj = document.elementFromPoint(d3.event.clientX,d3.event.clientY);
	this.show();
	if (obj == null) return;
	if (obj.id.length > 0)
		if (this.parent)
			this.parent.onMouseOut(obj.id);
		else
			onMouseOut(obj.id);
	else
		console.log("NetPopup error!");
}
NetPopup.prototype.onMouseUp = function(sid) {
	this.hide();
	var obj = document.elementFromPoint(d3.event.clientX,d3.event.clientY);
	this.show();
	if (obj == null) return;
	if (obj.id.length > 0)
		if (this.parent)
			this.parent.onMouseUp(obj.id);
		else
			onMouseUp(obj.id);
	else
		console.log("NetPopup error!");
}
NetPopup.prototype.onMouseDown = function(sid) {
	this.hide();
	var obj = document.elementFromPoint(d3.event.clientX,d3.event.clientY);
	this.show();
	if (obj == null) return;
	if (obj.id.length > 0)
		if (this.parent)
			this.parent.onMouseDown(obj.id);
		else
			onMouseDown(obj.id);
	else
		console.log("NetPopup error!");
}
NetPopup.prototype.onKeyDown = function(sid) {console.log("kdown="+sid);}
NetPopup.prototype.onKeyPress = function(sid) {console.log("press="+sid);}
NetPopup.prototype.onKeyUp = function(sid) {console.log("kup="+sid);}
NetPopup.prototype.onInput = function(sid) {console.log("input="+sid);}
NetPopup.prototype.onChange = function(sid) {console.log("change="+sid);}
NetPopup.prototype.onClick = function(sid) {
	this.hide();
	var obj = document.elementFromPoint(d3.event.clientX,d3.event.clientY);
	this.show();
	if (obj == null) return;
	if (this.parent)
		this.parent.onClick(obj.id);
	else
		onClick(obj.id);
}
NetPopup.prototype.onDblClick = function(sid) {
	this.hide();
	var obj = document.elementFromPoint(d3.event.clientX,d3.event.clientY);
	this.show();
	if (obj == null) return;
	if (this.parent)
		this.parent.onDblClick(obj.id);
	else
		onDblClick(obj.id);
}

NetPopup.prototype.addListeners = function() {
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

NetPopup.prototype.addListenersForSelection = function(sSelector) {
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