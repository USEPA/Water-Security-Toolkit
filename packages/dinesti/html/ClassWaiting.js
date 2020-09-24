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
function Waiting(sParentId) {
	this.sParentId = sParentId;
	var uniqueString = "Waiting";
	this.uniqueString = uniqueString;
	this.gid = "g" + uniqueString;
	this.gbg = "g" + uniqueString + "Bg";
	this.svgbg = "svg" + uniqueString + "Bg";
	this.rectbg = "rect" + uniqueString + "Bg";
	this.gtext = "g" + uniqueString + "Text";
	this.pieid = "pie" + uniqueString;
	this.textWidth = 160;
	this.textHeight = 80;
	GlobalData.resizeHooks[uniqueString] = {"this": this, "function": this.resizeWindow};
	var m_countTotal = -1;
	var m_count = 0;
	//
	function setCountTotal_private(val) {
		m_count = 0;
		m_countTotal = val;
	}
	function getCountTotal_private() {
		return m_countTotal;
	}
	this.setCountTotal = function(val) {
		setCountTotal_private(val);
	}
	this.getCountTotal = function() {
		return getCountTotal_private();
	}
	//
	function setCount_private(val) {
		m_count = val;
		return m_count;
	}
	function getCount_private() {
		return m_count;
	}
	this.setCount = function(val) {
		var count = setCount_private(val);
		var total = this.getCountTotal();
		//if (count >= total) this.hide();
		return count;
	}
	this.getCount = function() {
		return getCount_private();
	}
	//
	function getPercent_private() {
		if (m_countTotal > 0 && m_count <= m_countTotal) return m_count / m_countTotal;
		if (m_count > m_countTotal) return 100;
		return 0;
	}
	this.getPercent = function() {
		return getPercent_private();
	}
	//
	var m_TimerStart = 0;
	//
	function setTimerStart_private(val) {
		m_TimerStart = val;
		return m_TimerStart;
	}
	function getTimerStart_private() {
		return m_TimerStart;
	}
	this.setTimerStart = function() {
		var m_this = this;
		var start = (new Date).getTime();
		setTimerStart_private(start);
		var pid = setInterval(function() {
		//
			var now   = (new Date).getTime();
			var diff  = now - start;
			var hours = parseInt(diff / 60 /	60 / 1000);
			var mins  = parseInt((diff - hours * 60 * 60 * 1000) / 60 / 1000);
			var secs  = parseInt((diff - hours * 60 * 60 * 1000 - mins * 60 * 1000) / 1000);
			hours = ("" + hours).lpad("0", 2);
			mins  = ("" + mins ).lpad("0", 2);
			secs  = ("" + secs ).lpad("0", 2);
			var text = hours + ":" + mins + ":" + secs;
			m_this.d3timer.text(text);
		//
		}, 1000);
		setTimerPid_private(pid);
	}
	this.getTimerStart = function() {
		return getTimerStart_private();
	}
	//
	var m_TimerPid = 0;
	//
	function setTimerPid_private(val) {
		m_TimerPid = val;
		return m_TimerPid;
	}
	function getTimerPid_private() {
		return m_TimerPid;
	}
	this.setTimerPid = function(val) {
		return setTimerPid_private(val);
	}
	this.getTimerPid = function() {
		return getTimerPid_private();
	}
	//
	this.create();
}

Waiting.prototype.create = function() {
	this.d3g = d3.select("#" + this.sParentId)
		.append("g").attr("id", this.gid)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top",  convertToPx(0))
		.style("visibility", "hidden")
		;
	this.d3gbg = this.d3g
		.append("g").attr("id", this.gbg)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top",  convertToPx(0))
		;
	this.d3svgbg = this.d3gbg
		.append("svg").attr("id", this.svgbg)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top" , convertToPx(0))
		;
	this.d3rectbg = this.d3svgbg
		.append("rect").attr("id", this.rectbg)
		.attr("x", 0)
		.attr("y", 0)
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.5)
//		.attr("cursor","wait")
		;
	this.d3gtext = this.d3g
		.append("g").attr("id", this.gtext)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top" , convertToPx(0))
		.style("width" , convertToPx(this.textWidth ))
		.style("height", convertToPx(this.textHeight))
		;
	this.d3svgtext = this.d3gtext
		.append("svg").attr("id", "svg" + this.uniqueString + "Loading")
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top" , convertToPx(0))
		.style("width" , convertToPx(this.textWidth ))
		.style("height", convertToPx(this.textHeight))
		;
	this.d3rect = this.d3svgtext
		.append("rect").attr("id", "rect" + this.uniqueString + "Loading")
		.attr("x",0)
		.attr("y",0)
		.attr("width",  this.textWidth)
		.attr("height", this.textHeight)
		.attr("rx", 20)
		.attr("ry", 20)
		.attr("fill", d3.rgb( 0, 0, 0))
		.attr("fill-opacity", 0.55)
		;
	this.d3text = this.d3svgtext
		.append("text").attr("id", "text" + this.uniqueString + "Loading")
		.attr("x", this.textWidth /2 + 2)
		.attr("y", this.textHeight/2 + 8)
		.attr("text-anchor", "middle")
		.style("font-size", convertToPx(30))
		.attr("fill", d3.rgb(250,250,250))
		.attr("fill-opacity", 0.65)
		.text("Loading...")
		;
	this.d3timer = this.d3svgtext
		.append("text").attr("id", "text" + this.uniqueString + "Timer")
		.attr("x", this.textWidth /2 + 2)
		.attr("y", this.textHeight/2 + 8 + 25)
		.attr("text-anchor", "middle")
		.style("font-size", convertToPx(10))
		.attr("fill", d3.rgb(250,250,250))
		.attr("fill-opacity", 0.65)
		.text("00:00:00")
		;
	this.d3cover = this.d3svgtext
		.append("rect").attr("id", "rect" + this.uniqueString + "LoadingCover")
		.attr("x", 0)
		.attr("y", 0)
		.attr("width",  this.textWidth)
		.attr("height", this.textHeight)
		.attr("rx", 20)
		.attr("ry", 20)
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.0)
		;
	this.d3pie = this.d3svgtext.append("path").attr("id", this.pieid)
		.attr("d", this.getArcPath())
		.attr("fill", "black")
		;
}

