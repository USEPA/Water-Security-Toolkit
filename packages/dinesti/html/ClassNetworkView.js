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
function NetworkView(sParentId, nid, x, y) { // TODO - use this.x and this.y throughout instead of NETWORK_VIEW_BORDER
	this.sParentId = sParentId;
	//this.parent = window;
	this.nid = nid;
	this.x = NETWORK_VIEW_BORDER;
	this.y = NETWORK_VIEW_BORDER;
	var uniqueString = "NetworkView";
	this.uniqueString = uniqueString;
	this.gid = "g" + this.uniqueString + "_" + this.nid;
	this.svgid = "svg" + this.uniqueString + "_" + this.nid;
	this.rectid = "rect" + this.uniqueString + "_" + this.nid;
	this.viewid = "g" + this.uniqueString + "View" + "_" + this.nid;
	this.svgbgid = "svg" + this.uniqueString + "Bg" + "_" + this.nid;
	this.rectbgid = "rect" + this.uniqueString + "Bg" + "_" + this.nid;
	this.titleid = "text" + this.uniqueString + "Title" + "_" + this.nid;
	//
	this.zoombgid = "rectNetworkZoomBg" + "_" + this.nid;
	//
	this.zoominid = "rectNetworkZoomIn" + "_" + this.nid;
	this.zoomoutid = "rectNetworkZoomOut" + "_" + this.nid;
	this.zoomfullid = "rectNetworkZoomFull" + "_" + this.nid;
	this.zoomselectid = "rectNetworkZoomSelect" + "_" + this.nid;
	this.dotsizeid = "rectDotSize" + "_" + this.nid;
	this.cameraid = "rectCamera" + "_" + this.nid;
	//
	this.dotid = "pathDot" + "_" + this.nid;
	//
	this.zoominbgid = "rectNetworkZoomInBg" + "_" + this.nid;
	this.zoomoutbgid = "rectNetworkZoomOutBg" + "_" + this.nid;
	this.zoomfullbgid = "rectNetworkZoomFullBg" + "_" + this.nid;
	this.zoomselectbgid = "rectNetworkZoomSelectBg" + "_" + this.nid;
	this.dotsizebgid = "rectDotSizeBg" + "_" + this.nid;
	this.camerabgid = "rectCameraBg" + "_" + this.nid;
	//
	this.closeid = "rectNetworkCloseCover" + "_" + this.nid;
	this.closetextid = "rectNetworkCloseText" + "_" + this.nid;
	this.closerectid = "rectNetworkClose" + "_" + this.nid;
	//
	this.togglebgid        = "rectNetworkToggleBg"         + "_" + this.nid;
	//
	this.toggleJunctionId  = "rectNetworkToggleJunctions"  + "_" + this.nid;
	this.toggleTankId      = "rectNetworkToggleTanks"      + "_" + this.nid;
	this.toggleReservoirId = "rectNetworkToggleReservoirs" + "_" + this.nid;
	this.toggleValveId     = "rectNetworkToggleValves"     + "_" + this.nid;
	this.togglePumpId      = "rectNetworkTogglePumps"      + "_" + this.nid;
	this.togglePipeId      = "rectNetworkTogglePipes"      + "_" + this.nid;
	this.toggleSensorId    = "rectNetworkToggleSensors"    + "_" + this.nid;
	//
	this.toggleJunctionBG  = "rectNetworkToggleJunctionsBG"  + "_" + this.nid;
	this.toggleTankBG      = "rectNetworkToggleTanksBG"      + "_" + this.nid;
	this.toggleReservoirBG = "rectNetworkToggleReservoirsBG" + "_" + this.nid;
	this.toggleValveBG     = "rectNetworkToggleValvesBG"     + "_" + this.nid;
	this.togglePumpBG      = "rectNetworkTogglePumpsBG"      + "_" + this.nid;
	this.togglePipeBG      = "rectNetworkTogglePipesBG"      + "_" + this.nid;
	this.toggleSensorsBG   = "rectNetworkToggleSensorsBG"    + "_" + this.nid;
	//
	this.toggleIds = [
		null, // this forces a one-based array from a zero-based
		{"id":this.toggleJunctionId,  "bgid":this.toggleJunctionBG,  "pathId":"pathToggleJunction"  + "_" + this.nid, "f":this.showJunctions,  "select":"fill",   "color":d3.rgb(  0,  0,  0), "y":185},
		{"id":this.toggleTankId,      "bgid":this.toggleTankBG,      "pathId":"pathToggleTank"      + "_" + this.nid, "f":this.showTanks,      "select":"fill",   "color":d3.rgb(  0,  0,155), "y":210},
		{"id":this.toggleReservoirId, "bgid":this.toggleReservoirBG, "pathId":"pathToggleReservoir" + "_" + this.nid, "f":this.showReservoirs, "select":"fill",   "color":d3.rgb(  0,155,  0), "y":235},
		{"id":this.toggleValveId,     "bgid":this.toggleValveBG,     "pathId":"pathToggleValve"     + "_" + this.nid, "f":this.showValves,     "select":"fill",   "color":d3.rgb(  0,155,155), "y":260},
		{"id":this.togglePumpId,      "bgid":this.togglePumpBG,      "pathId":"pathTogglePump"      + "_" + this.nid, "f":this.showPumps,      "select":"fill",   "color":d3.rgb(155,155,  0), "y":285},
		{"id":this.togglePipeId,      "bgid":this.togglePipeBG,      "pathId":"pathTogglePipe"      + "_" + this.nid, "f":this.showPipes,      "select":"stroke", "color":d3.rgb(  0,  0,  0), "y":310},
		{"id":this.toggleSensorId,    "bgid":this.toggleSensorsBG,   "pathId":"pathToggleSensor"    + "_" + this.nid, "f":this.showSensors,    "select":"stroke", "color":d3.rgb(  0,  0,255), "y":335}
	]
	this.Toggle = new BitMasker("Toggle", 1, 7);
	this.bCalibrating = false;
	this.bZoomNodeActive = false;
	this.nStrokeWidth = GlobalData.nStrokeWidth;
	this.bZoomUpdated = false;
	this.dOpacity = 0.6;
	this.ZoomObj = {};
	this.m_data = {};
	this.m_NodeGraph = null;
	this.m_ImpactView = null;
	this.m_InversionNavigator = null;
	this.m_InversionGrid = null;
	this.svgFile = null;
	GlobalData.resizeHooks[uniqueString + "_" + this.nid] = {"this": this, "function": this.resizeWindow};
	//
	// Private properties
	//
	var m_uuid = "";
	var m_view = "";
	var m_selectionuuid = "";
	//
	// Private functions
	//
	function setUuid_private(val) {
		m_uuid = (val == null) ? "" : val;
	}
	function getUuid_private() {
		return m_uuid;
	}
	function setView_private(val) {
		m_view = (val == null) ? "" : val;
	}
	function getView_private() {
		return m_view;
	}
	function setSelectionUuid_private(val) {
		m_selectionuuid = (val == null) ? "" : val;
	}
	function getSelectionUuid_private() {
		return m_selectionuuid;
	}
	//
	// Privileged functions
	//
	this.setUuid = function(val) {
		setUuid_private(val);
	}
	this.getUuid = function() {
		return getUuid_private();
	}
	this.setView = function(val) {
		setView_private(val);
	}
	this.getView = function() {
		return getView_private();
	}
	this.setSelectionUuid = function(val) {
		setSelectionUuid_private(val);
	}
	this.getSelectionUuid = function() {
		return getSelectionUuid_private();
	}
	//
	this.create();
}


NetworkView.prototype.create = function() {
	this.d3g = 
	d3.select("#" + this.sParentId).append("g").attr("id", this.gid)
		.attr("class", this.uniqueString + this.nid)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top" , convertToPx(0))
		.style("visibility", "hidden")
		;
	this.d3svgbg = 
	this.d3g.append("svg").attr("id", this.svgbgid)
		.attr("class", this.uniqueString + this.nid)
		.style("position", "absolute")
		.style("left", convertToPx(0))
		.style("top" , convertToPx(0))
		;
	this.d3rectbg = 
	this.d3svgbg.append("rect").attr("id", this.rectbgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 0)
		.attr("y", 0)
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.5)
		;
	this.d3svg = 
	this.d3g.append("svg").attr("id", this.svgid)
		.attr("class", this.uniqueString + this.nid)
		.style("position", "absolute")
		.style("left", convertToPx(NETWORK_VIEW_BORDER))
		.style("top" , convertToPx(NETWORK_VIEW_BORDER))
		;
	this.d3rect = 
	this.d3svg.append("rect").attr("id", this.rectid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 0)
		.attr("y", 0)
		.attr("fill", d3.rgb(200,200,200))
