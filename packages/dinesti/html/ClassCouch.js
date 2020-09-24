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
function Couch() {
}

Couch.kill                 = "_kill";
Couch.erd                  = "_erd";
Couch.inp                  = "_inp";
Couch.tevasim              = "_tevasim";
Couch.impact               = "_impact";
Couch.inversion            = "_inversion";
Couch.grab                 = "_grab";
Couch.training             = "_training";
Couch.events               = "_events";
Couch.svg                  = "_svg";
Couch.gis                  = "_gis";
Couch.time                 = "_time";
Couch.compact              = "_compact";
Couch.canary               = "_canary";
Couch.InpList              = "m_InpList";
Couch.MeasureList          = "m_MeasureList";
Couch.ScenariosList        = "m_ScenariosList";
Couch.SimList              = "m_SimList";
Couch.SimListHide          = "m_SimListHide";
Couch.SimListPlus          = "m_SimListPlus";
Couch.ImpactList           = "m_ImpactList";
Couch.InversionList        = "m_InversionList";
Couch.InversionListByEvent = "m_InversionListByEvent";
Couch.GrabList             = "m_GrabList";
Couch.GrabListByEvent      = "m_GrabListByEvent";
Couch.TrainingList         = "m_TrainingList";
Couch.EventsList           = "m_EventsList";
Couch.EventsGrabGrid       = "m_EventsGrabGrid";
Couch.EventsCanaryGrid     = "m_EventsCanaryGrid";
Couch.EventsLog            = "m_EventsLog";
Couch.GisList              = "m_GisList";
Couch.SvgList              = "m_svgFile";
Couch.UnknownList          = "m_UnknownList";
Couch.GenList              = "m_GenList";
Couch.DeleteAll            = "m_DeleteAll";

// none of these are prototyped so they are static

Couch.setDbName = function(db, ProxyPass) {
	GlobalData.ProxyPass        = ProxyPass;
	GlobalData.CouchDbWithoutSlashes = db;
	GlobalData.CouchDb          = "/" + db + "/";
	GlobalData.CouchView        = "/" + db + "/_design/gui/_view/";
	GlobalData.CouchUpdateQuery = "/" + db + "/_design/gui/_update/query/";
	GlobalData.CouchInpExe      = "/" + db + "/_inp";
	GlobalData.CouchInpList     = "m_InpList";
}

Couch.createUniqueId = function(m_this, callback) {
	Couch.getDoc(m_this, "_uuid", function(data) {
		callback.call(m_this, data.uuid);
	});
}

Couch.createUniqueIds = function(m_this, nCount, callback) {
	Couch.getDoc(m_this, "_uuid?count=" + nCount, function(data) {
		if (data.uuids) {
			callback.call(m_this, data.uuids);
		} else if (data.uuid) {
			var uuids = [];
			uuids.push(data.uuid);
			callback.call(m_this, uuids);
		}
	});
}

Couch.SetValue = function(doc, key, value, callback) {
	Couch.GetDoc(doc, function(data) {
		data[key] = value;
		Couch.SetDoc(doc, data, function(e,res) {
			if (callback) callback(e,res);
		});
	});
}

Couch.GetValue = function(doc, key, callback) {
	Couch.GetDoc(doc, function(data) {
		callback(data[key]);
	});
}

Couch.DeleteKey = function(doc, key, callback) {
	Couch.GetDoc(doc, function(data) {
		delete data[key];
		Couch.SetDoc(doc, data, function(e,res) {
			if (callback) callback(e,res);
		});
	});
}

Couch.DeleteDoc = function(doc, callback) {
	var http = Couch.HttpFactory("HEAD", GlobalData.CouchDb + doc);
	http.Send(function(e,res) {
		var head = e.target.getResponseHeader("etag");
		var rev  = JSON.parse(head);
		var http = Couch.HttpFactory("DELETE", GlobalData.CouchDb + doc + "?rev=" + rev);
		http.Send(function(e,res) {
			if (callback) callback(e,res);
		});
	});
}

