#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

import sys
import os
from os.path import abspath, dirname, exists
import pyutilib.services
import pyutilib.component.core
import core
import atexit
import signal

valid_objectives = [ "cost", "ec", "dec", "td", "dtd", "mc", "dmc", "nfd", "ns", "pe", "dpe", "pk", "dpk", "pd", "dpd", "vc", "dvc" ]
#valid_statistics = [ "mean", "median", "var", "tce", "cvar", "total", "worst" ]

pyutilib.services.register_executable(name="evalsensor")


def create_parser():
    parser = core.OptionParser(add_help_option=False)
    parser.ignore_invalid()
    
    parser.add_option("--help",
        help="Print option information",
        action="store_true",
        dest="print_help",
        default=False)

    parser.add_option("--solver",
        help="Specify the type of solver that is used to find sensor placement(s).  This option can be specified multiple times to define a order of a sequence of optimization solves.",
        action="append",
        dest="solver",
        type="string",
        default=[])

    parser.add_option("--help-solver",
        help="Print options for a particular solver",
        action="store",
        dest="help_solver",
        type="string",
        default=None)

    parser.add_option("--help-solver-types",
        help="Print information about valid solver types",
        action="store_true",
        dest="help_solver_types",
        default=False)

    parser.add_option("--problem",
        help="This option specifies the type of problem that is "\
            "optimized to find sensor placement(s).",
        action="store",
        dest="problem",
        type="string",
        default="pmedian")

    parser.add_option("--help-problem",
        help="Print options for a particular problem",
        action="store",
        dest="help_problem",
        type="string",
        default=None)

    parser.add_option("--help-problem-types",
        help="Print information about valid problem types",
        action="store_true",
        dest="help_problem_types",
        default=False)

    parser.add_option("--help-postprocess",
        help="Print options for post-processing solutions",
        action="store_true",
        dest="help_postprocess",
        default=False)

    parser.add_option("--memmon",
        help="Summarize the maximum memory used by any of the executables",
        action="store_true",
        dest="memmon",
        default=False)

    # X
    parser.add_option("--memcheck",
        help="This option indicates that valgrind should run on one "\
            "or more executables.  If 'all' is specified, then valgrind is "\
            "run on all executables.  If other executables are specified, "\
            "then valgrind is run on just those executables."\
            "Output will be written to memcheck.{name}.{pid} .       ",
        action="append",
        dest="memcheck",
        default=[])

    parser.add_option("--stop-after-setup",
        help="Terminate after the problem setup phase.",
        action="store_true",
        dest="stop_after_setup",
        default=False)

    parser.add_option("--skip-setup",
        help="Skip the problem setup phase, and perform optimization using files generated from a previous problem setup phase.",
        action="store_true",
        dest="skip_setup",
        default=False)

    parser.add_option("--tmp-file",
        help="Name of temporary file prefix used in this computation. "\
            "The default name is `<network-name>'.",
        action="store",
        dest="tmpname",
        type="string",
        default="")

    parser.add_option("--path",
        help="Add this path to the set of paths searched "\
            "for executables and IP models.",
        action="append",
        dest="path",
        type="string",
        default=[])

    # X
    parser.add_option("--enable-dev-packages",
        help="Enable development packages.",
        action="store_true",
        dest="enable_dev_packages",
        default=False)

#parser.add_option("--ampl",
      #help=ampl_help, default="ampl",
      #action="store", dest="ampl", type="string")
#
#parser.add_option("--ampldata",
      #help=ampldata_help, 
      #action="store", dest="ampldata", type="string")
#
#parser.add_option("--amplmodel",
      #help=amplmodel_help, default="GeneralSP.mod",
      #action="store", dest="amplmodel", type="string")

    # X
    parser.add_option("--debug",  
        help="List status messages while processing.",
        action="store_true",
        dest="debug",
        default=False)

    parser.add_option("--version",  
        help="Print version information for the compiled executables used by this command.",
        action="store_true",
        dest="version",
        default=False)

    return parser

    
def print_help(*args):
    """
    Collect pre-defined option groups from each argument, and
    merge them into the parser (which is the first argument).
    """
    parser = args[0]
    for arg in args[1:]:
        for key in arg._parser_group:
            #
            # NOTE: we are changing the properties of the group
            # instances here.  This is OK _only_ because we are
            # printing the help info and then terminating.
            #
            arg._parser_group[key].parser = parser
            parser.add_option_group(arg._parser_group[key])
    parser.print_help()

