// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

//
// Constructor
//
function NodeGraph(parent, left, top, width, height) {
	this.sAttribute = "Demand";
	this.sAttribute = "Concentration";
	this.parent = parent;
	this.nid = parent.nid;
	var uniqueString = "NodeGraph_" + this.nid;
	this.uniqueString = uniqueString;
	this.sParentId = parent.gid;
	this.gid = "g" + this.uniqueString;
	this.svgid = "svg" + this.uniqueString;
	this.rectid = "rect" + this.uniqueString;
	this.g1id = "g1" + this.uniqueString;
	this.playid      = this.uniqueString + "_PlayPath";
	this.playbackid  = this.uniqueString + "_PlayBackPath";
	this.playforeid  = this.uniqueString + "_PlayForePath";
	this.circleid    = this.uniqueString + "_PlayCircle";
	this.circle2id   = this.uniqueString + "_PlayCircle2";
	this.ellipselid  = this.uniqueString + "_PlayEllipseL";
	this.ellipserid  = this.uniqueString + "_PlayEllipseR";
	this.moveid      = this.uniqueString + "_Move";
	this.movecoverid = this.uniqueString + "_MoveCover";
	this.left = left;
	this.top = top;
	this.width = width;
	this.height = height;
	this.nMarginL = 25;//Math.max(NodeGraph.DEFAULT_MARGIN, this.width  / 10 );
	this.nMarginR = 25;//Math.max(NodeGraph.DEFAULT_MARGIN, this.width  / 10 );
	this.nMarginT = 25;//Math.max(NodeGraph.DEFAULT_MARGIN, this.height / 10 + 20);
	this.nMarginB = 25;//Math.max(NodeGraph.DEFAULT_MARGIN, this.height / 10 );
	this.sBgFill = NodeGraph.DEFAULT_BG_FILL;
	this.sBgOpacity = NodeGraph.DEFAULT_BG_OPACITY;
	this.sLineColor  = NodeGraph.DEFAULT_LINE_COLOR;
	this.sLineColorX = NodeGraph.DEFAULT_AXIS_COLOR;
	this.sLineColorY = NodeGraph.DEFAULT_AXIS_COLOR;
	this.sLineTickColorX = NodeGraph.DEFAULT_AXIS_COLOR;
	this.sLineTickColorY = NodeGraph.DEFAULT_AXIS_COLOR;
	this.sTextColor  = NodeGraph.DEFAULT_TEXT_COLOR;
	this.nLineWidth  = NodeGraph.DEFAULT_LINE_WIDTH;
	this.nLineWidthX = NodeGraph.DEFAULT_AXIS_WIDTH;
	this.nLineWidthY = NodeGraph.DEFAULT_AXIS_WIDTH;
	this.nLineTickWidthX = NodeGraph.DEFAULT_AXIS_WIDTH;
	this.nLineTickWidthY = NodeGraph.DEFAULT_AXIS_WIDTH;
	this.nLineWidthText = NodeGraph.DEFAULT_TEXT_LINE_WIDTH;
	//
	this.createPlayButton();
	//
	this.startTime = 0;
	//
	this.data = {};
	//
	this.TimeLine = null;
	//
	this.nPlaying = NodeGraph.ANIMATION_READY;
	this.interpType = "rgb";
	this.interpType = "hsl";
	// white - red
	this.interpColor0 = d3.rgb(255,255,255);
	this.interpColor1 = d3.rgb(255,  0,  0);
	// white - black
	this.interpColor0 = d3.rgb(255,255,255);
	this.interpColor1 = d3.rgb(  0,  0,  0);
	// lt blue - black
	this.interpColor0 = "hsl(220,100%,080%)";
	this.interpColor1 = "hsl(220,100%,000%)";
	// blue - red
	this.interpColor0 = d3.rgb(  0,  0,255);
	this.interpColor1 = d3.rgb(255,  0,  0);
	//
	this.dragger = {"p":{"x":null,"y":null},"dragging":false};
	
	//
	// Private properties
	//
	var m_nodeId = "";
	var m_simId = "";
	var m_simName = "";
	var m_simSource = "";
	var m_animationProcessor = 0;
	var m_bands = 8;
	var m_dynamicType = null;
	//
	// Private functions
	//
	function setNodeId_private(val) {
		m_nodeId = val ? val : "";
	}
	function getNodeId_private() {
		return m_nodeId;
	}
	//
	function setSimId_private(val) {
		m_simId = val ? val : "";
	}
	function getSimId_private() {
		return m_simId;
	}
	//
	function setSimName_private(val) {
		m_simName = val ? val : "";
	}
	function getSimName_private() {
		return m_simName;
	}
	//
	function setSource_private(val) {
		m_simSource = val ? val : "";
	}
	//
	function getSource_private() {
		return m_simSource;
	}
	//
	function setAnimationProcessor_private(val) {
		m_animationProcessor = val ? val : 0;
	}
	function getAnimationProcessor_private() {
		return m_animationProcessor;
	}
	//
	function setBands_private(val) {
		m_bands = val;
	}
	function getBands_private() {
		return m_bands;
	}
	//
	function setDynamicType_private(val) {
		m_dynamicType = val;
	}
	function getDynamicType_private() {
		return m_dynamicType;
	}
	//
	// Privileged Functions
	//
	this.setNodeId = function(val) {
		setNodeId_private(val);
	}
	this.getNodeId = function() {
		return getNodeId_private();
	}
	//
	this.setSimId = function(val) {
		setSimId_private(val);
		//this.time = -1e9; // TODO - does there need to be something else here to reset the time if we leave the animation view and return?
		this.getSimData();
	}
	this.getSimId = function() {
		return getSimId_private();
	}
	//
	this.getSimName = function() {
		return getSimName_private();
	}
	//
	this.setSource = function(val) {
		setSource_private(val);
	}
	//
	this.getSource = function() {
		return getSource_private();
	}
	//
	this.getSimData = function() {
		var m_this = this;
		Couch.GetDoc(this.getSimId(), function(data) {
			setNodeId_private(data.selectedNode);
			setSimName_private(data.name);
			setSource_private(data.input_TSG[0]["a0"]);
			var dynamicType = GlobalData.getConfig("erd_dynamic_type");
			setDynamicType_private(data.measuregen ? "Gabe" : dynamicType);
		});
	}
	//
	this.setAnimationProcessor = function(val) {
		setAnimationProcessor_private(val);
	}
	this.getAnimationProcessor = function() {
		return getAnimationProcessor_private();
	}
	//
	this.setBands = function(val) {
		setBands_private(val);
	}
	this.getBands = function() {
		return getBands_private();
	}
	//
	this.getDynamicType = function() {
		return getDynamicType_private();
	}
	//
	this.create();
}

//
// Static Properties
//
NodeGraph.DEFAULT_BG_FILL = "rgb(255,255,255)";
NodeGraph.DEFAULT_BG_OPACITY = 0.8;
NodeGraph.DEFAULT_LINE_COLOR = "rgba(0,0,150,0.6)";
NodeGraph.DEFAULT_LINE_WIDTH = 2;
NodeGraph.DEFAULT_TEXT_COLOR = "rgba(30,30,30,0.8)";
NodeGraph.DEFAULT_TEXT_LINE_WIDTH = 0.05;
NodeGraph.DEFAULT_AXIS_COLOR = "rgba(0,0,0,0.3)";
NodeGraph.DEFAULT_AXIS_WIDTH = 1;
NodeGraph.DEFAULT_MARGIN = 25;
NodeGraph.ANIMATION_STOPPED = 0;
NodeGraph.ANIMATION_READY = 1;
NodeGraph.ANIMATION_PLAYING = 2;
NodeGraph.ANIMATION_REWIND = 3;
NodeGraph.ANIMATION_FASTFORWARD = 4;

