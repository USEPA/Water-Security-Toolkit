#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

from pyutilib.workflow import Component
from pyutilib.component.core import *


class ISPOTSolverRegistration(Interface):
    """An interface for accessing"""

    def create(self, name=None):
        """Create a solver, optionally specifying the name"""

    def type(self):
        """The type of solver supported by this service"""


class SolverRegistration(Plugin):

    implements(ISPOTSolverRegistration)

    def __init__(self, type, cls, description=""):
        self.name = type
        self._type = type
        self._cls = cls
        self.description = description

    def type(self):
        return self._type

    def create(self, **kwds):
        return self._cls(**kwds)


def SolverFactory(solver_name=None, **kwds):
    ep = ExtensionPoint(ISPOTSolverRegistration)
    if solver_name is None:
        return map(lambda x:(x.name,x.description), ep())
    service = ep.service(solver_name)
    if service is None:
        # TODO - replace this print statement with a logging statement
        print "Unknown solver '" + solver_name + "' - no plugin registered for this solver type"
        return None
    else:
        return ep.service(solver_name).create(**kwds)


class Solver(Component):

    def __init__(self, name):
        Component.__init__(self)
        self.name = name

    def _create_parser(self):
        Component._create_parser(self)
        self.parser.add_option("--debug",
            help="Debugging information",
            action="store_true",
            dest="debug",
            default=False)

    def _create_parser_groups(self):
        Component._create_parser_groups(self)
        self._parser_group["General Solver Options"] = self.parser.add_argument_group("General Solver Options")

        self._parser_group["General Solver Options"].add_option("--solver-options",
            help="This option contains solver-specific options for controlling "\
                    "the sensor placement solver.  The options are added to the solver command line.",
            action="store",
            dest="solver_options",
            type="string",
            default="")

        self._parser_group["General Solver Options"].add_option("--timelimit",
            help="Terminate the solver after a specified number of minutes (in wall-clock time).",
            action="store",
            dest="timelimit",
            type="float",
            default=None)

        self._parser_group["General Solver Options"].add_option("--print-log",
            help="Print the solver output",
            action="store_true",
            dest="print_log",
            default=False)

    def _process_options(self):
        Component._process_options(self)

    def solve(self, problem, results=None):
        raise NotImplementedError, "Solver.solve method is not implemented!"

    def print_log(self, filename):
        if not self.options.print_log:
            return
        INPUT=open(filename,'r')
        for line in INPUT:
            print line,
        INPUT.close()

    def printParameters(self):
        """
        Print the parameters of this solver.
        """
        if self.options.debug == True:
            print "Solver:               ", self.name
            print "Solver Options:       ", "'"+self.options.solver_options+"'"
            print "Time limit (minutes): ", self.options.timelimit

    def create_config_file(self, problem):
        """
        Write out the configuration file required by the solver.
        """

        OUTPUT = open(problem.global_options.tmpname + ".config","w")

        # name of files written by aggregateImpacts if it runs
        self.map_to_first_file = problem.global_options.tmpname + "_earliest.config"
        self.map_to_all_file = problem.global_options.tmpname + "_map.config"

        # the two-pass approach is necessary to handle cases where
        # uses specify side consraints on different statistics of
        # a primary objective goal.
        print >>OUTPUT, problem.num_nodes, `problem.options.delay`
    
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
                    print >>OUTPUT, problem.impactdir + os.sep + problem.options.network + "_" + goal + ".impact",
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
                    print >>OUTPUT, problem.impactdir + os.sep + problem.options.network + "_" + goal + ".impact",
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

        print >>OUTPUT, len(problem.fixed_vals),
        for val in problem.fixed_vals:
            print >>OUTPUT, val,
        print >>OUTPUT, ""

        print >>OUTPUT, len(problem.invalid_vals),
        for val in problem.invalid_vals:
            print >>OUTPUT, val,
        print >>OUTPUT, ""

        print >>OUTPUT, len(problem.cost_map),
        for val in problem.cost_map:
            print >>OUTPUT, "%s %d " % (val,problem.cost_map[val]),
        print >>OUTPUT, ""

        OUTPUT.close()


