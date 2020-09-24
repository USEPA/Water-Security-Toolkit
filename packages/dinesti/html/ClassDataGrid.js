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
function DataGrid(x,y,w,h) {
	this.x = x;
	this.y = y;
	this.w = w;
	this.h = h;
	this.rowHeight = 18;
	this.rowGap = 2;
	this.colGap = 8;
	this.colWidth = [];
	this.fontSize = convertToPx(11);
	//
	this.sParentId = "";
	this.gid = "";
	this.uniqueString = "";
	this.dataInFrontOf = "";
	this.parentOnMouseOver = null;
	this.selectedIndex = -1;
	this.selectedIndexOld = -1;
	this.makeVisibleIndex = -1;
	//
	this.nColumns = 0;
	this.colNames = [];
	this.colTitles = [];
	this.textFill = [];
	//this.getTextFill = function(d, iCol) { return "rgba(0,0,0,1)"; };
	//
	this.nHeaderHeight = 20;
	//
	this.ScrollHandleIdX = "";
	this.ScrollHandleIdY = "";
	//
	this.nScrollVThick        = 12;
	this.nScrollVHandleThick  =  8;
	this.nScrollVHandleLength = 20;
	this.nScrollVHandleCurve  =  4;
	//
	this.nScrollHThick        =  0;
	this.nScrollHHandleThick  =  8;
	this.nScrollHHandleLength = 20;
	this.nScrollHHandleCurve  =  4;
	//
	this.bgFill                  = "rgba(200,200,200,0.5)";
//	this.bgFill                  = "rgb(200,200,200)";
	this.headerFill1             = "rgb(240,240,240)";
	this.headerFill2             = "rgb(180,180,180)";
	this.headerTextFill          = "rgb(000,000,000)";
	this.rowFill                 = "rgb(235,235,235)";
	this.rowFillSelected         = "rgb(146,159,179)";//198,211,230//095,108,128//146,159,179
	this.textFillDefault         = "rgb(000,000,000)";
	//
	this.scrollFill              = "rgba(150,150,150,0.5)";
	this.scrollFillHover         = "rgba(150,150,150,0.5)";
	this.scrollHandleFill        = "rgba(050,050,050,0.4)";
	this.scrollHandleFillHover   = "rgba(030,030,030,0.6)";
	this.scrollHandleStroke      = "rgba(020,020,020,0.0)";
	this.scrollHandleStrokeHover = "rgba(020,020,020,0.0)";
	//
	this.bDragScrollV = false;
	this.bDragScrollH = false;
	this.dragStartPoint  = {"x":null,"y":null};
	this.dragStartCTM = null;
	//
	this.base = Control;
	this.base(null, null, null);
}

DataGrid.prototype = new Control;

////////////////////////////////////////////////////////////////////////////////////

DataGrid.prototype.eventOnSelectChange = function(sid){;}

// for super class
DataGrid.prototype.updateData = function(){;}

