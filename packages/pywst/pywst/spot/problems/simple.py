#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

from pywst.spot.core import *
import pywst.spot.util
import shutil
import pyutilib.misc
import pyutilib.services
import pyutilib.subprocess

class SimplePerfectSensorProblem(SPProblem):
    """
    A base class for variants of simple perfect-sensor problems
    """

    def __init__(self, statistic):
        """
        Initialize here
        """
        SPProblem.__init__(self)
        self.default_statistic = statistic
        self.ip_model = None
        self.aggregation_percent = {}
        self.aggregation_ratio = {}
        self.threshold = {}

    def _create_parser_groups(self):
        SPProblem._create_parser_groups(self)

        self._parser_group["Problem Formulation"].add_option("--ip-model",
            help="Specify the file name that defines the IP model.",
            action="store",
            dest="ip_model",
            default="GeneralSP.mod")

        self._parser_group["Aggregation"] = self.parser.add_argument_group("Aggregation")

        self._parser_group["Aggregation"].add_option("--aggregation-threshold","--threshold",
            help="Specifies the value (as `<goal>,<value>') used to aggregate `similar' " \
                "impacts.  This is used to reduce the total size of the " \
                "sensor placement formulation (for large problems). The " \
                "solution generated with non-zero thresholds is not " \
                "guaranteed to be globally optimal.",
            action="append",
            dest="aggregation_threshold",
            type="string",
            default=None)

        self._parser_group["Aggregation"].add_option("--aggregation-percent","--percent",
            help="A `<goal>,<value>' pair where value is a double between 0.0 and 1.0.  This is an " \
                "alternate way to compute the aggregation threshold. " \
                "Over all contamination incidents, we compute the " \
                "maximum difference d between the impact of the " \
                "contamination incident is not detected and the impact " \
                "it is detected at the earliest possible feasible " \
                "location.  We set the threshold " \
                "to d * aggregation_percent.  If both threshold and percent are " \
                "set to valid values in the command line, percent takes priority.",
            action="append",
            dest="aggregation_percent",
            type="string",
            default=[])

        self._parser_group["Aggregation"].add_option("--aggregation-ratio",
            help="A `<goal>,<value>' pair where value is a double between 0.0 and 1.0. "
                "This limits the aggregation "
                "by ensuring that for each block aggregated the ratio "
                "(lowest impact)/(highest impact) must be greater than or equal to this "
                "value.  The default value of this ratio is 1.0.  Setting this option "
                "to a value less than 1.0 only has an impact if the aggregation "
                "percent or threshold has been set to a non-zero value.  Otherwise "
                "the default zero-threshold aggregation is used, which is not "
                "impacted by this option.",
            action="append",
            dest="aggregation_ratio",
            type="string",
            default=None)

        self._parser_group["Aggregation"].add_option("--conserve-memory",
            help="If location aggregation is chosen, and the original impact files are " \
                "very large, you can choose to process them in a memory "\
                "conserving mode.  For example \"--conserve_memory=10000\" requests "\
                "that while original impact files are being processed into smaller "\
                "aggregated files, no more than 10000 impacts should be read into memory "\
                "at any one time.  Default is 10000 impacts.  Set to 0 to turn this off.",
            action="store",
            dest="maximum_impacts",
            type="int",
            default=10000)

        self._parser_group["Aggregation"].add_option("--distinguish-detection","--no-event-collapse",
            help="A goal for which aggregation should not allow incidents " \
                "to become trivial." \
                "That is, if the threshold is so large that all " \
                "locations, including the dummy, would form a single superlocation, " \
                "this forces the dummy to be in a superlocation by itself.  Thus the " \
                "sensor placement will distinguish between detecting and not detecting. " \
                "This option can be listed multiple times, to specify multiple goals."\
                "Note: the `detected' impact measures (e.g. dec, dvc) are always distinguished.",
            action="append",
            dest="distinguish_goal",
            type="string",
            default=[])

        self._parser_group["Aggregation"].add_option("--disable-aggregation",
            help="Disable aggregation for this goal, even at value zero, which "\
                "would incur no error.  Each witness incident will be in a separate "\
                "superlocation. "\
                "This option can be listed multiple times, to specify multiple goals. "\
                "You may list the goal `all' to specify all goals.",
            action="append",
            dest="disable_aggregation",
            type="string",
            default=[])

    def _process_options(self):
        """
        Process problem options
        """
        SPProblem._process_options(self)
        #
        # Error checking
        #
        if len(self.options.constraints) == 0:
            print "ERROR: no constraints specified."
            sys.exit(1)
        #
        # If they chose both --aggregation-threshold and --disable-aggregation
        # on the same objective, we disable.
        #
        if self.options.aggregation_threshold != None:
            for option in self.options.aggregation_threshold:
                tmp = option.split(",")
                if len(tmp) < 2:
                    print "ERROR: --aggregation_threshold=goal,amt or --threshold=goal,amt"
                    #print threshold_help
                    sys.exit(1)
            self.threshold[tmp[0]] = tmp[1] 
        #
        # TODO
        #
        if self.options.disable_aggregation != []:
            if self.options.disable_aggregation[0] == "all":
                for goal in valid_objectives:
                    self.threshold[goal] = -1.0
            else:
                for goal in self.options.disable_aggregation:
                    if not goal in valid_objectives:
                        print "ERROR: --disable_aggregation=goal"
                        print disable_aggregation_help
                        sys.exit(1)
                    self.threshold[goal] = -1.0
        #
        # TODO
        #
        if self.options.aggregation_percent != []:
            for option in self.options.aggregation_percent:
                tmp = option.split(",")
                if len(tmp) < 2:
                    print "ERROR: --aggregation_percent=goal,amt or --percent=goal,amt"
                    print percent_help
                    sys.exit(1)
                self.aggregation_percent[tmp[0]] = tmp[1]
        #
        # TODO
        #
        if self.options.aggregation_ratio != None:
            for option in self.options.aggregation_ratio:
                tmp = option.split(",")
                if len(tmp) < 2:
                    print "ERROR: --aggregation_ratio=goal,amt"
                    #print ratio_help
                    sys.exit(1)
                self.aggregation_ratio[tmp[0]] = tmp[1]
        #
        # Find the IP model file
        #
        if not os.path.isfile(self.options.ip_model):
            fname = pyutilib.misc.search_file(self.options.ip_model)
            self.ip_model = fname
        #
        # Distinguished goals
        #
        self.distinguish_detection = self.options.distinguish_goal + ["dec","dmc","dvc", "dtd","dpk","dpe","dpd"]
        self.aggregateImpacts = pyutilib.services.registered_executable("aggregateImpacts")


    def printParameters(self):
        """
        Print the parameters of the problem to be solved.
        """
        SPProblem.printParameters(self)

        if self.options.debug == True:
            print "Threshold:  ", self.threshold
            print "Percent:    ", self.options.aggregation_percent
            print "No collapse:", self.distinguish_detection
            print "Ratio:      ", self.options.aggregation_ratio
            print "IP Model:   ", self.ip_model

    def aggregate_impacts(self):
        """ Aggregate locations into superlocations.

          Create a new config file and new impact files that combine
          locations into superlocations, to reduce problem size.
          Writes new config file: tmpname_agg.config
        """
        if self.aggregateImpacts is None:
            print "WARNING: the 'aggregateImpacts' command is unavailable, so aggregation cannot be performed"
            return
        if self.total_number_side_constraints > 0:
            print "WARNING: Location aggregation does not work with side constraints"
            print "WARNING: Turning off location aggregation"
            return


        if ('all' in self.global_options.memcheck) or ('aggregateImpacts' in self.global_options.memcheck):
            valgrind = True
            valgrind_log = self.global_options.tmpname+"memcheck.aggregateImpacts.log"
        else:
            valgrind = False
            valgrind_log = ""

        logFileName = self.global_options.tmpname+"_aggregateImpacts.log"
        command = self.aggregateImpacts.get_path()+" "+self.global_options.tmpname+".config "
        if self.options.maximum_impacts > 0:
            command = command + " " + `self.options.maximum_impacts`
        if self.global_options.debug:
            print "Running command...",command
        pyutilib.subprocess.run(command, outfile=logFileName, valgrind=valgrind, valgrind_log=valgrind_log, memmon=self.             global_options.memmon)

        if self.global_options.memmon:
            pywst.spot.util.print_memmon_log("aggregateImpacts",logFileName)
        shutil.copyfile(self.global_options.tmpname+".config", self.global_options.tmpname+"_noagg.config")
        shutil.copyfile(self.global_options.tmpname+"_agg.config", self.global_options.tmpname+".config")


pyutilib.services.register_executable("aggregateImpacts")
