// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

function LockButton(svgid, uniqueString, dataInFrontOf, left, top, width) {
	this.left = left;
	this.top = top;
	this.width = width;
	this.pathFill = d3.rgb(50,50,50);
	this.pathOpacity = 1;
	this.bgFill = d3.rgb(250,250,250);
	this.bgOpacity = 0;
	this.coverFill = d3.rgb(0,0,0);
	this.coverOpacity = 0.3;
	this.coverCursor = "pointer"
	this.pathStroke = d3.rgb(0,0,0);
	this.pathStrokeOpacity = 1;
	this.pathStrokeWidth = 1.5 / 15 * this.width;
	this.pathid = "pathLockButton" + uniqueString;
	this.path1id = "path1LockButton" + uniqueString;
	this.coverid = "coverLockButton" + uniqueString;
	this.base = Control;
	this.base(svgid, uniqueString, dataInFrontOf);
	//
	var m_lock = false;
	//
	function setLock_private(val) {
		m_lock = val;
	}
	function getLock_private() {
		return m_lock;
	}
	//
	this.getLock = function() {
		return getLock_private();
	}
	this.setLock = function(val) {
		this.Lock(val);
	}
	//
	this.Lock = function(val) {
		if (val == null) val = true;
		setLock_private(val);
		if (val)
			var path = this.path1;
		else
			var path = this.path2;
		this.d3path2.attr("d", path);
	}
	this.Unlock = function(val) {
		if (val == null) val = true
		this.Lock(!val);
	}
	//
	this.ToggleLock = function() {
		var val = this.getLock();
		this.Lock(!val);
		return !val;
	}
	//
	var m_disabled = false;
	//
	function setDisabled_private(val) {
		if (val == null) val = true;
		m_disabled = val;
	}
	function getDisabled_private(val) {
		return m_disabled;
	}
	//
	this.setDisabled = function(val) {
		this.disable(val);
	}
	this.getDisabled = function() {
		return getDisabled_private();
	}
	//
	this.disable = function(val) {
		if (val == null) val == true;
		setDisabled_private(val);
		if (val == true) {
			this.d3cover.attr("fill-opacity", 0.0)
			this.d3cover.attr("cursor", "default")
			this.d3path.attr("fill", d3.rgb(100,100,100))
			this.d3path2.attr("stroke", d3.rgb(100,100,100))
		} else {
			this.d3cover.attr("fill-opacity", this.coverOpacity)
			this.d3cover.attr("cursor", this.coverCursor)
			this.d3path.attr("fill", d3.rgb(0,0,0))
			this.d3path2.attr("stroke", d3.rgb(0,0,0))
		}

	}
	this.enable = function(val) {
		if (val == null) val == true;
		this.disable(!val);
	}
}

LockButton.prototype = new Control;

LockButton.prototype.create = function() {
	this.path = ""
		+ " M " + (this.left + 4 / 15 * this.width) + " " + (this.top + 6 / 15 * this.width)
		+ " h " + (this.width - 8 / 15 * this.width)
		+ " v " + (this.width - 9 / 15 * this.width)
		+ " h " + (-this.width + 8 / 15 * this.width)
		+ " v " + (-this.width + 9 / 15 * this.width)
		;
	var r = (this.width - 12 / 15 * this.width) / 2
	this.path1 = ""
		+ " M " + (this.left + 6 / 15 * this.width) + " " + (this.top + 6 / 15 * this.width)
		+ " v " + (-2 / 15 * this.width) 
		+ " a " + (r) + " " + (r) + " 180 1 1 " + (2 * r) + " 0 "
		+ " v " + (+2 / 15 * this.width) 
		;
	this.path2 = ""
		+ " M " + (this.left + 6 / 15 * this.width) + " " + (this.top + 6 / 15 * this.width)
		+ " v " + (-2 / 15 * this.width) 
		+ " a " + (r) + " " + (r) + " 180 1 1 " + (2 * r) + " 0 "
		;
	//
	this.d3g = d3.select ("#" + this.sParentId)
		.append("g")
		.attr("id", this.gid)
		;
	this.d3bg = d3.select("#" + this.gid)
		.append("circle")
		.attr("id", this.bgid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("cx", this.left + this.width / 2)
		.attr("cy", this.top + this.width / 2)
		.attr("r", this.width / 2)
		.attr("stroke", "none")
		.attr("fill", this.bgFill)
		.attr("fill-opacity", this.bgOpacity)
		;
	this.d3path = d3.select("#" + this.gid)
		.append("path")
		.attr("id", this.pathid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("stroke", "none")
		.attr("fill", this.pathFill)
		.attr("fill-opacity", this.pathOpacity)
		.attr("d", this.path)
		;
	this.d3path2 = d3.select("#" + this.gid)
		.append("path")
		.attr("id", this.path1id)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("stroke", this.pathStroke)
		.attr("stroke-opacity", this.pathStrokeOpacity)
		.attr("stroke-width", this.pathStrokeWidth)
		.attr("fill", "none")
		.attr("d", this.path2)
		;
	this.d3cover = d3.select("#" + this.gid)
		.append("circle")
		.attr("id", this.coverid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		.attr("cursor", this.coverCursor)
		.attr("cx", this.left + this.width / 2)
		.attr("cy", this.top + this.width / 2)
		.attr("r", this.width / 2)
		.attr("stroke", "none")
		.attr("fill", d3.rgb(0,0,0))
		.attr("fill-opacity", this.coverOpacity)
		;
	this.addListeners();
}

LockButton.prototype.onClick = function(sid) {
	if (this.getDisabled()) return;
	switch(sid) {
		case this.coverid:
			var bLocked = this.ToggleLock();
			this.raiseEvent({"source": this, "event": "click", "locked": bLocked})
			break;
	}
}

LockButton.prototype.onMouseOver = function(sid) {
	if (this.getDisabled()) return;
	switch(sid) {
		case this.coverid:
			this.d3bg.attr("fill-opacity", 0.9)
			this.d3cover.attr("fill-opacity", 0)
			//this.d3cover.attr("stroke", d3.rgb(0,0,0))
			break;
	}
}

LockButton.prototype.onMouseOut = function(sid) {
	if (this.getDisabled()) return;
	switch(sid) {
		case this.coverid:
			this.d3bg.attr("fill-opacity", this.bgOpacity)
			this.d3cover.attr("fill-opacity", this.coverOpacity)
			this.d3cover.attr("stroke", "none")
			break;
	}
}

