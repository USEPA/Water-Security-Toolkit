#
# this python script is a sample of how to use the inp2svg visualization class to create an svg (html file) of an INP file.
# the resulting svg can be panned and zoomed using the mouse and the mouse wheel (or track pad).
# this file can be opened in the Chrome web bowser and then you can screen capture the network. 
#

import inp2svg

sInp = "../../../../examples/networks/Net1.inp"
svg = inp2svg.inp2svg(sInp)

# the pad is the percent of the vertical or horizontal ranges that is added to each side.
# the top and bottom each get that percentage of the vertical range and 
# the left and right sides get that percentage of the horizontal range.
# (value cannot be less than zero)

svg.setPad(0.10)

# the background color can be set using color names like 'black', 'white', 'gray'
# or the hexidecimal notation shown below (the default is '#666666') which is just
# a darker shade of gray than '#888888'

svg.setBackgroundColor("#888888")

#
# the radius of the junctions is calculated by default
# but it can be over-ridden
# the units of the radius are based on the units of the inp file coordinates
#

size = svg.getNodeSize()
svg.setNodeSize(size * 2)

#
# the stroke width of the pipe lines is calculated by default
# but it can be over-ridden
# the units of the width are based on the units of the inp file coordinates
#

size = svg.getLinkSize()
svg.setLinkSize(size * 2)

#
# the opacity by default is the same for both nodes and links but the user can
# set them independantly if they want
#

o = svg.getOpacity()
svg.setOpacity(o)

o = svg.getNodeOpacity()
svg.setNodeOpacity(o)

o = svg.getLinkOpacity()
svg.setLinkOpacity(o)

#
# they can also set the colors independantly if they dont like the defaults
# Reservoirs = "#009900", Tanks = "#000099", Junctions = "#000000"
# Pumps = "#999900", Valves = "#009999", Pipes ="#0000"
#

color = svg.getNodeColor()
svg.setNodeColor(color)

color = svg.getLinkColor()
svg.setLinkColor(color)

#
# Or you can set the size, color, and opacity of 
# Reservoir, Tank, Junction, Pump, Valve, Pipe independently
#

size = svg.getReservoirSize()
svg.setReservoirSize(size)

size = svg.getTankSize()
svg.setTankSize(size)

size = svg.getJunctionSize()
svg.setJunctionSize(size)

size = svg.getPumpSize()
svg.setPumpSize(size)

size = svg.getValveSize()
svg.setValveSize(size)

size = svg.getPipeSize()
svg.setPipeSize(size)

color = svg.getReservoirColor()
svg.setReservoirColor(color)

color = svg.getTankColor()
svg.setTankColor(color)

color = svg.getJunctionColor()
svg.setJunctionColor(color)

color = svg.getPumpColor()
svg.setPumpColor(color)

color = svg.getValveColor()
svg.setValveColor(color)

color = svg.getPipeColor()
svg.setPipeColor(color)

opacity = svg.getReservoirOpacity()
svg.setReservoirOpacity(opacity)

opacity = svg.getTankOpacity()
svg.setTankOpacity(opacity)

opacity = svg.getJunctionOpacity()
svg.setJunctionOpacity(opacity)

opacity = svg.getPumpOpacity()
svg.setPumpOpacity(opacity)

opacity = svg.getValveOpacity()
svg.setValveOpacity(opacity)

opacity = svg.getPipeOpacity()
svg.setPipeOpacity(opacity)

# ----- Usefull functions: --------------------------------------
#
svg.useEpanetIcons(value=True)
#
#		switch between dinesti type icons for the Tanks and Reservoirs and EPANET type icons
#
# svg.hideBackground(value=True)
#
#		hide the background rectangle
#
# svg.hideNodes(value=True)
#
#		hide (or show) all nodes (junctions, tanks, reservoirs)
#
# svg.hideLinks(value=True)
#
#		hide (or show) all links (pipes, pumps, valves)
#
# svg.hideJunctions(value=True)
#
#		hide (or show) all Junctions
#
# svg.hideTanks(value=True)
#
#		hide (or show) all tanks
#
# svg.hideReservoirs(value=True)
#
#		hide (or show) all reservoirs
#
# svg.hidePipes(value=True)
#
#		hide (or show) all pipes
#
# svg.hidePumps(value=True)
#
#		hide (or show) all pumps
#
# svg.hideValves(value=True)
#
#		hide (or show) all valves
#
# svg.hideShapes(value=True)
#
#		hide (or show) all user-defined shapes
#