//
// Public Member Methods
//
NodeGraph.prototype.create = function() {
	this.d3g = d3.select("#" + this.sParentId).append("g").attr("id", this.gid)
		.style("position","absolute")
		.style("top", convertToPx(this.top))
		.style("left", convertToPx(this.left))
		;		
	this.d3svg = this.d3g.append("svg").attr("id", this.svgid)
		.style("top", convertToPx(0))
		.style("left", convertToPx(0))
		.style("width", convertToPx(this.width))
		.style("height", convertToPx(this.height))
		;
	this.d3rect = this.d3svg.append("rect").attr("id", this.rectid)
		.attr("x", 0)
		.attr("y", 0)
		.attr("width", this.width)
		.attr("height", this.height)
		.attr("fill", this.sBgFill)
		.attr("fill-opacity", this.sBgOpacity)
		;
		//
	////
	// continuous legend
	//this.legendGradient = new Gradient("linearGradient_Legend3", this.svgid, 0, 0, 0, 100);
	//this.legendGradient.defaultOpacity = 0.3;
	//this.legendGradient.addStop( 0, "red");
	//this.legendGradient.addStop(25, "yellow");
	//this.legendGradient.addStop(50, "green");
	//this.legendGradient.addStop(75, "cyan");
	//this.legendGradient.addStop(95, "blue");
	//this.legendGradient.create();
	////
	// banded legend
	this.legendGradient = new Gradient("linearGradient_Legend1", this.svgid, 0, 0, 0, 100);
	this.legendGradient.defaultOpacity = 0.3;
	for (var i = 0; i < this.getBands(); i++) {
		var a1 = 99 / this.getBands();
		var a2 = 240 / this.getBands();
		this.legendGradient.addStop(a1*(i+1)-0.5, d3.hsl(a2*(i+0),1.0,0.5));
		this.legendGradient.addStop(a1*(i+1)+0.5, d3.hsl(a2*(i+1),1.0,0.5));
	}
	this.legendGradient.create();
	//
	this.time = -1e9;
	this.draw();
}

NodeGraph.prototype.getGabe = function(fNextStep) {
	var uuid = this.getSimId();
	Couch.getDoc(this, uuid, function(data) {
		var fileName = data.measuregen;
		var y_max = data.y_max;
		Couch.getFile(this, uuid, fileName, function(measuregen){
			this.data.Dynamic = {};
			this.data.Dynamic.Concentrations  = (measuregen) ? measuregen.Concentrations : [ ];
			this.data.Dynamic.numSteps        = (measuregen) ? measuregen.TimeSteps      :  0 ;
			this.data.Dynamic.timeStart       = (measuregen) ? measuregen.TimeFirst      :  0 ;
			this.data.Dynamic.timeStep        = (measuregen) ? measuregen.TimeStep       :  0 ;
			this.data.Static = {};
			this.data.Static.timeStep         = (measuregen) ? measuregen.TimeStep       :  0 ;
			this.data.Static.Concentrations   = [];
			for (var i = 0; i < this.data.Dynamic.numSteps; i++) {
				this.data.Static.Concentrations.push(0);
			}
			if (y_max == null) {
				y_max = 0;
				for (var nodeId in this.data.Dynamic.Concentrations) {
					var node = this.data.Dynamic.Concentrations[nodeId];
					for (var index in node) {
						var value = node[index];
						if (value > y_max) y_max = value;
					}
				}
			}
			this.data.Static.ConcentrationMax = y_max;
			this.data.Dynamic.ConcentrationMax = y_max;
			fNextStep.call(this);
		});
	});
}

NodeGraph.prototype.draw = function() {
	if (this.getDynamicType() == "Gabe") {
		this.getGabe(this.draw2);
	} else {
		this.draw2();
	}
}

