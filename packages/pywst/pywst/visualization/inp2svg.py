import subprocess
import json
import os
import string
import math
import pyutilib.services
#
class inp2svg:
	#
	LAYER_TYPE_NODE = "NODE"
	LAYER_TYPE_LINK = "LINK"
	#
	SHAPE_CIRCLE    = "Circle"
	SHAPE_SQUARE    = "Square"
	SHAPE_TRIANGLE  = "Triangle"
	SHAPE_DIAMOND   = "Diamond"
	SHAPE_PLUS      = "Plus"
	SHAPE_X         = "X"
	SHAPE_RESERVOIR = "Reservoir"
	SHAPE_TANK      = "Tank"
	SHAPE_JUNCTION  = "Junction"
	SHAPE_PUMP      = "Pump"
	SHAPE_VALVE     = "Valve"
	SHAPE_PIPE      = "Pipe"
	SHAPE_LINE      = "Line"
	#
	def __init__(self, inp_file_name, exe_location=None):
		self.__inp_file_name = inp_file_name
		if not os.path.exists(self.__inp_file_name):
			raise Exception("the specified inp_file_name does not exist: " + self.__inp_file_name)
		#
		if exe_location == None:
                    pyutilib.services.register_executable('jsonwriter')
                    self.__exe_location = pyutilib.services.registered_executable('jsonwriter')
                    if self.__exe_location is None:
                        raise RuntimeError("Cannot find the path to the jsonwriter executable")
                    self.__exe_location = self.__exe_location.get_path()
                else:
                    self.__exe_location = None
                    def _possible_exe(base):
                        yield base
                        for _ext in os.environ.get("PATHEXT", "").split(os.pathsep):
                            yield base+_ext
                    for _exe in _possible_exe(exe_location):
                        if os.path.isfile(_exe) and os.access(_exe, os.X_OK):
                            self.__exe_location = _exe
                            break
		    if self.__exe_location is None:
			raise Exception("the specified exe_location does not exist: " + exe_location)
		#
		args = []
		args.append(self.__exe_location)
		args.append(self.__inp_file_name)
		p = subprocess.Popen(args, stdout=subprocess.PIPE)
		com = p.communicate();
		if p.returncode:
                        raise RuntimeError("jsonwriter call failed with the following error:\n\t%s" % (com,))
		sInpData = com[0]
		InpData = json.loads(sInpData)
                for _field in ("SimList","TimeData"):
                        if _field in InpData:
                                del InpData[_field]
		#
		dataXY, dataXY2 = self.__parseInpFile(self.__inp_file_name)
		#
		Nodes = InpData["Nodes"]
		for line in dataXY:
		#	Nodes[line[0]]["id"] = line[0]
			Nodes[line[0]]["x" ] = float(line[1])
			Nodes[line[0]]["y" ] = float(line[2])
		#
		Vertices = {}
		for line in dataXY2:
			vert = Vertices.get(line[0])
			if vert == None:
				Vertices[line[0]] = []
			Vertices[line[0]].append({"x": float(line[1]), "y": float(line[2])})
		#
		Links = InpData["Links"]
		Xs,Ys = [], []
		linkLengths = []
#		sumLengths = 0
		for link in Links:
			ID = link["ID"]
			link["Vertices"] = Vertices.get(ID, [])
			for vert in link["Vertices"]:
				Xs.append(vert["x"])
				Ys.append(vert["y"])
			#
			x1,y1,x2,y2 = None,None,None,None
			sx = Nodes[link["Node1"]].get("x", 0)
			x1 = float(sx)
			link["x1"] = x1
			sy = Nodes[link["Node1"]].get("y", 0)
			y1 = float(sy)
			link["y1"] = y1
			sx = Nodes[link["Node2"]].get("x", 0)
			x2 = float(sx)
			link["x2"] = x2
			sy = Nodes[link["Node2"]].get("y", 0)
			y2 = float(sy)
			link["y2"] = y2
			#
			Xs.append(x1)
			Xs.append(x2)
			Ys.append(y1)
			Ys.append(y2)
			#
			del link["Node1"]
			del link["Node2"]
			#
			if x1 == None:break
			if y1 == None:break
			if x2 == None:break
			if y2 == None:break
			#
			a2 = pow(x2-x1,2)
			b2 = pow(y2-y1,2)
			c2 = pow(a2+b2,0.5)
			linkLengths.append(c2)
#			sumLengths += c2
			#
		x_min = min(Xs)
		y_min = min(Ys)
		x_max = max(Xs)
		y_max = max(Ys)
		x_range = x_max - x_min
		y_range = y_max - y_min
		min_range = min(x_range, y_range)
		max_range = max(x_range, y_range)
		#
		zFactor = 1
		for i in range(0, -13, -1):
			r = math.pow(10, i)
			if min_range < r: zFactor = math.pow(10, math.ceil(-math.log(r, 10)) + 4)
		#
		if (zFactor < 2) :
			xFactor = 0
			yFactor = 0
		else:
			xFactor = x_min * zFactor
			yFactor = y_min * zFactor
		#
		self.__Factors = {"a": zFactor, "bx": xFactor, "by": yFactor}
		#
		x_min, y_min = self.__scaleXY(x_min, y_min)
		x_max, y_max = self.__scaleXY(x_max, y_max)
		#
		for link in Links:
			link["x1"], link["y1"] = self.__scaleXY(link["x1"], link["y1"])
			link["x2"], link["y2"] = self.__scaleXY(link["x2"], link["y2"])
			for vert in link["Vertices"]:
				vert["x"], vert["y"] = self.__scaleXY(vert["x"], vert["y"])
		#
		for node in Nodes:
			Nodes[node]["x"], Nodes[node]["y"] = self.__scaleXY(Nodes[node].get("x", 0), Nodes[node].get("y", 0))
		#
		x_range = x_max - x_min
		y_range = y_max - y_min
		min_range = min(x_range, y_range)
		max_range = max(x_range, y_range)
		#
		InpData["Bounds" ] = self.__calculateBounds(x_min, y_min, x_max, y_max)
		InpData["Center" ] = self.__calculateCenter(x_min, y_min, x_range, y_range)
		InpData["Ratio"  ] = self.__calculateRatio(x_range, y_range)
		#
		self.__dNodeSize = None
		self.__dNodeSizeDefault = 10
		self.__dLinkSize = None
		self.__dLinkSizeDefault = 1
		#
		self.__dReservoirSize = None
		self.__dTankSize = None
		self.__dJunctionSize = None
		self.__dPumpSize = None
		self.__dValveSize = None
		self.__dPipeSize = None
		#
		self.__sNodeColor = None
		self.__sLinkColor = None
		#
		self.__sReservoirColor = None
		self.__sTankColor = None
		self.__sJunctionColor = None
		self.__sPumpColor = None
		self.__sValveColor = None
		self.__sPipeColor = None
		#
		self.__dOpacityDefault = 0.6
		self.__dOpacity = 0.6
		self.__dNodeOpacity = None
		self.__dLinkOpacity = None
		#
		self.__dReservoirOpacity = None
		self.__dTankOpacity = None
		self.__dJunctionOpacity = None
		self.__dPumpOpacity = None
		self.__dValveOpacity = None
		self.__dPipeOpacity = None
		#
		self.__bLegendDefault = False
		self.__bLegendNodes = None
		self.__bLegendLinks = None
		self.__bLegendReservoirs = None
		self.__bLegendTanks = None
		self.__bLegendJunctions = None
		self.__bLegendPumps = None
		self.__bLegendValves = None
		self.__bLegendPipes = None
		#
		self.__dPad = 0.1
		self.__nScale = None
		self.__nWidth  = None
		self.__nHeight = None
		self.__sBackgroundColor = "#999999"
		self.__bHideBackground = False
		self.__bHideNodes = False
		self.__bHideLinks = False
		self.__bHideJunctions = False
		self.__bHideTanks = False
		self.__bHideReservoirs = False
		self.__bHidePipes = False
		self.__bHidePumps = False
		self.__bHideValves = False
		self.__bHideShapes = False
		self.__bUseEpanetIcons = False
		self.__InpData = InpData
		self.__nUserJunctions = 0
		self.__nUserTanks = 0
		self.__nUserReservoirs = 0
		self.__nUserPipes = 0
		self.__nUserPumps = 0
		self.__nUserValves = 0
		self.__Shapes = []
		self.__TextAt = []
		self.__bShowLegend = False
		self.__sLegendColor = "#dddddd"
		self.__dLegendScale = 1
		self.__dLegendX = 0
		self.__dLegendY = 0
		self.__sFontStyle = "font-size:12px; font-family:Tahoma,Consolas,Lucida Console,Verdana,sans-serif"
		self.__Layers = []
		self.__Layers.append({"label": "Pipes"     })
		self.__Layers.append({"label": "Junctions" })
		self.__Layers.append({"label": "Pumps"     })
		self.__Layers.append({"label": "Valves"    })
		self.__Layers.append({"label": "Tanks"     })
		self.__Layers.append({"label": "Reservoirs"})
		return

	##############################################################################################

	def getFactors(self):
		return self.__Factors

	def getExeLocation(self):
		return self.__exe_location

	def getInpFileName(self):
		return self.__inp_file_name

	def getOutputFileName(self):
		return self.__output_file_name

	def getInpData(self):
		return self.__InpData

#
#################################################
#

	def getNodeColor(self):
		return self.__sNodeColor

	def setNodeColor(self, value):
		if isString(value) or value == None:
			old_value = self.__sNodeColor
			self.__sNodeColor = value
			return old_value
		raise Exception("the argument is not of type str or unicode")

	def getLinkColor(self):
		return self.__sLinkColor

	def setLinkColor(self, value):
		if isString(value) or value == None:
			old_value = self.__sLinkColor
			self.__sLinkColor = value
			return old_value
		raise Exception("the argument is not of type str or unicode")

#
#################################################
#

	def getOpacity(self):
		return self.__dOpacity

	def setOpacity(self, value):
		if value == None:
			old_value = self.__dOpacity
			self.__dOpacity = self.__dOpacityDefault
			return old_value
		if isNumeric(value):
			if value >= 0 and value <= 1:
				old_value = self.__dOpacity
				self.__dOpacity = value
				return old_value
			raise Exception("the argument can not be greater than one or less than zero")
		raise Exception("the argument is not of type float or int or NoneType")

	def getNodeOpacity(self):
		dOpacity = self.__dNodeOpacity
		if dOpacity == None: return self.__dOpacity
		return dOpacity

	def setNodeOpacity(self, value):
		if value == None:
			old_value = self.__dNodeOpacity
			self.__dNodeOpacity = None
			return old_value
		if isNumeric(value):
			if value >= 0 and value <= 1:
				old_value = self.__dNodeOpacity
				self.__dNodeOpacity = value
				return old_value
			raise Exception("the argument can not be greater than one or less than zero")
		raise Exception("the argument is not of type float or int or NoneType")

	def getLinkOpacity(self):
		val = self.__dLinkOpacity
		if val == None: return self.__dOpacity
		return val

	def setLinkOpacity(self, value):
		if value == None:
			old_value = self.__dLinkOpacity
			self.__dLinkOpacity = None
			return old_value
		if isNumeric(value):
			if value >= 0 and value <= 1:
				old_value = self.__dLinkOpacity
				self.__dLinkOpacity = value
				return old_value
			raise Exception("the argument can not be greater than one or less than zero")
		raise Exception("the argument is not of type float or int or NoneType")