#
# Adding things to the network
#

# svg.addCircleOn(node, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add a circlular shape on top of a Junction, Tank, or Reservoir
#
# svg.addSquareOn(node, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add a square shape on top of a Junction, Tank, or Reservoir
#
# svg.addTriangleOn(node, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add a triangular shape on top of a Junction, Tank, or Reservoir
#
# svg.addDiamondOn(node, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add a diamond shape on top of a Junction, Tank, or Reservoir
#
# svg.addPlusOn(node, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add a plus sign shape on top of a Junction, Tank, or Reservoir
#
# svg.addXOn(node, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add an x shape on top of a Junction, Tank, or Reservoir
#
# svg.addShapeOn(sType, node, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		sType = string, shape type (circle, square, diamond, triangle, plus, x)
#		node  = string, node id
#
# svg.addCircleAt(x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add a circle at x,y
#
# svg.addSquareAt(x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add a square at x,y
#
# svg.addTriangleAt(x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add a triangle at x,y
#
# svg.addDiamondAt(x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add a diamond at x,y
#
# svg.addPlusAt(x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add a plus sign at x,y
#
# svg.addXAt(x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		add an X at x,y
#
# svg.addShapeAt(sType, x, y, name=None, id=None, fs=None, sc=None, sw=None, so=None, fc=None, fo=None, a=None)
#
#		sType = string, shape type (circle, square, diamond, triangle, plus, x)
#		x     = float, horizontal position from bottom left
#		y     = float, vertical position from bottom left
#		name  = string, html element unique string identifier
#		id    = string, displayed when the mouse hovers over this shape
#		fs    = float, fill size (diameter of icon drawn)
#		sc    = string, stroke color
#		sw    = float, relative stroke width (relative to pipe line width)
#		so    = float, stroke opacity (0 - 1)
#		fc    = string, fill color
#		fo    = float, fill opacity (0 - 1)
#		a     = float, angle in degrees (rotates shape clockwise)
#
# svg.addJunction(x, y, name=None)
#
#		x     = float, horizontal position from bottom left
#		y     = float, vertical position from bottom left
#		name  = string, unique string used identify the object in the xml schema
#
# svg.addTank(self, x, y, name=None)
#
#		x     = float, horizontal position from bottom left
#		y     = float, vertical position from bottom left
#		name  = string, unique string used identify the object in the xml schema
#
# svg.addReservoir(self, x, y, name=None)
#
#		x     = float, horizontal position from bottom left
#		y     = float, vertical position from bottom left
#		name  = string, unique string used identify the object in the xml schema
#
# svg.addNode(self, x, y, sType, name=None)
#
#		x     = float, horizontal position from bottom left
#		y     = float, vertical position from bottom left
#		sType = string, node type (Junction, Tank, Reservoir) not case-sensative
#		name  = string, unique string used identify the object in the xml schema
#
# svg.addText(self, x, y, text, size=None, color=None, angle=None, opacity=None)
#
#		x       = float, horizontal position from bottom left (of 0,0 to bottom left of textbox)
#		y       = float, vertical   position from bottom left (of 0,0 to bottom left of textbox)
#		text    = string, the text to display on the network
#		size    = float, diameter of icon drawn
#		color   = string, name of color or helxidecimal notation
#		angle   = float, rotation angle clockwise from 12 o'clock
#		opacity = zero is invisible. one is completely visible.
#
# svg.addLine(self, x1=None, y1=None, x2=None, y2=None, name=None, id=None, width=None, color=None, opacity=None)
#
#		x1        = float, first end point horizontal location
#		y1        = float, first end point vertical location
#		x2        = second end point horizontal location
#		y2        = second end point vertical location
#		name      = string, html element unique string identifier
#		id        = string, displayed when the mouse hovers over this shape
#		width     = width of the line relative to the pipe lines
#		color     = string, name of color or helxidecimal notation
#		opacity   = zero is invisible. one is completely visible.
#
# svg.addLineBetweenNodes(self, node1, node2, width=None, color=None, opacity=None)
#
#		node1     = location of first end point
#		node2     = location of second end point
#		width     = width of the line relative to the pipe lines
#		color     = string, name of color or helxidecimal notation
#		opacity   = zero is invisible. one is completely visible.
#
# svg.addLineFromNode(self, node, x2, y2, width=None, color=None, opacity=None)
#
#		node      = location of first end point
#		x2        = second end point horizontal location
#		y2        = second end point vertical location
#		width     = width of the line relative to the pipe lines
#		color     = string, name of color or helxidecimal notation
#		opacity   = zero is invisible. one is completely visible.
#
# svg.addLineToNode(self, x1, y1, node, width=None, color=None, opacity=None)
#
#		x1        = first end point horizontal location
#		y1        = first end point vertical location
#		node      = location of second end point
#		width     = width of the line relative to the pipe lines
#		color     = string, name of color or helxidecimal notation
#		opacity   = zero is invisible. one is completely visible.
#
# svg.addLineOnLink(self, link_name, width=None, color=None, opacity=None)
#
#		link_name = the name from the INP file of the pipe, pump, or valve to draw on
#		width     = width of the line relative to the pipe lines
#		color     = string, name of color or helxidecimal notation
#		opacity   = zero is invisible. one is completely visible.
#


