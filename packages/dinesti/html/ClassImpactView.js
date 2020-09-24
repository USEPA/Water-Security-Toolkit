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
function ImpactView(parent, left, top) {
	this.parent = parent;
	this.nid = parent.nid;
	var uniqueString = "ImpactView_" + this.nid;
	this.uniqueString = uniqueString;
	this.sParentId = parent.gid;
	this.gid = "g" + this.uniqueString;
	this.svgid = "svg" + this.uniqueString;
	this.rectid = "rect" + this.uniqueString;
	this.scaleid = "scale" + this.uniqueString;
	this.g1id = "g1" + this.uniqueString;
	this.highid   = "ImpactScaleValueHighCover";
	this.mediumid = "ImpactScaleValueMediumCover";
	this.lowid    = "ImpactScaleValueLowCover";
	//
	this.left = left;
	this.top = top;
	this.width = 160 + 50;
	this.height = 160;
	//
	GlobalData.resizeHooks[uniqueString] = {"this": this, "function": this.resizeWindow};
	//
	this.create();
}

//
// Public Member Methods
//
ImpactView.prototype.create = function() {
	this.d3g =
	d3.select("#" + this.sParentId).append("g").attr("id", this.gid)
		.style("position","absolute")
		.style("top", convertToPx(this.top))
		.style("left", convertToPx(this.left))
		;
	this.d3svg =
	d3.select("#" + this.gid).append("svg").attr("id", this.svgid)
		.style("top", convertToPx(0))
		.style("left", convertToPx(0))
		.style("width", convertToPx(this.width))
		.style("height", convertToPx(this.height))
		;
	this.d3rect = 
	d3.select("#" + this.svgid).append("rect").attr("id", this.rectid)
		.attr("x", 0)
		.attr("y", 0)
		.attr("width", this.width - 50)
		.attr("height", this.height)
		.attr("fill", "white")
		.attr("fill-opacity", 0.6)
		;
	//
	this.legendGradient = new Gradient("linearGradient_ImpactLegend", this.svgid, 0, 0, 0, 100);
	this.legendGradient.defaultOpacity = 1.0;
	this.legendGradient.addStop(  0, "red"   );
	this.legendGradient.addStop( 50, "yellow");
	this.legendGradient.addStop(100, "green" );
	this.legendGradient.create();
	//
	this.d3scale = 
	d3.select("#" + this.svgid).append("rect").attr("id", this.scaleid)
		.attr("x", this.width - 50)
		.attr("y", 0)
		.attr("width", 50)
		.attr("height", this.height)
		.attr("fill", this.legendGradient.fill)
		.attr("fill-opacity", 0.6)
		;
	//
	this.label1Text = "Reachability:";
	this.label2Text = "Max count: ";
	this.label3Text = "Max count: ";
	this.label4Text = "% detected: ";
	//
	this.label1 = new Label(this.label1Text, this.svgid, "labelImpactViewTitle",             null, 10,   0+25, 100, 20);
	this.label2 = new Label(this.label2Text, this.svgid, "labelImpactViewMaxAssetCount",     null, 19,  40+25, 100, 20);
	this.label3 = new Label(this.label3Text, this.svgid, "labelImpactViewMaxInjectionCount", null, 19,  80+25, 100, 20);
	this.label4 = new Label(this.label4Text, this.svgid, "labelImpactViewPercent",           null, 19, 120+25, 100, 20);
	//
	this.b1 = new SvgButton("From Upstream"     , this, "ImpactButtonUpstream"  , null, 10,  0+25+7, 140, 20, 10);
	this.b2 = new SvgButton("To Downstream"     , this, "ImpactButtonDownstream", null, 10, 40+25+7, 140, 20, 10);
	this.b3 = new SvgButton("Sensor Evaluation" , this, "ImpactButtonSensorEval", null, 10, 80+25+7, 140, 20, 10);
	//
	this.value_y_max = 15;
	this.value_y_mid = this.height / 2 + 5;
	this.value_y_min = this.height - 5;
	//
	this.valueSelected =
	this.d3svg.append("text").attr("id", "ImpactViewScaleValueSelected")
		.attr("x", this.width - 25)
		.attr("y", this.value_y_mid)
		.attr("text-anchor", "middle")
		.text("")
		;
	//
	this.valueHigh =
	this.d3svg.append("text").attr("id", "ImpactViewScaleValueHigh")
		.attr("x", this.width - 25)
		.attr("y", this.value_y_max)
		.attr("text-anchor", "middle")
		.text("")
		;
	//
	this.valueMedium =
	this.d3svg.append("text").attr("id", "ImpactViewScaleValueMedium")
		.attr("x", this.width - 25)
		.attr("y", this.value_y_mid)
		.attr("text-anchor", "middle")
		.text("")
		;
	//
	this.valueLow =
	this.d3svg.append("text").attr("id", "ImpactViewScaleValueLow")
		.attr("x", this.width - 25)
		.attr("y", this.value_y_min)
		.attr("text-anchor", "middle")
		.text("")
		;
	//
	this.coverHigh = 
	this.d3svg.append("rect").attr("id", this.highid)
		.attr("x", this.width - 50)
		.attr("y", 0)
		.attr("width", 50)
		.attr("height", 20)
		.attr("fill", "black")
		.attr("fill-opacity", 0.0)
		;
	//
	this.coverMedium = 
	this.d3svg.append("rect").attr("id", this.mediumid)
		.attr("x", this.width - 50)
		.attr("y", this.height / 2 - 10)
		.attr("width", 50)
		.attr("height", 20)
		.attr("fill", "black")
		.attr("fill-opacity", 0.0)
		;
	//
	this.coverLow = 
	this.d3svg.append("rect").attr("id", this.lowid)
		.attr("x", this.width - 50)
		.attr("y", this.height - 20)
		.attr("width", 50)
		.attr("height", 20)
		.attr("fill", "black")
		.attr("fill-opacity", 0.0)
		;
	//
	var p1 = {"x": this.width - 50, "y": 0                       , "w": 44, "h":20};
	var p2 = {"x": this.width - 50, "y": this.height / 2 - 10 - 3, "w": 44, "h":20};
	var p3 = {"x": this.width - 50, "y": this.height / 1 - 20 - 6, "w": 44, "h":20};
	this.inputHigh   = new Textbox(this.gid, "ImpactViewInputHigh"   , null, p1.x, p1.y, p1.w, p1.h);
	this.inputMedium = new Textbox(this.gid, "ImpactViewInputHMedium", null, p2.x, p2.y, p2.w, p2.h);
	this.inputLow    = new Textbox(this.gid, "ImpactViewInputLow"    , null, p3.x, p3.y, p3.w, p3.h);
	//
	this.inputHigh  .hide();
	this.inputMedium.hide();
	this.inputLow   .hide();
	//
	this.draw();
}

