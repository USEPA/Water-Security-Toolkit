// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

//
// TODO - deal with horizontal scroll bar
//

//
// Constructor
//
function HttpRequest(sType, sConn, sBody) {
	this.sType = sType;
	this.sConn = sConn;
	this.sBody = sBody;
	this.headers = null;
	this.HttpRequest = new XMLHttpRequest();
	this.bFatal = false;
	this.bAlertError = false;
	this.bLogRequests = false;//true;
	this.nLogTries = 0;
	this.nLogId = 0;
}

//
// Static Properties
//
HttpRequest.bLogRequests = true;
HttpRequest.nLogId = 0;
HttpRequest.READY_STATE_UNINITIALIZED = 0;
HttpRequest.READY_STATE_OPEN          = 1;
HttpRequest.READY_STATE_SENT          = 2;
HttpRequest.READY_STATE_RECEIVING     = 3;
HttpRequest.READY_STATE_LOADED        = 4;

//
// Public Member Methods
//
HttpRequest.prototype.toString = function() {
	return this.sType + " \"" + this.sConn + "\"";
}
	
HttpRequest.prototype.Send = function(fCallBack) {
	var m_this = this;
	////
	if (this.sType == "") {
		fCallBack(null, {"data":{}});
		return;
	}
	////
	if (HttpRequest.bLogRequests && m_this.bLogRequests) {
		var date = {"date": new Date()};
		var date = JSON.stringify(date);
		var date = JSON.parse(date);
		m_this.Log(m_this, date.date);
	}
	////
	this.HttpRequest.open(this.sType, this.sConn);
	////
	if (this.headers) {
		for (var i = 0; i < this.headers.length; i++) {
			var header = this.headers[i];
			this.HttpRequest.setRequestHeader(header.key, header.value);
		}
	}
	////
	this.HttpRequest.onreadystatechange = function(e) {
		var bOpera = (this.toString() == "[object XMLHttpRequest]");
		var target = (bOpera) ? this : e.target;
		if (target == null) {
			if (m_this.bFatal) return;
			m_this.bFatal = true;
			if (m_this.bAlertError)
				alert("HttpRequest Error:\nthe event callback argument is null!\n\n" + m_this.toString());
			fCallBack(null,null);
			return;
		}
		//console.log(/*this,*/ target.readyState, target.status);
		if (target.readyState == HttpRequest.READY_STATE_LOADED) {
			if (target.status >= 400) {
				console.log(target.status, m_this)
				if (m_this.bAlertError)
					alert("HttpRequest Error " + e.target.status + ":\n" + e.target.statusText + "\n\n" + m_this.toString());
			}
			var res = {};
			res.text = target.responseText;
			if (target.status == 409) 
				res.text = m_this.sBody;
			if (res.text.length > 0) {
				// in Firefox the first call when refreshing is to _changes and returns "{'results':["
				//console.log(m_this.toString());
				//console.log(res.text);
				try {
					res.data = JSON.parse(res.text);
					 if (typeof res.data == "string") {
						if (res.data.length > 0)
							res.data = JSON.parse(res.data);
						else
							res.data = null;
					}
				} catch (e) {
					res.data = null;
				}
			}
			var e2 = {};
			e2.target = target;
			var e = (e) ? e : e2;
			fCallBack(e,res);
		}
	};
	////
	try {
		if (this.sBody) {
			var temp = 1234;
		}
		this.HttpRequest.send(this.sBody);
	} catch(e) {
		if (this.bFatal) return
		this.bFatal = true;
		if (this.bAlertError)
			alert("HttpRequest Error:\nmake sure you are not running this from \"file:///\"\n\n" + this.toString());
	}
}

HttpRequest.prototype.setHeader = function(key, value) {
	if (this.headers == null) this.headers = [];
	this.headers.push({"key": key, "value": value});
}

HttpRequest.prototype.Log = function(m_this, sDate, nLogId) {
	if (m_this.nLogTries > 999) return;
	if (HttpRequest.nLogId > 999999) HttpRequest.nLogId = 0;
	if (nLogId == null)
		m_this.nLogId = HttpRequest.nLogId++;
	else
		m_this.nLogId = nLogId;
	var sLogId = HttpRequest.Pad(m_this.nLogId, 6);
	//var sTries = HttpRequest.Pad(m_this.nLogTries++, 2);
	var sTries = "" + m_this.nLogTries++;
	var httpLog = new XMLHttpRequest();
	httpLog.open("PUT", GlobalData.CouchUpdateQuery + "-gui-HttpRequestLog?body_key=" + sLogId);//+ sDate + "-" + sLogId + "-" + sTries);
	httpLog.onreadystatechange = function(e) {
		if (e.target.readyState == HttpRequest.READY_STATE_LOADED) {
			if (e.target.status == 409) {
				m_this.Log(m_this, sDate, m_this.nLogId);
			}
			if (e.target.status >= 300) {
				if (m_this.bAlertError)
					alert(e.target.statusText);
			}
		}
	}
	//httpLog.send(m_this.toString());
	httpLog.send(JSON.stringify({"Date":sDate,"sTries":sTries,"sType":m_this.sType,"sConn":m_this.sConn,"sBody":m_this.sBody}))
}

HttpRequest.Pad = function(num, len) {
    var str = "" + num;
    while (str.length < len) {
		str = "0" + str;
	}
	return str;
}