# add 6 new junctions and then add a red shape of each type on top of each new junction.
# the .addJunction() method returns the Node object so we can get the new nodes auto-generated name
# and then use that name to specify which node to add the shape to.

node = svg.addJunction(-10, 50)
svg.addShapeOn("Circle",   node["id"], sc="red", sw=2, fo=0)
node = svg.addJunction(-10, 40)
svg.addShapeOn("Square",   node["id"], sc="red", sw=2, fo=0)
node = svg.addJunction(-10, 30)
svg.addShapeOn("Triangle", node["id"], sc="red", sw=2, fo=0)
node = svg.addJunction(-10, 20)
svg.addShapeOn("Diamond",  node["id"], sc="red", sw=2, fo=0)
node = svg.addJunction(-10, 10)
svg.addShapeOn("Plus",     node["id"], sc="red", sw=2, fo=0)
node = svg.addJunction(-10,  0)
svg.addShapeOn("X",        node["id"], sc="red", sw=2, fo=0)

# add 6 new tanks and then add a red shape of each type on top of each new tank

node = svg.addTank(0, 50)
svg.addShapeOn("Circle",   node["id"], sc="red", sw=2, fo=0)
node = svg.addTank(0, 40)
svg.addShapeOn("Square",   node["id"], sc="red", sw=2, fo=0)
node = svg.addTank(0, 30)
svg.addShapeOn("Triangle", node["id"], sc="red", sw=2, fo=0)
node = svg.addTank(0, 20)
svg.addShapeOn("Diamond",  node["id"], sc="red", sw=2, fo=0)
node = svg.addTank(0, 10)
svg.addShapeOn("Plus",     node["id"], sc="red", sw=2, fo=0)
node = svg.addTank(0,  0)
svg.addShapeOn("X",        node["id"], sc="red", sw=2, fo=0)

# add 6 new reservoirs and then add a red shape of each type on top of each new reservoir

node = svg.addReservoir(10, 50)
svg.addShapeOn("Circle",   node["id"], sc="red", sw=2, fo=0)
node = svg.addReservoir(10, 40)
svg.addShapeOn("Square",   node["id"], sc="red", sw=2, fo=0)
node = svg.addReservoir(10, 30)
svg.addShapeOn("Triangle", node["id"], sc="red", sw=2, fo=0)
node = svg.addReservoir(10, 20)
svg.addShapeOn("Diamond",  node["id"], sc="red", sw=2, fo=0)
node = svg.addReservoir(10, 10)
svg.addShapeOn("Plus",     node["id"], sc="red", sw=2, fo=0)
node = svg.addReservoir(10,  0)
svg.addShapeOn("X",        node["id"], sc="red", sw=2, fo=0)