ImpactView.prototype.draw = function() {
	var m_this = this;
	this.show();
	var sel = d3.select("." + this.uniqueString + "_removeable");
	if (sel) sel.remove();
	d3.select("#" + this.svgid)
		.append("g")
		.attr("id",this.g1id)
		.attr("class",this.uniqueString + "_removeable")
		.attr("transform", "translate(0,"+this.height+")")
		;
	//
	d3.select("#" + this.g1id)
		.append("rect")
		.attr("id",this.uniqueString + "_LegendRect")
		.attr("x",0)
		.attr("y",0)
		.attr("width",10)
		.attr("height",100)
		.attr("fill","url(#gradientLegend)")
		//.attr("fill","url(#" + this.legendGradient.uniqueString + ")")
		;
	//
	this.addListeners();
}

ImpactView.prototype.update = function(data) {
	this.data = data;
	this.label2.setText(this.label2Text +           this.data.Impact.Assets    .maxCount + " / " + this.data.Impact.Injections.values.length );
	this.label3.setText(this.label3Text +           this.data.Impact.Injections.maxCount + " / " + this.data.Impact.Assets.values.length     );
	this.label4.setText(this.label4Text + SigFig(3, this.data.Impact.PercentDetection    ));
	//
	this.b1.toggle(false);
	this.b2.toggle(false);
	this.b3.toggle(false);
	//
	var sel = m_NetworkView.getJunctions();
	sel.attr("fill",function(d,i) {
		return d3.rgb(0,0,0);
	});
	sel.attr("fill-opacity",function(d,i) {
		return m_NetworkView.dOpacity;
	});
	sel.attr("display",function(d,i) {
		return "inherit";
	});
	var sel = m_NetworkView.getReservoirs();
	sel.attr("fill",function(d,i) {
		return d3.rgb(0,150,0);
	});
	sel.attr("fill-opacity",function(d,i) {
		return m_NetworkView.dOpacity;
	});
	sel.attr("display",function(d,i) {
		return "inherit";
	});
	var sel = m_NetworkView.getTanks();
	sel.attr("fill",function(d,i) {
		return d3.rgb(0,0,150);
	});
	sel.attr("fill-opacity",function(d,i) {
		return m_NetworkView.dOpacity;
	});
	sel.attr("display",function(d,i) {
		return "inherit";
	});
	//
	this.label1.update2();
	this.label2.update2();
	this.label3.update2();
	this.label4.update2();
	//
	this.valueSelected.text("");
	this.valueHigh    .text("");
	this.valueMedium  .text("");
	this.valueLow     .text("");
	//
	this.hideValueCovers();
}

