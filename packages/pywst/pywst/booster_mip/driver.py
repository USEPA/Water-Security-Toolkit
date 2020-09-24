\
import argparse
import pywst.common
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
            # create output folder(s) if [output prefix] contains
            # path elements
            prefix = App.opts['configure']['output prefix']
            if prefix is not None:
                output_dir = os.path.dirname(prefix)
                if (len(output_dir) > 0) and \
                   (not os.path.exists(output_dir)):
                    output_dir = os.path.abspath(output_dir)
                    os.makedirs(output_dir)

            App.setLogger('booster_mip')
            App.validate()
        else: 
            raise RuntimeError("Invalid options")

    if options.isTemplate:
        template_options = {
            'network': {'epanet file': 'Net3.inp'},
            'scenario': {'tsg file': 'Net3.tsg'},
            'booster mip': {'detection': [111, 127, 179],
                            'duration': 600,
                            'objective': 'MC',
                            'feasible nodes': 'ALL',
                            'fixed nodes': [],
                            'infeasible nodes': 'NONE',
                            'max boosters': 2,
                            'model format': 'PYOMO',
                            'model type': 'NEUTRAL',
                            'response time': 0,
                            'strength': 4,
                            'type': 'FLOWPACED'},
            'solver': {'type': 'glpk'},
            'configure': {'output prefix': 'Net3'}}
        App.saveTemplate(options.projectfile,template_options)
    else:
        App.run()

#
# Add a subparser for the wst command
#
setup_parser(
    pywst.common.add_subparser('booster_mip', 
        func=main_exec, help='Place decontamination (booster) stations for contamination response', 
        description='This wst subcommand is used to optimize locations for booster stations that activate in response to contamination events.', 
        epilog='Say more here about the usage of this command, and the form of the results.'
        ))


def main(args=None):
    parser = argparse.ArgumentParser()
    setup_parser(parser)
    parser.set_defaults(func=main_exec)
    ret = parser.parse_args(args)
    ret = ret.func(ret)

