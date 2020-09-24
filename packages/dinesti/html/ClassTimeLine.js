// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function TimeLine(parent, sParentId) {
	var uniqueString = "TimeLine1";
	this.uniqueString = uniqueString;
	this.parent = parent; // must be NodeGraph object
	this.sParentId = sParentId;
	this.gid = sParentId;
	this.x = -1000;
	this.sLineColor = NodeGraph.DEFAULT_AXIS_COLOR;
	this.nLineWidth = NodeGraph.DEFAULT_AXIS_WIDTH;
	this.nHandleSize = 10;
	this.nHandleExtend = 10;
	this.sHandleStroke = "rgba(0,0,0,0.0)"
	this.sHandleHoverOff = this.sLineColor;
	this.sHandleHoverOn = "rgba(0,0,0,0.4)";
	this.sHandleHoverOn = "rgba(0,0,0,0.8)";
	this.dragger = {"p":{"x":null,"y":null},"dragging":false};
	//
	this.create();
}

TimeLine.prototype.eventDragTimeLine = function(clientX) {
	if (this.parent.eventDragTimeLine)
		return this.parent.eventDragTimeLine(clientX);
	else
		alert("TimeLine.parent.eventDragTimeLine() not implemented yet!");
	return false;
}

TimeLine.prototype.setY1 = function(val) {
	d3.select("#" + this.uniqueString + "line").attr("y1",val);
}

TimeLine.prototype.getY1 = function() {
	return d3.select("#" + this.uniqueString + "line").attr("y1");
}

TimeLine.prototype.setY2 = function(val) {
	d3.select("#" + this.uniqueString + "line").attr("y2", val + -1*this.nHandleExtend);
	d3.select("#" + this.uniqueString + "circle").attr("cy",val + -1*this.nHandleExtend + -1*this.nHandleSize/2.0);
}

TimeLine.prototype.getY2 = function() {
	return d3.select("#" + this.uniqueString + "line").attr("y2");
}
TimeLine.prototype.create = function() {
	this.d3line = d3.select("#" + this.sParentId)
		.append("line")
		.attr("id",this.uniqueString + "line")
		.attr("stroke",this.sLineColor)
		.attr("stroke-width",this.nLineWidth)
		.attr("x1",-1000)
		.attr("y1",0)
		.attr("x2",-1000)
		.attr("y2",100)
		;
	this.d3circle = d3.select("#" + this.sParentId)
		.append("circle")
		.attr("id",this.uniqueString + "circle")
		.attr("fill",this.sHandleHoverOff)
		.attr("stroke",this.sHandleStroke)
		.attr("cx",-1000)
		.attr("cy",100)
		.attr("r",this.nHandleSize/2)
		//.attr("cursor","move")
		;
	this.hide();
}

TimeLine.prototype.hide = function() {
	this.move(-1000);
}

TimeLine.prototype.move = function(x) {
	this.x = x;
	this.d3line  .attr("x1",x);
	this.d3line  .attr("x2",x);
	this.d3circle.attr("cx",x);
}

TimeLine.prototype.addListeners = function() {
//  i cant do a blanket select on all svg types since this class doesnt have an all-incompasing "g" container yet
//	this.addListenersForSelection("g"      )
//	this.addListenersForSelection("svg"    )
//	this.addListenersForSelection("rect"   )//svg
//	this.addListenersForSelection("polygon")//svg
//	this.addListenersForSelection("path"   )//svg
//	this.addListenersForSelection("circle" )//svg
//	this.addListenersForSelection("text"   )//svg
//	this.addListenersForSelection("input"  )//form
//	this.addListenersForSelection("select" )//form
//	this.addListenersForSelection("option" )//form
//	this.addListenersForSelection("button" )//form
	this.addListenersForSelection("#" + this.uniqueString + "line")
	this.addListenersForSelection("#" + this.uniqueString + "circle")
}

TimeLine.prototype.stopDragging = function() {
	this.dragger.dragging = false;
	this.d3circle.attr("fill",this.sHandleHoverOff);
	this.x = parseFloat(this.d3circle.attr("cx"));
}