ImpactView.prototype.redrawImpact = function() {
	var b1 = this.b1.getToggle();
	var b2 = this.b2.getToggle();
	var b3 = this.b3.getToggle();
	if        (b1) {
		this.onClick(this.b1.sid);
	} else if (b2) {
		this.onClick(this.b2.sid);
	} else if (b3) {
		this.onClick(this.b3.sid);
	}
}

ImpactView.prototype.getUuid = function() {
	return this.parent.getSelection();
}

ImpactView.prototype.hide = function() {
	d3.select("#" + this.gid).style("visibility","hidden");
}

ImpactView.prototype.show = function() {
	d3.select("#" + this.gid).style("visibility","inherit");
}

ImpactView.prototype.isHidden = function() {
	return d3.select("#" + this.gid).style("visibility") == "hidden";
}

ImpactView.prototype.isVisible = function() {
	return !this.isHidden();
}

ImpactView.prototype.resizeWindow = function(w, h) {
	this.top = h - this.height - NETWORK_VIEW_BORDER - 10;
	this.d3g.style("top", convertToPx(this.top));
	//m_NetworkView.d3title.
}

ImpactView.prototype.displayNodeValue = function(nodeId) {
	if (nodeId == null) return;
	if (this.ids == null) return
	if (this.ids[nodeId] == null) return;
	if (this.values == null) return;
	if (this.values[this.ids[nodeId]] == null) return;
	var b1 = this.b1.getToggle();
	var b2 = this.b2.getToggle();
	var b3 = this.b3.getToggle();
	//
	var min = this.valueMin;
	var max = this.valueMax;
	var ymin = this.value_y_min;
	var ymax = this.value_y_max;
	if (this.valueMax < this.valueMin) {
		min = this.valueMax;
		max = this.valueMin;
		ymin = this.value_y_max;
		ymax = this.value_y_min;
	}
	if (b1 || b2) {
		var value = this.values[this.ids[nodeId]].count;
	} else if (b3) {
		var value = this.values[this.ids[nodeId]] / this.maxInjections * 100;
	}
	var YLOC = d3.interpolateNumber(ymin, ymax);
	var y = YLOC((value - min) / (max - min));
	y = (value < min) ? ymin : y;
	y = (value > max) ? ymax : y;
	//
	if (b3) {
		value = value.toFixed(1) + "%";
	}
	//
	this.hideValueCovers();
	//
	this.valueSelected.attr("y", y)
	this.valueSelected.text(value);
	this.valueHigh    .text("");
	this.valueMedium  .text("");
	this.valueLow     .text("");
}

