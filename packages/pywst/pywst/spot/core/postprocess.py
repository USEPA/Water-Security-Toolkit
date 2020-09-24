#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

__all__ = ['PostprocesserRegistration', 'PostprocesserFactory', 'Postprocesser']

import os
import os.path
import sys
import time
from pyutilib.workflow import Component
from pyutilib.component.core import *
import pyutilib.services
import pyutilib.misc
import pyutilib.subprocess


class ISPOTPostprocesserRegistration(Interface):
    """An interface for accessing"""

    def create(self, name=None):
        """Create a postprocesser, optionally specifying the name"""

    def type(self):
        """The type of postprocesser supported by this service"""


class PostprocesserRegistration(Plugin):

    implements(ISPOTPostprocesserRegistration)

    def __init__(self, type, cls, description=""):
        self.name = type
        self._type = type
        self._cls = cls
        self.description = description

    def type(self):
        return self._type

    def create(self, **kwds):
        return self._cls(**kwds)


def PostprocesserFactory(postprocesser_name=None, **kwds):
    ep = ExtensionPoint(ISPOTPostprocesserRegistration)
    if postprocesser_name is None:
        return map(lambda x:(x.name,x.description), ep())
    service = ep.service(postprocesser_name)
    if service is None:
        # TODO - replace this print statement with a logging statement
        print "Unknown postprocesser '" + postprocesser_name + "' - no plugin registered for this postprocesser type"
        return None
    else:
        return ep.service(postprocesser_name).create(**kwds)


class Postprocesser(Component):
    """
    An object that encapsulates the postprocesser to be solved.
    """

    def __init__(self, problem):
        """
        Initialize here
        """
        Component.__init__(self)
        self.problem=problem

    def _create_parser(self):
        Component._create_parser(self)
        self.parser.add_option("--debug", 
            help="Debugging information", 
            action="store_true",
            dest="debug",
            default=False)

    def _create_parser_groups(self):
        Component._create_parser_groups(self)
        self._parser_group["Postprocesser Data"] = self.parser.add_argument_group("Postprocesser Data")

        self._parser_group["Postprocesser Data"].add_option("-o", "--output",
            help="Name of the output file that contains the sensor placement. "\
                "The default name is `<network-name>.sensors'.",
            action="store",
            dest="output_file",
            type="string",
            default="")

        self._parser_group["Postprocesser Data"].add_option("--summary",
            help="Name of the output file that contains summary information "\
                "about the sensor placement.",
            action="store",
            dest="summary_file",
            type="string",
            default="")

        self._parser_group["Postprocesser Data"].add_option("--format",
            help="Format of the summary information",
            action="store",
            dest="format",
            type="string",
            default="cout")

        self._parser_group["Postprocesser Data"].add_option("--eval-all",
            help="This option specifies that all impact files found will be used to "\
                "evaluate the final solution(s).",
            action="store_true",
            dest="eval_all",
            default=False)

    def _process_options(self):
        """
        Process postprocesser options
        """
        Component._process_options(self)
        #
        # Setup data
        #
        if self.options.output_file == "":
            self.options.output_file = self.problem.options.network + ".sensors"
        self.evalsensor = pyutilib.services.registered_executable("evalsensor")
        if self.evalsensor is None:
            print "ERROR: the 'evalsensor' command is not available"
            sys.exit(1)


    def _create_output_file(self, results):
        """
        Write the *.sensors file using the SolverResults object.

        This code assumes that the 's' variable is used to store the sensor locations.
        """
        OUTPUT = open(self.options.output_file,"w")
        print >>OUTPUT, "#"
        print >>OUTPUT, "# Generated by sp solver interface: " + time.asctime(time.gmtime(time.time()))
        print >>OUTPUT, "#"
        # This next lines are read by evalsensor
        for i in xrange(len(results.solution)):
            try:
                print >>OUTPUT, os.getpid(),
                keys = results.solution(i).variable.s.keys()
                keys.sort()
                ctr = 0
                for j in keys:
                    if results.solution(i).variable.s[j] is 1:
                        ctr += 1
                print >>OUTPUT, ctr," ",
                for j in keys:
                    if results.solution(i).variable.s[j] is 1:
                        print >>OUTPUT, str(j)," ",
                print >>OUTPUT, "\n#"
            except AttributeError:
                pass
        print >>OUTPUT, ""
        for line in str(results).split('\n'):
            print >>OUTPUT, "# "+line
        OUTPUT.close()

    def process(self, problem, results):
        """
        1. Process a Coopr results object to create a *.sensors file.
        2. Evaluate this file with 'evalsensor'.
        """
        self._create_output_file(results)
        if self.options.summary_file != "":
            pyutilib.misc.setup_redirect(self.options.summary_file)
        try:
            #
            # Print the lower bound, if it's available
            #
            if not results.problem.lower_bound is None:
                print "Objective lower bound: ", results.problem.lower_bound
            if len(results.solution) > 0:
                options=""
                #
                # The gamma option
                #
                try:
                    tmp = str(self.options.gamma)
                    options += " --gamma"+tmp
                except:
                    pass
                #
                # The costs option
                #
                if problem.options.cost_file != None :
                   options += " --costs=" + problem.options.cost_file
                #
                # Imperfect sensors options
                #
                try:
                    problem.options.imperfect_scfile
                    options += " --sc-probabilities=" + problem.imperfect_scfile + " --scs=" + problem.imperfect_jcfile
                except:
                    pass
                #
                # Standard options
                #
                options += " --responseTime="+str(problem.options.delay)+" --format=" + self.options.format + " --nodemap=" + problem.nodemap_file+" "+self.options.output_file
                #
                # Collect goals for the output
                #
                if self.options.eval_all:
                    goals = []
                    for goal in valid_objectives:
                        if (goal != "ns") and (goal != "cost") and (goal != "awd") and os.path.isfile(problem.impactdir + os.sep + problem.options.network + "_" + goal + ".impact"):
                            goals = goals + [ goal ]
                else:
                    goals = problem.goals
                #
                # Find the list of impact files that will be passed to 'evalsensor'
                #
                for goal in goals:
                  if (goal != "ns") and (goal != "cost") and (goal != "awd"):
                     options += " "  + problem.impactdir + os.sep + problem.options.network + "_" + goal + ".impact"
                # 
                # Setup and execute command
                #
                if ('all' in problem.global_options.memcheck) or ('evalsensor' in problem.global_options.memcheck):
                    valgrind=True
                    valgrind_log = problem.global_options.tmpname+"memcheck.evalsensor.log"
                else:
                    valgrind=False
                    valgrind_log = ""
    
                command = self.evalsensor.get_path()+options
                if problem.global_options.debug:
                    print "Running command...",command
                pyutilib.subprocess.run(command, valgrind=valgrind, valgrind_log=valgrind_log, memmon=problem.global_options.memmon)
        finally:
            #
            # Reset IO redirection
            #
            if self.options.summary_file != "":
                pyutilib.misc.reset_redirect()
    

pyutilib.services.register_executable("evalsensor")
