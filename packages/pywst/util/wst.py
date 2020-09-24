#  _________________________________________________________________________
#
#  TEVA-WST Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  _________________________________________________________________________

import sys

class WstInstaller(Installer):

    def __init__(self):
        Installer.__init__(self)
        self.default_dirname='pywst'
        self.config_file='https://software.sandia.gov/svn/teva/wst/vpy/common/admin/installer.ini'
        self.pypi_config=False
        if sys.version_info[:2] < (2,5) or sys.version_info[:2] >= (3,0):
            print ""
            print "------------------------------------------------------------------"
            print "WARNING: The pywst package will only work with Python 2.5 - 2.7."
            print "         You attempted to installed pywst with:"
            print sys.version
            print "------------------------------------------------------------------"
            print ""
            sys.exit(1)

    def modify_parser(self, parser):
        Installer.modify_parser(self, parser)

        parser.add_option('--coin',
            help='Use one or more packages from the Coin Bazaar software repository.  Multiple packages are   specified with a comma-separated list.',
            action='store',
            dest='coin',
            default='')
        parser.remove_option('--release')
        parser.remove_option('--stable')

    def adjust_options(self, options, args):
        global logger
        verbosity = options.verbose - options.quiet
        self.logger = Logger([(Logger.level_for_integer(2-verbosity), sys.stdout)])
        logger = self.logger
        #
        if options.trunk:
            # Disable the release installation if --trunk or --stable is specified
            options.release = False
        else:
            options.release = True
        options.stable = False
        Installer.adjust_options(self, options, args)

    def get_other_packages(self, options):
        if options.coin is None:
            return
        for pkg in options.coin.split(','):
            if pkg is '':
                continue
            if sys.version_info < (2,6,4):
                self.add_repository(pkg, root='http://projects.coin-or.org/svn/CoinBazaar/projects/'+pkg, dev=True, username=os.environ.get('COINOR_USERNAME',None))
            else:
                self.add_repository(pkg, root='https://projects.coin-or.org/svn/CoinBazaar/projects/'+pkg, dev=True, username=os.environ.get('COINOR_USERNAME',None))


def create_installer():
    return WstInstaller()

