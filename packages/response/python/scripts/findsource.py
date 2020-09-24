#!/usr/local/bin/python
#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

'''This program takes in a sampling results file and then compares
those sampling results to the remaining possible scenarios,
found in an additional input file, to eliminate those scenarios
that do not match the results. When the program is run with an
input file containing results of initial incidence of
contamination, the program goes through the file containing all
incidences of contamination in all of the pre-calculated
scenarios to determine the remaining possible scenarios.
It then uses a node-selecting algorithm, either the default
algorithm, Decorrelation, or the optional algorithm, MaxEnt,
to determine the nodes to be sampled in the next time step. 
'''

import sys, os
from sys import stdin, stdout, stderr
from tevaresponse import scenariomatch

def main( ):
    parser = scenariomatch.get_sm_parser()
    (options, args) = parser.parse_args()
    if len(sys.argv) < 2: options = scenariomatch.gui.get_options(options)
    db = scenariomatch.ScenarioDB()
    db.load_options(options)
    db.init = True
    db.load_libraries()
    db.make_database()
    bDone = False
    while not bDone:
        if not options.interactive: bDone = True
        db.load_samples()
        db.get_remaining_scenarios()
        db.set_possible_nodes_times()
        db.print_possible_nodes_times(options.psfileout)
        db.find_next_samples(options.nodes)
        if (options.interactive or
                options.pnfileout is None or
                options.pnfileout == ''): 
            db.print_next_samples()
        if (options.pnfileout is not None and not (options.pnfileout == '')):
            db.print_next_samples(options.pnfileout)
        if not bDone:
            isDone = raw_input('Type "quit" to exit or anything else to continue. ')
            isDone = isDone.strip().lower()
            if isDone == 'quit': bDone = True
    
if (__name__ == "__main__"):
    main()
    exit(0)
    
