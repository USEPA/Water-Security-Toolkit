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
function Dropdown(sParentId, uniqueString, dataInFrontOf, left, top, width, bFirstBlank, nSize, bMultiple) {
	var sel = d3.select("#" + this.gid);
	if (sel[0][0])
		sel.remove();
	this.left = left;
	this.top = top;
	this.width = width;
	this.height = null; // TODO
	this.bFirstBlank = bFirstBlank;
	if (bFirstBlank == null)
		this.bFirstBlank = false;
	this.bMultiple = bMultiple;
	if (bMultiple == null)
		this.bMultiple = false;
	this.nSize = nSize;
	this.uniqueAttribute = "";//"data-value";
	this.base = Control;
	this.base(sParentId, uniqueString, dataInFrontOf);
	this.data = null;
}

Dropdown.prototype = new Control;

Dropdown.prototype.create = function() {
	this.d3parent = d3.select("#" + this.sParentId);
	this.d3g = this.d3parent.append("g")
		.attr("id",this.gid)
		.attr("data-InFrontOf",this.dataInFrontOf)
		.style("position","absolute")
		.style("left",convertToPx(this.left))
		.style("top",convertToPx(this.top))
		;
	this.d3select = this.d3g.append("select")
		.attr("id",this.sid)
		.attr("data-InFrontOf", this.dataInFrontOf)
		;
	if (this.width)
		this.d3select.style("width", convertToPx(this.width));
	if (this.height)
		this.d3select.style("height", convertToPx(this.height));
	if (this.nSize)
		this.d3select.attr("size", this.nSize);
	if (this.bMultiple)
		this.d3select.attr("multiple","");
	if (this.bFirstBlank)
		this.d3select.append("option").attr("id","").attr("data-value","");
	this.addListeners();
}

Dropdown.prototype.createList = function(data, callback) {
	var m_this = this;
	this.removeAll();
	var nGroupSize  = 100;
	var nGroups  = parseInt(data.length / nGroupSize) + 1;
	var process = function(nGroups, igroup) {
		var istart = nGroupSize * (igroup);
		var iend   = nGroupSize * (igroup + 1);
		iend = (iend > data.length) ? data.length : iend;
		var sel = m_this.d3select.selectAll("option.Group" + "_" + igroup)
			.data(data.slice(istart,iend), function(d,i) {
				return d.Type + "_" + d.id;
			})
			;
		sel.enter().append("option");
		sel.call(GlobalAddListeners, m_this);
		m_this.EnterTransition(sel);
	}
	var igroup = 0;
	var busy = false;
	var processor = setInterval(function() {
		if (!busy) {
			busy = true;
			process(nGroups, igroup);
			if (++igroup == nGroups) {
				clearInterval(processor);
				if (callback) callback();
			}
			busy = false;
		}
	}, 15);
}

Dropdown.prototype.replaceList = function(newList, data) {
	var bDisabled = this.isDisabled();
	if (data == null) data = [];
	var g = document.getElementById(this.gid);
	var oldList = this.getObject();
	g.replaceChild(newList, oldList);
	this.disable(bDisabled);
	this.d3select = d3.select(newList);
	for (var i = 0; i < data.length; i++) {
		for (var j = 0; j < data[i].length; j++) {
			var node = data[i][j];
			//var sel = this.d3select.select("#" + "m_List_" + node.Type + "_" + node.id);
			var sel = this.d3select.select("#m_" + this.uniqueString + "_" + node.Type + "_" + node.id);
			sel.datum(node); // TODO - make generic
		}
	}
}

Dropdown.prototype.updateList2 = function(data, callback) {
	var m_this = this;
	var d3sel = m_this.d3select.selectAll(".m_" + m_this.uniqueString + "-option")
		.data(data, function(d,i){ 
			return d.Type + "_" + d.id;
		})
		;
	var sel = d3sel.enter().append("option");
	m_this.EnterTransition(sel);
	sel.call(GlobalAddListeners, m_this);
	m_this.EnterTransition(d3sel.transition());
	d3sel.exit().remove();
	d3sel.order();
	if (callback) callback();
}