ImpactView.prototype.showScaleValues = function(max, mid, min) {
	if (this.inputHigh  .isVisible()) return;
	if (this.inputMedium.isVisible()) return;
	if (this.inputLow   .isVisible()) return;
	if (max == null) max = this.valueMax;
	if (mid == null) mid = this.valueMid;
	if (min == null) min = this.valueMin;
	this.valueMax = max;
	this.valueMid = mid;
	this.valueMin = min;
	//
	this.showValueCovers();
	//
	var b1 = this.b1.getToggle();
	var b2 = this.b2.getToggle();
	var b3 = this.b3.getToggle();
	//
	if (b1 || b2) {	
		this.valueHigh  .text(max);
		this.valueMedium.text(mid);
		this.valueLow   .text(min);
		this.inputHigh  .setValue(max);
		this.inputMedium.setValue(mid);
		this.inputLow   .setValue(min);
	} else if (b3) {
		this.valueHigh   .text(max.toFixed(1) + "%");
		this.valueMedium .text(mid.toFixed(1) + "%");
		this.valueLow    .text(min.toFixed(1) + "%");
		this.inputHigh   .setValue(max);
		this.inputMedium .setValue(mid);
		this.inputLow    .setValue(min);
	} else {
		return;
	}
	//
	this.valueSelected.text("");
}

ImpactView.prototype.hideValueCovers = function() {
	this.coverHigh  .attr("cursor", null);
//	this.coverMedium.attr("cursor", null);
	this.coverLow   .attr("cursor", null);
}

ImpactView.prototype.showValueCovers = function() {
	this.coverHigh  .attr("cursor", "pointer");
//	this.coverMedium.attr("cursor", "pointer");
	this.coverLow   .attr("cursor", "pointer");
}

ImpactView.prototype.toggle = function(b1, b2, b3) {
	this.b1.toggle(b1);
	this.b2.toggle(b2);
	this.b3.toggle(b3);
}

ImpactView.prototype.visualize = function(max, min) {
	var m_this = this;
	var getcolor1 = d3.interpolateRgb("yellow", "red");
	var getcolor2 = d3.interpolateRgb("green", "yellow");
	var values = this.values;
	var ids = this.ids;
	var sel = this.parent.getNodes();
	sel.attr("fill", function(d,i) {
		if (d == null || d.id == null || ids[d.id] == null || values[ids[d.id]] == null) return "black";
		var count = values[ids[d.id]].count;
		//count = (count == null) ? values[ids[d.id]] : count; // for sensor evaluation
		if (count == null) {
			count = values[ids[d.id]] / m_this.maxInjections * 100;
		}
		if (count == 0) return "black";
		var ratio = (count - min) / (max - min);
		ratio = (ratio < 0) ? 0 : ratio;
		ratio = (ratio > 1) ? 1 : ratio;
		return (ratio > 0.5) ? getcolor1(2*(ratio-0.5)) : getcolor2(2*ratio);
	});
	sel.attr("fill-opacity", function(d,i) {
		if (d == null || d.id == null || ids[d.id] == null || values[ids[d.id]] == null) return "0";
		var count = values[ids[d.id]].count;
		count = (count == null) ? values[ids[d.id]] : count; // for sensor evaluation
		if (count == 0) return "0";
		return "1";
	});
	sel.attr("display", function(d,i) {
		if (d == null || d.id == null || ids[d.id] == null || values[ids[d.id]] == null) return "none";
		var count = values[ids[d.id]].count;
		count = (count == null) ? values[ids[d.id]] : count; // for sensor evaluation
		if (count == 0) return "none";
		return "inherit";
	});
}
///////////////////////////////////////////////////////