####################################################################
# Signal handlers, atexit function

def finalCleanup():
  """The clean up that should occur when the script terminates.

  All cleanup regardless of whether the script terminated normally or
  was interrupted before normal termination.
  TODO: what needs to be done here
  """

def displayFinish(signum, frame):
  """ The signal handler for a SIGINT.  Unix only.

  SIGINT indicates that the script should display the results
  so far, and then terminate.
  """
  soln = solver.getInterimSolution()
  print soln
  print "Exiting now ..."
  sys.exit(0)

#
# This function executes the main steps for 
# applying optimizers in SP.
#
def run(name='sp', args=sys.argv):
    ##pyutilib.component.core.PluginGlobals.pprint()
    #
    # Signals...
    #
    atexit.register(finalCleanup)
    signal.signal(signal.SIGINT, displayFinish)    # Unix only
    #
    # Parse command-line options
    #
    parser = create_parser()
    if len(args) == 1:
        print_help(parser)
        sys.exit(1)
    (options, _args) = parser.parse_args(args)
    if options.print_help:
        print_help(parser)
        sys.exit(1)
    if options.help_solver_types:
        print "Valid Solver Types:"
        for (name,desc) in core.SolverFactory():
            print " . "+name
            print "     "+desc
        sys.exit(1)
    if options.help_problem_types:
        print "Valid Problem Types:"
        for (name,desc) in core.ProblemFactory():
            print " . "+name
            print "     "+desc
        sys.exit(1)
    #
    # Augment the search path
    #
    if len(options.path) > 0:
        os.environ["PATH"] = os.pathsep.join([os.environ["PATH"], options.path])
    #
    # Query 'evalsensor' to get the "version" for sp.
    #
    if options.version:
        executable = pyutilib.services.registered_executable("evalsensor")
        if executable is None:
            print name+" (TEVA-SPOT Toolkit) unknown"
        else:
            (cmd_output, rc) = spawn_command(binpath+os.sep+"evalsensor --version")
            tokens = cmd_output.split(' ')
            tokens[0] = name
            print " ".join(tokens)
        sys.exit(1)
    #
    # Print solver help information
    #
    if not options.help_solver is None:
        solver_name = options.help_solver
        solver = core.SolverFactory(solver_name)
        if solver is None:
            pyutilib.component.core.PluginGlobals.env().log.error("Could not locate the '%s' solver" % solver_name)
            sys.exit(1)
        print_help(parser, solver)
        sys.exit(1)
    #
    # Print post-processing help information
    #
    if options.help_postprocess:
        postsolve = core.Postprocesser(None)
        print_help(parser, postsolve)
        sys.exit(1)
    #
    # Create the problem
    #
    if not options.help_problem is None:
        options.problem = options.help_problem
    problem = core.ProblemFactory(options.problem)
    if problem is None:
        pyutilib.component.core.PluginGlobals.env().log.error("Could not locate the '%s' problem" % options.problem)
        sys.exit(1)
    if not options.help_problem is None:
        print_help(parser, problem)
        sys.exit(1)
    #
    # Initialize the problem
    #
    problem.global_options=options
    problem.set_options(args)
    problem.printParameters()
    print ""
    #
    # Perform problem setup actions before the solvers are called
    #
    if not options.skip_setup:
        problem.setup()
    else:
        print "Skipping problem setup..."
    if options.stop_after_setup:
        print "Stopping after problem setup."
        print "Done with sp"
        return
    #
    # Create a series of solvers, which are executed sequentially
    #
    results=None
    if options.solver is []:
        options.solver = ['snl_grasp']
    for solver_name in options.solver:
        solver = core.SolverFactory(solver_name)
        if solver is None:
            pyutilib.component.core.PluginGlobals.env().log.error("Could not locate the '%s' solver" % solver_name)
            sys.exit(1)
        #
        # Initialize the solver
        #
        solver.global_options=options
        solver.set_options(args)
        solver.printParameters()
        print ""
        #
        # Run the solver
        #
        results = solver.solve(problem,results=results)
    #
    # Process the results with the post-solve component,
    # which dumps out the results in the format
    # requested by the user.
    #
    postsolve = core.Postprocesser(problem)
    postsolve.set_options(args)
    if options.debug:
        print ""
        results.write(only_variables=True)
        print ""
    postsolve.process(problem, results)
      
    print "Done with sp"