Dropdown.prototype.updateList = function(data, callback) {
	var m_this = this;
	this.uniqueSelection = this.getSelectedAttr(this.uniqueAttribute);
	this.d3select.selectAll(".m_" + this.uniqueString + "-option").remove();
	//
	if (data == null) 
		data = this.data;
	else
		this.data = data;
	//
	if (data == null) {
		//do nothing
	} else {
		var d3sel = this.d3select.selectAll(".m_" + this.uniqueString + "-option")
			.data(data)
			;
		var sel = d3sel.enter().append("option");
		sel.call(GlobalAddListeners, m_this);
		this.EnterTransition(sel);
		this.EnterTransition(d3sel.transition());
		d3sel.exit().remove();
		d3sel.order();
	}
	if (this.uniqueAttribute.length == 0) {
		var asdf = 0;
	} else if (this.uniqueSelection == null) {
		var asdf = 0;
	} else if (this.uniqueSelection && this.uniqueSelection.length > 0) {
		this.setSelectedAttr(this.uniqueAttribute, this.uniqueSelection);
	}
	if (callback) callback();
}

Dropdown.prototype.insert = function(i, new_option) {
	if (i < 0) return;
	if (this.data == null) this.data = [];	
	if (i >= this.data.length) return;
	this.data.splice(i, 0, new_option);
	this.updateList(this.data);
}

Dropdown.prototype.EnterTransition = function(sel) {
	var m_this = this;
	sel.attr("id",function(d,i) { return m_this.sid + "_" + i; });
	sel.attr("class", this.sid + "-option");
	this.setAttributes(sel);
}

Dropdown.prototype.setAttributes = function(sel) {
	sel.attr("data-value", function(d,i) {
		if (d.value)
			return d.value;
		return d;
	});
	sel.text(function(d,i) {
		if (d.text)
			return d.text;
		return d;
	});
}

Dropdown.prototype.removeAll = function() {
	var d3sel = this.d3select.selectAll(".m_" + this.uniqueString + "-option");
	d3sel.remove();
}

Dropdown.prototype.getLength = function() {
	var list = this.getObject();
	return list.options.length;
}

Dropdown.prototype.getSize = function() {
	return this.d3select.attr("size");
}

/*
Dropdown.prototype.getWidth = function() {
	var w = this.d3g.style("width");
	return parseFloat(w);
	var list = this.getObject();
	return list.clientWidth;
}

Dropdown.prototype.getHeight = function() {
	var h = this.d3g.style("height");
	return parseFloat(h);
	var list = this.getObject();
	return list.clientHeight;
}
*/

Dropdown.prototype.setSize = function(value) {
	this.nSize = value;
	this.d3select.attr("size", value);
}

Dropdown.prototype.setWidth = function(value) {
	this.width = value;
	this.d3select.style("width", convertToPx(value));
}

Dropdown.prototype.setHeight = function(value) {
//	var list = this.getObject();
//	list.clientHeight = value;
}

Dropdown.prototype.setLeft = function(value) {
	this.left = value;
	this.d3g.style("left", convertToPx(value));
}

Dropdown.prototype.setTop = function(value) {
	this.top = value;
	this.d3g.style("top", convertToPx(value));
}

//

Dropdown.prototype.getSelectedText = function() {
	var list = this.getObject();
	if (list.selectedIndex < 0) return null;
	return list[list.selectedIndex].value;
}

Dropdown.prototype.getSelectedValue = function() {
	return this.getSelectedUserData("value");
}

Dropdown.prototype.getSelectedUserData = function(value) {
	return this.getSelectedAttr("data-" + value);
}

Dropdown.prototype.getSelectedUserDataInt = function(value) {
	var sVal = this.getSelectedUserData(value);
	var nVal = parseInt(sVal);
	if (isNaN(nVal)) return sVal;
	return nVal;
}

Dropdown.prototype.getSelectedAttr = function(attr) {
	var list = this.getObject();
	if (list.selectedIndex == -1)
		return null;
	var val = list[list.selectedIndex].getAttribute(attr);
	//return (val == null) ? "" : val;
	return val;
}