ImpactView.prototype.onMouseMove = function(sid) {
}
ImpactView.prototype.onMouseOver = function(sid) {
	this.d3rect.transition().duration(20).attr("fill-opacity", 1);
	this.d3scale.transition().duration(20).attr("fill-opacity", 1);
	this.showValueCovers();
	this.showScaleValues();
	switch(sid) {
		case this.highid:
			this.valueHigh.attr("fill", "white");
			break;
		case this.mediumid:
//			this.valueMedium.attr("fill", "white");
			break;
		case this.lowid:
			this.valueLow.attr("fill", "white");
			break;
	}
}
ImpactView.prototype.onMouseOut = function(sid) {
	this.d3rect.transition().duration(3000).attr("fill-opacity", 0.6);
	this.d3scale.transition().duration(3000).attr("fill-opacity", 0.6);
	this.onMouseUp(sid);
	switch(sid) {
		case this.highid:
			this.valueHigh.attr("fill", "black");
			break;
		case this.mediumid:
//			this.valueMedium.attr("fill", "black");
			break;
		case this.lowid:
			this.valueLow.attr("fill", "black");
			break;
	}
}
ImpactView.prototype.onMouseUp = function(sid) {
	switch(sid) {
		case this.highid:
			this.coverHigh.attr("fill-opacity", 0);
			this.valueHigh.attr("fill", "black");
			break;
		case this.mediumid:
//			this.coverMedium.attr("fill-opacity", 0);
//			this.valueMedium.attr("fill", "black");
			break;
		case this.lowid:
			this.coverLow.attr("fill-opacity", 0);
			this.valueLow.attr("fill", "black");
			break;
	}
}
ImpactView.prototype.onMouseDown = function(sid) {
	switch(sid) {
		case this.highid:
			this.coverHigh.attr("fill-opacity", 0.5);
			this.valueHigh.attr("fill", "white");
			break;
		case this.mediumid:
//			this.coverMedium.attr("fill-opacity", 0.5);
//			this.valueMedium.attr("fill", "white");
			break;
		case this.lowid:
			this.coverLow.attr("fill-opacity", 0.5);
			this.valueLow.attr("fill", "white");
			break;
	}
}
//ImpactView.prototype.onMouseWheel = function(sid) {
//}
ImpactView.prototype.onKeyDown = function(sid, keyCode) {
	if (keyCode == null) keyCode = window.event.keyCode;
	switch (sid) {
		case this.inputHigh.sid:
			if (keyCode == 27) { // escape
				this.inputHigh.setValue(this.valueMax);
				this.inputHigh.hide();
				return;
			}
			if (keyCode == 13) {
				var value = this.inputHigh.getFloat();
				this.valueMax = value;
				this.valueMid = (this.valueMax - this.valueMin) / 2 + this.valueMin;
				this.inputHigh.hide();
				this.showScaleValues();
				this.inputHigh.setValue(value);
				this.visualize(this.valueMax, this.valueMin);
				return;
			}
			return;
//		case this.inputMedium.sid:
//			if (keyCode == 27) { // escape
//				this.inputMedium.setValue(this.valueMid);
//				this.inputMedium.hide();
//				return;
//			}
//			if (keyCode == 13) {
//				var value = this.inputMedium.getFloat();
//				this.valueMid = value;
//				this.inputMedium.hide();
//				this.showScaleValues();
//				this.inputMedium.setValue(value);
//				return;
//			}
//			return;
		case this.inputLow.sid:
			if (keyCode == 27) { // escape
				this.inputLow.setValue(this.valueMin);
				this.inputLow.hide();
				return;
			}
			if (keyCode == 13) {
				var value = this.inputLow.getFloat();
				this.valueMin = value;
				this.valueMid = (this.valueMax - this.valueMin) / 2 + this.valueMin;
				this.inputLow.hide();
				this.showScaleValues();
				this.inputLow.setValue(value);
				this.visualize(this.valueMax, this.valueMin);
				return;
			}
			return;
	}
}
ImpactView.prototype.onKeyPress = function(sid) {
}
ImpactView.prototype.onKeyUp = function(sid) {
}
ImpactView.prototype.onInput = function(sid) {
}
ImpactView.prototype.onChange = function(sid) {
}