# This shows all the different types of shapes you can add how you can manipulate the shapes

# the fill color can be specified as words (plus and x have no fill)

svg.addShapeAt("Circle",  90, 100, fc="red")
svg.addShapeAt("Circle", 100, 100, fc="orange")
svg.addShapeAt("Circle", 110, 100, fc="yellow")
svg.addShapeAt("Circle", 120, 100, fc="green")
svg.addShapeAt("Circle", 130, 100, fc="blue")

# the stroke color can be specified as words

svg.addShapeAt("Circle",  90, 90, sc="red")
svg.addShapeAt("Circle", 100, 90, sc="orange")
svg.addShapeAt("Circle", 110, 90, sc="yellow")
svg.addShapeAt("Circle", 120, 90, sc="green")
svg.addShapeAt("Circle", 130, 90, sc="blue")

# the fill opacity can range form zero to one

svg.addShapeAt("Triangle",  90, 80, fo=0.00)
svg.addShapeAt("Triangle", 100, 80, fo=0.25)
svg.addShapeAt("Triangle", 110, 80, fo=0.50)
svg.addShapeAt("Triangle", 120, 80, fo=0.75)
svg.addShapeAt("Triangle", 130, 80, fo=1.00)

# the stroke opacity can range from zero to one

svg.addShapeAt("Square",  90, 70, so=0.00)
svg.addShapeAt("Square", 100, 70, so=0.25)
svg.addShapeAt("Square", 110, 70, so=0.50)
svg.addShapeAt("Square", 120, 70, so=0.75)
svg.addShapeAt("Square", 130, 70, so=1.00)

# the outline color (stoke) can be specified as words

svg.addShapeAt("Diamond",  90, 60, a= 0)
svg.addShapeAt("Diamond", 100, 60, a= 4)
svg.addShapeAt("Diamond", 110, 60, a= 8)
svg.addShapeAt("Diamond", 120, 60, a=12)
svg.addShapeAt("Diamond", 130, 60, a=16)

# the size of the shape is in pixels

svg.addShapeAt("Plus",  90, 50, fs= 4)
svg.addShapeAt("Plus", 100, 50, fs= 8)
svg.addShapeAt("Plus", 110, 50, fs=12)
svg.addShapeAt("Plus", 120, 50, fs=16)
svg.addShapeAt("Plus", 130, 50, fs=20)

# the width of the outline (stroke) is in pixels

svg.addShapeAt("X",  90, 40, sw=0.2)
svg.addShapeAt("X", 100, 40, sw=0.5)
svg.addShapeAt("X", 110, 40, sw=1.0)
svg.addShapeAt("X", 120, 40, sw=2.0)
svg.addShapeAt("X", 130, 40, sw=3.0)

# the shape can be written several different ways and is not case-sensative

svg.addShapeAt("c",  80, 20) # circle   Cir Circle   CIRCLE
svg.addShapeAt("s",  90, 20) # square   Squ Square   SQUARE
svg.addShapeAt("t", 100, 20) # triangle Tri Triangle TRIANGLE
svg.addShapeAt("d", 110, 20) # diamond  Dia Diamond  DIAMOND
svg.addShapeAt("p", 120, 20) # plus     +   Plus     PLUS
svg.addShapeAt("x", 130, 20) # x                     X

# the stroke and fill colors can be specified as hexidecimal RGB

svg.addShapeAt("CIR",  60, 0, fo=1, so=1, fc="#000000", sc="#ffffff")
svg.addShapeAt("CIR",  70, 0, fo=1, so=1, fc="#222222", sc="#dddddd")
svg.addShapeAt("CIR",  80, 0, fo=1, so=1, fc="#444444", sc="#bbbbbb")
svg.addShapeAt("CIR",  90, 0, fo=1, so=1, fc="#666666", sc="#999999")
svg.addShapeAt("CIR", 100, 0, fo=1, so=1, fc="#999999", sc="#666666")
svg.addShapeAt("CIR", 110, 0, fo=1, so=1, fc="#bbbbbb", sc="#444444")
svg.addShapeAt("CIR", 120, 0, fo=1, so=1, fc="#dddddd", sc="#222222")
svg.addShapeAt("CIR", 130, 0, fo=1, so=1, fc="#ffffff", sc="#000000")

