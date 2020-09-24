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
function FilterList(parent, uniqueString, dataInFrontOf, left, top, width, bFirstBlank, nSize, bMultiple) {
	this.parent = parent;
	this.sParentId = parent.gid;
	this.nid = parent.nid;
	this.uniqueString = uniqueString;
	if (parent.nid == null) {
		this.nid = null;
		this.gid = "g" + uniqueString;
		this.sid = uniqueString	;
		this.filterid = "Filter";
		this.listid = "List";
	} else {
		this.nid = parent.nid;
		this.gid = "g" + uniqueString + "_" + this.nid;
		this.sid = uniqueString + "_" + this.nid;
		this.filterid = "Filter" + "_" + this.nid;
		this.listid = "List" + "_" + this.nid;
	}
	this.menuid = "pathMenu3";
	this.dataInFrontOf = dataInFrontOf;
	this.left = left;
	this.top = top;
	this.width = width;
	//this.height = height;
	this.bFirstBlank = bFirstBlank;
	if (bFirstBlank == null)
		this.bFirstBlank = false;
	this.bMultiple = bMultiple;
	if (bMultiple == null)
		this.bMultiple = false;
	this.nSize = nSize;
	this.HEIGHT_DEFAULT = 156+7;
	this.HEIGHT_SENSOR  = 23;
	this.sZoomNode = "";
	//
	this.create();
}

FilterList.prototype.create = function(sText) {
	var m_this = this;
	this.d3g = d3.select("#" + this.sParentId)
		.append("g")
		.attr("id",this.gid)
		.style("position","absolute")
		.style("left", convertToPx(this.left))
		.style("top", convertToPx(this.top))
		;
	this.Filter = new Textbox (this.gid, this.filterid, "", 0,  0, 0);
	this.Filter.parent = this.parent;
	this.List   = new Dropdown(this.gid, this.listid  , "", 0, 20, 0, this.bFirstBlank, this.nSize, this.bMultiple);
	this.List.parent = this.parent;
	this.List.setAttributes = function(sel) {
		sel.attr("data-value", function(d,i) {
			return i;
		});
		sel.attr("data-x",     function(d,i) {
			if (d.Type == "Pump" || d.Type == "Valve" || d.Type == "Pipe") {
				return (parseFloat(d.x1) + parseFloat(d.x2)) / 2;
			}
			return d.x;
		});
		sel.attr("data-y",     function(d,i) {
			if (d.Type == "Pump" || d.Type == "Valve" || d.Type == "Pipe") {
				return (parseFloat(d.y1) + parseFloat(d.y2)) / 2;
			}
			return d.y;
		});
		sel.attr("data-type", function(d,i) {
			return d.Type;
		})
		sel.attr("data-id", function(d,i) {
			switch (d.Type) {
				case "Junction":
				case "Tank":
				case "Reservoir":
					return "Node_" + d.id;
					break;
				default:
					return d.Type + "_" + d.ID;
			}
		})
		sel.text(function(d,i) {
			if (d.id == null) return d.ID;
			return d.id;
		});
	}
	this.d3svgmenu = this.d3g
	.append("svg")
	.attr("id","svgMenu" + "_" + this.parent.nid)
	.style("position","absolute")
	.style("left",   convertToPx( 3))
	.style("top",    convertToPx(90))
	.style("width",  convertToPx( 1))  // arbitrary
	.style("height", convertToPx( 1)) // arbitrary
	;
/*	this.gradientOn = new Gradient("FilterList_menuButtonOn", this.d3svgmenu.attr("id"), 0, 0, 0, 100);
	this.gradientOn.defaultOpacity = 0.3;
	this.gradientOn.addStop( 0, "rgb(220,220,220)");
	this.gradientOn.addStop(40, "rgb(180,180,180)");
	this.gradientOn.addStop(60, "rgb(090,090,090)");
	this.gradientOn.addStop(99, "rgb(060,060,060)");
	this.gradientOn.create();

	this.d3menu1 = this.d3svgmenu
	.append("path")
	.attr("id","pathMenu")
	.attr("fill", "white")
	.attr("stroke", "none")
	.attr("d",this.getMenuPath(0, 0))
	;
	this.d3menu2 = this.d3svgmenu
	.append("path")
	.attr("id","pathMenu2")
	.attr("fill", "none")
	.attr("stroke", "black")
	.attr("d",this.getMenuPath2(0))
	;*/
	this.addListeners();
}

FilterList.prototype.setSelectElement = function(val, data) {
	this.List.replaceList(val, data);
}

FilterList.prototype.getSelectElement = function() {
	return this.List.getObject();
}

FilterList.prototype.updateHide = function(arrays, fNextStep) {
	var m_this = this;
	var process = setTimeout(function() {
		var array = [];
		for (var i = 0; i < arrays.length; i++) {
			array = array.concat(arrays[i]);
		}
		m_this.List.forEach(function(item) {
			for (var i = 0; i < array.length; i++) {
				var text = item.text;
				var match = array[i].id;
				if (text == match) {
					d3.select(item).style("visibility","hidden");
				}
			}
		});
	});
}

