#  _________________________________________________________________________
#
#  Water Security Toolkit (WST)
#  Copyright (c) 2012 Sandia Corporation.
#  This software is distributed under the Revised BSD License.
#  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive 
#  license for use of this work by or on behalf of the U.S. government.
#  For more information, see the License Notice in the WST User Manual.
#  _________________________________________________________________________
#

import os, sys, datetime
import yaml, json
import time
import logging

import pywst.common.problem
import pywst.common.wst_config as wst_config
import pywst.visualization.inp2svg as inp2svg

logger = logging.getLogger('wst.visualization')

# TODO - SP subcommand doesnt require the INP file name in the YML file. It should be in the examples at least.

class Problem(pywst.common.problem.Problem):

    def __init__(self):
        pywst.common.problem.Problem.__init__(self, "visualization", ("network","visualization","configure"),) 
        return

    def run(self):
        logger.info("WST visualization subcommand")
        logger.info("---------------------------")
        
        # set start time
        nStartTime = time.time()
        
        # validate input
        logger.info("Validating configuration file")
        self.validate()
        if self.bInputValidationFailed: 
            for sError in self.dError:
                print "ERROR: " + sError
            return
        #
        logger.info("Creating HTML")
        svg = inp2svg.inp2svg(self.dInpFile)
        svg.setBackgroundColor(self.dBackgroundColor)
        #
        svg.setNodeOpacity     (self.dNodeOpacity     )
        svg.setLinkOpacity     (self.dLinkOpacity     )
        #
        svg.setNodeSize        (self.dNodeSize        )
        svg.setLinkSize        (self.dLinkSize        )
        #
        svg.setNodeColor       (self.dNodeColor       )
        svg.setLinkColor       (self.dLinkColor       )
        #
        svg.setReservoirSize   (self.dReservoirSize   )
        svg.setTankSize        (self.dTankSize        )
        svg.setJunctionSize    (self.dJunctionSize    )
        svg.setPumpSize        (self.dPumpSize        )
        svg.setValveSize       (self.dValveSize       )
        svg.setPipeSize        (self.dPipeSize        )
        #
        svg.setReservoirColor  (self.dReservoirColor  )
        svg.setTankColor       (self.dTankColor       )
        svg.setJunctionColor   (self.dJunctionColor   )
        svg.setPumpColor       (self.dPumpColor       )
        svg.setValveColor      (self.dValveColor      )
        svg.setPipeColor       (self.dPipeColor       )
        #
        svg.setReservoirOpacity(self.dReservoirOpacity)
        svg.setTankOpacity     (self.dTankOpacity     )
        svg.setJunctionOpacity (self.dJunctionOpacity )
        svg.setPumpOpacity     (self.dPumpOpacity     )
        svg.setValveOpacity    (self.dValveOpacity    )
        svg.setPipeOpacity     (self.dPipeOpacity     )
        #
        svg.setLegendNodes     (self.bLegendNodes     )
        svg.setLegendLinks     (self.bLegendLinks     )
        svg.setLegendReservoirs(self.bLegendReservoirs)
        svg.setLegendTanks     (self.bLegendTanks     )
        svg.setLegendJunctions (self.bLegendJunctions )
        svg.setLegendPumps     (self.bLegendPumps     )
        svg.setLegendValves    (self.bLegendValves    )
        svg.setLegendPipes     (self.bLegendPipes     )
        #
        svg.useEpanetIcons(self.bUseEpanetShapes)
        svg.showLegend(self.bShowLegend)
        svg.setLegendColor(self.dLegendColor)
        svg.setLegendScale(self.dLegendScale)
        svg.setLegendXY(self.dLegendX, self.dLegendY)
        #
        i = 0
        for layer in self.Layers:
            if layer.hide: continue
            #print i,') Layer = ', layer
            layer.visualize(svg)
            i += 1
        
        # write output file 
        prefix = os.path.basename(self.opts['configure']['output prefix'])      
        logfilename = logger.parent.handlers[0].baseFilename
        outfilename = logger.parent.handlers[0].baseFilename.replace('.log','.yml')
        visfilename = logger.parent.handlers[0].baseFilename.replace('.log','.html')

        #
        svg.writeFile(width=self.nWidth, height=self.nHeight, output_file_name=visfilename)
        
        config = wst_config.output_config()
        module_blocks = ['general']
        template_options = {
            'general':{
                'cpu time': round(time.time() - nStartTime,3),
                'directory': os.path.dirname(logfilename),
                'log file': os.path.basename(logfilename)}}
        if outfilename != None:
            self.saveOutput(outfilename, config, module_blocks, template_options)
        
        # print solution to screen
        logger.info("\nWST normal termination")
        logger.info("---------------------------")
        logger.info("Directory: "+os.path.dirname(logfilename))
        logger.info("Results file: "+os.path.basename(outfilename))
        logger.info("Log file: "+os.path.basename(logfilename))
        logger.info("Visualization file: "+os.path.basename(visfilename)+'\n')
        
        return

    def validate(self):
        self.dError = []
        self.bInputValidationFailed = False
        #
        self.dPrefix          = self.getConfigureOption("output prefix"      )
        self.dInpFile         = self.getNetworkOption  ("epanet file"        )
        #
        if isMissing(self.dInpFile):
            self.bInputValidationFailed = True
            self.dError.append("The INP file is missing from the input!")
            return
        if not os.path.exists(self.dInpFile):
            self.bInputValidationFailed = True
            self.dError.append("The INP file doesnt exist!")
            return
        if not os.path.isfile(self.dInpFile): 
            self.bInputValidationFailed = True
            self.dError.append("The INP file is not a file!")
            return
        #
        self.dBackgroundColor = self.opts['visualization']['screen']['color']
        ScreenSize = self.opts['visualization']['screen']['size'] 
        #
        bValid = False
        length = len(ScreenSize)
        if length > 1:
            x = ScreenSize[0]
            y = ScreenSize[1]
            if isNumeric(x) and isNumeric(y):
                self.nWidth = x
                self.nHeight = y
                bValid = True
                #
        self.bUseEpanetShapes = self.opts['visualization']['legend']['use EPANet symbols'] 
        self.bShowLegend      = self.opts['visualization']['legend']['show legend'] 
        self.dLegendColor     = self.opts['visualization']['legend']['color']
        self.dLegendScale     = self.opts['visualization']['legend']['scale'] 
        LegendLocation        = self.opts['visualization']['legend']['location'] 
        #
        bValid = False
        length = len(LegendLocation)
        if length > 1:
            x = LegendLocation[0]
            y = LegendLocation[1]
            if isNumeric(x) and isNumeric(y):
                self.dLegendX = x
                self.dLegendY = y
                bValid = True
                #
        self.bShowLegend = self.bShowLegend and bValid
        #
        self.dNodeColor        = self.getNodesOption     ("color"  )
        self.dNodeSize         = self.getNodesOption     ("size"   )
        self.dNodeOpacity      = self.getNodesOption     ("opacity")
        #
        self.dLinkColor        = self.getLinksOption     ("color"  )
        self.dLinkSize         = self.getLinksOption     ("size"   )
        self.dLinkOpacity      = self.getLinksOption     ("opacity")
        #
        self.dReservoirColor   = self.getReservoirsOption("color"  )
        self.dReservoirSize    = self.getReservoirsOption("size"   )
        self.dReservoirOpacity = self.getReservoirsOption("opacity")
        #
        self.dTankColor        = self.getTanksOption     ("color"  )
        self.dTankSize         = self.getTanksOption     ("size"   )
        self.dTankOpacity      = self.getTanksOption     ("opacity")
        #
        self.dJunctionColor    = self.getJunctionsOption ("color"  )
        self.dJunctionSize     = self.getJunctionsOption ("size"   )
        self.dJunctionOpacity  = self.getJunctionsOption ("opacity")
        #
        self.dPumpColor        = self.getPumpsOption     ("color"  )
        self.dPumpSize         = self.getPumpsOption     ("size"   )
        self.dPumpOpacity      = self.getPumpsOption     ("opacity")
        #
        self.dValveColor       = self.getValvesOption    ("color"  )
        self.dValveSize        = self.getValvesOption    ("size"   )
        self.dValveOpacity     = self.getValvesOption    ("opacity")
        #
        self.dPipeColor        = self.getPipesOption     ("color"  )
        self.dPipeSize         = self.getPipesOption     ("size"   )
        self.dPipeOpacity      = self.getPipesOption     ("opacity")
        #
        self.bLegendNodes      = self.getNodesOption     ("show in legend")
        self.bLegendLinks      = self.getLinksOption     ("show in legend")
        self.bLegendReservoirs = self.getReservoirsOption("show in legend")
        self.bLegendTanks      = self.getTanksOption     ("show in legend")
        self.bLegendJunctions  = self.getJunctionsOption ("show in legend")
        self.bLegendPumps      = self.getPumpsOption     ("show in legend")
        self.bLegendValves     = self.getValvesOption    ("show in legend")
        self.bLegendPipes      = self.getPipesOption     ("show in legend")
        #
        layers = self.getLayers()
        self.Layers = []
        if layers:
            for layer in layers:
                _layer = Layer(layer)
                self.Layers.append(_layer)
        return

    def getVisualizationOption(self,  name):
        value = self.opts["visualization"][name]
        if value in pywst.common.problem.none_list: return None
        return value
    def getLayers(self):
        value = self.opts["visualization"]["layers"]
        if value in pywst.common.problem.none_list: return None
        return value
    def getNodesOption(self, name):
        value = self.opts["visualization"]["nodes"][name]
        if value in pywst.common.problem.none_list: return None
        return value
    def getLinksOption(self, name):
        value = self.opts["visualization"]["links"][name]
        if value in pywst.common.problem.none_list: return None
        return value
    def getReservoirsOption(self, name):
        value = self.opts["visualization"]["reservoirs"][name]
        if value in pywst.common.problem.none_list: return None
        return value
    def getTanksOption(self, name):
        value = self.opts["visualization"]["tanks"][name]
        if value in pywst.common.problem.none_list: return None
        return value
    def getJunctionsOption(self, name):
        value = self.opts["visualization"]["junctions"][name]
        if value in pywst.common.problem.none_list: return None
        return value
    def getPumpsOption(self, name):
        value = self.opts["visualization"]["pumps"][name]
        if value in pywst.common.problem.none_list: return None
        return value
    def getValvesOption(self, name):
        value = self.opts["visualization"]["valves"][name]
        if value in pywst.common.problem.none_list: return None
        return value
    def getPipesOption(self, name):
        value = self.opts["visualization"]["pipes"][name]
        if value in pywst.common.problem.none_list: return None
        return value
    def getLayersOption(self, i, name):
        value = self.opts["visualization"]["layers"][i][name]
        if value in pywst.common.problem.none_list: return None
        return value
    def getConfigureOption(self,  name):
        value = self.opts["configure"][name]
        if value in pywst.common.problem.none_list: return None
        return value
    def getNetworkOption(self,  name):
        value = self.opts["network"][name]
        if value in pywst.common.problem.none_list: return None
        return value
    #

