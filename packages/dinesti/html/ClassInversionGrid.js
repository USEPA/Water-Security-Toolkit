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
function InversionGrid(parent) {
	this.sParentId = parent.gid;
	this.nid = parent.nid;
	this.left = NETWORK_VIEW_BORDER + 10;
	this.top = 0;
	this.width = 230;
	this.height = 310;
	this.uniqueString = "InversionGrid";
	this.gid = "g" + this.uniqueString + "_" + this.nid;
	this.svgid = "svg" + this.uniqueString + "_" + this.nid;
	this.rectbgid = "rect" + this.uniqueString + "Bg" + "_" + this.nid;
	this.sBgFill = InversionGrid.DEFAULT_BG_FILL;
	//
	// Private properties
	//
	var m_id = "";
	var m_results = {};
	var m_sensors = [];
	//
	// Private functions
	//
	function setId_private(val) {
		m_id = (val == null) ? "" : val;
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
			setResults_private((data.results) ? data.results : "{}");
			m_this.draw();
		});
	}
	//
	this.create();
	this.addListeners();
}


InversionGrid.DEFAULT_BG_FILL = "rgba(255,255,255,0.8)";

InversionGrid.prototype.create = function() {
	this.d3g = 
	d3.select("#" + this.sParentId).append("g").attr("id", this.gid)
		.style("position","absolute")
		.style("left", convertToPx(this.left))
		;
	this.d3svg = 
	d3.select("#" + this.gid).append("svg").attr("id", this.svgid)
		.style("top", convertToPx(0))
		.style("left", convertToPx(0))
		.style("width", convertToPx(this.width))
		.style("height", convertToPx(this.height))
		;
	this.d3rectBg = 
	d3.select("#" + this.svgid).append("rect").attr("id", this.rectbgid)
		.attr("x",0)
		.attr("y",0)
		.attr("width", this.width)
		.attr("height", this.height)
		.attr("fill", this.sBgFill)
		;
	this.svg = this.d3svg[0][0];
	//
	var height = 300 - 20;
	this.Grid = new DataGrid(5, 5, this.width - 10, height);
	this.Grid.sParentId = this.gid;
	this.Grid.uniqueString = this.uniqueString + "Grid"
	this.Grid.gid = "g" + this.uniqueString + "Grid" + "_" + this.nid;
	this.Grid.parentOnMouseOver = this.gridOnMouseOver;
	this.Grid.nColumns = 3;
	this.Grid.colNames = [];
	this.Grid.colNames.push("order");
	this.Grid.colNames.push("Node");
	this.Grid.colNames.push("Probability");
	this.Grid.colTitles = [];
	this.Grid.colTitles.push("#");
	this.Grid.colTitles.push("Location");
	this.Grid.colTitles.push("Probability");
	this.Grid.colWidth = [40, 75];
	this.Grid.createDataGrid([]);
	this.Grid.registerListener(this.uniqueString, this, this.handleGridEvents);
	//
	this.draw();
}

InversionGrid.prototype.draw = function() {
	var m_this = this;
	var data = this.getResults();
	var d = [];
	for (var id in data.ids) {
		var index = data.ids[id];
		var value = data.list[index].Objective;
		d[index] = {"value": {"order": index + 1, "Node": id, "Probability": value}};
	}
//	var d = [];
//	var i = 1;
//	for (var id in data.ids) {
//		var index = data.ids[id];
//		var value = data.list[index].Objective;
//		d[i-1] = {"value": {"order": i++, "Node": id, "Probability": value}};
//	}
	this.Grid.updateDataGrid(d);
	this.show();
}

InversionGrid.prototype.hide = function() {
	d3.select("#" + this.gid).style("visibility","hidden");
}

InversionGrid.prototype.show = function() {
	d3.select("#" + this.gid).style("visibility","inherit");
}

InversionGrid.prototype.resizeWindow = function(w, h) {
	this.top = h - NETWORK_VIEW_BORDER - 10 - this.height;
	this.d3g.style("top", convertToPx(this.top));
}

////////////////////////////////////////////////////////////////////

InversionGrid.prototype.handleGridEvents = function(e) {
	switch (e.event) {
		case "onClickRow":
			var data = (e && e.data && e.data.value) ? e.data.value : null;
			var id = (data && data.Node) ? data.Node : "";
			m_NetworkView.drawNodePopupAtNode(id);
			break;
		case "onChange":
			break;
	}
}

InversionGrid.prototype.gridOnMouseOver = function(sid) {
}

InversionGrid.prototype.onMouseMove = function(sid) {
	DataGrid.prototype.onMouseMove.call(this.Grid, sid);
}
InversionGrid.prototype.onMouseOver = function(sid) {}
InversionGrid.prototype.onMouseOut = function(sid) {}
InversionGrid.prototype.onMouseUp = function(sid) {
	DataGrid.prototype.onMouseUp.call(this.Grid, sid);	
}
InversionGrid.prototype.onMouseDown = function(sid) {}
InversionGrid.prototype.onMouseWheel = function(sid_or_e) {
	this.Grid.onMouseWheel(sid_or_e);
}
InversionGrid.prototype.onKeyDown = function(sid) {
	DataGrid.prototype.onKeyDown.call(this.Grid, sid);
}
InversionGrid.prototype.onKeyPress = function(sid) {}
InversionGrid.prototype.onKeyUp = function(sid) {}
InversionGrid.prototype.onInput = function(sid) {}
InversionGrid.prototype.onChange = function(sid) {}
InversionGrid.prototype.onClick = function(sid) {}
InversionGrid.prototype.onDblClick = function(sid) {}

////////////////////////////////////////////////////////////////////

InversionGrid.prototype.addListeners = function() {
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

InversionGrid.prototype.addListenersForSelection = function(sSelector) {
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