NodeGraph.prototype.draw2 = function() {
	var m_this = this;
	var bVisible = this.data && this.data.Static && this.getNodeId() && this.getNodeId().length > 0;
	if (bVisible || this.getDynamicType() == "Gabe") {
		if (this.data.Static.Error) {
			this.d3g1 = this.d3svg.append("g")
				.attr("id", this.g1id)
				.attr("class", this.uniqueString + "_removeable")
				.attr("transform", "translate(0,"+this.height+")")
				;
			this.d3g1.append("text")
				.attr("id", "text1" + this.uniqueString)
				.attr("stroke", this.sTextColor)
				.attr("stroke-width", this.nLineWidthText)
				//.attr("x", (this.width < 150) ? 0 : this.nMarginL * 0.5)
				.attr("x", this.nMarginL * 0.1)
				.attr("y", -1 * (this.height - Math.max(11, 0.2*this.nMarginT)))
				.style("font-size", convertToPx(Math.max(11,0.2*this.nMarginT)))
				.text("Error: " + this.data.Static.Error)
				;
			return;
		}
		this.nPlaying = NodeGraph.ANIMATION_READY;
		var data = this.data.Static[this.sAttribute + "s"];
		var dMax = this.data.Static[this.sAttribute + "Max"];
		if (dMax == 0) dMax = 1;
		if (this.data.Static) {
			this.nStep = this.data.Static.timeStep;
		} else if (this.data.Dynamic) {
			this.nStep = this.data.Dynamic.timeStep;
		} else {
			this.nStep = 0;
		}
		var step = this.nStep;
		var maxSeconds = step * data.length;
		this.TicksX = [];
		if (maxSeconds == 0) {
			// do nothing
		} else if (maxSeconds <= 5*60) {
			this.TicksX = [1*60/step, 2*60/step, 3*60/step, 4*60/step];
		} else if (maxSeconds <= 6*60) {
			this.TicksX = [2*60/step, 4*60/step];
		} else if (maxSeconds <= 8*60) {
			this.TicksX = [2*60/step, 4*60/step, 6*60/step];
		} else if (maxSeconds <= 10*60) {
			this.TicksX = [2*60/step, 4*60/step, 6*60/step, 8*60/step];
		} else if (maxSeconds <= 15*60) {
			this.TicksX = [5*60/step, 10*60/step];
		} else if (maxSeconds <= 20*60) {
			this.TicksX = [5*60/step, 10*60/step, 15*60/step];
		} else if (maxSeconds <= 25*60) {
			this.TicksX = [5*60/step, 10*60/step, 15*60/step, 20*60/step];
		} else if (maxSeconds <= 30*60) {
			this.TicksX = [10*60/step, 20*60/step];
		} else if (maxSeconds <= 40*60) {
			this.TicksX = [10*60/step, 20*60/step, 30*60/step];
		} else if (maxSeconds <= 50*60) {
			this.TicksX = [10*60/step, 20*60/step, 30*60/step, 40*60/step];
		} else if (maxSeconds <= 60*60) {
			this.TicksX = [15*60/step, 30*60/step, 45*60/step];
		} else if (maxSeconds <= 75*60) {
			this.TicksX = [15*60/step, 30*60/step, 45*60/step, 60*60/step];
		} else if (maxSeconds <= 120*60) {
			this.TicksX = [];
			for (var i = 1; i < 6; i++) {
				if (i*30*60 < maxSeconds)
					this.TicksX.push(i*30*60 / step);
			}
		} else if (maxSeconds <= 7*60*60) {
			this.TicksX = [];
			for (var i = 1; i < 7; i++) {
				if (i*1*60*60 < maxSeconds)
					this.TicksX.push(i*1*60*60 / step);
			}
		} else if (maxSeconds <= 14*60*60) {
			this.TicksX = [];
			for (var i = 1; i < 7; i++) {
				if (i*2*60*60 < maxSeconds)
					this.TicksX.push(i*2*60*60 / step);
			}
		} else if (maxSeconds <= 21*60*60) {
			this.TicksX = [];
			for (var i = 1; i < 7; i++) {
				if (i*3*60*60 < maxSeconds)
					this.TicksX.push(i*3*60*60 / step);
			}
		} else if (maxSeconds <= 28*60*60) {
			this.TicksX = [];
			for (var i = 1; i < 7; i++) {
				if (i*4*60*60 < maxSeconds)
					this.TicksX.push(i*4*60*60 / step);
			}
		} else if (maxSeconds <= 42*60*60) {
			this.TicksX = [];
			for (var i = 1; i < 7; i++) {
				if (i*6*60*60 < maxSeconds)
					this.TicksX.push(i*6*60*60 / step);
			}
		} else if (maxSeconds <= 84*60*60) {
			this.TicksX = [];
			for (var i = 1; i < 7; i++) {
				if (i*12*60*60 < maxSeconds)
					this.TicksX.push(i*12*60*60 / step);
			}
		} else if (maxSeconds <= 168*60*60) {
			this.TicksX = [];
			for (var i = 1; i < 7; i++) {
				if (i*24*60*60 < maxSeconds)
					this.TicksX.push(i*24*60*60 / step);
			}
		}
		//
		this.show();
		//
		var sel = d3.select("." + this.uniqueString + "_removeable");
		if (sel) sel.remove();
		this.scaleXinverse = d3.scale.linear().domain([this.nMarginL ,  this.width  - this.nMarginR ]).range([0,data.length]);
		this.scaleYinverse = d3.scale.linear().domain([-this.nMarginB,-(this.height - this.nMarginT)]).range([0,dMax       ]);
		this.scaleX = d3.scale.linear().domain([0,data.length]).range([this.nMarginL ,  this.width  - this.nMarginR ]);
		this.scaleY = d3.scale.linear().domain([0,dMax       ]).range([-this.nMarginB,-(this.height - this.nMarginT)]);
		var line = d3.svg.line()
			.x(function(d,i) { return m_this.scaleX(i); })
			.y(function(d,i) { return m_this.scaleY(d); })
			;
		this.d3g1 = this.d3svg.append("g")
			.attr("id", this.g1id)
			.attr("class", this.uniqueString + "_removeable")
			.attr("transform", "translate(0,"+this.height+")")
			;
		//
		this.d3g1.append("rect")
			.attr("id", this.uniqueString + "_LegendRect")
			.attr("x", this.scaleX(data.length) + this.nMarginR*0.2)
			.attr("y", this.scaleY(dMax))
			.attr("width", this.nMarginR*0.6)
			.attr("height", Math.abs(this.scaleY(0) - this.scaleY(dMax)))
			.attr("fill", "url(#gradientLegend)")
			.attr("fill", "url(#" + this.legendGradient.uniqueString + ")")
			;
		this.d3xaxis = this.d3g1.append("line").attr("id","lineX" + this.uniqueString)
			.attr("x1", this.scaleX(0))
			.attr("y1", this.scaleY(0))
			.attr("x2", this.scaleX(data.length))
			.attr("y2", this.scaleY(0))
			.attr("stroke", this.sLineColorX)
			.attr("stroke-width", this.nLineWidthX)
			;
		this.d3yaxis = this.d3g1.append("line").attr("id","lineY" + this.uniqueString)
			.attr("x1", this.scaleX(0))
			.attr("y1", this.scaleY(0))
			.attr("x2", this.scaleX(0))
			.attr("y2", this.scaleY(dMax))
			.attr("stroke", this.sLineColorY)
			.attr("stroke-width", this.nLineWidthY)
			;
		var bands = [];
		for (var i = 0; i < m_this.getBands(); i++) {
			bands.push(i+1);
		}
		this.d3ygrid = this.d3g1
			.selectAll(".lineGridY")
			.data(bands)
			.enter().append("line")
			.attr("class", "lineGridY")
			.attr("id", function(d,i) { return "lineGridY" + d + this.uniqueString; })
			.attr("x1", this.scaleX(0))
			.attr("y1", function(d,i) { return m_this.scaleY(dMax * d / m_this.getBands()); })
			.attr("x2", this.scaleX(data.length))
			.attr("y2", function(d,i) { return m_this.scaleY(dMax * d / m_this.getBands()); })
			.attr("stroke", this.sLineColorX)
			.attr("stroke-width", this.nLineWidthX / 2)
			;
		this.d3xgrid = this.d3g1
			.selectAll("line." + this.uniqueString + "_xTicks")
			.data(this.TicksX)
			.enter().append("svg:line")
			.attr("class", this.uniqueString + "_xTicks")
			.attr("x1", function(d) { return m_this.scaleX(d); })
			.attr("y1", this.scaleY(0))
			.attr("x2", function(d) { return m_this.scaleX(d); })
			.attr("y2", this.scaleY(0)+3)
			.attr("stroke", this.sLineTickColorY)
			.attr("stroke-width", this.nLineTickWidthY)
			;
//		d3.select("#" + this.g1id)
//			.selectAll("line." + this.uniqueString + "_yTicks")
//			.data(this.scaleY.ticks(4))
//			.enter().append("line")
//			.attr("class",this.uniqueString + "_yTicks")
//			.attr("x1",this.scaleX(0))
//			.attr("y1",function(d) { return m_this.scaleY(d); })
//			.attr("x2",this.scaleX(0)-3)
//			.attr("y2",function(d) { return m_this.scaleY(d); })
//			.attr("stroke",this.sLineTickColorY)
//			.attr("stroke-width",this.nLineTickWidthY)
//			;
		this.d3line = this.d3g1.append("path")
			.attr("id","path" + this.uniqueString)
			.attr("d", line(data))
			.attr("fill", d3.rgb(0,0,0))
			.attr("fill-opacity", 0)
			.attr("stroke", this.sLineColor)
			.attr("stroke-width", this.nLineWidth)
			;
		this.d3title = this.d3g1.append("text")
			.attr("id", "text1" + this.uniqueString)
			.attr("stroke", this.sTextColor)
			.attr("stroke-width", this.nLineWidthText)
//			.attr("x", this.nMarginL * 0.1)
//			.attr("y", -1 * (this.height - Math.max(11, 0.2*this.nMarginT)))
			.attr("x", 0) // arbitrary
			.attr("y", 0) // arbitrary
			.style("font-size", convertToPx(Math.max(11, 0.2*this.nMarginT)))
			.text(this.sAttribute + " @ " + this.getNodeId())
			;
		//
		this.TimeLine = new TimeLine(this, this.g1id);
		this.TimeLine.move(this.scaleX(this.time));
		this.TimeLine.setY1(this.scaleY(0));
		this.TimeLine.setY2(this.scaleY(dMax));
		//
//		var width = 120;
//		var left = Math.max(width, this.width  - this.nMarginR - 120);
//		var top = this.height - (this.nMarginB + 20) / 2;
		//
		this.d3yaxisLabels = this.d3g1
			.selectAll("text." + this.uniqueString + "_yaxis")
			.data([0].concat(bands))
			.enter()
			.append("text")
			.attr("id", function(d,i) { return "textY" + d + m_this.uniqueString; })
			.attr("class", this.uniqueString + "_yaxis")
			.attr("x", 0) // arbitrary. resized.
			.attr("y", 0) // arbitrary. resized.
			.attr("text-anchor", "middle")
			.style("font-size", convertToPx(Math.max(11, 0.2 * this.nMarginT)))
			.text(function(d,i) { return SigFig(2, dMax * d / m_this.getBands()); })
			;
		//
		this.d3xaxisLabels =  this.d3g1
			.selectAll("text." + this.uniqueString + "_xaxis")
			.data(this.TicksX)
			.enter()
			.append("text")
			.attr("id", function(d,i) { return "textX" + d + m_this.uniqueString; })
			.attr("class", this.uniqueString + "_xaxis")
			.attr("x", 0) // arbitrary. resized.
			.attr("y", 0) // arbitrary. resized.
			.attr("text-anchor", "middle")
			.style("font-size", convertToPx(Math.max(11, 0.2 * this.nMarginT)))
			.text(function(d,i) {
				var nSeconds = d * m_this.nStep;
				var sTime = convertSecondsToTime(nSeconds);
				return sTime;
			})
			;
		this.d3xaxisTitle = this.d3g1.append("text")
			.attr("id", this.uniqueString + "xaxis_label")
			.attr("class", this.uniqueString + "_xaxis")
			.attr("x", 0) // arbitrary. resized.
			.attr("y", 0) // arbitrary. resized.
			.attr("stroke", this.sTextColor)
			.attr("stroke-width", this.nLineWidthText)
			.attr("text-anchor", "middle")
			.style("font-size", convertToPx(Math.max(11, 0.2 * this.nMarginT)))
			.text("Time (hr:min)")
			;
		//
		// create play button
		//
		this.d3playBack = this.d3g1.append("path").attr("id", this.playbackid)
			.attr("d", this.pathPlayBack)
			.attr("stroke-width", 1.5)
			.attr("stroke-linecap","round")
			.attr("stroke", d3.rgb(0,0,0))
			.attr("stroke-opacity", 0.5)
			.attr("fill", d3.rgb(0,0,0))
			.attr("fill-opacity", 0.5)
			;
		this.d3playFore = this.d3g1.append("path").attr("id", this.playforeid)
			.attr("d", this.pathPlayFore)
			.attr("stroke-width", 1.5)
			.attr("stroke-linecap","round")
			.attr("stroke", d3.rgb(0,0,0))
			.attr("stroke-opacity", 0.5)
			.attr("fill", d3.rgb(0,0,0))
			.attr("fill-opacity", 0.5)
			;
		this.d3ellipseL = this.d3g1.append("ellipse").attr("id", this.ellipselid)
			.attr("cx", this.playX - this.playR * 0.5) // arbitrary
			.attr("cy", this.playY) // arbitrary
			.attr("rx", this.playR * 1.5) // arbitrary
			.attr("ry", this.playR * 0.8) // arbitrary
			.attr("stroke-width", 1.5)
			.attr("stroke", d3.rgb(0,0,0))
			.attr("stroke-opacity", 0)
			.attr("fill", d3.rgb(0,150,0))
			.attr("fill-opacity", 0.5)
			;
		this.d3ellipseR = this.d3g1.append("ellipse").attr("id", this.ellipserid)
			.attr("cx", this.playX + this.playR * 0.5) // arbitrary
			.attr("cy", this.playY) // arbitrary
			.attr("rx", this.playR * 1.5) // arbitrary
			.attr("ry", this.playR * 0.8) // arbitrary
			.attr("stroke-width", 1.5)
			.attr("stroke", d3.rgb(0,0,0))
			.attr("stroke-opacity", 0)
			.attr("fill", d3.rgb(0,150,0))
			.attr("fill-opacity", 0.5)
			;
		this.d3circle2 = this.d3g1.append("circle").attr("id", this.circle2id)
			.attr("cx", this.playX) // arbitrary
			.attr("cy", this.playY) // arbitrary
			.attr("r",  this.playR) // arbitrary
			.attr("stroke-width", 1.5)
			.attr("stroke", d3.rgb(0,0,0))
			.attr("stroke-opacity", 0.0)
			.attr("fill", d3.rgb(255,255,255))
			.attr("fill-opacity", 1.0)
			;
		this.d3play = this.d3g1.append("path").attr("id", this.playid)
			.attr("d", this.pathPlay)
			.attr("stroke-width", 2)
			.attr("stroke-linecap","round")
			.attr("stroke", d3.rgb(0,0,0))
			.attr("stroke-opacity", 0.5)
			.attr("fill", d3.rgb(0,0,0))
			.attr("fill-opacity", 0.5)
			;
		this.d3circle = this.d3g1.append("circle").attr("id", this.circleid)
			.attr("cx", this.playX)
			.attr("cy", this.playY)
			.attr("r",  this.playR)
			.attr("stroke", "none")
			.attr("fill", d3.rgb(0,150,0))
			.attr("fill-opacity", 0.5)
			;
		//
		// create move corner
		//
		this.d3move = this.d3g1
			.selectAll("rect." + this.moveid)
			.data([{"x":1,"y":1},{"x":1,"y":2},{"x":1,"y":3},{"x":2,"y":1},{"x":2,"y":2},{"x":3,"y":1}])
			.enter()
			.append("rect")
			.attr("id", function(d,i) { return m_this.moveid + (i + 1); })
			.attr("class", this.moveid)
			.attr("x", function(d,i){ return +1*(m_this.width  - 1 - d.x*3); })
			.attr("y", function(d,i){ return -1*(m_this.height - 1 - d.y*3); })
			.attr("width", 2)
			.attr("height", 2)
			.attr("fill", d3.rgb(150,150,150))
			.attr("fill-opacity", 0.5)
			;
		this.d3moveCover = this.d3g1.append("polygon")
			.attr("id", this.movecoverid)
			.attr("points",	(this.width     ) + "," + (-this.height     ) + " " + 
							(this.width     ) + "," + (-this.height + 16) + " " +
							(this.width - 16) + "," + (-this.height     )       )
			.attr("stroke", "none")
			.attr("fill", d3.rgb(0,0,0))
			.attr("fill-opacity", 0.0)
			.attr("cursor","ne-resize")
			;
		//
		this.resize();
		this.addListeners();
//		if (m_this.time > -1) {
//			this.getErdDataDynamic(function() {
//				m_this.updateGraph(m_this.time);
//			});
//		}
	} else {
		this.hide();
	}
}

