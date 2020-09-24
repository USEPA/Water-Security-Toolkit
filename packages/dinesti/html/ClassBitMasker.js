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
function BitMasker(sid, n1, n2) {
	this.sid = sid;
	if (n1 == null) {
		this.n = 0;
		this.imin = 0;
	} else if (n2 == null) {
		this.n = n1;
		this.imin = 0;
	} else {
		this.n = n2;
		this.imin = n1;
	}
	this.values = [];
	for (var i = n1; i < n2 + 1; i++) {
		this.values[i] = false;
	}
}

BitMasker.prototype.setOn = function(indecies) {
	if (typeof(indecies) == "number") {
		return this.setValue(indecies, true);
	} else {
		var bChange = false;
		for (var i = 0; i < indecies.length; i++) {
			var index = indecies[i];
			bChange = bChange | this.setValue(index, true);
		}
		return bChange;
	}
}

BitMasker.prototype.setOff = function(indecies) {
	if (typeof(indecies) == "number") {
		return this.setValue(indecies, false);
	} else {
		var bChange = false;
		for (var i = 0; i < indecies.length; i++) {
			var index = indecies[i];
			bChange = bChange | this.setValue(index, false);
		}
		return bChange;
	}
}

BitMasker.prototype.flip = function(indecies) {
	if (typeof(indecies) == "number") {
		var value = this.values[indecies]
		return this.setValue(indecies, !value);
	} else {
		for (var i = 0; i < indecies.length; i++) {
			var index = indecies[i];
			var value = this.values[index]
			this.setValue(index, !value);
		}
		return true;
	}
}

BitMasker.prototype.set = function(index_or_value, value_or_null) {
	return this.setValue(index_or_value, value_or_null);
}

BitMasker.prototype.setValues = function(values) {
	if (typeof(values) == "number") {
		for (var i = this.imin; i < this.imin + this.n; i++) {
			return this.setValue(i, values);
		}
	} else {
		var bChange = false;
		for (var i = 0; i < values.length; i++) {
			var value = values[i];
			if (value == null) continue;
			bChange = bChange | this.setValue(i, value);
		}
		return bChange;
	}
}

BitMasker.prototype.setValue = function(index_or_value, value_or_null) {
	var index;
	var value;
	if (value_or_null == null) {
		value = index_or_value;
		return this.setAll(value);
	}
	index = index_or_value;
	value = value_or_null;
	var oldVal = this.values[index];
	if (value == 0)
		this.values[index] = false;
	else if (value == 1)
		this.values[index] = true;
	else
		this.values[index] = value;
	return !(value == oldVal);
}

BitMasker.prototype.setAll = function(value) {
	var bChange = false;
	for (var i = this.imin; i < this.imin + this.n; i++) {
		bChange = bChange | this.setValue(i, value);
	}
	return bChange;
}

BitMasker.prototype.get = function(index) {
	return this.getValue(index);
}

BitMasker.prototype.getValue = function(index) {
	if (!this.checkBounds(index)) return null;
	return this.values[index];
}

BitMasker.prototype.getInt = function(index) {
	var value = this.getValue(index);
	if (value == null) return null;
	return value ? 1 : 0;
}

BitMasker.prototype.checkBounds = function(index) {
	return (index >= this.imin && index < this.imin + this.n);
}

BitMasker.prototype.or = function(indecies) {
	if (this.n == 0) return false;
	if (indecies == null || indecies.length == 0) {
		var all = [];
		for (var i = this.imin; i < this.imin + this.n; i++)
			all.push(i);
		return this.or(all);
	}
	var retval = false;
	for (var index = 0; index < indecies.length; index++)
		retval = retval || this.get(index);
	return retval;
}

BitMasker.prototype.and = function(indecies) {
	if (indecies == null || indecies.length == 0) {
		var all = [];
		for (var i = this.imin; i < this.imin + this.n; i++)
			all.push(i);
		return this.and(all);
	}
	var retval = false;
	for (var index = 0; index < indecies.length; index++)
		retval = retval && this.get(index);
	return retval;
}

BitMasker.prototype.toString = function() {
	var s = "";
	for (var index = this.imin; index < this.imin + this.n; index++) {
		if (index > 0) s = s + ", ";
		s = s + this.values[index].toString();
	}
	return s;
}

BitMasker.prototype.setFromDict = function(obj) {
	//[{"index":0, "value":true/false}]
	for (var i in obj) {
		this.set(obj[i].index, obj[i].value);
	}
}

BitMasker.prototype.toDict = function(obj) {
	var retVal = []
	for (var i = this.imin; i < this.imin + this.n; i++) {
		retVal.push({"index": i, "value": this.values[i]});
	}
	return retVal;
}