#
# This shows how you can add Text and how to manipulate the text
#

svg.addText(86, 104, "change fill color")
svg.addText(86,  94, "change stroke color")
svg.addText(86,  84, "change fill opacity")
svg.addText(86,  74, "change stroke opacity")
svg.addText(86,  64, "rotate shape degrees clockwise")
svg.addText(86,  54, "change shape size")
svg.addText(86,  44, "change stroke width")
svg.addText(76,  24, "specify the shape type in multple ways (not case-sensative)")

svg.addText( 76, 15, "circle"  , size=0.8)
svg.addText( 76, 13, "cir"     , size=0.8)
svg.addText( 76, 11, "c"       , size=0.8)

svg.addText( 86, 15, "square"  , size=0.8)
svg.addText( 86, 13, "squ"     , size=0.8)
svg.addText( 86, 11, "s"       , size=0.8)

svg.addText( 96, 15, "triangle", size=0.8)
svg.addText( 96, 13, "tri"     , size=0.8)
svg.addText( 96, 11, "t"       , size=0.8)

svg.addText(106, 15, "diamond" , size=0.8)
svg.addText(106, 13, "dia"     , size=0.8)
svg.addText(106, 11, "d"       , size=0.8)

svg.addText(116, 15, "plus"    , size=0.8)
svg.addText(116, 13, "p"       , size=0.8)
svg.addText(116, 11, "+"       , size=0.8)

svg.addText(126, 15, "x"       , size=0.8)

svg.addText(56,   4, "specify stroke and fill colors with hexidecimal notation")

svg.addText( 46,  -6, "fill ="  , size=0.8)
svg.addText( 46,  -9, "stroke =", size=0.8)

svg.addText( 56,  -6, "#000000", size=0.8)
svg.addText( 56,  -9, "#ffffff", size=0.8)

svg.addText( 66,  -6, "#222222", size=0.8)
svg.addText( 66,  -9, "#dddddd", size=0.8)

svg.addText( 76,  -6, "#444444", size=0.8)
svg.addText( 76,  -9, "#bbbbbb", size=0.8)

svg.addText( 86,  -6, "#666666", size=0.8)
svg.addText( 86,  -9, "#999999", size=0.8)

svg.addText( 96,  -6, "#999999", size=0.8)
svg.addText( 96,  -9, "#666666", size=0.8)

svg.addText(106,  -6, "#bbbbbb", size=0.8)
svg.addText(106,  -9, "#444444", size=0.8)

svg.addText(116,  -6, "#dddddd", size=0.8)
svg.addText(116,  -9, "#222222", size=0.8)

svg.addText(126,  -6, "#ffffff", size=0.8)
svg.addText(126,  -9, "#000000", size=0.8)

for i in range(0, 15):
	size = 1.5 - i / 10.0
	svg.addText(-15, 100 - i * 3.0, "change text size, " + str(size), size=size)

svg.addText(60, 100, "change text color", color="red"    )
svg.addText(60,  97, "change text color", color="orange" )
svg.addText(60,  94, "change text color", color="yellow" )
svg.addText(60,  91, "change text color", color="green"  )
svg.addText(60,  88, "change text color", color="blue"   )
svg.addText(60,  85, "change text color", color="purple" )

svg.addText(20,  85, "change text angle, -30", angle=-30)
svg.addText(20,  85, "change text angle, -20", angle=-20)
svg.addText(20,  85, "change text angle, -10", angle=-10)
svg.addText(20,  85, "change text angle, 0"  , angle=+ 0)
svg.addText(20,  85, "change text angle, 10" , angle=+10)
svg.addText(20,  85, "change text angle, 20" , angle=+20)
svg.addText(20,  85, "change text angle, 30" , angle=+30)