NodeGraph.prototype.setPlayButton = function(sPath) {
	if (this.d3play)
		this.d3play.attr("d", sPath);
}

NodeGraph.prototype.setLeft = function(x) {
	this.d3g.style("left", convertToPx(x));
}

NodeGraph.prototype.setTop = function(y) {
	this.top = y;
	this.d3g.style("top", convertToPx(y));
}

NodeGraph.prototype.hide = function() {
	this.nPlaying = NodeGraph.ANIMATION_READY;
	this.setPlayButton(this.pathPlay);
	clearInterval(this.getAnimationProcessor());
	//
	this.d3g.style("visibility","hidden");
//	if (this.parent) {
		var id = this.getNodeId();
		var sel = this.parent.getNode(id);
		sel.attr("stroke-width",0);
		sel.attr("stroke","rgb(0,0,0,0)");
//	}
}

NodeGraph.prototype.show = function() {
	this.nPlaying = NodeGraph.ANIMATION_READY;
	this.d3g.style("visibility","inherit");
	var val = this.d3g.style("visibility");
}

NodeGraph.prototype.isHidden = function() {
	var val = this.d3g.style("visibility");
	return val == "hidden";
}

NodeGraph.prototype.isVisible = function() {
	return !this.isHidden();
}

NodeGraph.prototype.getErdData = function(fNextStep/*(data)*/) {
	var m_this = this;
	if (this.getDynamicType() == "Gabe") {		
		fNextStep();
		return;
	}
	var uuid = this.getSimId();
	var sourceId = this.getSource();
	var nodeId = this.getNodeId();
	var url = Couch.erd + "?uuid=" + uuid + "&attribute=" + this.sAttribute + "&call=static&nodeId=" + nodeId + "&sourceId=" + sourceId;
	Couch.GetDoc(url, function(data) {
		if (data && data.debug) {
			alert(data.debug);
			return;
		}
		if (m_this.getNodeId().length == 0 && data)
			m_this.setNodeId(data.id);
		m_this.data.Static = data;
		if (m_this.nPlaying == NodeGraph.ANIMATION_READY) {
			m_this.draw();
		} else {
			m_this.resize();
		}
		fNextStep(data);
	});
}

