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


class PMedianProblem(SimplePerfectSensorProblem):
    """
    A base class for variants of p-median problems
    """

    def __init__(self):
        """
        Initialize here
        """
        SimplePerfectSensorProblem.__init__(self, "mean")

ProblemRegistration("simple-mean", PMedianProblem, description="Minimize mean with simple perfect sensors")
ProblemRegistration("pmedian", PMedianProblem, description="Minimize mean with simple perfect sensors")