DataGrid.prototype.createDataGrid = function(data) {
	if (data) this.data = data;
	this.sid = "g" + this.uniqueString;
	var m_this = this;
	d3.select("#" + this.sParentId).append("g").attr("id",this.gid)
		.style("position","absolute")
		.style("left",convertToPx(this.x))
		.style("top",convertToPx(this.y))
		;
	this.d3gHeader = 
	d3.select("#" + this.gid).append("g").attr("id",this.gid + "-Header")
		.style("position","absolute")
		.style("left",convertToPx(0))
		.style("top",convertToPx(0))
		;
	this.d3svgHeader =
	d3.select("#" + this.gid + "-Header").append("svg").attr("id","svg" + this.uniqueString + "-Header")
		.attr("width" ,this.w)
		.attr("height",this.nHeaderHeight)
		;
	this.HeaderGradient = new Gradient("linearGradient_HeaderGradient1","svg" + this.uniqueString + "-Header", 0, 0, 0, 100);
	this.HeaderGradient.addStop(  0, this.headerFill1, 1.0);
	this.HeaderGradient.addStop( 95, this.headerFill2, 1.0);
	this.HeaderGradient.addStop(100, "rgba(90,90,90,1)", 1.0);
	this.HeaderGradient.create();
	this.d3rectHeader =
	d3.select("#svg" + this.uniqueString + "-Header").append("rect").attr("id","rect" + this.uniqueString + "-Header")
		.attr("data-InFrontOf",this.dataInFrontOf)
		.attr("x",0)
		.attr("y",0)
		.attr("width",this.w)
		.attr("height",this.nHeaderHeight)
		//.attr("fill",this.headerFill)
		//.attr("fill","url(#" + this.HeaderGradient.uniqueString + ")")
		.attr("fill", this.HeaderGradient.fill)
		;
	////
	var x = this.colGap;
	for (var iCol = 0; iCol < this.nColumns; iCol++) {
		var w = this.colWidth[iCol];
		if (iCol == this.nColumns-1)
			w = this.w - x;
		this.d3svgHeader.append("text").attr("id","text" + this.uniqueString + "-Header" + iCol)
			.attr("data-InFrontOf",this.dataInFrontOf)
			.attr("x",x)
			.attr("y",this.nHeaderHeight/2 + 4)
			.attr("fill",this.headerTextFill)
			.text(this.colTitles[iCol])
			;
		this.d3svgHeader.append("rect").attr("id","rectTextCover" + this.uniqueString + "-Header" + iCol)
			.attr("data-InFrontOf",this.dataInFrontOf)
			.attr("x",x)
			.attr("y",0)
			.attr("width",w)
			.attr("height",this.nHeaderHeight)
			.attr("fill","rgba(0,0,0,0)")
			.text(this.colTitles[iCol])
			;
		x = x + this.colWidth[iCol] + this.colGap;
	}
	////
	this.d3gMain = 
	d3.select("#" + this.gid).append("g").attr("id",this.gid + "-Main")
		.style("position","absolute")
		.style("left",convertToPx(0))
		.style("top",convertToPx(this.nHeaderHeight))
		;
	this.d3svgMain = 
	d3.select("#" + this.gid + "-Main").append("svg").attr("id","svg" + this.uniqueString)
		.attr("width" ,this.w)
		.attr("height",this.h)
		;
	this.d3rectMain = 
	d3.select("#svg" + this.uniqueString).append("rect").attr("id","rect" + this.uniqueString + "Bg")
		.attr("data-InFrontOf",this.dataInFrontOf)
		.attr("x",0)
		.attr("y",0)
		.attr("width",this.w)
		.attr("height",this.h)
		.attr("fill",this.bgFill)
		;
	d3.select("#svg" + this.uniqueString).append("g").attr("id","g" + this.uniqueString)
		.attr("transform","translate(0,0)")
		;
	////
	this.updateDataGrid();
	////
	this.d3rectScrollV = 
	d3.select("#svg" + this.uniqueString).append("rect").attr("id","rect" + this.uniqueString + "ScrollV")
		.attr("data-InFrontOf",this.dataInFrontOf)
		.attr("x",this.w - this.nScrollVThick)
		.attr("y",0)
		.attr("width",this.nScrollVThick)
		.attr("height",this.h - this.nScrollHThick)//use horizontal here
		.attr("fill",this.scrollFill)
		.attr("cursor","default")
		;
	this.d3rectScrollVHandle = 
	d3.select("#svg" + this.uniqueString).append("rect").attr("id","rect" + this.uniqueString + "ScrollVHandle")
		.attr("data-InFrontOf",this.dataInFrontOf)
		.attr("x",this.w - this.nScrollVHandleThick - 0.5*(this.nScrollVThick - this.nScrollVHandleThick))
		.attr("y",0)
		.attr("rx",this.nScrollVHandleCurve)
		.attr("ry",this.nScrollVHandleCurve)
		.attr("width",this.nScrollVHandleThick)
		.attr("height",this.nScrollVHandleLength)
		.attr("fill",this.scrollHandleFill)
		.attr("cursor","pointer")
		;
	this.d3disableHeader = 
	this.d3svgHeader.append("rect").attr("id", "rect" + this.uniqueString + "Disable")
		.attr("data-InFrontOf",this.dataInFrontOf)
		.attr("x",0)
		.attr("y",0)
		.attr("width",this.w)
		.attr("height",this.h)
		.attr("fill","black")
		.attr("fill-opacity", 0.5)
		.attr("display", "none")
//		.style("visibility", "hidden")
		;
	this.d3disableMain = 
	this.d3svgMain.append("rect").attr("id", "rect" + this.uniqueString + "Disable")
		.attr("data-InFrontOf",this.dataInFrontOf)
		.attr("x",0)
		.attr("y",0)
		.attr("width",this.w)
		.attr("height",this.h)
		.attr("fill","black")
		.attr("fill-opacity", 0.5)
		.attr("display", "none")
//		.style("visibility", "hidden")
		;
	this.addListeners();
}