Couch.SetDoc = function(doc, data, callback) {
		var http = Couch.HttpFactory("PUT", GlobalData.CouchDb + doc, JSON.stringify(data));
		http.Send(function(e,res){
			if (callback) callback(e,res);
		});
}

Couch.GetDoc = function(doc, callback) {
	Couch.Get(GlobalData.CouchDb + doc, function(data) {
		if (callback) callback(data);
	});
}

/////////////////////////////////////////////////////////////////////////////
Couch.updateDoc = function(m_this, doc, key, value, callback) {
	var url = GlobalData.CouchUpdateQuery + doc + "?body_key=" + key;
	var http = Couch.HttpFactory("PUT", url, value);
	http.Send(function(e,res) {
		if (callback) callback.call(m_this, e, res);
	});
}
Couch.setValue = function(m_this, doc, key, value, callback) {
	Couch.getDoc(m_this, doc, function(data) {
		data[key] = value;
		Couch.setDoc(m_this, doc, data, function(e,res) {
			if (callback) callback.call(m_this, e, res);
		});
	});
}
Couch.getValue = function(m_this, doc, key, callback) {
	Couch.getDoc(m_this, doc, function(data) {
		callback.call(m_this, data[key]);
	});
}
Couch.deleteKey = function(m_this, doc, key, callback) {
	Couch.getDoc(m_this, doc, function(data) {
		delete data[key];
		Couch.setDoc(m_this, doc, data, function(e,res) {
			if (callback) callback.call(m_this, e, res);
		});
	});
}
Couch.deleteDoc = function(m_this, doc, callback) {
	var http = Couch.HttpFactory("HEAD", GlobalData.CouchDb + doc);
	http.Send(function(e,res) {
		var head = e.target.getResponseHeader("etag");
		var rev  = JSON.parse(head);
		var http = Couch.HttpFactory("DELETE", GlobalData.CouchDb + doc + "?rev=" + rev);
		http.Send(function(e,res) {
			if (callback) callback.call(m_this, e, res);
		});
	});
}
Couch.setDoc = function(m_this, doc, data, callback) {
	Couch.set(m_this, GlobalData.CouchDb + doc, data, function(e,res) {
		if (callback) callback.call(m_this, e, res);
	});
}
Couch.getDoc = function(m_this, doc, callback) {
	Couch.Get(GlobalData.CouchDb + doc, function(data) {
		if (callback) callback.call(m_this, data);
	});
}
Couch.getView = function(m_this, view, callback) {
	Couch.Get(GlobalData.CouchView + view, function(data) {
		if (callback) callback.call(m_this, data);
	});
}
Couch.getFile = function(m_this, doc, fileName, callback) {
	var url = GlobalData.CouchDb + doc + "/" + fileName;
	var http = Couch.HttpFactory("GET", url);
	http.Send(function(e,res){
		var data = (res && res.data) ? res.data : null;
		var text = (res && res.text) ? res.text : "";
		if (callback) callback.call(m_this, data, text);
	});	
}
Couch.get = function(m_this, url, callback) {
	var http = Couch.HttpFactory("GET", url);
	http.Send(function(e,res) {
		var data = (res && res.data) ? res.data : null;
		if (callback) callback.call(m_this, data);
	});
}
Couch.getText = function(m_this, url, callback) {
	var http = Couch.HttpFactory("GET", url);
	http.Send(function(e,res) {
		var text = (res && res.text) ? res.text : null;
		if (callback) callback.call(m_this, text);
	});
}
Couch.set = function(m_this, url, data, callback) {
	var http = Couch.HttpFactory("PUT", url, JSON.stringify(data));
	http.Send(function(e,res){
		if (callback) callback.call(m_this, e, res);
	});
}

