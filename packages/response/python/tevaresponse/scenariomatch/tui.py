#  _________________________________________________________________________
#
#  TEVA-SPOT Toolkit: Tools for Designing Contaminant Warning Systems
#  Copyright (c) 2008 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README file in the top software directory.
#  _________________________________________________________________________

from core import merlionBool

def __load_samples_interactive(self):
    print 'Please enter the collected sample data'
    print 'Type <CONTROL>+<C> when you are done entering data'
    try:
        ct = 0
        while True:
            nodeName = raw_input('  Location: ')
            nodeName = nodeName.lower()
            nodeName = nodeName.strip()
            if not self.nodeNameToID.has_key(nodeName) and \
                   not self.nodeIdToName.has_key(nodeName):
                print '  *** Location "%s" does not exist!' % nodeName
                continue
            nde = nodeName
            ndeTime = raw_input('  Time of sample (hour of day): ')
            time = int(ndeTime)
            if self.currentTime is None:
                self.currentTime = time
            elif self.currentTime < time:
                self.currentTime = time
            ndeDetect = raw_input('  Anomaly detected (yes/no): ')
            detect = merlionBool(ndeDetect)
            if detect is None:
                print '  *** You must use "yes", "no", "true" or "false"'
                continue
            self.add_result(nde, time, detect, ct)
            ct = ct + 1
    except (KeyboardInterrupt, EOFError):
        print '\n Done reading data from user.'
    return