DataGrid.prototype.disable = function(bDisabled) {
	bDisabled = (bDisabled == null) ? true : bDisabled;
	var s = (bDisabled) ? "inherit" : "none";
	if (this.d3disableHeader) this.d3disableHeader.attr("display", s);
	if (this.d3disableMain  ) this.d3disableMain  .attr("display", s);
	this.disabled = bDisabled;

}

DataGrid.prototype.enable = function(bEnabled) {
	bEnabled = (bEnabled == null) ? true : bEnabled;
	this.disable(!bEnabled);
}

DataGrid.prototype.updateDataGrid = function(data) {
	var m_this = this;
	if (data) this.data = data;
	this.nRows = this.data.length;
	////
	var d3SimData = 
			d3.select("#g" + this.uniqueString)
				.selectAll("rect." + this.uniqueString + "Data")
				.data(this.data)
				;
	this.d3EnterTransition_rect(d3SimData.enter().insert("rect"), "Data", false);
	this.d3EnterTransition_rect(d3SimData.transition()          , "Data", false);
	d3SimData.exit().remove();
	////
	//for (var iCol = 0; iCol < this.colNames.length; iCol++) {
	for (var iCol = 0; iCol < this.nColumns; iCol++) {
		var dWidth = this.colGap;
		for (var i = 0; i < iCol; i++) {
			dWidth = dWidth + this.colWidth[i] + this.colGap;
		}
		var d3SimData = d3.select("#g" + this.uniqueString).selectAll("text." + this.uniqueString + "Data" + (iCol+1))
			.data(this.data)
			;
		this.d3EnterTransition_text(d3SimData.enter().insert("text"), iCol, dWidth);
		this.d3EnterTransition_text(d3SimData.transition()          , iCol, dWidth);
		d3SimData.exit().remove()
			;
	}
	////
	var d3SimData = 
			d3.select("#g" + this.uniqueString)
				.selectAll("rect." + this.uniqueString + "TextCover")
				.data(this.data)
				;
	this.d3EnterTransition_rect(d3SimData.enter().insert("rect"), "TextCover", true);
	this.d3EnterTransition_rect(d3SimData.transition()          , "TextCover", true);
	d3SimData.exit().remove();
	////
	if (this.selectedIndex >= this.data.length)
		this.selectRow(-1);
	this.onChange();
	this.addListeners();
	//
	var m = this.getCTM();
	var mf = -m.f;
	var mfmax = this.getTotalRowHeight(this.data.length);
	if (mf > mfmax)  this.makeRowVisible(0);
	//
	if (this.makeVisibleIndex > -1) this.makeRowVisible(this.makeVisibleIndex);
}

