#  _________________________________________________________________________
#
#  Coopr: A COmmon Optimization Python Repository
#  Copyright (c) 2010 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the Coopr README.txt file.
#  _________________________________________________________________________

from pyomo.environ import *
#from coopr.core.plugin import *

from pyomo.pysp import solutionwriter
from pyomo.pysp.scenariotree import *

import string
import json

#
# a simple utility to munge the index name into something a bit more json-friendly and
# in general more readable. at the current time, we just eliminate any leading and trailing
# parantheses and change commas to colons - the latter because it's a json file!
#

def index_to_string(index):
    result = str(index)
    result = result.lstrip('(').rstrip(')')
    result = result.replace(',',':')
    result = result.replace(' ','')
    return result


class JSONSolutionWriter(SingletonPlugin):

    implements (solutionwriter.ISolutionWriterExtension)

    def write(self, scenario_tree, instance_dictionary, output_file_prefix):
        if not isinstance(scenario_tree, ScenarioTree):
            raise RuntimeError, "JSONSolutionWriter write method expects ScenarioTree object - type of supplied object="+str(type(scenario_tree))
        #
        output_filename = output_file_prefix + ".json"
        output_file = open(output_filename,"w")
        #
        data = {}
        for stage in scenario_tree._stages:
            stage_name = stage._name
            if not stage_name in data:
                data[stage_name] = {}
            for tree_node in stage._tree_nodes:
                tree_node_name = tree_node._name
                if not tree_node_name in data[stage_name]:
                    data[stage_name][tree_node_name] = {}
                for variable_id, (var_name, index) in iteritems(tree_node._variable_ids):
                    if not var_name in data[stage_name][tree_node_name]:
                        data[stage_name][tree_node_name][var_name] = {}
                    print stage_name, tree_node_name, var_name, index
                    data[stage_name][tree_node_name][var_name][str(index_to_string(index))] = tree_node._solution[variable_id]
                    
        #
        #data['ScenarioCosts'] = {}
        print 'Scenario Costs'
        for stage in scenario_tree._stages:
            #data['ScenarioCosts'].setdefault(stage._name, {})
            print stage._name,
            for tree_node in stage._tree_nodes:
                #data['ScenarioCosts'][stage._name].setdefault(tree_node._name, {})
                print tree_node._name
                #for scenario in tree_node._scenarios:
                #    instance=ph._instances[scenario._name]
                #    print "    ",scenario._name, stage._cost_variable[0].name,                getattr(instance, stage._cost_variable[0].name)[stage._cost_variable[1]].value

        #
        print >>output_file, json.dumps(data, indent=2, sort_keys=True)
        output_file.close()
        #
        print "Scenario tree solution written to file="+output_filename

