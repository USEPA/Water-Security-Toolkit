
import argparse
import pywst.common
import problem
import solver
import os.path
import pyutilib.services
import sys
from os.path import abspath, dirname, join

try:
    from collections import OrderedDict
except:
    from ordereddict import OrderedDict


def setup_parser(parser, spold=False):
    parser.add_argument("--template", dest="isTemplate", action="store_true", 
                        default=False,
                        help="print a YAML configuration file with default settings (wst subcommand is not run)")
    if not spold:
        parser.add_argument("--help-problems", dest="help_problems", action="store_true", 
                        default=False,
                        help="print a list of the sensor placement problem types that can be solved")
        parser.add_argument("--help-solvers", dest="help_solvers", action="store_true", 
                        default=False,
                        help="print a table that shows which solvers can be applied to which problem types")
        parser.add_argument("projectfile", metavar="CONFIGFILE", default=None, nargs='?',
                        help="a YAML configuration file")
    else:
        parser.add_argument("projectfile", metavar="CONFIGFILE", default=None,
                        help="a YAML configuration file")

def sp_exec(options):
    if options.help_problems:
        solver.help_problems()
        return
    if options.help_solvers:
        solver.help_solvers()
        return
    App = problem.Problem()
    if not options.isTemplate:
        if options.projectfile is None:
            raise IOError, "No configuration file was specified."
        if not os.path.exists(options.projectfile):
            raise IOError, "Configuration file '%s' does not exist." % options.projectfile
        if App.load(options.projectfile):
            # create output folder if [output prefix] has a path
            output_dir = App.opts['configure']['output prefix']
            if output_dir is None:
                output_dir = os.getcwd()
            else:
                output_dir = os.path.dirname(output_dir)
                if len(output_dir) > 0 and not os.path.exists(output_dir):
                    os.mkdir(output_dir)
            App.setLogger('sp')
            try:
                solver.run(App)
            finally:
                App.resetLogger('sp')
        else: 
            raise RuntimeError("Invalid YAML configuration file.")
    #
    else:
        if options.projectfile is None:
            raise IOError, "No configuration file was specified."
        template_options = {
        'impact data': [{'impact file': 'Net3_mc.impact','name': 'impact1','nodemap file': 'Net3.nodemap'}],
        'cost': [{'name': None}],
        'aggregate': [{'name': None}],
        'constraint': [{'bound': 5,'goal': 'NS','name': 'const1','statistic': 'TOTAL'}],
        'imperfect': [{'name': None}],
        'objective': [{'goal': 'impact1', 'name': 'obj1', 'statistic': 'MEAN'}],
        'sensor placement': {'constraint': 'const1',
                              'location': [{'feasible nodes': 'ALL'}],
                              'modeling language': 'NONE',
                              'objective': 'obj1',
                              'type': 'default'},
        'solver': {'type': 'snl_grasp'},
        'configure': {'output prefix': 'Net3'}}
        App.saveTemplate(options.projectfile,template_options)

def spold_exec(options):
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
            App.setLogger('sp')
            try:
                App.run_sp()
            finally:
                App.resetLogger('sp')
        else: 
            raise RuntimeError("Invalid YAML configuration file")
    #
    else:
        App.saveTemplate(options.projectfile,join(dirname(abspath(__file__)),'Net3_sp_simple.yml'))

#
# Add a subparser for the wst command
#
setup_parser(
    pywst.common.add_subparser('sp', 
        func=sp_exec, help='Place sensors to detect contaminants', 
        description='This wst subcommand is used to optimize locations for contaminant sensors.', 
        epilog='Say more here about the usage of this command, and the form of the results.'
        ))

setup_parser(
    pywst.common.add_subparser('spold', 
        func=spold_exec, help='Place sensors to detect contaminants', 
        description='This wst subcommand is used to optimize locations for contaminant sensors.', 
        epilog='Say more here about the usage of this command, and the form of the results.'
        ))


def sp_main(args=None):
    parser = argparse.ArgumentParser()
    setup_parser(parser)
    parser.set_defaults(func=sp_exec)
    ret = parser.parse_args(args)
    ret = ret.func(ret)

def spold_main(args=None):
    parser = argparse.ArgumentParser()
    setup_parser(parser, spold=True)
    parser.set_defaults(func=spold_exec)
    ret = parser.parse_args(args)
    ret = ret.func(ret)

