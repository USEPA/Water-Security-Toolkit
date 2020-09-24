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
function InversionNavigator(parent) {
	this.sParentId = parent.gid;
	this.nid = parent.nid;
	this.left = 0;
	this.top = NETWORK_VIEW_BORDER;
	this.width = 170;
	this.height = 34;
	this.uniqueString = "InversionNavigator";
	this.gid = "g" + this.uniqueString + "_" + this.nid;
	this.svgid = "svg" + this.uniqueString + "_" + this.nid;
	this.leftid = "rect" + this.uniqueString + "Left" + "_" + this.nid;
	this.rightid = "rect" + this.uniqueString + "Right" + "_" + this.nid;
	this.coverid = "rect" + this.uniqueString + "NameCover" + "_" + this.nid;
	this.sBgFill = InversionNavigator.DEFAULT_BG_FILL;
	this.sTextColor  = InversionNavigator.DEFAULT_TEXT_COLOR;
	this.nLineWidthText = InversionNavigator.DEFAULT_TEXT_LINE_WIDTH;
	//
	// Private properties
	//
	var m_id = "";
	var m_name = "";
	var m_status = "";
	var m_results = {};
	var m_sensors = [];
	//
	// Private functions
	//
	function setId_private(val) {
		m_id = (val == null) ? "" : val;
	}
	function setName_private(val) {
		m_name = (val == null) ? "" : val;
	}
	function setStatus_private(val) {
		m_status = (val == null) ? "" : val;
	}
	function setResults_private(val) {
		m_results = (val == null) ? {} : val;
	}
	function setSensors_private(val) {
		m_sensors = (val == null) ? [] : val;
	}
	//
	function getId_private(val) {
		return m_id;
	}
	function getName_private(val) {
		return m_name;
	}
	function getStatus_private(val) {
		return m_status;
	}
	function getResults_private(val) {
		return m_results;
	}
	function getSensors_private(val) {
		return m_sensors;
	}
	//
	// Privileged Functions
	//
	this.setId = function(val) {
		setId_private(val);
		this.getData();
	}
	this.setSensors = function(val) {
		setSensors_private(val);
	}
	//
	this.getId = function() {
		return getId_private();
	}
	this.getName = function() {
		return getName_private();
	}
	this.getStatus = function() {
		return getStatus_private();
	}
	this.getResults = function() {
		return getResults_private();
	}
	this.getSensors = function() {
		return getSensors_private();
	}
	//
	this.getData = function() {
		var m_this = this;
		Couch.GetDoc(this.getId(), function(data) {
			setName_private((data.name) ? data.name : "null");
			setStatus_private((data.status) ? data.status : "null");
			setResults_private((data.results) ? data.results : "{}");
			m_this.draw();
		});
	}
	//
	this.create();
	this.addListeners();
}

InversionNavigator.DEFAULT_BG_FILL = "rgba(255,255,255,0.8)";
InversionNavigator.DEFAULT_TEXT_COLOR = "rgba(30,30,30,0.8)";
InversionNavigator.DEFAULT_TEXT_LINE_WIDTH = 0.05;

InversionNavigator.prototype.create = function() {
	this.d3g = 
	d3.select("#" + this.sParentId).append("g").attr("id", this.gid)
		.style("position","absolute")
		.style("top", convertToPx(this.top))
		.style("left", convertToPx(this.left)) // TODO - not needed
		;
	this.d3svg = 
	d3.select("#" + this.gid).append("svg").attr("id", this.svgid)
		.style("top", convertToPx(0))
		.style("left", convertToPx(0))
		.style("width", convertToPx(this.width))
		.style("height", convertToPx(this.height))
		;
	this.d3rectBg = 
	d3.select("#" + this.svgid).append("rect").attr("id","rect" + this.uniqueString + "Bg" + "_" + this.nid)
		.attr("x",0)
		.attr("y",0)
		.attr("width", this.width)
		.attr("height", this.height)
		.attr("fill", this.sBgFill)
		;
	this.d3textTitle = 
	d3.select("#" + this.svgid).append("text").attr("id", "text" + this.uniqueString + "Title" + "_" + this.nid)
		.attr("stroke",this.sTextColor)
		.attr("stroke-width",this.nLineWidthText)
		.attr("x", this.width / 2)
		.attr("y", 10)
		.attr("text-anchor","middle")
		.style("font-size","7pt")
		.text("Inversion Navigator")
		;
	var b = 3;
	this.d3pathLeft = 
	d3.select("#" + this.svgid).append("path").attr("id", "path" + this.uniqueString + "Left" + "_" + this.nid)
		.attr("d",
		"M" + (this.height - 2 * b) + " " + (2 * b)               + " " + 
		"L" + (1.5 * b)             + " " + (this.height / 2)     + " " + 
		"L" + (this.height - 2 * b) + " " + (this.height - 2 * b)
		)
		.attr("fill", "rgba(0,0,0,0.2)")
		;
	this.d3pathRight = 
	d3.select("#" + this.svgid).append("path").attr("id", "path" + this.uniqueString + "Right" + "_" + this.nid)
		.attr("d",
		"M" + (this.width - this.height + 2 * b) + " " + (2 * b)               + " " + 
		"L" + (this.width - 1.5 * b)             + " " + (this.height / 2)     + " " + 
		"L" + (this.width - this.height + 2 * b) + " " + (this.height - 2 * b)
		)
		.attr("fill", "rgba(0,0,0,0.2)")
		;
	var b = 0;
	this.d3rectLeft = 
	d3.select("#" + this.svgid).append("rect").attr("id", this.leftid)
		.attr("x", b)
		.attr("y", b)
		.attr("width", this.height - 2 * b)
		.attr("height", this.height - 2 * b)
		.attr("fill", "rgba(200,200,200,0.0)")//0.3)")
		.attr("cursor","pointer")
		;
	this.d3rectRight = 
	d3.select("#" + this.svgid).append("rect").attr("id", this.rightid)
		.attr("x", this.width - b - (this.height - 2 * b))
		.attr("y", b)
		.attr("width", this.height - 2 * b)
		.attr("height", this.height - 2 * b)
		.attr("fill", "rgba(200,200,200,0.0)")//0.3)")
		.attr("cursor","pointer")
		;
	this.d3textName = 
	d3.select("#" + this.svgid).append("text").attr("id", "text" + this.uniqueString + "Name" + "_" + this.nid)
		.attr("fill","rgba(120,120,120,1.0)")
		.attr("stroke-width",this.nLineWidthText)
		.attr("x", this.width / 2)
		.attr("y", 28)
		.attr("text-anchor","middle")
		.style("font-size","12pt")
		.text("?")
		;
	this.d3rectDataBg = 
	d3.select("#" + this.svgid).append("rect").attr("id", "rect" + this.uniqueString + "DataBg" + "_" + this.nid)
		.attr("x",this.height)
		.attr("y",this.height)
		.attr("width",this.width - 2 * this.height)
		.attr("height", 900)
		.attr("fill",this.sBgFill)
		;
	this.d3rectTextCover =
	d3.select("#" + this.svgid).append("rect").attr("id", this.coverid)
		.attr("fill","rgba(0,0,0,0)")
		.attr("x", this.height)
		.attr("y", 0)
		.attr("width", this.width - 2 * this.height)
		.attr("height", this.height)
		;
	//
	this.svg           = this.d3svg[0][0];
	this.rectLeft      = this.d3rectLeft[0][0];
	this.rectRight     = this.d3rectRight[0][0];
	this.rectTextCover = this.d3rectTextCover[0][0];
	//
	this.draw();
}