class AttributesChild(object):
    Color   = None
    Size    = None
    Opacity = None
    def __init__(self):
        pass

class Attributes(object):
    Fill = AttributesChild()
    Line = AttributesChild()
    def __init__(self):
        pass

class Layer(object):
    #
    LOCATION_TYPE_NODE   = "NODE"
    LOCATION_TYPE_LINK   = "LINK"
    #
    KEY_TYPE_LOCATION    = "LOCATION"
    KEY_TYPE_COLOR       = "COLOR"
    KEY_TYPE_SIZE        = "SIZE"
    KEY_TYPE_OPACITY     = "OPACITY"
    #
    DEFAULT_SHAPE        = "Circle"
    DEFAULT_FILL_COLOR   = "white"
    DEFAULT_FILL_SIZE    = 20
    DEFAULT_FILL_OPACITY = 0.6
    DEFAULT_LINE_COLOR   = "white"
    DEFAULT_LINE_SIZE    = 2
    DEFAULT_LINE_OPACITY = 0.6
    #
    shape                = None
    fill_color           = None
    fill_size            = None
    fill_opacity         = None
    line_color           = None
    line_size            = None
    line_opacity         = None
    #
    AttrMax = Attributes()
    #
    def __init__(self, config=None):
        #
        if config == None: return
        #
        self.label              = config['label']
        self.data_file          = config['file'] 
        self.location_type      = config['location type']
        self.locations          = config['locations']
        self.dhape              = config['shape'] 
        self.fill_color         = config['fill']['color'] 
        self.fill_size          = config['fill']['size']
        self.fill_opacity       = config['fill']['opacity'] 
        self.line_color         = config['line']['color']
        self.line_size          = config['line']['size'] 
        self.line_opacity       = config['line']['opacity'] 
        self.fill_color_range   = config['fill']['color range']
        self.fill_size_range    = config['fill']['size range'] 
        self.fill_opacity_range = config['fill']['opacity range'] 
        self.line_color_range   = config['line']['color range'] 
        self.line_size_range    = config['line']['size range'] 
        self.line_opacity_range = config['line']['opacity range']
        self.hide               = config['hide'] 
        if self.hide: return
        #
        # Validate that the label is at least a blank string
        #
        if self.label == None: self.label = ""
        #
        # Validate the data file
        #
        if self.data_file:
            if isString(self.data_file):
                if os.path.exists(self.data_file):
                    if os.path.isfile(self.data_file):
                        pass
                    else:
                        self.data_file = None
                else:
                    self.data_file = None
            else:
                self.data_file = None
        #
        # Validate that the location type is one from the list above (or has an 's' on the end)
        #
        if self.location_type == None:
            self.location_type = self.LOCATION_TYPE_NODE
        elif self.location_type.upper() == self.LOCATION_TYPE_NODE:
            self.location_type = self.LOCATION_TYPE_NODE
        elif self.location_type.upper() == self.LOCATION_TYPE_NODE + "S":
            self.location_type = self.LOCATION_TYPE_NODE
        elif self.location_type.upper() == self.LOCATION_TYPE_LINK:
            self.location_type = self.LOCATION_TYPE_LINK
        elif self.location_type.upper() == self.LOCATION_TYPE_LINK + "S":
            self.location_type = self.LOCATION_TYPE_LINK
        else:
            self.location_type = None
        #
        #self.fill_color_max   = None
        #self.fill_size_max    = None
        #self.fill_opacity_max = None
        #self.line_color_max   = None
        #self.line_size_max    = None
        #self.line_opacity_max = None
        #
        # Read json file
        #
        if self.data_file:
            f = open(self.data_file, 'r')
            data = None
            if self.data_file.endswith(".yml"):
                data = yaml.load_all(f).next()
            else:
                data = json.load(f)
            f.close()
            #
            locations_key    = self.locations
            fill_color_key   = self.fill_color
            fill_size_key    = self.fill_size
            fill_opacity_key = self.fill_opacity
            line_color_key   = self.line_color
            line_size_key    = self.line_size
            line_opacity_key = self.line_opacity
            #
            self.locations    = []
            self.fill_color   = []
            self.fill_size    = []
            self.fill_opacity = []
            self.line_color   = []
            self.line_size    = []
            self.line_opacity = []

            i = 0
            while True:
                location = self.__parseKey(data, locations_key, i, None, self.KEY_TYPE_LOCATION)
                if location == None: break
                self.locations   .append(location)
                self.fill_color  .append(self.__parseKey(data, fill_color_key  , i, 0, self.KEY_TYPE_COLOR  ))
                self.fill_size   .append(self.__parseKey(data, fill_size_key   , i, 0, self.KEY_TYPE_SIZE   ))
                self.fill_opacity.append(self.__parseKey(data, fill_opacity_key, i, 0, self.KEY_TYPE_OPACITY))
                self.line_color  .append(self.__parseKey(data, line_color_key  , i, 0, self.KEY_TYPE_COLOR  ))
                self.line_size   .append(self.__parseKey(data, line_size_key   , i, 0, self.KEY_TYPE_SIZE   ))
                self.line_opacity.append(self.__parseKey(data, line_opacity_key, i, 0, self.KEY_TYPE_OPACITY))
                i += 1
            #
            length = i
            self.fill_color   = self.__applyRange(self.KEY_TYPE_COLOR  , self.fill_color  , self.fill_color_range  , length)
            self.fill_size    = self.__applyRange(self.KEY_TYPE_SIZE   , self.fill_size   , self.fill_size_range   , length)
            self.fill_opacity = self.__applyRange(self.KEY_TYPE_OPACITY, self.fill_opacity, self.fill_opacity_range, length)
            self.line_color   = self.__applyRange(self.KEY_TYPE_COLOR  , self.line_color  , self.line_color_range  , length)
            self.line_size    = self.__applyRange(self.KEY_TYPE_SIZE   , self.line_size   , self.line_size_range   , length)
            self.line_opacity = self.__applyRange(self.KEY_TYPE_OPACITY, self.line_opacity, self.line_opacity_range, length)
            #
        else:
            self.fill_color_range   = None
            self.fill_size_range    = None
            self.fill_opacity_range = None
            self.line_color_range   = None
            self.line_size_range    = None
            self.line_opacity_range = None
            #
            if type(self.locations) == list:
                values = self.locations
                self.locations = []
                for value in values:
                    self.locations.append(str(value))
            else:
                value = self.locations
                self.locations = []
                self.locations.append(str(value))
            #
        return

    def __parseKey(self, data, key, i, default, key_type):
        value, bKey = self.__parseKey1(data, key, i, default, key_type)
        if bKey and key_type != self.KEY_TYPE_LOCATION:
            pass
        return value

    def __parseKey1(self, data, key, i, default, key_type):
        cmd = None
        if isString(key) and len(key.strip()) > 0 and key.strip()[0] != "#":
            cmd = "value = data" + key
        elif type(key)==list and len(key) == 1:
            cmd = "value = data" + key[0]
        
        if cmd is not None:
            try:
                exec(cmd)
                return self.__parseKey3(key_type, value), True
            except IndexError:
                return default, False
            except: # SyntaxError, KeyError, NameError
                return self.__parseKey2(key, i, default, key_type), False
        else:
            return self.__parseKey2(key, i, default, key_type), False

    def __parseKey2(self, key, i, default, key_type):
        if type(key) == list:
            try:
                return self.__parseKey3(key_type, key[i])
            except: # IndexError, ValueError
                return default
        else:
            if i == 1 and key_type == self.KEY_TYPE_LOCATION:
                return None
            try:
                return self.__parseKey3(key_type, key)
            except: # ValueError, TypeError
                return default

    def __parseKey3(self, key_type, value):
        if key_type == self.KEY_TYPE_LOCATION:
            return str(value)
        elif key_type == self.KEY_TYPE_COLOR:
            if isString(value):
                return value
            return value
            #return convertColorIntToHex(value)
        else:
            return float(value)

    def __applyRange(self, key_type, List, Range, length):
        if Range == None: return List
        nLen = len(Range)
        if nLen < 1: return List
        high = max(List)
        low  = min(List)
        diff = high - low
        nListLen = len(List)
        interval = float(diff) / float(nLen - 1) if nLen > 1 else 0
        for i in range(0, length):
            if interval == 0:
                if key_type == self.KEY_TYPE_COLOR:
                    List[i] = convertColorStringToHex(Range[nLen - 1])
                elif key_type == self.KEY_TYPE_SIZE or key_type == self.KEY_TYPE_OPACITY:
                    List[i] = Range[nLen - 1]
                continue
            r = float(List[i] - low) / float(interval)
            ir = int(r)
            if r == ir:
                if key_type == self.KEY_TYPE_COLOR:
                    List[i] = convertColorStringToHex(Range[ir])
                elif key_type == self.KEY_TYPE_SIZE or key_type == self.KEY_TYPE_OPACITY:
                    List[i] = Range[ir]
            else:
                if key_type == self.KEY_TYPE_COLOR:
                    c1 = convertColorStringToObject(Range[ir])
                    c2 = convertColorStringToObject(Range[ir + 1])
                elif key_type == self.KEY_TYPE_SIZE or key_type == self.KEY_TYPE_OPACITY:
                    c1 = Range[ir]
                    c2 = Range[ir + 1]
                lo = low + interval * ir
                hi = lo + interval
                v  = float(List[i] - lo) / float(hi - lo)
                if key_type == self.KEY_TYPE_COLOR:
                    c = interpolateColor(c1, c2, v)
                    List[i] = convertColorObjectToHex(c)
                elif key_type == self.KEY_TYPE_SIZE or key_type == self.KEY_TYPE_OPACITY:
                    List[i] = interpolateValue(c1, c2, v)
        return List

    def __repr__(self):
        s =  ''
        s += '\n label         = ' + str(self.label        )
        s += '\n data_file     = ' + str(self.data_file    )
        s += '\n location_type = ' + str(self.location_type)
        s += '\n locations     = ' + str(self.locations    )
        s += '\n shape         = ' + str(self.dhape        )
        s += '\n fill color    = ' + str(self.fill_color   )
        s += '\n fill size     = ' + str(self.fill_size    )
        s += '\n fill opacity  = ' + str(self.fill_opacity )
        s += '\n line color    = ' + str(self.line_color   )
        s += '\n line size     = ' + str(self.line_size    )
        s += '\n line opacity  = ' + str(self.line_opacity )
        return s

    def visualize(self, svg):
        iLayer = svg.addLayer(self.label)
        #
        if self.location_type == self.LOCATION_TYPE_NODE:
            svg.getLayer(iLayer)["type"] = svg.LAYER_TYPE_NODE
            #
            i = 0
            for location in self.locations:
                shape = self.__getFromList(self.dhape       , i, self.DEFAULT_SHAPE)
                fc    = self.__getFromList(self.fill_color  , i, self.DEFAULT_FILL_COLOR)
                fs    = self.__getFromList(self.fill_size   , i, 20)
                fo    = self.__getFromList(self.fill_opacity, i, 0.6)
                #   
                lc, ls, lo = 0, 0, 0
                if self.__checkVisible(self.line_color, self.line_size, self.line_opacity, i):
                    lc = self.__getFromList(self.line_color  , i)
                    ls = self.__getFromList(self.line_size   , i)
                    lo = self.__getFromList(self.line_opacity, i, 0.6)
                #
                if isNumeric(fc): fc = convertColorIntToHex(fc)
                if isNumeric(lc): lc = convertColorIntToHex(lc)
                #
                if i == 0:
                    svg.getLayer(iLayer)["shape"       ] = shape
                    svg.getLayer(iLayer)["fill color"  ] = fc
                    svg.getLayer(iLayer)["fill opacity"] = fo
                    svg.getLayer(iLayer)["line color"  ] = lc
                    svg.getLayer(iLayer)["line size"   ] = ls
                    svg.getLayer(iLayer)["line opacity"] = lo
                #
                svg.addShapeOn(shape, location, fs=fs, fc=fc, fo=fo, sc=lc, sw=ls, so=lo, layer=iLayer)
                i += 1
                #
        elif self.location_type == self.LOCATION_TYPE_LINK:
            svg.getLayer(iLayer)["type"        ] = svg.LAYER_TYPE_LINK
            i = 0
            for location in self.locations:
                color   = self.__getFromList(self.fill_color  , i)
                width   = self.__getFromList(self.fill_size   , i)
                opacity = self.__getFromList(self.fill_opacity, i, 0)
                if isNumeric(color): fc = convertColorIntToHex(color)
                #
                if i == 0:
                    svg.getLayer(iLayer)["shape"       ] = "pipe"
                    svg.getLayer(iLayer)["line color"  ] = color
                    svg.getLayer(iLayer)["line opacity"] = opacity
                #
                svg.addLineOnLink(location, color=color, width=width, opacity=opacity, layer=iLayer)
                i += 1
        return

    def __getFromList(self, l, i, default=None):
        if l == None: return default
        if type(l) == list:
            if len(l) == 1:
                return default if l[0] == None else l[0]
            return default if l[i] == None else l[i]
        return l

    def __checkVisible(self, color, size, opacity, i):
        c = self.__getFromList(color  , i)
        s = self.__getFromList(size   , i)
        o = self.__getFromList(opacity, i)
        return not not (c or s or o)

