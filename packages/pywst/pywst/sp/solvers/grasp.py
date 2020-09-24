
import json
import os, sys
import os.path
import shutil
from string import lower

import pyutilib.services
import pyutilib.subprocess
from pyutilib.workflow import functor_api
from pywst.sp.util import get_solvername


@functor_api(namespace='pywst.sp')
def grasp_default(data, prob=None):
    """
    Perform optimization with grasp.

    Required:
        prob:       Problem instance
    """
    #
    # Error Checking
    #
    debug = prob.getConfigureOption('debug')
    solvername = get_solvername(prob)
    prob_name = prob.getConfigureOption('problem')
    con_name = prob.getProblemOption('constraint', prob_name)
    aggr_name = prob.getProblemOption('aggregate', prob_name)
    if len(con_name) > 1:
        raise RuntimeError, "Cannot use multiple constrains with with the %s solver" % solvername
    goal_name = prob.getConstraintOption('goal', con_name[0])
    if lower(goal_name) != 'ns':    
        cost_name = prob.getCostOption('cost file', goal_name)
        if not cost_name is None:
            raise RuntimeError, "Cannot use a cost constraint goal with the %s solver" % solvername
    if aggr_name is not None:
        raise RuntimeError, "Cannot use a aggregation with the %s solver" % solvername
    #
    # Create the config file
    #
    config_file = prob.create_wst_configfile(data.prefix)
    pyutilib.services.TempfileManager.add_tempfile(config_file, False)
    #
    # Setup the grasp command-line
    #
    cmd = None
    if solvername == 'att_grasp':
        cmd = pyutilib.services.registered_executable('randomsample')
    elif solvername == 'snl_grasp':
        cmd = pyutilib.services.registered_executable('new_randomsample')
    if cmd is None:
        raise RuntimeError, "Cannot optimize with solver '%s'" % solvername
    else:
        cmd = [ cmd.get_path() ]
    #
    cmd.append( config_file )
    #
    numsamples = prob.getSolverOption('number of samples')
    if numsamples is None:
        if data.ptype == 'worst-case perfect-sensor':
            cmd.append( '10' )
        else:
            cmd.append( '20' )
    else:
        cmd.append(str(numsamples))
    #
    seed = prob.getSolverOption('seed')
    if seed is None:
        cmd.append( '1' )
    else:
        cmd.append(str(seed))
    #
    repn = prob.getSolverOption('representation')
    if repn is None:
        cmd.append( '1' )
    else:
        cmd.append(str(repn))
    #
    timelimit = prob.getSolverOption('timelimit')
    if timelimit is None:
        cmd.append( '0.0' )
    else:
        cmd.append(str(timelimit))
    #
    sensors_file = data.prefix + '.sensors'
    pyutilib.services.TempfileManager.add_tempfile(sensors_file, False)
    cmd.append(sensors_file)
    #
    json_file = data.prefix + '.json'
    pyutilib.services.TempfileManager.add_tempfile(json_file, False)
    cmd.append(json_file)
    #
    # Run the solver
    #
    data.command_line = ' '.join(cmd)
    if debug:
        print "Running command ..."
        print data.command_line
        rc, output = pyutilib.subprocess.run(cmd, tee=True)
    else:
        rc, output = pyutilib.subprocess.run(cmd)
    if rc != 0:
        print "ERROR executing the '%s' command" % solvername
        raise RuntimeError, "Bad return code for grasp solver: %d" % rc
    #
    # Read the JSON file
    #
    INPUT = open(json_file,'r')
    results = json.load(INPUT)
    INPUT.close()
    soln = []
    for i in range(len(results['solutions'])):
        soln.append( results['solutions'][i]['ids'] )
        val = results['solutions'][i]['value']
        if data.objective is None or val < data.objective:
            data.objective = val
    data.solutions = soln
    data.upper_bound = data.objective

