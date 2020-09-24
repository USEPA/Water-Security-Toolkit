/** 
 *  SVGPan library 1.2.1
 * ======================
 *
 * Given an unique existing element with id "viewport" (or when missing, the first g 
 * element), including the the library into any SVG adds the following capabilities:
 *
 *  - Mouse panning
 *  - Mouse zooming (using the wheel)
 *  - Object dragging
 *
 * You can configure the behaviour of the pan/zoom/drag with the variables
 * listed in the CONFIGURATION section of this file.
 *
 * Known issues:
 *
 *  - Zooming (while panning) on Safari has still some issues
 *
 * Releases:
 *
 * 1.2.1, Mon Jul  4 00:33:18 CEST 2011, Andrea Leofreddi
 *	- Fixed a regression with mouse wheel (now working on Firefox 5)
 *	- Working with viewBox attribute (#4)
 *	- Added "use strict;" and fixed resulting warnings (#5)
 *	- Added configuration variables, dragging is disabled by default (#3)
 *
 * 1.2, Sat Mar 20 08:42:50 GMT 2010, Zeng Xiaohui
 *	Fixed a bug with browser mouse handler interaction
 *
 * 1.1, Wed Feb  3 17:39:33 GMT 2010, Zeng Xiaohui
 *	Updated the zoom code to support the mouse wheel on Safari/Chrome
 *
 * 1.0, Andrea Leofreddi
 *	First release
 *
 **/

"use strict";

function SVGPan(svgRootId) {
	this.svgroot = document.getElementById(svgRootId);
	this.enablePan  = true;
	this.enableZoom = true;
	this.state = "none";
	this.stateOrigin = null;
	this.stateTf = null;
	this.setupHandlers();
}

SVGPan.prototype.setupHandlersForSelection = function(selection){
	var m_this = this;
	var sel = d3.select(selection);
	sel.on("mouseup"   , function() { m_this.onMouseUp   (d3.event); });
	sel.on("mousedown" , function() { m_this.onMouseDown (d3.event); });
	sel.on("mousemove" , function() { m_this.onMouseMove (d3.event); });
	sel.on("mousewheel", function() { m_this.onMouseWheel(d3.event); });
}

SVGPan.prototype.setupHandlers = function(){
	this.setupHandlersForSelection("#" + this.svgroot.id);
	if (this.svgroot.addEventListener)
		this.svgroot.addEventListener("DOMMouseScroll", SVGPan_handleMouseWheel, false);
//	this.svgroot.onmousewheel = SVGPan_handleMouseWheel;
}

SVGPan.prototype.setRoot = function(sid) {
	var obj = document.getElementById("#" + sid);
	this.root = obj;
}

SVGPan.prototype.getRoot = function() {
	if (this.root == null) {
		var gs = this.svgroot.getElementsByTagName("g")
		var g = gs[0]; // get the first g element under this svg root
		this.setCTM(g, g.getCTM());
		g.removeAttribute("viewBox");
		this.root = g;
	}
	return this.root;
}

SVGPan.prototype.getEventPoint = function(e) {
	var p = this.svgroot.createSVGPoint();
	p.x = e.clientX - this.getLeft();
	p.y = e.clientY - this.getTop();
	return p;
}

SVGPan.prototype.setCTM = function(element, matrix) {
	var s = this.dumpMatrix(matrix);
	element.setAttribute("transform", s);
}

SVGPan.prototype.dumpMatrix = function(matrix) {
	var s = "matrix(" + matrix.a + "," + matrix.b + "," + matrix.c + "," + matrix.d + "," + matrix.e + "," + matrix.f + ")";
	return s;
}

SVGPan.prototype.onMouseWheel = function(e) {
	if (!this.enableZoom) return;
	if (e.preventDefault) e.preventDefault();
	e.returnValue = false;
	//
	var delta;
	if (e.wheelDelta) {
		delta = e.wheelDelta / 3600; // Chrome/Safari
	}else{
		delta = e.detail / -90; // Mozilla
	}
	var z = 1 + delta; // Zoom factor: 0.9/1.1
	//
	var g = this.getRoot();
	var m = g.getCTM();
	var m = m.inverse();
	var p = this.getEventPoint(e);
	p = p.matrixTransform(m);
	//
	// Compute new scale matrix in current mouse position
	var m = this.svgroot.createSVGMatrix();
	var m = m.translate(p.x, p.y);
	var m = m.scale(z);
	var m = m.translate(-p.x, -p.y);
	this.setCTM(g, g.getCTM().multiply(m));
	if (this.stateTf == null)
		this.stateTf = g.getCTM().inverse();
	this.stateTf = this.stateTf.multiply(m.inverse());
	//
	if (false) {
		m_NetworkView.updateZoom(1/z);
		m_NetworkView.resizeNodes(null, null, 0);
	}
}

