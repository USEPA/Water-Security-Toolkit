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
function DateList(sParentId, uniqueString, dataInFrontOf, left, top, width) {
	this.left = left;
	this.top = top;
	this.width = width;
	this.base = Dropdown;
	this.base (sParentId, uniqueString, dataInFrontOf, left, top, width, true       , null , false    );
	//Dropdown(sParentId, uniqueString, dataInFrontOf, left, top, width, bFirstBlank, nSize, bMultiple) {
	//
//	var m_doUpdate = false;
//	function getDoUpdate_private() {
//		return m_doUpdate;
//	}
//	function setDoUpdate_private(val){
//		m_doUpdate = val;
//	}
	//
	var m_epoch = null;
	function getEpoch_private() {
		return m_epoch;
	}
	function setEpoch_private(val) {
		m_epoch = val;
	}
	this.getEpoch = function() {
		return getEpoch_private();
	}
	this.setEpoch = function(val) {
		setEpoch_private(val);
		this.updateData(val);
	}
	this.updateData = function(epoch) {
		setEpoch_private(epoch);
		if (isNull(epoch) || isBlank(epoch)) {
			Clock.getServerTime(this, function(data) {
				this.updateDataWithTime(data.epoch);
				this.selectValue("0");
				//this.updateDataAddMoreListeners();
			});
		} else {
			this.updateDataWithTime(epoch);
			epoch = this.getSecondsForDay(epoch);
			this.selectUserData("epoch", "" + epoch);
			//this.updateDataAddMoreListeners();
		}
	}
}

DateList.prototype = new Dropdown;

//NetworkList.uniqueAttribute = "data-id";

DateList.prototype.setAttributes = function(sel) {
	sel.attr("data-order", 			function(d,i) {
		return d.order;								});
	sel.attr("data-index", 			function(d,i) {
		return d.index;								});
	sel.attr("data-value", 			function(d,i) {
		return d.index;								});
	sel.attr("data-year", 			function(d,i) {
		return d.year;								});
	sel.attr("data-month", 			function(d,i) {
		return d.month;								});
	sel.attr("data-date", 			function(d,i) {
		return d.date;								});
	sel.attr("data-day", 			function(d,i) {
		return d.day;								});
//	sel.attr("data-inpstartday", 	function(d,i) {
//		return d.inpStartDay;						});
//	sel.attr("data-inpday", 		function(d,i) {
//		return d.inpDay;							});
	sel.attr("data-currenttime", 	function(d,i) {
		return d.currentTime; 						});
	sel.attr("data-more", 			function(d,i) {
		return d.more; 								});
	sel.attr("data-range", 			function(d,i) {
		return d.range;								});
	sel.attr("data-epoch", 			function(d,i) {
		return d.epoch;								});
	sel.text(						function(d,i) {
		return d.text;								});
}

DateList.prototype.updateDataWithTime = function(epoch, beforeRange, afterRange) {
	if (epoch       == null) return;
	if (beforeRange == null) beforeRange = 10;
	if (afterRange  == null) afterRange  = 10;
	//
	var item = {}
	item.text = "More..."
	item.more = 1; // 1 is just the value that denotes the top of the list
	item.range = beforeRange;
	//
	var list = [];
	list.push(item);
	//
	for (var i = -beforeRange; i < 0; i++) {
		var item = this.updateDataWithTime2(epoch, i, i + beforeRange);
		list.push(item);
	}
	//
	list.push({"index": 0, "order": beforeRange, "text": "", "currentTime": epoch});
	//
	for (var i = 0; i < afterRange; i++) {
		var item = this.updateDataWithTime2(epoch, i, afterRange + 1 + i);
		list.push(item);
	}
	//
	var item = {}
	item.text = "More..."
	item.more = 2; // 2 is just the value that denotes the bottome of the list
	item.range = afterRange;
	list.push(item);
	this.updateList(list);
}

DateList.prototype.updateDataWithTime2 = function(epoch, i, order) {
	var item = {};
	item.order = order;
	item.index = i;
	item.day = i;
	item.currentTime = epoch;
	var nSecondsForDay = this.getSecondsForDay(epoch);
	item.epoch = nSecondsForDay + 24 * 60 * 60 * i; // add a days-worth of seconds
	item.date = "";
	var date = new Date(item.epoch * 1000);
	item.year = date.getFullYear();
	item.month = date.getMonth();
	item.date = date.getDate();
	item.day = date.getDay(); // the JSON days start with Sunday as zero
	var sDay = GlobalData.Days[item.day];
	var sMon = GlobalData.Months[item.month];
	item.text = sDay + " " + sMon + " " + item.date;
	//item.inpStartDay = 1; // Starts on a Monday (TODO - put this in couch config doc)
	//item.inpDay = (item.day < item.inpStartDay) ? item.day - item.inpStartDay + 7 : item.day - item.inpStartDay;
	return item;
}

DateList.prototype.getSecondsForDay = function(epoch) {
	//var epoch = this.getEpoch();
	if (epoch == null) return this.getSelectedUserDataInt("epoch");
	var date = new Date(epoch * 1000);
	var y = date.getFullYear();
	var m = date.getMonth();
	var n = date.getDate();
	var date = new Date(y, m, n, 0, 0, 0, 0);
	return parseInt( date.valueOf() / 1000 );
}

DateList.prototype.getSecondsForTime = function() {
	var epoch = this.getEpoch();
	var nSecondsForDay = this.getSecondsForDay(epoch);
	return epoch - nSecondsForDay;
}

DateList.prototype.onChange = function(sid) {
	var nMore = this.getSelectedUserDataInt("more");
	var item = {};
	if (nMore == 1) {
		var len = this.getLength();
		var beforeRange = this.getUserDataInt(1, "range");
		var afterRange = this.getUserDataInt(len - 1, "range");
		var epoch = this.getUserDataFromValue("0", "currentTime");
		this.updateDataWithTime(parseInt(epoch), beforeRange + 5, afterRange);
		this.selectValue("0");
	} else if (nMore == 2) {
		var len = this.getLength();
		var beforeRange = this.getUserDataInt(1, "range");
		var afterRange = this.getUserDataInt(len - 1, "range");
		var epoch = this.getUserDataFromValue("0", "currentTime");
		this.updateDataWithTime(parseInt(epoch), beforeRange, afterRange + 5);
		this.selectValue("0");
	}
}



