#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

__all__ = ['ProblemRegistration', 'ProblemFactory', 'SPProblem']

import os
import os.path
import sys
from pyutilib.workflow import Component
from pyutilib.component.core import *
import pyutilib.misc


#
# TODO: migrate this into PyUtilib.
#
def count_lines(file):
  count = 0
  for line in open(file,"r"):
    count = count + 1
  return count


class ISPOTProblemRegistration(Interface):
    """An interface for accessing"""

    def create(self, name=None):
        """Create a problem, optionally specifying the name"""

    def type(self):
        """The type of problem supported by this service"""


class ProblemRegistration(Plugin):

    implements(ISPOTProblemRegistration)

    def __init__(self, type, cls, description=""):
        self.name = type
        self._type = type
        self._cls = cls
        self.description = description

    def type(self):
        return self._type

    def create(self, **kwds):
        return self._cls(**kwds)


def ProblemFactory(problem_name=None, **kwds):
    ep = ExtensionPoint(ISPOTProblemRegistration)
    if problem_name is None:
        return map(lambda x:(x.name,x.description), ep())
    service = ep.service(problem_name)
    if service is None:
        # TODO - replace this print statement with a logging statement
        print "Unknown problem '" + problem_name + "' - no plugin registered for this problem type"
        return None
    else:
        return ep.service(problem_name).create(**kwds)


