#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

__all__ = []

from pywst.spot.core import *
from simple import SimplePerfectSensorProblem


class RobustTailStatisticsProblem(SimplePerfectSensorProblem):
    """
    A worst-case p-median objective
    """

    def __init__(self, statistic):
        """
        Initialize here
        """
        SimplePerfectSensorProblem.__init__(self, statistic)

    def create_parser_groups(self):
        self._parser_group["Robust Statistics"] = self.parser.add_argument_group("Robust Statistics")

        self._parser_group["Robust Statistics"].add_option("-g", "--gamma",  
            help="Specifies the fraction of the distribution of impacts "\
                "that will be used to compute the var, cvar and tce "\
                "measures.  Gamma is assumed to lie in the interval "\
                "(0,1].  It can be interpreted as specifying the "\
                "100*gamma percent of the worst contamination incidents " \
                "that are used for these calculations.  Default: .05 ",
            action="store",
            dest="gamma",
            type="float",
            default=0.05)

    def _process_options(self):
        """
        Set problem options
        """
        SimplePerfectSensorProblem._process_options(self)
        #
        # Error checking
        #
        if (self.options.gamma <= 0.0) or (self.options.gamma > 1.0):
            print "ERROR: gamma must be in the interval (0,1]."
            sys.exit(1)


class SimpleCVaRProblem(RobustTailStatisticsProblem):
    """
    Minimize CVaR with simple perfect sensors.
    """

    def __init__(self, statistic):
        RobustTailStatisticsProblem.__init__(self, 'cvar')


class SimpleTCEProblem(RobustTailStatisticsProblem):
    """
    Minimize TCE with simple perfect sensors.
    """

    def __init__(self, statistic):
        RobustTailStatisticsProblem.__init__(self, 'tce')


class SimpleVaRProblem(RobustTailStatisticsProblem):
    """
    Minimize VaR with simple perfect sensors.
    """

    def __init__(self, statistic):
        RobustTailStatisticsProblem.__init__(self, 'var')


class SimpleWorstCaseProblem(SimplePerfectSensorProblem):
    """
    Minimize worst-case with simple perfect sensors.
    """

    def __init__(self):
        SimplePerfectSensorProblem.__init__(self, "worst")


ProblemRegistration("simple-cvar", SimpleCVaRProblem, description="Minimize CVaR with simple perfect sensors")
ProblemRegistration("simple-var", SimpleVaRProblem, description="Minimize VaR with simple perfect sensors")
ProblemRegistration("simple-tce", SimpleTCEProblem, description="Minimize TCE with simple perfect sensors")
ProblemRegistration("simple-worst", SimpleWorstCaseProblem, description="Minimize worst-case with simple perfect sensors")