DataGrid.prototype.d3EnterTransition_rect = function(d3sel, sClass, bCover) {
	var m_this = this;
	d3sel
	.attr("class",this.uniqueString + sClass)
	.attr("id",function(d,i) {
		return "rect" + m_this.uniqueString + sClass + "-" + (i + 1);
	})
	.attr("data-InFrontOf",this.dataInFrontOf)
	.attr("x",0)
	.attr("y",function(d,i) {
		return m_this.getTotalRowHeight(i);
	})
	.attr("width",this.w)
	.attr("height",this.rowHeight)
	;
//	.attr("fill",function(d,i) {
//		if (bCover)
//			return "rgba(0,0,0,0.0)";
//		if (i == m_this.selectedIndex)
//			return m_this.rowFillSelected;	
//		else
//			return m_this.rowFill;
//	})
//	;	
	var type = typeof(m_this.rowFill); // function, string, object(array)
	var length = m_this.rowFill.length;
	if (bCover) {
		d3sel.attr("fill", d3.rgb(0,0,0));
		d3sel.attr("fill-opacity", 0);
	} else if (type == "function") {
		d3sel.attr("fill", function(d,i) { return m_this.rowFill.call(m_this, d, i); });
//	} else if (type == "function") {
//		d3sel.attr("fill", function(d,i) { return m_this.rowFill(d,i); });
//	} else if (type == "function") {
//		d3sel.attr("fill", m_this.rowFill);
	} else if (type == "string") {
		d3sel.attr("fill", function(d,i) {
			if (i == m_this.selectedIndex) {
				return m_this.rowFillSelected;	
			} else {
				return m_this.rowFill;
			}
		});
	} else if (type == "object") {
		d3sel.attr("fill", d3.rgb(255,0,0));
	}
	return;
	
}

DataGrid.prototype.d3EnterTransition_text = function(d3sel, iCol, dWidth) {
	var m_this = this;
	d3sel
	.attr("class",this.uniqueString + "Data" + (iCol+1))
	.attr("id",function(d,i) {
		return "text" + m_this.uniqueString + "Data" + (iCol+1) + "-" + (i+1);
	})
	.attr("data-InFrontOf",this.dataInFrontOf)
	.attr("x",dWidth)
	.attr("y",function(d,i) {
		return 0.72 * m_this.rowHeight + m_this.getTotalRowHeight(i);
	})
	.attr("fill", this.textFill[iCol])
	.style("font-size",this.fontSize)
	;
	//
	var type = typeof(m_this.colNames); // function, string, object(array)
	if (type == "object") {
		d3sel
		.text(function(d,i) {
			var t = typeof(m_this.colNames[iCol]); // function, string, object(array)
			if (t == "function") {
				return m_this.colNames[iCol].call(m_this, d, i, iCol);
			} else if (t == "string") {
				var s = d.value[m_this.colNames[iCol]];
				if (!s)
					return "-";
				return s;
			}
		})
		;
	} else if (type == "function") {
		d3sel
		.text(function(d,i) { return m_this.colNames.call(m_this, d, i, iCol); })
		;
	}
}

DataGrid.prototype.resize = function(w, h) {
	this.w = w;
	this.h = h;
	if (this.d3svgHeader == null) return;
	this.d3svgHeader.attr("width", this.w);
	this.d3rectHeader.attr("width", this.w);
	this.d3svgMain.attr("width", this.w);
	this.d3svgMain.attr("height", this.h);
	this.d3rectMain.attr("width", this.w);
	this.d3rectMain.attr("height", this.h);
	this.d3rectScrollV.attr("height", this.h - this.nScrollHThick);
	this.d3rectScrollV.attr("x",this.w - this.nScrollVThick);
	this.d3rectScrollVHandle.attr("x",this.w - this.nScrollVHandleThick - 0.5*(this.nScrollVThick - this.nScrollVHandleThick));
}

DataGrid.prototype.removeRow = function() {
	var fRemoveRow = function(arr,irow) {
		var arr1 = arr.slice(0,iRow);
		var arr2 = arr.slice(iRow+1);
		return arr1.concat(arr2);
	}
	var iRow = this.selectedIndex;
	this.data = fRemoveRow(this.data,iRow);
	this.updateDataGrid();
}