class SPProblem(Component):
    """
    An object that encapsulates the problem to be solved.
    """

    def __init__(self):
        """
        Initialize here
        """
        Component.__init__(self)
        self.impactdir = os.getcwd()
        self.modpath = os.getcwd()
        self.data_dir = os.getcwd()
        self.threshold={}
        self.num_nodes=-1
        self.num_lines=-1
        self.default_statistic="mean"
        self.impact_file={}
        self.total_number_side_constraints=0
        self.fixed_vals = []
        self.invalid_vals = []
        self.cost_map = {}

    def _create_parser(self):
        Component._create_parser(self)
        self.parser.add_option("--debug", 
            help="Debugging information", 
            action="store_true",
            dest="debug",
            default=False)

    def _create_parser_groups(self):
        Component._create_parser_groups(self)
        self._parser_group["Problem Data"] = self.parser.add_argument_group("Problem Data")

        self._parser_group["Problem Data"].add_option("-n", "--network", 
            help="Name of network file", 
            type="string",
            action="store",
            dest="network")


        self._parser_group["Problem Data"].add_option("--impact-dir",  
            help="Specifies the directory the contains impact files.  By " \
                "default the current directory is used.",
            action="store",
            dest="impact_directory",
            type="string",
            default=None)

        self._parser_group["Problem Data"].add_option("--costs", "--costs_ids",
            help="This file contains costs for the installation of sensors " \
                "throughout the distribution network.  This file contains " \
                "id/cost pairs, and default costs can be specified with the " \
                "id: __default__.",
            action="store",
            dest="cost_file",
            type="string",
            default=None)

        self._parser_group["Problem Data"].add_option("--costs-indices",
            help="This file contains costs for the installation of sensors " \
                "throughout the distribution network.  This file contains " \
                "index/cost pairs, and default costs can be specified with the " \
                "index: -1.",
            action="store",
            dest="cost_index_file",
            type="string",
            default=None)

        self._parser_group["Problem Data"].add_option("--sensor-locations",
            help="This file contains information about whether network ids "\
                "are feasible for sensor placement, and whether a sensor placement "\
                "is fixed at a given location.",
            action="store",
            dest="locations_file",
            type="string",
            default=None)

        self._parser_group["Problem Formulation"] = self.parser.add_argument_group("Problem Formulation")

        self._parser_group["Problem Formulation"].add_option("--objective", 
            help="The objective goals are:                            " \
                "....cost   the cost of sensor placement               " \
                "....ec     extent of contamination                    " \
                "....dec    detected extent of contamination           " \
                "....td     time to detection                          " \
                "....dtd    detected time to detection                 " \
                "....mc     mass consumed                              " \
                "....dmc    detected mass consumed                     " \
                "....nfd    number of failed detections                " \
                "....ns     the number of sensors                      " \
                "....pe     population exposed                         " \
                "....dpe    detected population exposed                " \
                "....pk     population killed                          " \
                "....dpk    detected population killed                 " \
                "....pd     population dosed                           " \
                "....dpd    detected population dosed                  " \
                "....vc     volume consumed                            " \
                "....dvc    detected volume consumed                   ",
            default=[],
            type="string",
            action="append",
            dest="objective")

        self._parser_group["Problem Formulation"].add_option("-r", "--responseTime",  
            help="This parameter indicates the number of minutes that are needed to respond to the detection of a contaminant.  As the response time increases, the impact increases because the contaminant affects the network for a greater length of time. Unit: minutes.",
            action="store",
            dest="delay",
            type="int",
            default=0)

        self._parser_group["Problem Formulation"].add_option("--ub-constraint", "--ub",
            help="This option specifies a constraint (<objective>,<ub-value>) " \
                "on the maximal value of an objective type. " \
                "This option can be repeated multiple " \
                "times with different objectives." ,
            action="append",
            dest="constraints",
            type="string",
            default=[])


    def _process_options(self):
        """
        Process problem options
        """
        Component._process_options(self)
        #
        # Error checking
        #
        if len(self.options.objective) < 1:
            print "ERROR: You must specify at least one objective"
            sys.exit(1)
        if len(self.options.constraints) == 0:
            print "ERROR: no constraints specified."
            sys.exit(1)
        if self.options.network is None:
            print "ERROR: must specify the network name"
            sys.exit(1)
        if self.global_options.tmpname is "":
            self.global_options.tmpname=self.options.network
        #
        # Setup data
        #
        if not self.options.impact_directory is None:
            self.impactdir = self.options.impact_directory
        self.nodemap_file = self.impactdir + os.sep + self.options.network + ".nodemap"
        #
        # Initialize
        #
        self.goals=[]
        self.measures={}
        self.objflag={}
        self.allmeasures=[]
        self.bounds={}
        for obj in self.options.objective:
          tmp = obj.split("_")
          if tmp[0] not in self.goals:
             self.goals = self.goals + [ tmp[0] ]
             self.measures[tmp[0]] = []
             self.objflag[tmp[0]] = True
          if tmp[0] == "ns":
             self.measures[tmp[0]] = ["total"]
             self.allmeasures = self.allmeasures + ["total"]
             self.allmeasures = self.allmeasures + ["mean"]
             self.bounds[tmp[0]] = [tmp[0]]
          elif tmp[0] == "cost":
             self.measures[tmp[0]] = ["total"]
             self.allmeasures = self.allmeasures + ["total"]
             self.allmeasures = self.allmeasures + ["mean"]
             self.bounds[tmp[0]] = [tmp[0]]
          elif tmp[0] == "awd":
             self.measures[tmp[0]] = ["total"]
             self.allmeasures = self.allmeasures + ["total"]
             self.allmeasures = self.allmeasures + ["mean"]
             self.bounds[tmp[0]] = [tmp[0]]
          elif len(tmp) == 1:
             self.measures[tmp[0]] = self.measures[tmp[0]] + [self.default_statistic]
             self.allmeasures = self.allmeasures + [self.default_statistic]
             self.bounds[tmp[0]] = [tmp[0]]
          else:
             self.measures[tmp[0]] = self.measures[tmp[0]] + [tmp[1]]
             self.allmeasures = self.allmeasures + [tmp[1]]
             self.bounds[tmp[0]] = [tmp[0]]

        for con in self.options.constraints:
          tmp = (con.split(",")[0]).split("_")
          if len(con.split(",")) <= 1:
             print "ERROR: constraint "+con+" is incorrectly specified; "+\
                   "a \",\" separating the goal and the associated bound is required"
             sys.exit(1)
          bound = con.split(",")[1]
          if tmp[0] not in self.goals:
             self.goals = self.goals + [ tmp[0] ]
             self.measures[tmp[0]] = []
             self.bounds[tmp[0]] = []
             self.objflag[tmp[0]] = False
          if tmp[0] == "ns":
             self.measures[tmp[0]] = ["total"]
             self.allmeasures = self.allmeasures + ["total"]
             self.bounds[tmp[0]] = [bound]
             self.number_of_sensors = int(bound)
          elif tmp[0] == "cost":
             self.measures[tmp[0]] = ["total"]
             self.allmeasures = self.allmeasures + ["total"]
             self.bounds[tmp[0]] = [bound]
          elif tmp[0] == "awd":
             self.measures[tmp[0]] = ["total"]
             self.allmeasures = self.allmeasures + ["total"]
             self.bounds[tmp[0]] = [bound]
          elif len(tmp) == 1:
             self.measures[tmp[0]] = self.measures[tmp[0]] + [self.default_statistic]
             self.allmeasures = self.allmeasures + ["total"]
             self.bounds[tmp[0]] = self.bounds[tmp[0]] + [bound]
          else:
             self.measures[tmp[0]] = self.measures[tmp[0]] + [tmp[1]]
             self.allmeasures = self.allmeasures + [tmp[1]]
             self.bounds[tmp[0]] = self.bounds[tmp[0]] + [bound]

        for goal in self.goals:
          fname=self.impactdir + os.sep + self.options.network + "_" + goal + ".impact"
          if not os.path.isfile(fname):
             print "ERROR: sp failed to open impact file="+fname
             sys.exit(1)
          elif self.objflag[goal] == True:
             self.impact_file[goal] = fname
             [self.num_nodes, valid_delay, self.num_lines] = self._read_impact_data(fname, goal, self.options.delay)
             if not valid_delay:
                print "ERROR: impact file has an invalid delay value!"
                sys.exit(1)

          if self.objflag[goal] == True:
            if self.threshold.has_key(goal) and (self.threshold[goal] == -1):
              continue
            self.all_aggregation_disabled = False
            break


    def printParameters(self):
        """
        Print the parameters of the problem to be solved.
        """
        print ""
        print "Number of Nodes                : " + `self.num_nodes`
        print "Number of Contamination Impacts: " + `(self.num_lines - 2)`
        print ""

        if self.options.debug == True:
            print "Network:    ", self.options.network
            print "Goals:      ", self.goals
            print "Objectives: ", self.options.objective
            print "Delay:      ", self.options.delay
            print "ImpactDir:  ", self.impactdir
            print "Costs:      ", self.options.cost_index_file

    def setup(self):
        self._processInputFiles()

    def _processInputFiles(self):
        """
        Read and process various input files.
        """
        if self.options.cost_file != None or self.options.locations_file != None:
            self.junction_map = self._read_node_map(self.nodemap_file)

        if self.options.locations_file != None:
            [self.invalid_vals, self.fixed_vals] = read_sensor_locations(self.options.locations_file, self.junction_map)
        if self.global_options.debug:
            print "Number of fixed sensor locations:", len(self.fixed_vals)
            print "Number of invalid sensor locations:", len(self.invalid_vals)
            print "Fixed:      ", self.fixed_vals
            print "Invalid:    ", self.invalid_vals

        if self.options.cost_index_file != None:
            self.cost_map = read_costs(self.options.cost_index_file)
        elif self.options.cost_file != None:
            tmp = read_costs(self.options.cost_file)
            self.cost_map = {}
            for val in tmp:
                if val == "__default__":
                    self.cost_map[-1] = tmp[val]
                else:
                    self.cost_map[self.junction_map[val]] = tmp[val]
            else:
                self.cost_map = {}

    def _read_impact_data(self, filename, goal, delay):
        num_nodes = 0
        valid_delay=False
        got_num_nodes=False
        # We look for several files, but only save num_nodes & num_lines
        # from the first found.  What's the right thing to do? TODO
        if not os.path.isfile(filename):
            print "ERROR: sp failed to open impact file="+filename
            sys.exit(1)
        INPUT = open(filename,"r")
        line = INPUT.readline()
        line = line.strip()
        num_nodes = eval(line.split(" ")[0])
        line = INPUT.readline()
        line = line.strip()
        vals = line.split(" ")
        for word in vals[1:]:
          if delay == eval(word):
            valid_delay=True
            break
        INPUT.close()
        num_lines = pyutilib.misc.count_lines(filename)
        return [num_nodes, valid_delay, num_lines]