NodeGraph.prototype.getErdDataDynamic = function(fNextStep/*(data)*/) {
	var m_this = this;
	var uuid = this.getSimId();
	var sourceId = this.getSource();
	//
	if (m_this.getDynamicType() == "Gabe") {
		fNextStep();
		return;
	}
	//
	var bBusy = false;
	var nNodes = this.parent.getData().Nodes.length;
	var nGroupSize = 10;
	var nGroupSize = Math.ceil(100000 / nNodes);//6
	var nGroupSize = Math.ceil(200000 / nNodes);//12
	var nGroupSize = Math.ceil(300000 / nNodes);//17
	var nGroupSize = Math.ceil(400000 / nNodes);//23
	var nGroupSize = Math.ceil(500000 / nNodes);
	var nGroupSize = Math.ceil(1000000 / nNodes);
	var nGroupSize = Math.ceil(1520000 / nNodes);//85
	var nGroupSize = Math.ceil(1785714 / nNodes);//100
	var nGroupSize = Math.ceil(2034000 / nNodes);//114
	var nGroupSize = Math.ceil(6048000 / nNodes);//338
	var nGroupSize = Math.ceil(4050000 / nNodes);//226
	var nGroupSize = Math.ceil(12114000 / nNodes);//675
	//var nGroupSize = 13;
	GlobalData.getErdDataStart = new Date().getTime();
	//console.log("group.size = ", nGroupSize);
	var iGroup = -1;
	var iStart;
	var iEnd;
	var dZero = this.data.Static[this.sAttribute + "Max"]/1000;
	var dZero = 0;
	var dMax = this.data.Static[this.sAttribute + "Max"];
	var url = Couch.erd + "?uuid=" + uuid + "&attribute=" + this.sAttribute + "&call=dynamic" + this.getDynamicType() + "&sourceId=" + sourceId + "&t0={0}&t1={1}&zero=" + dZero + "&max=" + dMax + "&bands=" + this.getBands();
	var process = function(url, t0, t1, callback) {
		Couch.GetDoc(url.format(t0,t1), function(data) {
			if (data && data.debug) {
				alert(data.debug);
				return;
			}
			if (m_this.getDynamicType() < 5) {
				if (t0 == 0) {
					m_this.data.Dynamic = data.Dynamic;
				} else {
					for (var id in data.Dynamic[m_this.sAttribute + "s"]) {
						m_this.data.Dynamic[m_this.sAttribute + "s"][id] = m_this.data.Dynamic[m_this.sAttribute + "s"][id].concat(data.Dynamic[m_this.sAttribute + "s"][id]);					
					}
				}
			} else {
				if (!m_this.data) m_this.data = {};
				if (t0 == 0) m_this.data.Dynamic = {};
				for (var key in data.Dynamic) {
					if (key == m_this.sAttribute + "s") {
						if (t0 == 0) m_this.data.Dynamic[key] = {};
						for (var id in data.Dynamic[key]) {
							var node = data.Dynamic[key][id];
							if (t0 == 0) m_this.data.Dynamic[key][id] = [];
							for (var i = 0; i < node.length; i++) {
								var pair = node[i];
								for (var j = 0; j < pair[0]; j++) {
									m_this.data.Dynamic[key][id] = m_this.data.Dynamic[key][id].concat(pair[1]);
								}
							}
						}
					} else {
						if (t0 == 0) {
							m_this.data.Dynamic[key] = data.Dynamic[key];
						}
					}
				}
			}
/*   */		var nCount = 0;
/*   */		//for (var id in res.data.Dynamic[m_this.sAttribute + "s"]) {
/*   */		for (var id in data.Dynamic[m_this.sAttribute + "s"]) {
/*   */			nCount++;
/*   */		}
			// if a particular node has zero's over the entire time frame they might not exist in the data returned from the server.
			for (var i = 0; i < m_this.parent.getData().Nodes.length; i++) {
				var id = m_this.parent.getData().Nodes[i].id;
				if (m_this.data.Dynamic[m_this.sAttribute + "s"][id] == null)
					m_this.data.Dynamic[m_this.sAttribute + "s"][id] = [];
				var jStart = m_this.data.Dynamic[m_this.sAttribute + "s"][id].length;
				var jEnd = Math.min(t1, m_this.data.Dynamic.numSteps);
				for (var j = jStart; j < jEnd; j++) {
					m_this.data.Dynamic[m_this.sAttribute + "s"][id][j] = 0.0;
				}
			}
/*   */		for (var id in m_this.data.Dynamic[m_this.sAttribute + "s"]) {
/*   */			var nLength = m_this.data.Dynamic[m_this.sAttribute + "s"][id].length;
/*   */			var nStart = GlobalData.getErdDataStart;
/*   */			var nEnd = new Date().getTime();
/*   */			var nTime = nEnd - nStart
/*   */			var nTotal = Math.ceil(nTime / nLength * m_this.data.Dynamic.numSteps / 1000);
/*   */			//console.log(nLength, nCount, nStart, nEnd, nTime, nTotal);
/*   */			break
/*   */		}
			bBusy = false;
			if (iGroup == 0) fNextStep(data);
		});
	}
	var processor = setInterval(function() {
		if (!bBusy) {
			bBusy = true;
			iGroup++;
			iStart = iGroup * nGroupSize;
			if (iGroup == 0 || iStart < m_this.data.Dynamic.numSteps) {
				iEnd = iStart + (nGroupSize - 1);
				if (iGroup > 0 && iEnd >= m_this.data.Dynamic.numSteps) iEnd = m_this.data.Dynamic.numSteps - 1;
				process.call(m_this, url, iStart, iEnd, null);
			} else {
				clearInterval(processor);
			}
		}
	}, 15);
	this.setAnimationProcessor(processor);
}

NodeGraph.prototype.playAnimation = function() {
	var m_this = this;
	m_Waiting.show();
	this.TimeLine.disableMoveCursor();
	this.nPlaying = NodeGraph.ANIMATION_PLAYING;
	this.setPlayButton(this.pathPause);
	this.getErdDataDynamic(function(data) {
		m_Waiting.hide();
		var dMax = m_this.data.Static[m_this.sAttribute + "Max"];
		var sel = m_this.parent.getNodes();
		var dMax = m_this.data.Static[m_this.sAttribute + "Max"];
		var getcolor;
		if (m_this.interpType == "hsl") {
			getcolor = d3.interpolateHsl(m_this.interpColor0, m_this.interpColor1)
		} else {
			getcolor = d3.interpolateRgb(m_this.interpColor0, m_this.interpColor1)
		}
		sel.attr("fill-opacity", 1.0);
		//
		var nSteps = m_this.data.Dynamic.numSteps;
		var nDelay = 200;
		var doAnimation = function(t) {
			var itime = t;
			m_this.time = t;
			// these two lines need to be in this loop in case this.create is called
			// which wipes out the drawing so its got to be recreated
			// or in case the NodeGraph is resized by grabbing the top-right corner
			m_this.TimeLine.setY1(m_this.scaleY(0));
			m_this.TimeLine.setY2(m_this.scaleY(dMax));
			m_this.TimeLine.move(m_this.scaleX(t));
			var nTimeStep = m_this.data.Dynamic.timeStep;
			var nTimeStart = m_this.data.Dynamic.timeStart;
			//var dVal = m_this.data.Dynamic[m_this.sAttribute + "s"][m_this.getNodeId()][itime.toString()];
			var dVal = m_this.data.Static[m_this.sAttribute + "s"][itime.toString()];
			var sVal = (dVal == null) ? "?" : "" + dVal;
			var sData = "C";
			var sTime = convertSecondsToTime(nTimeStart + itime * nTimeStep);
			m_this.d3title.text(m_this.sAttribute + " @ " + m_this.getNodeId() + 
				", t = " + sTime +
				", " + sData + " = " + dVal)
				;
			sel.attr("fill",function(d,i) {
				//var dVal = m_this.data.Dynamic[m_this.sAttribute + "s"][d.id][itime.toString()];
				//var ratio = dVal / dMax;
				var data           = m_this.data;
				var Dynamic        = (data          ) ? m_this.data.Dynamic                : null;
				var Concentrations = (Dynamic       ) ? Dynamic[m_this.sAttribute + "s"]   : null;
				var node           = (Concentrations) ? Concentrations[d.id]               : null;
				var iBand          = (node          ) ? node[itime.toString()]             : null;
				var max = (m_this.getDynamicType() == "Gabe") ? dMax : m_this.getBands();
				var ratio = (iBand == null) ? null : iBand / max;
				if (ratio > 1) ratio = 1;
				if (ratio < 0) ratio = 0;
				var color = (ratio == null) ? "black" : getcolor(ratio);
				return color;
			});
			//
			if (m_this.nPlaying == NodeGraph.ANIMATION_REWIND) {
				m_this.nPlaying = NodeGraph.ANIMATION_PLAYING;
				itime = 0;
			}
			if (m_this.nPlaying == NodeGraph.ANIMATION_FASTFORWARD) {
				m_this.nPlaying = NodeGraph.ANIMATION_PLAYING;
				itime = nSteps - 2;
			}
			if (m_this.nPlaying == NodeGraph.ANIMATION_STOPPED) {
				return;
			}
			if (m_this.nPlaying == NodeGraph.ANIMATION_READY  ) {
				m_this.TimeLine.hide();
				return;
			}
			if (++itime < nSteps) {
				setTimeout(doAnimation, nDelay, itime);
			} else {
				m_this.stopAnimation();
			}
		}
		//doAnimation(0);
		doAnimation(m_this.startTime);
	});
}