//		.attr("fill", d3.rgb(255,255,255))//kate
		.attr("fill-opacity", 1.0)
		;
	this.d3view = 
	this.d3svg.append("g").attr("id", this.viewid)
		.attr("class", this.uniqueString + this.nid)
		.attr("transform", "rotate(180)")
		.data([{"Type":"View"}])
		;
	this.imageTransform = "rotate(-180) scale (-1,1)";
	this.d3image = this.d3view.append("image").attr("id", this.imagebgid)
		.attr("id","image" + this.uniqueString + "_" + this.nid)
		.attr("transform", this.imageTransform)
		.attr("x", 0)
		.attr("y", 0)
		.attr("width", 1280)
		.attr("height", 1280)
		.attr("opacity", 0.30)
		;
	this.d3title =  this.d3svg.append("text").attr("id", this.titleid)
		.attr("x", 340)
		.attr("y", 25)
		.text("")
		.style("font-size", 20)
		.attr("display", "none")
		;
	////////////////////////////////////////////////////////////////////	

	this.m_NodeGraph           = new NodeGraph         (                       this, NETWORK_VIEW_BORDER + 10, 530, 300, 250);
	this.m_ImpactView          = new ImpactView        (                       this, NETWORK_VIEW_BORDER + 10, 300);
	this.m_InversionNavigator  = new InversionNavigator(                       this);
	this.m_InversionGrid       = new InversionGrid     (                       this);
	this.NodePopup             = new NetPopup          ("NetPopupNodePopup"  , this);
	this.ToolTip               = new NetPopup          ("NetPopupToolTip"    , this);
	this.NodeFilter            = new FilterList        (                       this, "NodeFilter", "", null, NETWORK_VIEW_BORDER + 4, 0, false, 20, true);
	this.WizardCompleteButton  = new SvgButton         ("Wizard Complete"    , this, "NetworkWizardCompleteButton", null, 200, 200, 150, 30, 15);
	this.ExportButton          = new SvgButton         ("Export to GIS"      , this, "NetworkExportButton"        , null, 250, 250, 150, 30, 15);
	this.GatherDataButton      = new SvgButton         ("Gather more data...", this, "NetworkGatherDataButton"    , null, 300, 300, 150, 30, 15);

	////////////////////////////////////////////////////////////////////
	this.d3ZoomSelection = 
	this.d3svg.append("rect").attr("id", "rectZoomSelection" + "_" + this.nid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 0)
		.attr("y", 0)
		.attr("width" , 0)
		.attr("height", 0)
		.attr("fill", d3.rgb(0,0,255))
		.attr("fill-opacity", 0.1)
		.attr("display", "none")
		.attr("data-state", "")
		;
	////////////////////////////////////////////////////////////////////
	this.d3svg.append("path").attr("id", "pathDot" + "_" + this.nid)
		.attr("class", this.uniqueString + this.nid)
		//.attr("fill", d3.rgb(255,255,255))//kate
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.5)
		.attr("d", this.getDotPath(45, 130))
		.attr("data-x","45")
		.attr("data-y","130")
		;
	////////////////////////////////////////////////////////////////////
	this.d3zoombg = 
	this.d3svg.append("rect").attr("id", this.zoombgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 15)
		.attr("y", 15)	
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width" , 30)
		.attr("height", 155)
		.attr("fill", d3.rgb(150,150,150))
		.attr("fill-opacity", 0.2)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	////////////////////////////////////////////////////////////////////
	this.d3svg.append("rect").attr("id", this.zoominbgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 20)	
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width",  20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.2)
		;
	this.d3svg.append("line").attr("id", "line1NetworkZoomIn" + "_" + this.nid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x1", 23)
		.attr("y1", 30)
		.attr("x2", 37)
		.attr("y2", 30)
		.attr("stroke", d3.rgb(255,255,255))
		.attr("stroke-opacity", 0.7)
		.attr("stroke-opacity", 1.0)
		.attr("stroke-width", 4)
		;
	this.d3svg.append("line").attr("id", "line2NetworkZoomIn" + "_" + this.nid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x1", 30)
		.attr("y1", 23)
		.attr("x2", 30)
		.attr("y2", 37)
		.attr("stroke", d3.rgb(255,255,255))
		.attr("stroke-opacity", 0.7)
		.attr("stroke-opacity", 1.0)
		.attr("stroke-width", 4)
		;
	this.d3svg.append("rect").attr("id", this.zoominid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 20)	
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width",  20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	////////////////////////////////////////////////////////////////////
	this.d3svg.append("rect").attr("id", this.zoomoutbgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 45)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width" , 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.2)
		.attr("cursor", "pointer")
		;
	this.d3svg.append("line").attr("id", "lineNetworkZoomOut" + "_" + this.nid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x1", 23)
		.attr("y1", 55)
		.attr("x2", 37)
		.attr("y2", 55)
		.attr("stroke", d3.rgb(255,255,255))
		.attr("stroke-opacity", 0.7)
		.attr("stroke-opacity", 1.0)
		.attr("stroke-width", 4)
		.attr("cursor", "pointer")
		;
	this.d3svg.append("rect").attr("id", this.zoomoutid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 45)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width" , 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	////////////////////////////////////////////////////////////////////
	this.d3svg.append("rect").attr("id", this.zoomfullbgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 70)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.2)
		.attr("cursor", "pointer")
		;
	////
	var path =	"M22 77 L22 72  "
			 +	"M27 72 L22 72  "
			 +	"M27 77 L22 72  "
			 +	"M33 72 L38 72  "
			 +	"M38 77 L38 72  "
			 +	"M33 77 L38 72  "
			 +	"M38 83 L38 88  "
			 +	"M33 88 L38 88  "
			 +	"M33 83 L38 88  "
			 +	"M28 88 L22 88  "
			 +	"M22 83 L22 88  "
			 +	"M28 83 L22 88  ";
	////
	var path =	"M22 72 v+5 "
			 +	"M22 72 h+5 "
			 +	"M22 72 l+5 +5 "
			 +	"M38 72 h-5 "
			 +	"M38 72 v+5 "
			 +	"M38 72 l-5 +5 "
			 +	"M38 88 v-5 "
			 +	"M38 88 h-5 "
			 +	"M38 88 l-5 -5 "
			 +	"M22 88 h+5 "
			 +	"M22 88 v-5 "
			 +	"M22 88 l+5 -5 ";
	////
	var path =	"M23 73 v+5 "
			 +	"M23 73 h+5 "
			 +	"M23 73 l+5 +5 "
			 +	"M37 73 h-5 "
			 +	"M37 73 v+5 "
			 +	"M37 73 l-5 +5 "
			 +	"M37 87 v-5 "
			 +	"M37 87 h-5 "
			 +	"M37 87 l-5 -5 "
			 +	"M23 87 h+5 "
			 +	"M23 87 v-5 "
			 +	"M23 87 l+5 -5 ";
	////
	var path =	"M23 73 v+4 "
			 +	"M23 73 h+4 "
			 +	"M23 73 l+6 +6 "
			 +	"M37 73 h-4 "
			 +	"M37 73 v+4 "
			 +	"M37 73 l-6 +6 "
			 +	"M37 87 v-4 "
			 +	"M37 87 h-4 "
			 +	"M37 87 l-6 -6 "
			 +	"M23 87 h+4 "
			 +	"M23 87 v-4 "
			 +	"M23 87 l+6 -6 ";
	////
	var path =	"M23 73 v+4 "
			 +  "l+4 -4 "
			 +	"M23 73 h+4 "
			 +  "l-4 +4 "
			 +	"M23 73 l+4 +4 "
			 +	"M37 73 h-4 "
			 +  "l+4 +4 "
			 +	"M37 73 v+4 "
			 +  "l-4 -4 "
			 +	"M37 73 l-4 +4 "
			 +	"M37 87 v-4 "
			 +  "l-4 +4 "
			 +	"M37 87 h-4 "
			 +  "l+4 -4 "
			 +	"M37 87 l-4 -4 "
			 +	"M23 87 h+4 "
			 +  "l-4 -4 "
			 +	"M23 87 v-4 "
			 +  "l+4 +4 "
			 +	"M23 87 l+4 -4 ";
	////
	var path =	"M23 73 v+3 "
			 +  "l+3 -3 "
			 +	"M23 73 h+3 "
			 +  "l-3 +3 "
			 +	"M23 73 l+3 +3 "
			 +	"M37 73 h-3 "
			 +  "l+3 +3 "
			 +	"M37 73 v+3 "
			 +  "l-3 -3 "
			 +	"M37 73 l-3 +3 "
			 +	"M37 87 v-3 "
			 +  "l-3 +3 "
			 +	"M37 87 h-3 "
			 +  "l+3 -3 "
			 +	"M37 87 l-3 -3 "
			 +	"M23 87 h+3 "
			 +  "l-3 -3 "
			 +	"M23 87 v-3 "
			 +  "l+3 +3 "
			 +	"M23 87 l+3 -3 ";
	////
	var path =	"M23 73 v+3 "
			 +  "l+3 -3 "
			 +	"M23 73 h+3 "
			 +  "l-3 +3 "
			 +	"M23 73 l+4 +4 "
			 +	"M37 73 h-3 "
			 +  "l+3 +3 "
			 +	"M37 73 v+3 "
			 +  "l-3 -3 "
			 +	"M37 73 l-4 +4 "
			 +	"M37 87 v-3 "
			 +  "l-3 +3 "
			 +	"M37 87 h-3 "
			 +  "l+3 -3 "
			 +	"M37 87 l-4 -4 "
			 +	"M23 87 h+3 "
			 +  "l-3 -3 "
			 +	"M23 87 v-3 "
			 +  "l+3 +3 "
			 +	"M23 87 l+4 -4 ";
	////
	var path =	"M23 73 v+3 "
			 +  "l+3 -3 "
			 +	"M23 73 h+3 "
			 +  "l-3 +3 "
			 +	"M23 73 l+4 +4 "
			 +	"M37 73 h-3 "
			 +  "l+3 +3 "
			 +	"M37 73 v+3 "
			 +  "l-3 -3 "
			 +	"M37 73 l-4 +4 "
			 +	"M37 87 v-3 "
			 +  "l-3 +3 "
			 +	"M37 87 h-3 "
			 +  "l+3 -3 "
			 +	"M37 87 l-4 -4 "
			 +	"M23 87 h+3 "
			 +  "l-3 -3 "
			 +	"M23 87 v-3 "
			 +  "l+3 +3 "
			 +	"M23 87 l+4 -4 ";
	////
	var path =	"M24 74 v+3 "
			 +  "l+3 -3 "
			 +	"M24 74 h+3 "
			 +  "l-3 +3 "
			 +	"M24 74 l+4 +4 "
			 +	"M36 74 h-3 "
			 +  "l+3 +3 "
			 +	"M36 74 v+3 "
			 +  "l-3 -3 "
			 +	"M36 74 l-4 +4 "
			 +	"M36 86 v-3 "
			 +  "l-3 +3 "
			 +	"M36 86 h-3 "
			 +  "l+3 -3 "
			 +	"M36 86 l-4 -4 "
			 +	"M24 86 h+3 "
			 +  "l-3 -3 "
			 +	"M24 86 v-3 "
			 +  "l+3 +3 "
			 +	"M24 86 l+4 -4 ";
	////
	var path =	"M24 74 v+3 "
			 +	"M24 74 h+3 "
			 +	"M24 74 l+6 +6 "
			 +	"M36 74 h-3 "
			 +	"M36 74 v+3 "
			 +	"M36 74 l-6 +6 "
			 +	"M36 86 v-3 "
			 +	"M36 86 h-3 "
			 +	"M36 86 l-6 -6 "
			 +	"M24 86 h+3 "
			 +	"M24 86 v-3 "
			 +	"M24 86 l+6 -6 ";
	////
	var path =	"M24 74 l+0 +3 "
			 +	"M24 74 l+3 +0 "
			 +	"M24 74 l+6 +6 "
			 +	"M36 74 l-3 +0 "
			 +	"M36 74 l-0 +3 "
			 +	"M36 74 l-6 +6 "
			 +	"M36 86 l-0 -3 "
			 +	"M36 86 l-3 -0 "
			 +	"M36 86 l-6 -6 "
			 +	"M24 86 l+3 -0 "
			 +	"M24 86 l+0 -3 "
			 +	"M24 86 l+6 -6 ";
	////
	var path1 =	"M24 74 l+1 +3 "
			 +	"M24 74 l+3 +1 "
			 +	"M24 74 l+6 +6 "
			 +	"M36 74 l-3 +1 "
			 +	"M36 74 l-1 +3 "
			 +	"M36 74 l-6 +6 "
			 +	"M36 86 l-1 -3 "
			 +	"M36 86 l-3 -1 "
			 +	"M36 86 l-6 -6 "
			 +	"M24 86 l+3 -1 "
			 +	"M24 86 l+1 -3 "
			 +	"M24 86 l+6 -6 ";
	////
	var path1 =	"M24 74 v+3 "
			 +  "l+3 -3 "
			 +	"M24 74 h+3 "
			 +  "l-3 +3 "
			 +	"M24 74 l+6 +6 "
			 +	"M36 74 h-3 "
			 +  "l+3 +3 "
			 +	"M36 74 v+3 "
			 +  "l-3 -3 "
			 +	"M36 74 l-6 +6 "
			 +	"M36 86 v-3 "
			 +  "l-3 +3 "
			 +	"M36 86 h-3 "
			 +  "l+3 -3 "
			 +	"M36 86 l-6 -6 "
			 +	"M24 86 h+3 "
			 +  "l-3 -3 "
			 +	"M24 86 v-3 "
			 +  "l+3 +3 "
			 +	"M24 86 l+6 -6 ";
	////
	this.d3svg.append("path").attr("id", "pathNetworkZoomFull" + "_" + this.nid)
		.attr("class", this.uniqueString + this.nid)
		.attr("d", path)
		.attr("stroke", d3.rgb(255,255,255))
		.attr("stroke-opacity", 0.7)
		.attr("stroke-opacity", 1.0)
		.attr("stroke-width", 2.0)
		.attr("stroke-linecap", "round")
		.attr("cursor", "pointer")
		;
	////
	this.d3svg.append("rect").attr("id", this.zoomfullid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 70)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;

	////////////////////////////////////////////////////////////////////
	this.d3svg.append("rect").attr("id", this.zoomselectbgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 95)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.2)
		.attr("cursor", "pointer")
		;
	this.d3svg.append("circle").attr("id", "circleNetworkZoomSelect" + "_" + this.nid)
		.attr("class", this.uniqueString + this.nid)
		.attr("cx", 28)
		.attr("cy", 103)
		.attr("r", 4.5)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.1)
		.attr("stroke", d3.rgb(255,255,255))
		.attr("stroke-opacity", 0.7)
		.attr("stroke-opacity", 1.0)
		.attr("stroke-width", 2.5)
		.attr("stroke-linecap", "round")
		.attr("cursor", "pointer")
		;
	this.d3svg.append("line").attr("id", "lineNetworkZoomSelect" + "_" + this.nid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x1", 32)
		.attr("y1", 107)
		.attr("x2", 36)
		.attr("y2", 111)
		.attr("fill", "none")
		.attr("stroke", d3.rgb(255,255,255))
		.attr("stroke-opacity", 0.7)
		.attr("stroke-opacity", 1.0)
		.attr("stroke-width", 2.5)
		.attr("stroke-linecap", "round")
		.attr("cursor", "pointer")
		;
	this.d3svg.append("rect").attr("id", this.zoomselectid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 95)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;

	////////////////////////////////////////////////////////////////////
	this.d3svg.append("rect").attr("id", this.dotsizebgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 120)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.2)
		.attr("cursor", "pointer")
		;
	this.d3svg.append("circle").attr("id", "circleDotSize" + "_" + this.nid)
		.attr("class", this.uniqueString + this.nid)
		.attr("cx", 30)
		.attr("cy", 130)
		.attr("r", 4.5)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.7)
		.attr("fill-opacity", 1.0)
		.attr("cursor", "pointer")
		;
	this.d3svg.append("rect").attr("id", this.dotsizeid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 120)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;

	////////////////////////////////////////////////////////////////////
	this.d3svg.append("rect").attr("id", this.camerabgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 145)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.2)
		.attr("cursor", "pointer")
		;
	this.d3svg.append("rect").attr("id", this.cameraid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", 145)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;

	////////////////////////////////////////////////////////////////////
	this.d3togglebg = 
	this.d3svg.append("rect").attr("id", this.togglebgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 15)
		.attr("y", 180)	
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width" , 30)
		.attr("height", 180)
		.attr("fill", d3.rgb(150,150,150))
		.attr("fill-opacity", 0.2)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	///////////////////////////////////////////////////////
	var toggle = this.getToggle(this.toggleJunctionId);
	this.d3svg.append("rect").attr("id", toggle.bgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		;
	this.d3svg.append("path").attr("id", toggle.pathId)
		.attr("class", this.uniqueString + this.nid)
		.attr("d", this.getJunctionPath(30, toggle.y + 10, 1))	
		.attr("transform", "rotate(180 30 " + (toggle.y + 10) + ")")
		.attr(toggle.select, toggle.color)
		.attr("fill-opacity", this.dOpacity)
		;
	this.d3svg.append("rect").attr("id", toggle.id)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		//.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	////////////////////////////////////////
	var toggle = this.getToggle(this.toggleTankId);
	this.d3svg.append("rect").attr("id", toggle.bgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		;
	this.d3svg.append("path").attr("id", toggle.pathId)
		.attr("class", this.uniqueString + this.nid)
		.attr("d", this.getTankPath(30, toggle.y + 10, 1))		
		.attr("transform", "rotate(180 30 " + (toggle.y + 10) + ")")
		.attr(toggle.select, toggle.color)
		.attr("fill-opacity", this.dOpacity)
		;
	this.d3svg.append("rect").attr("id", toggle.id)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	////////////////////////////////////////
	var toggle = this.getToggle(this.toggleReservoirId);
	this.d3svg.append("rect").attr("id", toggle.bgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		;
	this.d3svg.append("path").attr("id", toggle.pathId)
		.attr("class", this.uniqueString + this.nid)
		.attr("d", this.getReservoirPath(30, toggle.y + 10, 1))		
		.attr("transform", "rotate(180 30 " + (toggle.y + 10) + ")")
		.attr(toggle.select, toggle.color)
		.attr("fill-opacity", this.dOpacity)
		;
	this.d3svg.append("rect").attr("id", toggle.id)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	////////////////////////////////////////
	var toggle = this.getToggle(this.toggleValveId);
	this.d3svg.append("rect").attr("id", toggle.bgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		;
	this.d3svg.append("path").attr("id", toggle.pathId)
		.attr("class", this.uniqueString + this.nid)
		.attr("d", this.getValvePath(22, toggle.y + 10, 38, toggle.y + 10, 1))		
		.attr("transform", "rotate(180 30 " + (toggle.y + 10) + ")")
		.attr(toggle.select, toggle.color)
		.attr("fill-opacity", this.dOpacity)
		;
	this.d3svg.append("rect").attr("id", toggle.id)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	////////////////////////////////////////
	var toggle = this.getToggle(this.togglePumpId);
	this.d3svg.append("rect").attr("id", toggle.bgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		;
	this.d3svg.append("path").attr("id", toggle.pathId)
		.attr("class", this.uniqueString + this.nid)
		.attr("d", this.getPumpPath(22, toggle.y + 10, 38, toggle.y + 10, 1))
		.attr("transform", "rotate(180 30 " + (toggle.y + 10) + ")")
		.attr(toggle.select, toggle.color)
		.attr("fill-opacity", this.dOpacity)
		;
	this.d3svg.append("rect").attr("id", toggle.id)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	////////////////////////////////////////
	var toggle = this.getToggle(this.togglePipeId);
	this.d3svg.append("rect").attr("id", toggle.bgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		;
	this.d3svg.append("path").attr("id", toggle.pathId)
		.attr("class", this.uniqueString + this.nid)
		.attr("d", this.getPipePath(22, toggle.y + 10, 38, toggle.y + 10, 1))		
		.attr("transform", "rotate(180 30 " + (toggle.y + 10) + ")")
		.attr("fill", "none")
		.attr("stroke-width", 1)
		.attr(toggle.select, toggle.color)
		.attr("fill-opacity", this.dOpacity)
		;
	this.d3svg.append("rect").attr("id", toggle.id)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	////////////////////////////////////////
	var toggle = this.getToggle(this.toggleSensorId);
	this.d3svg.append("rect").attr("id", toggle.bgid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		;
	this.d3svg.append("path").attr("id", toggle.pathId)
		.attr("class", this.uniqueString + this.nid)
		.attr("d", this.getSensorPath(30, toggle.y + 10, 0.50))		
		.attr("transform", "rotate(180 30 " + (toggle.y + 10) + ")")
		.attr("fill", "none")
		.attr("stroke-width", 2)
		.attr(toggle.select, toggle.color)
		.attr("fill-opacity", this.dOpacity)
		;
	this.d3svg.append("rect").attr("id", toggle.id)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 20)
		.attr("y", toggle.y)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 20)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.5)
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;

	////////////////////////////////////////////////////////////////////
	this.d3svg.append("rect").attr("id", "rectMouseXY" + "_" + this.nid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 0)//60)
		.attr("y", 20)
		.attr("rx", 3)
		.attr("ry", 3)
		.attr("width", 0)//70)
		.attr("height", 20)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 0.0)//0.5
		;
	this.d3svg.append("text").attr("id", "textMouseXY" + "_" + this.nid)
		.attr("class", this.uniqueString + this.nid)
		.attr("x", 0)//95)
		.attr("y", 34)
		.attr("text-anchor", "middle")
		.attr("fill", d3.rgb(40,40,40))
		.attr("fill-opacity", 1.0)
		.text("")
		;

	////////////////////////////////////////////////////////////////////
	this.d3CloseRect = 
	this.d3svg.append("rect").attr("id", this.closerectid)
		.attr("class", this.uniqueString + this.nid)
		.attr("rx", 8)
		.attr("ry", 8)
		.attr("width", 50)
		.attr("height", 30)
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity",0.5)
		;
	this.d3CloseText = 
	this.d3svg.append("text").attr("id", this.closetextid)
		.attr("class", this.uniqueString + this.nid)
		.attr("fill", d3.rgb(255,255,255))
		.attr("fill-opacity", 1.0)
		.text("Close")
		;
	this.d3CloseCover = 
	this.d3svg.append("rect").attr("id", this.closeid)
		.attr("class", this.uniqueString + this.nid)
		.attr("rx", 8)
		.attr("ry", 8)
		.attr("width", 50)
		.attr("height", 30)
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", 0.0)
		.attr("cursor", "pointer")
		;
	this.d3Close = d3.selectAll("#" + this.closerectid + "," + "#" + this.closetextid + "," + "#" + this.closeid);
	return;
}

////////////////////////////////////////////////////////////////////
NetworkView.prototype.getData = function() {
	return this.m_data;
}

NetworkView.prototype.getJunctions = function() {
	var sel = this.d3view.selectAll("circle.Junction"); // TODO
	return sel;
}
NetworkView.prototype.getReservoirs = function() {
	var sel = this.d3view.selectAll("path.Reservoir"); // TODO
	return sel;
}
NetworkView.prototype.getTanks = function() {
	var sel = this.d3view.selectAll("path.Tank"); // TODO
	return sel;
}
NetworkView.prototype.getNodes = function() {
	var sel = this.d3view.selectAll("circle.Junction, path.Tank, path.Reservoir"); // TODO
	return sel;
}
NetworkView.prototype.getNode = function(id) {
	var sel = this.d3view.selectAll("#Node_" + id);
	return sel;
}
NetworkView.prototype.getLinks = function() {
	var sel = this.d3view.selectAll("path.Pump, path.Valve, path.Pipe"); // TODO
	return sel;
}
NetworkView.prototype.getSensors = function() {
	var sel = this.d3view.selectAll("path.Sensor");
	return sel;
}
NetworkView.prototype.getSensor = function(id) {
	//var sel = this.d3view.selectAll("#Sensor_" + id + "_" + this.nid);
	var sel = this.d3view.selectAll("#" + "Sensor" + "_" + id);
	return sel;
}
NetworkView.prototype.removeSensors = function() {
	var sel = this.getSensors();
	sel.remove();
}
////////////////////////////////////////////////////////////////////
NetworkView.prototype.hideNodeGraph = function(bHide) {
	if (bHide == null || bHide) {
		this.m_NodeGraph.hide();
	} else {
		this.m_NodeGraph.show();
	}
}
NetworkView.prototype.showNodeGraph = function(bShow) {
	var bHide = !(bShow == null || bShow);
	this.hideNodeGraph(bHide);
}

NetworkView.prototype.hideImpactView = function(bHide) {
	if (bHide == null || bHide) {
		this.m_ImpactView.hide();
	} else {
		this.m_ImpactView.show();
	}
}
NetworkView.prototype.showImpactView = function(bShow) {
	var bHide = !(bShow == null || bShow);
	this.hideImpactView(bHide);
}

NetworkView.prototype.hideInversionNavigator = function(bHide) {
	if (bHide == null || bHide) {
		this.m_InversionNavigator.hide();
	} else {
		this.m_InversionNavigator.show();
	}
}
NetworkView.prototype.showInversionNavigator = function(bShow) {
	var bHide = !(bShow == null || bShow);
	this.hideInversionNavigator(bHide);
}

NetworkView.prototype.hideInversionGrid = function(bHide) {
	if (bHide == null || bHide) {
		this.m_InversionGrid.hide();
	} else {
		this.m_InversionGrid.show();
	}
}
NetworkView.prototype.showInversionGrid = function(bShow) {
	var bHide = !(bShow == null || bShow);
	this.hideInversionGrid(bHide);
}

NetworkView.prototype.hideEventButtons = function(bHide) {
	if (bHide == null || bHide) {
		this.WizardCompleteButton.hide();
		this.ExportButton.hide();
		this.GatherDataButton.hide();
	} else {
		this.WizardCompleteButton.show();
		this.ExportButton.show();
		this.GatherDataButton.show();
	}
}
NetworkView.prototype.showEventButtons = function(bHide) {
	var bHide = !(bShow == null || bShow);
	this.hideEventButtons(bHide);
}

NetworkView.prototype.hideClose = function(bHide) {
	if (bHide == null || bHide) {
		this.d3Close.attr("display", "none");
	} else {
		this.d3Close.attr("display", "inherit");
	}
}
NetworkView.prototype.showClose = function(bShow) {
	var bHide = !(bShow == null || bShow);
	this.hideClose(bHide);
}

NetworkView.prototype.hideNodePopup = function(bHide) {
	if (bHide == null || bHide) {
		this.NodePopup.hide();
	} else {
		this.NodePopup.show();
	}
}
NetworkView.prototype.showNodePopup = function(bShow) {
	var bHide = (bShow == null || bShow);
	this.hideNodePopup(bHide);
}
////////////////////////////////////////////////////////////////////
NetworkView.prototype.showPieces = function(list) {
	this.showJunctions (list[0] == 1);
	this.showTanks     (list[1] == 1);
	this.showReservoirs(list[2] == 1);
	this.showPumps     (list[3] == 1);
	this.showValves    (list[4] == 1);
	this.showPipes     (list[5] == 1);
	this.showSensors   (list[6] == 1);
}

NetworkView.prototype.showJunctions = function(bShow) {
	if (bShow == null || bShow) {
		this.showSelection("circle.Junction, path.Junctions");
	} else {
		this.hideSelection("circle.Junction, path.Junctions");
	}
}
NetworkView.prototype.showTanks = function(bShow) {
	if (bShow == null || bShow) {
		this.showSelection("path.Tank");
	} else {
		this.hideSelection("path.Tank");
	}
}
NetworkView.prototype.showReservoirs = function(bShow) {
	if (bShow == null || bShow) {
		this.showSelection("path.Reservoir");
	} else {
		this.hideSelection("path.Reservoir");
	}
}
NetworkView.prototype.showPumps = function(bShow) {
	if (bShow == null || bShow) {
		this.showSelection("path.Pump");
	} else {
		this.hideSelection("path.Pump");
	}
}
NetworkView.prototype.showValves = function(bShow) {
	if (bShow == null || bShow) {
		this.showSelection("path.Valve");
	} else {
		this.hideSelection("path.Valve");
	}
}
NetworkView.prototype.showPipes = function(bShow) {
	if (bShow == null || bShow) {
		this.showSelection("#Pipes_" + this.nid);
	} else {
		this.hideSelection("#Pipes_" + this.nid);
	}
}
NetworkView.prototype.showSensors = function(bShow) {
	if (bShow == null || bShow) {
		this.showSelection("path.Sensor");
	} else {
		this.hideSelection("path.Sensor");
	}
}

NetworkView.prototype.showSelection = function(selection) {
	this.d3view.selectAll(selection).attr("visibility","inherit");
}

NetworkView.prototype.hideSelection = function(selection) {
	this.d3view.selectAll(selection).attr("visibility","hidden");
}
////////////////////////////////////////////////////////////////////
NetworkView.prototype.drawCoordinates = function(e) {
	//return;
	var p_max = this.convertToActual(this.m_data.Bounds.max.x, this.m_data.Bounds.max.y);
	var p_min = this.convertToActual(this.m_data.Bounds.min.x, this.m_data.Bounds.min.y);
	var max = 0
	var max = Math.max(max, Math.abs(p_max.x));
	var max = Math.max(max, Math.abs(p_max.y));
	var max = Math.max(max, Math.abs(p_min.x));
	var max = Math.max(max, Math.abs(p_min.y));
	//
	var p_range = this.convertToActualRange(this.m_data.Bounds.range.x, this.m_data.Bounds.range.y);
	var view = this.getNetworkViewBounds();
	var ratio = { "x": p_range.x / view.w, "y": p_range.y/view.h };
	var unitsPerPixel = ratio.x;
	if (ratio.y > ratio.x) unitsPerPixel = ratio.y;
	var log = Math.log(unitsPerPixel) / Math.log(10);
	var nFix = 0;
	if (log < 0) nFix = -1 * log + 1;
	//
	if (e) {
		var p = this.getActualGraphPointFromScreenXY(e.clientX - this.x, e.clientY - this.y);
		var x = p.x.toFixed(nFix);
		var y = p.y.toFixed(nFix);
		d3.select("#textMouseXY_" + this.nid).text("(" + x + ", " + y + ")");
	} else {
		d3.select("#textMouseXY_" + this.nid).text("( , )");		
	}
	var text = document.getElementById("textMouseXY_" + this.nid);
	var box = text.getBBox();
	var max = Math.log(max) / Math.log(10);
	var max = Math.floor(max) + 1;
	var center = 108 + max * 3 * 2;
	d3.select("#textMouseXY_" + this.nid)
		.attr("x", center)
		;
	d3.select("#rectMouseXY_" + this.nid)
		.attr("x", center - (box.width + 5) / 2)
		.attr("width", box.width + 5)
		.attr("fill",d3.rgb(255,255,255))
		.attr("fill-opacity",0.5)
		;
}

NetworkView.prototype.getNodeLocation = function(id) {
	var Node = this.getNodeData(id);
	var x = parseFloat(Node.x);
	var y = parseFloat(Node.y);
	var p = this.getScreenXYFromGraphPoint(x, y);
	return {"clientX": p.x + this.x, "clientY": p.y + this.y, "x":x, "y": y, "type": Node.Type};
}

NetworkView.prototype.getNodeData = function(id) {
	var index = this.m_data.NodeIds[id];
	var Node = this.m_data.Nodes[index];
	return Node;
}

NetworkView.prototype.drawNodePopupAtNode = function(id) {
	var p = this.getNodeLocation(id);
	this.NodePopup.redraw(p.clientX, p.clientY, id, p.type, true);
}

NetworkView.prototype.drawNodePopup = function(id, type, optional_e) {
	var e = optional_e ? optional_e : d3.event;
	this.NodePopup.redraw(e.clientX, e.clientY, id, type, true);
}
////////////////////////////////////////////////////////////////////
// called from SVGPan
NetworkView.prototype.updateZoom = function(z) {
	this.nStrokeWidth *= z;
	this.bZoomUpdated = true;
}

NetworkView.prototype.ZoomFull = function() {
	this.NodePopup.hide();
	this.ZoomTo(1000, "cubic-in-out", this.m_data.Center);
}

NetworkView.prototype.Zoom = function(nZoom, iter) {
	var m_this = this;
	this.NodePopup.hide();
//	if (this.ZoomTracker.or()) {
//		// use this to keep multiple clicks alive while busy zooming
//		if (iter > 200) return;
//		setTimeout(function() {
//				m_this.Zoom(nZoom, ++iter); 
//			},50);
//		return;
//	}
	var p = this.getGraphPointFromScreenCenter();
	var svg = document.getElementById(this.svgid);
	var m = svg.createSVGMatrix();
	var k = m.translate(p.x, p.y); //i dont know why this is here but it was in the code that i got from SVGPan.js
	var k = k.scale(nZoom);
	var k = k.translate(-p.x, -p.y);
	var g = document.getElementById(this.viewid);
	var m = g.getCTM();
	var m = m.multiply(k);
	var sEase = "cubic-in-out";
	var nDuration = 1000;
	var selection = "#" + this.viewid;
	var transform = "matrix(" + m.a + "," + m.b + "," + m.c + "," + m.d + "," + m.e + "," + m.f + ")";
	this.resizeNodes(sEase, nDuration, selection, transform);
}

NetworkView.prototype.ZoomNode = function(obj) {
	var m_this = this;
	this.bZoomNodeActive = true;
	this.NodePopup.hide();
	this.ZoomObj = obj;
	this.ZoomObj.Step = 0;
	//
	var cx = obj.x;
	var cy = obj.y;
	var id = obj.text;
	var type = obj.type;
	//
	var nScaleOld = this.getCurrentScale(); 
	var nScaleFull = this.getScaleFull(this.m_data);
	var bSkipZoomOut = (nScaleOld < 5 * nScaleFull);

	var fEnding = function() {
		var p = m_this.getScreenXYFromGraphPoint(parseFloat(cx),parseFloat(cy));
		m_this.NodePopup.redraw(p.x + m_this.x, p.y + m_this.y, id, type, true, false);
		m_this.bZoomNodeActive = false;		
	}
	var fStep2 = function() {
		m_this.ZoomObj.Step = 2;
		var nDuration = 1000;
		if (bSkipZoomOut) nDuration = 2000;
		var sEase = "circle-in";
		if (bSkipZoomOut) sEase = "cubic-in-out";
		m_this.ZoomTo(nDuration, sEase, {"x":cx,"y":cy,"zoom":10}, fEnding);
	}
	if (bSkipZoomOut) {
		//go straight to step 2
		var rmax = m_this.m_data.Bounds.range.max;
		fStep2();
	} else {
		var rmax = m_this.nStrokeWidth * 3000;
		m_this.ZoomObj.Step = 1;
		var p = this.getGraphPointFromScreenCenter();
		var p = d3.interpolate({"x":cx,"y":cy},{"x":p.x,"y":p.y})(0.5)
		var sEase = "circle-out";
		var nDuration = 1000;
		this.ZoomTo(nDuration, sEase, {"x":p.x,"y":p.y,"zoom":5}, fStep2);
	}
}

NetworkView.prototype.ZoomTo = function(nDuration, sEase, centerPoint, fNextStep) {
	this.NodePopup.hide();
	//
	if (!fNextStep)
		fNextStep = function(){;};
	//
	var dRangeX = this.m_data.Bounds.range.x / centerPoint.zoom;
	var dRangeY = this.m_data.Bounds.range.y / centerPoint.zoom;
	var dScale = this.getScaleAt({"w":dRangeX,"h":dRangeY});
	dScale = parseFloat(dScale.toPrecision(2));
	//
	var dX = -centerPoint.x;
	var b = this.getNetworkViewBounds();
	var dY = -centerPoint.y - b.h / dScale;
	var b = this.getNetworkViewBounds();
	dX = dX + b.w / dScale / 2;
	dY = dY + b.h / dScale / 2;
	//
	var selection = "#" + this.viewid;
	var transform = "rotate(180) scale( -" + dScale + "," + dScale + ") translate(" + dX + "," + dY + ")";
	this.resizeNodes(sEase, nDuration, selection, transform, fNextStep);
}

NetworkView.prototype.resize = function() {
	var m_this = this;
	var saved = this.saveView()
	var dotSize = this.nStrokeWidth * 5;
	var svgFile = (this.m_data.svgFile) ? this.m_data.svgFile : this.assembleSvg();
	if (true) {
		svgFile = svgFile.replace(/transform===view_transform===/, "transform='" + saved.transform + "'");
		svgFile = svgFile.replace(/r===junction_radius===/g, "r='" + dotSize + "'");
		svgFile = svgFile.replace(/===junction_radius===/g, "" + dotSize);
		svgFile = svgFile.replace(/===junction_radius_times_2===/g, "" + 2 * dotSize);
		svgFile = svgFile.replace(/stroke-width===pipe_stroke_width===/g, "stroke-width='" + 0.2 * dotSize + "'");
	}
	//
	this.m_data.dotSize = dotSize;
	this.replaceView(this.m_data, svgFile);
	this.updateToggle();
	//
	var selection = "path.Tank, path.Reservoir, path.Pump, path.Valve, path.Sensor";
	this.resizeNodes(null, 0, selection, null, function() {
		//this.resizeNodes(null, 1000, selection, null, null);
		if (m_this.m_data.m_SimList) {
			//TODO - color nodes by concentration
			m_this.drawSelectedNode();
		}
		if (m_this.m_data.m_ImpactList) {
			m_this.m_ImpactView.redrawImpact();
			//m_ImpactList.ImpactList.modifyNetwork();
		}
		if (m_this.m_data.m_InvList) {
			var sel = m_this.getNodes();
			var results = m_this.m_data.inversionResults;
			Inversion.modifyNetwork(sel, results);		
		}
	});
}

NetworkView.prototype.resizeNodes = function(sEase, nDuration, selection, transform, fNextStep) {
	var m_this = this;
	//
	if (sEase == null)
		sEase = "cubic-in-out";
	if (nDuration == null)
		nDuration = 1000;
	if (fNextStep == null) 
		fNextStep = function(){;};
	//
	var dR = this.nStrokeWidth * 5;
//	if (selection == null)
//		selection = "path.Pipe, path.Pipes, circle.Junction, path.Junctions, path.Tank, path.Reservoir, path.Pump, path.Valve, path.Sensor";
	//
	var sel = this.d3svg.selectAll(selection);
	//
	var change_transform = function(d,i) {
		if (d == null) return null;
		if (d.Type == "View") return transform;
		if (d.Type == "Junction") return "translate(" + d.x + "," + d.y + ")";
		return null;
	}
//	var change_r = function(d,i) {
//		if (d == null) return null;
//		if (d.Type != "Junction") return null;
//		return dR;
//	}
	var change_d = function(d,i) {
		if (d == null) return null;
		if (d.Type == "View") return null;
		if (d.Type == "Junction" ) return m_this.getJunctionPath (d.x, d.y);
		if (d.Type == "Tank"     ) return m_this.getTankPath     (d.x, d.y);
		if (d.Type == "Reservoir") return m_this.getReservoirPath(d.x, d.y);
		if (d.Type == "Pump"     ) return m_this.getPumpPath     (d.x1, d.y1, d.x2, d.y2); 
		if (d.Type == "Valve"    ) return m_this.getValvePath    (d.x1, d.y1, d.x2, d.y2);
		if (d.Type == "Pipe"     ) return m_this.getPipePath     (d.x1, d.y1, d.x2, d.y2);
		if (this.id.substring(0,7) == "Sensor_") {
			var i = m_this.m_data.NodeIds[d];
			if (i == null) return m_this.getSensorPath(0, 0);
			var x = m_this.m_data.Nodes[i].x;
			var y = m_this.m_data.Nodes[i].y;
			return m_this.getSensorPath(x, y);
		}
	}
	if (nDuration == 0) {
		sel .attr("transform", change_transform)
			.attr("d", change_d)
//			.attr("r", change_r)
//			.attr("stroke", this.change_stroke)
//			.attr("stroke-width", this.change_stroke_width)
//			.attr("stroke-opacity", this.change_stroke_opacity)
			;
		fNextStep();
	} else {
		sel	.transition()
			.duration(nDuration)
			.ease(sEase)
			.attr("transform", change_transform)
			.attr("d", change_d)
//			.attr("r", change_r)
//			.attr("stroke", this.change_stroke)
//			.attr("stroke-width", this.change_stroke_width)
//			.attr("stroke-opacity", this.change_stroke_opacity)
//			.each("start",function(d,i) {
//				console.log("s" + i);
//			})
			.each("end",function(d,i) {
				if (i == 0) fNextStep();
//				console.log("e" + i);
			})
			;
	}
}


NetworkView.prototype.change_stroke = function(d,i) {
	// use m_NetworkView instead of "this" because this function is passed as an argument to a different class...
	if (d == null) return null;
	if (this.id.substring(0,7) == "Sensor_") return d3.rgb(0,0,255);
	if (d.Type == "Pipe") return d3.rgb(0,0,0);
	var selectedNode = m_NetworkView.m_NodeGraph.getNodeId();
	if (m_NetworkView.m_data.m_SimList && d.id == selectedNode) return d3.rgb(200,0,0);
	return "none";
}
NetworkView.prototype.change_stroke_width = function(d,i) {
	if (d == null) return null;
	if (this.id.substring(0,7) == "Sensor_") return m_NetworkView.nStrokeWidth * 3.0; // TODO ???
	if (d.Type == "Pipe") 	return m_NetworkView.nStrokeWidth * 1.0;
	return m_NetworkView.nStrokeWidth * 2.0;
}
NetworkView.prototype.change_stroke_opacity = function(d,i) {
	if (d == null) return null;
	if (this.id.substring(0,7) == "Sensor_") return 0.7; // TODO ???
	if (m_NetworkView.m_NodeGraph.isVisible() && d.id == m_NetworkView.m_NodeGraph.getNodeId()) return 0.7;
	return m_NetworkView.dOpacity;
}


NetworkView.prototype.getSerializedString = function() {
	if (this.svgFile) return this.svgFile;
	this.svgFile = this.createSerializedString();
	return this.svgFile;
}

NetworkView.prototype.createSerializedString = function() {
	var serializer = new XMLSerializer();
	this.svgFile = serializer.serializeToString(this.d3svg[0][0]);
	return this.svgFile;
}

NetworkView.prototype.createSnapshot = function() {
	var serializer = new XMLSerializer();
	var text1 = serializer.serializeToString(this.d3rect[0][0]);
	var text2 = serializer.serializeToString(this.d3view[0][0]);
	var svgFile = "<svg xmlns='http://www.w3.org/2000/svg'>" + text1 + text2 + "</svg>";
	return svgFile;
}

NetworkView.prototype.getSelection = function(data) {
	if (data == null) data = this.m_data;
	if (data.m_InpList) return data.uuid;
	if (data.m_SimList && m_SimList.getSelectedData()) return m_SimList.getSelectedData().id;
	if (data.m_ImpactList) return m_ImpactList.ImpactList.getSelectionUuid();
	if (data.m_InvList) return m_InversionList.InversionList.getSelectionUuid();
	return null;
}

NetworkView.prototype.assembleSvg = function(data/*optional*/) {
	if (data == null) data = this.m_data;
	var pipes = (true ) ? data.svgPipePath : data.svgPipes;
	var nodes = (false) ? data.svgNodePath : data.svgNodes;
	var s = data.svgStart;
	/*if (this.getToggleValue(this.togglePipeId     )) */s += pipes;
	/*if (this.getToggleValue(this.toggleJunctionId )) */s += nodes;
	/*if (this.getToggleValue(this.toggleReservoirId)) */s += data.svgReservoirs;
	/*if (this.getToggleValue(this.toggleTankId     )) */s += data.svgTanks;
	/*if (this.getToggleValue(this.togglePumpId     )) */s += data.svgPumps;
	/*if (this.getToggleValue(this.toggleValveId    )) */s += data.svgValves;
	s += data.svgEnd;
	return s;
}

NetworkView.prototype.replaceView = function(data, svgFile) {
	var div = document.createElement("div");
	div.innerHTML = svgFile;
	var svg = div.firstChild;
	var view = svg.childNodes[1];
	this.d3view.remove();
	var before = this.d3svg[0][0].childNodes[1];
	this.d3svg[0][0].insertBefore(view, before);
	this.d3view = d3.select(view);
	m_SVGPan.setRoot(this.viewid);
	//
	this.d3view.datum({"Type":"View"});
	this.d3view.select("#" + "Pipes_" + this.nid).datum({"Type":"Pipe"});
	//
	this.nStrokeWidth = 0.2 * data.dotSize;
	//
	for (var i = 0; i < data.Nodes.length; i++) {
		var node = data.Nodes[i];
		this.d3view.select("#" + "Node" + "_" + node.id).datum(node);
	}
	for (var i = 0; i < data.Links.length; i++) {
		var link = data.Links[i];
		this.d3view.select("#" + link.Type + "_" + link.ID).datum(link);
	}
	//
	if (data.m_InvList) this.drawSensors(data.Sensors);
	if (data.m_ImpactList) this.drawSensors(data.Sensors);
	//
	this.svgFile = svgFile;
}

NetworkView.prototype.viewNetwork = function(data) {
	var m_this = this;
	this.m_data = data;
	this.loadToggle(data.toggle);
	//d3.select("body").style("overflow","hidden");//added document.body.style.overflow=hidden to gui.html
	this.d3title.attr("display", "none");
	this.hideNodeGraph         (!data.m_SimList   );
	this.hideImpactView        (!data.m_ImpactList);
	this.hideInversionNavigator(!data.m_InvList   ||  !!data.m_Event);
	this.hideInversionGrid     (!data.m_InvList   );
	this.showClose             (!data.m_Event     );
	this.hideEventButtons      (!data.m_Event     );
	//
	var opacity = (data.m_Event) ? 0.0 : 0.5;
	this.d3rectbg.attr("fill-opacity", opacity)
	//
	var sNewView = "";
	sNewView = (data.m_InpList   ) ? "m_InpList"    : sNewView;
	sNewView = (data.m_SimList   ) ? "m_SimList"    : sNewView;
	sNewView = (data.m_ImpactList) ? "m_ImpactList" : sNewView;
	sNewView = (data.m_InvList   ) ? "m_InvList"    : sNewView;
	sNewView = (data.m_Event     ) ? "m_Event"      : sNewView;
	var sSelectionUuid = data.selectionUuid ? data.selectionUuid : this.getSelection(data);
	this.setSelectionUuid(sSelectionUuid);
	//
	var bSameUuid = this.getUuid() == data.uuid;
	var bSameView = this.getView() == sNewView;
	var bSameSelection = false; // this.getSelectionUuid() == sSelectionUuid;
	var bSame = (sNewView == "m_SimList") ? (bSameView && bSameSelection) : (bSameView && bSameUuid);
	if (bSame) {
		this.removeSensors();
		if (data.m_InvList) {
			this.drawSensors(data.Sensors); // modified from "(data)"
		} else if (data.m_ImpactList) {
			this.drawSensors(data.Sensors);
		} else {
			this.getNodes().attr("fill-opacity", this.dOpacity);
			this.getJunctions().attr("fill", d3.rgb(0,0,0));
			this.getReservoirs().attr("fill", d3.rgb(0,155,0));
			this.getTanks().attr("fill", d3.rgb(0,0,155));
		}
		this.updateToggle();
		this.drawSelectedNode();
		this.show();
		m_Waiting.hide();
		return;
	}
	//
	var svgFile = (data.svgFile) ? data.svgFile : this.assembleSvg(data);
	//
	if (data.dotSize) {
		var drawTransform = (data.drawTransform) ? data.drawTransform : "";
		svgFile = svgFile.replace(/transform===view_transform===/, "transform='" + drawTransform + "'");
		svgFile = svgFile.replace(/r===junction_radius===/g, "r='" + data.dotSize + "'");
		svgFile = svgFile.replace(/===junction_radius===/g, "" + data.dotSize);
		svgFile = svgFile.replace(/===junction_radius_times_2===/g, "" + 2 * data.dotSize);
		svgFile = svgFile.replace(/stroke-width===pipe_stroke_width===/g, "stroke-width='" + 0.2 * data.dotSize + "'");
	}
	//
	if (svgFile) {
		this.setUuid(data.uuid);
		this.setView(sNewView);
		this.NodeFilter.setText("");
		//
		this.replaceView(data, svgFile);
		this.updateToggle();
		//
		if (data.drawTransform == null) {
			ResizeWindow(); // make sure the Network window is full size before we try to get the scale factor
			this.ZoomTo(0, "", data.Center);
		}
		//
		d3.select(div).remove();
		this.drawCoordinates(d3.event);
		this.drawSelectedNode();
		this.saveView();
		this.show();
	}
	//
	m_Waiting.hide();
	//
	if (data.filterList) {
		var div = document.createElement("div");
		div.innerHTML = data.filterList;
		var select = div.firstChild;
		d3.select(div).remove();
		this.NodeFilter.setSelectElement(select, [data.Junctions, data.Tanks, data.Reservoirs]);
		this.NodeFilter.resizeList([data.Junctions, data.Tanks, data.Reservoirs]);
	} else {
		m_this.NodeFilter.createList([data.Junctions, data.Tanks, data.Reservoirs]);
	}	
/*Arpan*/
	if (false) {
		var sensors = [];
		for (var node in data.Junctions) {
			var obj = data.Junctions[node];
			for (var sensor in this.m_data.Sensors) {
				if (obj.id == this.m_data.Sensors[sensor])
					sensors.push(obj);
			}
		}
		for (var node in data.Tanks) {
			var obj = data.Tanks[node];
			for (var sensor in this.m_data.Sensors) {
				if (obj.id == this.m_data.Sensors[sensor])
					sensors.push(obj);
			}
		}
		for (var node in data.Reservoirs) {
			var obj = data.Reservoirs[node];
			for (var sensor in this.m_data.Sensors) {
				if (obj.id == this.m_data.Sensors[sensor])
					sensors.push(obj);
			}
		}
		this.NodeFilter.update(sensors);
	}
}

NetworkView.prototype.saveView = function() {
	var transform = this.d3view.attr("transform");
	Couch.SetValue(this.m_data.uuid, "drawTransform", transform); // ===transform===
	//Couch.SetValue(data.uuid, "dotsize", dotSize);
	return {"transform": transform};
}

NetworkView.prototype.drawSelectedNode = function(oldId) {
	// if this is the Simulation View (this.m_data.m_SimList == true)
	var id = (this.m_data && this.m_data.m_SimList) ? m_SimList.getSelectedData().value.selectedNode : null;
	if (id) {
		var sel = this.getNode(id);
		sel
			.attr("stroke", this.change_stroke)
			.attr("stroke-width", this.change_stroke_width)
			.attr("stroke-opacity", this.change_stroke_opacity)
			;
	}
	// the old one will be deselected since there is a condition in the "change_" routines to find the ".selectedNode"
	if (oldId) {
		var sel = this.getNode(oldId);
		sel
			.attr("stroke", this.change_stroke)
			.attr("stroke-width", this.change_stroke_width)
			.attr("stroke-opacity", this.change_stroke_opacity)
			;
	}
}

NetworkView.prototype.drawSensors = function(sensors) {
	var m_this = this;
	if (sensors == null) return;
	// iterate in reverse to help with deleting while iterating.
	for (var i = sensors.length - 1; i >= 0; i--) {
		if (sensors[i].trim().length == 0) 
			sensors.splice(i, 1);
	}
	// create sensors
	this.d3view.selectAll("path.Sensor")
		.data(sensors)
		.enter()
		.insert("path")
		//.attr("class", "Node" + " " + "Sensor" + "_" + this.nid)
		.attr("id", function(d,i) { return "Sensor" + "_" + d; }) // TODO - this doesnt account for this.nid
		.attr("class", "Node" + " " + "Sensor")
		.attr("data-id", function(d,i) { return d; })
		.attr("fill", "none")
		.attr("d", function(d,i) { 
			var i = m_this.m_data.NodeIds[d];
			var x = m_this.m_data.Nodes[i].x;
			var y = m_this.m_data.Nodes[i].y;
			return m_this.getSensorPath(x, y);
		})
		//
		.attr("stroke", this.change_stroke)
		.attr("stroke-width", this.change_stroke_width)
		.attr("stroke-opacity", this.change_stroke_opacity)
		//
		;
	//
	if (false) { //	use this to put sensors behind the pipes and nodes
		var view = this.d3view[0][0];
		var sensors = this.getSensors();
		for (var i = 0; i< sensors[0].length; i++) {
			if (false) { // behind the pipes
				view.insertBefore(sensors[0][i], view.firstChild);
			} else { // behind the nodes
				view.insertBefore(sensors[0][i], view.childNodes[1]);
			}
		}
	}
}

// obselete. only called from drawNetwork_old
NetworkView.prototype.removeAll = function() {
	this.d3view.selectAll("circle.Nodes").remove();
	this.d3view.selectAll("circle.Junction").remove();
	this.d3view.selectAll("path.Tank").remove();
	this.d3view.selectAll("path.Reservoir").remove();
	this.d3view.selectAll("path.Pump").remove();
	this.d3view.selectAll("path.Valve").remove();
	this.d3view.selectAll("path.Pipe").remove();
	this.d3view.selectAll("path.Sensor").remove();
}
// obsolete. only here for reference if we ever want to bring back progress pie graph or calibration grid
NetworkView.prototype.drawNetwork_old = function(data) {
	var m_this = this;
	var link = "";
	this.d3image.attr("visibility","hidden");
	if (data && data.map && data.map.node1 && data.map.node2) {
		var b1=false,b2=false;var x1=0,y1=0,x2=0,y2=0;
		for (var i = 0; i < data.Nodes.length; i++) {
			if (data.Nodes[i].id == data.map.node1.id) {
				b1 = true;
				x1 = parseFloat(data.Nodes[i].x);
				y1 = parseFloat(data.Nodes[i].y);
			}
			if (data.Nodes[i].id == data.map.node2.id) {
				b2 = true;
				x2 = parseFloat(data.Nodes[i].x);
				y2 = parseFloat(data.Nodes[i].y);
			}
			if (b1 && b2) break;
		}
		this.scaleXLon = d3.scale.linear().domain([x1,x2]).range([data.map.node1.lon,data.map.node2.lon]);
		this.scaleYLat = d3.scale.linear().domain([y1,y2]).range([data.map.node1.lat,data.map.node2.lat]);
		this.map = data.map;
		//
		var sWeight = "weight:0";
		if (this.bCalibrating) sWeight = "weight:1";
		var sMapColor = "color:0xff0000ff";
		var sPath = this.scaleYLat(data.Center.y) + "," + this.scaleXLon(data.Center.x);
		var x = data.Center.x;
		var y = data.Center.y;
		var r = data.Bounds.range.min / 10;
		var n = 6;
		for (var i = 0; i < n; i++) {
			sPath = sPath
				+ "|" + this.scaleYLat(+1*(i*r)+y) + "," + this.scaleXLon(+1*(i*r)+x)
				+ "|" + this.scaleYLat(-1*(i*r)+y) + "," + this.scaleXLon(+1*(i*r)+x)
				+ "|" + this.scaleYLat(-1*(i*r)+y) + "," + this.scaleXLon(-1*(i*r)+x)
				+ "|" + this.scaleYLat(+1*(i*r)+y) + "," + this.scaleXLon(-1*(i*r)+x)
				+ "|" + this.scaleYLat(+1*(i*r)+y) + "," + this.scaleXLon(+1*(i*r)+x)
				;
		}
		link = "http://maps.googleapis.com/maps/api/staticmap"
			+ "?center=" + this.scaleYLat(data.Center.y) + "," + this.scaleXLon(data.Center.x)
			+ "&path=" + sWeight + "|" + sMapColor + "|" + sPath
			+ "&size=640x640"
			+ "&scale=2"
//			+ "&zoom=9"
//			+ "&maptype=roadmap"
			+ "&sensor=false"
			;
		var s = 0.0818;
		var s = data.map.scale;
		var x = -1*(s*640-data.Center.x)/s;
		var y = +1*(s*640-data.Center.y)/s;
		this.d3image.attr("transform",this.imageTransform + " scale("+s+") translate(0,-1280)");
		this.d3image.attr("transform",this.imageTransform + " scale("+s+") translate(0,-1280) translate("+x+","+y+")");
		this.d3image.attr("visibility","inherit");
	}
	this.d3image.attr("xlink:href",link);
	//
	this.removeAll();
	//
	var path = this.getPipesPath(data);
//	var path = this.getEverythingPath(data);
	if (this.bCalibrating) {
		path = this.getCalibratePath(data);
		m_Waiting.hide();
	}
	//
	this.d3view.selectAll("path.Pipe")
		.data([{"Type":"Pipe"}])
		.enter()
		.append("path")
		.attr("id", "Pipes_" + this.nid)
		.attr("class", "Pipe")
		.attr("d", path)
		.attr("stroke", d3.rgb(0,0,0))
		.attr("stroke-width", this.nStrokeWidth)
		.attr("stroke-opacity", this.dOpacity)
		.attr("stroke-linecap", "round")
		.attr("fill", "none")
		;
	//
	if (!this.bCalibrating) {
		var nGroupSize1 = 100;
		var nGroups1 = parseInt(data.Links.length / nGroupSize1) + 1;
		var nGroupSize2 = 100;
		var nGroups2 = parseInt(data.Nodes.length / nGroupSize2) + 1;
		var nGroupSize  = 100;
		var nGroups  = parseInt(data.Nodes.length / nGroupSize) + 1;
		m_Waiting.setCountTotal(nGroups1 + nGroups2 + nGroups);
		//
		data.Pumps = [];
		data.Valves = [];
		data.Pipes = [];
		var separateLinks = function(fNextStep1) {
			var process1 = function(nGroups1, igroup1) {
				var istart1 = nGroupSize1 * (igroup1    );
				var iend1   = nGroupSize1 * (igroup1 + 1);
				iend1 = (iend1 > data.Links.length) ? data.Links.length : iend1;
				for (var i1 = istart1; i1 < iend1; i1++) {
					var link = data.Links[i1];
					if      (link.Type == "Pump" ) data.Pumps.push(link);
					else if (link.Type == "Valve") data.Valves.push(link);
					else if (link.Type == "Pipe" ) data.Pipes.push(link);
				}
			};
			var igroup1 = 0;
			var busy1 = false;
			var processor1 = setInterval(function() {
				if (!busy1) {
					busy1 = true;
					process1(nGroups1, igroup1);
					if (++igroup1 == nGroups1) {
						clearInterval(processor1);
						if (fNextStep1) fNextStep1();
					}
					busy1 = false;
					m_Waiting.update();
				}
			}, 15);
		}
		data.Junctions  = [];
		data.Tanks      = [];
		data.Reservoirs = [];
		var separateNodes = function(fNextStep2) {
			var process2 = function(nGroups2, igroup2) {
				var istart2 = nGroupSize2 * (igroup2    );
				var iend2   = nGroupSize2 * (igroup2 + 1);
				iend2 = (iend2 > data.Nodes.length) ? data.Nodes.length : iend2;
				for (var i2 = istart2; i2 < iend2; i2++) {
					var node = data.Nodes[i2];
					if (node.Type == "Junction" ) data.Junctions.push(node);
					if (node.Type == "Tank"     ) data.Tanks.push(node);
					if (node.Type == "Reservoir") data.Reservoirs.push(node);
				}
			};
			var igroup2 = 0;
			var busy2 = false;
			var processor2 = setInterval(function() {
				if (!busy2) {
					busy2 = true;
					process2(nGroups2, igroup2);
					if (++igroup2 == nGroups2) {
						clearInterval(processor2);
						if (fNextStep2) fNextStep2();
					}
					busy2 = false;
					m_Waiting.update();
				}
			}, 15);
		}
		//
		var drawJunctions = function(fNextStep) {
			var start = new Date().getTime();
			var process = function(nGroups, igroup) {
				var istart = nGroupSize * (igroup);
				var iend   = nGroupSize * (igroup + 1);
				iend = (iend > data.Junctions.length) ? data.Junctions.length : iend;
				m_this.d3view.selectAll("circle.JunctionGroup" + "_" + igroup)
					.data(data.Junctions.slice(istart,iend))
					.enter()
					.insert("circle")
					.attr("class", "Node Junction JunctionGroup" + "_" + igroup)
					.attr("fill", d3.rgb(0,0,0))
					.attr("fill-opacity", m_this.dOpacity)
					.attr("id", function(d,i) { return "Node_" + d.id; })
					.attr("r", m_this.nStrokeWidth * 5)
					//.attr("cx", function(d,i) { return d.x; })
					//.attr("cy", function(d,i) { return d.y; })
					.attr("transform", function(d,i) { return "translate(" + d.x + "," + d.y + ")"; })
					.attr("stroke", "none")
					;
			}
			var nGroups = parseInt(data.Junctions.length / nGroupSize) + 1;
			var igroup = 0;
			var busy = false;
			var processor = setInterval(function() {
				if (!busy) {
					busy = true;
					process(nGroups, igroup);
					if (++igroup == nGroups) {
						clearInterval(processor);
						if (fNextStep) fNextStep();
					}
					busy = false;
					m_Waiting.update();
				}
			}, 15);
		};
		var drawNodes = function() {
			drawJunctions(function() {
				m_this.hide(false); //////////////////////////////
				m_Waiting.hide();
				m_this.NodeFilter.masker.setValues([true,true,true,false,false,false]);
				m_this.NodeFilter.update([data.Junctions, data.Tanks, data.Reservoirs]);
				drawTanks();
				drawReservoirs();
				drawSensors();
				drawPumps();
				drawValves();
				m_this.addListenersForSelection("path.Tank");
				m_this.addListenersForSelection("path.Reservoir");
				m_this.addListenersForSelection("path.Sensor");
			});
		};
		var drawTanks = function() {
			m_this.d3view.selectAll("path.Tank")
				.data(data.Tanks)
				.enter()
				.insert("path")
				.attr("class", "Node Tank")
				.attr("id", function(d,i) { return "Node_" + d.id; })
				.attr("d", function(d,i) { return m_this.getTankPath(d.x, d.y); })
				.attr("fill", d3.rgb(0,0,155))
				.attr("fill-opacity", m_this.dOpacity)
				.attr("stroke", "none")
				;
		}
		var drawReservoirs = function() {
			m_this.d3view.selectAll("path.Reservoir")
				.data(data.Reservoirs)
				.enter()
				.insert("path")
				.attr("class", "Node Reservoir")
				.attr("id", function(d,i) { return "Node_" + d.id;	})
				.attr("d", function(d,i) { return m_this.getReservoirPath(d.x, d.y); })
				.attr("fill", d3.rgb(0,155,0))
				.attr("fill-opacity", m_this.dOpacity)
				.attr("stroke","none")
				;
		}
		//
		var drawPumps = function() {
			m_this.d3view.selectAll("path.Pump")
				.data(data.Pumps)
				.enter()
				.insert("path")
				.attr("id", function(d,i) { return "Pump" + "_" + d.ID; })
				.attr("class", "Link Pump")
				.attr("stroke", "none")
				.attr("fill", d3.rgb(155,155,0))
				.attr("fill-opacity", m_this.dOpacity)
				.attr("d", function(d,i) { 	return m_this.getPumpPath(d.x1, d.y1, d.x2, d.y2); 	})
				;
		}
		var drawValves = function() {
			m_this.d3view.selectAll("path.Valve")
				.data(data.Valves)
				.enter()
				.insert("path")
				.attr("id", function(d,i) { return "Valve" + "_" + d.ID; })
				.attr("class", "Link Valve")
				.attr("stroke", "none")
				.attr("fill", d3.rgb(0,155,155))
				.attr("fill-opacity", m_this.dOpacity)
				.attr("d", function(d,i) { 	return m_this.getValvePath(d.x1, d.y1, d.x2, d.y2); })
				;
		}
		var drawSensors = function() {
			if (data.m_InvList) {
				m_this.d3view.selectAll("path.Sensor")
					.data(data.Sensors)
					.enter()
					.insert("path")
					.attr("id", function(d,i) { return "Sensor" + "_" + d; }) 
					.attr("class", "Node Sensor")
					.attr("stroke", d3.rgb(255,255,255))
					.attr("stroke-opacity", 1)
					.attr("stroke-width", m_this.nStrokeWidth * 1.5)
					.attr("fill", "none")
					.attr("d", function(d,i) { 
						var i = m_this.m_data.NodeIds[d];
						var x = m_this.m_data.Nodes[i].x;
						var y = m_this.m_data.Nodes[i].y;
						return m_this.getSensorPath(x, y);
					})
					;
			}
		}
		separateLinks(function() {
		separateNodes(function() {
		drawNodes(function() {
		});
		});
		});
	}
	this.addListenersForSelection("path.Tank");
	this.addListenersForSelection("path.Reservoir");
	this.addListenersForSelection("path.Pump");
	this.addListenersForSelection("path.Valve");
	this.addListenersForSelection("path.Sensor");
	this.NodePopup.create();
}

NetworkView.prototype.resizeDot = function(w) {
	var m_this = this;
	this.d3svg.select("#" + this.dotid).attr("visibility", "inherit");
	if (this.dotWaitHandle) clearTimeout(this.dotWaitHandle);
	this.dotWaitHandle = setTimeout(function() { 
			m_this.hideDot();
		}, 2000);
	this.d3svg.select("#" + this.dotid).attr("d", this.getDotPath(null, null, w));
}

NetworkView.prototype.hideDot = function() {
	var m_this = this;
	m_Waiting.show("Resizing Nodes...");
	this.d3svg.select("#" + this.dotid).attr("visibility", "hidden");
	setTimeout(function() {
			m_this.resize();
			m_Waiting.hide();
		}, 15);
}

NetworkView.prototype.getDotPath = function(/*Optional*/ x, /*Optional*/ y, /*Optional*/ w) {
	if (w == null) w = this.nStrokeWidth;
	if (x == null) x = this.d3svg.select("#" + this.dotid).attr("data-x");
	if (y == null) y = this.d3svg.select("#" + this.dotid).attr("data-y");
	var x = parseFloat(x);
	var y = parseFloat(y);
	var r = 5 * w;
	var s = this.getCurrentScale();
	if (s < 0) return "M 0 0";
	var r = 5 * w * s;
	var path = ""
	+ " M " + x + " " + y + " "
	+ " a " + (+1.0*r) + " " + (+1.0*r) + " 0 1 0 " + (+2.0*r) + " " + (+0.0*r) + " "
	+ " a " + (+1.0*r) + " " + (+1.0*r) + " 0 1 0 " + (-2.0*r) + " " + (+0.0*r) + " "
	;
	return path;
}

/*kate*/
NetworkView.prototype.getSensorPath = function(x, y, /*Optional*/ w) {
	if (w == null) w = this.nStrokeWidth;
	var x = parseFloat(x);
	var y = parseFloat(y);
	var r = w * 9.0;
	var path = ""
	+ " M " + x + " " + y + " "
	+ " m " + (+0.0*r) + " " + (+1.0*r) + " "
	+ " l " + (-1.0*r) + " " + (+0.0*r) + " "
	+ " l " + (+0.0*r) + " " + (-2.0*r) + " "
	+ " l " + (+2.0*r) + " " + (+0.0*r) + " "
	+ " l " + (+0.0*r) + " " + (+2.0*r) + " "
	+ " l " + (-1.0*r) + " " + (+0.0*r) + " "
	;
	return path;
}

NetworkView.prototype.getPipePath = function(x1, y1, x2, y2, /*Optional*/ w) {
	if (w == null) w = this.nStrokeWidth;
	var x1 = parseFloat(x1);
	var y1 = parseFloat(y1);
	var x2 = parseFloat(x2);
	var y2 = parseFloat(y2);
	var path = ""
	+ " M " + x1 + " " + y1
	+ " L " + x2 + " " + y2
	;
	return path;
}

NetworkView.prototype.getPumpPath = function(x1, y1, x2, y2, /*Optional*/ w) {
	if (w == null) w = this.nStrokeWidth;
	var x1 = parseFloat(x1);
	var y1 = parseFloat(y1);
	var x2 = parseFloat(x2);
	var y2 = parseFloat(y2);
	var cx = 0.5 * (x1 + x2);
	var cy = 0.5 * (y1 + y2);
	var lx = x2 - x1;
	var ly = y2 - y1;
	var l = Math.sqrt(lx * lx + ly * ly);
	var sx = (lx > 0) ? 1 : -1;
	var sy = (ly > 0) ? 1 : -1;
	var n = 1.2 * w;
	var nx = lx * n / l;
	var ny = ly * n / l;
	var r = 3 * n;
	var path = ""
	+ " M " + (cx - r) + " " + cy
	+ " a " + r + " " + r + " 0 0 0 " + (+r) + " " + (+r)
	+ " h " + ( 1.60 * r)
	+ " v " + (-0.80 * r)
	+ " h " + (-0.60 * r)
	+ " v " + (-0.20 * r)
	+ " a " + r + " " + r + " 0 0 0 " + (-0.5*r) + " " + (-0.87*r)
	+ " l " + ( 0.8 * r) + " " + (-0.38*r)
	+ " v " + (-0.2 * r)
	+ " h " + (-2.6 * r)
	+ " v " + ( 0.2 * r)
	+ " l " + ( 0.8 * r) + " " + ( 0.38*r)
	+ " a " + r + " " + r + " 0 0 0 " + (-0.5*r) + " " + (+0.87*r)
	+ " M " + (cx   ) + " " + (cy   )
	+ " L " + (x1+ny) + " " + (y1-nx)
	+ " A " + (n/2)   + " " + (n/2)   + " 0 0 0 " + (x1-ny) + " " + (y1+nx)
	+ " L " + (cx   ) + " " + (cy   )
	+ " L " + (x2+ny) + " " + (y2-nx)
	+ " A " + (n/2)   + " " + (n/2)   + " 0 0 1 " + (x2-ny) + " " + (y2+nx)
	+ " L " + (cx   ) + " " + (cy   )
	;
	return path;
}

NetworkView.prototype.getValvePath = function(x1, y1, x2, y2, /*Optional*/ w) {
	if (w == null) w = this.nStrokeWidth;
	var x1 = parseFloat(x1);
	var y1 = parseFloat(y1);
	var x2 = parseFloat(x2);
	var y2 = parseFloat(y2);
	var lx = x2 - x1;
	var ly = y2 - y1;
	var l = Math.sqrt(lx * lx + ly * ly);
	var n = 4 * w;
	var nx = lx * n / l;
	var ny = ly * n / l;
	var cx = 0.5 * (x1 + x2);
	var cy = 0.5 * (y1 + y2);
	var path = ""
	+ " M " + (cx+0.0*nx+0.2*ny) + " " + (cy+0.0*ny-0.2*nx)
	+ " L " + (cx+1.0*nx+1.0*ny) + " " + (cy+1.0*ny-1.0*nx)
	+ " L " + (cx+1.4*nx+1.0*ny) + " " + (cy+1.4*ny-1.0*nx)
	+ " L " + (cx+1.4*nx+0.1*ny) + " " + (cy+1.4*ny-0.1*nx)
	+ " L " + (x2+0.0*nx+0.3*ny) + " " + (y2+0.0*ny-0.3*nx)
	+ " A " + (0.3*n) + " " + (0.3*n) + " 0 0 1 " + (x2+0.0*nx-0.3*ny) + " " + (y2+0.0*ny+0.3*nx)
	+ " L " + (cx+1.4*nx-0.1*ny) + " " + (cy+1.4*ny+0.1*nx)
	+ " L " + (cx+1.4*nx-1.0*ny) + " " + (cy+1.4*ny+1.0*nx)
	+ " L " + (cx+1.0*nx-1.0*ny) + " " + (cy+1.0*ny+1.0*nx)
	+ " L " + (cx+0.0*nx-0.2*ny) + " " + (cy+0.0*ny+0.2*nx)
	+ " L " + (cx-0.0*nx-0.2*ny) + " " + (cy-0.0*ny+0.2*nx)
	+ " L " + (cx-1.0*nx-1.0*ny) + " " + (cy-1.0*ny+1.0*nx)
	+ " L " + (cx-1.4*nx-1.0*ny) + " " + (cy-1.4*ny+1.0*nx)
	+ " L " + (cx-1.4*nx-0.1*ny) + " " + (cy-1.4*ny+0.1*nx)
	+ " L " + (x1-0.0*nx-0.3*ny) + " " + (y1-0.0*ny+0.3*nx)
	+ " A " + (0.3*n) + " " + (0.3*n) + " 0 1 1 " + (x1-0.0*nx+0.3*ny) + " " + (y1-0.0*ny-0.3*nx)
	+ " L " + (cx-1.4*nx+0.1*ny) + " " + (cy-1.4*ny-0.1*nx)
	+ " L " + (cx-1.4*nx+1.0*ny) + " " + (cy-1.4*ny-1.0*nx)
	+ " L " + (cx-1.0*nx+1.0*ny) + " " + (cy-1.0*ny-1.0*nx)
	+ " L " + (cx-0.0*nx+0.2*ny) + " " + (cy-0.0*ny-0.2*nx)
	+ " Z "
	;
	return path;
}

NetworkView.prototype.getTankPath = function(x, y, /*Optional*/ w) {
	if (w == null) w = this.nStrokeWidth;
	var x = parseFloat(x);
	var y = parseFloat(y);
	var r = 5 * w;
	var path = ""
	+ " M " + (x-0.7*r) + " " + (y-1.0*r)
	+ " l " + (0-0.0*r) + " " + (0+2.0*r)
	+ " l " + (0+0.1*r) + " " + (0+0.0*r)
	+ " l " + (0+0.2*r) + " " + (0-0.2*r)
	+ " l " + (0+0.2*r) + " " + (0+0.2*r)
	+ " l " + (0+0.2*r) + " " + (0-0.2*r)
	+ " l " + (0+0.2*r) + " " + (0+0.2*r)
	+ " l " + (0+0.2*r) + " " + (0-0.2*r)
	+ " l " + (0+0.2*r) + " " + (0+0.2*r)
	+ " l " + (0+0.1*r) + " " + (0+0.0*r)
	+ " l " + (0-0.0*r) + " " + (0-2.0*r)
	+ " a " + r + " " + r + " 0 0 0 " + (0-1.4*r) + " " + (0+0.0*r)
	;
	return path;
}

NetworkView.prototype.getReservoirPath = function(x, y, /*Optional*/ w) {
	if (w == null) w = this.nStrokeWidth;
	var x = parseFloat(x);
	var y = parseFloat(y);
	var r = 5 * w;
	var path = ""
	+ " M " + (x+0.00*r) + " " + (y-0.60*r)
	+ " A " + (0.5*r) + " " + (0.5*r) + " 0 1 1 " + (x+0.57*r) + " " + (y-0.19*r)
	+ " A " + (0.5*r) + " " + (0.5*r) + " 0 1 1 " + (x+0.35*r) + " " + (y+0.49*r)
	+ " A " + (0.5*r) + " " + (0.5*r) + " 0 1 1 " + (x-0.35*r) + " " + (y+0.49*r)
	+ " A " + (0.5*r) + " " + (0.5*r) + " 0 1 1 " + (x-0.57*r) + " " + (y-0.19*r)
	+ " A " + (0.5*r) + " " + (0.5*r) + " 0 1 1 " + (x+0.00*r) + " " + (y-0.60*r)
	;
	return path;
}

NetworkView.prototype.getJunctionPath = function(x, y, /*Optional*/ w) {
	if (w == null) w = this.nStrokeWidth;
	var x = parseFloat(x);
	var y = parseFloat(y);
	var r = 5 * w;
	var path = ""
	+ " M " + (x+r) + " " + y
	+ " a " + r + " " + r + " 0 0 0 " + (-2*r) + " 0 "
	+ " a " + r + " " + r + " 0 0 0 " + (+2*r) + " 0 "
	;
	return path;
}
// obsolete.
NetworkView.prototype.getJunctionsPath = function(data) {
	var path = "M 0 0";
	for (var i=0; i < data.Nodes.length; i++) {
		if (data.Nodes[i].Type == "Junction") {
			path += this.getJunctionPath(data.Nodes[i].x, data.Nodes[i].y);
		}
	}
	return path;
}
// obsolete.
NetworkView.prototype.getPipesPath = function(data) {
	var path = "M 0 0";
	if (data.Links == null) return path;
	for (var i=0; i < data.Links.length; i++) {
		if (data.Links[i].Type == "Pipe") {
			path += " " + "M";
			path += " " + data.Links[i].x1;
			path += " " + data.Links[i].y1;
			path += " " + "L";
			path += " " + data.Links[i].x2;
			path += " " + data.Links[i].y2;
		}
	}
	return path;
}
// obsolete.
NetworkView.prototype.getEverythingPath = function(data) {
	var path = ""
	path += this.getPipesPath(data);
	path += this.getJunctionsPath(data);
	return path;	
}
// obsolete.
NetworkView.prototype.getCalibratePath = function(data) {
	var path = "M 0 0 ";
	var x = data.Center.x;
	var y = data.Center.y;
	var r = data.Bounds.range.min / 10;
	var n = 6;
	for (var i = 0; i < n; i++) {
		path = path 
			+ "M" + (+1*(i*r)+x) + " " + (+1*(i*r)+y) + " "
			+ "L" + (-1*(i*r)+x) + " " + (+1*(i*r)+y) + " "
			+ "L" + (-1*(i*r)+x) + " " + (-1*(i*r)+y) + " "
			+ "L" + (+1*(i*r)+x) + " " + (-1*(i*r)+y) + " "
			+ "L" + (+1*(i*r)+x) + " " + (+1*(i*r)+y) + " "
			;
	}
	return path;
}

// called by the global function, ResizeWindow()
NetworkView.prototype.resizeWindow = function(w, h) {
	this.d3svgbg
		.style("width" , convertToPx(w))
		.style("height", convertToPx(h))
		;
	this.d3rectbg
		.attr("width" , w)
		.attr("height", h)
		;
	d3.select("#gNodeGraph" + "_" + this.nid)
		.style("top", convertToPx(h - 200 - NETWORK_VIEW_BORDER - 10))
		;
	this.d3svg
		.style("width",  convertToPx(w - 2 * NETWORK_VIEW_BORDER))
		.style("height", convertToPx(h - 2 * NETWORK_VIEW_BORDER))
		;
	d3.select("#" + this.rectid)//this.d3rect
		.attr("width",  w - 2 * NETWORK_VIEW_BORDER)
		.attr("height", h - 2 * NETWORK_VIEW_BORDER)
		;
	d3.select("#" + this.closerectid)
		.attr("x",w - 2 * NETWORK_VIEW_BORDER - 60)
		.attr("y",h - 2 * NETWORK_VIEW_BORDER - 40)
		;
	d3.select("#" + this.closetextid)
		.attr("x",w - 2 * NETWORK_VIEW_BORDER - 49)
		.attr("y",h - 2 * NETWORK_VIEW_BORDER - 21)
		;
	d3.select("#" + this.closeid)
		.attr("x",w - 2 * NETWORK_VIEW_BORDER - 60)
		.attr("y",h - 2 * NETWORK_VIEW_BORDER - 40)
		;
	var right = window.outerWidth - NETWORK_VIEW_BORDER - 4;
	if (this.NodeFilter)
		this.NodeFilter.setRight(right);
	if (this.m_NodeGraph)
		this.m_NodeGraph.setTop(h - NETWORK_VIEW_BORDER - this.m_NodeGraph.height - 10);
	if (this.m_InversionNavigator)
		this.m_InversionNavigator.resizeWindow(w, h);
	if (this.m_InversionGrid)
		this.m_InversionGrid.resizeWindow(w, h);
	//
	this.WizardCompleteButton.setTop (h - this.WizardCompleteButton.getHeight() - NETWORK_VIEW_BORDER - 10);
	this.ExportButton        .setTop (h - this.ExportButton        .getHeight() - NETWORK_VIEW_BORDER - 10);
	this.GatherDataButton    .setTop (h - this.GatherDataButton    .getHeight() - NETWORK_VIEW_BORDER - 10);
	var L1 = NETWORK_VIEW_BORDER + 270;
	var R1 = L1 + this.WizardCompleteButton.getWidth();
	var L3 = w - this.GatherDataButton.getWidth() - NETWORK_VIEW_BORDER - 10;
	var L2 = R1 + (L3 - R1) / 2 - this.ExportButton.getWidth() / 2;
	this.WizardCompleteButton.setLeft(L1);
	this.ExportButton        .setLeft(L2);
	this.GatherDataButton    .setLeft(L3);	
}
// 
NetworkView.prototype.updateNodeFilter = function() {
	this.NodeFilter.disable();
	// get the text the user wants to use as the filter
	var s = this.NodeFilter.getText();
	var type = "";
	var icolon = s.indexOf(":");
	if (icolon > -1) {
		type = s.substring(0, icolon).toUpperCase();
		s = s.substring(icolon + 1);
	}
	// create a Regular Expression for matching
	var exp = new RegExp("^"+s,"i"); // starts with
	var exp = new RegExp(s,"i"); // contains
	//
	var bJunctions  = (type == "ALL" || type == "" || type == "JUNCTION"  || type == "JUNCTIONS"  || type == "NODE" || type == "NODES");
	var bTanks      = (type == "ALL" || type == "" || type == "TANK"      || type == "TANKS"      || type == "NODE" || type == "NODES");
	var bReservoirs = (type == "ALL" || type == "" || type == "RESERVOIR" || type == "RESERVOIRS" || type == "NODE" || type == "NODES");
	var bPumps      = (type == "ALL" ||               type == "PUMP"      || type == "PUMPS"      || type == "LINK" || type == "LINKS");
	var bValves     = (type == "ALL" ||               type == "VALVE"     || type == "VALVES"     || type == "LINK" || type == "LINKS");
	var bPipes      = (type == "ALL" ||               type == "PIPE"      || type == "PIPES"      || type == "LINK" || type == "LINKS");
	//
	var hideJunctions = [];
	var hideTanks = [];
	var hideReservoirs = [];
	var hidePumps = [];
	var hideValves = [];
	var hidePipes = [];
	//
	var filterJunctions = [];
	//if (this.getToggleValue(this.toggleJunctionId)) {
	if (bJunctions) {
		if (s == "") {
			filterJunctions = filterJunctions.concat(this.m_data.Junctions);
		} else {
			filterJunctions = this.m_data.Junctions.filter(function(val) {
				return val.id.match(exp);
			});
		}
	}
	var filterTanks = [];
//	if (this.getToggleValue(this.toggleTankId)) {
	if (bTanks) {
		if (s == "") {
			filterTanks = filterTanks.concat(this.m_data.Tanks);
		} else {
			filterTanks = this.m_data.Tanks.filter(function(val) {
				return val.id.match(exp);
			});
		}
	}
	var filterReservoirs = [];
	//if (this.getToggleValue(this.toggleReservoirId)) {
	if (bReservoirs) {
		if (s == "") {
			filterReservoirs = filterReservoirs.concat(this.m_data.Reservoirs);
		} else {
			filterReservoirs = this.m_data.Reservoirs.filter(function(val) {
				return val.id.match(exp);
			});
		}
	}
	var filterPumps = [];
	//if (this.getToggleValue(this.togglePumpId)) {
	if (bPumps) {
		if (s == "") {
			filterPumps = filterPumps.concat(this.m_data.Pumps);
		} else {
			filterPumps = this.m_data.Pumps.filter(function(val) {
				return val.ID.match(exp); //TODO "ID" -> "id"
			});
		}
	}
	var filterValves = [];
	//if (this.getToggleValue(this.toggleValveId)) {
	if (bValves) {
		if (s == "") {
			filterValves = filterValves.concat(this.m_data.Valves);
		} else {
			filterValves = this.m_data.Valves.filter(function(val) {
				return val.ID.match(exp); //TODO "ID" -> "id"
			});
		}
	}
	var filterPipes = [];
	//if (this.getToggleValue(this.togglePipeId)) {
	if (bPipes) {
		if (s == "") {
			filterPipes = filterPipes.concat(this.m_data.Pipes);
		} else {
			filterPipes = this.m_data.Pipes.filter(function(val) {
				return val.ID.match(exp); //TODO "ID" -> "id"
			});
		}
	}
	this.NodeFilter.update([filterJunctions, filterTanks, filterReservoirs, filterPumps, filterValves, filterPipes]);
}
//
NetworkView.prototype.hide = function(bHide) {
	if (bHide == null || bHide)
		this.d3g.style("visibility","hidden");
	else
		this.d3g.style("visibility","inherit");
}

NetworkView.prototype.show = function(bShow) {
	if (bShow == null || bShow)
		this.d3g.style("visibility","inherit");
	else
		this.d3g.style("visibility","hidden");
}


NetworkView.prototype.isVisible = function() {
	var sVisible = this.d3g.style("visibility");
	return !(sVisible == "hidden");
}

/////////////////////////////////////////////////////////////////////

NetworkView.prototype.cancelToolTip = function(sid) {
	this.initiateToolTip("");
}

NetworkView.prototype.initiateToolTip = function(sid) {
	var m_this = this;
	if (sid == this.currentToolTipId) return;
	this.ToolTip.hide();
	this.currentToolTipId = sid;
	if (this.toolTipHandle) clearTimeout(this.toolTipHandle);
	this.toolTipHandle = setTimeout(function() {
		m_this.doToolTip(sid);
	}, 1200);
}

NetworkView.prototype.doToolTip = function(sid) {
	var text = "";
	var p = {};
	switch (sid) {
		case this.zoominid:
			text = "Click to zoom in.";
			p = {"x": 60, "y": 45};
			break;
		case this.zoomoutid:
			text = "Click to zoom out.";
			p = {"x": 60, "y": 70};
			break;
		case this.zoomfullid:
			text = "Click to zoom full.";
			p = {"x": 60, "y": 95};
			break;
		case this.zoomselectid:
			text = "Click to start zoom selection.";
			p = {"x": 60, "y": 120};
			break;
		case this.dotsizeid:
			text = "Scroll up/down with a mouse wheel or a track pad to resize nodes.";
			p = {"x": 60, "y": 145};
			break;
		case this.cameraid:
			text = "Click to capture a screen shot (*.png) of the visible area of the network. (currently disabled).";
			p = {"x": 60, "y": 170};
			break;
		case this.toggleJunctionId:
			text = "Click to hide/show Junctions.";
			p = {"x": 60, "y": 210};
			break;
		case this.toggleTankId:
			text = "Click to hide/show Tanks.";
			p = {"x": 60, "y": 235};
			break;
		case this.toggleReservoirId:
			text = "Click to hide/show Reservoirs.";
			p = {"x": 60, "y": 260};
			break;
		case this.toggleValveId:
			text = "Click to hide/show Valves.";
			p = {"x": 60, "y": 285};
			break;
		case this.togglePumpId:
			text = "Click to hide/show Pumps.";
			p = {"x": 60, "y": 310};
			break;
		case this.togglePipeId:
			text = "Click to hide/show Pipes.";
			p = {"x": 60, "y": 335};
			break;
		case this.toggleSensorId:
			text = "Click to hide/show Sensors.";
			p = {"x": 60, "y": 360};
			break;
	}
	if (text.length > 0)
		this.ToolTip.redraw(p.x, p.y, text, "ToolTip", true);
}

/////////////////////////////////////////////////////////////////////
NetworkView.prototype.loadToggle = function(toggle) {
	var m_this = this;
	this.Toggle.setAll(true);
	if (toggle) {
		m_this.loadToggleData(toggle);
	} else {
		var uuid = this.getSelection();
		Couch.GetValue(uuid, "toggle", function(data) {
			m_this.loadToggleData(data);
		});
	}
}

NetworkView.prototype.loadToggleData = function(data) {
	//if (data == null) data = this.m_data.toggle;
	this.Toggle.setFromDict(data);
	this.updateToggle();
//	for (var index in this.toggleIds) {
//		var toggle = this.toggleIds[index];
//		if (toggle) this.updateToggle(toggle.id);
//	}
}

NetworkView.prototype.saveToggle = function() {
	var dict = this.Toggle.toDict();
	var uuid = this.getSelection();
	Couch.SetValue(uuid, "toggle", dict);
}

NetworkView.prototype.updateToggle = function(sid) {
	if (sid == null) {
		for (var index in this.toggleIds) {
			var toggle = this.toggleIds[index];
			if (toggle) this.updateToggle(toggle.id);
		}
		return;
	}
	var toggle = this.getToggle(sid);
	var sel = d3.selectAll("#" + toggle.pathId);
	var color = (toggle.show) ? toggle.color : d3.rgb(255,255,255);
	var opacity = (toggle.show) ? this.dOpacity : 1;
	sel.attr(toggle.select, color);
	sel.attr(toggle.select + "-opacity", opacity);
	toggle.f.call(this, toggle.show);
}

NetworkView.prototype.getToggleIndex = function(i_or_sid) {
	if (typeof(i_or_sid) == "number") {
		var i = i_or_sid;
		return this.toggleIds[i].id;
	} else {
		var sid = i_or_sid;
		for (var i = 1; i < 7 + 1; i++) {
			if (this.toggleIds[i].id == sid) return i;
		}
	}
	return -1;
}

NetworkView.prototype.getToggle = function(sid) {
	var index = this.getToggleIndex(sid);
	var toggle = this.toggleIds[index];
	var bShow = this.Toggle.getValue(index);
	toggle.show = bShow;
	toggle.index = index;
	return toggle;
}

NetworkView.prototype.getToggleValue = function(sid) {
	var toggle = this.getToggle(sid);
	return toggle.show;
}

/////////////////////////////////////////////////////////////////////
NetworkView.prototype.getNetworkViewBounds = function() {
	var sWidth  = this.d3svg.style("width");
	var nWidth  = parseInt(sWidth);//remove the "px"
	var sHeight = this.d3svg.style("height");
	var nHeight = parseInt(sHeight);//remove the "px"
	return {"w":nWidth,"h":nHeight};
}
NetworkView.prototype.getNetworkViewCenter = function() {
	var b = this.getNetworkViewBounds();
	return {"x":b.w/2,"y":b.h/2};
}
NetworkView.prototype.getGraphPointFromScreenCenter = function() {
	var p = this.getNetworkViewCenter();
	return this.getGraphPointFromScreenXY(p.x,p.y);
}
NetworkView.prototype.getGraphPointFromScreenXY = function(x,y) {
	var g = document.getElementById(this.viewid);
	var p = this.createPoint(x,y);
	var m = g.getCTM();
	var mi = m.inverse();
	var p = p.matrixTransform(mi);
	return p;
}
NetworkView.prototype.getActualGraphPointFromScreenXY = function(x,y) {
	var p = this.getGraphPointFromScreenXY(x,y);
	return this.convertToActual(p.x,p.y);
}
NetworkView.prototype.createPoint = function(x,y) {
	var svg = document.getElementById(this.svgid);
	var p = svg.createSVGPoint();
	p.x = x;
	p.y = y;
	return p;
}
NetworkView.prototype.convertToActual = function(x,y) {
	var zFactor = this.m_data.zFactor ? this.m_data.zFactor : {"a": 1, "bx": 0, "by": 0};
	var p = this.createPoint(x,y);
	p.x = (p.x + zFactor.bx) / zFactor.a;
	p.y = (p.y + zFactor.by) / zFactor.a;
	return p;
}
NetworkView.prototype.convertToActualRange = function(x,y) {
	var zFactor = this.m_data.zFactor ? this.m_data.zFactor : {"a": 1, "bx": 0, "by": 0};
	var p = this.createPoint(x,y);
	p.x = p.x / zFactor.a;
	p.y = p.y / zFactor.a;
	return p;
}
NetworkView.prototype.getScreenXYFromGraphPoint = function(x,y) {
	var svg = document.getElementById(this.svgid);
	var p = svg.createSVGPoint();
	p.x = x;
	p.y = y;
	var g = document.getElementById(this.viewid);
	var m = g.getCTM();
	var p = p.matrixTransform(m);
	return p;
}
NetworkView.prototype.getCurrentScale = function() {
	var g = document.getElementById(this.viewid);
	var m = g.getCTM();
	return m.a;
}
NetworkView.prototype.getScaleAt = function(rect) {
	var b = this.getNetworkViewBounds();
	var dPad = 10;
	if (rect.w / b.w > rect.h / b.h) {
		return b.w / rect.w / (1 + 2 * dPad/100);
	} else {
		return b.h / rect.h / (1 + 2 * dPad/100);
	}
}
NetworkView.prototype.getScaleFull = function(data) {
	if (data == null) data = this.m_data;
	return this.getScaleAt({"w":data.Bounds.range.x,"h":data.Bounds.range.y});
}

NetworkView.prototype.getZoomSelectionState = function() {
	return this.d3ZoomSelection.attr("data-state");
}
/////////////////////////////////////////////////////////////////////
NetworkView.prototype.onMouseMove = function(sid, optional_e) {
	var m_this = this;
	this.m_InversionGrid.onMouseMove(sid);
	var e = optional_e ? optional_e : d3.event;
	if (m_SVGPan.state == "pan") {
		m_SVGPan.onMouseMove(e);
		return;
	}
	if (this.d3ZoomSelection.attr("data-state") == "started") {
		var x1 = parseFloat(this.d3ZoomSelection.attr("data-x"));
		var y1 = parseFloat(this.d3ZoomSelection.attr("data-y"));
		var x2 = e.clientX - this.x;
		var y2 = e.clientY - this.y;
		var deltaX = x2 - x1;
		var deltaY = y2 - y1;
		if (deltaX > 0) {
			this.d3ZoomSelection.attr("x"    , x1);
			this.d3ZoomSelection.attr("width", deltaX);
		} else {
			this.d3ZoomSelection.attr("x"    , x2);
			this.d3ZoomSelection.attr("width", -deltaX);
		}
		if (deltaY > 0) {
			this.d3ZoomSelection.attr("y"     , y1);
			this.d3ZoomSelection.attr("height", deltaY);
		} else {
			this.d3ZoomSelection.attr("y"     , y2);
			this.d3ZoomSelection.attr("height", -deltaY);
		}
	} else {
		this.d3ZoomSelection.attr("display", "none");
		this.d3ZoomSelection.attr("width", 0);
		this.d3ZoomSelection.attr("height", 0);
		this.d3ZoomSelection.attr("data-x", "");
		this.d3ZoomSelection.attr("data-y", "");
	}
	/////////////////////////////////////////////////
	if (sid == null) return;
	switch (sid) {
		case this.zoominid:
		case this.zoomoutid:
		case this.zoomfullid:
		case this.zoomselectid:
		case this.dotsizeid:
		case this.cameraid:
			this.initiateToolTip(sid);
			this.d3zoombg.transition().duration(20).attr("fill-opacity", 1.0);
			return;
		case this.toggleJunctionId:
		case this.toggleTankId:
		case this.toggleReservoirId:
		case this.toggleValveId:
		case this.togglePumpId:
		case this.togglePipeId:
		case this.toggleSensorId:
			this.initiateToolTip(sid);
			this.d3togglebg.transition().duration(20).attr("fill-opacity", 1.0);
			return;
		case this.zoombgid:
			this.d3zoombg.transition().duration(20).attr("fill-opacity", 1.0);
			return;
		case this.togglebgid:
			this.d3togglebg.transition().duration(20).attr("fill-opacity", 1.0);
			return;
	}
	/////////////////////////////////////////////////
	var obj = document.getElementById(sid);
	var parent = obj.parentElement;
	var pid = parent.id;
	var prePid = getPrefix2(pid);
	var sufPid = getSuffix2(pid);
	switch (prePid) {
		case "gNetworkViewView":
		case "svgNetworkView":
			this.drawCoordinates(e);
			var sel = d3.selectAll("#" + sid);
			var id = sel.attr("data-id");
			if (id != null) {
				var type = sel.attr("data-type");
				this.drawNodePopup(id, type, e);
				if (this.m_data.m_ImpactList) {
					this.m_ImpactView.displayNodeValue(id);
				}
			}
			break;
	}
}

NetworkView.prototype.onMouseUp = function(sid, optional_e) {
	var m_this = this;
	var e = optional_e ? optional_e : d3.event;
	this.m_NodeGraph.onMouseUp();
	if (m_SVGPan) m_SVGPan.onMouseUp(e, false);
	//
	if (this.d3ZoomSelection.attr("data-state") == "started") {
		this.d3ZoomSelection.attr("data-state", "");
		this.onMouseOut(this.zoomselectid);
		this.d3ZoomSelection.attr("display", "none");
		var w = parseFloat(this.d3ZoomSelection.attr("width"));
		var h = parseFloat(this.d3ZoomSelection.attr("height"));
		if (h > 0 && w > 0) {
			var x = parseFloat(this.d3ZoomSelection.attr("x"));
			var y = parseFloat(this.d3ZoomSelection.attr("y"));
			var p1 = this.getGraphPointFromScreenXY(x, y);
			var p2 = this.getGraphPointFromScreenXY(x + w, y + h);
			var rect = {"w": p2.x - p1.x, "h": p1.y - p2.y};
			var z2 = this.getScaleAt(rect);
			var z1 = this.getScaleFull(this.m_data);
			var z = z2 / z1;
			var cx = x + w / 2;
			var cy = y + h / 2;
			var p = this.getGraphPointFromScreenXY(cx, cy);
			this.ZoomTo(1000,"cubic-in-out",{"x":p.x,"y":p.y,"zoom":z});
			return;
		}
	}
	/////////////////////////////////////////////////
	if (sid == null) return;
	var sel = d3.selectAll("#" + sid);
	var id = sel.attr("data-id");
	if (id != null) {
		var type = sel.attr("data-type");
		switch (type) {
			case "Junction":
			case "Tank":
			case "Reservoir":
				if (e.which == MOUSE_CLICK_RIGHT) {
				} else if (e.which == MOUSE_CLICK_LEFT ) {
					//
					// a node was clicked in the network view
					if (!this.m_NodeGraph) return;
					if (this.m_NodeGraph.isHidden()) return;
					var index = this.m_data.NodeIds[id];
					var type = this.m_data.Nodes[index].Type;
					//this.drawNodePopup(id, type);
					this.drawNodePopup(id, type, e);
					//
					// save the old selected node for deselection below
					var old_id = this.m_NodeGraph.getNodeId();
					if (id == old_id) return;
					//
					// store new selected node in the couch database
					m_SimList.getSelectedData().value.selectedNode = id;
					this.m_NodeGraph.setNodeId(id);
					var uuid = m_SimList.getSelectedData().id;
					var http = Couch.HttpFactory("PUT", GlobalData.CouchUpdateQuery + uuid + "?body_key=selectedNode", id);
					http.Send(function(e,res) {
						m_SimList.viewNetwork(m_this);
					});
					//
					// clone newly selected node so it sits on top of the other nodes
					var sel2 = this.getNode(id);
					var node = sel2[0][0];
					var newNode = node.cloneNode(true);
					var hidden_id = node.id + "_hiddenClone";
					var sel = this.d3view.selectAll("#" + hidden_id);
					if (!sel.empty()) sel2.remove();
					sel2.attr("visibility", "hidden");
					node.id = hidden_id;
					this.d3view[0][0].appendChild(newNode);
					//
					// highlight the newly selected node with a red outline (and deselect the old one)
					var sel3 = this.getNode(id);
					sel3.datum(sel2.datum());
					this.drawSelectedNode(old_id);
				}
				break;
		}
	}
}

NetworkView.prototype.onMouseOver = function(sid) {
	this.cancelToolTip();
	if (this.d3ZoomSelection.attr("data-state") != "") return;
	var m_this = null;
	d3.select("#"+sid).each(function() { m_this = this; });
	var sel = d3.select("#"+sid);
	switch (sid) {
		case this.zoominid:
			//sel.attr("fill-opacity", 0.09);
			//d3.select("#" + "line1NetworkZoomIn" + "_" + this.nid).attr("stroke-opacity", 0.9);
			//d3.select("#" + "line2NetworkZoomIn" + "_" + this.nid).attr("stroke-opacity", 0.9);
			d3.select("#" + this.zoominbgid).attr("fill-opacity", 0.09);
			d3.select("#" + this.zoominbgid).attr("fill-opacity", 0.6);
			break;
		case this.zoomoutid:
			//sel.attr("fill-opacity", 0.09);
			//d3.select("#" + "lineNetworkZoomOut" + "_" + this.nid).attr("stroke-opacity", 0.9);
			d3.select("#" + this.zoomoutbgid).attr("fill-opacity", 0.09);
			d3.select("#" + this.zoomoutbgid).attr("fill-opacity", 0.6);
			break;
		case this.zoomfullid:
			//sel.attr("fill-opacity", 0.00);
			//d3.select("#" + "pathNetworkZoomFull" + "_" + this.nid).attr("stroke-opacity", 0.9);
			d3.select("#" + this.zoomfullbgid).attr("fill-opacity", 0.09);
			d3.select("#" + this.zoomfullbgid).attr("fill-opacity", 0.6);
			break;
		case this.zoomselectid:
			//sel.attr("fill-opacity", 0.09);
			d3.select("#" + "lineNetworkZoomSelect"   + "_" + this.nid).attr("stroke", d3.rgb(255,255,255));
			//d3.select("#" + "lineNetworkZoomSelect"   + "_" + this.nid).attr("stroke-opacity", 0.9);
			d3.select("#" + "circleNetworkZoomSelect" + "_" + this.nid).attr("stroke", d3.rgb(255,255,255));
			//d3.select("#" + "circleNetworkZoomSelect" + "_" + this.nid).attr("stroke-opacity", 0.9);
			//sel.attr("fill", d3.rgb(255,255,255));
			d3.select("#" + this.zoomselectbgid).attr("fill-opacity", 0.09);
			d3.select("#" + this.zoomselectbgid).attr("fill-opacity", 0.6);
			d3.select("#" + this.zoomselectbgid).attr("fill", d3.rgb(0,0,0));
			break;
		case this.dotsizeid:
			//sel.attr("fill-opacity", 0.09);
			//d3.select("#" + "circleDotSize" + "_" + this.nid).attr("stroke-opacity", 0.9);
			d3.select("#" + this.dotsizebgid).attr("fill-opacity", 0.09);
			d3.select("#" + this.dotsizebgid).attr("fill-opacity", 0.6);
			break;
		case this.cameraid:
			//sel.attr("fill-opacity", 0.09);
			d3.select("#" + this.camerabgid).attr("fill-opacity", 0.09);
			d3.select("#" + this.camerabgid).attr("fill-opacity", 0.6);
			break;
		////////
		case this.toggleJunctionId:
		case this.toggleTankId:
		case this.toggleReservoirId:
		case this.toggleValveId:
		case this.togglePumpId:
		case this.togglePipeId:
		case this.toggleSensorId:
			//sel.attr("fill-opacity", 0.09);
			var toggle = this.getToggle(sid);
			d3.select("#" + toggle.bgid).attr("fill-opacity", 0.9);
			var color = (toggle.show) ? toggle.color : d3.rgb(200,200,200);
			d3.select("#" + toggle.pathId).attr(toggle.select, color);
			break;
		case this.closeid:
			d3.select("#" + this.closetextid).attr("fill", "black");
			break;
	}
	/////////////////////////////////////////////////
	if (sid == null) return;
	var sidPrefix = getPrefix2(sid);
	var id        = getSuffix2(sid);
	var pre = getPrefix2(this.NodeFilter.List.sid);
	if (sid.startsWith(pre) && sid != this.NodeFilter.List.sid) {
		var item = document.getElementById(sid);
		var nodeId = this.NodeFilter.List.getText(item.index);
		var type = this.NodeFilter.List.getAttr(item.index, "data-type");
		var x = this.NodeFilter.List.getAttr(item.index, "data-x");
		var y = this.NodeFilter.List.getAttr(item.index, "data-y");
		var p = this.getScreenXYFromGraphPoint(parseFloat(x), parseFloat(y));
		this.NodePopup.redraw(p.x + this.x, p.y + this.y, nodeId, type, true, true);
	}
}
NetworkView.prototype.onMouseOut = function(sid) {
	this.cancelToolTip();
	if (this.d3ZoomSelection.attr("data-state") != "") return;
	var sel = d3.select("#" + sid);
	switch (sid) {
		case this.zoominid:
			//d3.select("#" + "line1NetworkZoomIn" + "_" + this.nid).attr("stroke-opacity", 0.7);
			//d3.select("#" + "line2NetworkZoomIn" + "_" + this.nid).attr("stroke-opacity", 0.7);
			//sel.attr("fill-opacity", 0.5);
			d3.select("#" + this.zoominbgid).attr("fill-opacity", 0.5);
			d3.select("#" + this.zoominbgid).attr("fill-opacity", 0.2);
			break;
		case this.zoomoutid:
			//d3.select("#" + "lineNetworkZoomOut" + "_" + this.nid).attr("stroke-opacity", 0.7);
			//sel.attr("fill-opacity", 0.5);
			d3.select("#" + this.zoomoutbgid).attr("fill-opacity", 0.5);
			d3.select("#" + this.zoomoutbgid).attr("fill-opacity", 0.2);
			break;
		case this.zoomfullid:
			//d3.select("#" + "pathNetworkZoomFull" + "_" + this.nid).attr("stroke-opacity", 0.7);
			//sel.attr("fill-opacity", 0.3);
			d3.select("#" + this.zoomfullbgid).attr("fill-opacity", 0.5);
			d3.select("#" + this.zoomfullbgid).attr("fill-opacity", 0.2);
			break;
		case this.zoomselectid:
			d3.select("#" + "lineNetworkZoomSelect"   + "_" + this.nid).attr("stroke", d3.rgb(255,255,255));
			//d3.select("#" + "lineNetworkZoomSelect"   + "_" + this.nid).attr("stroke-opacity", 0.7);
			d3.select("#" + "lineNetworkZoomSelect"   + "_" + this.nid).attr("stroke-opacity", 1.0);
			d3.select("#" + "circleNetworkZoomSelect" + "_" + this.nid).attr("stroke", d3.rgb(255,255,255));
			//d3.select("#" + "circleNetworkZoomSelect" + "_" + this.nid).attr("stroke-opacity", 0.7);
			d3.select("#" + "circleNetworkZoomSelect" + "_" + this.nid).attr("stroke-opacity", 1.0);
			//sel.attr("fill", d3.rgb(255,255,255));
			//sel.attr("fill-opacity", 0.5);
			d3.select("#" + this.zoomselectbgid).attr("fill", d3.rgb(0,0,0));
			d3.select("#" + this.zoomselectbgid).attr("fill-opacity", 0.5);
			d3.select("#" + this.zoomselectbgid).attr("fill-opacity", 0.2);
			break;
		case this.dotsizeid:
			//d3.select("#" + "circleDotSize" + "_" + this.nid).attr("stroke-opacity", 0.7);
			//sel.attr("fill-opacity", 0.5);
			d3.select("#" + this.dotsizebgid).attr("fill-opacity", 0.5);
			d3.select("#" + this.dotsizebgid).attr("fill-opacity", 0.2);
			break;
		case this.cameraid:
			//sel.attr("fill-opacity", 0.5);
			d3.select("#" + this.camerabgid).attr("fill-opacity", 0.5);
			d3.select("#" + this.camerabgid).attr("fill-opacity", 0.2);
			break;
		case this.toggleJunctionId:
		case this.toggleTankId:
		case this.toggleReservoirId:
		case this.toggleValveId:
		case this.togglePumpId:
		case this.togglePipeId:
		case this.toggleSensorId:
			//sel.attr("fill-opacity", 0.5);
			var toggle = this.getToggle(sid);
			d3.select("#" + toggle.bgid).attr("fill-opacity", 0.5);
			var color = (toggle.show) ? toggle.color : "white";
			d3.select("#" + toggle.pathId).attr(toggle.select, color);
			break;
		case this.closeid:
			d3.select("#" + this.closetextid).attr("fill", "white");
			break;
	}
	var bSame = false;
	if (!bSame) {
		this.d3zoombg.transition().duration(3000).attr("fill-opacity", 0.2);
		this.d3zoombg.transition().duration(3000).attr("fill-opacity", 0.0);
		this.d3togglebg.transition().duration(3000).attr("fill-opacity", 0.2);
		this.d3togglebg.transition().duration(3000).attr("fill-opacity", 0.0);
	}
}

NetworkView.prototype.onMouseDown = function(sid) {
	var sel = d3.select("#"+sid);
	switch (sid) {
		case this.rectid:
		case this.viewid:
			if (d3.event.ctrlKey || d3.event.metaKey || this.d3ZoomSelection.attr("data-state") == "ready") {
				this.d3ZoomSelection.attr("data-state","started");
				this.d3ZoomSelection.attr("data-x", d3.event.clientX - this.x);
				this.d3ZoomSelection.attr("data-y", d3.event.clientY - this.y);
				this.d3ZoomSelection.attr("x", d3.event.clientX - this.x);
				this.d3ZoomSelection.attr("y", d3.event.clientY - this.y);
				this.d3ZoomSelection.attr("width", 0);
				this.d3ZoomSelection.attr("height", 0);
				this.d3ZoomSelection.attr("fill", d3.rgb(0,0,255));
				this.d3ZoomSelection.attr("fill-opacity", 0.1);
				this.d3ZoomSelection.attr("display", "inherit");
			}
			break;
	}
	if (m_SVGPan) m_SVGPan.onMouseDown(d3.event, false);
	window.onMouseDown(sid);
}
//NetworkView.prototype.onMouseWheel = function(sid) {}
NetworkView.prototype.onKeyDown = function(sid) {
	switch (sid) {
		case this.NodeFilter.Filter.sid:
			if (d3.event.keyCode == 13) { // Return/Enter
				this.updateNodeFilter();				
			}
			break;
			if (false/*d3.event.keyCode == 13*/) { // Return/Enter
				var length = this.NodeFilter.List.getLength();
				var text1 = this.NodeFilter.getText();
				var text2 = (length < 1) ? "" : this.NodeFilter.List.getText(0);
				if (text1.toUpperCase() == text2.toUpperCase()) {
					this.NodeFilter.List.selectFirst();
					this.NodeFilter.ZoomToSelection();
				}
			}
			break;
	}
}
NetworkView.prototype.onKeyPress = function(sid) {
	window.onKeyPress(sid);
}
NetworkView.prototype.onKeyUp = function(sid) {
	window.onKeyUp(sid);
}
NetworkView.prototype.onInput = function(sid) {
	switch (sid) {
		case this.NodeFilter.Filter.sid:
			//smm.2013.01.13.1824//this.updateNodeFilter();
			break;
	}
}
NetworkView.prototype.onChange = function(sid) {
	switch (sid) {
		case this.NodeFilter.List.sid:
			this.NodeFilter.ZoomToSelection();
	}
}
NetworkView.prototype.onClick = function(sid) {
	var m_this = null;
	d3.select("#" + sid).each(function() { m_this = this; });
	switch (sid) {
		case this.NodeFilter.List.sid:
			this.NodeFilter.ZoomToSelection();
			break;
		case this.zoomselectid:
			if (this.d3ZoomSelection.attr("data-state") == "") {
				this.d3ZoomSelection.attr("data-state","ready");
				d3.select("#" + "lineNetworkZoomSelect"   + "_" + this.nid).attr("stroke", d3.rgb(0,0,255));
				//d3.select("#" + "lineNetworkZoomSelect"   + "_" + this.nid).attr("stroke-opacity", 0.5);
				d3.select("#" + "circleNetworkZoomSelect" + "_" + this.nid).attr("stroke", d3.rgb(0,0,255));
				//d3.select("#" + "circleNetworkZoomSelect" + "_" + this.nid).attr("stroke-opacity", 0.5);
				//d3.select("#" + this.zoomselectid).attr("fill", d3.rgb(0,0,255));
				//d3.select("#" + this.zoomselectid).attr("fill-opacity", 0.1);
				d3.select("#" + this.zoomselectbgid).attr("fill", d3.rgb(0,0,255));
				d3.select("#" + this.zoomselectbgid).attr("fill-opacity", 0.1);
			} else {
				this.d3ZoomSelection.attr("data-state","");
				this.onMouseOver(this.zoomselectid);
			}
			break;
		case this.closeid:
			var selectedNode = m_NetworkView.m_NodeGraph.getNodeId();
			var sel = this.getNode(selectedNode);
			sel.attr("stroke", "none") // deselect selectedNode
			this.NodePopup.hide();
			this.m_NodeGraph.hide()
			this.hide();
			this.saveView();
			//d3.select("body").style("overflow","auto");//added document.body.style.overflow=hidden to gui.html
			break;
		case this.zoominid:
			var iter = 0;
			this.Zoom(2/1, iter);
			break;
		case this.zoomoutid:
			var iter = 0;
			this.Zoom(1/2, iter)
			break;
		case this.zoomfullid:
			this.ZoomFull();
			break;
		case this.dotsizeid:
			break;
			var z = 1;
			if (d3.event.which == MOUSE_CLICK_RIGHT) {
				z = 1/2;
			} else if (d3.event.which == MOUSE_CLICK_LEFT ) {
				z = 2/1;
			}
			this.nStrokeWidth *= z;
			if (!this.m_data.dotSize)
				GlobalData.nStrokeWidth *= z;
			this.NodePopup.hide();
			this.resizeDot();
			break;
		case this.toggleJunctionId:
		case this.toggleTankId:
		case this.toggleReservoirId:
		case this.toggleValveId:
		case this.togglePumpId:
		case this.togglePipeId:
		case this.toggleSensorId:
			var index = this.getToggleIndex(sid);
			this.Toggle.flip(index);
			this.updateToggle(sid);
			this.saveToggle();
			break;
		case this.WizardCompleteButton.sid:
			m_EventView.hide();
			m_NetworkView.hide();
			var uuids = m_EventView.getUuids();
			Events.log(null, uuids.log, ["Complete (network view)", ""]);
			this.NodePopup.hide();
			break;
		case this.ExportButton.sid:
			m_Waiting.show()
			var uuids = m_EventView.getUuids(); // TODO - if you want to export outside of the event view this will need to be delt with
			Events.log(null, uuids.log, "Export inversion results");
			var uuid = this.getSelectionUuid();
			Couch.getDoc(this, Couch.gis + "?uuid=" + uuid, function(data) {
				if (data.error) {
					alert(data.reason);
				} else {
					var url = GlobalData.CouchDb + data.uuid + "/" + data.fileName;
					window.open(url, "_self");
				}
				m_Waiting.hide()
			});
			break;
		case this.GatherDataButton.sid:
			var uuids = m_EventView.getUuids();
			m_Waiting.show();
			Events.log(null, uuids.log, "Gather more data");
			//Events.log(this, uuids.log, "Gather more data", function() {
				this.NodePopup.hide();			
				m_EventGrabView.show();
				m_Waiting.hide();
			//});
			break;
	}
	/////////////////////////////////////////////////
	var sidPrefix = getPrefix2(sid);
	var id        = getSuffix2(sid);
	switch (sidPrefix) {
		case this.NodeFilter.List.sid:
			this.NodeFilter.ZoomToSelection();
			break;
	}
	var pre = getPrefix2(this.NodeFilter.List.sid);
	if (sid.startsWith(pre)) {
		this.NodeFilter.ZoomToSelection();
	}
}

NetworkView.prototype.onDblClick = function(sid) {
	var m_this = this;
	var dSpacer = 10;//%
	var dRatio = 1 / (1 - dSpacer/100*2);
	switch (sid) {
		case this.dotsizeid:
			var scale = this.getCurrentScale();
			this.m_data.dotSize = 8 / scale;
			this.nStrokeWidth = 0.2 * this.m_data.dotSize;
//			if (!this.m_data.dotSize)
//				GlobalData.nStrokeWidth *= 0.2 * this.m_data.dotSize;
			this.NodePopup.hide();
			this.hideDot();
			break;
		case this.cameraid:
			var svgFile = this.createSnapshot();
			var w = window.innerWidth  - 2 * NETWORK_VIEW_BORDER;
			var h = window.innerHeight - 2 * NETWORK_VIEW_BORDER;
			Couch.createUniqueId(this, function(uuid) {
				var fileName = m_this.m_data.Name;
				var data = {
					"oldName"   : fileName,
					"text"      : svgFile,
					"w"         : w * dRatio,
					"h"         : h * dRatio,
					"m_svgFile" : true,
					"Date"      : new Date()
					};
				Couch.setDoc(this, uuid, data, function(e,res) {
					var uuid = (res && res.data) ? res.data.id : "";
					Couch.getDoc(this, Couch.svg + "?uuid=" + uuid, function(data) {
						if (data) {
							var url = GlobalData.CouchDb + uuid + "/" + data.png;
							//window.location.assign(url);
							var r = (w > h) ? 200 / h : 200 / w;
							if (r > 1) r = 1;
							var args = "width=" + (w * r) + ",height=" + (h * r) + "";
							var myWindow = window.open(url, fileName, args);
							myWindow.moveTo(0,0);
						}
					});
				});
			});
			break;
	}
}

/////////////////////////////////////////////////////////////////////
NetworkView.prototype.addListeners = function() {
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
	this.m_NodeGraph         .addListeners();
	this.m_ImpactView        .addListeners();
	this.m_InversionNavigator.addListeners();
	this.NodePopup           .addListeners();
	this.NodeFilter          .addListeners();
	this.WizardCompleteButton.addListeners();
	this.ExportButton        .addListeners();
	this.GatherDataButton    .addListeners();
}

NetworkView.prototype.addListenersForSelection = function(sSelector) {
	//var m_this = window;
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
