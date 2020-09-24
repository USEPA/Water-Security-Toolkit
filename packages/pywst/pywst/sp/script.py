#
# 1sp - sensor placement solver
#
#  This script is known to work with Python 2.5.  It is known
#  to fail with Python 2.3.4.

#  To add a command line option:  Look at the parser.add_option
#   statements.  Read the comment at "How to add options:" for 
#   more help.  Process the command line option and write it to
#   the problem object in SPProblem::processOptions().
#
#  To add a solver subclass:  Look at "class simpleSolver" for an
#   an explanation of what methods and variables need to be
#   defined by a solver object.  Instantiate your solver after
#   the parse_args call.  Add your solver to the solver_help message.
#
# WARNING:
#  At the start of a problem, cleanupOutputFiles() removes every
#  file that ends in .log, .dat, .lag and .config.

import sys 
import os
import glob
from os.path import abspath, dirname, exists, join

#
# Find paths to python modules if we are in a spot installation
#

currdir=dirname(abspath(__file__))
acro_py_dir=join(dirname(currdir),"python")
if exists(acro_py_dir):
  sys.path.insert(0, acro_py_dir)
  eggDirs =glob.glob(join(acro_py_dir,"*.egg"))
  for _dir in eggDirs:
    try:
      files = os.listdir(_dir)
      sys.path.insert(0, _dir)
    except OSError:   # it's not a directory
      pass

import signal 
import atexit
import tempfile
import commands
import shutil
import string
import time
import re
from optparse import OptionParser

from pyutilib.subprocess import SubprocessMngr, run_command
from pyutilib.misc import search_file
from pywst.spot.loc_map import location_map
import pywst.spot.pyunit

import pyutilib.services

####################################################################
# Functions

def count_lines(file):
  count = 0
  for line in open(file,"r"):
    count = count + 1
  return count

def spawn_command(command, use_timelimits=False, stdout_file=None):
  """ Run a command in a subprocess

  Run command and return a tuple containing stdout and 
  stderr (concatenated) of command, and exit code of command.
 
  If use_timelimits is True, then the solver's updateSolution 
  method will be called at every notification interval, and if
  there is a time limit, the command will be killed after the 
  time limit runs out.
 
  If stdout_file is not None, it is the name of a file to which
  the command's stdout stream will be written.  The first item 
  returned in the tuple will be that open file object.  The
  output is unbuffered, so solver can read it in updateSolution
  while command is still running.
 
  """

  global options
  if options.debug:
        print "Trying to execute this command: "+command

  command = command.strip()
  
  cmdout = None
  #fname = stdout_file
  #if fname == None:
    #fname = "tmp.txt"

  timelimit = None
  notify = 0
  exitcode = None
  pollInterval=5 # something small for checking if the process is done
  if use_timelimits == True:
    if problem.notifyInterval > 0:
      notify = problem.notifyInterval * 60
      if problem.timelimit > 0:
        timelimit = problem.timelimit * 60

  ans = pyutilib.subprocess.run(command, outfile=stdout_file, timelimit=timelimit)
  return [ans[1], ans[0]]


def system_call(command,outfile=""):
  if outfile != "":
    res = run_command(command, outfile=outfile)
  else:
    res = run_command(command)
  return 0


def executable_exists(output_file):
  if sys.platform[0:3] == "win":
     fname = output_file + ".exe"
  elif sys.platform[0:5] == "linux" or sys.platform == "cygwin":
     fname = output_file
  else:
     raise RuntimeError("ERROR: unknown platform " + sys.platform)
  if os.access(fname, os.X_OK):
    return True
  else:
    return False


####################################################################
# Create a parser for all possible command line options.
#
# How to add options:
# 
# Options are added with the "add_option" method.
#
# You can list as many option strings for an option as you like.
# The option strings must conform to GNU option standards:
#
#   parser.add_option("-f", "--filename", ...)
#
# Attributes of the option appear in any order after the option strings.
#
# Default attributes are:
#   type is "string"
#   action is "store"
#   dest is the first long option name ("filename" in the example above)
#   default is None
#
#   so: parser.add_option("-f", "--filename", "--fn",
#                         help="Name of input file", 
#                         type="string", action="store", 
#                         dest="filename", default="/dev/null")
#
# "dest" is the name of the field in which the option value will be stored.
# It's used in the automatically generated help message, so make it
# descriptive.
#
# Action "store" means the value entered on the command line for the 
# option will be stored in options.{dest}.
#
# For options that can be listed more than once, action should be "append".
# (Then options.{dest} is a list.)
#
# For boolean options, action should be "store_true" or "store_false".
#
# For non-boolean options, use these types for the "type" attribute: 
#   "string", "int", "float", "long", "complex"
#
# Values of unset options are None unless you provide a default.
#
# In help messages, embedded newlines and preceding spaces are stripped 
# out, and message is formatted with a 54 character width.  So if you
# want new lines, pad lines with spaces to 54 characters, and if you want
# to indent, then pad with a character to indent.
#
#  http://docs.python.org/lib/module-optparse.html
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
# Lengthier help strings.  Shorter ones are in the add_option call.
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
objective_help=\
"Objective names have the form: <goal>_<statistic>     " \
"..The objective goals are:                            " \
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
"....dvc    detected volume consumed                   " \
"..The objective statistics are:                       " \
"....mean   the mean impact                            " \
"....median the median impact                          " \
"....var    value-at-risk of impact distribution       " \
"....tce    tail-conditioned expectation of imp dist   " \
"....cvar   approximation to TCE used with IPs         " \
"....worst  the worst impact                           " \
"An objective name of the form <goal> is assumed to " \
"refer to the objective <goal>_mean.  This option may "\
"be listed more than once.                                "

gamma_help=\
"Specifies the fraction of the distribution of impacts "\
"that will be used to compute the var, cvar and tce "\
"measures.  Gamma is assumed to lie in the interval "\
"(0,1].  It can be interpreted as specifying the "\
"100*gamma percent of the worst contamination incidents " \
"that are used for these calculations.  Default: .05 "
   
greedy_help=\
"Tells evalsensor to output the greedy sensor ranking"
   
scfile_help=\
"Specifies the name of a file defining detection "\
"probabilities for all sensor categories. Used with the "\
"imperfect-sensor model.  Must be specified in "\
"conjunction with the --imperfect-jcfile option."

jcfile_help=\
"Specifies the name of a file defining a sensor " \
"category for each network junction. Used with the " \
"imperfect-sensor model. Must be specified in "\
"conjunction with the --imperfect-scfile option."

numsamples_help=\
"Specifies the number of candidate solutions generated " \
"by the grasp heuristic. Defaults vary based on " \
"statistic and sensor model formulation (perfect vs. imperfect)."

grasp_representation_help=\
"Specifies whether the grasp heuristic uses a sparse " \
"matrix (0) or dense matrix (1) representation to store " \
"the impact file contents.  The default is 1."

impact_dir_help=\
"Specifies the directory the contains impact files.  By " \
"default the current directory is used."

threshold_help=\
"Specifies the value (as `<goal>,<value>') used to aggregate `similar' " \
"impacts.  This is used to reduce the total size of the " \
"sensor placement formulation (for large problems). The " \
"solution generated with non-zero thresholds is not " \
"guaranteed to be globally optimal."

percent_help=\
"A `<goal>,<value>' pair where value is a double between 0.0 and 1.0.  This is an " \
"alternate way to compute the aggregation threshold. " \
"Over all contamination incidents, we compute the " \
"maximum difference d between the impact of the " \
"contamination incident is not detected and the impact " \
"it is detected at the earliest possible feasible " \
"location.  We set the threshold " \
"to d * aggregation_percent.  If both threshold and percent are " \
"set to valid values in the command line, percent takes priority."

ratio_help=\
"A `<goal>,<value>' pair where value is a double between 0.0 and 1.0. "
"This limits the aggregation "
"by ensuring that for each block aggregated the ratio "
"(lowest impact)/(highest impact) must be greater than or equal to this "
"value.  The default value of this ratio is 1.0.  Setting this option "
"to a value less than 1.0 only has an impact if the aggregation "
"percent or threshold has been set to a non-zero value.  Otherwise "
"the default zero-threshold aggregation is used, which is not "
"impacted by this option."

distinguish_help=\
"A goal for which aggregation should not allow incidents " \
"to become trivial." \
"That is, if the threshold is so large that all " \
"locations, including the dummy, would form a single superlocation, " \
"this forces the dummy to be in a superlocation by itself.  Thus the " \
"sensor placement will distinguish between detecting and not detecting. " \
"This option can be listed multiple times, to specify multiple goals."\
"Note: the `detected' impact measures (e.g. dec, dvc) are always distinguished."

conserve_memory_help=\
"If location aggregation is chosen, and the original impact files are " \
"very large, you can choose to process them in a memory "\
"conserving mode.  For example \"--conserve_memory=10000\" requests "\
"that while original impact files are being processed into smaller "\
"aggregated files, no more than 10000 impacts should be read into memory "\
"at any one time.  Default is 10000 impacts.  Set to 0 to turn this off."

disable_aggregation_help=\
"Disable aggregation for this goal, even at value zero, which "\
"would incur no error.  Each witness incident will be in a separate "\
"superlocation. "\
"This option can be listed multiple times, to specify multiple goals. "\
"You may list the goal `all' to specify all goals."

ub_constraint_help=\
"This option specifies a constraint (<objective>,<ub-value>) " \
"on the maximal value of an objective type. " \
"This option can be repeated multiple " \
"times with different objectives." 

cost_help=\
"This file contains costs for the installation of sensors " \
"throughout the distribution network.  This file contains " \
"id/cost pairs, and default costs can be specified with the " \
"id: __default__." \

cost_index_help=\
"This file contains costs for the installation of sensors " \
"throughout the distribution network.  This file contains " \
"index/cost pairs, and default costs can be specified with the " \
"index: -1." \

locations_help=\
"This file contains information about whether network ids "\
"are feasible for sensor placement, and whether a sensor placement "\
"is fixed at a given location."

solver_help=\
"This option specifies the type of solver that is used "\
"to find sensor placement(s).  The following solver "\
"types are currently supported:                        "\
"..att_grasp  multistart local search heuristic (AT&T) "\
"..snl_grasp  TEVA-SPOT license-free grasp clone       "\
"..lagrangian lagrangian relaxation heuristic solver   "\
"..pico       mixed-integer programming solver (PICO)  "\
"..glpk       mixed-integer programming solver (GLPK)  "\
"..picoamp    MIP solver with AMPL                     "\
"..cplexamp   commercial MIP solver                    "\
"The default solver is snl_grasp.                      "

solver_options_help=\
"This option contains solver-specific options for controlling "\
"the sensor placement solver.  The options are added to the solver command line."

runtime_help=\
"Terminate the solver after the specified number of wall clock "\
"minutes have elapsed.  By default, no limit is placed on the "\
"runtime.  Some solvers can provide their best solution so far at the point of termination."

notify_help=\
"Some solvers can output preliminary solutions while they are running. "\
"This option supplies the interval in minutes at which candidate solutions "\
"should be printed out. " 

tmp_file_help=\
"Name of temporary file prefix used in this computation. "\
"The default name is `<network-name>'."

output_help=\
"Name of the output file that contains the sensor placement. "\
"The default name is `<network-name>.sensors'."

summary_help=\
"Name of the output file that contains summary information "\
"about the sensor placement."

path_help=\
"Add this path to the set of paths searched "\
"for executables (sp executables, valgrind, etc) "\
"and IP models.  Use multiple times to add multiple "\
"paths for searching. (for example "\
"--path=../mypath --path=otherpath/foo). sp will use the "\
"first valid executable or GeneralSP.mod found in this order: "\
" 1) a path listed with a -- path option. If there are multiple valid "\
"paths in the list, the first one, 2) the location in a standard spot "\
"distribution relative to the currently invoked sp, 3) for executables "\
"the PATH environment variable and for mod files the current working directory."

amplcplexpath_help=\
"Look for ampl and cplexamp executables in this directory. "\
"If this path is not specified, sp looks for these executables "\
"in the --path options, then the spot bin directory for the " \
"currently invoked sp, and then to the user's system path."