FilterList.prototype.createList = function(arrays, fNextStep) {
	this.update(arrays, true, fNextStep);
}

FilterList.prototype.update = function(arrays, bCreate, fNextStep) {
	var m_this = this;
	var process = setTimeout(function() {
		var array = [];
		for (var i = 0; i < arrays.length; i++) {
			array = array.concat(arrays[i]);
		}
		//
		array.sort(function(a,b) {
			var v1 = a.id ? a.id : a.ID;
			var v2 = b.id ? b.id : b.ID;
			if (v1 == v2) {
				if (a.Type == "Junction" ) return -1;
				if (a.Type == "Tank"     ) return -1;
				if (a.Type == "Reservoir") return -1;
				if (b.Type == "Junction" ) return +1;
				if (b.Type == "Tank"     ) return +1;
				if (b.Type == "Reservoir") return +1;
			} else if (parseInt(v1) == v1 && parseInt(v2) == v2) {
				return v1 - v2;
			} else {
				if (v1 > v2) return +1;
				if (v2 > v1) return -1;
			}
			return 0;
		});
		//
		d3.select("#" + m_this.List.sid).style("width","");
		if (bCreate) {
			m_this.List.createList(array, function() {
				m_this.resizeList(array, fNextStep);
			});
		} else {
			m_this.List.updateList2(array, function() {
				m_this.resizeList(array, fNextStep);
			});
		}
		m_this.disable(false);
	}, 100);
}

FilterList.prototype.resizeList = function(array, fNextStep) {
	var m_this = this;
	m_this.List.setSize(Math.min(m_this.nSize, array.length));
	m_this.List.setSize(Math.min(m_this.nSize));
	m_this.setRight(m_this.right);
	var w = m_this.List.getWidth();
	var min = m_this.getMinWidth();
	if (w < min) {
		w = min;
		m_this.List.setWidth(w);
	}
	m_this.Filter.setWidth(w-6);
	var h = m_this.List.getHeight() + m_this.Filter.getHeight();
	m_this.d3svgmenu.style("top", convertToPx(h-5));
	var h = parseFloat(m_this.d3svgmenu.style("height"));
	if (h < 9+7) h = 9+7;
	m_this.updateMenu(w, h);
	if (fNextStep) fNextStep();
}

FilterList.prototype.updateMenu = function(w, h) {
	this.d3svgmenu.style("height", h);
	this.d3svgmenu.style("width", w-3);
/*	this.d3menu1.attr("d", this.getMenuPath(w-6, h-7));
	this.d3menu2.attr("d", this.getMenuPath2(w-6));*/
}

FilterList.prototype.ZoomToSelection = function() {
	var newNode = this.List.getSelectedText();
	if (newNode == this.sZoomNode) return;
	if (this.List.hasSelection()) {
		this.sZoomNode = newNode;
		var obj = {};
		obj.list = this.List.getObject();
		obj.text = this.List.getSelectedText();
		obj.type = this.List.getSelectedAttr("data-type");
		obj.id   = this.List.getSelectedAttr("data-id");
		obj.x    = this.List.getSelectedAttr("data-x");
		obj.y    = this.List.getSelectedAttr("data-y");
		this.parent.ZoomNode(obj);
	}
}

FilterList.prototype.getMenuPath = function(w,h) {
	var path = ""
	+ " M 0  0"
	+ " l 0 +" + h
	+ " a 5  5 0 0 0 5 +5"
	+ " l " + (w - 5 - 5) + "  0 "
	+ " a 5  5 0 0 0 5 -5"
	+ " l 0 -" + h
	;
	return path;
}

FilterList.prototype.getMenuPath2 = function(w) {
	var x = w/2-5;
	var path = ""
	+ " M " + (x+0) + "  5"
	+ " l 5 +5 "
	+ " l 5 -5 "
	+ " M " + (x+2) + "  5"	
	+ " l 3 +3 "
	+ " l 3 -3 "
	;
	return path;
}

FilterList.prototype.getJunctionPath = function(x, y, /*Optional*/ w) {
	if (w == null) w = 1;
	var r = 5 * w;
	var path = ""
	+ " M " + (x - r) + " " + y
	+ " a " + r + " " + r + " 0 0 0 " + (+2*r) + " " + 0
	+ " a " + r + " " + r + " 0 0 0 " + (-2*r) + " " + 0
	;
	return path;
}

FilterList.prototype.getPipePath = function(x1, y1, x2, y2, /*Optional*/ w) {
	if (w == null) w = 1;
	var path = ""
	+ " M " + x1 + " " + y1
	+ " L " + x2 + " " + y2
	;
	return path;
}

FilterList.prototype.setText = function(val) {
	this.Filter.setValue(val);
}

FilterList.prototype.getText = function() {
	var val = this.Filter.getText();
	return val;
}

FilterList.prototype.setRight = function(val) {
	this.right = val;
	this.left = null;
	var w = this.getWidth();
	var left = val - w;
	this.d3g.style("left", convertToPx(left));
}

FilterList.prototype.setLeft = function(val) {
	this.left = val;
	this.right = null;
	this.d3g.style("left", convertToPx(val));
}