Waiting.prototype.resizeWindow = function(nWidth,nHeight) {
	this.d3gbg
		.style("width" , convertToPx(nWidth ))
		.style("height", convertToPx(nHeight))
		;
	this.d3svgbg
		.style("width" , convertToPx(nWidth ))
		.style("height", convertToPx(nHeight))
		;
	this.d3rectbg
		.attr("width" , nWidth )
		.attr("height", nHeight)
		;
	this.d3gtext
		.style("left", convertToPx(nWidth /2 - this.textWidth /2))
		.style("top" , convertToPx(nHeight/2 - this.textHeight/2))
		;
}

Waiting.prototype.getArcPath = function(percent) {
	//if (percent == null) percent = 90;
	//var angle = percent / 100 * 360;
	var angle = percent * 360;
	var rad = angle * Math.PI / 180;
	var r = 40;
	var x = 0;
	var y = 0;
	var s = "0 0 1";
	if (angle < 0) {
	} else if (angle <= 90) {
		x = 0 + r * Math.sin(rad);
		y = r - r * Math.cos(rad);
		s = "0 0 1";
	} else if (angle <= 180) {
		x = 0 + r * Math.cos(rad - 1/2*Math.PI);
		y = r + r * Math.sin(rad - 1/2*Math.PI);
		s = "0 0 1";
	} else if (angle <= 270) {
		x = 0 - r * Math.sin(rad - 2/2*Math.PI);
		y = r + r * Math.cos(rad - 2/2*Math.PI);
		s = "0 1 1";
	} else if (angle <= 360) {
		x = 0 - r * Math.cos(rad - 3/2*Math.PI);
		y = r - r * Math.sin(rad - 3/2*Math.PI);
		s = "0 1 1";
	}
	var path = "M 80 40 l 0 " + (-1*r) + " a " + r + " " + r + " " + s + " " + x + " " + y;
	return path;
}

Waiting.prototype.incrCount = function() {
	var count = this.getCount();
	return this.setCount(++count);
}

Waiting.prototype.update = function() {
	this.incrCount();
	var percent = this.getPercent();
	this.d3pie.attr("d", this.getArcPath(percent));
//	this.d3text.text(Waiting.count);
}

Waiting.prototype.show = function(text) {
	//console.log("Waiting.show");
	clearInterval(this.getTimerPid());
	this.d3timer.text("00:00:00");
	this.setTimerStart();
	//
	if (text == null) text = "Loading..."
	this.d3text.text(text);
	this.d3g.style("visibility","inherit");
	var text = this.d3text[0][0];
	var box = text.getBBox();
	var w = box.width;
	this.textWidth = w + 28;
	//this.d3gtext.style("width", convertToPx(this.textWidth));
	this.d3svgtext.style("width", convertToPx(this.textWidth));
	this.d3rect.attr("width",  this.textWidth);
	this.d3text.attr("x", this.textWidth / 2 + 2);
	this.d3timer.attr("x", this.textWidth / 2 + 2);
	this.d3cover.attr("width",  this.textWidth);
	ResizeWindow();
	this.update();
}

Waiting.prototype.hide = function() {
	//console.log("Waiting.hide");
	clearInterval(this.getTimerPid());
	this.d3g.style("visibility","hidden");
}

Waiting.prototype.isVisible = function() {
	var sVisible = this.d3g.style("visibility");
	return !(sVisible == "hidden");
}