picopath_help=\
"Look for the PICO executable in this directory. "\
"If this is not specified, sp looks uses the first PICO executable" \
"in any path specified by "\
"the --path option, then to the default location in an acro-pico " \
"check out in the same directory as spot, then to the spot bin directory for the "\
"currently invoked sp command, then to the user's system path."

glpkpath_help=\
"Look for the GLPK executable in this directory. "\
"This defaults to the path used for executables specified by "\
"the --path option, then to the spot bin directory for the "\
"currently invoked sp command, then to the user's system path.."

ampl_help="The name of the ampl executable (this defaults to `ampl')."

ampldata_help=\
"An auxillary AMPL data file.  This option is used when "\
"integrating auxillary information into the AMPL IP model."

amplmodel_help=\
"An alternative AMPL model file.  This option is used when "\
"applying a non-standard AMPL model for solving sensor "\
"placement with an IP."

seed_help=\
"The value of a seed for the random number generator used "\
"by the solver.  This can be used to ensure a deterministic, "\
"repeatable output from the solver. Should be >= 1."

eval_all_help=\
"This option specifies that all impact files found will be used to "\
"evaluate the final solution(s)."

memcheck_help=\
"This option indicates that valgrind should run on one "\
"or more executables.                                  "\
"..all               run on all executables            "\
"..solver            run on the solver executable "\
"..createIPData       run on createIPData           "\
"..preprocessImpacts run on preprocessImpacts     "\
"..evalsensor        run on evalsensor             "\
"..aggregateImpacts  run on aggregateImpacts  "\
"Output will be written to memcheck.{name}.{pid} .       "

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

parser = OptionParser()
parser.usage = "sp [options]"

parser.add_option("-n", "--network", 
              help="Name of network file", 
              type="string",   action="store", dest="network")

parser.add_option("--objective", 
              help=objective_help, default=[],
              type="string",   action="append", dest="objective")

parser.add_option("-r", "--responseTime",  
          help="This parameter indicates the number of minutes that are needed to respond to the detection of a contaminant.  As the response time increases, the impact increases because the contaminant affects the network for a greater length of time. Unit: minutes.",
          action="store", dest="delay", type="int", default=0)

parser.add_option("-g", "--gamma",  
          help=gamma_help,
          action="store", dest="gamma", type="float", default=0.05)

parser.add_option("--compute-greedy-ranking",  
          help=greedy_help,
          action="store_true", dest="compute_greedy_ranking", default=False)

parser.add_option("--incident-weights",  
          help="This parameter specifies a file that contains the weights for contamination incidents.  This file supports the optimization of weighted impact metrics.  By default, incidents are optimized with weight 1.0",
          action="store", dest="incident_weights", type="string", default=None)

parser.add_option("--imperfect-scfile",  
          help=scfile_help,
          action="store", dest="scfile", type="string")

parser.add_option("--imperfect-jcfile",  
          help=jcfile_help,
          action="store", dest="jcfile", type="string")

parser.add_option("--num", "--numsamples",  
          help=numsamples_help,
          action="store", dest="numsamples", type="int", default=-1)

parser.add_option("--grasp-representation",
          help=grasp_representation_help, default=1,
          action="store", dest="grasp_representation", type="int")

parser.add_option("--impact-dir",  
          help=impact_dir_help, 
          action="store", dest="impact_directory", type="string")

parser.add_option("--aggregation-threshold","--threshold",
          help=threshold_help,
          action="append", dest="aggregation_threshold", type="string")

parser.add_option("--aggregation-percent","--percent",
          help=percent_help,
          action="append", dest="aggregation_percent", type="string")

parser.add_option("--aggregation-ratio",
          help=ratio_help,
          action="append", dest="aggregation_ratio", type="string")

parser.add_option("--conserve-memory",
          help=conserve_memory_help, default=10000,
          action="store", dest="maximum_impacts", type="int")

parser.add_option("--distinguish-detection","--no-event-collapse",
          help=distinguish_help, default=[],
          action="append", dest="distinguish_goal", type="string")

parser.add_option("--disable-aggregation",
          help=disable_aggregation_help,
          action="append", dest="disable_aggregation", type="string")

parser.add_option("--ub-constraint", "--ub",
          help=ub_constraint_help, default=[],
          action="append", dest="ub_constraint", type="string")

parser.add_option("--baseline-constraint", "--baseline",
          help="Baseline constraints are not currently supported.",
          action="append", dest="baseline_constraint", type="string")

parser.add_option("--reduction-constraint", "--reduction",
          help="Reduction constraints are not currently supported.",
          action="append", dest="reduction_constraint", type="string")

parser.add_option("--costs", "--costs_ids",
          help=cost_help, 
          action="store", dest="cost_file", type="string")

parser.add_option("--costs-indices",
          help=cost_index_help, 
          action="store", dest="cost_index_file", type="string")

parser.add_option("--sensor-locations",
          help=locations_help, 
          action="store", dest="locations_file", type="string")

parser.add_option("--solver",
          help=solver_help,
          action="store", dest="solver", type="string", default="snl_grasp")

parser.add_option("--solver-options",
          help=solver_options_help,
          action="store", dest="solver_options", type="string")

parser.add_option("--runtime",
          help=runtime_help,
          action="store", dest="runtime", type="int", default=-1)

parser.add_option("--notify",
          help=notify_help,
          action="store", dest="interval", type="int", default=-1)

parser.add_option("--compute-bound",
          help="Only compute a bound on the value of the optimal solution.",
          action="store_true", dest="compute_bound", default=False)

parser.add_option("--memmon",
          help="Summarize the maximum memory used by any of the executables",
          action="store_true", dest="memmon", default=False)

parser.add_option("--memcheck",
          help=memcheck_help,
          type="string", action="store", dest="memchecktarget", default=None)

parser.add_option("--tmp-file",
          help=tmp_file_help, default="",
          action="store", dest="tmp_file", type="string")

parser.add_option("-o", "--output",
          help=output_help, default="",
          action="store", dest="output_file", type="string")

parser.add_option("--summary",
          help=summary_help, default="",
          action="store", dest="summary_file", type="string")

parser.add_option("--format",
          help="Format of the summary information",
          action="store", dest="format", type="string", default="cout")

parser.add_option("--print-log",
          help="Print the solver output",
          action="store_true", dest="print_log", default=False)

parser.add_option("--path",
          help=path_help,
          action="append", dest="path", type="string", default=[])

parser.add_option("--amplcplexpath",
          help=amplcplexpath_help, default=None, 
          action="store", dest="amplcplexpath", type="string")

parser.add_option("--picopath",
          help=picopath_help,
          action="store", dest="picopath", type="string")

parser.add_option("--glpkpath",
          help=glpkpath_help,
          action="store", dest="glpkpath", type="string")

parser.add_option("--ampl",
          help=ampl_help, default="ampl",
          action="store", dest="ampl", type="string")

parser.add_option("--ampldata",
          help=ampldata_help, 
          action="store", dest="ampldata", type="string")

parser.add_option("--amplmodel",
          help=amplmodel_help, default="GeneralSP.mod",
          action="store", dest="amplmodel", type="string")

parser.add_option("--seed",
          help=seed_help,
          action="store", dest="seed", type="int", default=1)

parser.add_option("--eval-all",  
          help=eval_all_help,
          action="store_true", dest="eval_all", default=False)

parser.add_option("--debug",  
          help="List status messages while processing.",
          action="store_true", dest="debug", default=False)

parser.add_option("--gap",  
          help="TODO gap help string.",
          action="store", dest="gap", default=0)

parser.add_option("--version",  
          help="Print version information for the compiled executables used by this command and exit.",
          action="store_true", dest="version", default=False)

#####################################################################
# The problem to be solved
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

