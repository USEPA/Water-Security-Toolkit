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
function Clock(sParentId) {
	this.sParentId = sParentId;
	var uniqueString = "Clock";
	this.uniqueString = uniqueString;
	this.gid = "g" + uniqueString;
	this.gbg = "g" + uniqueString + "Bg";
	this.svgbg = "svg" + uniqueString + "Bg";
	this.rectbg = "rect" + uniqueString + "Bg";
	this.gtext = "g" + uniqueString + "Text";
	this.pieid = "pie" + uniqueString;
	this.textWidth = 220;
	this.textHeight = 20;
	this.nProcess = null;
	this.nInterval = 1000;
	GlobalData.resizeHooks[uniqueString] = {"this": this, "function": this.resizeWindow};
	var m_time = 0;
	//
	function setTime_private(val) {
		m_time = val;
	}
	function getTime_private() {
		return m_time;
	}
	this.setTime = function(val) {
		setTime_private(val);
	}
	this.getTime = function() {
		return getTime_private();
	}
	//
	this.create();
}

Clock.prototype.create = function() {
	this.d3g = d3.select("#" + this.sParentId)
		.append("g").attr("id", this.gid)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top",  convertToPx(0))
		.style("visibility", "hidden")
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
		.attr("rx", 4)
		.attr("ry", 4)
		.attr("fill", d3.rgb( 0, 0, 0))
		.attr("fill-opacity", 0.75)
		;
	this.d3text = this.d3svgtext
		.append("text").attr("id", "text" + this.uniqueString + "Loading")
		.attr("x", this.textWidth /2 + 2)
		.attr("y", this.textHeight/2 + 4)
		.attr("text-anchor", "middle")
		.style("font-size", convertToPx(14))
		.attr("fill", d3.rgb(250,250,250))
		.attr("fill-opacity", 0.85)
		.text("")
		;
	this.d3cover = this.d3svgtext
		.append("rect").attr("id", "rect" + this.uniqueString + "LoadingCover")
		.attr("x", 0)
		.attr("y", 0)
		.attr("width",  this.textWidth)
		.attr("height", this.textHeight)
		.attr("rx", 4)
		.attr("ry", 4)
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.0)
		;
}

Clock.prototype.resizeWindow = function(nWidth, nHeight) {
	this.d3gtext
		.style("left", convertToPx(nWidth - this.textWidth))
		.style("top" , convertToPx(0))
		;
}

// Static function

Clock.getServerTime = function(m_this, callback) {
	var d = new Date();
	var sDate = GlobalData.formatHttpDebugDate(d);
	var url = Couch.time + "?call=get&date=" + sDate;
	Couch.getDoc(m_this, url, function(data) {
		if (callback) callback.call(m_this, data);
	});
}

Clock.prototype.update = function() {
	Clock.getServerTime(this, function(data) {
		if (data == null) return;
		if (data.repeat && data.interval) {
			this.nInterval = data.interval;
			this.start()
		} else {
			this.stop();
		}
		var epoch = data.epoch;
		this.setTime(epoch);
		var d = new Date(epoch * 1000);
		this.d3text.text(GlobalData.formatClockDate(d));
		this.d3g.style("visibility","inherit");
		var text = this.d3text[0][0];
		var box = text.getBBox();
		var w = box.width;
		this.textWidth = w + 20;
		this.d3svgtext.style("width", convertToPx(this.textWidth));
		this.d3rect.attr("width",  this.textWidth);
		this.d3text.attr("x", this.textWidth / 2 + 2);
		this.d3cover.attr("width",  this.textWidth);
		ResizeWindow();
	});
}

Clock.prototype.stop = function() {
	if (this.nProcess) clearInterval(this.nProcess);	
}

Clock.prototype.start = function() {
	this.stop();
	this.nProcess = setInterval(function() {
		m_Clock.update();
	}, GlobalData.config_clock_interval);
}

Clock.prototype.show = function() {
	this.update();
}

Clock.prototype.hide = function() {
	this.d3g.style("visibility","hidden");
}

Clock.prototype.isVisible = function() {
	var sVisible = this.d3g.style("visibility");
	return !(sVisible == "hidden");
}

