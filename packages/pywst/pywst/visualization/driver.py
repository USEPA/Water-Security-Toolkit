
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
            output_dir = os.path.dirname(App.opts['configure']['output prefix'])
            if len(output_dir) > 0:
                if not os.path.exists(output_dir):
                    os.mkdir(output_dir)
            App.setLogger('visualization')
        else: 
            raise RuntimeError("Invalid YAML configuration file")
    #
    if options.isTemplate:
        template_options = {
            'network': {'epanet file': 'Net3.inp'},
            'visualization': { 'screen': { 'size': [1200, 800]},
                               'legend': { 'location': [800, 20]},
                               'layers': [
                                          { 'label' : 'pipes with different colors',
                                            'location type': 'link',
                                            'locations': ['101', '171'],
                                            'fill': { 'color':['yellow', 'red'],
                                                      'opacity': [0.5,1],
                                                      'size': [10,20]},
                                          },
                                          { 'label': 'orange nodes',
                                            'location type': 'node',
                                            'locations': ['105', '35', '15'],
                                            'shape': 'diamond',
                                            'fill':{ 'color': 'orange', 
                                                     'size': 10,},
                                            'line': { 'color': 'black',
                                                      'opacity': 1,
                                                      'size': 1},
                                          },
                                  ],
                               },
                'configure': {'output prefix': 'Net3'}}
        App.saveTemplate(options.projectfile, template_options)
    else:
        App.run()

#
# Add a subparser for the wst command
#
setup_parser(
    pywst.common.add_subparser('visualization', 
        func=main_exec, help='Create an HTML visualization of the results from one of the other WST subcommands.', 
        description='This wst subcommand is used to create an HTML file with scalar vector graphics to visualize the network model and the results of one of the other subcommands.', 
        epilog=''
        ))

def main(args=None):
    parser = argparse.ArgumentParser()
    setup_parser(parser)
    parser.set_defaults(func=main_exec)
    ret = parser.parse_args(args)
    ret = ret.func(ret)