class SPProblem:
  "An object that encapsulates the problem to be solved."

  def __init__(self):
    "initialize here"

  def cleanupOutputFiles(self):
    """ Remove all files created to solve a previous problem.

    """
    if os.path.isfile(self.output_file):
      os.remove(self.output_file);
    fls = os.listdir(".")
    pat1=re.compile("[\w.]+.dat$")
    pat2=re.compile("[\w.]+.log$")
    pat3=re.compile("[\w.]+.config$")
    pat4=re.compile("[\w.]+.lag$")
    for fl in fls:
      if pat1.match(fl) or pat2.match(fl):
        os.remove(fl)
      elif pat3.match(fl) or pat4.match(fl): 
        os.remove(fl)
  
  def processOptions(self):
    """ Process command line options.

    Process all command line arguments and write them to the problem
    object.  Do verifications that are not solver specific.
    We assume the solver object has already been instantiated.
    """
    self.objectives = []
    for obj in options.objective:
        if not obj in self.objectives:
            self.objectives.append(obj)
    self.network = options.network
    self.delay = options.delay
    self.gamma = options.gamma
    self.compute_greedy_ranking=options.compute_greedy_ranking
    self.imperfect_scfile = options.scfile
    self.imperfect_jcfile = options.jcfile
    self.numsamples = options.numsamples
    self.grasp_representation = options.grasp_representation
    self.impactdir = options.impact_directory
    self.distinguish_detection = options.distinguish_goal + ["dec","dmc","dvc","dtd","dpk","dpe","dpd"]
    self.constraints=options.ub_constraint
    self.costs_name = options.cost_file 
    self.costs = options.cost_index_file 
    self.sensor_locations = options.locations_file
    self.solver_options = options.solver_options
    self.solver = options.solver
    self.timelimit = options.runtime
    self.notifyInterval = options.interval
    self.debug = options.debug
    self.compute_bound = options.compute_bound
    self.gap = options.gap
    self.tmpname = options.tmp_file
    self.output_file = options.output_file
    self.summary_file = options.summary_file
    self.printlog = options.print_log
    self.format_str = options.format
    self.seed = options.seed
    self.maximum_impacts = options.maximum_impacts
    self.eval_all = options.eval_all
    self.amplname = options.ampl
    self.ampldata = options.ampldata
    self.amplmodel = options.amplmodel
  
    self.threshold={}
    self.aggregation_percent={}
    self.aggregation_ratio={}
    self.all_aggregation_disabled = True
    self.memmon=""
    self.memcheck=None
    self.valgrindCommand=None

    # The path where this sp script is residing upon invocation.
    # This should be OS independent, at least for "normal" invocations.
    # This should *not* be the current working directory os.getcwd().  That is
    # the place where sp was invoked from, usually some directory where running tests
    # for a paper or comparing sensor designs for a city.
    ##self.sp_path=sys.path[0]
    ##self.sp_path = self.sp_path.strip()
    self.spot_path = pywst.spot.pyunit.topdir
    ##self.binpath = None

    self.nodemap_file=""
    self.imperfect_model=False

    self.goals = []
    self.measures = {}
    self.bounds = {}
    self.objflag = {}
    self.allmeasures = []
    self.number_of_sensors = 0

    self.data_dir = os.getcwd();
  
    if "median" in self.objectives:
      raise RuntimeError("ERROR: the objective statistic `median' is not currently allowed")
    elif len(self.objectives) < 1:
      raise RuntimeError("ERROR: You must specify an objective.")
  
    # If they chose both --aggregation-threshold and --disable-aggregation
    # on the same objective, we disable.

    if options.aggregation_threshold != None:
      for option in options.aggregation_threshold:
        tmp = option.split(",")
        if len(tmp) < 2:
          raise RuntimeError("ERROR: --aggregation_threshold=goal,amt or --threshold=goal,amt\n" + threshold_help)
        self.threshold[tmp[0]] = tmp[1]
  
    if self.solver == "lagrangian" and options.disable_aggregation is None:
        options.disable_aggregation = ["all"]

    if options.disable_aggregation != None:
      if options.disable_aggregation[0] == "all":
        for goal in valid_objectives:
          self.threshold[goal] = -1.0
      else:
        for goal in options.disable_aggregation:
          if not goal in valid_objectives:
            raise RuntimeError("ERROR: --disable_aggregation=goal\n" + disable_aggregation_help)
          self.threshold[goal] = -1.0
  
    if options.aggregation_percent != None:
      for option in options.aggregation_percent:
        tmp = option.split(",")
        if len(tmp) < 2:
          raise RuntimeError("ERROR: --aggregation_percent=goal,amt or --percent=goal,amt\n" + percent_help)
        self.aggregation_percent[tmp[0]] = tmp[1]
  
    if options.aggregation_ratio != None:
      for option in options.aggregation_ratio:
        tmp = option.split(",")
        if len(tmp) < 2:
          raise RuntimeError("ERROR: --aggregation_ratio=goal,amt\n" + ratio_help)
        self.aggregation_ratio[tmp[0]] = tmp[1]
  
    if options.baseline_constraint != None:
      raise RuntimeError("ERROR: baseline constraints are not currently supported")
  
    if options.reduction_constraint != None:
      raise RuntimeError("ERROR: reduction constraints are not currently supported")
  
    if options.memmon == True:
      self.memmon=" -v "

    if options.memchecktarget != None:
      valid_memcheck_choices= \
        ["preprocessimpacts", "aggregateImpacts", "setupipdata", "evalsensor", "solver", "all"]
      problem.memcheck = options.memchecktarget
      problem.memcheck.lower()
      if not problem.memcheck in valid_memcheck_choices:
        raise RuntimeError("ERROR: invalid memcheck option/nERROR: choose from ",valid_memcheck_choices)

      valgrindExecutable = pyutilib.services.registered_executable('valgrind')
      if valgrindExecutable == None:
            raise RuntimeError("Executable valgrind is not found.")

      problem.valgrindCommand=\
            valgrindExecutable.get_path() +" -v --tool=memcheck --log-file-exactly="+\
            "memcheck.xxxx."+`pid`+" --trace-children=yes "

    # We'll set up the modpath now, even if we aren't using integer programs in this call
    # because the modpath may be altered by the fixCygwinPaths call if this is running under cygwin.
    # We look for GeneralSP.mod in order
    # 1) The locations specified in the commandline using --path.  Use the first one.
    # 2) Otherwise, the default relative to the currently invoked sp script
    # 3) Otherwise the current working directory
    # Warning: this is just based on GeneralSP.mod.  If there are ever other relevant mod files, this
    # should change to allow those files to be in different places.
    self.modpath = None;
    if options.path != None:
      for path in options.path:
        if os.path.isfile(join(path, "GeneralSP.mod")):
           self.modpath = path
    if self.modpath == None:
      if os.path.isfile(join(self.spot_path,"etc","mod","GeneralSP.mod")):
        self.modpath=join(self.spot_path,"etc","mod")
      else:
        self.modpath=os.getcwd()

    if self.impactdir == None:
       self.impactdir = os.getcwd();
    self.impactdir = self.impactdir.strip()

    if self.memmon != "":
        executable = pyutilib.services.registered_executable('memmon')
        if executable is None:
            raise RuntimeError("ERROR: memmon is not available")
        else:
            self.memmon = executable.get_path() + self.memmon

    if sys.platform == "cygwin":
      solver.fixCygwinPaths()

    if self.network == None:
       raise RuntimeError("ERROR: network name is unspecified.")
  
    if self.tmpname == "":
      self.tmpname = self.network
    if self.output_file == "":
      self.output_file = self.network + ".sensors"

    self.nodemap_file = join(self.impactdir, self.network+".nodemap")

    if (self.gamma <= 0.0) or (self.gamma > 1.0):
       raise RuntimeError("ERROR: gamma must be in the interval (0,1].")

    if len(self.constraints) == 0:
       raise RuntimeError("ERROR: no constraints specified.")

    if self.timelimit!=-1:
      if self.timelimit<=0:
        raise RuntimeError("ERROR: The time limit must be > 0 minutes")
      if self.notifyInterval==-1:
        # Need to notify just before killing solver
        self.notifyInterval = self.timelimit

    if self.notifyInterval!=-1:
      if (self.notifyInterval<=0):
        raise RuntimeError("ERROR: The time limit must be > 0 minutes")

    for obj in self.objectives:
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
         self.measures[tmp[0]] = self.measures[tmp[0]] + ["mean"]
         self.allmeasures = self.allmeasures + ["mean"]
         self.bounds[tmp[0]] = [tmp[0]]
      else:
         self.measures[tmp[0]] = self.measures[tmp[0]] + [tmp[1]]
         self.allmeasures = self.allmeasures + [tmp[1]]
         self.bounds[tmp[0]] = [tmp[0]]

    for con in self.constraints:
      tmp = (con.split(",")[0]).split("_")
      if len(con.split(",")) <= 1:
         raise RuntimeError("ERROR: constraint "+con+" is incorrectly specified; "+\
               "a \",\" separating the goal and the associated bound is required")
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
         self.measures[tmp[0]] = self.measures[tmp[0]] + ["mean"]
         self.allmeasures = self.allmeasures + ["total"]
         self.bounds[tmp[0]] = self.bounds[tmp[0]] + [bound]
      else:
         self.measures[tmp[0]] = self.measures[tmp[0]] + [tmp[1]]
         self.allmeasures = self.allmeasures + [tmp[1]]
         self.bounds[tmp[0]] = self.bounds[tmp[0]] + [bound]

    for goal in self.goals:
      if self.objflag[goal] == True:
        if self.threshold.has_key(goal) and (self.threshold[goal] == -1):
          continue
        self.all_aggregation_disabled = False
        break
    #print "HERE",self.goals, self.all_aggregation_disabled, self.threshold

  def print_summary(self, solver):
        if len(string.split(problem.objectives[0],"_")) == 1:
            problem.objectives[0] = problem.objectives[0] + "_mean"
        objective=string.strip(string.split(problem.objectives[0],"_")[0])
        statistic=string.strip(string.split(problem.objectives[0],"_")[1])
        constraint=""

        if "ns" in problem.bounds:
          constraint = "Number of sensors="+problem.bounds["ns"][0]
        elif "cost" in problem.bounds:
          constraint = "Cost="+problem.bounds["cost"][0]
        else:
          raise RuntimeError("Error in print_summary")

        impactfile=join(problem.impactdir, problem.network+"_"+objective)
        if solver.aggregate_impact_files == True:
            impactfile=impactfile+"_agg.impact"
        else:
            impactfile=impactfile+".impact"

        print constraint
        print "Objective="+objective
        print "Statistic="+statistic
        print "Impact file="+impactfile
        print "Delay="+`problem.delay`
        print ""

#####################################################################
# Functions to read/write files
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

def read_impact_files():
  num_nodes = 0
  valid_delay=False
  got_num_nodes=False
  # We look for several files, but only save num_nodes & num_lines
  # from the first found.  What's the right thing to do? TODO
  for goal in problem.goals:
    if (goal != "ns") and (goal != "cost") and (goal != "awd"):
      fname=join(problem.impactdir, problem.network+"_"+goal+".impact")
      print "read_impact_files: ",fname
      if not os.path.isfile(fname):
        raise RuntimeError("ERROR: sp failed to open impact file="+fname)
      if got_num_nodes == False:
        INPUT = open(fname,"r")
        line = INPUT.readline()
        line = line.strip()
        num_nodes = eval(line.split(" ")[0])
        line = INPUT.readline()
        line = line.strip()
        vals = line.split(" ")
        for word in vals[1:]:
          if problem.delay == eval(word):
            valid_delay=True
            break
        INPUT.close()
        num_lines = count_lines(fname)
        got_num_nodes = True

  return [num_nodes, valid_delay, num_lines]

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
       raise RuntimeError("ERROR: bad format for sensor_locations input file!/nERROR: line missing sensor locations: " + line)
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
       raise RuntimeError("ERROR: bad sensor_location value")
    #
    # Apply the setting to all locations
    #
    for piece in pieces[1:]:
      if (piece == "_ALL_") or (piece == "ALL") or (piece == "*"):
         for key in junctionstatus.keys():
           junctionstatus[key] = status
      elif piece not in junction_map.keys():
         raise RuntimeError("ERROR: Bad junction ID in sensor locations file: " + piece)
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

def read_costs(fname):
  INPUT = open(fname,"r")
  result = {}
  for line in INPUT.xreadlines():
    piece = re.split('[ \t]+', line.strip())
    #print piece[0],piece[1]
    result[ piece[0].strip() ] = eval(piece[1])
  INPUT.close()
  return result

def getSACachedSolutions(fname):
  """ Read in solutions written by a solver using SACache

  The SACache class manages a file of solutions.  Here we
  read that file and write to the solutions, attributes
  and responses lists.  If SACache changes the way the file
  is written, we need to rewrite this function.  But wrapping
  the SACache C++ class for python seems like overkill for
  this small task.
  
  Return 0 if no solutions were found.
  Return 1 if solutions were found.
  """
       
  try:
    f = open(fname, "r")
  except IOError:
    # No solutions written yet, not an error
    return 0

  solutions = []
  attributes = []
  responses = []
  attrTypes = {}

  lines = f.readlines()
  f.close()
  numAttrs = int(lines[0].strip())

  for i in range(2, 2+numAttrs):
    words = lines[i].split()
    attrTypes[words[0].strip()] = words[1].strip()

  nextLine = numAttrs + 3

  numSolns = int(lines[nextLine].strip())

  if numSolns == 0:
    return 0

  nextLine = nextLine + 2

  while nextLine < len(lines):
    soln = lines[nextLine + 1].strip()
    resp = float(lines[nextLine + 2].strip())
    numAttrs = int(lines[nextLine + 3].strip())

    solns = soln.split(" ")[1:]
    solnList = []
    for soln in solns:
      solnList.append(int(soln))
    solutions.append(solnList)

    # Eventually we may record more than one type of response
    # That's why we have a list of (only 1) tuples.
    responses.append([("response",resp)])

    nextLine = nextLine + 4

    attrList = []
    cacheAttrs = lines[nextLine:nextLine+numAttrs*2]
    for i in range(0, numAttrs*2, 2):
      atName = cacheAttrs[i].strip()
      atVal = cacheAttrs[i+1].strip()
      if "vector" in attrTypes[atName]:
        atVals = atVal.split()
        if len(atVals) > 1:
          atVals = atVals[1:]
          atVal = " ".join(atVals)
        else:
          atVal = ""

      attrList.append((atName, atVal))

    attributes.append(attrList)

    nextLine = nextLine + numAttrs*2 + 1

  solver.solutions = solutions
  solver.attributes = attributes
  solver.responses = responses

  return 1

#####################################################################
# Solvers:
#   base class "Solver "
#     IPSolver
#       PICO
#       AMPLIPSolver
#         PICOAMP
#         CPLEXAMP
#     grasp
#     lagrangian
#     preprocess

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
# A base class for solvers