#
#################################################
#

	def getNodeSize(self):
		val = self.__dNodeSize
		if val == None: return self.__dNodeSizeDefault
		return val

	def getLinkSize(self):
		val = self.__dLinkSize
		if val == None: return self.__dLinkSizeDefault
		return val

	def setNodeSize(self, value):
		if isNumeric(value) or value == None:
			if value >= 0 or value == None:
				old_value = self.getNodeSize()
				self.__dNodeSize = value
				return old_value
			raise Exception("the argument can not be less than zero")
		raise Exception("the argument is not of type int or float or NoneType")

	def setLinkSize(self, value):
		if isNumeric(value) or value == None:
			if value >= 0 or value == None:
				old_value = self.getLinkSize()
				self.__dLinkSize = value
				return old_value
			raise Exception("the argument can not be less than zero")
		raise Exception("the argument is not of type int or float or NoneType")

#
#################################################
#

	def getReservoirSize(self):
		val = self.__dReservoirSize
		if val == None: return self.getNodeSize()
		return val

	def getTankSize(self):
		val = self.__dTankSize
		if val == None: return self.getNodeSize()
		return val

	def getJunctionSize(self):
		val = self.__dJunctionSize
		if val == None: return self.getNodeSize()
		return val

	def getPumpSize(self):
		val = self.__dPumpSize
		if val == None: return self.getNodeSize()
		return val

	def getValveSize(self):
		val = self.__dValveSize
		if val == None: return self.getNodeSize()
		return val

	def getPipeSize(self):
		val = self.__dPipeSize
		if val == None: return self.getLinkSize()
		return val

#
#################################################
#

	def getReservoirColor(self):
		val = self.__sReservoirColor
		if val == None: 
			val = self.getNodeColor()
			if val == None: return "#009900"
		return val

	def getTankColor(self):
		val = self.__sTankColor
		if val == None: 
			val = self.getNodeColor()
			if val == None: return "#000099"
		return val

	def getJunctionColor(self):
		val = self.__sJunctionColor
		if val == None: 
			val = self.getNodeColor()
			if val == None: return "#000000"
		return val

	def getPumpColor(self):
		val = self.__sPumpColor
		if val == None: 
			val = self.getLinkColor()
			if val == None: return "#999900"
		return val

	def getValveColor(self):
		val = self.__sValveColor
		if val == None: 
			val = self.getLinkColor()
			if val == None: return "#009999"
		return val

	def getPipeColor(self):
		val = self.__sPipeColor
		if val == None: 
			val = self.getLinkColor()
			if val == None: return "#000000"
		return val

#
#################################################
#

	def getReservoirOpacity(self):
		val = self.__dReservoirOpacity
		if val == None: return self.getNodeOpacity()
		return val

	def getTankOpacity(self):
		val = self.__dTankOpacity
		if val == None: return self.getNodeOpacity()
		return val

	def getJunctionOpacity(self):
		val = self.__dJunctionOpacity
		if val == None: return self.getNodeOpacity()
		return val

	def getPumpOpacity(self):
		val = self.__dPumpOpacity
		if val == None: return self.getLinkOpacity()
		return val

	def getValveOpacity(self):
		val = self.__dValveOpacity
		if val == None: return self.getLinkOpacity()
		return val

	def getPipeOpacity(self):
		val = self.__dPipeOpacity
		if val == None: return self.getLinkOpacity()
		return val

#
#################################################
#

	def setReservoirSize(self, value):
		if isNumeric(value) or value == None:
			if value >= 0 or value == None:
				old_value = self.getReservoirSize()
				self.__dReservoirSize = value
				return old_value
			raise Exception("the argument can not be less than zero")
		raise Exception("the argument is not of type int or float or NoneType")

	def setTankSize(self, value):
		if isNumeric(value) or value == None:
			if value >= 0 or value == None:
				old_value = self.getTankSize()
				self.__dTankSize = value
				return old_value
			raise Exception("the argument can not be less than zero")
		raise Exception("the argument is not of type int or float or NoneType")

	def setJunctionSize(self, value):
		if isNumeric(value) or value == None:
			if value >= 0 or value == None:
				old_value = self.getJunctionSize()
				self.__dJunctionSize = value
				return old_value
			raise Exception("the argument can not be less than zero")
		raise Exception("the argument is not of type int or float or NoneType")

	def setPumpSize(self, value):
		if isNumeric(value) or value == None:
			if value >= 0 or value == None:
				old_value = self.getPumpSize()
				self.__dPumpSize = value
				return old_value
			raise Exception("the argument can not be less than zero")
		raise Exception("the argument is not of type int or float or NoneType")

	def setValveSize(self, value):
		if isNumeric(value) or value == None:
			if value >= 0 or value == None:
				old_value = self.getValveSize()
				self.__dValveSize = value
				return old_value
			raise Exception("the argument can not be less than zero")
		raise Exception("the argument is not of type int or float or NoneType")

	def setPipeSize(self, value):
		if isNumeric(value) or value == None:
			if value >= 0 or value == None:
				old_value = self.getPipeSize()
				self.__dPipeSize = value
				return old_value
			raise Exception("the argument can not be less than zero")
		raise Exception("the argument is not of type int or float or NoneType")

#
#################################################
#

	def setReservoirColor(self, value):
		if isString(value) or value == None:
			old_value = self.__sReservoirColor
			self.__sReservoirColor = value
			return old_value
		raise Exception("the argument is not of type str or unicode or NoneType")

	def setTankColor(self, value):
		if isString(value) or value == None:
			old_value = self.__sTankColor
			self.__sTankColor = value
			return old_value
		raise Exception("the argument is not of type str or unicode or NoneType")

	def setJunctionColor(self, value):
		if isString(value) or value == None:
			old_value = self.__sJunctionColor
			self.__sJunctionColor = value
			return old_value
		raise Exception("the argument is not of type str or unicode or NoneType")

	def setPumpColor(self, value):
		if isString(value) or value == None:
			old_value = self.__sPumpColor
			self.__sPumpColor = value
			return old_value
		raise Exception("the argument is not of type str or unicode or NoneType")

	def setValveColor(self, value):
		if isString(value) or value == None:
			old_value = self.__sValveColor
			self.__sValveColor = value
			return old_value
		raise Exception("the argument is not of type str or unicode or NoneType")

	def setPipeColor(self, value):
		if isString(value) or value == None:
			old_value = self.__sPipeColor
			self.__sPipeColor = value
			return old_value
		raise Exception("the argument is not of type str or unicode or NoneType")

#
#################################################
#

	def setReservoirOpacity(self, value):
		if isNumeric(value) or value == None:
			if (value >=0 and value <= 1) or value == None:
				old_value = self.__dReservoirOpacity
				self.__dReservoirOpacity = value
				return old_value
			raise Exception("the argument can not be greater than one or less than zero")
		raise Exception("the argument is not of type float or int or NoneType")

	def setTankOpacity(self, value):
		if isNumeric(value) or value == None:
			if (value >=0 and value <= 1) or value == None:
				old_value = self.__dTankOpacity
				self.__dTankOpacity = value
				return old_value
			raise Exception("the argument can not be greater than one or less than zero")
		raise Exception("the argument is not of type float or int or NoneType")

	def setJunctionOpacity(self, value):
		if isNumeric(value) or value == None:
			if (value >=0 and value <= 1) or value == None:
				old_value = self.__dJunctionOpacity
				self.__dJunctionOpacity = value
				return old_value
			raise Exception("the argument can not be greater than one or less than zero")
		raise Exception("the argument is not of type float or int or NoneType")

	def setPumpOpacity(self, value):
		if isNumeric(value) or value == None:
			if (value >=0 and value <= 1) or value == None:
				old_value = self.__dPumpOpacity
				self.__dPumpOpacity = value
				return old_value
			raise Exception("the argument can not be greater than one or less than zero")
		raise Exception("the argument is not of type float or int or NoneType")

	def setValveOpacity(self, value):
		if isNumeric(value) or value == None:
			if (value >=0 and value <= 1) or value == None:
				old_value = self.__dValveOpacity
				self.__dValveOpacity = value
				return old_value
			raise Exception("the argument can not be greater than one or less than zero")
		raise Exception("the argument is not of type float or int or NoneType")

	def setPipeOpacity(self, value):
		if isNumeric(value) or value == None:
			if (value >=0 and value <= 1) or value == None:
				old_value = self.__dPipeOpacity
				self.__dPipeOpacity = value
				return old_value
			raise Exception("the argument can not be greater than one or less than zero")
		raise Exception("the argument is not of type float or int or NoneType")

#
#################################################
#

	def getLegendNodes(self):
		if self.__bLegendNodes == None: return self.__bLegendDefault
		return self.__bLegendNodes

	def getLegendLinks(self):
		if self.__bLegendLinks == None: return self.__bLegendDefault
		return self.__bLegendLinks

	def getLegendReservoirs(self):
		if self.__bLegendReservoirs == None: return self.getLegendNodes()
		return self.__bLegendReservoirs

	def getLegendTanks(self):
		if self.__bLegendTanks == None: return self.getLegendNodes()
		return self.__bLegendTanks

	def getLegendJunctions(self):
		if self.__bLegendJunctions == None: return self.getLegendNodes()
		return self.__bLegendJunctions

	def getLegendPumps(self):
		if self.__bLegendPumps == None: return self.getLegendLinks()
		return self.__bLegendPumps

	def getLegendValves(self):
		if self.__bLegendValves == None: return self.getLegendLinks()
		return self.__bLegendValves

	def getLegendPipes(self):
		if self.__bLegendPipes == None: return self.getLegendLinks()
		return self.__bLegendPipes

