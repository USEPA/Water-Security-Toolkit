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
function NetworkList(sParentId, uniqueString, dataInFrontOf, left, top, width) {
	this.left = left;
	this.top = top;
	this.width = width;
	this.base = Dropdown;
	this.base (sParentId, uniqueString, dataInFrontOf, left, top, width, true       , null , false    );
	//Dropdown(sParentId, uniqueString, dataInFrontOf, left, top, width, bFirstBlank, nSize, bMultiple) {
}

NetworkList.prototype = new Dropdown;

//NetworkList.uniqueAttribute = "data-id";

NetworkList.prototype.setAttributes = function(sel) {
	sel.attr("data-rev",      function(d,i) { 
		return (d && d.value) ? d.value.rev      : null; });
	sel.attr("data-date",     function(d,i) { 
		return (d && d.value) ? d.key            : null; });
	sel.attr("data-id",       function(d,i) {
		return (d && d.value) ? d.id             : null; });
	sel.attr("data-value",    function(d,i) {
		return (d && d.value) ? d.id             : null; });
	sel.attr("data-jsonfile", function(d,i) { 
		return (d && d.value) ? d.value.jsonFile : null; });
	sel.attr("data-wqmfile",  function(d,i) { 
		return (d && d.value) ? d.value.wqmFile  : null; });
	sel.attr("data-filename", function(d,i) {
		return (d && d.value) ? d.value.fileName : null; });
	sel.attr("data-duration", function(d,i) {
		return (d && d.value && d.value.TimeData) ? d.value.TimeData.Duration   : null; });
	sel.attr("data-step",     function(d,i) {
		return (d && d.value && d.value.TimeData) ? d.value.TimeData.ReportStep : null; });
	sel.text(                 function(d,i) { 
		return (d && d.value) ? d.value.name     : null; });
}

NetworkList.prototype.updateData = function(uuid) {
	Couch.getView(this, GlobalData.CouchInpList, function(data) {
		var rows = data ? data.rows : [];
		this.updateList(rows);
		this.selectValue(uuid);
	});
}

NetworkList.prototype.getInpInfo = function() {
	var docId    = this.getValue();
	var fileName = this.getSelectedUserData   ("filename");
	var wqmFile  = this.getSelectedUserData   ("wqmfile" );
	var duration = this.getSelectedUserDataInt("duration");
	var step     = this.getSelectedUserDataInt("step"    );
	return {"docId": docId, "fileName": fileName, "wqmFile": wqmFile, "duration": duration, "step": step};
}

NetworkList.prototype.getSelectedDuration = function() {
	return this.getSelectedUserDataInt("duration");
}