NodeGraph.prototype.initialize = function() {
	this.time = -1e9;
	this.startTime = 0;
}

NodeGraph.prototype.resetAnimation = function() {
	this.startTime = 0;
	this.nPlaying = NodeGraph.ANIMATION_READY;
	this.setPlayButton(this.pathPlay);
	this.TimeLine.hide();
	this.d3title.text(this.sAttribute + " @ " + this.getNodeId());
	var sel = this.parent.d3view.selectAll("circle.Junction");
	sel.attr("fill","rgba(0,0,0," + this.parent.dOpacity + ")");
	var sel = this.parent.d3view.selectAll("path.Reservoir");
	sel.attr("fill","rgba(0,155,0," + this.parent.dOpacity + ")");
	var sel = this.parent.d3view.selectAll("path.Tank");
	sel.attr("fill","rgba(0,0,155," + this.parent.dOpacity + ")");
}

NodeGraph.prototype.stopAnimation = function() {
	this.nPlaying = NodeGraph.ANIMATION_STOPPED;
	//this.setPlayButton(this.pathRewind);
	this.setPlayButton(this.pathPlay);
	if (this.TimeLine) this.TimeLine.enableMoveCursor();
}

NodeGraph.prototype.close = function() {
}

NodeGraph.prototype.createPlayButton = function() {
	//this.playX = this.width - this.nMarginR/2;
	//this.playY = -this.nMarginB/2;
	this.playR = Math.max(7, Math.min(this.nMarginB / 3, this.nMarginR / 3));
	this.playX = this.width - 2 * this.playR - 4;
	this.playY =          0 - 1 * this.playR - 4;
	var x = this.playX;
	var y = this.playY;
	var r = this.playR;
	this.pathPlay	= "M" + (x-r*2/7) + " " + (y-r*2/7)
					+ "L" + (x+r*2/7) + " " + (y-r*0/7)
					+ "L" + (x-r*2/7) + " " + (y+r*2/7)
					+ "L" + (x-r*2/7) + " " + (y-r*2/7)
					;
	this.pathPause 	= "M" + (x-r*3/7) + " " + (y-r*2/6)
					+ "L" + (x-r*3/7) + " " + (y+r*2/6)
					+ "L" + (x-r*2/7) + " " + (y+r*2/6)
					+ "L" + (x-r*2/7) + " " + (y-r*2/6)
					+ "L" + (x-r*3/7) + " " + (y-r*2/6)
					+ "M" + (x+r*3/7) + " " + (y-r*2/6)
					+ "L" + (x+r*3/7) + " " + (y+r*2/6)
					+ "L" + (x+r*2/7) + " " + (y+r*2/6)
					+ "L" + (x+r*2/7) + " " + (y-r*2/6)
					+ "L" + (x+r*3/7) + " " + (y-r*2/6)
					;
	this.pathRewind	= "M" + (x-r*3/7) + " " + (y-r*2.5/7)
					+ "L" + (x-r*3/7) + " " + (y+r*2.5/7)
					+ "M" + (x-r*1/7) + " " + (y-r*0/7)
					+ "L" + (x+r*2/7) + " " + (y+r*2/7)
					+ "L" + (x+r*2/7) + " " + (y-r*2/7)
					+ "L" + (x-r*1/7) + " " + (y-r*0/7)
					;
	var x = this.playX - this.playR * 1.6;
	this.pathPlayBack	= "M" + (x-r*1/7) + " " + (y-r*0/7)
						+ "L" + (x+r*2/9) + " " + (y-r*2/7)
						+ "M" + (x-r*1/7) + " " + (y-r*0/7)
						+ "L" + (x+r*2/9) + " " + (y+r*2/7);
	var x = this.playX + this.playR * 1.6;
	this.pathPlayFore	= "M" + (x+r*1/7) + " " + (y-r*0/7)
						+ "L" + (x-r*2/9) + " " + (y-r*2/7)
						+ "M" + (x+r*1/7) + " " + (y-r*0/7)
						+ "L" + (x-r*2/9) + " " + (y+r*2/7);
}