def convertColorIntToHex(value):
    i = int(value)
    s = hex(i)
    s = s[2:]
    s = s.zfill(6)
    return "#" + s

def convertColorStringToObject(s):
    s = s.strip()
    if len(s) == 7 and s[0:1] == "#":
        return convertColorHexToObject(s)
    return convertColorNameToObject(s)

def convertColorStringToHex(s):
    obj = convertColorStringToObject(s)
    return convertColorObjectToHex(obj)

def convertColorHexToObject(color):
    red   = color[1:3]
    green = color[3:5]
    blue  = color[5:7]
    return {"red": int(red, 16), "green": int(green, 16), "blue": int(blue, 16)}

def convertColorNameToObject(name):
    if name.strip().upper() == "RED"    : return {"red": 255, "green":   0, "blue":   0}
    if name.strip().upper() == "ORANGE" : return {"red": 255, "green": 165, "blue":   0}
    if name.strip().upper() == "YELLOW" : return {"red": 255, "green": 255, "blue":   0}
    if name.strip().upper() == "GREEN"  : return {"red":   0, "green": 128, "blue":   0}
    if name.strip().upper() == "BLUE"   : return {"red":   0, "green":   0, "blue": 255}
    if name.strip().upper() == "PURPLE" : return {"red": 128, "green":   0, "blue": 128}
    if name.strip().upper() == "BLACK"  : return {"red":   0, "green":   0, "blue":   0}
    if name.strip().upper() == "WHITE"  : return {"red": 255, "green": 255, "blue": 255}
    if name.strip().upper() == "LIME"   : return {"red":   0, "green": 255, "blue":   0}
    if name.strip().upper() == "NAVY"   : return {"red":   0, "green":   0, "blue": 128}
    if name.strip().upper() == "AQUA"   : return {"red":   0, "green": 255, "blue": 255}
    if name.strip().upper() == "TEAL"   : return {"red":   0, "green": 128, "blue": 128}
    if name.strip().upper() == "OLIVE"  : return {"red": 128, "green": 128, "blue":   0}
    if name.strip().upper() == "MAROON" : return {"red": 128, "green":   0, "blue":   0}
    if name.strip().upper() == "FUCHSIA": return {"red": 255, "green":   0, "blue": 255}
    if name.strip().upper() == "SILVER" : return {"red": 192, "green": 192, "blue": 192}
    if name.strip().upper() == "GRAY"   : return {"red": 128, "green": 128, "blue": 128}
    if name.strip().upper() == "GREY"   : return {"red": 128, "green": 128, "blue": 128}
    return None

def interpolateColor(c1, c2, value):
    red   = interpolateValue(c1["red"  ], c2["red"  ], value)
    green = interpolateValue(c1["green"], c2["green"], value)
    blue  = interpolateValue(c1["blue" ], c2["blue" ], value)
    return {"red": red, "green": green, "blue": blue}

def convertColorObjectToHex(color):
    red   = hex(int(color["red"  ]))[2:].zfill(2)
    green = hex(int(color["green"]))[2:].zfill(2)
    blue  = hex(int(color["blue" ]))[2:].zfill(2)
    return "#" + red + green + blue

def interpolateValue(v1, v2, value):
    return v1 + ( v2 - v1 ) * value

def isNotMissing(value):
    return not isMissing(value)
def isMissing(value):
    return (value in pywst.common.problem.none_list)
def isNotNumeric(value):
    return not isNumeric(value)
def isNumeric(value):
    return (type(value) == float or type(value) == int)
def isNotString(value):
    return not isString(value)
def isString(value):
    return (type(value) == str or type(value) == unicode)