Dropdown.prototype.getSelectedIndex = function() {
	var list = this.getObject();
	return list.selectedIndex;
}

//

Dropdown.prototype.setSelectedIndex = function(i) {
	var list = this.getObject();
	list.selectedIndex = i;
}

Dropdown.prototype.setSelectedAttr = function(attr, value) {
	var list = this.getObject();
	for (var i = 0; i < this.getLength(); i++) {
		if (list[i].getAttribute(attr) == value) {
			this.setSelectedIndex(i);
			return;
		}
	}
}

Dropdown.prototype.selectPrevious = function() {
	if (this.getLength() == 0) return;
	if (!this.hasSelection()) {
		this.selectFirst();
		return;
	}
	if (this.firstSelected()) return;
	var index = this.getSelectedIndex();
	this.setSelectedIndex(index - 1);
}

Dropdown.prototype.selectNext = function() {
	if (this.getLength() == 0) return;
	if (!this.hasSelection()) {
		this.selectFirst();
		return;
	}
	if (this.lastSelected()) return;
	var index = this.getSelectedIndex();
	this.setSelectedIndex(index + 1);
}

Dropdown.prototype.selectFirst = function () {
	if (this.getLength() == 0) return;
	this.setSelectedIndex(0);
}

Dropdown.prototype.selectLast = function () {
	var len = this.getLength();
	this.setSelectedIndex(len - 1);
}

Dropdown.prototype.firstSelected = function () {
	if (!this.hasSelection()) return false;
	return this.getSelectedIndex() == 0;
}
	
Dropdown.prototype.lastSelected = function () {
	if (!this.hasSelection()) return false;
	return this.getSelectedIndex() == this.getLength() - 1;
}

Dropdown.prototype.hasSelection = function() {
	return !(this.getSelectedIndex() == -1);
}

Dropdown.prototype.getText = function(i) {
	if (i == null) 
		return this.getSelectedText();
	var list = this.getObject();
	return list[i].value;
}

Dropdown.prototype.getValue = function(i) {
	if (i == null) 
		return this.getSelectedValue();
	var list = this.getObject();
	var val = list[i].getAttribute("data-value");
	if (val == null)
		val = list[i].value;
	return val;
}

Dropdown.prototype.getUserData = function(i, name) {
	if (i == null)
		return this.getSelectedUserData(name);
	return this.getAttr(i, "data-" + name);
}

Dropdown.prototype.getUserDataInt = function(i, name) {
	return parseInt(this.getUserData(i, name));
}

Dropdown.prototype.getUserDataFromValue = function(sValue, name) {
	if (sValue == null)
		return this.getSelectedUserData(name);
	for (var i = 0; i < this.getLength(); i++) {
		var value = this.getValue(i);
		if (value == sValue) {
			return this.getUserData(i, name);
		}
	}
	return null;
}

Dropdown.prototype.getAttr = function(i, attr) {
	var list = this.getObject();
	var val = list[i].getAttribute(attr);
	return val;
}

Dropdown.prototype.getItem = function(i) {
	if (i == null)
		i = this.getSelectedIndex();
	if (i < 0) return null;
	var list = this.getObject();
	if (i >= list.length) return null; 
	return list[i];
}

Dropdown.prototype.getFirstItem = function() {
	return this.getItem(0);
}

Dropdown.prototype.getLastItem = function() {	
	var list = this.getObject();
	return list[list.length];
}

Dropdown.prototype.forEach = function(fCallback) {
	var list = this.getObject();
	for (var i = 0; i < this.getLength(); i++) {
		fCallback(list[i]);
	}
}

Dropdown.prototype.selectText = function(sText) {
	for (var i = 0; i < this.getLength(); i++) {
		var text = this.getText(i);
		if (text == sText) {
			this.setSelectedIndex(i);
			break;
		}
	}
}

Dropdown.prototype.selectUserData = function(datatype, sValue) {
	var oldVal = this.getSelectedUserData(datatype);
	if (sValue == oldVal)
		return false;
	for (var i = 0; i < this.getLength(); i++) {
		var value = this.getUserData(i, datatype);
		if (value == sValue) {
			this.setSelectedIndex(i);
			return true;
		}
	}
	return false;
}

