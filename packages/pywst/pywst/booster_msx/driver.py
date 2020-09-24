
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
            # create output folder if [output prefix] is a path
            output_dir = App.opts['configure']['output prefix']
            if output_dir:
                output_dir = os.path.dirname(output_dir)
                if len(output_dir) > 0:
                    if not os.path.exists(output_dir):
                        os.mkdir(output_dir)
            else:
                output_dir = ''
            App.setLogger('booster_msx')
            App.validate()
        else: 
            raise RuntimeError("Invalid YAML configuration file")

    if options.isTemplate:
        template_options = {
            'network': {'epanet file': 'Net3.inp'},
            'scenario': {'end time': 1440,
                        'location': [101],
                        'msx file': 'Net3_bio.msx',
                        'msx species': 'BIO',
                        'species': 'BIO',
                        'start time': 0,
                        'strength': 100,
                        'type': 'MASS'},
            'impact': {'metric': ['MC'], 
                        'msx species': 'BIO'},
            'booster msx': {'decon species': 'CLF',
                        'detection': [111, 127, 179],
                        'duration': 600,
                        'max boosters': 2,
                        'response time': 0,
                        'strength': 4,
                        'toxin species': 'BIO',
                        'type': 'FLOWPACED'},
             'solver': {'type': 'coliny_ea'},
             'configure': {'output prefix': 'Net3'}}
        App.saveTemplate(options.projectfile,template_options)
    else:
        App.run()

#
# Add a subparser for the wst command
#
setup_parser(
    pywst.common.add_subparser('booster_msx', 
        func=main_exec, help='Place decontamination (booster) stations', 
        description='This wst subcommand is used to optimize locations for booster stations.', 
        epilog='Say more here about the usage of this command, and the form of the results.'
        ))


def main(args=None):
    parser = argparse.ArgumentParser()
    setup_parser(parser)
    parser.set_defaults(func=main_exec)
    ret = parser.parse_args(args)
    ret = ret.func(ret)

