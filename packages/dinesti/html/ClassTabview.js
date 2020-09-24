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
function Tabview(parent, uniqueString, dataInFrontOf, left, top, width, height) {
	this.uniqueString = uniqueString;
	this.parent = parent;
	this.dataInFrontOf = dataInFrontOf;
	this.left = left;
	this.top = top;
	this.width = width;
	this.height = height;
	this.gid = "g" + this.uniqueString;
	this.svgid = "svg" + this.uniqueString;
	this.rectid = "rect" + this.uniqueString;
	this.pathid = "pathPanel" + this.uniqueString;
	this.path2id = "pathOutline" + this.uniqueString;
	this.titleid = "titleText" + this.uniqueString;
	this.coverid = "titleCover" + this.uniqueString;
	this.gpanelid = "gPanel" + this.uniqueString;
	this.svgpanelid = "svgPanel" + this.uniqueString;
	this.gipanelid = "giPanel" + this.uniqueString;
	//
	// number of tabs determined by nTabWidth array length
	//
	this.nTabWidth = [];
	this.sTabTitles = [];
	//
	this.nCornerRadius = Tabview.DEFAULT_CORNER_RADIUS;
	this.nTabHeight    = Tabview.DEFAULT_TAB_HEIGHT   ;
	this.nMarginLeft   = Tabview.DEFAULT_MARGIN_LEFT  ;
	this.nMarginTop    = Tabview.DEFAULT_MARGIN_TOP   ;
	this.nMarginRight  = Tabview.DEFAULT_MARGIN_RIGHT ;
	this.nMarginBottom = Tabview.DEFAULT_MARGIN_BOTTOM;
	//
	this.BgColor       = Tabview.DEFAULT_BG_COLOR     ;
	this.dBgOpacity    = Tabview.DEFAULT_BG_OPACITY   ;
	//
	GlobalData.resizeHooks[this.uniqueString] = {"this": this, "function": this.resizeWindow};
	//
	var m_activeTab = Tabview.DEFAULT_ACTIVE_TAB;
	//
	function getActiveTab_private() {
		return m_activeTab;
	}
	function setActiveTab_private(val) {
		m_activeTab = val;
	}
	function isTabActive_private(itab) {
		return m_activeTab == itab;
	}
	//
	this.getActiveTab = function() {
		return getActiveTab_private();
	}
	this.setActiveTab = function(val) {
		var iOld = getActiveTab_private();
		if (iOld == val) return;
		setActiveTab_private(val);
		this.d3tabs.attr("display", function(d,i) {
			if (i == val) return "inherit";
			return "none";
		});
		this.d3gPanels.style("visibility", function(d,i) {
			if (i == val) return "inherit";
			return "hidden";
		})
	}
	this.isTabActive = function(itab) {
		return isTabActive_private(itab);
	}
	//
	this.base = Control;
	this.base(parent.gid, this.uniqueString, this.dataInFrontOf);
}

Tabview.prototype = new Control;
//
Tabview.DEFAULT_ACTIVE_TAB    =  0;
//
Tabview.DEFAULT_CORNER_RADIUS =  3;
Tabview.DEFAULT_TAB_HEIGHT    = 25;
Tabview.DEFAULT_MARGIN_LEFT   = 13;
Tabview.DEFAULT_MARGIN_TOP    = 30;
Tabview.DEFAULT_MARGIN_RIGHT  = 13;
Tabview.DEFAULT_MARGIN_BOTTOM = 13;
//
Tabview.DEFAULT_BG_COLOR      = d3.rgb(0,0,0);
Tabview.DEFAULT_BG_OPACITY    = 0.0;