#
#################################################
#

	def setLegendNodes(self, value):
		if type(value) == bool or value == None:
			old_value = self.__bLegendNodes
			self.__bLegendNodes = value
			return old_value
		raise Exception("the argument is not of type bool or NoneType")

	def setLegendLinks(self, value):
		if type(value) == bool or value == None:
			old_value = self.__bLegendLinks
			self.__bLegendLinks = value
			return old_value
		raise Exception("the argument is not of type bool or NoneType")

	def setLegendReservoirs(self, value):
		if type(value) == bool or value == None:
			old_value = self.__bLegendReservoirs
			self.__bLegendReservoirs = value
			return old_value
		raise Exception("the argument is not of type bool or NoneType")

	def setLegendTanks(self, value):
		if type(value) == bool or value == None:
			old_value = self.__bLegendTanks
			self.__bLegendTanks = value
			return old_value
		raise Exception("the argument is not of type bool or NoneType")

	def setLegendJunctions(self, value):
		if type(value) == bool or value == None:
			old_value = self.__bLegendJunctions
			self.__bLegendJunctions = value
			return old_value
		raise Exception("the argument is not of type bool or NoneType")

	def setLegendPumps(self, value):
		if type(value) == bool or value == None:
			old_value = self.__bLegendPumps
			self.__bLegendPumps = value
			return old_value
		raise Exception("the argument is not of type bool or NoneType")

	def setLegendValves(self, value):
		if type(value) == bool or value == None:
			old_value = self.__bLegendValves
			self.__bLegendValves = value
			return old_value
		raise Exception("the argument is not of type bool or NoneType")

	def setLegendPipes(self, value):
		if type(value) == bool or value == None:
			old_value = self.__bLegendPipes
			self.__bLegendPipes = value
			return old_value
		raise Exception("the argument is not of type bool or NoneType")

