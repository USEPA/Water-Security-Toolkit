
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
def lagrangian_default(data, prob=None):
    """
    Perform optimization with the Lagrangian solver.

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
    if len(con_name) != 1:
        raise RuntimeError("The Lagrangian solver expected a single constraint name")
    con_name = con_name[0]

    aggr_name = prob.getProblemOption('aggregate', prob_name)
    goal_name = prob.getConstraintOption('goal', con_name)
    if lower(goal_name) != 'ns':    
        cost_name = prob.getCostOption('cost file', goal_name)
        if not cost_name is None:
            raise RuntimeError, "Cannot use a cost constraint goal with the %s solver" % solvername
    else:
        ns = prob.getConstraintOption('bound', con_name)
    if aggr_name is not None:
        raise RuntimeError, "Cannot use a aggregation with the %s solver" % solvername
    #
    # Create the Lagrangian data file
    #
    createLagDataExecutable = pyutilib.services.registered_executable('createLagData')
    if createLagDataExecutable is None:
        raise RuntimeError("Cannot optimize with solver '%s'.  Executable 'createLagData' is missing." % solvername)
    uflExecutable = pyutilib.services.registered_executable('ufl')
    if uflExecutable is None:
        raise RuntimeError("Cannot optimize with solver '%s'.  Executable 'ufl' is missing." % solvername)
    #
    # Create the config file
    #
    config_file = prob.create_wst_configfile(data.prefix)
    pyutilib.services.TempfileManager.add_tempfile(config_file, False)
    #
    if debug:
        print "Setting up Lagrangian data files..."
    lagdata_file = pyutilib.services.TempfileManager.create_tempfile(prefix=data.prefix, suffix='.lag')
    gamma = prob.getProblemOption('gamma', prob_name)
    if gamma is None:
        gamma = []
    else:
        gamma = ["--gamma", str(gamma)]
    cmd = [createLagDataExecutable.get_path()] + gamma + ["--output", lagdata_file, config_file]
    if debug:
        print "Running command ..."
        print " ".join(cmd)
        rc, output = pyutilib.subprocess.run(cmd, tee=True)
    else:
        rc, output = pyutilib.subprocess.run(cmd)
    if rc != 0:
        print "ERROR executing the '%s' command" % solvername
        raise RuntimeError, "Bad return code for createLagData: %d" % rc
    #
    # Setup the Lagrangian command-line
    #
    cmd = [ uflExecutable.get_path() ]
    #
    cmd.append( lagdata_file )
    cmd.append( str(int(ns)) )
    #
    gap = prob.getSolverOption('gap')
    if gap is None:
        cmd.append( '0' )
    else:
        cmd.append(str(int(gap)))
    #
    json_file = data.prefix + '.json'
    pyutilib.services.TempfileManager.add_tempfile(json_file, False)
    cmd.append(json_file)
    #
    # Run the solver
    #
    data.command_line = ' '.join(cmd)
    if debug or prob.getSolverOption('verbose'):
        print "Running command ..."
        print data.command_line
        rc, output = pyutilib.subprocess.run(cmd, tee=True)
    else:
        rc, output = pyutilib.subprocess.run(cmd)
    if rc != 0:
        print "ERROR executing the '%s' command" % solvername
        raise RuntimeError, "Bad return code for lagrangian solver: %d" % rc
    #
    # Read the JSON file
    #
    #INPUT = open(json_file,'r')
    #for line in INPUT:
    #    #print line,
    #INPUT.close()
    INPUT = open(json_file,'r')
    results = json.load(INPUT)
    INPUT.close()
    soln = []
    if not prob.getProblemOption('compute bound'):
        for i in range(len(results['solutions'])):
            soln.append( results['solutions'][i]['ids'] )
            val = results['solutions'][i]['value']
            if data.objective is None or val < data.objective:
                data.objective = val
        data.upper_bound = val
    data.solutions = soln
    data.lower_bound = results['lower bound']