def read_costs(fname):
  INPUT = open(fname,"r")
  result = {}
  for line in INPUT.xreadlines():
    piece = re.split('[ \t]+', line.strip())
    #print piece[0],piece[1]
    result[ piece[0].strip() ] = eval(piece[1])
  INPUT.close()
  return result

def read_node_map(fname):
  INPUT = open(fname)
  jmap={}
  for line in INPUT.xreadlines():
    vals = line.split(" ");
    vals[1] = vals[1].strip()
    #print ":" + vals[1] + ":" + vals[0] + ":"
    jmap[vals[1]] = vals[0]
  INPUT.close()
  return jmap

def read_sensor_locations(fname, junction_map):
  INPUT = open(fname,"r")
  junctionstatus = {}
  fixed=[]
  invalid=[]
  for key in junction_map.keys():
    junctionstatus[ junction_map[key] ] = "fu"  # feasible-unfixed

  for line in INPUT.xreadlines():
    pieces = re.split('[ \t]+', line.strip())
    if pieces[0] == "#":
       continue
    if len(pieces) < 2:
       print "ERROR: bad format for sensor_locations input file!"
       print "ERROR: line missing sensor locations: " + line
       sys.exit(1)
    #
    # Figure out the setting for this line
    #
    if pieces[0] == "fixed":
       status = "ff"
    elif pieces[0] == "unfixed":
       status = "fu"
    elif pieces[0] == "feasible":
       status = "fu"
    elif pieces[0] == "infeasible":
       status = "iu"
    else:
       print "ERROR: bad sensor_location value"
       sys.exit(1)
    #
    # Apply the setting to all locations
    #
    for piece in pieces[1:]:
      if (piece == "_ALL_") or (piece == "ALL") or (piece == "*"):
         for key in junctionstatus.keys():
           junctionstatus[key] = status
      elif piece not in junction_map.keys():
         print "ERROR: Bad junction ID in sensor locations file: " + piece
         sys.exit(1)
      else:
         junctionstatus[junction_map[piece]] = status
  #
  # Setup the invalid_vals and fixed_vals arrays
  #
  for key in junctionstatus.keys():
    if junctionstatus[key] == "iu":
       invalid = invalid + [ eval(key) ]
    elif junctionstatus[key] == "ff":
       fixed = fixed + [ eval(key) ]
  invalid.sort()
  fixed.sort()

  return [invalid, fixed]