SVGPan.prototype.onMouseMove = function(e) {
//	console.log("SVGPan.onMouseMove = " + e.target.id + ", " + e.currentTarget.id);
	if (e.preventDefault) e.preventDefault();
	e.returnValue = false;
	//
	var g = this.getRoot();
	if (this.state == "pan" && this.enablePan) {
		var p = this.getEventPoint(e).matrixTransform(this.stateTf);
		var x = p.x - this.stateOrigin.x;
		var y = p.y - this.stateOrigin.y;
		var m = this.stateTf;
		var m = m.inverse();
		var m = m.translate(x,y);
		this.setCTM(g, m);
		//
		m_NetworkView.hideNodePopup();
	}
}

SVGPan.prototype.onMouseDown = function(e, bPreventDefault) {
	//console.log(e.currentTarget.id + ", " + e.target.id);
	//
	var bDebug = false;
	if (bDebug) {
		var parent = e.target;
		var indent = "";
		while (parent) {
			console.log(indent + parent.id);
			indent = indent + "   ";
			parent = parent.parentElement;
		}
	}
	//
	if (HasAncestorId(e.target, m_NetworkView.m_InversionGrid.gid)) return;
	if (HasAncestorId(e.target, m_NetworkView.m_InversionNavigator.gid)) return;
	if (HasAncestorId(e.target, m_NetworkView.NodeFilter.gid)) return;
	if (HasAncestorId(e.target, m_NetworkView.m_NodeGraph.gid)) return;
	if (HasAncestorId(e.target, m_NetworkView.m_ImpactView.gid)) return;
	//
	if (e.target.id == m_NetworkView.zoominid) return;
	if (e.target.id == m_NetworkView.zoomoutid) return;
	if (e.target.id == m_NetworkView.zoomfullid) return;
	if (e.target.id == m_NetworkView.zoomselectid) return;
	if (e.target.id == m_NetworkView.dotsizeid) return;
	if (e.target.id == m_NetworkView.cameraid) return;
	if (e.target.id == m_NetworkView.zoombgid) return;
	if (e.target.id == m_NetworkView.closeid) return;
	//	
	if (e.target.id == m_NetworkView.toggleJunctionId) return;
	if (e.target.id == m_NetworkView.toggleTankId) return;
	if (e.target.id == m_NetworkView.toggleReservoirId) return;
	if (e.target.id == m_NetworkView.toggleValveId) return;
	if (e.target.id == m_NetworkView.togglePumpId) return;
	if (e.target.id == m_NetworkView.togglePipeId) return;
	if (e.target.id == m_NetworkView.toggleSensorId) return;
	if (e.target.id == m_NetworkView.togglebgid) return;
	//
	if (e.target.id == m_NetworkView.WizardCompleteButton.sid) return;	
	if (e.target.id == m_NetworkView.ExportButton.sid) return;	
	if (e.target.id == m_NetworkView.GatherDataButton.sid) return;	
	//
	var sidPrefix = getPrefix2(e.target.id);
	var id        = getSuffix2(e.target.id);
	switch (sidPrefix) {
	}
	//
	if (bDebug) {
		console.log(indent + "    SVGPan_onMouseDown");
		console.log(indent + "        " + e.currentTarget.id);
	}
///////////////////////////////////////////////////////////////////////////////////////
	if (e.metaKey || e.ctrlKey || m_NetworkView.getZoomSelectionState().length > 0) return;
	if (bPreventDefault = null || bPreventDefault) {
		if (e.preventDefault) e.preventDefault();
		e.returnValue = false;
	}
	if (this.enablePan) {
		this.state = "pan";
		var g = this.getRoot();
		var m = g.getCTM();
		var m = m.inverse();
		this.stateTf = m;
		this.stateOrigin = this.getEventPoint(e).matrixTransform(this.stateTf);
	}
}

SVGPan.prototype.onMouseUp = function(e, bPreventDefault) {
//	console.log("SVGPan.onMouseUp");
	if (bPreventDefault = null || bPreventDefault) {
		if (e.preventDefault)
			e.preventDefault();
		e.returnValue = false;
	}
	if (this.state == "pan") this.state = "";
}

SVGPan.prototype.getLeft = function() {
	return this.svgroot.offsetLeft ? this.svgroot.offsetLeft : 0;
}

SVGPan.prototype.getTop = function() {
	return this.svgroot.offsetTop ? this.svgroot.offsetTop : 0;
}