TimeLine.prototype.doDrag = function(x,y) {
	if (this.parent.nPlaying == NodeGraph.ANIMATION_STOPPED) {
		if (this.dragger.dragging) {
			var screenX = d3.event ? d3.event.screenX : 0;
			var screenY = d3.event ? d3.event.screenY : 0;
			x = x ? x : screenX;
			y = y ? y : screenY;
			var deltaX = x - this.dragger.p.x;
			var deltaY = y - this.dragger.p.y;
			var newX = this.x + deltaX;
			var retVal = this.eventDragTimeLine(newX);
			if (retVal.bLimited) {
				this.move(retVal.newX);
				this.x = newX;
			} else {
				this.move(newX);
			}
			var scaleX = (this.width  + deltaX) / this.width;
			var scaleY = (this.height - deltaY) / this.height;
			this.dragger.p.x = x;
			this.dragger.p.y = y;
		}
	}
}

TimeLine.prototype.enableMoveCursor = function() {
	this.d3circle.attr("cursor", "move");
}

TimeLine.prototype.disableMoveCursor = function() {
	this.d3circle.attr("cursor", null);
}

TimeLine.prototype.onMouseMove = function(sid) {
	this.doDrag();
}

TimeLine.prototype.onMouseOver = function(sid) {
	var sel = d3.select("#" + sid);
	switch(sid) {
		case this.uniqueString + "circle":
			if (this.parent.nPlaying == NodeGraph.ANIMATION_STOPPED) {
				sel.attr("fill",this.sHandleHoverOn);
			}
			break;
		default:
			break;
	}
}

TimeLine.prototype.onMouseOut = function(sid) {
	var sel = d3.select("#" + sid);
	switch(sid) {
		case this.uniqueString + "circle":
			if (!this.dragger.dragging)
				sel.attr("fill",this.sHandleHoverOff);
			break;
		default:
			break;
	}	
}

TimeLine.prototype.onMouseUp = function(sid) {
	if (this.dragger.dragging) this.stopDragging();
	var sel = d3.select("#" + sid);
	switch(sid) {
		case this.uniqueString + "circle":
			if (this.parent.nPlaying == NodeGraph.ANIMATION_STOPPED) {
				sel.attr("fill",this.sHandleHoverOn);
			}
			break;
		default:
			break;
	}	
}

TimeLine.prototype.onMouseDown = function(sid) {
	var sel = d3.select("#" + sid);
	switch(sid) {
		case this.uniqueString + "circle":
			if (this.parent.nPlaying == NodeGraph.ANIMATION_STOPPED) {
				sel.attr("fill",this.sHandleClickOn);
				this.dragger.dragging = true;				
				this.dragger.p.x = d3.event.screenX;
				this.dragger.p.y = d3.event.screenY;
			}
			break;
		default:
			break;
	}	
}

TimeLine.prototype.addListenersForSelection = function(sSelector) {
	var m_this = this;
	var d3g = d3.select("#" + this.gid);
	var sel = d3g.selectAll(sSelector);
	d3g.selectAll(sSelector)
		.on("mousemove" , function() { m_this.onMouseMove (this.id); })
		.on("mouseover" , function() { m_this.onMouseOver (this.id); })
		.on("mouseout"  , function() { m_this.onMouseOut  (this.id); })
		.on("mouseup"   , function() { m_this.onMouseUp   (this.id); })
		.on("mousedown" , function() { m_this.onMouseDown (this.id); })
//		.on("mousewheel", function() { m_this.onMouseWheel(this.id); })
//		.on("keydown"   , function() { m_this.onKeyDown   (this.id); })
//		.on("keypress"  , function() { m_this.onKeyPress  (this.id); })
//		.on("keyup"     , function() { m_this.onKeyUp     (this.id); })
//		.on("input"     , function() { m_this.onInput     (this.id); })
//		.on("change"    , function() { m_this.onChange    (this.id); })
//		.on("click"     , function() { m_this.onClick     (this.id); })
//		.on("dblclick"  , function() { m_this.onDblClick  (this.id); })
		;
}