InversionNavigator.prototype.draw = function() {
	var m_this = this;
	var bVisible = true;
	if (!bVisible) {		
		this.hide();
	} else {
		this.d3textName.text(this.getName());
		this.show();
	}
}

InversionNavigator.prototype.hide = function() {
	d3.select("#" + this.gid).style("visibility","hidden");
}

InversionNavigator.prototype.show = function() {
	d3.select("#" + this.gid).style("visibility","inherit");
}

InversionNavigator.prototype.viewPrevious = function() {
	m_InversionList.selectPrevious();
	m_InversionList.viewNetwork(m_NetworkView);
}

InversionNavigator.prototype.viewNext = function() {
	m_InversionList.selectNext();
	m_InversionList.viewNetwork(m_NetworkView);
}

InversionNavigator.prototype.resizeWindow = function(w, h) {
	this.d3g.style("left", convertToPx(w / 2 - this.width / 2));
}

////////////////////////////////////////////////////////////////////

InversionNavigator.prototype.onMouseMove = function(sid) {}
InversionNavigator.prototype.onMouseOver = function(sid) {
	var sidPrefix = getPrefix2(sid);
	var id        = getSuffix2(sid);
	switch (sidPrefix) {
		case "":
			break;
	}
	switch (sid) {
		case this.rectLeft.id:
			this.d3pathLeft.attr("fill","rgba(  0,  0,  0,0.6)");
			break;
		case this.rectRight.id:
			this.d3pathRight.attr("fill","rgba(  0,  0,  0,0.6)");
			break;
//		case this.rectTextCover.id:
//			this.d3svg.style("height",convertToPx(this.height + this.numberOfDataPoints * this.dataTextHeight));
//			break;
	}
}
InversionNavigator.prototype.onMouseOut = function(sid) {
	//console.log("out  - " + sid);
	var sidPrefix = getPrefix2(sid);
	var id        = getSuffix2(sid);
	switch (sidPrefix) {
		case "":
			break;
	}
	switch (sid) {
		case this.rectLeft.id:
			this.d3pathLeft.attr("fill","rgba(  0,  0,  0,0.2)");
			break;
		case this.rectRight.id:
			this.d3pathRight.attr("fill","rgba(  0,  0,  0,0.2)");
			break;
//		case this.rectTextCover.id:
//			this.d3svg.style("height",convertToPx(this.height));
//			break;
	}
}
InversionNavigator.prototype.onMouseUp = function(sid) {}
InversionNavigator.prototype.onMouseDown = function(sid) {}
//InversionNavigator.prototype.onMouseWheel = function(sid_or_e) {}
InversionNavigator.prototype.onKeyDown = function(sid) {}
InversionNavigator.prototype.onKeyPress = function(sid) {;}
InversionNavigator.prototype.onKeyUp = function(sid) {;}
InversionNavigator.prototype.onInput = function(sid) {;}
InversionNavigator.prototype.onChange = function(sid) {;}
InversionNavigator.prototype.onClick = function(sid) {
	var sidPrefix = getPrefix2(sid);
	var id        = getSuffix2(sid);
	switch (sidPrefix) {
		case "":
			break;
	}
	switch (sid) {
		case this.rectLeft.id:
			this.viewPrevious();
			break;
		case this.rectRight.id:
			this.viewNext();
			break;
	}
}
InversionNavigator.prototype.onDblClick = function(sid) {;}

////////////////////////////////////////////////////////////////////

InversionNavigator.prototype.addListeners = function() {
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

InversionNavigator.prototype.addListenersForSelection = function(sSelector) {
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