ImpactView.prototype.onClick = function(sid) {
	var m_this = this;
	var sel = d3.select("#" + sid);
	switch(sid) {
		case this.highid:
			this.inputHigh.show();
			this.inputHigh.focus();
			this.inputMedium.hide();
			this.inputLow.hide();
			break;
		case this.mediumid:
//			this.inputHigh.hide();
//			this.inputMedium.show();
//			this.inputMedium.focus();
//			this.inputLow.hide();
			break;
		case this.lowid:
			this.inputHigh.hide();
			this.inputMedium.hide();
			this.inputLow.show();
			this.inputLow.focus();
			break;
		case this.b1.sid:
			this.toggle(true, false, false);
			this.inputHigh.hide();
			this.inputMedium.hide();
			this.inputLow.hide();
			m_NetworkView.d3title.text("Impact View: Reachability from Upstream");
			m_NetworkView.d3title.attr("display", "inherit");
			var uuid = this.getUuid();
			Couch.getDoc(this, uuid, function(data) {
				var ids = data.Impact.NodeIds;
				var values = data.Impact.Assets.values;
				this.values = values;
				this.ids = ids;
				var max = data.Impact.Assets.maxCount;
				this.showScaleValues(max, max / 2, 0);
				this.visualize(max, 0);
			});
			break;
		case this.b2.sid:
			this.toggle(false, true, false);
			this.inputHigh.hide();
			this.inputMedium.hide();
			this.inputLow.hide();
			m_NetworkView.d3title.text("Impact View: Reachability to Downstream");
			m_NetworkView.d3title.attr("display", "inherit");
			var uuid = this.getUuid();
			this.zero_icount = 0;
			Couch.getDoc(this, uuid, function(data) {
				var ids = data.Impact.SimIds;
				var values = data.Impact.Injections.values;
				this.values = values;
				this.ids = ids;
				var max = data.Impact.Injections.maxCount;
				this.showScaleValues(max, max / 2, 0);
				this.visualize(max, 0);
			});
			break;
		case this.b3.sid:
			this.toggle(false, false, true);
			this.inputHigh.hide();
			this.inputMedium.hide();
			this.inputLow.hide();
			m_NetworkView.d3title.text("Impact View: Sensor Evaluation");
			m_NetworkView.d3title.attr("display", "inherit");
			var uuid = this.getUuid();
			Couch.getDoc(this, uuid, function(data) {
				var ids = data.Impact.SensorIds;
				var values = data.Impact.Sensors.values;
				this.values = values;
				this.ids = ids;
				var max = data.Impact.Sensors.maxInjections;
				var max = data.Impact.PercentDetection / 100 * data.Impact.Sensors.maxInjections;
				var max = data.Impact.PercentDetection;
				this.maxInjections = data.Impact.Sensors.maxInjections;
				this.maxDetected = max;
				this.showScaleValues(data.Impact.PercentDetection, data.Impact.PercentDetection / 2, 0);
				var sel = m_this.parent.getNodes();
				this.visualize(max, 0);
			});
			break;
		default:
			break;
	}
}

ImpactView.prototype.onDblClick = function(sid) {
}

ImpactView.prototype.onBlur = function(sid) {
	switch (sid) {
		case this.inputHigh.sid:
			this.onKeyDown(sid, 27)
			return;
		case this.inputMedium.sid:
			return;
		case this.inputLow.sid:
			return;
	}
}

ImpactView.prototype.addListeners = function() {
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
	//
	if (this.b1)
		this.b1.addListeners();
	if (this.b2)
		this.b2.addListeners();
	if (this.b3)
		this.b3.addListeners();
}

ImpactView.prototype.addListenersForSelection = function(sSelector) {
	var m_this = this;
	var d3g = d3.select("#" + this.gid);
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
		.on("blur"      , function() { m_this.onBlur      (this.id); })
		;
}