#
#################################################
#

	# dPad is a fraction of the range that you want to add to both sides of the figure
	def getPad(self):
		return self.__dPad

	def setPad(self, value):
		if isNumeric(value):
			old_value = self.__dPad
			self.__dPad = float(value)
			return old_value
		raise Exception("the argument is not of type float or int")

	def getScale(self):
		return self.__nScale

	def setScale(self, value=None):
		if value == None:
				old_value = self.__nScale
				self.__nScale = value
				return old_value
		if isNumeric(value):
			old_value = self.__nScale
			self.__nScale = float(value)
			return old_value
		raise Exception("the argument is not of type float or int or NoneType")

	def getWidth(self):
		return self.__nWidth

	def setWidth(self, value):
		if value == None:
				old_value = self.__nScale
				self.__nWidth = value
				return old_value
		if isNumeric(value):
			if value >= 0:
				old_value = self.__nWidth
				self.__nWidth = float(value)
				return old_value
		raise Exception("the argument is not of type float or int or NoneType")

	def getHeight(self):
		return self.__nHeight

	def setHeight(self, value):
		if value == None:
				old_value = self.__nScale
				self.__nHeight = value
				return old_value
		if isNumeric(value):
			if value >= 0:
				old_value = self.__nHeight
				self.__nHeight = float(value)
				return old_value
		raise Exception

	def getBackgroundColor(self):
		return self.__sBackgroundColor

	def setBackgroundColor(self, value):
		old_value = self.__bHideBackground
		self.__sBackgroundColor = value
		return old_value

	def getHideBackground(self):
		return self.__bHideBackground

	def hideBackground(self, value=True):
		if type(value) == bool:
				old_value = self.__bHideBackground
				self.__bHideBackground = value
				return old_value
		raise Exception("the argument is not of type bool")

	def hideNodes(self, value=True):
		if type(value) == bool:
				old_value = self.__bHideNodes
				self.__bHideNodes = value
				return old_value
		raise Exception("the arugument is not of type bool")

	def hideLinks(self, value=True):
		if type(value) == bool:
				old_value = self.__bHideLinks
				self.__bHideLinks = value
				return old_value
		raise Exception("the argument is not of type bool")

	def hideJunctions(self, value=True):
		if type(value) == bool:
				old_value = self.__bHideJunctions
				self.__bHideJunctions = value
				return old_value
		raise Exception("the argument is not of type bool")

	def hideTanks(self, value=True):
		if type(value) == bool:
				old_value = self.__bHideTanks
				self.__bHideTanks = value
				return old_value
		raise Exception("the argument is not of type bool")

	def hideReservoirs(self, value=True):
		if type(value) == bool:
				old_value = self.__bHideReservoirs
				self.__bHideReservoirs = value
				return old_value
		raise Exception("the argument is not of type bool")

	def hidePipes(self, value=True):
		if type(value) == bool:
				old_value = self.__bHidePipes
				self.__bHidePipes = value
				return old_value
		raise Exception("the argument is not of type bool")

	def hidePumps(self, value=True):
		if type(value) == bool:
				old_value = self.__bHidePumps
				self.__bHidePumps = value
				return old_value
		raise Exception("the argument is not of type bool")

	def hideValves(self, value=True):
		if type(value) == bool:
				old_value = self.__bHideValves
				self.__bHideValves = value
				return old_value
		raise Exception("the argument is not of type bool")

	def hideShapes(self, value=True):
		if type(value) == bool:
				old_value = self.__bHideShapes
				self.__bHideShapes = value
				return old_value
		raise Exception("the argument is not of type bool")

	def addCircleOn(  self, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeOn(self.SHAPE_CIRCLE  , node, fs, sc, sw, so, fc, fo, a, layer)

	def addSquareOn(  self, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeOn(self.SHAPE_SQUARE  , node, fs, sc, sw, so, fc, fo, a, layer)

	def addTriangleOn(self, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeOn(self.SHAPE_TRIANGLE, node, fs, sc, sw, so, fc, fo, a, layer)

	def addDiamondOn( self, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeOn(self.SHAPE_DIAMOND , node, fs, sc, sw, so, fc, fo, a, layer)

	def addPlusOn(    self, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeOn(self.SHAPE_PLUS    , node, fs, sc, sw, so, fc, fo, a, layer)

	def addXOn(       self, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeOn(self.SHAPE_X       , node, fs, sc, sw, so, fc, fo, a, layer)

	# shape type, node id, relative radius, stroke color, stroke width, stroke opacity
	def addShapeOn(self, sType, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		sTypeSpecified = sType
		sType = self.correctShape(sType)
		if sType == None: raise Exception("the argument, sType, is not one of the valid shape types: " + sTypeSpecified)
		data = self.__InpData["Nodes"].get(node)
		if data == None: raise BadNodeException("the argument, node, is not in the INP file: " + node)
		x = data["x"]
		y = data["y"]
		n = len(self.__Shapes) + 1
		name = sType + str(n)
		return self.addShapeAt(sType, x, y, name, node, fs, sc, sw, so, fc, fo, a, layer)

	def addCircleAt  (self, x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeAt(self.SHAPE_CIRCLE  , x, y, name, fs, sc, sw, so, fc, fo, a, layer)

	def addSquareAt  (self, x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeAt(self.SHAPE_SQUARE  , x, y, name, fs, sc, sw, so, fc, fo, a, layer)

	def addTriangleAt(self, x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeAt(self.SHAPE_TRIANGLE, x, y, name, fs, sc, sw, so, fc, fo, a, layer)

	def addDiamondAt (self, x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeAt(self.SHAPE_DIAMOND , x, y, name, fs, sc, sw, so, fc, fo, a, layer)

	def addPlusAt    (self, x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeAt(self.SHAPE_PLUS    , x, y, name, fs, sc, sw, so, fc, fo, a, layer)

	def addXAt       (self, x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		return self.addShapeAt(self.SHAPE_X       , x, y, name, fs, sc, sw, so, fc, fo, a, layer)

	# shape type, x, y, relative radius, stroke color, stroke-width, stroke-opacity, fill-color, fill-opacity
	def addShapeAt(self, sType, x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None, layer=None):
		sTypeSpecified = sType
		sType = self.correctShape(sType)
		if sType == None: raise Exception("the argument, sType, is not one of the valid shape types: " + sTypeSpecified)
		x, y = self.__scaleXY(x, y)
		if name == None:
			n = len(self.__Shapes) + 1
			name = sType + str(n)
		if id == None:
			id = name
		shape = {}
		shape["x"] = x
		shape["y"] = y
		shape["Type"] = sType
		shape["Name"] = name
		shape["id"]   = id
		if not fs    == None: shape["fs"   ] = fs
		if not sc    == None: shape["sc"   ] = sc
		if not sw    == None: shape["sw"   ] = sw
		if not so    == None: shape["so"   ] = so
		if not fc    == None: shape["fc"   ] = fc
		if not fo    == None: shape["fo"   ] = fo
		if not a     == None: shape["a"    ] = a
		if not layer == None: shape["layer"] = layer
		self.__Shapes.append(shape)
		return shape

	def getShapes(self):
		return self.__Shapes

	def getNodes(self):
		return self.__InpData.get("Nodes")

	def getLinks(self):
		return self.__InpData.get("Links")

	def useEpanetIcons(self, value=True):
		old_value = self.__bUseEpanetIcons
		self.__bUseEpanetIcons = value
		return old_value

	def showLegend(self, value=True):
		old_value = self.__bShowLegend
		self.__bShowLegend = bool(value)
		return old_value

	def hideLegend(self, value=True):
		return not self.showLegend(not value)

	def getLegendColor(self):
		return self.__sLegendColor

	def setLegendColor(self, value):
		old_value = self.__sLegendColor
		self.__sLegendColor = str(value)
		return old_value

	def getLegendScale(self):
		return self.__dLegendScale

	def setLegendScale(self, value):
		old_value = self.__dLegendScale
		self.__dLegendScale = str(value)
		return old_value

	def getLegendX(self):
		return self.__dLegendX

	def setLegendX(self, value):
		old_value = self.__dLegendX
		self.__dLegendX = value
		return old_value

	def getLegendY(self):
		return self.__dLegendY

	def setLegendY(self, value):
		old_value = self.__dLegendY
		self.__dLegendY = value
		return old_value

	def setLegendXY(self, x, y):
		old_x = self.setLegendX(x)
		old_y = self.setLegendY(y)
		return {"x": old_x, "y": old_y}

	def getLegendXY(self):
		x = self.getLegendX()
		y = self.getLegendY()
		return {"x": x, "y": y}

	def addLayer(self, label):
		self.__Layers.append({"label": label})
		return len(self.__Layers) - 1

	def getLayer(self, i):
		return self.__Layers[i]

	def getLayerByLabel(self, label):
		for layer in self.__Layers:
			if layer["label"] == label: return layer
		return None

	def addJunction(self, x, y, name=None):
		return self.addNode(x, y, "Junction", name)

	def addTank(self, x, y, name=None):
		return self.addNode(x, y, "Tank", name)

	def addReservoir(self, x, y, name=None):
		return self.addNode(x, y, "Reservoir", name)

	def addNode(self, x, y, sType, name=None):
		sTypeSpecified = sType
		sType = self.__correctNodeType(sType)
		if sType == None: raise Exception("the argument, sType, is not one of the valid shape types: " + sTypeSpecified)
		if name == None:
			old = []
			while not old == None:
				name = self.__getUserNodeName(sType)
				old = self.__InpData["Nodes"].get(name)
		else:
			if not self.__InpData["Nodes"].get(name) == None:
				raise Exception("the node name specified already exists in the Nodes list: " + name)
		x, y = self.__scaleXY(x, y)
		node = {"x": x, "y": y, "Type": sType, "id": name}
		#
		self.__updateMinMax(x, y)
		self.__updateBounds()
		#
		self.__InpData["Nodes"][name] = node
		return node

	def addText(self, x, y, text, size=None, color=None, angle=None, opacity=None):
		x, y = self.__scaleXY(x, y)
		obj = {"x": x, "y": y}
		obj["text"] = text
		if not size    == None: obj["size"   ] = size
		if not color   == None: obj["color"  ] = color
		if not angle   == None: obj["angle"  ] = angle
		if not opacity == None: obj["opacity"] = opacity
		self.__TextAt.append(obj)
		return obj

	def addLine(self, x1=None, y1=None, x2=None, y2=None, name=None, id=None, width=None, color=None, opacity=None, layer=None):
		sType = "Line"
		x1, y1 = self.__scaleXY(x1, y1)
		x2, y2 = self.__scaleXY(x2, y2)
		obj = {"x1": x1, "y1": y1, "x2": x2, "y2": y2}
		if name == None:
			n = len(self.__Shapes) + 1
			name = sType + str(n)
		if id == None:
			id = name
		obj["Type"] = sType
		obj["Name"] = name
		obj["id"  ] = id
		if not width   == None: obj["width"  ] = width
		if not color   == None: obj["color"  ] = color
		if not opacity == None: obj["opacity"] = opacity
		if not layer   == None: obj["layer"  ] = layer
		self.__Shapes.append(obj)
		return obj

	def addLineBetweenNodes(self, node1, node2, width=None, color=None, opacity=None, layer=None):
		data1 = self.__InpData["Nodes"].get(node1)
		if data1 == None: raise BadNodeException("the argument, node1, was not found in the INP files list of nodes: " + node1)
		x1 = data1["x"]
		y1 = data1["y"]
		data2 = self.__InpData["Nodes"].get(node2)
		if data2 == None: raise BadNodeException("the argument, node2, was not found in the INP files list of nodes: " + node2)
		x2 = data2["x"]
		y2 = data2["y"]
		return self.addLine(x1, y1, x2, y2, None, None, width, color, opacity, layer)

	def addLineFromNode(self, node, x2, y2, width=None, color=None, opacity=None, layer=None):
		data = self.__InpData["Nodes"].get(node)
		if data == None: raise BadNodeException("the argument, node, was not found in the INP files list of nodes: " + node)
		x1 = data["x"]
		y1 = data["y"]
		return self.addLine(x1, y1, x2, y2, None, None, width, color, opacity, layer)

	def addLineToNode(self, x1, y1, node, width=None, color=None, opacity=None, layer=None):
		data = self.__InpData["Nodes"].get(node)
		if data == None: raise BadNodeException("the argument, node, was not found in the INP files list of nodes: " + node)
		x2 = data["x"]
		y2 = data["y"]
		return self.addLine(x1, y1, x2, y2, None, None, width, color, opacity, layer)

	def addLineOnLink(self, link_name, width=None, color=None, opacity=None, layer=None):
		Links = self.__InpData["Links"]
		bFound = False
		for link in Links: # TODO - this will take a while on large networks. 
			if link["ID"] == link_name:
				if bFound: continue
				bFound = True
				x1 = link["x1"]
				y1 = link["y1"]
				x2 = link["x2"]
				y2 = link["y2"]
		if not bFound:
			raise BadLinkException("the argument, link_name, was not found in INP files list of links: " + link_name)
		return self.addLine(x1, y1, x2, y2, None, link_name, width, color, opacity, layer)

	##############################################################################################

	def correctShape(self, sType):
		if sType.upper() == "CIRCLE"	: return self.SHAPE_CIRCLE
		if sType.upper() == "CIR"		: return self.SHAPE_CIRCLE
		if sType.upper() == "C"			: return self.SHAPE_CIRCLE
		if sType.upper() == "O"			: return self.SHAPE_CIRCLE
		if sType.upper() == "SQUARE"	: return self.SHAPE_SQUARE
		if sType.upper() == "SQU"		: return self.SHAPE_SQUARE
		if sType.upper() == "SQ"		: return self.SHAPE_SQUARE
		if sType.upper() == "S"			: return self.SHAPE_SQUARE
		if sType.upper() == "TRIANGLE"	: return self.SHAPE_TRIANGLE
		if sType.upper() == "TRI"		: return self.SHAPE_TRIANGLE
		if sType.upper() == "T"			: return self.SHAPE_TRIANGLE
		if sType.upper() == "DIAMOND"	: return self.SHAPE_DIAMOND
		if sType.upper() == "DIAM"		: return self.SHAPE_DIAMOND
		if sType.upper() == "DIA"		: return self.SHAPE_DIAMOND
		if sType.upper() == "D"			: return self.SHAPE_DIAMOND
		if sType.upper() == "PLUS"		: return self.SHAPE_PLUS
		if sType.upper() == "P"			: return self.SHAPE_PLUS
		if sType.upper() == "CROSS"		: return self.SHAPE_PLUS
		if sType.upper() == "+"			: return self.SHAPE_PLUS
		if sType.upper() == "X"			: return self.SHAPE_X
		if sType.upper() == "RESERVOIR" : return self.SHAPE_RESERVOIR
		if sType.upper() == "TANK"      : return self.SHAPE_TANK
		if sType.upper() == "JUNCTION"  : return self.SHAPE_JUNCTION
		if sType.upper() == "PUMP"      : return self.SHAPE_PUMP
		if sType.upper() == "VALVE"     : return self.SHAPE_VALVE
		if sType.upper() == "PIPE"      : return self.SHAPE_PIPE
		if sType.upper() == "LINE"      : return self.SHAPE_LINE
		return None

	def __scaleXY(self, x, y):
		a  = self.__Factors["a"]
		bx = self.__Factors["bx"]
		by = self.__Factors["by"]
		x = x * a - bx
		y = y * a - by
		return x, y

	def __updateMinMax(self, x, y):
		x_min = self.__InpData["Bounds"]["min"]["x"]
		y_min = self.__InpData["Bounds"]["min"]["y"]
		x_max = self.__InpData["Bounds"]["max"]["x"]
		y_max = self.__InpData["Bounds"]["max"]["y"]
		if x < x_min: self.__InpData["Bounds"]["min"]["x"] = float(x)
		if y < y_min: self.__InpData["Bounds"]["min"]["y"] = float(y)
		if x > x_max: self.__InpData["Bounds"]["max"]["x"] = float(x)
		if y > y_max: self.__InpData["Bounds"]["max"]["y"] = float(y)

	def __getUserNodeName(self, sType):
		if sType.upper() == "JUNCTION":
			self.__nUserJunctions += 1
			return "J" + str(self.__nUserJunctions)
		if sType.upper() == "TANK":
			self.__nUserTanks += 1
			return "T" + str(self.__nUserTanks)
		if sType.upper() == "RESERVOIR":
			self.__nUserReservoirs += 1
			return "R" + str(self.__nUserReservoirs)
		raise Exception("the argument, sType, is not a valid node type (Junction, Tank, Reservoir): " + sType)

	def __correctNodeType(self, sType):
		if sType.upper() == "JUNCTION":
			return "Junction"
		if sType.upper() == "TANK":
			return "Tank"
		if sType.upper() == "RESERVOIR":
			return "Reservoir"
		return None

	def __parseInpFile(self, inp_file_name=None):
		f = open(inp_file_name)
		sInpText = f.read()
		dataXY = []									# this will store our results
		dataXY2 = []
		Lines = sInpText.splitlines()				# make a string array from all the text in the inp file
		sCOORDINATES = "[COORDINATES]"				# this is our target search string
		sVERTICES    = "[VERTICES]"
		bParseXY = False							# this is a flag that says we have reached the target area in the inp file
		bParseXY2 = False
		for line in Lines:							# cycle through each line in the file
			sline = line.strip(string.whitespace)	# strip all white space from the start and end of this line
			index = sline.find(";")					# search for the comment descriptor
			if index == 0: continue					# if the comment descriptor is the first character move to next line
			index = sline.find("[")					# each section heading in the inp file starts with an open bracket
			if index == 0 and bParseXY: 			# if we found a new section AND we already started parsing coordinates...
				bParseXY = False					# stop parsing coordinates
			if index == 0 and bParseXY2:			# if we found a new section AND we already started parsing vertices... 
				bParseXY2 = False					# stop parsing vertices
			if bParseXY:							# if we are in the coordinates section...
				arr = self.__parseInpFileLine(line)	# get an array of items on that line
				if len(arr) > 0:					# if the array contains at least one element...
					dataXY.append(arr)				# then add it to the dataXY results array
			if bParseXY2:							# if we are in the vertices section...
				arr = self.__parseInpFileLine(line)	# get an array of items on that line
				if len(arr) > 0:					# if the array contains at least one element...
					dataXY2.append(arr)				# then add it to the dataXY2 results array
			if len(sline) == 0:	continue			# if this line only contains whitespace... then move on to the next line
			index = sline.find(sCOORDINATES)		# is the coordinates header on this line?
			if index > -1:
				bParseXY = True						# if we found the coordinates heading, begin parsing x's and y's at the next line
				bParseXY2 = False
			index = sline.find(sVERTICES)			# is the coordinates header on this line?
			if index > -1:
				bParseXY2 = True					# if we found the vertices heading, begin parsing x's and y's at the next line
				bParseXY = False
		return dataXY, dataXY2						# once we have reached the end of this section... return the results 

	def __parseInpFileLine(self, line=""):
		line = line.strip(string.whitespace)		# remove leading and trailing whitespace
		line = line.expandtabs(1)					# change internal tabs into spaces
		arr = line.split(";")						# remove any comments
		arr = arr[0].split()						# create a string array with any whitespace as the delimiter
		return arr[0:3]

	def __updateBounds(self):
		x_min = self.__InpData["Bounds"]["min"]["x"]
		y_min = self.__InpData["Bounds"]["min"]["y"]
		x_max = self.__InpData["Bounds"]["max"]["x"]
		y_max = self.__InpData["Bounds"]["max"]["y"]
		self.__InpData["Bounds"] = self.__calculateBounds(x_min, y_min, x_max, y_max)
		x_min = self.__InpData["Bounds"]["min"]["x"]
		y_min = self.__InpData["Bounds"]["min"]["y"]
		x_range = self.__InpData["Bounds"]["range"]["x"]
		y_range = self.__InpData["Bounds"]["range"]["y"]
		self.__InpData["Center"] = self.__calculateCenter(x_min, y_min, x_range, y_range)
		x_range = self.__InpData["Bounds"]["range"]["x"]
		y_range = self.__InpData["Bounds"]["range"]["y"]
		self.__InpData["Ratio" ] = self.__calculateRatio(x_range, y_range)
		return

	def __calculateBounds(self, x_min, y_min, x_max, y_max):
		x_range = x_max - x_min
		y_range = y_max - y_min
		min_range = min(x_range, y_range)
		max_range = max(x_range, y_range)
		Bounds = {}
		Bounds["min"  ] = {"x": x_min  , "y": y_min  }
		Bounds["max"  ] = {"x": x_max  , "y": y_max  }
		Bounds["range"] = {"x": x_range, "y": y_range, "min": min_range, "max": max_range}
		return Bounds

	def __calculateCenter(self, x_min, y_min, x_range, y_range):
		x_center = x_min + x_range / 2
		y_center = y_min + y_range / 2
		return {"x": x_center, "y": y_center}

	def __calculateRatio(self, x_range, y_range):
		return y_range / x_range

	def __calculateScale(self, w, h, pad):
		InpData = self.getInpData()
		Bounds = InpData["Bounds"]
		Range = Bounds["range"]
		dx = Range["x"]
		dy = Range["y"]
		if w == None and h == None:
			raise Exception("both the width and height are missing")
		if w == None:
			h = h / (1 + 2 * pad)
			return h / dy
		if h == None:
			w = w / (1 + 2 * pad)
			return w / dx
		w = w / (1 + 2 * pad)
		h = h / (1 + 2 * pad)
		ratio = (dx / dy) / (w / h)
		if ratio < 1:
			return h / dy
		return w / dx

	def __addJavascript(self):
		text = ""
		text+= "<script id= 'script1' type='text/javascript'>\n"
		text+= "	\n"
		text+= "	var glSvgRoot = document.getElementById('svgNetView');\n"
		text+= "	var glRoot = document.getElementById('gNetView');\n"
		text+= "	var glEventState = '';\n"
		text+= "	var glEventStateTf = null;\n"
		text+= "	var glEventStateOrigin = null;\n"
		text+= "	\n"
		text+= "	window.onload = function(e) {\n"
		text+= "		document.getElementById('svgNetView').ondragstart = function() { return false; };\n"
		text+= "		var elements = document.getElementsByTagName('text');\n"
		text+= "		var max = 0;\n"
		text+= "		for (var index in elements) {\n"
		text+= "			var element = elements[index];\n"
		text+= "			var legend = GetData(element, 'legend');\n"
		text+= "			if (legend) {\n"
		text+= "				var box = element.getBBox();\n"
		text+= "				max = Math.max(max, box.width);\n"
		text+= "			}\n"
		text+= "		}\n"
		text+= "		var element = document.getElementById('rectLegend');\n"
		text+= "		if (element) {\n"
		text+= "			element.setAttribute('width', max + 30 + 7);\n"
		text+= "		}\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function SetCTM(element, m) {\n"
		text+= "		var s = 'matrix('+m.a+','+m.b+','+m.c+','+m.d+','+m.e+','+m.f+')';\n"
		text+= "		element.setAttribute('transform', s);\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function GetEventPoint(e) {\n"
		text+= "		var p = glSvgRoot.createSVGPoint();\n"
		text+= "		var offset = glSvgRoot.getBoundingClientRect();\n"
		text+= "		p.x = e.clientX;\n"
		text+= "		p.y = e.clientY;\n"
		text+= "		return p;\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function OnMouseWheel(e) {\n"
		text+= "		var e = window.event || e;\n"
		text+= "		if (e && e.target) {\n"
		text+= "			var target = e.target;\n"
		text+= "		} else if (e && e.srcElement) {\n"
		text+= "			var target = e.srcElement;\n"
		text+= "		} else {\n"
		text+= "			var target = null;\n"
		text+= "		}\n"
		text+= "		if (target && target.id == '') return;\n"
		text+= "		if (target && target.id == 'rectLegend') return;\n"
		text+= "		var delta;\n"
		text+= "		if (e.wheelDelta) {\n"
		text+= "			delta = e.wheelDelta / 3600;\n" #// Chrome/Safari"
		text+= "		} else {\n"
		text+= "			delta = e.detail / -90;\n" #// Mozilla/Firefox"
		text+= "		}\n"
		text+= "		var z = 1 + delta;\n"
		text+= "		var m = glRoot.getCTM();\n"
		text+= "		m = m.inverse();\n"
		text+= "		p = GetEventPoint(e);\n"
		text+= "		p = p.matrixTransform(m);\n"
		text+= "		var m = glSvgRoot.createSVGMatrix();\n"
		text+= "		var m = m.translate(p.x, p.y);\n"
		text+= "		var m = m.scale(z);\n"
		text+= "		var m = m.translate(-p.x, -p.y);\n"
		text+= "		var ctm = glRoot.getCTM();\n"
		text+= "		var ctm = ctm.multiply(m);\n"
		text+= "		SetCTM(glRoot, ctm);\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function OnMouseDown(e) {\n"
		text+= "		var e = window.event || e;\n"
		text+= "		if (e && e.target) {\n"
		text+= "			var target = e.target;\n"
		text+= "		} else if (e && e.srcElement) {\n"
		text+= "			var target = e.srcElement;\n"
		text+= "		} else {\n"
		text+= "			return;\n"
		text+= "		}\n"
		text+= "		if (target.id == '') return;\n"
		text+= "		if (target.id == 'rectLegend') {\n"
		text+= "			glEventState = 'pan_legend';\n"
		text+= "			var root = document.getElementById('gLegend');\n"
		text+= "			var m = root.getCTM();\n"
		text+= "		} else {\n"
		text+= "			glEventState = 'pan';\n"
		text+= "			var m = glRoot.getCTM();\n"
		text+= "		}\n"
		text+= "		glEventStateTf = m.inverse();\n"
		text+= "		var p = GetEventPoint(e);\n"
		text+= "        glEventStateOrigin = p.matrixTransform(glEventStateTf);\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function OnMouseUp(e) {\n"
		text+= "		var e = window.event || e;\n"
		text+= "		glEventState = '';\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function OnMouseMove(e) {\n"
		text+= "		var e = window.event || e;\n"
		text+= "		DoPopup(e);\n"
		text+= "		if (glEventState == 'pan' || glEventState == 'pan_legend') {\n"
		text+= "			var p = GetEventPoint(e);\n"
		text+= "			var p = p.matrixTransform(glEventStateTf);\n"
		text+= "			var x = p.x - glEventStateOrigin.x;\n"
		text+= "			var y = p.y - glEventStateOrigin.y;\n"
		text+= "			var m = glEventStateTf.inverse();\n"
		text+= "			var m = m.translate(x, y);\n"
		text+= "			if (glEventState == 'pan') {\n"
		text+= "				SetCTM(glRoot, m);\n"
		text+= "			} else {\n"
		text+= "				var root = document.getElementById('gLegend');\n"
		text+= "				SetCTM(root, m);\n"
		text+= "				\n"
		text+= "			}\n"
		text+= "		}\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function OnMouseOver(e) {\n"
		text+= "		var e = window.event || e;\n"
		text+= "		var target = GetTarget(e);\n"
		text+= "		DoLayers(e);\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function OnMouseOut(e) {\n"
		text+= "		var e = window.event || e;\n"
		text+= "		var target = GetTarget(e);\n"
		text+= "		if (target == null) return;\n"
		text+= "		var legend = GetData(target, 'legend');\n"
		text+= "		if (legend == null && target.id != 'rectLegend') {\n"
		text+= "			DoLayers2(-1);\n"
		text+= "		}\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function DoLayers(e) {\n"
		text+= "		var target = GetTarget(e);\n"
		text+= "		var legend = GetData(target, 'legend');\n"
		text+= "		if (legend) {\n"
		text+= "			var layer = GetData(target, 'layer');\n"
		text+= "			var ilayer = parseInt(layer);\n"
		text+= "			DoLayers2(ilayer);\n"
		text+= "		}\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function DoLayers2(ilayer) {\n"
		text+= "		var elements = document.getElementsByTagName('*');\n"
		text+= "		for (var index in elements) {\n"
		text+= "			var element = elements[index];\n"
		text+= "			var layer = GetData(element, 'layer');\n"
		text+= "			var legend = GetData(element, 'legend');\n"
		text+= "			if (layer && legend == null) {\n"
		text+= "				var i = parseInt(layer);\n"
		text+= "				if (i == ilayer || ilayer < 0) {\n"
		text+= "					element.setAttribute('display', 'inherit');\n"
		text+= "				} else {\n"
		text+= "					element.setAttribute('display', 'none');\n"
		text+= "				}\n"
		text+= "			}\n"
		text+= "		}\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function DoPopup(e) {\n"
		text+= "		var popup = document.getElementById('m_Popup')\n"
		text+= "		if (e && e.target) {\n"
		text+= "			var target = e.target;\n"
		text+= "		} else if (e && e.srcElement) {\n"
		text+= "			var target = e.srcElement;\n"
		text+= "		} else {\n"
		text+= "			popup.textContent = '';\n"
		text+= "			return;\n"
		text+= "		}\n"
		text+= "		var id = GetData(target, 'id');\n"
		text+= "		if (target.id == 'm_Popup') {\n"
		text+= "			popup.setAttribute('visibility','hidden');\n"
		text+= "			var behind = document.elementFromPoint(e.clientX, e.clientY);\n"
		text+= "			popup.setAttribute('visibility','inherit');\n"
		text+= "			id = GetData(behind, 'id');\n"
		text+= "		}\n"
		text+= "		if (id) {\n"
		text+= "			popup.textContent = id;\n"
		text+= "		} else {\n"
		text+= "			popup.textContent = '';\n"
		text+= "		}\n"
		text+= "		popup.setAttribute('x', e.clientX + 4);\n"
		text+= "		popup.setAttribute('y', e.clientY - 4);\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function GetTarget(e) {\n"
		text+= "		if (e && e.target    ) return e.target    ;\n"
		text+= "		if (e && e.srcElement) return e.srcElement;\n"
		text+= "		return null;\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function GetData(target, variable) {\n"
		text+= "		if (target    == null) return null;\n"
		text+= "		if (target.id == null) return null;\n"
		text+= "		if (target.dataset) return target.dataset[variable];\n"
		text+= "		return target.getAttribute('data-' + variable);\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	function addListeners() {\n"
		text+= "		var elements = document.getElementsByTagName('*');\n"
		text+= "		for (var index in elements) {\n"
		text+= "			var element = elements[index];\n"
		text+= "			if (element.addEventListener) {\n"
		text+= "				element.addEventListener('mousewheel'    , OnMouseWheel, false);\n"
		text+= "				element.addEventListener('DOMMouseScroll', OnMouseWheel, false);\n"
		text+= "				element.addEventListener('mousedown'     , OnMouseDown , false);\n"
		text+= "				element.addEventListener('mouseup'       , OnMouseUp   , false);\n"
		text+= "				element.addEventListener('mousemove'     , OnMouseMove , false);\n"
		text+= "				element.addEventListener('mouseover'     , OnMouseOver , false);\n"
		text+= "				element.addEventListener('mouseover'     , OnMouseOut  , false);\n"
		text+= "			} else if (element.attachEvent) {\n"
		text+= "				element.attachEvent('onmousewheel', OnMouseWheel);\n"
		text+= "				element.attachEvent('mousedown'   , OnMouseDown );\n"
		text+= "				element.attachEvent('mouseup'     , OnMouseUp   );\n"
		text+= "				element.attachEvent('mousemove'   , OnMouseMove );\n"
		text+= "				element.attachEvent('mouseover'   , OnMouseOver );\n"
		text+= "				element.attachEvent('mouseover'   , OnMouseOut  );\n"
		text+= "			}\n"
		text+= "		}\n"
		text+= "	}\n"
		text+= "	\n"
		text+= "	addListeners();\n"
		text+= "</script>\n"
		return text

	#
	# call after all mods have been made to create the hmtl (svg) file
	#
	def writeFile(self, output_file_name=None, width=None, height=None):
		if output_file_name == None:
			self.__output_file_name = self.getInpFileName() + ".html"
		else:
			self.__output_file_name = output_file_name
		#
		for shape in self.__Shapes:
			if shape.get("Type") == "Line":
				x = shape["x1"]
				y = shape["y1"]
				self.__updateMinMax(x, y)
				x = shape["x2"]
				y = shape["y2"]
				self.__updateMinMax(x, y)
			else:
				x = shape["x"]
				y = shape["y"]
				self.__updateMinMax(x, y)
		for text in self.__TextAt:
			x = text["x"]
			y = text["y"]
			self.__updateMinMax(x, y)
		#
		self.__updateBounds()
		InpData = self.getInpData()
		dRatio = InpData["Ratio"]
		#
		if width == None:
			width = self.getWidth()
		else:
			self.setWidth(width)
		#
		if height == None:
			height = self.getHeight()
		else:
			self.setHeight(height)
		#
		if width == None and height == None:
			if dRatio > 1: # portrait
				height = 600
			else: # landscape
				width = 600
			#
		#
		o = self.getOpacity()
		f = self.getScale()
		dPad = self.getPad()
		if f == None:
			f = self.__calculateScale(width, height, dPad)
			#
		if width == None:
			width = height / dRatio
			self.setWidth(width)
			#
		if height == None:
			height = width * dRatio
			self.setHeight(height)
		#
		bgColor = self.getBackgroundColor()
		#
		dPad1 = 1 + dPad
		dPad2 = 1 + 2 * dPad
		dPadX = dPad * InpData["Bounds"]["range"]["x"]
		dPadY = dPad * InpData["Bounds"]["range"]["y"]
		dTransformX = -InpData["Bounds"]["min"  ]["x"] + dPadX
		dTransformY = -InpData["Bounds"]["max"  ]["y"] - dPadY
		#
		lBg = InpData["Bounds"]["min"  ]["x"] - 10000 # dPadX
		tBg = InpData["Bounds"]["min"  ]["y"] - 10000 # dPadY
		wBg = InpData["Bounds"]["range"]["x"] + 20000 # 2*dPadX
		hBg = InpData["Bounds"]["range"]["y"] + 20000 # 2*dPadY
		#
		svgHtml  = "<html>"
		svgBody  = "<body bgcolor='#ffffff' style='overflow: hidden;'>"
		svgSvg	 = "<svg id='svgNetView' style='position: absolute; left: 0px; top: 0px; width: {0}px; height: {1}px'>".format(20000, 20000)
		svgBg    = "<rect id='rectNetBg' x={0} y={1} width={2} height={3} fill='{4}'></rect>".format(lBg, tBg, wBg, hBg, bgColor)
		svgView  = "<g id='gNetView' transform='rotate(180) scale({0},{1}) translate({2},{3})'>".format(-f, f, dTransformX, dTransformY)
		#
		if self.getHideBackground(): svgRect = ""
		#
		pathPipe      = "<path   data-layer=0 class='Link Pipe'      id='Pipes'     d='{0}' stroke-width={1} stroke='{2}' stroke-opacity={3} stroke-linecap='round' fill='none'></path>"
		circle        = "<circle data-layer=1 class='Node Junction'  id='Node_{0}'  data-id='{0}' data-type='Junction' transform='translate({1},{2})' r={3} fill='{4}' fill-opacity={5}></circle>"
		pathTank      = "<path   data-layer=4 class='Node Tank'      id='Node_{0}'  data-id='{0}' data-type='Tank'      d='{1}' stroke='none' fill='{2}' fill-opacity={3}></path>"
		pathReservoir = "<path   data-layer=5 class='Node Reservoir' id='Node_{0}'  data-id='{0}' data-type='Reservoir' d='{1}' stroke='none' fill='{2}' fill-opacity={3}></path>"
		pathPump      = "<path   data-layer=2 class='Link Pump'      id='Pump_{0}'  data-id='{0}' data-type='Pump'      d='{1}' stroke='none' fill='{2}' fill-opacity={3}></path>"
		pathValve     = "<path   data-layer=3 class='Link Valve'     id='Valve_{0}' data-id='{0}' data-type='Valve'     d='{1}' stroke='none' fill='{2}' fill-opacity={3}></path>"
		pathShape     = "<path   class='Shape {0}'      id='Shape_{1}' data-id='{2}' data-type='{0}'       d='{3}' stroke='{4}' stroke-width={5} stroke-opacity={6} fill='{7}' fill-opacity={8} transform='rotate({9} {10} {11})' data-layer='{12}'></path>"
		line          = "<line   class='Shape Line {0}' id='Shape_{1}' data-id='{2}' data-type='{0}' x1='{3}' y1='{4}' x2='{5}' y2='{6}' stroke-width='{7}' stroke='{8}' stroke-opacity={9} stroke-linecap='round' data-layer='{10}'></line>"
		text          = "<text   class='Text' x=0 y=0 transform='translate({0},{1}) scale({2},{3}) rotate({4})' fill='{5}' fill-opacity={6} font-family='Tahoma,Consolas,Lucida Console,Verdana,sans-serif'>{7}</text>"
		#
		svgJunctions  = ""
		svgTanks      = ""
		svgReservoirs = ""
		svgPipes      = ""
		svgPumps      = ""
		svgValves     = ""
		svgShapes     = ""
		svgLines      = ""
		svgText       = ""
		#
		svgPopup      = "<text id='m_Popup' x=0 y=0 style='{0}'>TESTING123</text>".format(self.__sFontStyle)
		#
		Nodes = InpData["Nodes"]
		for id in Nodes:
			node = Nodes[id]
			x = node.get("x", 0)
			y = node.get("y", 0)
			if node["Type"].upper() == "JUNCTION":
				if not self.__bHideNodes:
					if not self.__bHideJunctions:
						s = self.getJunctionSize()
						c = self.getJunctionColor()
						o = self.getJunctionOpacity()						
						svgJunctions += circle.format(id, x, y, 0.5 * s / f, c, o)
			elif node["Type"].upper() == "TANK":
				if not self.__bHideNodes:
					if not self.__bHideTanks:
						s = self.getTankSize()
						c = self.getTankColor()
						o = self.getTankOpacity()
						svgTanks += pathTank.format(id, self.__getTankPath(x, y, 0.5 * s / f), c, o)
			elif node["Type"].upper() == "RESERVOIR":
				if not self.__bHideNodes: 
					if not self.__bHideReservoirs: 
						s = self.getReservoirSize()
						c = self.getReservoirColor()
						o = self.getReservoirOpacity()
						svgReservoirs += pathReservoir.format(id, self.__getReservoirPath(x, y, 0.5 * s / f), c, o)
		#
		d = "M 0 0 "
		Links = InpData["Links"]
		for link in Links:
			if link["Type"].upper() == "PIPE":
				if not self.__bHideLinks:
					if not self.__bHidePipes:
						d += " M " + str(link["x1"]) + " " + str(link["y1"])
						for vert in link["Vertices"]:
							d += " L " + str(vert["x"]) + " " + str(vert["y"])
						d += " L " + str(link["x2"]) + " " + str(link["y2"])
			elif link["Type"].upper() == "PUMP":
				if not self.__bHideLinks:
					if not self.__bHidePumps:
						s = self.getPumpSize()
						c = self.getPumpColor()
						o = self.getPumpOpacity()
						svgPumps += pathPump.format(link["ID"], self.__getPumpPath(link["x1"], link["y1"], link["x2"], link["y2"], s / f), c, o)
			elif link["Type"].upper() == "VALVE":
				if not self.__bHideLinks:
					if not self.__bHideValves:
						s = self.getValveSize()
						c = self.getValveColor()
						o = self.getValveOpacity()
						svgValves += pathValve.format(link["ID"], self.__getValvePath(link["x1"], link["y1"], link["x2"], link["y2"], s / f), c, o)
		s = self.getPipeSize()
		c = self.getPipeColor()
		o = self.getPipeOpacity()
		svgPipes = pathPipe.format(d, s / f, c, o)
		#
		if not self.__bHideShapes:
			for shape in self.__Shapes:
				sType = shape.get("Type")
				if sType == "Line":
					width   = shape.get("width"  , 1      ) / f
					color   = shape.get("color"  , "black")
					opacity = shape.get("opacity", 1      )
					layer   = shape.get("layer"  , "")
					svgShapes += line.format(shape["Type"], shape["Name"], shape["id"], shape["x1"], shape["y1"], shape["x2"], shape["y2"], width, color, opacity, layer)
				else:
					x  = shape.get("x" , 0)
					y  = shape.get("y" , 0)
					sc = shape.get("sc", "#ffffff") # stroke color
					fs = shape.get("fs", self.getNodeSize()) # fill size
					sw = shape.get("sw", self.getLinkSize()) # line size
					so = shape.get("so", o) # stroke opacity
					fc = shape.get("fc", "#ffffff") # fill color
					fo = shape.get("fo", o) # fill opacity
					a  = shape.get("a" , 0) # rotation angle
					i  = shape.get("layer", "") # layer number
					#
					path = self.__getPath(shape["Type"], x, y, fs / f)
					#
					svgShapes += pathShape.format(shape["Type"], shape["Name"], shape["id"], path, sc, sw / f, so, fc, fo, -1 * a, x, y, i)
		#
		for t in self.__TextAt:
			size    = t.get("size"   , 1      ) / f
			angle   = t.get("angle"  , 0      )
			color   = t.get("color"  , "black")
			opacity = t.get("opacity", 1      )
			sText   = t.get("text"   , ""     )
			svgText += text.format(t["x"], t["y"], size, -size, angle, color, opacity, sText)
		#
		layer = self.getLayerByLabel("Pipes")
		if self.getLegendPipes():
			layer["type"        ] = self.LAYER_TYPE_LINK
			layer["shape"       ] = "pipe"
			layer["line color"  ] = self.getPipeColor()
			layer["line opacity"] = self.getPipeOpacity()
		else:
			layer["hide"] = True
		layer = self.getLayerByLabel("Pumps")
		if self.getLegendPumps():
			layer["type"        ] = self.LAYER_TYPE_LINK
			layer["shape"       ] = "pump"
			layer["fill color"  ] = self.getPumpColor()
			layer["fill opacity"] = self.getPumpOpacity()
		else:
			layer["hide"] = True
		layer = self.getLayerByLabel("Valves")
		if self.getLegendValves():
			layer["type"        ] = self.LAYER_TYPE_LINK
			layer["shape"       ] = "valve"
			layer["fill color"  ] = self.getValveColor()
			layer["fill opacity"] = self.getValveOpacity()
		else:
			layer["hide"] = True
		layer = self.getLayerByLabel("Junctions")
		if self.getLegendJunctions():
			layer["type"        ] = self.LAYER_TYPE_NODE
			layer["shape"       ] = "circle"
			layer["fill color"  ] = self.getJunctionColor()
			layer["fill opacity"] = self.getJunctionOpacity()
		else:
			layer["hide"] = True
		layer = self.getLayerByLabel("Tanks")
		if self.getLegendTanks():
			layer["type"        ] = self.LAYER_TYPE_NODE
			layer["shape"       ] = "tank"
			layer["size"        ] = 7
			layer["fill color"  ] = self.getTankColor()
			layer["fill opacity"] = self.getTankOpacity()
		else:
			layer["hide"] = True
		layer = self.getLayerByLabel("Reservoirs")
		if self.getLegendReservoirs():
			layer["type"        ] = self.LAYER_TYPE_NODE
			layer["shape"       ] = "reservoir"
			layer["size"        ] = 7
			layer["fill color"  ] = self.getReservoirColor()
			layer["fill opacity"] = self.getReservoirOpacity()
		else:
			layer["hide"] = True
		#
		layer_count = 0
		for layer in self.__Layers:
			if layer.get("hide"): continue
			layer_count += 1
		#
		svgLegend = ""
		if self.__bShowLegend:
			x = 30
			h = 20
			svgLegend += " <g"
			svgLegend += " id='gLegend'"
			svgLegend += " style='position: absolute; {0}'".format(self.__sFontStyle)
			svgLegend += " transform='translate({0},{1}) scale({2})'".format(self.getLegendX(), self.getLegendY(), self.getLegendScale())
			svgLegend += " >"
			svgLegend += " <rect"
			svgLegend += " id='rectLegend'"
			svgLegend += " width={0}".format(200)
			svgLegend += " height={0}".format(h * (layer_count + 0.5))
			svgLegend += " fill={0}".format(self.getLegendColor())
			svgLegend += " cursor='move'"
			svgLegend += " >"
			svgLegend += " </rect>"
			y = 0
			for i in range(0, len(self.__Layers)):
				layer = self.__Layers[i]
				if layer.get("hide"): continue
				size = layer.get("size", 10)
				y += h
				svgLegend += " <text "
				svgLegend += " x={0} y={1}".format(x, y)
				svgLegend += " data-legend=true"
				svgLegend += " data-layer=" + str(i)
				svgLegend += " >"
				svgLegend += " {0}".format(layer["label"])
				svgLegend += " </text>"
				if layer.get("type") == self.LAYER_TYPE_NODE:
					shape = layer.get("shape", "circle")
					shape = self.correctShape(shape)
					path  = self.__getPath(shape, 0.5 * x, y - 4, size)
					svgLegend += " <path "
					svgLegend += " transform='rotate({0} {1} {2})'".format(180, 0.5 * x, y - 4)
					svgLegend += " d='{0}'                        ".format(path)
					svgLegend += " fill='{0}'                     ".format(layer.get("fill color"  , "none"))
					svgLegend += " fill-opacity='{0}'             ".format(layer.get("fill opacity", 0     ))
					svgLegend += " stroke='{0}'                   ".format(layer.get("line color"  , "none"))
					svgLegend += " stroke-width='{0}'             ".format(layer.get("line size"   , 0     ))
					svgLegend += " stroke-opacity='{0}'           ".format(layer.get("line opacity", 0     ))
					svgLegend += " data-legend='true'"
					svgLegend += " data-layer=" + str(i)
					svgLegend += " >"
					svgLegend += " </path>"
				elif layer.get("type") == self.LAYER_TYPE_LINK:
 					# reverse y in getPath because scale = -1 to make pump point right direction
					shape = layer.get("shape", "line")
					shape = self.correctShape(shape)
					path  = self.__getPath(shape, x - 4, -(y - 4), size, 0 + 4)
					svgLegend += " <path "
					svgLegend += " transform='scale(1,-1)'"
					svgLegend += " d='{0}'                ".format(path)
					svgLegend += " fill='{0}'             ".format(layer.get("fill color"  , "none"))
					svgLegend += " fill-opacity={0}       ".format(layer.get("fill opacity", 0     ))
					svgLegend += " stroke='{0}'           ".format(layer.get("line color"  , "none"))
					svgLegend += " stroke-opacity={0}     ".format(layer.get("line opacity", 0     ))
					svgLegend += " stroke-width={0}       ".format(2)
					svgLegend += " stroke-linecap='round' "
					svgLegend += " data-legend='true'"
					svgLegend += " data-layer=" + str(i)
					svgLegend += " >"
					svgLegend += " </path>"
			svgLegend += " </g>"
		#
		svgFile = ""
		svgFile += svgHtml       ##############################
		svgFile += svgBody       ##########################
		svgFile += svgSvg        ######################
		svgFile += svgBg
		svgFile += svgView       ##################
		svgFile += svgPipes
		svgFile += svgJunctions
		svgFile += svgPumps
		svgFile += svgValves
		svgFile += svgTanks
		svgFile += svgReservoirs
		svgFile += svgShapes
		svgFile += svgLines
		svgFile += svgText
		svgFile += "</g>"        ##################
		svgFile += svgLegend
		svgFile += svgPopup
		svgFile += "</svg>"      ######################
		svgFile += "</body>"     ##########################
		svgFile += "</html>"     ##############################
		svgFile += self.__addJavascript()
		#
		if os.path.exists(self.__output_file_name):
			pass
		f = open(self.__output_file_name, "w")
		f.write(svgFile)

	def __getPath(self, shape, x, y, r, x2=None, y2=None):
		if x2 == None: x2 = x
		if y2 == None: y2 = y
		if shape == self.SHAPE_CIRCLE   : return self.__getCirclePath   (x, y, r)
		if shape == self.SHAPE_SQUARE   : return self.__getSquarePath   (x, y, r)
		if shape == self.SHAPE_TRIANGLE : return self.__getTrianglePath (x, y, r)
		if shape == self.SHAPE_DIAMOND  : return self.__getDiamondPath  (x, y, r)
		if shape == self.SHAPE_PLUS     : return self.__getPlusPath     (x, y, r)
		if shape == self.SHAPE_X        : return self.__getXPath        (x, y, r)
		if shape == self.SHAPE_RESERVOIR: return self.__getReservoirPath(x, y, r)
		if shape == self.SHAPE_TANK     : return self.__getTankPath     (x, y, r)
		if shape == self.SHAPE_JUNCTION : return self.__getJunctionPath (x, y, r)
		if shape == self.SHAPE_PUMP     : return self.__getPumpPath     (x, y, x2, y2, r)
		if shape == self.SHAPE_VALVE    : return self.__getValvePath    (x, y, x2, y2, r)
		if shape == self.SHAPE_PIPE     : return self.__getPipePath     (x, y, x2, y2, r)
		if shape == self.SHAPE_LINE     : return self.__getLinePath     (x, y, x2, y2, r)
		return "M 0 0"

	def __getCirclePath(self, x, y, s):
		r = 0.5 * s
		s1 = 0.9
		s1 = 0.707
		path = ""
		path += " M " + str(x-s1*r) + " " + str(y-s1*r)
		path += " A " + str(0+s1*r) + " " + str(0+s1*r) + " 0 1 1 " + str(x+s1*r) + " " + str(y+s1*r)
		path += " A " + str(0+s1*r) + " " + str(0+s1*r) + " 0 1 1 " + str(x-s1*r) + " " + str(y-s1*r)
		return path

	def __getSquarePath(self, x, y, s):
		r = 0.5 * s
		s1 = 1.2
		s2 = s1 * 2
		path = ""
		path += " M " + str(x-s1*r) + " " + str(y-0.*r)
		path += " l " + str(0+0.*r) + " " + str(0+s1*r)
		path += " l " + str(0+s2*r) + " " + str(0+0.*r)
		path += " l " + str(0+0.*r) + " " + str(0-s2*r)
		path += " l " + str(0-s2*r) + " " + str(0+0.*r)
		path += " l " + str(0+0.*r) + " " + str(0+s1*r)
		return path

	def __getTrianglePath(self, x, y, s):
		r = 0.5 * s
		s1 = 1.7
		s2 = s1 * 1.91
		yo = 0.6 * r
		xo1 = 0.175 * r
		xo2 = xo1 * 2
		path = ""
		path += " M " + str(x+0.*r+0.0) + " " + str(y-s1*r+yo)
		path += " l " + str(0+s1*r+xo1) + " " + str(0+0.*r+0.)
		path += " l " + str(0-s1*r-xo1) + " " + str(0+s2*r+0.)
		path += " l " + str(0-s1*r-xo1) + " " + str(0-s2*r+0.)
		path += " l " + str(0+s1*r+xo1) + " " + str(0+0.*r+0.)
		return path

	def __getDiamondPath(self, x, y, s):
		r = 0.5 * s
		sx1 = 0.7
		sx2 = sx1 * 2
		sy1 = 0.9
		sy2 = sy1 * 2
		path = ""
		path += " M " + str(x-sx1*r) + " " + str(y-sy1*r)
		path += " l " + str(0+sx1*r) + " " + str(0-sy1*r)
		path += " l " + str(0+sx2*r) + " " + str(0+sy2*r)
		path += " l " + str(0-sx2*r) + " " + str(0+sy2*r)
		path += " l " + str(0-sx2*r) + " " + str(0-sy2*r)
		path += " l " + str(0+sx1*r) + " " + str(0-sy1*r)
		return path

	def __getPlusPath(self, x, y, s):
		r = 0.5 * s
		s1 = 1.4
		s2 = s1 * 2
		path = ""
		path += " M " + str(x-s1*r) + " " + str(y+0.*r)
		path += " l " + str(0+s2*r) + " " + str(0+0.*r)
		path += " M " + str(x+0.*r) + " " + str(y-s1*r)
		path += " l " + str(0+0.*r) + " " + str(0+s2*r)
		return path

	def __getXPath(self, x, y, s):
		r = 0.5 * s
		s1 = 1.1
		s2 = s1 * 2
		path = ""
		path += " M " + str(x-s1*r) + " " + str(y-s1*r)
		path += " l " + str(0+s2*r) + " " + str(0+s2*r)
		path += " M " + str(x-s1*r) + " " + str(y+s1*r)
		path += " l " + str(0+s2*r) + " " + str(0-s2*r)
		return path

	def __getTankPath(self, x, y, r):
		if self.__bUseEpanetIcons: return self.__getTankPathEpanet(x, y, r)
		path = ""
		path += " M " + str(x-0.7*r) + " " + str(y-1.0*r)
		path += " l " + str(0-0.0*r) + " " + str(0+2.0*r)
		path += " l " + str(0+0.1*r) + " " + str(0+0.0*r)
		path += " l " + str(0+0.2*r) + " " + str(0-0.2*r)
		path += " l " + str(0+0.2*r) + " " + str(0+0.2*r)
		path += " l " + str(0+0.2*r) + " " + str(0-0.2*r)
		path += " l " + str(0+0.2*r) + " " + str(0+0.2*r)
		path += " l " + str(0+0.2*r) + " " + str(0-0.2*r)
		path += " l " + str(0+0.2*r) + " " + str(0+0.2*r)
		path += " l " + str(0+0.1*r) + " " + str(0+0.0*r)
		path += " l " + str(0-0.0*r) + " " + str(0-2.0*r)
		path += " a " + str(      r) + " " + str(      r) + " 0 0 0 " + str(0-1.4*r) + " " + str(0+0.0*r)
		return path

	def __getReservoirPath(self, x, y, r):
		if self.__bUseEpanetIcons: return self.__getReservoirPathEpanet(x, y, r)
		path = ""
		path += " M " + str(x+0.0*r) + " " + str(y-0.6*r)
		path += " A " + str(  0.5*r) + " " + str(  0.5*r) + " 0 1 1 " + str(x+0.57*r) + " " + str(y-0.19*r)
		path += " A " + str(  0.5*r) + " " + str(  0.5*r) + " 0 1 1 " + str(x+0.35*r) + " " + str(y+0.49*r)
		path += " A " + str(  0.5*r) + " " + str(  0.5*r) + " 0 1 1 " + str(x-0.35*r) + " " + str(y+0.49*r)
		path += " A " + str(  0.5*r) + " " + str(  0.5*r) + " 0 1 1 " + str(x-0.57*r) + " " + str(y-0.19*r)
		path += " A " + str(  0.5*r) + " " + str(  0.5*r) + " 0 1 1 " + str(x+0.00*r) + " " + str(y-0.60*r)
		return path

	def __getPumpPath(self, x1, y1, x2, y2, _r):
		#if self.__bUseEpanetIcons: return self.__getPumpPathEpanet(x1, y1, x2, y2, _r)
		cx = 0.5 * (x1 + x2)
		cy = 0.5 * (y1 + y2)
		lx = x2 - x1
		ly = y2 - y1
		l = math.sqrt(lx * lx + ly * ly)
		w = 0.1 * _r
		n = 1.2 * w
		try:
			nx = lx * n / l
			ny = ly * n / l
		except:
			nx = 0
			ny = 0
		r = 3 * n;
		path = ""
		path += " M " + str(cx - r) + " " + str(cy)
		path += " a " + str(     r) + " " + str( r) + " 0 0 0 " + str(+r) + " " + str(+r)
		path += " h " + str( 1.60 * r)
		path += " v " + str(-0.80 * r)
		path += " h " + str(-0.60 * r)
		path += " v " + str(-0.20 * r)
		path += " a " + str(r) + " " + str(r) + " 0 0 0 " + str(-0.5*r) + " " + str(-0.87*r)
		path += " l " + str( 0.8 * r) + " " + str(-0.38*r)
		path += " v " + str(-0.2 * r)
		path += " h " + str(-2.6 * r)
		path += " v " + str( 0.2 * r)
		path += " l " + str( 0.8 * r) + " " + str( 0.38*r)
		path += " a " + str(r) + " " + str(r) + " 0 0 0 " + str(-0.5*r) + " " + str(+0.87*r)
		path += " M " + str(cx   ) + " " + str(cy   )
		path += " L " + str(x1+ny) + " " + str(y1-nx)
		path += " A " + str(n/2)   + " " + str(n/2)   + " 0 0 0 " + str(x1-ny) + " " + str(y1+nx)
		path += " L " + str(cx   ) + " " + str(cy   )
		path += " L " + str(x2+ny) + " " + str(y2-nx)
		path += " A " + str(n/2)   + " " + str(n/2)   + " 0 0 1 " + str(x2-ny) + " " + str(y2+nx)
		path += " L " + str(cx   ) + " " + str(cy   )
		return path

	def __getValvePath(self, x1, y1, x2, y2, _r):
		#if self.__bUseEpanetIcons: return self.__getValvePathEpanet(x1, y1, x2, y2, _r)
		w = 0.1 * _r
		lx = x2 - x1
		ly = y2 - y1
		l = math.sqrt(lx * lx + ly * ly)
		n = 4 * w
		try:
			nx = lx * n / l
			ny = ly * n / l
		except:
			nx = 0
			ny = 0
		cx = 0.5 * (x1 + x2)
		cy = 0.5 * (y1 + y2)
		path = ""
		path += " M " + str(cx+0.0*nx+0.2*ny) + " " + str(cy+0.0*ny-0.2*nx)
		path += " L " + str(cx+1.0*nx+1.0*ny) + " " + str(cy+1.0*ny-1.0*nx)
		path += " L " + str(cx+1.4*nx+1.0*ny) + " " + str(cy+1.4*ny-1.0*nx)
		path += " L " + str(cx+1.4*nx+0.1*ny) + " " + str(cy+1.4*ny-0.1*nx)
		path += " L " + str(x2+0.0*nx+0.3*ny) + " " + str(y2+0.0*ny-0.3*nx)
		path += " A " + str(0.3*n) + " " + str(0.3*n) + " 0 0 1 " + str(x2+0.0*nx-0.3*ny) + " " + str(y2+0.0*ny+0.3*nx)
		path += " L " + str(cx+1.4*nx-0.1*ny) + " " + str(cy+1.4*ny+0.1*nx)
		path += " L " + str(cx+1.4*nx-1.0*ny) + " " + str(cy+1.4*ny+1.0*nx)
		path += " L " + str(cx+1.0*nx-1.0*ny) + " " + str(cy+1.0*ny+1.0*nx)
		path += " L " + str(cx+0.0*nx-0.2*ny) + " " + str(cy+0.0*ny+0.2*nx)
		path += " L " + str(cx-0.0*nx-0.2*ny) + " " + str(cy-0.0*ny+0.2*nx)
		path += " L " + str(cx-1.0*nx-1.0*ny) + " " + str(cy-1.0*ny+1.0*nx)
		path += " L " + str(cx-1.4*nx-1.0*ny) + " " + str(cy-1.4*ny+1.0*nx)
		path += " L " + str(cx-1.4*nx-0.1*ny) + " " + str(cy-1.4*ny+0.1*nx)
		path += " L " + str(x1-0.0*nx-0.3*ny) + " " + str(y1-0.0*ny+0.3*nx)
		path += " A " + str(0.3*n) + " " + str(0.3*n) + " 0 1 1 " + str(x1-0.0*nx+0.3*ny) + " " + str(y1-0.0*ny-0.3*nx)
		path += " L " + str(cx-1.4*nx+0.1*ny) + " " + str(cy-1.4*ny-0.1*nx)
		path += " L " + str(cx-1.4*nx+1.0*ny) + " " + str(cy-1.4*ny-1.0*nx)
		path += " L " + str(cx-1.0*nx+1.0*ny) + " " + str(cy-1.0*ny-1.0*nx)
		path += " L " + str(cx-0.0*nx+0.2*ny) + " " + str(cy-0.0*ny-0.2*nx)
		path += " Z "
		return path

	def __getPipePath(self, x1, y1, x2, y2, r):
		path = ""
		path += " M " + str(x1) + " " + str(y1)
		path += " L " + str(x2) + " " + str(y2)
		return path

	def __getLinePath(self, x1, y1, x2, y2, r):
		path = ""
		path += " M " + str(x1) + " " + str(y1)
		path += " L " + str(x2) + " " + str(y2)
		return path

	def __getTankPathEpanet(self, x, y, r):
		path = ""
		path += " M " + str(x+0.0*r) + " " + str(y+1.0*r)
		path += " l " + str(0+1.0*r) + " " + str(0+0.0*r)
		path += " l " + str(0+0.0*r) + " " + str(0-1.0*r)
		path += " l " + str(0-0.6*r) + " " + str(0+0.0*r)
		path += " l " + str(0+0.0*r) + " " + str(0-1.0*r)
		path += " l " + str(0-0.8*r) + " " + str(0+0.0*r)
		path += " l " + str(0+0.0*r) + " " + str(0+1.0*r)
		path += " l " + str(0-0.6*r) + " " + str(0+0.0*r)
		path += " l " + str(0+0.0*r) + " " + str(0+1.0*r)
		path += " l " + str(0+1.0*r) + " " + str(0+0.0*r)
		return path

	def __getReservoirPathEpanet(self, x, y, r):
		path = ""
		path += " M " + str(x+0.0*r) + " " + str(y-0.9*r)
		path += " l " + str(0+1.0*r) + " " + str(0+0.0*r)
		path += " l " + str(0+0.0*r) + " " + str(0+1.8*r)
		path += " l " + str(0-0.2*r) + " " + str(0+0.0*r)
		path += " l " + str(0+0.0*r) + " " + str(0-0.6*r)
		path += " l " + str(0-1.6*r) + " " + str(0+0.0*r)
		path += " l " + str(0+0.0*r) + " " + str(0+0.6*r)
		path += " l " + str(0-0.2*r) + " " + str(0+0.0*r)
		path += " l " + str(0+0.0*r) + " " + str(0-1.8*r)
		path += " l " + str(0+1.0*r) + " " + str(0+0.0*r)
		return path

	def __getPumpPathEpanet(self, x1, y1, x2, y2, _r):
		return "M 0 0"

	def __getValvePathEpanet(self, x1, y1, x2, y2, _r):
		return "M 0 0"

class BadNodeException(Exception):
	pass
class BadLinkException(Exception):
	pass

def isNotNumeric(value):
    return not isNumeric(value)
def isNumeric(value):
    return (type(value) == float or type(value) == int)
def isNotString(value):
    return not isString(value)
def isString(value):
    return (type(value) == str or type(value) == unicode)