svg.addText(32, 12, "(30,10)", size=0.7)
svg.addText(52, 12, "(50,10)", size=0.7)
#svg.addText(32, 42, "(30,40)", size=0.7)
svg.addText(72, 72, "(70,70)", size=0.7)
svg.addText(52, 72, "(50,70)", size=0.7)

svg.addText(56.4, 65.0, "change text opacity, 1.0", opacity=1.0)
svg.addText(56.4, 62.5, "change text opacity, 0.9", opacity=0.9)
svg.addText(56.4, 60.0, "change text opacity, 0.8", opacity=0.8)
svg.addText(56.4, 57.5, "change text opacity, 0.7", opacity=0.7)
svg.addText(56.4, 55.0, "change text opacity, 0.6", opacity=0.6)
svg.addText(56.4, 52.5, "change text opacity, 0.5", opacity=0.5)
svg.addText(56.4, 50.0, "change text opacity, 0.4", opacity=0.4)
svg.addText(56.4, 47.5, "change text opacity, 0.3", opacity=0.3)
svg.addText(56.4, 45.0, "change text opacity, 0.2", opacity=0.2)
svg.addText(56.4, 42.5, "change text opacity, 0.1", opacity=0.1)

#
# This shows the 5 different ways to add a line
#
#	1. add line with (x1, y1) and (x2, y2)
#	2. add line with link name
#	3. add line with node name and (x2, y2)
#	4. add line with (x1, y1) and node name
#	5. add line with node1 name and node2 name 

svg.addText(19, 10, "add line segments with different opacities", angle=-90, size=0.7, opacity=1)
svg.addLine(20, 60, 20, 50, width=2, opacity=1.0)
svg.addLine(20, 50, 20, 40, width=2, opacity=0.9)
svg.addLine(20, 40, 20, 30, width=2, opacity=0.8)
svg.addLine(20, 30, 20, 20, width=2, opacity=0.7)
svg.addLine(20, 20, 20, 10, width=2, opacity=0.6)
svg.addLine(20, 10, 20,  0, width=2, opacity=0.5)
svg.addLine(20,  0, 30,  0, width=2, opacity=0.4)
svg.addLine(30,  0, 40,  0, width=2, opacity=0.3)
svg.addLine(30,  0, 50,  0, width=2, opacity=0.2)

svg.addText(31, 67, "add line on top of a link", size=0.55, color="black")
svg.addText(31, 67, "add line on top of a link", size=0.55, color="red", opacity=0.6)
svg.addLineOnLink("10", width=3, color="red")
svg.addText(31, 59, "add line from node to xy", size=0.55, color="black")
svg.addText(31, 59, "add line from node to xy", size=0.55, color="orange", opacity=0.6)
svg.addLineFromNode("10", 30, 60, width=3, color="orange")
svg.addText(31, 50, "add line from xy to node", size=0.55, color="black")
svg.addText(31, 50, "add line from xy to node", size=0.55, color="yellow", opacity=0.6)
svg.addLineToNode(30, 50, "10", width=3, color="yellow")
svg.addText(31, 42, "add line from node to node", size=0.55, color="black")
svg.addText(31, 42, "add line from node to node", size=0.55, color="green", opacity=0.6)
svg.addLineBetweenNodes("10", "21", width=3, color="green")


#
# Use this call to make the tanks and reservoirs look like the ones in EPANet
# rather than than the ones used in the dinesti gui
#

svg.useEpanetIcons()

#
# write the resulting svg file to hard disk
#
#		output_file_name = string, optional
#						   specifies the location on the hard drive to save the resulting html file
#						   if no file name is given the default is the inp file name with ".html" tacked onto the end
#
#		width & height   = float, greater than zero, optional
#						   if width  is given, height = width  * aspect_ratio
#						   if height is given, width  = height / aspect_ratio
#						   where aspect_ratio = rise / run
#						   if neither of these are given the default is:
#								width  = 600 (for wider  networks, i.e. landscapes), height = width  * aspect_ratio
#								or
#								height = 600 (for taller networks, i.e. portraits ), width  = height / aspect_ratio

svg.writeFile(width=1200, height=750, output_file_name="Net1.html")

















