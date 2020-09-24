
import argparse
import pywst.common
import problem
import os.path
import pyutilib.services
import sys

def setup_parser(parser):
    parser.add_argument("--template", dest="isTemplate", action="store_true",
                        default=False,
                        help="print a YAML configuration file with default settings (wst subcommand is not run)") 
    parser.add_argument("projectfile", metavar="CONFIGFILE", default=None,
                        help="a YAML configuration file")

def main_exec(options):
    App = problem.Problem()
    print ""
    if not options.isTemplate:
        if not os.path.exists(options.projectfile):
            raise IOError, "Missing configuration file '%s'" % options.projectfile
        if App.load(options.projectfile):
             # create output folder if [output prefix] is a path
            output_dir = os.path.dirname(App.opts['configure']['output prefix'])
            if len(output_dir) > 0:
                if not os.path.exists(output_dir):
                    os.mkdir(output_dir)
            App.validate()
        else: 
            raise RuntimeError("Invalid options")
    #
    if options.isTemplate:
        App.save(options.projectfile)
    #
    else:
        App.run()

#
# Add a subparser for the wst command
#
setup_parser(
    pywst.common.add_subparser('booster_quality', 
        func=main_exec, help='Place decontamination (booster) stations to maintain water quality', 
        description='This wst subcommand is used to optimize locations for booster stations that maintain water quality.', 
        epilog='Say more here about the usage of this command, and the form of the results.'
        ))


def main(args=None):
    parser = argparse.ArgumentParser()
    setup_parser(parser)
    parser.set_defaults(func=main_exec)
    ret = parser.parse_args(args)
    ret = ret.func(ret)