class Solver:
  "A base class for all solvers"

  def __init__(self):
    "init function"

  ## Values set by solver subclass
  #
  solverName="noname" # Descriptive name of solver
  solutions=[]        # List of solutions (integer sensor ID lists), last
                      # solution in list is evaluated by evalsensor
  responses=[]        # (str type, float val) tuples, 1 list of tuples per soln
  attributes=[]       # (str name, float val) tuples, 1 list of tuples per soln
  command_line=""     # Command line used to run solver
  run_log=""          # Log of solver input/output 
  exit_code=0         # Solver's exit code
  aggregate_impact_files=False  # Set to True if aggregating impacts
  map_to_first_file=None    # name of file output by aggregateImpact
  map_to_all_file=None      # name of file output by aggregateImpact
  #
  ## 

  num_nodes=0
  num_lines=0
  junction_map = {}
  invalid_vals = []
  fixed_vals = []
  cost_map={}
  total_number_side_constraints=0

  def updateSolution(self):
    """ Solver specific interim solution

    Solver should override this method if it can compute interim
    solutions while still running.  It should write the solutions
    list, and optionally the attributes and/or responses lists.
    """
    print "Warning: this solver can not generate interim answers"

  def getLatestSolutions(self):
    """ Return a string with the latest solution(s)

    """ 
    soln="#Solver: "+self.solverName
    for s in range(0,len(self.solutions)):
      if len(self.solutions[s]) > 0:
        soln=soln+"\n#Sensor IDs: "
        for i in range(0,len(self.solutions[s])):
          if (i > 0) and (i % 20) == 0:
            soln = soln+"\n#  "
          soln = soln+`self.solutions[s][i]`+" "
        soln = soln+"\n#"
      if (len(self.responses) > 0) and (len(self.responses[s]) > 0):
        soln = soln + "\n#  Responses:"
        for (name, val) in self.responses[s]:
          soln = soln + "\n#    " + name + ": " + `val`
      if (len(self.attributes) > 0) and (len(self.attributes[s]) > 0):
        soln = soln + "\n#  Attributes:"
        for (name, val) in self.attributes[s]:
          soln = soln + "\n#    " + name + ": " + `val`
      soln = soln+"\n#"
    return soln

  def getInterimSolution(self):
    self.updateSolution()
    return self.getLatestSolutions()

  def fixCygwinPaths(self):
    problem.impactdir = commands.getoutput("cygpath -m " + problem.impactdir)
    problem.data_dir = commands.getoutput("cygpath -m " + problem.data_dir)
    problem.modpath = commands.getoutput("cygpath -m " + problem.modpath)

  def processOptions(self):
    """ Checks that must be done before running any solver.

    """

    problem.cleanupOutputFiles()   #get rid of last run's files

    [self.num_nodes, valid_delay, self.num_lines] = read_impact_files()

    if valid_delay==False:
      raise RuntimeError("ERROR: the delay value "+`problem.delay`+ \
            " does not appear in one or more impact files!")

    if self.num_nodes == 0:
      raise RuntimeError("ERROR: could not identify the number of nodes\n" +\
        "ERROR: possible problem with the specification of the objectives")

    #TODO: which subclasses of solvers use the following files/maps?  Put
    # the checks there.
    if problem.sensor_locations != None:
      if not os.path.isfile(problem.sensor_locations):
        raise RuntimeError("ERROR: missing file", problem.sensor_locations)

      if not os.path.isfile(problem.nodemap_file):
        raise RuntimeError("ERROR: Missing nodemap file "+problem.nodemap_file+\
          " required when specifying fixed/invalid sensor locations by index")

    if problem.costs != None and not os.path.isfile(problem.costs):
      raise RuntimeError("ERROR: missing file", problem.costs)

    if problem.costs_name != None and not os.path.isfile(problem.costs_name):
      raise RuntimeError("ERROR: missing file", problem.costs_name)

    if not self.solverName in ["grasp", "heuristic", "att_grasp", "snl_grasp", "new_grasp"]:
      if problem.imperfect_scfile != None or problem.imperfect_jcfile != None:
        raise RuntimeError("ERROR: Only the grasp solver supports "+\
              "optimization of the imperfect-sensor placement model")

    if (self.solverName != "PICO") and (self.solverName != "CPLEX with AMPL") and \
       (self.solverName != "PICO with AMPL") :
      if (problem.compute_bound==True):
        raise RuntimeError("ERROR: can only compute the bound with an IP solver!")

  def processInputFiles(self):
    """ Read and process various input files.

    """

    if problem.costs_name != None or problem.sensor_locations != None:
      self.junction_map = read_node_map(problem.nodemap_file)

    if problem.sensor_locations != None:
      [self.invalid_vals, self.fixed_vals] = \
        read_sensor_locations(problem.sensor_locations, self.junction_map)
      if problem.debug:
         print "Number of fixed sensor locations:", len(self.fixed_vals)
         print "Number of invalid sensor locations:", len(self.invalid_vals)
         print "Fixed:      ", self.fixed_vals
         print "Invalid:    ", self.invalid_vals

    if problem.costs != None:
       self.cost_map = read_costs(problem.costs)
    elif problem.costs_name != None:
       tmp = read_costs(problem.costs_name)
       self.cost_map = {}
       for val in tmp:
         if val == "__default__":
            self.cost_map[-1] = tmp[val]
         else:
            self.cost_map[self.junction_map[val]] = tmp[val]
    else:
       self.cost_map = {}

  def createConfigurationFile(self):
    """ Write out the configuration file required by the solver.

    """

    OUTPUT = open(problem.tmpname + ".config","w")

    # name of files written by aggregateImpacts if it runs
    self.map_to_first_file = problem.tmpname + "_earliest.config"
    self.map_to_all_file = problem.tmpname + "_map.config"

    # the two-pass approach is necessary to handle cases where
    # uses specify side consraints on different statistics of
    # a primary objective goal.
    print >>OUTPUT, self.num_nodes, `problem.delay`
    
    # output the number of goals (objectives or side constraints)
    print >>OUTPUT, len(problem.goals)
    
    # the total number of "real", i.e., goal-related, side constraints
    # (this leaves out just "ns" and "cost" goals.
    self.total_number_side_constraints=0
    
    # output the objective and any side constraints related to the objective goal
    for goal in problem.goals:
      if problem.objflag[goal] == True:
        print >>OUTPUT, goal,
        if goal == "ns" or goal == "cost" or goal == "awd":
           print >>OUTPUT, "none",
        else:
           print >>OUTPUT, join(problem.impactdir, problem.network+"_"+goal+".impact"),
        if problem.threshold.has_key(goal):
           print >>OUTPUT, problem.threshold[goal],
        else:
           print >>OUTPUT, 1e-7,
        if problem.aggregation_percent.has_key(goal):
           print >>OUTPUT, problem.aggregation_percent[goal],
        else:
           print >>OUTPUT, 0.0,
        if problem.aggregation_ratio.has_key(goal):
           print >>OUTPUT, problem.aggregation_ratio[goal],
        else:
           print >>OUTPUT, 0.0,
        if goal in problem.distinguish_detection:
           print >>OUTPUT, 1,
        else:
           print >>OUTPUT, 0,
        # only one measure for the objective goal can be specified
        print >>OUTPUT, len(problem.measures[goal]),
        for i in range(0,len(problem.measures[goal])):
           print >>OUTPUT, problem.measures[goal][i],
        print >>OUTPUT, "o",
        for bound in problem.bounds[goal]:
           if bound==goal:
             print >>OUTPUT, "-99999",
           else:
             print >>OUTPUT, bound,
             if goal != "ns" and goal != "cost":
               self.total_number_side_constraints=self.total_number_side_constraints+1
        print >>OUTPUT, ""
    
    # output the non-objective goal side constraints
    for goal in problem.goals:
      if problem.objflag[goal] == False:
        print >>OUTPUT, goal,
        if goal == "ns" or goal == "cost" or goal == "awd":
           print >>OUTPUT, "none",
        else:
           print >>OUTPUT, join(problem.impactdir, problem.network + "_" + goal + ".impact"),
        if problem.threshold.has_key(goal):
           print >>OUTPUT, problem.threshold[goal],
        else:
           print >>OUTPUT, 0.0,
        if problem.aggregation_percent.has_key(goal):
           print >>OUTPUT, problem.aggregation_percent[goal],
        else:
           print >>OUTPUT, 0.0,
        if problem.aggregation_ratio.has_key(goal):
           print >>OUTPUT, problem.aggregation_ratio[goal],
        else:
           print >>OUTPUT, 0.0,
        if goal in problem.distinguish_detection:
           print >>OUTPUT, 1,
        else:
           print >>OUTPUT, 0,
        print >>OUTPUT, len(problem.measures[goal]),
        for measure in problem.measures[goal]:
          print >>OUTPUT, measure,
          if goal != "ns" and goal != "cost":
            self.total_number_side_constraints=self.total_number_side_constraints+1
        print >>OUTPUT, "c",
        for bound in problem.bounds[goal]:
          print >>OUTPUT, bound,
        print >>OUTPUT, ""
    
    print >>OUTPUT, len(self.fixed_vals),
    for val in self.fixed_vals:
      print >>OUTPUT, val,
    print >>OUTPUT, ""
    
    print >>OUTPUT, len(self.invalid_vals),
    for val in self.invalid_vals:
      print >>OUTPUT, val,
    print >>OUTPUT, ""
    
    print >>OUTPUT, len(self.cost_map),
    for val in self.cost_map:
      print >>OUTPUT, "%s %d " % (val,self.cost_map[val]),
    print >>OUTPUT, ""
    
    OUTPUT.close()

    if self.total_number_side_constraints > 0:
        print "WARNING: Location aggregation does not work with side constraints"
        print "WARNING: Turning off location aggregation"
        self.aggregate_impact_files = False 

    if self.aggregate_impact_files == True:
      """ Aggregate locations into superlocations.

          Create a new config file and new impact files that combine
          locations into superlocations, to reduce problem size.
          Writes new config file: tmpname_agg.config
      """

      if (problem.memcheck == "all") or (problem.memcheck == "aggregateImpacts"):
        valgrind = problem.valgrindCommand.replace("xxxx","aggregateImpacts")
      else:
        valgrind = ""

      aggExecutable = pyutilib.services.registered_executable('aggregateImpacts')
      if aggExecutable is None:
        raise RuntimeError("Executable aggregateImpacts not found.")

      command = valgrind + problem.memmon + aggExecutable.get_path() + " --read-limit " +str(problem.maximum_impacts) +" "+ problem.tmpname + ".config"

      print command 

      if problem.memmon != "":
        #we want to see memmon output
        (out, rc) = spawn_command(command)
        print out
      else:
        rc = system_call(command)

      if rc != 0:
        raise RuntimeError("Error: "+command)

      shutil.copyfile(problem.tmpname+".config", problem.tmpname+"_noagg.config")
      shutil.copyfile(problem.tmpname+"_agg.config", problem.tmpname+".config")

  def printParameters(self):
    """ Print the parameters of the problem to be solved.  

    """
    if problem.debug == True:
      print "Network:    ", problem.network
      print "Goals:      ", problem.goals
      print "Objectives: ", problem.objectives
      print "Delay:      ", problem.delay
      print "ImpactDir:  ", problem.impactdir
      print "Threshold:  ", problem.threshold
      print "Percent:    ", problem.aggregation_percent
      print "No collapse:", problem.distinguish_detection
      print "Ratio:      ", problem.aggregation_ratio
      print "Costs:      ", problem.costs
      print "Solver:     ", problem.solver
      print "Time limit (minutes): ", problem.timelimit
      print "Notify interval (minutes): ", problem.notifyInterval
      print "ModPath:    ", problem.modpath

    print ""
    print "Number of Nodes                :  " + `self.num_nodes`
    print "Number of Contamination Impacts: " + `(self.num_lines - 2)`
    print ""

  def outputResults(self):
    """ Print out/Write out results

    """

    if len(self.solutions) == 0:
      raise RuntimeError("Error running ", self.solverName)
      #print "Current directory:", os.getcwd()
      #print "Exit Code:", self.exit_code
      #print "Command: ", self.command_line
      #print  "--- START COMMAND LOG ---"
      #print  self.run_log
      #print  "--- END COMMAND LOG ---")

    if self.aggregate_impact_files == True:
      #
      # Map the super locations to real locations
      # 
      # Replace original config file for evalsensor
      #
      # TODO: The solution should be an object.  One solution is
      # a list of integer location IDs.  The object may have several
      # solutions, with associated responses and attributes.  The
      # object would know whether or not a solution had been
      # translated from superlocation IDs to real IDs, and would do
      # the translation when needed.
      # For now we just translate at the end.  Intermediate answers
      # are being displayed as superlocation IDs, but I dont' think
      # anyone is using that feature yet.

      shutil.copyfile(problem.tmpname+"_noagg.config", problem.tmpname+".config")

      latestSoln = []
      if len(self.solutions) > 0:
        latestSoln = self.solutions[-1]
        try:
          ffirst = open(self.map_to_first_file,"r")
        except IOError:
          print "Can not open ",self.map_to_first_file
          print "Location IDs are untranslated"
        else:
          try:
            fmap = open(self.map_to_all_file,"r")
          except IOError:
            ffirst.close()
            print "Can not open ",self.map_to_all_file
            print "Location IDs are untranslated"
          else:
            realsoln = location_map(latestSoln, ffirst, fmap)
            #print latestSoln," -> ",realsoln
            ffirst.close()
            fmap.close()
            if len(realsoln) == 0:
              print "ERROR: Location IDs are superlocation IDs"
              print "ERROR: map to original locations failed"
              print "ERROR: see ",self.map_to_all_file
              print "ERROR: Try lower level of aggregation."
            else:
              self.solutions[-1] = realsoln

    elif (problem.sensor_locations != None):
      #
      # Transform the locations to original location IDs before invalid
      # locations were removed.
      #
      # If we created aggregated impact files, the mapping done in the
      # location_map() function mapped chosen locations back to original
      # locations, so this transformation is not necessary.
      #
      if len(self.solutions) > 0:
        reduced_repn_map = {}
        translatedSoln = []
        j=1
        for i in range(1,len(self.junction_map)+1):
          if i not in self.invalid_vals:
             reduced_repn_map[j] = i
             j = j + 1

        for val in self.solutions[-1]:
          translatedSoln.append(reduced_repn_map[val])

        self.solutions[-1] = translatedSoln

    OUTPUT = open(problem.output_file,"w")
    print >>OUTPUT, "#"
    print >>OUTPUT, "# Generated by sp solver interface: " + time.asctime(time.gmtime(time.time()))
    print >>OUTPUT, "#"
    print >>OUTPUT, "# " + self.command_line
    print >>OUTPUT, "# exit code " + `self.exit_code`
    print >>OUTPUT, "#"
    print >>OUTPUT, pid,
    # This next line is read by evalsensor
    if len(self.solutions) > 0:
      latestSoln = self.solutions[-1]
      latestSoln.sort()
      print >>OUTPUT, len(latestSoln)," ",
      for val in latestSoln:
        print >>OUTPUT, `val`," ",
    print >>OUTPUT, "\n#"
    # A little redundant, but this is the whole solution
    print >>OUTPUT, self.getLatestSolutions()
    print >>OUTPUT
    OUTPUT.close()
    
    if problem.printlog:
      print self.run_log
    
    OUTPUT = open(problem.tmpname+".log","w")
    print >>OUTPUT, self.run_log,
    OUTPUT.close()
  
    #
    # Print summary of output
    #
    impactfiles=""
    if problem.eval_all == True:
       problem.goals = []
       for goal in valid_objectives:
         if (goal != "ns") and (goal != "cost") and (goal != "awd") and os.path.isfile(join(problem.impactdir, problem.network+"_"+goal+".impact")):
            problem.goals = problem.goals + [ goal ]

    for goal in problem.goals:
      if (goal != "ns") and (goal != "cost") and (goal != "awd"):
         impactfiles = impactfiles + " "  + join(problem.impactdir, problem.network+"_"+goal+".impact")

    if problem.costs_name != None :
       cost_option = " --costs=" + problem.costs_name
    else:
       cost_option = ""

    evalsensorExecutable = pyutilib.services.registered_executable('evalsensor')

    if problem.compute_greedy_ranking:
        compute_greedy_ranking=" --compute-greedy-ranking"
    else:
        compute_greedy_ranking=""

    if problem.debug:
       if problem.imperfect_model==True:
          print evalsensorExecutable.get_path() + " --responseTime=" + `problem.delay` + " --gamma=" + `problem.gamma` + compute_greedy_ranking + " --format=" + problem.format_str + " --nodemap=" + problem.nodemap_file + cost_option + " --sc-probabilities=" + problem.imperfect_scfile + " --scs=" + problem.imperfect_jcfile + " " + problem.output_file + " " + impactfiles
       else:
          print evalsensorExecutable.get_path() + " --responseTime=" + `problem.delay` + " --gamma=" + `problem.gamma` + compute_greedy_ranking + " --format=" + problem.format_str + " --nodemap=" + problem.nodemap_file + cost_option + " " + problem.output_file + " " + impactfiles

    # This test says that we plan to actually compute a sensor placement, which means we
    # should be able to evaluate it.
    if problem.compute_bound == False:
       if not os.path.isfile(problem.output_file):
          raise RuntimeError("ERROR: File " + problem.output_file + " is missing")
       if evalsensorExecutable is None:
          raise RuntimeError("ERROR: Executable evalsensor is missing")

       #
       # Launch eval sensors
       #

       if (problem.memcheck == "all") or (problem.memcheck == "evalsensor"):
         valgrind = problem.valgrindCommand.replace("xxxx","evalsensor")
       else:
         valgrind = ""

       if problem.imperfect_model==True:
          (cmd_output, rc) = spawn_command(valgrind + evalsensorExecutable.get_path() + " --responseTime=" + `problem.delay` + " --gamma=" + `problem.gamma` + compute_greedy_ranking + " --format=" + problem.format_str + " --nodemap=" + problem.nodemap_file + cost_option + " --sc-probabilities=" + problem.imperfect_scfile + " --scs=" + problem.imperfect_jcfile + " " + problem.output_file + " " + impactfiles)
       else:
          (cmd_output, rc) = spawn_command(valgrind + evalsensorExecutable.get_path()+ " --responseTime=" + `problem.delay` + " --gamma=" + `problem.gamma` + compute_greedy_ranking + " --format=" + problem.format_str + " --nodemap=" + problem.nodemap_file + cost_option + " " + problem.output_file + " " + impactfiles)
    else:
       lines = self.run_log.split("\n")
       for line in lines:
         words = re.split("[\t ]*",line)
         if len(words) >= 3  and  words[2] == "value=":
            bound = words[3]
            break
       cmd_output = "Objective lower bound: " + bound
    
    if problem.summary_file == "":
       print cmd_output
    else:
       OUTPUT = open(problem.summary_file,"w")
       print >>OUTPUT, cmd_output
       OUTPUT.close()

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
# Direct subclasses of Solver class