FilterList.prototype.getLeft = function() {
	return parseFloat(this.d3g.style("left"));
}

FilterList.prototype.setTop = function(val) {
	this.top = val;
	this.d3g.style("top", convertToPx(val));
}

FilterList.prototype.getTop = function() {
	return parseFloat(this.d3g.style("top"));
}

FilterList.prototype.setWidth = function(val) {
	this.width = val;
	this.List.setWidth(val);
	this.Filter.setWidth(val);
}

FilterList.prototype.getWidth = function() {
	var w = this.List.getWidth();
	var min = this.getMinWidth();
	return (w > min) ? w : min;
}

FilterList.prototype.getMinWidth = function() {
	return 30;
}

FilterList.prototype.disable = function(bDisabled) {
	if (bDisabled == null) bDisabled = true;
	this.Filter.disable(bDisabled);
	this.List.disable(bDisabled);
}

FilterList.prototype.hide = function(bHide) {
	if (bHide == null || bHide)
		this.d3g.style("visibility", "hidden");
	else
		this.d3g.style("visibility", "inherit");
}

FilterList.prototype.show = function(bShow) {
	if (bShow == null || bShow)
		this.d3g.style("visibility", "inherit");
	else
		this.d3g.style("visibility", "hidden");
}

FilterList.prototype.setPosition = function(point) {
	this.setLeft(point.x);
	this.setTop (point.y);
}

FilterList.prototype.getFilter = function() {
	return document.getElementById(this.filterid);
}

FilterList.prototype.getList = function() {
	return document.getElementById(this.listid);
}

FilterList.prototype.getSelectedXY = function() {
	var p = {"x":null,"y":null};
	p.x = this.List.getSelectedAttr("data-x");
	p.y = this.List.getSelectedAttr("data-y");
	return p;
}

FilterList.prototype.getButtonIndex = function(e) {
	var x = e.clientX - this.getLeft();
	var y = e.clientY - this.getTop()  - this.List.getHeight() - this.Filter.getHeight();
	var cx = this.List.getWidth() / 2 + 1;
	var w = this.List.getWidth();
	if (x > 3 && x < w - 2) {
		return parseInt((y + 9) / 23);
	}
	return -1;
}

//

FilterList.prototype.onMouseMove = function(sid) {
	if (this.parent.onMouseMove) this.parent.onMouseMove(sid);
}
FilterList.prototype.onMouseOver = function(sid) {
	switch (sid) {
		case this.menuid:
			var w = this.List.getWidth();
			var h = this.HEIGHT_DEFAULT;
			if (this.parent.m_data.m_InvList) h += this.HEIGHT_SENSOR;
			this.updateMenu(w, h);
			break;
	}
	if (this.parent.onMouseOver) this.parent.onMouseOver(sid);
}
FilterList.prototype.onMouseOut = function(sid) {
	switch (sid) {
		case this.menuid:
			var w = this.List.getWidth();
			this.d3svgmenu.style("width",w-3);
			this.d3svgmenu.style("height",9+7);
			break;
	}
	if (this.parent.onMouseOut) this.parent.onMouseOut(sid);
}
FilterList.prototype.onMouseUp      = function(sid) { if (this.parent.onMouseUp   ) this.parent.onMouseUp   (sid); }
FilterList.prototype.onMouseDown    = function(sid) { if (this.parent.onMouseDown ) this.parent.onMouseDown (sid); }
//FilterList.prototype.onMouseWheel = function(sid) { if (this.parent.onMouseWheel) this.parent.onMouseWheel(sid); }
FilterList.prototype.onKeyDown      = function(sid) { if (this.parent.onKeyDown   ) this.parent.onKeyDown   (sid); }
FilterList.prototype.onKeyPress     = function(sid) { if (this.parent.onKeyPress  ) this.parent.onKeyPress  (sid); }
FilterList.prototype.onKeyUp        = function(sid) { if (this.parent.onKeyUp     ) this.parent.onKeyUp     (sid); }
FilterList.prototype.onInput        = function(sid) { if (this.parent.onInput     ) this.parent.onInput     (sid); }
FilterList.prototype.onChange       = function(sid) { if (this.parent.onChange    ) this.parent.onChange    (sid); }
FilterList.prototype.onClick        = function(sid) {}
FilterList.prototype.onDblClick = function(sid) { if (this.parent.onDblClick) this.parent.onDblClick(sid); }

//

FilterList.prototype.addListeners = function() {
	this.addListenersForSelection("g"      )
	this.addListenersForSelection("svg"    )
	this.addListenersForSelection("rect"   )//svg
	this.addListenersForSelection("polygon")//svg
	this.addListenersForSelection("path"   )//svg
	this.addListenersForSelection("circle" )//svg
	this.addListenersForSelection("text"   )//svg
	this.addListenersForSelection("input"  )//form
	this.addListenersForSelection("select" )//form
	//this.addListenersForSelection("option" )//form ////////////////////////////////////////////
	this.addListenersForSelection("button" )//form
	this.List.addListeners(); ////////////////////////////////////////////
	this.Filter.addListeners();
}

FilterList.prototype.addListenersForSelection = function(sSelector) {
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