NodeGraph.prototype.resize = function(deltaX, deltaY) {
	var m_this = this;
	//
	var sTime = " ";
	var nTime = 0;
	var sVal = " ";
	var Dynamic = this.data ? this.data.Dynamic : null;
	if (Dynamic != null) {
		var nTimeStep = Dynamic.timeStep;
		var nTimeStart = Dynamic.timeStart;
		var nTime = nTimeStart + this.time * nTimeStep;
		var sTime = ", t = " + convertSecondsToTime(nTime);
		var sAttr = "C";
		var dVal = this.data.Static[this.sAttribute + "s"][Math.round(this.time).toString()];
		var sVal = ", " + sAttr + " = " + dVal;
	}
	if (nTime < 0)
		this.d3title.text(this.sAttribute + " @ " + this.getNodeId());
	else
		this.d3title.text(this.sAttribute + " @ " + this.getNodeId() + sTime + sVal);
	//
	var dMaxWidth = NodeGraph.DEFAULT_MARGIN;
	for (var i = 0; i < this.getBands(); i++) {
		var obj = document.getElementById("textY" + (i+1) + this.uniqueString);
		var box = obj.getBBox();
		dMaxWidth = Math.max(dMaxWidth, box.width + 5);
	}
	//
	this.nMarginL = Math.max(dMaxWidth,                this.width  / 10 );
	this.nMarginR = Math.max(NodeGraph.DEFAULT_MARGIN, this.width  / 10 );
	this.nMarginT = Math.max(NodeGraph.DEFAULT_MARGIN, this.height / 10 + 20);
	this.nMarginB = Math.max(NodeGraph.DEFAULT_MARGIN, this.height / 10 + 20);
	//this.nMarginW = this.nMarginL + this.nMarginR;
	//
	this.d3g  .style("top"   , convertToPx(this.top));
	this.d3svg.style("width" , convertToPx(this.width));
	this.d3svg.style("height", convertToPx(this.height));
	this.d3rect.attr("width" , this.width);
	this.d3rect.attr("height", this.height);
	this.d3g1.attr("transform", "translate(0," + this.height + ")");
	//
	var left = Math.max(0, this.width  - this.nMarginR - 120);
	var top = this.height - (this.nMarginB + 20)/2;
	//
	d3.select("#" + "text2" + this.uniqueString)
		.attr("x", this.width / 2)
		.attr("y", -1 * this.nMarginB / 2 + 5)
		;
	this.d3move
		.attr("x",function(d,i){ return +1 * (m_this.width  - 1 - d.x * 3); })
		.attr("y",function(d,i){ return -1 * (m_this.height - 1 - d.y * 3); })
		;
	this.d3moveCover
		.attr("points",	(this.width     ) + "," + (-this.height     ) + " " + 
						(this.width     ) + "," + (-this.height + 16) + " " +
						(this.width - 16) + "," + (-this.height     )       )
		;
	this.d3title
		.attr("x", this.nMarginL * 0.1)
		.attr("y", -1 * (this.height  - Math.max(11, 0.5 * this.nMarginT) )) // 0.2 // 0.3
		.style("font-size", convertToPx(Math.max(11, 0.3 * this.nMarginT))) // 0.2
		;
	this.createPlayButton();
	if (this.nPlaying == NodeGraph.ANIMATION_READY) {
		var d = this.pathPlay;
	} else if (this.nPlaying == NodeGraph.ANIMATION_PLAYING) {
		var d = this.pathPause;
	} else if (this.nPlaying == NodeGraph.ANIMATION_STOPPED) {
		//var d = this.pathRewind;
		var d = this.pathPlay;
	}
	this.setPlayButton(d);
	this.d3playBack.attr("d",  this.pathPlayBack)
	this.d3playFore.attr("d",  this.pathPlayFore)
	this.d3circle  .attr("cx", this.playX);
	this.d3circle  .attr("cy", this.playY);
	this.d3circle  .attr("r" , this.playR);
	this.d3circle2 .attr("cx", this.playX);
	this.d3circle2 .attr("cy", this.playY);
	this.d3circle2 .attr("r" , this.playR * 1.07);
	this.d3ellipseL.attr("cx", this.playX - this.playR * 0.5);
	this.d3ellipseL.attr("cy", this.playY);
	this.d3ellipseL.attr("rx", this.playR * 1.5);
	this.d3ellipseL.attr("ry", this.playR * 0.8);
	this.d3ellipseR.attr("cx", this.playX + this.playR * 0.5);
	this.d3ellipseR.attr("cy", this.playY);
	this.d3ellipseR.attr("rx", this.playR * 1.5);
	this.d3ellipseR.attr("ry", this.playR * 0.8);
	//
	var data = this.data.Static[this.sAttribute + "s"];
	var dMax = this.data.Static[this.sAttribute + "Max"];
	if (dMax == 0) dMax = 1;
	this.scaleXinverse = d3.scale.linear().domain([this.nMarginL ,  this.width  - this.nMarginR ]).range([0,data.length]);
	this.scaleYinverse = d3.scale.linear().domain([-this.nMarginB,-(this.height - this.nMarginT)]).range([0,dMax       ]);
	this.scaleX = d3.scale.linear().domain([0,data.length]).range([this.nMarginL,this.width  - this.nMarginR]);
	this.scaleY = d3.scale.linear().domain([0,dMax       ]).range([-this.nMarginB,-(this.height - this.nMarginT)]);
	var line = d3.svg.line()
		.x(function(d,i) { return m_this.scaleX(i); })
		.y(function(d,i) { return m_this.scaleY(d); })
		;
	d3.select("#" + this.uniqueString + "_LegendRect")
		.attr("x",this.scaleX(data.length) + this.nMarginR * 0.2)
		.attr("y",this.scaleY(dMax))
		.attr("width",this.nMarginR*0.6)
		.attr("height",Math.abs(this.scaleY(0) - this.scaleY(dMax)))
		;
	this.d3xaxis.attr("x1",this.scaleX(0))
	this.d3xaxis.attr("y1",this.scaleY(0))
	this.d3xaxis.attr("x2",this.scaleX(data.length))
	this.d3xaxis.attr("y2",this.scaleY(0))
	this.d3yaxis.attr("x1",this.scaleX(0));
	this.d3yaxis.attr("y1",this.scaleY(0));
	this.d3yaxis.attr("x2",this.scaleX(0));
	this.d3yaxis.attr("y2",this.scaleY(dMax));
	this.d3ygrid.attr("x1",this.scaleX(0));
	this.d3ygrid.attr("y1",function(d,i) { return m_this.scaleY(dMax * d / m_this.getBands()); });
	this.d3ygrid.attr("x2",this.scaleX(data.length));
	this.d3ygrid.attr("y2",function(d,i) { return m_this.scaleY(dMax * d / m_this.getBands()); });
	this.d3xgrid.attr("x1",function(d) { return m_this.scaleX(d); });
	this.d3xgrid.attr("y1",this.scaleY(0));
	this.d3xgrid.attr("x2",function(d) { return m_this.scaleX(d); });
	this.d3xgrid.attr("y2",this.scaleY(0)+3);
//	d3.selectAll("line." + this.uniqueString + "_yTicks")
//		.attr("x1",this.scaleX(0))
//		.attr("y1",function(d) { return m_this.scaleY(d); })
//		.attr("x2",this.scaleX(0)-3)
//		.attr("y2",function(d) { return m_this.scaleY(d); })
//		;
	this.d3line.attr("d", line(data));
	var obj = document.getElementById("textY1" + this.uniqueString);
	var box = obj.getBBox();
	this.d3yaxisLabels.attr("x", this.nMarginL / 2);
	this.d3yaxisLabels.attr("y", function(d,i) { return m_this.scaleY(dMax * d / m_this.getBands()) + 0.3 * box.height; });
	this.d3xaxisLabels.attr("x", function(d,i) { return m_this.scaleX(d); });
	this.d3xaxisLabels.attr("y", this.scaleY(0) + 15);
	this.d3xaxisTitle .attr("x", this.scaleX(data.length / 2));
	this.d3xaxisTitle .attr("y", this.scaleY(0) + 0.8 * this.nMarginB);
	//
	this.TimeLine.setY1(this.scaleY(0));
	this.TimeLine.setY2(this.scaleY(dMax));
	this.TimeLine.move(this.scaleX(this.time));
}

///////////////////////////////////////////////////////

NodeGraph.prototype.eventDragTimeLine = function(clientX) {
	//var itime = Math.round(this.scaleXinverse(clientX));
	var itime = this.scaleXinverse(clientX);
	var retVal = this.updateGraph(itime);
	return {"bLimited":retVal.bLimited,"newX":this.scaleX(retVal.itime)};
}
NodeGraph.prototype.updateGraph = function(itime) {
	if (this.nPlaying == NodeGraph.ANIMATION_READY) return {"bLimited":true,"newX":-10000};
	var bLimited = false;
	var m_this = this;
	if (itime < 0) {
		bLimited = true;
		itime = 0;
	}
	var dMax = this.data.Static[m_this.sAttribute + "Max"];
	var nTimeStep = this.data.Dynamic.timeStep;
	var nTimeStart = this.data.Dynamic.timeStart;
	var nSteps = this.data.Dynamic.numSteps;
	if (itime > nSteps - 1) {
		bLimited = true;
		itime = nSteps - 1;
	}
	//var dVal = this.data.Dynamic[sData][this.getNodeId()][itime.toString()];
	//var dVal = this.data.Dynamic[m_this.sAttribute + "s"][this.getNodeId()][Math.round(itime).toString()];
	var dVal = this.data.Static[m_this.sAttribute + "s"][Math.round(itime).toString()];
	if (dVal == null) {
		return;
	}
	//this.time = itime; // TODO - get max time by getting number of time steps?
	this.time = Math.round(itime); // TODO - get max time by getting number of time steps?
	var sel = this.parent.getNodes();
	var data = this.data.Static[m_this.sAttribute + "s"];
	var getcolor;
	if (this.interpType == "hsl") {
		getcolor = d3.interpolateHsl(this.interpColor0, this.interpColor1)
	} else {
		getcolor = d3.interpolateRgb(this.interpColor0, this.interpColor1)
	}
	var sData = "C";
	var sTime = convertSecondsToTime(nTimeStart + this.time * nTimeStep);
	this.d3title.text(
		this.sAttribute + " @ " + this.getNodeId() + 
		", t = " + sTime +
		", " + sData + " = " + dVal)
	sel.attr("fill", function(d,i) {
//		var dVal = m_this.data.Dynamic[m_this.sAttribute + "s"][d.id][m_this.time.toString()];
//		var ratio = dVal / dMax;
		var data           = m_this.data;
		var Dynamic        = (data          ) ? m_this.data.Dynamic                : null;
		var Concentrations = (Dynamic       ) ? Dynamic[m_this.sAttribute + "s"]   : null;
		var node           = (Concentrations) ? Concentrations[d.id]               : null;
		var iBand          = (node          ) ? node[Math.round(itime).toString()] : null;
		var max = (m_this.getDynamicType() == "Gabe") ? dMax : m_this.getBands();
		var ratio = (iBand == null) ? null : iBand / max;
		if (ratio > 1) ratio = 1;
		if (ratio < 0) ratio = 0;
		var color = (ratio == null) ? "black" : getcolor(ratio);
		return color;
	});
	return {"bLimited":bLimited,"itime":itime};
}

