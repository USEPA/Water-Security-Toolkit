
import re
import gzip
import pywst.spot.core
from pyutilib.component.core import alias
import pyutilib.services


def convert_tuning(data, filename, location_category, options):
    #print data
    OUTPUT = open(filename,'w')
    location = {}
    ctr=1
    print >>OUTPUT, "param P := %d;" % options.num_sensors
    print >>OUTPUT, "param fp_threshold := %f;" % options.fp_threshold
    #
    print >>OUTPUT, "set TuningLevels :=",
    levels = sorted(options.fp.keys())
    for i in levels:
        print >>OUTPUT, " "+str(i),
    print >>OUTPUT, " ;"
    #
    locations=[]
    for i in data.keys():
        locations += data[i].location_map
    print >>OUTPUT, "set Locations := ",
    lkeys = list(set(locations))
    lkeys.sort()
    for id in lkeys:
        print >>OUTPUT, id,
    print >>OUTPUT, ";"
    #
    customers=[]
    for i in data.keys():
        customers += data[i].impact.keys()
    print >>OUTPUT, "set Customers := ",
    ckeys = list(set(customers))
    ckeys.sort()
    for id in ckeys:
        print >>OUTPUT, id,
    print >>OUTPUT, ";"
    #
    print >>OUTPUT, "set ValidIndices :="
    for c in ckeys:
        for l in lkeys:
            for i in levels:
                if l == -1 or l in data[location_category[l]].impact[c]:
                    print >>OUTPUT, l, c, i
    print >>OUTPUT, ";"
    #
    print >>OUTPUT, "param d :="
    for c in ckeys:
        for l in lkeys:
            for i in levels:
                if l == -1:
                    print >>OUTPUT, l, c, i, data[1].impact[c][l]
                else:
                    k = location_category[l]
                    if l in data[k].impact[c]:
                        #print "HERE",l,c,i,k,data[k].impact[c].keys()
                        #print "HERE x", k,i,options.pd[k][i]
                        #print "HERE x", data[k].impact[c].keys(), 1 in data[k].impact[c]
                        #print "HERE x", k, c, l, data[k].impact[c][l]
                        print >>OUTPUT, l, c, i, options.pd[k][i]*data[k].impact[c][l] + (1.0-options.pd[k][i])*data[k].impact[c][-1]
    print >>OUTPUT, ";"
    #
    print >>OUTPUT, "param fp :="
    for l in lkeys:
        for i in levels:
            print >>OUTPUT, l, i, options.fp[i]
    print >>OUTPUT, ";"
    OUTPUT.close()


class impact2dat_tuning_Task(pywst.spot.core.Task):

    alias('impact2dat_tuning')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        self.inputs.declare('impact_data')
        self.inputs.declare('num_sensors')
        self.inputs.declare('num_levels')
        self.inputs.declare('fp_threshold')
        self.inputs.declare('fp')
        self.inputs.declare('pd')
        #self.inputs.declare('nmap')
        self.inputs.declare('location_categories')
        #
        self.outputs.declare('dat_filename')

    def execute(self):
        if self.dat_filename is None:
            self.dat_filename = pyutilib.services.TempfileManager.create_tempfile(suffix='.dat')
        convert_tuning(self.impact_data, self.dat_filename, self.location_categories, self)


