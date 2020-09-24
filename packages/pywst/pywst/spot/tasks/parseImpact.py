
import re
import gzip
import pywst.spot.core
from pyutilib.component.core import alias
from pyutilib.misc import Container


def output_file(filename):
    return filename[:-6]+"dat"


def parse_simple_impact(filename):
    if filename.endswith('.gz'):
        f = gzip.open(filename, 'rb')
        f.read().split()
        f.close()
    INPUT=open(filename, 'r')
    INPUT.readline()
    INPUT.readline()
    #
    data = {}
    locations = set()
    newscenario=True
    for line in INPUT:
        tokens = map(eval, re.split('[ \t]+', line.strip()))
        if newscenario:
            data[tokens[0]] = {}
            newscenario = False
        if tokens[1] == -1:
            newscenario = True
        locations.add(tokens[1])
        data[tokens[0]][tokens[1]] = tokens[3]
    #
    INPUT.close()
    locmap={}
    for loc in locations:
        locmap[loc]=loc
    return Container(impact=data, location_map=locmap)


class parseSimpleImpact_Task(pywst.spot.core.Task):

    alias('parse_simple_impact')

    def __init__(self, *args, **kwds):
        pywst.spot.core.Task.__init__(self, *args, **kwds)
        #
        self.inputs.declare('impact_filename')
        self.inputs.declare('tso_impact_data')
        #
        self.outputs.declare('impact_data')
        self.outputs.declare('default_dat_filename')

    def execute(self):
        if not self.impact_filename is None:
            filename = self.impact_filename
        elif not self.tso_impact_data is None:
            filename = self.tso_impact_data.impact
        #
        self.impact_data = parse_simple_impact(filename)
        self.default_dat_filename = output_file(filename)


