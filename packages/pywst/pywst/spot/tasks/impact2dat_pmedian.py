
import re
import gzip
import pywst.spot.core
from pyutilib.component.core import alias
import pyutilib.services


def convert_GeneralSP():
    pass


def convert_pmedian(data, filename, options):
    OUTPUT = open(filename,'w')
    location = {}
    ctr=1
    print >>OUTPUT, "param P := %d;" % options.num_sensors
    print >>OUTPUT, "set Locations := ",
    lkeys = list(data.location_map)
    lkeys.sort()
    for id in lkeys:
        #location[id] = ctr
        print >>OUTPUT, id,
        ctr += 1
    #print "HERE",keys
    print >>OUTPUT, ";"
    print >>OUTPUT, "set Customers := ",
    customer = {}
    ctr=1
    ckeys = data.impact.keys()
    ckeys.sort()
    for id in ckeys:
        #customer[id] = ctr
        print >>OUTPUT, id,
        ctr += 1
    print >>OUTPUT, ";"
    #print >>OUTPUT, "param N := %d;" % len(location)
    #print >>OUTPUT, "param M := %d;" % len(customer)
    print >>OUTPUT, "param : ValidIndices : d :="
    for c in ckeys:
        for l in lkeys:
            if l in data.impact[c]:
                print >>OUTPUT, l, c, data.impact[c][l]
    print >>OUTPUT, ";"
    OUTPUT.close()


class impact2simpleDat_Task(pywst.spot.core.Task):

    alias('impact2dat_pmedian')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        self.inputs.declare('impact_data')
        self.inputs.declare('num_sensors')
        #
        self.outputs.declare('dat_filename')

    def execute(self):
        if self.dat_filename is None:
            self.dat_filename = pyutilib.services.TempfileManager.create_tempfile(suffix='.dat')
        convert_pmedian(self.impact_data, self.dat_filename, self)