Tabview.prototype.create = function() {
	var m_this = this;
	//
	if (this.nTabWidth && this.nTabWidth.length == 0) return;
	//
	this.d3g = this.parent.d3g.append("g").attr("id", this.gid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.style("position", "absolute")
		.style("left", convertToPx(this.left))
		.style("top", convertToPx(this.top))
		.style("visibility", "hidden")
		;
	this.d3svg = this.d3g.append("svg").attr("id", this.svgid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top", convertToPx(0))
		.style("width" , convertToPx(this.width))
		.style("height", convertToPx(this.height))
		;
	this.d3rect = this.d3svg.append("rect").attr("id", this.rectid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("x", 0)
		.attr("y", 0)
		.attr("width",  this.width)
		.attr("height", this.height)
		.attr("fill", this.BgColor)
		.attr("fill-opacity", this.dBgOpacity)
		;
	this.d3tabOutlines = this.d3svg
		.selectAll("path." + this.path2id)
		.data(this.nTabWidth)
		.enter().append("path")
		.attr("class", "tabOutlines" + this.uniqueString)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("id", function(d,i) { return m_this.path2id + "_" + i; })
		.attr("d", function(d,i) { return m_this.getTabPath2(i); })
		.attr("stroke", d3.rgb(0,0,0))
		.attr("stroke-opacity", 1.0)
		.attr("stroke-width", 0.8)
//		.attr("fill", d3.rgb(190,190,190))
//		.attr("fill-opacity", 0.65)
		.attr("fill", "none")
		;
	//
	this.gradient = new Gradient("gradient" + this.uniqueString, this.svgid, 0, 0, 0, 100);
	this.gradient
		.addStop(  0, d3.rgb(255,255,255), 1.0)
		.addStop(100, d3.rgb(190,190,190), 1.0)
		.create()
		;
	//
	this.d3tabs = this.d3svg
		.selectAll("path." + this.pathid)
		.data(this.nTabWidth)
		.enter().append("path")
		.attr("class", this.pathid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("id", function(d,i) { return m_this.pathid + "_" + i; })
		.attr("d", function(d,i) { return m_this.getTabPath(i); })
		.attr("display", function(d,i) { return m_this.isTabActive(i) ? "inherit" : "none"; })
		.attr("stroke", "none")
//		.attr("fill", d3.rgb(190,190,190))
//		.attr("fill", d3.rgb(255,255,255))
		.attr("fill", this.gradient.fill)
		.attr("fill-opacity", 0.65)
		.attr("fill-opacity", function(d,i) { return 0.70 - i * 0.03; })
		;
	this.d3titles = this.d3svg
		.selectAll("text." + this.titleid)
		.data(this.nTabWidth)
		.enter().append("text")
		.attr("class", this.titleid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("id", function(d,i) { return m_this.titleid + "_" + i; })
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("text-anchor", "middle")
		.attr("x", function(d,i) { return m_this.nMarginLeft + m_this.getTabLeft(i) + m_this.getTabWidth(i) / 2; })
		.attr("y", 17 + this.nMarginTop)
		.attr("fill", d3.rgb(0, 0, 0))
		.attr("fill-opacity", 1.0)
		.text(function(d,i) { 
			var s = m_this.sTabTitles[i] 
			return s == null ? "" : s;
		})
		;
	this.d3titleCovers = this.d3svg
		.selectAll("path." + this.coverid)
		.data(this.nTabWidth)
		.enter().append("path")
		.attr("class", this.coverid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("id", function(d,i) { return m_this.coverid + "_" + i; })
		.attr("d", function(d,i) { return m_this.getTabPath2(i); })
		.attr("stroke", "none")
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0)
		;
	this.d3gPanels = this.d3g
		.selectAll("g." + this.gpanelid)
		.data(this.nTabWidth)
		.enter().append("g")
		.attr("class", this.gpanelid)
		.attr("id", function(d,i) { return m_this.gpanelid + "_" + i; })
		.attr("data-InFrontOf", this.dataInFrontOf)
		.style("position", "absolute")
		.style("left", convertToPx(this.nMarginLeft))
		.style("top", convertToPx(this.nMarginTop + this.nTabHeight))
		.style("visibility", function(d,i) { return m_this.isTabActive(i) ? "inherit" : "hidden"; })
		//
	this.d3svgPanels = this.d3gPanels
		.append("svg")
		.attr("class", this.svgpanelid)
		.attr("id", function(d,i) { return m_this.svgpanelid + "_" + i; })
		.attr("data-InFrontOf", this.dataInFrontOf)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top", convertToPx(0))
		.style("width" , convertToPx(this.width - this.nMarginLeft - this.nMarginRight))
		.style("height", convertToPx(this.height - this.nTabHeight - this.nMarginTop - this.nMarginBottom))
		;
	this.d3giPanels = this.d3gPanels
		.append("g")
		.attr("id", function(d,i) { return m_this.gipanelid + "_" + i; })
		;
	//
	this.addListeners();
}

Tabview.prototype.getTabId1 = function(itab) {
	return this.svgpanelid + "_" + itab;
	//return d3.select("#" + this.svgpanelid + "_" + itab).attr("id");
}

Tabview.prototype.getTabId2 = function(itab) {
	return this.gipanelid + "_" + itab;
	//return this.d3giPanels[0][itab].attr("id");
	//return d3.select("#" + this.gipanelid + "_" + itab).attr("id");
}

Tabview.prototype.getTabPath2 = function(itab) {
	//return this.getTabPath(itab)
	var x = this.getTabLeft(itab);
	var m = 3;
	var r = this.nCornerRadius - m;
	var path = "";
	path = " M " +  +(x + this.nMarginLeft + m) + " " + (this.nTabHeight + this.nMarginTop - m) + " "
		 + " v " +  -(this.nTabHeight - this.nCornerRadius - m)
		 + " a " +  r + " " + r + " " +  "90 0 1 " + r + " " + -r
		 + " h " +  +(this.getTabWidth(itab) - 2 * this.nCornerRadius)
		 + " a " +  r + " " + r + " " +  "90 0 1 " + r + " " +  r
		 + " v " +  +(this.nTabHeight - this.nCornerRadius - m)
		 + " h " +  -(this.getTabWidth(itab) - 2 * m)
		 ;
	return path;
}

Tabview.prototype.getTabPath = function(itab) {
	var x = this.getTabLeft(itab);
	var r = this.nCornerRadius;
	if (itab == 0) {
		var variable1 = ""
		 + " h " +  -r
		 + " v " +  +r
		 ;
	} else {
		var variable1 = ""
		 + " a " +  r + " " + r + " " +  "90 0 0 " + -r + " " + r
		 ;
	}
	var path = "";
	path = " M " +  (x + this.nMarginLeft) + " " + (this.nTabHeight + this.nMarginTop) + " "
		 + " h " +  -(x - r)
		 + variable1
		 + " v " +  +(this.height - this.nMarginTop - this.nMarginBottom - this.nTabHeight - 2 * r)
		 + " a " +  r + " " + r + " " +  "90 0 0 " + r + " " + r
		 + " h " +  +(this.width - this.nMarginLeft - this.nMarginRight - 2 * r)
		 + " a " +  r + " " + r + " " +  "90 0 0 " + r + " " + -r
		 + " v " +  -(this.height - this.nMarginTop - this.nMarginBottom - this.nTabHeight - 2 * r)
		 + " a " +  r + " " + r + " " +  "90 0 0 " + -r + " " + -r
		 + " h " +  -(this.width - this.nMarginLeft - this.nMarginRight - x - this.getTabWidth(itab) - r)
		 + " v " +  -(this.nTabHeight - r)
		 + " a " +  r + " " + r + " " +  "90 0 0 " + -r + " " + -r
		 + " h " +  -(this.getTabWidth(itab) - 2 * r)
		 + " a " +  r + " " + r + " " +  "90 0 0 " + -r + " " + +r
		 + " v " +  +(this.nTabHeight - r)
		 + " Z "
		 ;
	return path;
}

Tabview.prototype.getTabWidth = function(itab) {
	if (itab >= this.nTabWidth.length) return 0;
	return this.nTabWidth[itab];
}

Tabview.prototype.getTabLeft = function(itab) {
	if (itab >= this.nTabWidth.length) return -1000;
	var left = 0;
	for (var i = 0; i < itab; i++) {
		left += this.nTabWidth[i];
	}
	return left;
}

Tabview.prototype.resizeWindow = function(nWidth, nHeight) {
}

Tabview.prototype.show = function() {
	this.d3g.style("visibility", "visible");
}

Tabview.prototype.hide = function() {
	this.d3g.style("visibility", "hidden");
}

Tabview.prototype.isVisible = function() {
	var sVisible = this.d3g.style("visibility");
	return !(sVisible == "hidden");
}

Tabview.prototype.isHidden = function() {
	return !this.isVisible();
}

//Tabview.prototype.onMouseMove = function(sid) {}
Tabview.prototype.onMouseOver = function(sid) {
	var sidPrefix = getPrefix2(sid);
	var index     = getSuffixIndex(sid);
	switch (sidPrefix) {
		case this.coverid:
			this.d3tabOutlines.attr("fill", function(d,i) {
				if (i == index) return d3.rgb(255,255,255);
				return d3.rgb(0,0,0);
			})
			this.d3tabOutlines.attr("fill-opacity", function(d,i) {
				if (i == index) return 0.5;
				return 0.0;
			})
			this.d3tabOutlines.attr("stroke", function(d,i) {
				if (i == index) return "none";
				return d3.rgb(0,0,0);
			})
			break;
	}
}
Tabview.prototype.onMouseOut = function(sid) {
	var sidPrefix = getPrefix2(sid);
	var index     = getSuffixIndex(sid);
	switch (sidPrefix) {
		case this.coverid:
			this.d3tabOutlines.attr("fill", function(d,i) {
				return "none";
			})
			this.d3tabOutlines.attr("stroke", function(d,i) {
				return d3.rgb(0,0,0);
			})
			break;
	}
}
Tabview.prototype.onMouseUp = function(sid) {
	var sidPrefix = getPrefix2(sid);
	var index     = getSuffixIndex(sid);
	switch (sidPrefix) {
		case this.coverid:
			this.d3tabOutlines.attr("fill", function(d,i) {
				if (i == index) return d3.rgb(255,255,255);
				return d3.rgb(0,0,0);
			})
			this.d3tabOutlines.attr("fill-opacity", function(d,i) {
				if (i == index) return 0.5;
				return 0.0;
			})
			break;
	}

}
Tabview.prototype.onMouseDown = function(sid) {
	var sidPrefix = getPrefix2(sid);
	var index     = getSuffixIndex(sid);
	switch (sidPrefix) {
		case this.coverid:
			this.d3tabOutlines.attr("fill", function(d,i) {
				if (i == index) return d3.rgb(255,255,255);
				return d3.rgb(0,0,0);
			})
			this.d3tabOutlines.attr("fill-opacity", function(d,i) {
				if (i == index) return 0.9;
				return 0.0;
			})
			break;
	}
}
//Tabview.prototype.onMouseWheel = function(sid) {}
//Tabview.prototype.onKeyDown = function(sid) {}
//Tabview.prototype.onKeyPress = function(sid) {}
//Tabview.prototype.onKeyUp = function(sid) {}
//Tabview.prototype.onInput = function(sid) {}
//Tabview.prototype.onChange = function(sid) {}
Tabview.prototype.onClick = function(sid) {
	var sidPrefix = getPrefix2(sid);
	var index     = getSuffixIndex(sid);
	switch (sidPrefix) {
		case this.coverid:
			this.setActiveTab(index);
			break;
	}
}
//Tabview.prototype.onDblClick = function(sid) {}
































