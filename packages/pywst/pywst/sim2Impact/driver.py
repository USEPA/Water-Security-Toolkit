
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
            # create output folder if [output prefix] has a path
            output_dir = os.path.dirname(App.opts['configure']['output prefix'])
            if len(output_dir) > 0:
                if not os.path.exists(output_dir):
                    os.mkdir(output_dir)
            App.setLogger('sim2Impact')
        else: 
            raise RuntimeError("Invalid YAML configuration file")
    
    if options.isTemplate:
        template_options = {
            'impact': {'erd file': ['Net3.erd'], 
                        'metric': ['MC']},
            'configure': {'output prefix': 'Net3'}}
        App.saveTemplate(options.projectfile,template_options)
    else:
        App.run()

#
# Add a subparser for the wst command
#
setup_parser(
    pywst.common.add_subparser('sim2Impact', 
        func=main_exec, help='Compute impacts using sim2Impact', 
        description='This wst subcommand is used to run sim2Impact.', 
        epilog='The sim2Impact command processes TSO and ERD Files and generates one or more IMPACT Files. An IMPACT File is a plain text File that summarizes the consequence of each contamination incident.'
        ))


def main(args=None):
    parser = argparse.ArgumentParser()
    setup_parser(parser)
    parser.set_defaults(func=main_exec)
    ret = parser.parse_args(args)
    ret = ret.func(ret)