class IPSolver(Solver):
  def __init__(self):
    Solver.__init__(self)

  def processOptions(self):
    """ Checks that must be done before running any IP solver.

    """
    Solver.processOptions(self)

    for measure in problem.allmeasures:
      if (measure == "tce"):
         raise RuntimeError("ERROR: cannot use the TCE measure with IP solvers!")
      if (measure == "var"):
         raise RuntimeError("ERROR: cannot use the VAR measure with IP solvers!")

  def runcreateIPData(self):
    createIPDataExecutable = pyutilib.services.registered_executable('createIPData')
    if createIPDataExecutable is None:
      raise RuntimeError("Executable createIPData is missing")

    if (problem.memcheck == "all") or (problem.memcheck == "setupipdata"):
      valgrind = problem.valgrindCommand.replace("xxxx","createIPData")
    else:
      valgrind = ""

    outFileName = problem.tmpname + ".dat"

    command = valgrind + problem.memmon + createIPDataExecutable.get_path() + \
                   " --gamma=" + `problem.gamma` + " " \
                 + problem.tmpname + ".config "

    print "Creating IP Data File"
    print "Command:",command
    rc = system_call(command, outFileName)

    if rc:
       raise RuntimeError("ERROR: problem launching createIPData")

    # When we run createIPData on Windows with shared libraries, we can
    # get a WARNING while it's running, which ends up in the data file
    # (at the top, before createIPData write the rest of the file)

    try:
      f = open(outFileName, "r")
    except IOError:
      raise RuntimeError("createIPData failure")

    line = f.readline()

    if "WARNING" in line:
      f.close()
      INPUT = open(outFileName,"r")
      OUTPUT = open(outFileName + "-fixed.dat","w")
      for line in INPUT.xreadlines():
        if not "WARNING" in line:
          print >>OUTPUT, line,
      OUTPUT.close()
      INPUT.close()
      os.remove(outFileName)
      os.rename(outFileName+"-fixed.dat",outFileName)
    else:
      f.close()
    
    if problem.memmon != "":
      #we want to see memmon output, but not include it in .dat file
      try:
        f = open(outFileName, "r")
      except IOError:
        raise RuntimeError("createIPData failure")

      lines = f.read()
      f.close()
      l2 = lines.split("end;")
      l2[0] = l2[0] + "end;\n"
      f = open(outFileName, "w")
      f.write(l2[0])              # ampl input file
      f.close()
      print l2[1]                 # memmon output

class lagrangian(Solver):
  solverName = "Lagrangian"

  def __init__(self):
    Solver.__init__(self)

  def processOptions(self):
    """ Checks that must be done before running this solver.

    """
    Solver.processOptions(self)

    if problem.all_aggregation_disabled == False:
      self.aggregate_impact_files = True

  def solve(self):
    """ solve

    """
    problem.print_summary(self)

    createLagDataExecutable = pyutilib.services.registered_executable('createLagData')
    if createLagDataExecutable is None:
      raise RuntimeError("Executable createLagData is missing")

    uflExecutable = pyutilib.services.registered_executable('ufl')
    if uflExecutable is None:
      raise RuntimeError("Executable ufl is missing")

    print "Setting up Lagrangian data files..."

    if (problem.memcheck == "all") or (problem.memcheck == "setupipdata"):
      valgrind = problem.valgrindCommand.replace("xxxx","createLagData")
    else:
      valgrind = ""

    command = valgrind + problem.memmon + createLagDataExecutable.get_path() + " --gamma=" + `problem.gamma` + " " + problem.tmpname + ".config "

    (out, rc) = spawn_command(command)

    if rc != 0:
      raise RuntimeError("ERROR: problem launching createLagData")

    if problem.memmon != "":
      print out

    print "Running UFL solver ..."
  
    # We now pass the number of sensors on the command line.
    # Lagrangian solver wants a sensor to place on the dummy, so we
    # increment the number of sensors by 1.  Jon points out:
    # but this is a problem if the dummy is never a witness.
    # In the no-dummy-witness case, incrementing the sensor bound
    # gave it too many sensors!  We need to modify the
    # ufl code to lock down the dummy as a sensor.
    
    # Build up the command line in pieces, since we have to construct 
    # the data file names.

    if (problem.memcheck == "all") or (problem.memcheck == "solver"):
      valgrind = problem.valgrindCommand.replace("xxxx","ufl")
    else:
      valgrind = ""

    self.command_line = valgrind + problem.memmon + " " + uflExecutable.get_path()

    if problem.solver_options != None:
      self.command_line = self.command_line + " " + problem.solver_options

    # Might eventually be able to get the objective information from the 
    # objectives list, but right now, I'm confused about whether this is a 
    # list of goals or goal_measure.  We'll use the goals list now, in the 
    # manner it's used to create the config file.

    for goal in problem.goals:
      if problem.objflag[goal] == True:
        if self.aggregate_impact_files == True:
          self.command_line = self.command_line + " " + problem.network + "_" + goal + "_agg.lag "
        else:
          self.command_line = self.command_line + " " + problem.network + "_" + goal + ".lag "

        break

    self.command_line = self.command_line + " " + \
      `eval(problem.bounds['ns'][0]) + 1` + " " + `problem.gap`

    for goal in problem.goals:
      if problem.objflag[goal] == False and goal != "ns":
        if self.aggregate_impact_files == True:
          self.command_line = self.command_line + " " + problem.network + "_" + goal + "_agg.lag " + `eval(problem.bounds[goal][0])`
        else:
          self.command_line = self.command_line + " " + problem.network + "_" + goal + ".lag " + `eval(problem.bounds[goal][0])`

    if os.path.exists('sensors.txt') == 1:
      os.remove('sensors.txt')

    print self.command_line

    (out, rc) = spawn_command(self.command_line, True)
    
    self.run_log = self.command_line + "\n" + out + "\n"
    self.exit_code = rc

    if self.exit_code == 0:
      self.updateSolution()
    else:
      self.solutions = []
      self.attributes = []

  def updateSolution(self):
    sensors = []
    attrs = []
   
    try:
      f = open("sensors.txt", "r")
    except IOError:
      return
    except OSError:
      # Solver hasn't written a solution yet
      return
    else:
      # Return the solution in the last line
      lines = f.readlines()
      f.close()
      # lagrangian output file can have multiple lines of sensor ids
      # so look for the line that starts with: "Sensor locations" adn the gather all data from the resof the lines.
      res=None
      for line in lines:
        if(line.find("Sensor location")>=0):
          res=""
        else:
          if(res <> None):
            res=res+line
      answer=res.split();
      #
      # If the solver gives us fewer sensors than we requested,
      # then we assume that that's OK.  For example, this occurs
      # when the bound is larger than then number of locations.
      #
      #if len(answer) < problem.number_of_sensors:    
        # incomplete last line
        #return

      for val in answer:
        sensors.append(int(val) + 1)

      lower_bound = float(lines[0].split(":")[1].strip())
      attrValue = float(lines[1].split(":")[1].strip())
      attrs.append(("Best integer solution",attrValue))
      attrs.append(("Lower bound",lower_bound))

    self.solutions.append(sensors)
    self.attributes.append(attrs)