Couch.getConfig = function(m_this, url, callback) {
	if (url == null) url = "";
	if (url.length > 0) 
		var url = location.origin + "/_config/" + url;
	else
		var url = location.origin + "/_config" + url;
	Couch.get(m_this, url, function(data) {
		if (callback) callback.call(m_this, data);
	});
}
Couch.setConfig = function(m_this, url, data, callback) {
	if (url == null) url = "";
	if (url.length > 0) 
		var url = location.origin + "/_config/" + url;
	else
		var url = location.origin + "/_config" + url;
	Couch.set(m_this, url, data, function(e,res) {
		if (callback) callback.call(m_this, e, res);
	});
}

Couch.restart = function() {
	var url = location.origin + "/_restart";
	var http = new HttpRequest("POST", url, "{}");
	http.setHeader("Content-Type", "application/json");
	http.Send(function(e,res) {
		var data = res ? res.data : null;
	});
}

Couch.compact = function(m_this, callback) {
	var url = GlobalData.CouchDb + "_compact";
	var http = new HttpRequest("POST", url, "{}");
	http.setHeader("Content-Type", "application/json");
	http.Send(function(e,res) {
		var data = res ? res.data : null;
		if (callback) callback.call(m_this, data);
	});
}

Couch.getLog = function(m_this, nCount, callback) {
	var url = location.origin + "/_log?bytes=" + nCount;
	Couch.getText(m_this, url, function(text) {
		if (callback) callback.call(m_this, text);
	});
}

/////////////////////////////////////////////////////////////////////////////

Couch.deleteEntireView = function(view) {
	Couch.getView(this, view, function(data) {
		var rows = data && data.rows ? data.rows : [];
		console.log(0, "deleting", view, "count", rows.length);
		for (var i = 0; i < rows.length; i++) {
			var row = rows[i];
			var id = row.id;
			Couch.DeleteDoc(id);
		}
	});
}

Couch.getViewRowCount = function(m_this, view, callback) {
	Couch.getView(m_this, view, function(data) {
		var rows = data && data.rows ? data.rows : [];
		if (callback) callback.call(m_this, rows.length);
	});
}

Couch.GetView = function(view, callback) {
	Couch.Get(GlobalData.CouchView + view, function(data) {
		if (callback) callback(data);
	});
}

Couch.GetFile = function(doc, fileName, callback) {
	var url = GlobalData.CouchDb + doc + "/" + fileName;
	var http = Couch.HttpFactory("GET", url);
	http.Send(function(e,res){
		var data = (res && res.data) ? res.data : null;
		var text = (res && res.text) ? res.text : "";
		if (callback) callback(data, text);
	});
}

Couch.Get = function(url, callback) {
	var http = Couch.HttpFactory("GET", url);
	http.Send(function(e,res) {
		var data = (res && res.data) ? res.data : null;
		if (callback) callback(data);
	});
}

Couch.Upload = function(doc, fileName, rev, contentType, text, callback) {
	var sRev = (rev) ? "?rev=" + rev : "";
	var http = Couch.HttpFactory("PUT", GlobalData.CouchDb + doc + "/" + fileName + sRev, text);
	http.setHeader("Content-Type", contentType);
	http.Send(function(e,res) {
		if (callback) callback(e,res);
	});
}

Couch.HttpFactory = function(req, url, text) {
	var bChanges = url.indexOf("_changes?") > 0;
	if (url.indexOf("?") > 0 && url.indexOf("/_") > 0 && !bChanges)
		url += "&db=" + GlobalData.CouchDbWithoutSlashes;
	//var bDebug = false;
	var bDebug = true;
	if (bDebug) {
		if (url.indexOf("BDGIJHKBIHFAJRHYOLWJVLZRPQXSY") > 0 && req == "PUT") {
			var data = null;
		}
		if (url.indexOf("?") > 0)
			url += "&_DEBUG_CLIENT_SIDE";
		else
			url += "?_DEBUG_CLIENT_SIDE";
	}
	if (GlobalData.ProxyPass)
		url = "/" + GlobalData.ProxyPass + url;
	return new HttpRequest(req, url, text);
}