DataGrid.prototype.selectRowNext = function() {
	var i = this.selectedIndex;
	if (i == -1)
		return;
	if (i >= this.getRowCount() - 1)
		return;
	//this.deselectAllRows();
	//this.deselectRow(i);
	this.selectRow(i+1);
	this.makeRowVisible(this.selectedIndex);
	return;
	var i2 = this.selectedIndex;
	var sRowY = d3.select("#rect" + this.uniqueString + "Data-" + (i2+1)).attr("y");
	var dRowY = parseFloat(sRowY);
	var m = this.getCTM();
	if (dRowY - (-m.f) > this.h - this.rowHeight) {
		m.f = this.h - this.rowHeight - dRowY;
		this.setCTM(m);
		this.setScrollV(m);
	}
}

DataGrid.prototype.selectRowPrevious = function() {
	var i = this.selectedIndex;
	if ( i < 1)
		return;
	//this.deselectAllRows();
	//this.deselectRow(i);
	this.selectRow(i-1);
	this.makeRowVisible(this.selectedIndex);
	return;
	var i2 = this.selectedIndex;
	var sRowY = d3.select("#rect" + this.uniqueString + "Data-" + (i2+1)).attr("y");
	var dRowY = parseFloat(sRowY);
	var m = this.getCTM();
	if (dRowY - (-m.f) < 0) {
		m.f = -dRowY;
		this.setCTM(m);
		this.setScrollV(m);
	}
}

DataGrid.prototype.selectLastRow = function() {
	var iRow = this.data.length;
	this.selectRow(iRow);
	this.makeRowVisible(iRow);
}

DataGrid.prototype.selectRow = function(irow) {
	if (irow == this.selectedIndex) return;
	if (irow < 0 || irow >= this.getRowCount()) {
		if (this.selectedIndex == -1) return;
	}
	this.selectedIndexOld = this.selectedIndex;
	d3.select("#rect" + this.uniqueString + "Data-" + (this.selectedIndex + 1)).attr("fill",this.rowFill);
	this.selectedIndex = irow;
	d3.select("#rect" + this.uniqueString + "Data-" + (this.selectedIndex + 1)).attr("fill",this.rowFillSelected);
	this.onChange();
}

DataGrid.prototype.deselectRow = function(irow) {
	d3.select("#rect" + this.uniqueString + "Data-" + (irow + 1)).attr("fill",this.rowFill);
	this.selectRow(-1);
}

DataGrid.prototype.deselectAllRows = function(irow) {
	d3.select("#rect" + this.uniqueString + "Data-" + (this.selectedIndex + 1)).attr("fill",this.rowFill);
	this.selectRow(-1);
}

DataGrid.prototype.isVisible = function() {
	var sValue = d3.select("#" + this.sParentId).style("visibility");
	return sValue == "visible";
}

DataGrid.prototype.isSelected = function() {
	var index = this.selectedIndex;
	return index < 0;
}

DataGrid.prototype.makeRowVisible = function(iRow) {
	var i = (iRow == null) ? this.selectedIndex : iRow;
	var d3sel = d3.select("#rect" + this.uniqueString + "Data-" + (i + 1));
	//if (d3sel[0][0] == null) return;
	var bNull = (d3sel[0][0] == null);	
	var sRowY = (bNull) ? this.getTotalRowHeight(iRow) : d3sel.attr("y");
	var dRowY = parseFloat(sRowY);
	var m = this.getCTM();
	var mfbefore = m.f;
	var bHidden = false;
	// if the row is hidden (or partially hidden) at the bottom
	if (dRowY - (-m.f) > this.h - this.rowHeight) {
		m.f = this.h - this.rowHeight - dRowY;
		bHidden = true;
	}
	// if the row is hidden (or partially hidden) at the top
	if (dRowY - (-m.f) < 0) {
		m.f = -dRowY;
		bHidden = true;
	}
	if (bHidden) {
		this.setCTM(m);
		this.setScrollV(m);
	}
}