Dropdown.prototype.selectValue = function(sValue) {
	//var t = typeof sValue;
	//if (t != "string") sValue = "" + sValue;
	var oldVal = this.getSelectedValue();
	if (sValue == oldVal)
		return false;
	for (var i = 0; i < this.getLength(); i++) {
		var value = this.getValue(i);
		if (value == sValue) {
			this.setSelectedIndex(i);
			return true;
		}
	}
	return false;
}

Dropdown.prototype.set = function(sVal) {
	var index = 0;
	for (var i = 0; i < this.getLength(); i++) {
		if (this.getValue(i) == sVal) {
			index = i;
			break;
		}
	}
	this.setSelectedIndex(index);
}

Dropdown.prototype.checkValue = function(val) {
	var list = this.getObject();
	var i = list.selectedIndex;
	if (i == -1)
		return true;
	var sVal = list[i].getAttribute("data-value");
	if (sVal == null)
		sVal = list.value;
	return sVal != val;
}

Dropdown.prototype.isEmpty = function() {
	var list = this.getObject();
	if (list.length == 0) return true;
	return !this.checkValue("");
}

Dropdown.prototype.getObject = function() {
	var list = document.getElementById(this.sid);
	if (list == null) {
		return null;
	}
	return list;
}

Dropdown.prototype.move = function(dx, dy) {
	this.left -= dx;
	this.top  -= dy;
	this.setPosition();
}

Dropdown.prototype.setPosition = function(x, y) {
	this.left = x ? x : this.left;
	this.top  = y ? y : this.top ;
	this.d3g.style("left", convertToPx(this.left));
	this.d3g.style("top" , convertToPx(this.top));
}

Dropdown.prototype.isDisabled = function(){
	var input = this.getObject();
	return input.disabled;
}

Dropdown.prototype.disable = function(bDisabled) {
	if (bDisabled == null) bDisabled = true;
	var input = this.getObject();
	input.disabled = bDisabled;
	if (bDisabled) {
		var bg = d3.select("#" + input.id).style("background");
		var bg = d3.select("#" + input.id).style("background-color");
		var bg = d3.select("#" + input.id).style("border-color");
		d3.select("#" + input.id).style("background-color","rgba(150,150,150,1.0)");
		d3.select("#" + input.id).style("border-color"    ,"rgba(170,170,170,1.0)");
	} else {
		d3.select("#" + input.id).style("background-color","");
		d3.select("#" + input.id).style("border-color"    ,"");		
	}
}
//

Dropdown.prototype.stopScroll = function(e) {
	var obj = this.getObject();
	var rows = obj.rows ? obj.rows : 0;
	var count = this.getLineCount ? this.getLineCount() : obj.childElementCount;
	var size = obj.size ? obj.size : rows;
	if (obj.scrollTop == 0 && getWheelDelta(e) > 0) // TODO - global
		return cancelEvent(e); // TODO - global
	var a = obj.scrollTop / ((obj.clientHeight+1) / size);
	var b = count - size;
	if ((a == b || b < 0) && getWheelDelta(e) < 0) // TODO - global
		return cancelEvent(e); // TODO - global
	//
	var deltaX = getWheelDeltaX(e);
	var bLeft = (obj.scrollLeft == 0);
	var bRight = (obj.scrollWidth - obj.scrollLeft == obj.clientWidth);
	if (bLeft && deltaX > 0) return cancelEvent(e);
	if (bRight && deltaX < 0) return cancelEvent(e);
	//
	return;
}

//Dropdown.prototype.enlarge = function(z) {
//	var n = this.d3select.style("font-size");
//	var n = parseInt(n);
//	this.d3select.style("font-size", convertToPx(n * z));
//	var n = this.d3select.style("font-size");
//}

//

Dropdown.prototype.addListeners = function() {
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

Dropdown.prototype.addListenersForSelection = function(sSelector) {
	var m_this = this;
	var d3g = d3.select("#" + this.gid);
	var sel = d3g.selectAll(sSelector).call(GlobalAddListeners, m_this);
}