class grasp(Solver):
  solverName = "grasp"

  repr = 0
  impactfile = None
  objective = None
  statistic = None
  executable = None

  def __init__(self):
    Solver.__init__(self)

  def processOptions(self):
    """ Checks that must be done before running this solver.

    """
    Solver.processOptions(self)

    if problem.all_aggregation_disabled == False:
      self.aggregate_impact_files = True

    if self.aggregate_impact_files == True:
        print "Note: witness aggregation disabled for grasp"
    self.aggregate_impact_files = False


    if problem.imperfect_scfile != None and problem.imperfect_jcfile != None:
      if not os.path.exists(problem.imperfect_scfile):
        raise RuntimeError("Could not open imperfect sc file="+problem.imperfect_scfile)
      if not os.path.exists(problem.imperfect_jcfile):
        raise RuntimeError("Could not open imperfect jc file="+problem.imperfect_jcfile)
      if self.aggregate_impact_files == True:
        raise RuntimeError("Location aggregation does not work with imperfect locations yet")
      problem.imperfect_model = True
    elif problem.imperfect_scfile != None or problem.imperfect_jcfile != None:
      raise RuntimeError("ERROR: Options --imperfect-sc and --imperfect-jc "+ \
            "must be specified simultaneously")

    for measure in problem.allmeasures:
      if (measure == "cvar"):
        raise RuntimeError("ERROR: cannot use the CVaR measure with grasp!")
        
    if problem.bounds.has_key("ns") == 0:
      raise RuntimeError("Heurisic solvers require an upper bound on the number of sensors")
    if len(problem.objectives)>1:
      raise RuntimeError("ERROR: The grasp solvers only support individual objectives")
    self.repr = problem.grasp_representation
    if self.repr != 3 and self.repr != 2 and self.repr != 1 and self.repr != 0:
       raise RuntimeError("ERROR: The grasp representation can only be 0 (sparse), 1 (dense), 2 (file-based), or 3 (cached) ")

    if len(string.split(problem.objectives[0],"_")) == 1:
      problem.objectives[0] = problem.objectives[0] + "_mean"
    self.objective=string.strip(string.split(problem.objectives[0],"_")[0])
    self.statistic=string.strip(string.split(problem.objectives[0],"_")[1])

    self.impactfile=join(problem.impactdir, problem.network+"_"+self.objective)
    if self.aggregate_impact_files == True:
      self.impactfile=self.impactfile+"_agg.impact"
    else:
      self.impactfile=self.impactfile+".impact"

    if not os.path.exists(self.impactfile):
      print "Could not open impact file="+self.impactfile

    if((self.statistic != "mean") and (self.statistic != "var") and
       (self.statistic != "tce") and (self.statistic != "worst")):
      raise RuntimeError("Heuristic solvers only supports optimization of mean, var, tce, and worst statistics")
 
    if((problem.imperfect_model==True)and(self.statistic!="mean")):
       raise RuntimeError("Heuristic solvers only support optimization of the mean statistic for the imperfect sensor model")

    if((problem.imperfect_model==True)and (self.total_number_side_constraints>=1)):
       raise RuntimeError("ERROR: The imperfect-sensor grasp solver currently ignores all side constraints other than the number of sensors")


  def solve(self):
    if (problem.timelimit == -1):
      problem.timelimit = 0.0

    problem.print_summary(self)

    valgrind = ""
    config_file = problem.tmpname + ".config"

    sensors = []

    numsensors=eval(problem.bounds["ns"][0])
    self.executable = None
    if numsensors > 0:
       if problem.imperfect_model == True:
         # for the imperfect-sensor model, default to a single local optimum
         if (problem.numsamples==-1):
            problem.numsamples=1
   
         if (problem.memcheck == "all") or (problem.memcheck == "solver"):
           valgrind = problem.valgrindCommand.replace("xxxx","imperfect")
   
         if problem.solver in ["att_grasp", "heuristic", "grasp"] :
           self.executable = pyutilib.services.registered_executable('imperfect')
           if self.executable is None:
             raise RuntimeError("Executable imperfect is missing")
         elif problem.solver in ["new_grasp","snl_grasp"]:
           self.executable = pyutilib.services.registered_executable('new_imperfect')
           if self.executable == None:
             raise RuntimeError("Executable new_imperfect is missing")
         else:
           raise RuntimeError("SP script error in grasp class "+problem.solver)
   
         print "Running iterated descent grasp for *imperfect* sensor model"
   
         self.command_line = valgrind + problem.memmon + self.executable.get_path() + " "
   
         if problem.solver_options != None:
           self.command_line = self.command_line + problem.solver_options + " "
   
         self.command_line = self.command_line + config_file + " " + problem.imperfect_scfile + " " + problem.imperfect_jcfile + " " + `problem.numsamples` + " " + `problem.seed` + " " + `self.repr` + " " + `problem.timelimit` + " " + problem.output_file
   
       else:
         # mean solves are fast; default to 20. the others are not, default to 10.
         if (problem.numsamples==-1):
           if (self.statistic=="mean"):
             problem.numsamples=20
           else:
             problem.numsamples=10
   
         if self.total_number_side_constraints > 1:
           raise RuntimeError("ERROR: The perfect-sensor grasp solver currently support specification of a single side constraint (other than the constraint on the number of sensors)")

         if problem.solver in ["new_grasp", "snl_grasp"]:
   
           # new_randomsample handles 0 or 1 sideconstraint
           if (problem.memcheck == "all") or (problem.memcheck == "solver"):
             valgrind = problem.valgrindCommand.replace("xxxx","new_randomsample")
           self.executable = pyutilib.services.registered_executable('new_randomsample')
           if self.executable == None:
             raise RuntimeError("Executable new_randomsample is missing")
           print "Running iterated descent SNL grasp for *perfect* sensor model"
   
         elif self.total_number_side_constraints == 0:
   
           if (problem.memcheck == "all") or (problem.memcheck == "solver"):
             valgrind = problem.valgrindCommand.replace("xxxx","randomsample")
           self.executable = pyutilib.services.registered_executable('randomsample')
           if self.executable is None:
             raise RuntimeError("Executable randomsample is missing")
           print "Running iterated descent AT&T grasp for *perfect* sensor model"
   
         elif self.total_number_side_constraints == 1:
   
           if (problem.memcheck == "all") or (problem.memcheck == "solver"):
             valgrind = problem.valgrindCommand.replace("xxxx","sideconstraints")
           self.executable = pyutilib.services.registered_executable('sideconstraints')
           if self.executable is None:
             raise RuntimeError("Executable sideconstraints is missing")
           print "Running iterated descent AT&T grasp for *perfect* sensor model"
   
         self.command_line = valgrind + problem.memmon + self.executable.get_path() + " "
         if problem.solver_options != None:
           self.command_line = self.command_line + problem.solver_options + " "
   
         self.command_line = self.command_line + config_file + " " + `problem.numsamples` + " " + `problem.seed` + " " + `self.repr` + " " + `problem.timelimit` + " " + problem.output_file
   
       if self.executable is None:
          raise RuntimeError("ERROR: Executable is missing")
   
       (self.run_log, self.exit_code) = spawn_command(self.command_line)

       try:
         INPUT = open(problem.output_file,"r")
       except IOError:
         raise RuntimeError("IO ERROR reading results from %s\n" % problem.output_file)
         #print "Current directory:", os.getcwd()
         #print "Exit Code:", self.exit_code,
         #print "Command: ", self.command_line
         #print "--- START COMMAND LOG ---" 
         #print self.run_log
         #print "--- END COMMAND LOG ---"
      
       for line in INPUT.xreadlines():
         tokens = line.split()
         if len(tokens) < 3:
           continue
         if tokens[0].isdigit():
            for i in range(2, len(tokens)):
               sensors.append(eval(tokens[i]))
            break;
       INPUT.close()
       os.remove(problem.output_file)

    self.solutions.append(sensors)

    print "Iterated descent completed"

class preprocess(Solver):
  solverName = "preprocess"

  def __init__(self):
    Solver.__init__(self)

  def processOptions(self):
    """ Checks that must be done before running preprocess.

    """
    Solver.processOptions(self)


  def solve(self):

    print "Preprocessing..."
    preprocessImpactsExecutable = pyutilib.services.registered_executable('preprocessImpacts')
    if preprocessImpactsExecutable is None:
      raise RuntimeError("Executable preprocessImpacts is missing")

    if (problem.memcheck == "all") or (problem.memcheck == "preprocessimpacts"):
      valgrind = problem.valgrindCommand.replace("xxxx","preprocessImpacts")
    else:
      valgrind = ""

    command = valgrind + problem.memmon + preprocessImpactsExecutable + " "\
         + problem.tmpname + ".config "

    if problem.memmon != "":
      #we want to see memmon output
      (out, rc) = spawn_command(command)
      print out
    else:
      rc = system_call(command)

    if rc:
      raise RuntimeError("ERROR: problem launching preprocessImpacts")
    #
    # Copy aux files
    #
    newtmp = join(problem.impactdir, problem.tmpname+"_tmp.nodemap")
    oldtmp = join(problem.impactdir, problem.tmpname+".nodemap")
    shutil.copyfile(oldtmp,newtmp)

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

class AMPLIPSolver(IPSolver):

  def __init__(self):
    IPSolver.__init__(self)

  def fixCygwinPaths(self):
    problem.modpath = commands.getoutput("cygpath -d " + problem.modpath)
    problem.data_dir = commands.getoutput("cygpath -d " + problem.data_dir)

  def processOptions(self):
    """ Checks that must be done before running any AMPL IP solver.

    """
    IPSolver.processOptions(self)

    if not os.path.isfile(join(problem.modpath, "GeneralSP.mod")):
      raise RuntimeError("AMPL model GeneralSP.mod is missing")

  def runAMPL(self):
    problem.print_summary(self)

    model_file=[]
    model_file.append("#")
    model_file.append("# File generated by the 'sp' solver interface")
    model_file.append("#")
    model_file.append("model %s;" % (join(problem.modpath, problem.amplmodel),))
    model_file.append("data %s.dat;" % (join(problem.data_dir, problem.tmpname),))
    if problem.ampldata != None:
      model_file.append("data %s;" % (join(problem.data_dir, problem.ampldata),))
    model_file.append("option show_stats 1;")
    if problem.compute_bound == True:
      model_file.append("option relax_integrality 1;")
    if self.solverName == "CPLEX with AMPL":
      model_file.append("option solver cplexamp;")
      if problem.timelimit!=-1:
        model_file.append("option cplex_options 'mipdisplay=3 mipinterval=1 prestats=1 timing=1 timelimit="+`problem.timelimit`+"';")
      else:
        model_file.append("option cplex_options 'mipdisplay=3 mipinterval=1 prestats=1 timing=1';")
    elif self.solverName == "PICO with AMPL":
      min_sensors_obj = False;
      for goal in problem.goals:
        if problem.objflag[goal] == True and goal == "ns":
          min_sensors_obj = True;
      pico = pyutilib.services.registered_executable('PICO')
      if pico is None:
        raise RuntimeError("Missing solver PICO")
      model_file.append("option solver \"%s\";" % pico.get_path())
      if min_sensors_obj == False:
        model_file.append("option pico_options \"lpType=clp RRTrialsPerCall=8 RRDepthThreshold=-1 feasibilityPump=false usingCuts=true\";")
      else:
        model_file.append("option pico_options \"lpType=clp RRTrialsPerCall=8 RRDepthThreshold=-1 feasibilityPump=false usingCuts=true absTolerance=.99999\";")
    else:
      raise RuntimeError("Invalid solver type in runAMPL")
    
    model_file.append("solve;")

    model_file.append("printf \"\\nObjective Information: %s\\n\", objectiveGoal & \" \" & objectiveMeasure;")
    model_file.append("printf(\"\\nSuperLocation Information:\\n\");")
    model_file.append("for {g in ActiveGoals} printf \"%s\\n\",  g & \" \" & slThreshold[g] & \" \" &        slAggregationRatio[g];")
    model_file.append("  printf(\";\\n\\n\");")

    model_file.append("display s;")

    if problem.compute_bound == True:
      # TODO - find lower bound
      if self.solverName == "CPLEX with AMPL":
         model_file.append("printf \"\\tLP value= %f\\n\", Objective;")

    OUTPUT = open(problem.tmpname + ".mod","w")
    OUTPUT.write("\n".join(model_file))
    OUTPUT.close()

    amplExecutable = pyutilib.services.registered_executable('ampl')
    if amplExecutable == None:
        raise RuntimeError("Executable ampl is missing")

    print "Running AMPL script..."

    if (problem.memcheck == "all") or (problem.memcheck == "solver"):
      valgrind = problem.valgrindCommand.replace("xxxx","ampl")
    else:
      valgrind = ""

    self.command_line = valgrind + amplExecutable.get_path() + " " + problem.tmpname + ".mod"

    (self.run_log, self.exit_code) = spawn_command(self.command_line)

    if self.exit_code != 0:
       raise RuntimeError("ERROR: problem launching solver: "+self.solverName)
       #print "Current directory:", os.getcwd()
       #print  "Exit Code:", self.exit_code
       #print  "Command: ", self.command_line
       #print  "--- START COMMAND LOG ---"
       #print  self.run_log
       #print  "--- END COMMAND LOG ---"

    loglines = self.run_log.split("\n")
    sensors=[]
    gather_sensors=False
    for line in loglines:
      tokens = line.split()
      if gather_sensors == True:
        if tokens[0] == ";":
          gather_sensors = False
          break
        else:
          for i in range(0, len(tokens), 2):
            if tokens[i+1] == "1":
              print "tokens ",tokens[i]," ",tokens[i+1]
              sensors.append(eval(tokens[i]))
      elif (len(tokens) > 2) and (tokens[0] == "s") and (tokens[2] == ":="):
        gather_sensors=True

    self.solutions.append(sensors)

    # include model file commands as part of command line
    self.command_line = self.command_line+":"
    for line in model_file:
      if line[0] != "#":
        self.command_line = self.command_line+"\n# "+line

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