DataGrid.prototype.scrollTo = function(i) {
	var h = this.getTotalRowHeight(i);
	var m = this.getCTM();
	m.f = -(h);
	this.setCTM(m);
	this.setScrollV(m);
}

DataGrid.prototype.hasSelection = function() {
	return (this.selectedIndex > -1) && (this.selectedIndex < this.data.length);
}

DataGrid.prototype.getSelectedData = function() {
	if (this.selectedIndex == -1) return null;
	return this.data[this.selectedIndex];
}

DataGrid.prototype.getCTM = function() {
	var g = document.getElementById("g" + this.uniqueString);
	return g.getCTM();
}

DataGrid.prototype.setCTM = function(m) {
	var sid = "g" + this.uniqueString;
	d3.select("#" + sid).attr("transform", "matrix(" + m.a + "," + m.b + "," + m.c + "," + m.d + "," + m.e + "," + m.f + ")");
}

DataGrid.prototype.setScrollV = function(m) {
	if (typeof m  == "number") {
		alert("DataGrid.setScrollV(m) takes a transform matrix object, not a number");
		return;
	} 
	var sScrollVHandle = "rect" + this.uniqueString + "ScrollVHandle";
	d3.select("#" + sScrollVHandle).attr("y", -1 * m.f * this.getVScrollIncr());
}

DataGrid.prototype.getRowCount = function() {
	if (true) return this.data.length;
	var nCount = 0;
	for (var i = 0; i < this.data.length; i++) {
		nCount++;
	}
	return nCount;
}

DataGrid.prototype.getTotalRowHeight = function(i) {
	var n = (i == null) ? 1 : i;
	return n * (this.rowHeight + this.rowGap);
}

DataGrid.prototype.getVisibleRowCount = function() {
	return this.h / this.getTotalRowHeight();
}

DataGrid.prototype.getMaxVScrollTranslation = function() {
	return this.getTotalRowHeight() * (this.getRowCount() - 1.5);
}

DataGrid.prototype.getVScrollTranslation = function() {
	var m = getCTM();
	return -m.f;
}

DataGrid.prototype.limitTranslate = function(mf) {
	var max = this.getMaxVScrollTranslation();
	if (-mf > max) 
		return -max;
	if (-mf < 0)
		return 0;
	return mf;
}

DataGrid.prototype.getMaxVScrollLength = function() {
	return this.h - this.nScrollVHandleLength;
}

DataGrid.prototype.getVScrollIncr = function() {
	return (this.getMaxVScrollLength()) / (this.getMaxVScrollTranslation());
}

///////////////////////////////////////////////////////////////////////////////////////////////
// event handlers
//
DataGrid.prototype.onMouseMove = function(sid) {
	if (this.bDragScrollV) {
		this.doDrag();
	}
}

DataGrid.prototype.doDrag = function() {
	if (this.bDragScrollV) {
		var delta = this.dragStartPoint.y - GlobalData.y;
		var svg = document.getElementById("svg" + this.uniqueString);
		var m = svg.createSVGMatrix();
		var m = m.translate(0, delta / this.getVScrollIncr());
		var m = this.dragStartCTM.multiply(m);
		m.f = this.limitTranslate(m.f);
		this.setCTM(m);
		this.setScrollV(m);
	}
}

DataGrid.prototype.onMouseOver = function(sid) {
	switch(sid) {
		case "rect" + this.uniqueString + "ScrollV":
			break;
		case "rect" + this.uniqueString + "ScrollVHandle":
			d3.select("#" + sid).attr("fill",  this.scrollHandleFillHover  );
			d3.select("#" + sid).attr("stroke",this.scrollHandleStrokeHover);
			break;
	}
	this.parentOnMouseOver(this.dataInFrontOf);
}

