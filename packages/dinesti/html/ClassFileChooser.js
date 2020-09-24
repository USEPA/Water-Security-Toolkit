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
function FileChooser(sParentId, uniqueString, dataInFrontOf, left, top) {
	this.sParentId = sParentId;
	this.uniqueString = uniqueString;
	this.gid = "g" + uniqueString;
	this.sid = "m_" + uniqueString;
	this.dataInFrontOf = dataInFrontOf;
	this.left = left;
	this.top = top;
	this.contentType = "text/plain";
	this.create();
}

FileChooser.prototype.create = function() {
	this.d3parent = d3.select("#" + this.sParentId);
	this.d3g = this.d3parent.append("g")
		.attr("id","g" + this.uniqueString)
		.attr("data-InFrontOf",dataInFrontOf)
		.style("position","absolute")
		.style("left",convertToPx(this.left))
		.style("top",convertToPx(this.top))
		;
	//this.d3obj = 
	this.d3text = this.d3g.append("input")
		.attr("id",this.sid)
		.attr("data-InFrontOf",this.dataInFrontOf)
		.attr("type","file")
		//.attr("accept","text/plain")
		//.attr("accept","text/tsg")
		//.attr("accept","text/*")
		;
	this.addListeners();
}

FileChooser.prototype.createNewData = function() {
	var data = {
		"fileName"      : this.getFileName()
	};
}

FileChooser.prototype.upload = function(m_Waiting, callback) {
	var m_this = this;
	if (m_Waiting) m_Waiting.show();
	Couch.createUniqueId(this, function(uuid) {
		var data = this.createNewData();
		var http = Couch.HttpFactory("PUT", GlobalData.CouchDb + uuid, JSON.stringify(data));
		http.Send(function(e,res) {
			var rev = res.data.rev;
			var formData;
			try {
				formData = new FormData();
			} catch (e) {
				d3.select("#gAll").style("visibility","");
				alert("FormData object not supported!");
				return;
			}
			m_this.uploadTo(m_this, uuid, rev, m_Waiting, callback);
		});
	});
}

FileChooser.prototype.uploadTo = function(m_this, uuid, rev, m_Waiting, callback) {
	var fileName = this.getFileName();
	var contentType = this.contentType;
	var f = this.getFile();
	var reader = new FileReader();
	reader.onloadend = function(e) {
		var text = e.target.result;
		var http = Couch.HttpFactory("PUT", GlobalData.CouchDb + uuid + "/" + fileName + "?rev=" + rev, text);
		http.setHeader("Content-Type", contentType);
		http.Send(function(e,res) {
			if (m_Waiting) m_Waiting.hide();
			if (callback) callback.call(m_this, e, res);
		});
	}
	reader.readAsText(f);
}

FileChooser.prototype.getFileName = function() {
	if (!this.hasFile()) return "";
	return this.getFile().name;
}

FileChooser.prototype.hasFile = function() {
	if (!this.getFiles()) return false;
	return (this.getFiles().length > 0);
}

FileChooser.prototype.getFile = function() {
	if (!this.hasFile()) return null;
	return this.getFiles(0);
}

FileChooser.prototype.getFiles = function(i) {
	var obj = this.getObject();
	if (i == null) return obj.files;
	var file = obj.files[i];
	return file;
}

FileChooser.prototype.recreate = function() {
	if (!this.hasFile()) return false;
	var obj = this.getObject();
	var opacity = this.d3text.style("opacity");
	var disabled = obj.disabled;
	var parent = obj.parentNode;
	parent.removeChild(obj);
	//this.d3obj = 
	this.d3text = this.d3g.append("input")
		.attr("id",this.sid)
		.attr("data-InFrontOf",this.dataInFrontOf)
		.attr("type","file")
		;
	this.setOpacity(opacity);
	this.disable(disabled);
	this.addListeners();
}

FileChooser.prototype.getObject = function() {
	var obj = document.getElementById(this.sid);
	return obj;
}

FileChooser.prototype.disable = function(bDisabled) {
	if (bDisabled == null) bDisabled = true;
	var obj = this.getObject();
	obj.disabled = bDisabled;
}

FileChooser.prototype.enable = function(bEnabled) {
	if (bEnabled == null) bEnabled = true;
	this.disable(!bEnabled);
}

FileChooser.prototype.setWidth = function(value) {
	this.d3text.style("width", convertToPx(value));
	//this.d3text.attr("width", value);
}

FileChooser.prototype.setOpacity = function(value) {
	this.d3text.style("opacity", value);
}

FileChooser.prototype.open = function() {
	var obj = this.getObject();
	obj.click();
}

FileChooser.prototype.hide = function(bHide) {
	if (bHide == null) bHide = true;
	if (bHide) {
		this.d3g.style("visibility", "hidden");
	} else {
		this.d3g.style("visibility", "inherit");
	}
}

FileChooser.prototype.show = function(bShow) {
	this.hide(!bShow);
}

//

FileChooser.prototype.onMouseMove = function(sid) {;}
FileChooser.prototype.onMouseOver = function(sid) {;}
FileChooser.prototype.onMouseOut = function(sid) {;}
FileChooser.prototype.onMouseUp = function(sid) {;}
FileChooser.prototype.onMouseDown = function(sid) {;}
//FileChooser.prototype.onMouseWheel = function(sid) {;}
FileChooser.prototype.onKeyDown = function(sid) {;}
FileChooser.prototype.onKeyPress = function(sid) {;}
FileChooser.prototype.onKeyUp = function(sid) {;}
FileChooser.prototype.onInput = function(sid) {;}
FileChooser.prototype.onChange = function(sid) {;}
FileChooser.prototype.onClick = function(sid) {;}
FileChooser.prototype.onDblClick = function(sid) {;}

//

FileChooser.prototype.addListeners = function() {
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

FileChooser.prototype.addListenersForSelection = function(sSelector) {
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
		.on("change"    , function() { 
			m_this.onChange    (this.id); })
		.on("click"     , function() { m_this.onClick     (this.id); })
		.on("dblclick"  , function() { m_this.onDblClick  (this.id); })
		.on("blur"      , function() { 
			m_this.onBlur      (this.id); })
		;
}
