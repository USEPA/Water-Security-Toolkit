#  _________________________________________________________________________
#
#  Water Security Toolkit (WST)
#  Copyright (c) 2012 Sandia Corporation.
#  This software is distributed under the Revised BSD License.
#  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive 
#  license for use of this work by or on behalf of the U.S. government.
#  For more information, see the License Notice in the WST User Manual.
#  _________________________________________________________________________
#

import os
import os.path
import sys
import logging

try:
    import pkg_resources
    #
    # Load modules associated with Plugins that are defined in
    # EGG files.
    #
    for entrypoint in pkg_resources.iter_entry_points('pywst.common'):
        try:
            plugin_class = entrypoint.load()
        except:
            print "Error loading entry point %s:" % entrypoint
            import traceback
            traceback.print_exc()
except Exception, err:
    print "Error loading pywst.common entry points:",str(err),' entrypoint="%s"' % str(entrypoint)


import wst_parser

# Set the system-wide flag so errors deleting tempfiles are not fatal
from pyutilib.component.config import tempfiles
tempfiles.deletion_errors_are_fatal = False


def main(args=None):
    os.environ['PATH'] = os.environ['PATH'] + os.pathsep + os.path.join(os.path.dirname(sys.executable))
    parser = wst_parser.get_parser()
    if args is None:
        ret = parser.parse_args()
    else:
        ret = parser.parse_args(args)
    if ret.trace:
        # Execution the function option, passing the option values into the function
        ret.func(ret)
    else:
        try:
            ret.func(ret)
        except Exception, e:
            logger = logging.getLogger('wst')
            logger.info("\nWST abnormal termination")
            logger.info("---------------------------")
            logger.error(str(e))
            raise