class GLPK(IPSolver):
  solverName = "GLPK"
  sensors = []
  attrs = []

  def __init__(self):
    IPSolver.__init__(self)

  def processOptions(self):
    """ Checks that must be done before running this solver.

    """
    IPSolver.processOptions(self)
    if pyutilib.services.registered_executable('glpsol') is None:
      raise RuntimeError("Executable GLPK is missing")
    if not os.path.isfile(join(problem.modpath, "GeneralSP.mod")):
      raise RuntimeError("AMPL model GeneralSP.mod is missing")

  def solve(self):
    problem.print_summary(self)

    self.runcreateIPData()

    if (problem.ampldata != None):
       INPUT = open(problem.tmpname + ".dat","r")
       OUTPUT = open(problem.tmpname + "-dummy.dat","w")
       for line in INPUT.xreadlines():
         if line.strip() != "end;":
            print >>OUTPUT, line,
       INPUT.close()
       OUTPUT.close()
       INPUT = open(problem.ampldata,"r")
       OUTPUT = open(problem.tmpname + "-dummy.dat","a")
       for line in INPUT.xreadlines():
         print >>OUTPUT, line,
       print >>OUTPUT, ""
       print >>OUTPUT, "end;"
       INPUT.close()
       OUTPUT.close()
       os.remove(problem.tmpname+".dat")
       os.rename(problem.tmpname+"-dummy.dat",problem.tmpname+".dat")

    print "Running GLPK ..."
    if problem.solver_options == None:
      problem.solver_options = ""

    if problem.compute_bound == True:
      problem.solver_options = problem.solver_options + " --onlyRootLP=true "
    else:
      min_sensors_obj = False
      for goal in problem.goals:
        if problem.objflag[goal] == True and goal == "ns":
          min_sensors_obj = True
      if min_sensors_obj == True:
        problem.solver_options = problem.solver_options + " --RRTrialsPerCall=8 --RRDepthThreshold=-1 --feasibilityPump=false --usingCuts=true --absTolerance=.99999 "
      else:
        problem.solver_options = problem.solver_options + " --RRTrialsPerCall=8 --RRDepthThreshold=-1 --feasibilityPump=false --usingCuts=true "
 
    if problem.timelimit!=-1:
       glpk_timelimit = "--maxWallMinutes=" + `problem.timelimit/60.0`
    else:
       glpk_timelimit = ""
    shutil.copyfile(join(problem.modpath, "GeneralSP.mod"), \
                    problem.tmpname + ".mod")
    OUTPUT = open(problem.tmpname + ".mod","a")
    print >>OUTPUT, "printf \"\\nObjective Information: %s\\n\", objectiveGoal & \" \" & objectiveMeasure;"
    print >>OUTPUT, "printf(\"\\nSuperLocation Information:\\n\");"
    print >>OUTPUT, "for {g in ActiveGoals} printf \"%s\\n\",  g & \" \" & slThreshold[g] & \" \" &        slAggregationRatio[g];"
    print >>OUTPUT, "printf(\";\\n\\n\");"
    print >>OUTPUT, "end;"
    OUTPUT.close()

    if (problem.memcheck == "all") or (problem.memcheck == "solver"):
      valgrind = problem.valgrindCommand.replace("xxxx","glpsol")
    else:
      valgrind = ""

    glpsol = pyutilib.services.registered_executable('glpsol')
    self.command_line = valgrind + problem.memmon + glpsol.get_path() + " --debug=1 --lpType=clp " + glpk_timelimit + " " + problem.solver_options + " " + problem.tmpname + ".mod " + join(problem.data_dir, problem.tmpname+".dat")

    print self.command_line

    (self.run_log, self.exit_code) = spawn_command(self.command_line)

    print "... GLPK done"

    if self.exit_code != 0:
       raise RuntimeError("ERROR: problem launching the GLPK solver")
       #print "Current directory:", os.getcwd()
       #print "Exit Code:", self.exit_code
       #print "Command: ", self.command_line
       #print "--- START COMMAND LOG ---"
       #print self.run_log
       #print "--- END COMMAND LOG ---"
    if problem.compute_bound == False:
       #
       # Process output of GLPK to compute a valid lower bound
       #
       loglines = self.run_log.split("\n")
       state=4
       nobjectives=0
       objective='unknown'
       measure='unknown'
       min_ratio=1.0
       fvalue=0.0
       for line in loglines:
         line = string.strip(line)
         #print `state` + " " + line
         tokens = line.split(" ")
         #print tokens
         if (state == 2):
            if (tokens[0] == "Final") and (tokens[3] == "Value"):
               state=3
               fvalue = eval(tokens[5])
         if (state == 1):
            if (tokens[0] == ";"):
               state=2
            else:
               nobjectives = nobjectives + 1
               if eval(tokens[2]) < min_ratio:
                  min_ratio = eval(tokens[2])
         if ((state == 0) and (tokens[0] == "SuperLocation") and (tokens[1] == "Information:")):
            state=1
         if ((state == 4) and (tokens[0] == "Objective") and (tokens[1] == "Information:")):
            objective=tokens[2]
            measure=tokens[3]
            state = 0

       if (min_ratio == 1.0) or ((nobjectives == 1) and (measure == "mean")):
          self.run_log = self.run_log + \
            "\nValid Lower Bound for " + objective + "_" + measure + " : " + " " + `min_ratio*fvalue` + "\n"
          self.attrs.append(("lower bound", min_ratio * fvalue))
       #
       # Process the GLPK solution file to get the sensor placements
       #
       INPUT = open(problem.tmpname + ".sol.txt","r")
       for line in INPUT.xreadlines():
         tokens = line.split(" ")
         #
         # Process a line to see if its a sensor variable
         #
         item = tokens[0].split("[")
         if (item[0] == "s"):
           val = eval(item[1].split("]")[0])
           self.sensors = self.sensors + [ val ]
       INPUT.close()
    else:
       # TODO - find the lower bound
       print "Computing a lower bound"

    self.solutions.append(self.sensors)
    self.attributes.append(self.attrs)
 
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

class PICO(IPSolver):
  solverName = "PICO"
  sensors = []
  attrs = []

  def __init__(self):
    IPSolver.__init__(self)

  def processOptions(self):
    """ Checks that must be done before running this solver.

    """
    IPSolver.processOptions(self)
    if pyutilib.services.registered_executable('PICO') is None:
      raise RuntimeError("Executable PICO is missing")
    if not os.path.isfile(join(problem.modpath, "GeneralSP.mod")):
      raise RuntimeError("AMPL model GeneralSP.mod is missing")

  def solve(self):
    problem.print_summary(self)

    self.runcreateIPData()

    if (problem.ampldata != None):
       INPUT = open(problem.tmpname + ".dat","r")
       OUTPUT = open(problem.tmpname + "-dummy.dat","w")
       for line in INPUT.xreadlines():
         if line.strip() != "end;":
            print >>OUTPUT, line,
       INPUT.close()
       OUTPUT.close()
       INPUT = open(problem.ampldata,"r")
       OUTPUT = open(problem.tmpname + "-dummy.dat","a")
       for line in INPUT.xreadlines():
           print >>OUTPUT, line,
       print >>OUTPUT, ""
       print >>OUTPUT, "end;"
       INPUT.close()
       OUTPUT.close()
       os.remove(problem.tmpname+".dat")
       os.rename(problem.tmpname+"-dummy.dat",problem.tmpname+".dat")

    if problem.solver_options == None:
      problem.solver_options = ""

    if problem.compute_bound == True:
      problem.solver_options = problem.solver_options + " --onlyRootLP=true "
    else:
      min_sensors_obj = False
      for goal in problem.goals:
        if problem.objflag[goal] == True and goal == "ns":
          min_sensors_obj = True
      if min_sensors_obj == True:
        problem.solver_options = problem.solver_options + " --RRTrialsPerCall=8 --RRDepthThreshold=-1 --feasibilityPump=false --usingCuts=true --absTolerance=.99999 "
      else:
        problem.solver_options = problem.solver_options + " --RRTrialsPerCall=8 --RRDepthThreshold=-1 --feasibilityPump=false --usingCuts=true "
 
    if problem.timelimit!=-1:
       pico_timelimit = "--maxWallMinutes=" + `problem.timelimit/60.0`
    else:
       pico_timelimit = ""
    shutil.copyfile(join(problem.modpath, "GeneralSP.mod"), \
                    problem.tmpname + ".mod")
    OUTPUT = open(problem.tmpname + ".mod","a")
    print >>OUTPUT, "printf \"\\nObjective Information: %s\\n\", objectiveGoal & \" \" & objectiveMeasure;"
    print >>OUTPUT, "printf(\"\\nSuperLocation Information:\\n\");"
    print >>OUTPUT, "for {g in ActiveGoals} printf \"%s\\n\",  g & \" \" & slThreshold[g] & \" \" &        slAggregationRatio[g];"
    print >>OUTPUT, "printf(\";\\n\\n\");"
    print >>OUTPUT, "end;"
    OUTPUT.close()

    if (problem.memcheck == "all") or (problem.memcheck == "solver"):
      valgrind = problem.valgrindCommand.replace("xxxx","PICO")
    else:
      valgrind = ""

    glpsol = pyutilib.services.registered_executable('glpsol')
    pico = pyutilib.services.registered_executable('PICO')

    print "Running glpsol to generate MPS file..."
    # CAP: the --check is critical, since it tells glpk to not solve the problem (only check it).  We can live with the checking.
    # Without this, glpk solves the IP.
    self.command_line = glpsol.get_path() + " --check -m "+problem.tmpname + ".mod -d  " + join(problem.data_dir, problem.tmpname+".dat") + " --wfreemps "+problem.tmpname+".mps"
    print '%s' % self.command_line
    (self.run_log, self.exit_code) = spawn_command(self.command_line)
    if self.exit_code != 0:
       raise RuntimeError("ERROR: problem launching glpsol to generate an MPS file")
       #print "Current directory:", os.getcwd()
       #print "Exit Code:", self.exit_code
       #print "Command: ", self.command_line
       #print "--- START COMMAND LOG ---"
       #print self.run_log
       #print "--- END COMMAND LOG ---"
    print "... glpsol done"

    print "Running PICO..."
    self.command_line = valgrind + problem.memmon + pico.get_path() + " --debug=1 --lpType=clp " + pico_timelimit + " " + problem.solver_options + " " + problem.tmpname + ".mps "
    (self.run_log, self.exit_code) = spawn_command(self.command_line)
    print "... PICO done"
    if self.exit_code != 0:
       raise RuntimeError("ERROR: problem launching the PICO solver")
       #print "Current directory:", os.getcwd()
       #print "Exit Code:", self.exit_code
       #print "Command: ", self.command_line
       #print "--- START COMMAND LOG ---"
       #print self.run_log
       #print "--- END COMMAND LOG ---"

    if problem.compute_bound == False:
       #
       # Process output of PICO to compute a valid lower bound
       #
       loglines = self.run_log.split("\n")
       state=4
       nobjectives=0
       objective='unknown'
       measure='unknown'
       min_ratio=1.0
       fvalue=0.0
       for line in loglines:
         line = string.strip(line)
         #print `state` + " " + line
         tokens = line.split(" ")
         #print tokens
         if (state == 2):
            if (tokens[0] == "Final") and (tokens[3] == "Value"):
               state=3
               fvalue = eval(tokens[5])
         if (state == 1):
            if (tokens[0] == ";"):
               state=2
            else:
               nobjectives = nobjectives + 1
               if eval(tokens[2]) < min_ratio:
                  min_ratio = eval(tokens[2])
         if ((state == 0) and (tokens[0] == "SuperLocation") and (tokens[1] == "Information:")):
            state=1
         if ((state == 4) and (tokens[0] == "Objective") and (tokens[1] == "Information:")):
            objective=tokens[2]
            measure=tokens[3]
            state = 0

       if (min_ratio == 1.0) or ((nobjectives == 1) and (measure == "mean")):
          self.run_log = self.run_log + \
            "\nValid Lower Bound for " + objective + "_" + measure + " : " + " " + `min_ratio*fvalue` + "\n"
          self.attrs.append(("lower bound", min_ratio * fvalue))
       #
       # Process the PICO solution file to get the sensor placements
       #
       INPUT = open(problem.tmpname + ".sol.txt","r")
       for line in INPUT.xreadlines():
         tokens = line.split(" ")
         #
         # Process a line to see if its a sensor variable
         #
         item = tokens[0].split("[")
         if (item[0] == "s"):
           val = eval(item[1].split("]")[0])
           self.sensors = self.sensors + [ val ]
       INPUT.close()
    else:
       # TODO - find the lower bound
       print "Computing a lower bound"

    self.solutions.append(self.sensors)
    self.attributes.append(self.attrs)
 
