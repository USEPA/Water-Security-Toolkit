
import argparse
import pywst.common
import pywst.inversion.problem as problem
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
    parser.add_argument("--inp",dest="inp_file", action="store",default=None,
                        help="an optional inp file name that overrides the one inside the configfile")
    parser.add_argument("--meas", dest="measurements", action="store", default=None,
                        help=" an optional measurements file name that overrides the one inside the configfile")

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
            App.setLogger('inversion')
        else: 
            raise RuntimeError("Invalid YAML configuration file")
    #
    if options.isTemplate:
        template_options = {
            'network': {'epanet file': 'Net3.inp'},
            'measurements': {'grab samples': 'measures.dat'},
            'inversion': {'algorithm': 'optimization',
                       'candidate threshold': None,
                       'confidence': None,
                       'feasible nodes': None,
                       'formulation': 'MIP_discrete_nd',
                       'horizon': 1440,
                       'ignore merlion warnings': True,
                       'measurement failure': 0.05,
                       'model format': 'PYOMO',
                       'negative threshold': 0.1,
                       'num injections': 1.0,
                       'output impact nodes': False,
                       'positive threshold': 100.0},
         
         'solver': {'type': 'glpk'},
         'configure': {'output prefix': 'Net3'}}
        App.saveTemplate(options.projectfile, template_options)
    else:
        App.run(options)

#
# Add a subparser for the wst command
#
setup_parser(
    pywst.common.add_subparser('inversion', 
        func=main_exec, help='Determine contamination source using measurement data.', 
        description='This wst subcommand is used to determine likely contamination sources given measurement data.', 
        epilog='Say more here about the usage of this command, and the form of the results.'
        ))


def main(args=None):
    parser = argparse.ArgumentParser()
    setup_parser(parser)
    parser.set_defaults(func=main_exec)
    ret = parser.parse_args(args)
    ret = ret.func(ret)

