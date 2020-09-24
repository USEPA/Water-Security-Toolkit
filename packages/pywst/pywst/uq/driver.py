
import argparse
from pywst.common import add_subparser 
import pywst.uq.problem as problem
import problem
import os.path
import pyutilib.services
import sys
from os.path import abspath, dirname, join

def setup_parser(parser):
    parser.add_argument("--template", dest="isTemplate", action="store_true",
                        default=False,
                        help="print a YAML configuration file with default settings (wst subcommand is not run)") 
    parser.add_argument("projectfile", metavar="CONFIGFILE", default=None,
                        help="a YAML configuration file")

def main_exec(options):
    App = problem.Problem()
    if not options.isTemplate:
        if not os.path.exists(options.projectfile):
            raise IOError, "Missing configuration file '%s'" % options.projectfile
        if App.load(options.projectfile):
            # create output folder if [output prefix] is a path
            output_dir = os.path.dirname(App.opts['configure']['output prefix'])
            if len(output_dir) > 0:
                if not os.path.exists(output_dir):
                    os.mkdir(output_dir)
            App.setLogger('uq')
        else: 
            raise RuntimeError("Invalid YAML configuration file")
    #
    if options.isTemplate:
        template_options = {
            'configure': {'output prefix': ''},
            'scenario': {'signals': 'SCENARIO_SIGNALS'},
            'measurements':{'grab samples': None},
            'uq': {'analysis time':60,
                   'threshold':1e-2}}
        App.saveTemplate(options.projectfile, template_options)
    else:
        App.run()

#
# Add a subparser for the wst command
#
setup_parser(
    add_subparser('uq', 
                  func=main_exec, help='Determine node contamination probabilities based on measurement information.', 
                  description='This wst subcommand is used to determine node probabilities of contamination in a water distribution system. The calculation is based on bayesian statistics.', 
                  epilog='Say more here about the usage of this command, and the form of the results.'
              ))


def main(args=None):
    parser = argparse.ArgumentParser()
    setup_parser(parser)
    parser.set_defaults(func=main_exec)
    ret = parser.parse_args(args)
    ret = ret.func(ret)