class PICOAMP(AMPLIPSolver):
  solverName = "PICO with AMPL"

  def __init__(self):
    AMPLIPSolver.__init__(self)

  def processOptions(self):
    """ Checks that must be done before running this solver.

    """
    AMPLIPSolver.processOptions(self)

    if pyutilib.services.registered_executable('PICO') is None:
      raise RuntimeError("Executable PICO is missing")

  def solve(self):
    self.runcreateIPData()
    self.runAMPL()

class CPLEXAMP(AMPLIPSolver):
  solverName = "CPLEX with AMPL"

  def __init__(self):
    AMPLIPSolver.__init__(self)

  def processOptions(self):
    """ Checks that must be done before running this solver.

    """
    AMPLIPSolver.processOptions(self)

    if pyutilib.services.registered_executable('cplexamp') is None:
       raise RuntimeError("Executable cplexamp is missing")

  def solve(self):
    self.runcreateIPData()
    self.runAMPL()

class simpleSolver(Solver):
  """ Example of a solver sub class.  

      Methods that must be defined and instance variables that must be set
      are commented with "REQUIRED".

      To determine your parent class, read the docstring for
      each solver class to determine where your's fits in.  This
      simpleSolver is a subclass of the most general base class.
  """

  solverName = "simple pointless example"  # REQUIRED: define the solverName

  def __init__(self):
    """ REQUIRED: init function, executed when solver object is instantiated

        Call the parent class's init function, followed by any initialization
        required by simpleSolver.  Right now we never use the init functions,
        but if someday a parent class defines one, the subclasses need to
        call it.
    """    
    Solver.__init__(self)

  def processOptions(self):
    """ REQUIRED: Checks that must be done before running this solver.

        Call the parent class's processOptions method, then do our
        own processing of command line options.  The command line options
        are instance variables of the "problem" object. 
    """
    Solver.processOptions(self)

    if problem.costs_name != None :
      raise RuntimeError("ERROR - this solver can not process sensor placement costs")

  def solve(self):
    """ REQUIRED: calculate the solution

        This method calculates the solution to the problem.  It is
        may write the following variables:
          self.solutions=[]       REQUIRED
          self.responses=[]
          self.attributes=[]
          self.command_line       REQUIRED
          self.run_log            REQUIRED
          self.exit_code

        The 'solutions', 'responses' and 'attributes' lists mirror the 
        design of the SACache class.  The thought is that the solver
        may be compiled with SACache, and may be writing out intermediate
        solutions while it is running.
       
        An element of the 'solutions' list contains a list of sensor IDs.
        It may be an interim or a final solution generated by the solver.
        The sensor IDs are ints.

        Element i of the 'responses' list is a list of (name, value) tuples
        associated with solution i of the 'solutions' list.  For example,
        these may be names and values of performance metrics.  The name
        is a string and the value is a float.

        Element i of the 'attributes' list is a list of (name, value)
        tuples associated with solution i of the 'solutions' list.  These
        are arbitrary named attributes relevant to the particular solver.
        The name and value are strings.

        If the solver can generate interim solutions, then spawn_command
        should be called with True as it's second argument, and the solver
        should define the updateSolution method.  See the lagrangian
        class as an example.
    """

    sensors = []

    self.command_line = "null"
    self.run_log = self.command_line+"\nrunning example\n"

    if problem.number_of_sensors > 0:
      sensors = range(1, problem.number_of_sensors+1)
      self.exit_code=0
    else:
      print "ERROR: invalid problem"
      self.exit_code=1
 
    self.solutions.append(sensors)

  def updateSolution(self):
    """ OPTIONAL: obtain solutions computed so far from the running solver

    If the second argument to spawn_command is True, the spawn_command
    function will call updateSolution at intervals of notifyInterval
    minutes and print out intermediate solutions.

    If updateSolution is defined for a solver it should write to the
    solutions list.  It can optionally write to the responses and
    attributes lists.  See the lagrangian class for an example.
    """

####################################################################
# Signal handlers, atexit function

def finalCleanup():
  """The clean up that should occur when the script terminates.

  All cleanup regardless of whether the script terminated normally or
  was interrupted before normal termination.
  TODO: what needs to be done here
  """

# If SIGUSR1 is sent to the sp script, this signal handler runs,
# and then when control returns back to where it was, if it was
# in certain system calls (proc.wait(), proc.stderr.read(), etc)
# the script dies with "IOError: [Errno 4] Interrupted system call".
# So we'd have to block signals around all system calls which is
# too much trouble.
#
#def displayContinue(signum, frame):
#  """ The signal handler for a SIGUSR1. Unix only.
#
#  SIGUSR1 indicates that the script should display the results
#  so far, and then continue processing.
#  (printed lines don't flush until exit.  Maybe use StreamHandler? TODO)
#  """
#
#  soln = solver.getInterimSolution()
#  print soln
#  signal.signal(signal.SIGUSR1, displayContinue) 
#  print "Continuing calculation..."

def displayFinish(signum, frame):
  """ The signal handler for a SIGINT.  Unix only.

  SIGINT indicates that the script should display the results
  so far, and then terminate.
  """
  soln = solver.getInterimSolution()
  print soln
  print "Exiting now ..."
  sys.exit(0)

#####################################################################
# Begin
#####################################################################

# Globals

global problem, options, solver

valid_objectives = [ "cost", "ec", "dec", "td", "dtd", "mc", "dmc", "nfd", "ns", "pe", "dpe", "pk", "dpk", "pd", "dpd", "vc", "dvc",  "pd0", "pd1", "pd2", "pd3", "pd4", "pd5", "pd6", "pd7"]
valid_statistics = [ "mean", "median", "var", "tce", "cvar", "total", "worst" ]
pid = os.getpid()

(options, args) = parser.parse_args()    # parse command line options
if len(sys.argv) == 1:
   parser.print_help()
   raise RuntimeError("Invalid command line options")

# Setup the path

_new_paths = []
if len(options.path) > 0:
    tmp = os.pathsep.join(options.path)
    if tmp.startswith('--path='):
      tmp = tmp[7:]
    _new_paths.append(tmp)
if options.glpkpath is not None:
    _new_paths.append(options.glpkpath)
if options.amplcplexpath is not None:
    _new_paths.append(options.amplcplexpath)
if options.picopath is not None:
    _new_paths.append(options.picopath)
_new_paths.append(os.environ['PATH'])
os.environ['PATH'] = os.pathsep.join(_new_paths)

# Register executables

pyutilib.services.register_executable('PICO')
pyutilib.services.register_executable('glpsol')
pyutilib.services.register_executable('ampl')
pyutilib.services.register_executable('evalsensor')
pyutilib.services.register_executable('cplexamp')
pyutilib.services.register_executable('cbc')
pyutilib.services.register_executable('memmon')
pyutilib.services.register_executable('valgrind')
pyutilib.services.register_executable('grasp')
pyutilib.services.register_executable('heuristic')
pyutilib.services.register_executable('att_grasp')
pyutilib.services.register_executable('snl_grasp')
pyutilib.services.register_executable('new_grasp')
pyutilib.services.register_executable('aggregateImpacts')
pyutilib.services.register_executable('createIPData')
pyutilib.services.register_executable('createLagData')
pyutilib.services.register_executable('ufl')
pyutilib.services.register_executable('imperfect')
pyutilib.services.register_executable('new_imperfect')
pyutilib.services.register_executable('randomsample')
pyutilib.services.register_executable('new_randomsample')
pyutilib.services.register_executable('sideconstraints')
pyutilib.services.register_executable('preprocessImpacts')

if options.version:
    evalsensorExecutable = pyutilib.services.registered_executable('evalsensor')
    if evalsensorExecutable:
        (cmd_output, rc) = spawn_command(evalsensorExecutable.get_path() + " --version")
        tokens = cmd_output.split(' ')
        tokens[0] = "sp"
        print " ".join(tokens)
    else:
        print "sp (TEVA-SPOT Toolkit) unknown"
    sys.exit(1)

if options.solver == "pico":
  solver=PICO()
elif options.solver == "glpk":
  solver=GLPK()
elif options.solver == "picoamp":
  solver=PICOAMP()
elif options.solver == "cplexamp":
  solver=CPLEXAMP()
elif options.solver == "lagrangian":
  solver=lagrangian()
elif options.solver in ["grasp", "heuristic", "new_grasp", "snl_grasp", "att_grasp"]:
  solver=grasp()
elif options.solver == "preprocess":
  solver=preprocess()
elif options.solver == "example":
  solver=simpleSolver()
else:
  raise RuntimeError("ERROR: you must specify a solver with --solver")

problem = SPProblem()              # create the problem object
problem.processOptions()           # process the command line options
#print "HERE 0",problem.all_aggregation_disabled,solver.aggregate_impact_files

solver.processOptions()            # solver processing of command line options
#print "HERE 1",problem.all_aggregation_disabled,solver.aggregate_impact_files
if options.solver != "preprocess":
  solver.printParameters()           # print out solver parameters
  #print "HERE 2",problem.all_aggregation_disabled,solver.aggregate_impact_files
  solver.processInputFiles()         # preprocessing of input files
  #print "HERE 3",problem.all_aggregation_disabled,solver.aggregate_impact_files
  solver.createConfigurationFile()   # write configuration file

atexit.register(finalCleanup)
signal.signal(signal.SIGINT, displayFinish)    # Unix only
#signal.signal(signal.SIGUSR1, displayContinue) # Unix only

solver.solve()             # run solver

solver.outputResults()     # write output file, run evalsensor, etc.

print "Done with sp"