NodeGraph.prototype.doDrag = function(x, y) {
	if (this.dragger.dragging) {
		x = x ? x : (d3.event ? d3.event.screenX : 0);
		y = y ? y : (d3.event ? d3.event.screenY : 0);
		var deltaX = x - this.dragger.p.x;
		var deltaY = y - this.dragger.p.y;
		var scaleX = (this.width  + deltaX) / this.width;
		var scaleY = (this.height - deltaY) / this.height;
		this.dragger.p.x += deltaX;
		this.dragger.p.y += deltaY;
		this.top += deltaY;
		this.width += deltaX;
		this.height -= deltaY;
		this.resize(deltaX, deltaY);
	}
}

NodeGraph.prototype.onMouseMove = function(sid) {
	this.d3rect.transition().duration(20).attr("fill-opacity", 1);
	this.doDrag();
}

NodeGraph.prototype.onMouseOver = function(sid) {
	this.d3rect.transition().duration(20).attr("fill-opacity", 1);
	var sel = d3.select("#" + sid);
	switch(sid) {
		case this.circleid:
			sel.attr("fill", d3.rgb(0, 210, 0));
			sel.attr("fill-opacity", 0.3);
			sel.attr("r", this.playR * 7 / 6);
			this.d3circle2.attr("stroke-opacity", 0.7);
			this.d3play.attr("stroke", d3.rgb(0, 0, 0));
			this.d3play.attr("stroke-opacity", 1.0);
			this.d3play.attr("fill", d3.rgb(0, 0, 0));
			this.d3play.attr("fill-opacity", 0.7);
			break;
		case this.ellipselid:
			this.d3ellipseL.attr("stroke-opacity", 0.6);
			this.d3ellipseL.attr("fill", d3.rgb(0, 210, 0));
			this.d3ellipseL.attr("fill-opacity", 0.3);
			this.d3playBack.attr("stroke-opacity", 0.8)
			break;
		case this.ellipserid:
			this.d3ellipseR.attr("stroke-opacity", 0.6);
			this.d3ellipseR.attr("fill", d3.rgb(0, 210, 0));
			this.d3ellipseR.attr("fill-opacity", 0.3);
			this.d3playFore.attr("stroke-opacity", 0.8);
			break;
		case this.movecoverid:
			this.d3move
				.attr("fill", d3.rgb(70, 70, 70))
				.attr("fill-opacity", 0.7)
				;
			break;
	}
}

NodeGraph.prototype.onMouseOut = function(sid) {
	var bSame = HasAncestorId(d3.event.relatedTarget, this.gid);
	if (!bSame) this.d3rect.transition().duration(3000).attr("fill-opacity", this.sBgOpacity);
	this.doDrag();
	var sel = d3.select("#" + sid);
	switch(sid) {
		case this.circleid:
			sel.attr("fill", d3.rgb(0, 150, 0));
			sel.attr("fill-opacity", 0.5);
			sel.attr("r", this.playR);
			this.d3circle2.attr("stroke-opacity", 0.0);
			this.d3play.attr("stroke", d3.rgb(0, 0, 0));
			this.d3play.attr("stroke-opacity", 0.5);
			this.d3play.attr("fill", d3.rgb(0, 0, 0));
			this.d3play.attr("fill-opacity", 0.5);
			break;
		case this.ellipselid:
			this.d3ellipseL.attr("stroke-opacity", 0);
			this.d3ellipseL.attr("fill", d3.rgb(0, 150, 0));
			this.d3ellipseL.attr("fill-opacity", 0.5);
			this.d3playBack.attr("stroke-opacity", 0.5)
			break;
		case this.ellipserid:
			this.d3ellipseR.attr("stroke-opacity", 0);
			this.d3ellipseR.attr("fill", d3.rgb(0, 150, 0));
			this.d3ellipseR.attr("fill-opacity", 0.5);
			this.d3playFore.attr("stroke-opacity", 0.5);
			break;
		case this.movecoverid:
			this.d3move
				.attr("fill", d3.rgb(150, 150, 150))
				.attr("fill-opacity", 0.5)
				;
			break;
	}
}

NodeGraph.prototype.onMouseUp = function(sid) {
	if (this.TimeLine) this.TimeLine.onMouseUp();
	this.dragger.dragging = false;
	if (!sid) return;
	switch(sid) {
		case this.movecoverid:
			this.d3move
				.attr("fill", d3.rgb(90, 90, 90))
				.attr("fill-opacity", 0.6)
				;
			var sTop = this.d3g.style("top");
			this.top = parseFloat(sTop.substr(0,sTop.length-2));
			break;
	}
}

NodeGraph.prototype.onMouseDown = function(sid) {
	switch(sid) {
		case this.movecoverid:
			this.d3move.attr("fill", d3.rgb(0, 0, 0));
			this.d3move.attr("fill-opacity", 0.9);
			this.dragger.dragging = true;
			this.dragger.p.x = d3.event.screenX;
			this.dragger.p.y = d3.event.screenY;
			var sTop = this.d3g.style("top");
			this.top = parseFloat(sTop.substr(0,sTop.length-2));
			break;
	}
}

//NodeGraph.prototype.onMouseWheel = function(sid) {}
NodeGraph.prototype.onKeyDown = function(sid) {}
NodeGraph.prototype.onKeyPress = function(sid) {}
NodeGraph.prototype.onKeyUp = function(sid) {}
NodeGraph.prototype.onInput = function(sid) {}
NodeGraph.prototype.onChange = function(sid) {}

NodeGraph.prototype.onClick = function(sid) {
	var m_this = this;
	var sel = d3.select("#" + sid);
	switch(sid) {
		case this.circleid:
			if (this.nPlaying == NodeGraph.ANIMATION_READY) {
				this.playAnimation();
			} else if (this.nPlaying == NodeGraph.ANIMATION_PLAYING) {
				this.stopAnimation();
			} else if (this.nPlaying == NodeGraph.ANIMATION_STOPPED) {
				this.startTime = this.time;
				this.playAnimation();
			}
			break;
		case this.ellipselid:
			if (this.nPlaying == NodeGraph.ANIMATION_READY) {
				// do nothing
			} else if (this.nPlaying == NodeGraph.ANIMATION_PLAYING) {
				this.nPlaying = NodeGraph.ANIMATION_REWIND;
			} else if (this.nPlaying == NodeGraph.ANIMATION_STOPPED) {
				this.resetAnimation();
			}
			break;
		case this.ellipserid:
			if (this.nPlaying == NodeGraph.ANIMATION_READY) {
				this.startTime = this.data.Dynamic.numSteps - 1;
				this.playAnimation();
			} else if (this.nPlaying == NodeGraph.ANIMATION_PLAYING) {
				this.nPlaying = NodeGraph.ANIMATION_FASTFORWARD;
			} else if (this.nPlaying == NodeGraph.ANIMATION_STOPPED) {
				this.startTime = this.data.Dynamic.numSteps - 1;
				this.playAnimation();
			}
			break;
		default:
			break;
	}
}

NodeGraph.prototype.onDblClick = function(sid) {}

NodeGraph.prototype.addListeners = function() {
	this.addListenersForSelection("g"      )
	this.addListenersForSelection("svg"    )
	this.addListenersForSelection("rect"   )//svg
	this.addListenersForSelection("polygon")//svg
	this.addListenersForSelection("path"   )//svg
	this.addListenersForSelection("circle" )//svg
	this.addListenersForSelection("ellipse")//svg
	this.addListenersForSelection("text"   )//svg
	this.addListenersForSelection("input"  )//form
	this.addListenersForSelection("select" )//form
	this.addListenersForSelection("option" )//form
	this.addListenersForSelection("button" )//form
	if (this.TimeLine)
		this.TimeLine.addListeners();
}

NodeGraph.prototype.addListenersForSelection = function(sSelector) {
	var m_this = this;
	//var d3g = d3.select("#" + this.gid);
	var d3g = this.d3g;
	d3g.selectAll(sSelector)
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