DataGrid.prototype.onMouseOut = function(sid) {
	switch(sid) {
		case "rect" + this.uniqueString + "ScrollV":
			break;
		case "rect" + this.uniqueString + "ScrollVHandle":
			d3.select("#" + sid).attr("fill",  this.scrollHandleFill  );
			d3.select("#" + sid).attr("stroke",this.scrollHandleStroke);
			break;
	}
}
DataGrid.prototype.onMouseUp = function(sid) {
	this.bDragScrollV = false;
	this.dragStartPoint  = {"x":null,"y":null};
	this.dragStartCTM = null;
	if (!sid) return;
	switch(sid) {
		case "rect" + this.uniqueString + "ScrollV":
			break;
		case "rect" + this.uniqueString + "ScrollVHandle":
			break;
	}
}
DataGrid.prototype.onMouseDown = function(sid) {
	switch(sid) {
		case "rect" + this.uniqueString + "ScrollV":
			break;
		case "rect" + this.uniqueString + "ScrollVHandle":
			this.bDragScrollV = true;
			this.dragStartPoint.x  = GlobalData.x;
			this.dragStartPoint.y  = GlobalData.y;
			this.dragStartCTM = this.getCTM();
			break;
	}
}
DataGrid.prototype.onMouseWheel = function(sid_or_e) {
	var count = this.getRowCount();
	if (count < 2) return;
	var sid,e;
	if (typeof(sid_or_e) == "string") {
		sid = sid_or_e;
		e = window.event;		
	} else {
		e = sid_or_e;
		//sid = e.target.id;
		sid = e.target.parentElement.id;
	}
	switch(sid) {
		case "svg" + this.uniqueString:
		case "g" + this.uniqueString:
			var delta = getWheelDelta(e);
			var svg = document.getElementById("svg" + this.uniqueString);
			var m = svg.createSVGMatrix();
			var m = m.translate(0,delta);
			var mg = this.getCTM();
			var m = mg.multiply(m);
			m.f = this.limitTranslate(m.f);
			this.setCTM(m);
			this.setScrollV(m);
			return;
	}		
}
DataGrid.prototype.onKeyDown = function(sid, keyCode) {
	if (keyCode == null) keyCode = window.event.keyCode;
	switch(sid) {
		case "":
			switch (keyCode) {
				case 38:
					this.selectRowPrevious();
					break;
				case 40:
					this.selectRowNext();
					break;
			}
			break;
		default:
			break;
	}
}
DataGrid.prototype.onKeyPress = function(sid) {}
DataGrid.prototype.onKeyUp = function(sid) {}
DataGrid.prototype.onChange = function(sid) {
	this.raiseEvent({"source": this, "event": "onChange"});
}

DataGrid.prototype.onInput = function(sid) {}
DataGrid.prototype.onClick = function(sid) {
	switch(sid) {
		case "rect" + this.uniqueString + "ScrollV":
			var y = d3.event.offsetY;
			var scrollY = d3.select("#rect" + this.uniqueString + "ScrollVHandle").attr("y");
			var m = this.getCTM();
			if (y > scrollY) {
				m.f = m.f - this.h;
			} else {
				m.f = m.f + this.h;
			}
			m.f = this.limitTranslate(m.f);
			this.setCTM(m);
			this.setScrollV(m);
			return; // always return in this function
	}
	////
	for (var i = 0; i < this.data.length; i++) {
		var sid2 = "rect" + this.uniqueString + "TextCover-" + (i + 1);
		var sel = d3.select("#" + sid);
		var data = sel.datum();
		switch(sid) {
			case sid2:
				//this.deselectAllRows();
				this.selectRow(i);
				this.makeRowVisible(this.selectedIndex);
				this.raiseEvent({"source": this, "event": "onClickRow", "index": i, "data": data});
				break;
			case "":
				break;
			default:
				break;
		}
	}
}
DataGrid.prototype.onDblClick = function(sid) {}
DataGrid.prototype.onBlur = function(sid) {
	var temp = 0;
}
