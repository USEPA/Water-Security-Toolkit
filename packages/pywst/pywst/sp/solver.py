#
# hard/soft constraints (?)
# full/aggregated
#

# two-tiered solver...
#   unknown obj
#   0 constraints
#   perfect
#   single obj
#   1-stage

import json, yaml
import time
import os, sys, datetime
import os.path
import logging
from string import lower
try:
    from collections import OrderedDict
except:
    from ordereddict import OrderedDict

from pyutilib.misc import Options
import pyutilib.services
import pyutilib.common
import pywst.common.wst_config as wst_config
import pywst.common.wst_util as wst_util
from pyutilib.misc.config import ConfigBlock
from pyutilib.workflow import FunctorAPIFactory

from problem import Problem
from util import evalsensors
from pyomo.environ import SolverFactory

currdir = os.path.dirname(os.path.abspath(__file__))+os.sep

logger = logging.getLogger('wst.sp')

def get_solver_availability():
    logging.disable(logging.WARNING)
    available = {}
    try:
        opt = SolverFactory('pico')
        available['pico'] = opt.available(False) and (not opt.executable() is None)
        del opt
    except pyutilib.common.ApplicationError:
        available['pico'] = False
    try:
        opt = SolverFactory('cplex')
        available['cplex'] = opt.available(False) and (not opt.executable() is None)
        del opt
    except pyutilib.common.ApplicationError:
        available['cplex'] = False
    try:
        opt = SolverFactory('cplex_direct')
        available['cplex_direct'] = opt.available(False) and (not opt.executable() is None)
        del opt
    except pyutilib.common.ApplicationError:
        available['cplex_direct'] = False
    try:
        opt = SolverFactory('gurobi')
        available['gurobi'] = opt.available(False) and (not opt.executable() is None)
        del opt
    except pyutilib.common.ApplicationError:
        available['gurobi'] = False
    try:
        opt = SolverFactory('gurobi_direct')
        available['gurobi_direct'] = opt.available(False) and (not opt.executable() is None)
        del opt
    except pyutilib.common.ApplicationError:
        available['gurobi_direct'] = False
    try:
        opt = SolverFactory('glpk')
        available['glpk'] = opt.available(False) and (not opt.executable() is None)
        del opt
    except pyutilib.common.ApplicationError:
        available['glpk'] = False
    try:
        opt = SolverFactory('cbc')
        available['cbc'] = opt.available(False) and (not opt.executable() is None)
        del opt
    except pyutilib.common.ApplicationError:
        available['cbc'] = False
    try:
        opt = SolverFactory('xpress')
        available['xpress'] = opt.available(False) and (not opt.executable() is None)
        del opt
    except pyutilib.common.ApplicationError:
        available['xpress'] = False
    logging.disable(logging.NOTSET)
    return available


descr = OrderedDict([
(('default', 'p-median', 'average-case perfect-sensor'),
"""
mean obj
0 constraints
perfect
single obj
1 stage
exact
"""),

(('p-center', 'worst-case perfect-sensor'),
"""
worst obj
0 constraints
perfect
single obj
1 stage
exact
"""),

('robust-cvar perfect-sensor',
"""
cvar obj
0 constraints
perfect
single obj
1 stage
exact
"""),

('side-constrained',
"""
unknown obj
1+ constraints
perfect
single obj
1 stage
exact
"""),

('min-sensors',
"""
min sensors
1+ constraints
perfect
single obj
1 stage
exact
"""),

('one imperfect witness',
"""
mean obj
0 constraints
imperfect
single obj
1 stage
exact
"""),

('multi-stage',
"""
mean obj
0 constraints
perfect
single obj
n stage
exact
"""),

('custom',
"""
0 constraints/1+ constraints
perfect/imperfect sensors
single/multi objective
1 stage/n stage
exact/bound
""")
])


#
# TODO: add glpk_direct and cplex_direct solvers (?)
#
problems = {
('pico', 'pyomo', 'default'):
'pywst.sp.pyomo_GeneralSP_default', 
('pico', 'pyomo', 'p-median'):
'pywst.sp.pyomo_GeneralSP_pmedian', 
('pico', 'pyomo', 'average-case perfect-sensor'): 
'pywst.sp.pyomo_GeneralSP_avgcase_perfect',
('pico', 'pyomo', 'p-center'):
'pywst.sp.pyomo_GeneralSP_pcenter',
('pico', 'pyomo', 'worst-case perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_worstcase_perfect',
('pico', 'pyomo', 'robust-cvar perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_robust_cvar_perfect',
('pico', 'pyomo', 'side-constrained'):
'pywst.sp.pyomo_GeneralSP_sc',
('pico', 'pyomo', 'min-sensors'):
'pywst.sp.pyomo_GeneralSP_min_sensors',

('xpress', 'pyomo', 'default'): 
'pywst.sp.pyomo_GeneralSP_default',
('xpress', 'pyomo', 'p-median'): 
'pywst.sp.pyomo_GeneralSP_pmedian',
('xpress', 'pyomo', 'average-case perfect-sensor'): 
'pywst.sp.pyomo_GeneralSP_avgcase_perfect',
('xpress', 'pyomo', 'p-center'):
'pywst.sp.pyomo_GeneralSP_pcenter',
('xpress', 'pyomo', 'worst-case perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_worstcase_perfect',
('xpress', 'pyomo', 'robust-cvar perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_robust_cvar_perfect',
('xpress', 'pyomo', 'side-constrained'):
'pywst.sp.pyomo_GeneralSP_sc',
('xpress', 'pyomo', 'min-sensors'):
'pywst.sp.pyomo_GeneralSP_min_sensors',

('cplex', 'pyomo', 'default'): 
'pywst.sp.pyomo_GeneralSP_default',
('cplex', 'pyomo', 'p-median'): 
'pywst.sp.pyomo_GeneralSP_pmedian',
('cplex', 'pyomo', 'average-case perfect-sensor'): 
'pywst.sp.pyomo_GeneralSP_avgcase_perfect',
('cplex', 'pyomo', 'p-center'):
'pywst.sp.pyomo_GeneralSP_pcenter',
('cplex', 'pyomo', 'worst-case perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_worstcase_perfect',
('cplex', 'pyomo', 'robust-cvar perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_robust_cvar_perfect',
('cplex', 'pyomo', 'side-constrained'):
'pywst.sp.pyomo_GeneralSP_sc',
('cplex', 'pyomo', 'min-sensors'):
'pywst.sp.pyomo_GeneralSP_min_sensors',

('gurobi', 'pyomo', 'default'): 
'pywst.sp.pyomo_GeneralSP_default',
('gurobi', 'pyomo', 'p-median'): 
'pywst.sp.pyomo_GeneralSP_pmedian',
('gurobi', 'pyomo', 'average-case perfect-sensor'): 
'pywst.sp.pyomo_GeneralSP_avgcase_perfect',
('gurobi', 'pyomo', 'p-center'):
'pywst.sp.pyomo_GeneralSP_pcenter',
('gurobi', 'pyomo', 'worst-case perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_worstcase_perfect',
('gurobi', 'pyomo', 'robust-cvar perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_robust_cvar_perfect',
('gurobi', 'pyomo', 'side-constrained'):
'pywst.sp.pyomo_GeneralSP_sc',
('gurobi', 'pyomo', 'min-sensors'):
'pywst.sp.pyomo_GeneralSP_min_sensors',

('glpk', 'pyomo', 'default'): 
'pywst.sp.pyomo_GeneralSP_default',
('glpk', 'pyomo', 'p-median'): 
'pywst.sp.pyomo_GeneralSP_pmedian',
('glpk', 'pyomo', 'average-case perfect-sensor'): 
'pywst.sp.pyomo_GeneralSP_avgcase_perfect',
('glpk', 'pyomo', 'p-center'):
'pywst.sp.pyomo_GeneralSP_pcenter',
('glpk', 'pyomo', 'worst-case perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_worstcase_perfect',
('glpk', 'pyomo', 'robust-cvar perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_robust_cvar_perfect',
('glpk', 'pyomo', 'side-constrained'):
'pywst.sp.pyomo_GeneralSP_sc',
('glpk', 'pyomo', 'min-sensors'):
'pywst.sp.pyomo_GeneralSP_min_sensors',

('cbc', 'pyomo', 'default'): 
'pywst.sp.pyomo_GeneralSP_default',
('cbc', 'pyomo', 'p-median'): 
'pywst.sp.pyomo_GeneralSP_pmedian',
('cbc', 'pyomo', 'average-case perfect-sensor'): 
'pywst.sp.pyomo_GeneralSP_avgcase_perfect',
('cbc', 'pyomo', 'p-center'):
'pywst.sp.pyomo_GeneralSP_pcenter',
('cbc', 'pyomo', 'worst-case perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_worstcase_perfect',
('cbc', 'pyomo', 'robust-cvar perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_robust_cvar_perfect',
('cbc', 'pyomo', 'side-constrained'):
'pywst.sp.pyomo_GeneralSP_sc',
('cbc', 'pyomo', 'min-sensors'):
'pywst.sp.pyomo_GeneralSP_min_sensors',

('ef', 'pyomo', 'multi-stage'):
'pywst.sp.pyomo_multi_stage',
('ph', 'pyomo', 'multi-stage'):
'pywst.sp.pyomo_multi_stage',

('pico', 'ampl', 'default'): 
'pywst.sp.pico_ampl_GeneralSP', 
('pico', 'ampl', 'p-median'): 
'pywst.sp.pico_ampl_GeneralSP', 
('pico', 'ampl', 'average-case perfect-sensor'): 
'pywst.sp.pico_ampl_GeneralSP',
('pico', 'ampl', 'worst-case perfect-sensor'):
'pywst.sp.pico_ampl_GeneralSP',
('pico', 'pyomo', 'robust-cvar perfect-sensor'):
'pywst.sp.pyomo_GeneralSP_robust_cvar_perfect',
('pico', 'ampl', 'side-constrained'):
'pywst.sp.pico_ampl_GeneralSP',

('snl_grasp', 'none', 'default'):
'pywst.sp.grasp_default', 
('snl_grasp', 'none', 'p-median'):
'pywst.sp.grasp_default', 
('snl_grasp', 'none', 'average-case perfect-sensor'):
'pywst.sp.grasp_default', 
('snl_grasp', 'none', 'worst-case perfect-sensor'):
'pywst.sp.grasp_default', 

('att_grasp', 'none', 'default'):
'pywst.sp.grasp_default', 
('att_grasp', 'none', 'p-median'):
'pywst.sp.grasp_default', 
('att_grasp', 'none', 'average-case perfect-sensor'):
'pywst.sp.grasp_default',
('att_grasp', 'none', 'worst-case perfect-sensor'):
'pywst.sp.grasp_default',

('lagrangian', 'none', 'default'):
'pywst.sp.lagrangian_default', 
('lagrangian', 'none', 'p-median'):
'pywst.sp.lagrangian_default', 
('lagrangian', 'none', 'average-case perfect-sensor'):
'pywst.sp.lagrangian_default'

}

def help_problems():
    print "==============================================="
    print "Sensor Placement Problem Types Supported by WST"
    print "==============================================="
    print ""
    for key in descr:
        if type(key) is tuple:
            print ", ".join(key)
        else:
            print key
        print "----------------------------------------------",
        print descr[key]

def help_solvers_old():
    col = [6,17,12]
    for solver, lang, prob in problems:
        if len(solver) > col[0]:
            col[0] = len(solver)
        if len(lang) > col[1]:
            col[1] = len(lang)
        if len(prob) > col[2]:
            col[2] = len(prob)
    format = "%-"+str(col[0]+2)+"s %-"+str(col[1]+2)+"s %-"+str(col[2])+"s"
    print format
    print format % ("Solver","Modeling Language", "Problem Type")
    print "=" * 60
    prev = None
    for solver, lang, prob in problems:
        if not prev is None and prev != solver:
            print "-" * 60
        prev = solver
        print format % (solver, lang, prob)
    print "-" * 60


def help_solvers():
    ampl_available = not pyutilib.services.registered_executable('ampl') is None
    glpsol_available = not pyutilib.services.registered_executable('glpsol') is None
    available = get_solver_availability()
    #
    col = [6,17,12]
    tmp = []
    for solver, lang, prob in problems:
        if len(solver) > col[0]:
            col[0] = len(solver)
        if len(lang) > col[1]:
            col[1] = len(lang)
        if len(prob) > col[2]:
            col[2] = len(prob)
        tmp.append((prob, solver, lang))
    format = "%-"+str(col[2]+2)+"s %-"+str(col[0]+4)+"s %-"+str(col[1]+2)+"s"
    print "=" * 70
    print "Table of Registered WST Solvers"
    print "=" * 70
    print "    Asterisks (*) indicate installed solvers"
    print""
    print format % ("Problem Type", "Solver", "Modeling Language")
    print "=" * 70
    prev = None
    for prob, solver, lang in sorted(tmp):
        if not prev is None and prev != prob:
            print "-" * 70
        prev = prob
        if not available.get(solver,True):
            installed=' '
        elif lang == 'ampl' and not (ampl_available or glpsol_available):
            installed=' '
        else:
            installed='*'
        print format % (prob, installed+solver, lang)
    print "-" * 70



def run(prob, addDate=False):
    """
    This is a master script that guide the execution of sensor placement
    optimization.
    """
    pyutilib.services.TempfileManager.push()
    
    logger.info("WST sp subcommand")
    logger.info("---------------------------")
        
    # set start time
    startTime = time.time()
        
    # Call the validate() routine to initialize the problem data
    logger.info("Validating configuration file")
    prob.validate() 

    #
    # Setup the temporary data directory
    #
    if not prob.getConfigureOption('temp directory') in prob.none_list:
        pyutilib.services.TempfileManager.tempdir = os.path.abspath(prob.getConfigureOption('temp directory'))
    else:
        pyutilib.services.TempfileManager.tempdir = os.path.abspath(os.getcwd())
    #
    # Setup object to store global data
    #
    data = Options()
    data.prefix = pyutilib.services.TempfileManager.create_tempfile(prefix='wst_sp_')
    #
    # Collect problem statistics
    #
    ##nobj = len(prob.getProblemOption('objective'))
    ##ncon = len(prob.getProblemOption('constraint'))
    ##imperfect = prob.getProblemOption('imperfect') not in Problem.none_list
    ##aggregate = prob.getProblemOption('aggregate') not in Problem.none_list
    ##stages = 1      # TODO: figure out stages later
    ##bound = False   # TODO: figure out bounding formulation later
    #
    # Call the preprocess() routine
    logger.info("Preprocessing data")
    prob.preprocess(data.prefix)
    
    # The problem type, modeling language and solver type are used to select the
    # manner in which optimization is performed.
    #
    data.ptype = lower( prob.getProblemOption('type') )
    data.lang = lower( prob.getProblemOption('modeling language') )
    data.solvertype = lower(prob.getSolverOption('type', 0))
    #
    if (data.solvertype, data.lang, data.ptype) in problems:
        logger.info('Optimizing sensor locations with WST...')
        logger.debug('    Solver:            %s' % data.solvertype)
        logger.debug('    Modeling Language: %s' % str(data.lang))
        logger.debug('    Problem Type:      %s' % data.ptype)
        functor = FunctorAPIFactory(problems[data.solvertype, data.lang, data.ptype])
        if not functor is None:
            data = functor(data, prob=prob).data
    elif (data.solvertype, 'pyomo', data.ptype) in problems:
        data.lang = 'pyomo'
        logger.info('Optimizing sensor locations with WST...')
        logger.debug('    Solver:            %s' % data.solvertype)
        logger.debug('    Modeling Language: %s' % str(data.lang))
        logger.debug('    Problem Type:      %s' % data.ptype)
        #
        # The default modeling language is 'none'.  If that doesn't exist, then
        # try out 'pyomo'.
        #
        functor = FunctorAPIFactory(problems[data.solvertype, data.lang, data.ptype])
        if not functor is None:
            data = functor(data, prob=prob).data
    else:
        #
        # Anything else, we use the old SP command
        #
        logger.info('Optimizing sensor locations with an external script...')
        logger.debug('    Solver:            %s' % data.solvertype)
        logger.debug('    Modeling Language: %s' % str(data.lang))
        logger.debug('    Problem Type:      %s' % data.ptype)
        prob.run_sp(addDate=addDate)
        pyutilib.services.TempfileManager.pop( not prob.getConfigureOption('keepfiles') )
        return
    #
    # TODO: should we print the optimization log here, or print it as it's generated?
    #
    #if problem.printlog:
    #  print self.run_log
    #
    #
    # TODO: other postprocess steps
    #
    logger.debug('Optimization Results')
    #
    output_prefix = ""
    evalsensor_output = ""
    if prob.opts['configure']['output prefix'] not in prob.none_list:
        output_prefix = prob.opts['configure']['output prefix'] + "_"
    results_fname = wst_util.get_tempfile(None, 'sp.json')
    #results_fname = os.path.join(os.path.abspath(os.curdir), output_prefix + 'sp.json')
    #
    if data.solutions is None:
        logger.debug('No results recorded!')
    else:
        if type(data.objective) is dict:
            pass # TODO for multi-objective
        elif not data.objective is None:
            logger.debug('Objective:   '+ str(data.objective))
        if not data.lower_bound is None:
            logger.debug('Lower Bound: '+ str(data.lower_bound))
        if not data.upper_bound is None:
            logger.debug('Upper Bound: '+ str(data.upper_bound))
        #
        # Translate the solutions if a junction map was provided
        #
        data.locations = prob.translate_solutions(data.solutions)
        for i in range(len(data.solutions)):
            data.solutions[i] = sorted(data.solutions[i])
        for i in range(len(data.locations)):
            data.locations[i] = sorted(data.locations[i])
        logger.debug('Solutions: '+ str(data.solutions))
        logger.debug('Locations: '+ str(data.locations))
        #
        # Summarize results
        #
        if not prob.getProblemOption('compute bound'):
            #
            # Run evalsensors
            #
            evalsensor_output = evalsensors(prob, data)
            logger.debug('Evalsensor output:')
            logger.debug(evalsensor_output)
        #
        # Translate stage2 data
        #
        if not data.stage2_solutions is None:
            data.stage2_locations = {}
            print '---------------'
            print "Stage 2 Results"
            print '---------------'
            for key in data.stage2_solutions:
                data.stage2_locations[key] = prob.translate_solution(data.stage2_solutions[key])
                print "Scenario ",key 
                print "Node IDs ",data.stage2_solutions[key] 
                print "Junctions",data.stage2_locations[key] 
                print ""
            
    data.CPU_time = time.time() - startTime
    #
    # Store final results in a JSON file.
    #
    Solution = {}
    Solution['run date'] = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    Solution['CPU time'] = time.time() - startTime
    Solution['objective'] = data.objective
    Solution['lower bound'] = data.lower_bound
    Solution['upper bound'] = data.upper_bound
    Solution['node ID'] = data.solutions
    Solution['EPANET node ID'] = data.locations
    Solution['modeling language'] = data.lang
    Solution['solver type'] = data.solvertype
    Solution['problem type'] = data.ptype
    f = open(results_fname,'w')
    json.dump(Solution,f,indent=2)
    f.close()
    #OUTPUT = open(results_fname, 'w')    
    #print >>OUTPUT, json.dumps(data, indent=2, sort_keys=True)
    #OUTPUT.close()
    pyutilib.services.TempfileManager.pop( not prob.getConfigureOption('keepfiles') )
    
    # remove temporary files if debug = 0
    if prob.opts['configure']['debug'] == 0:
        pyutilib.services.TempfileManager.clear_tempfiles()
            
    # write output file 
    prefix = os.path.basename(prob.opts['configure']['output prefix'])      
    logfilename = os.path.join(os.path.dirname(prob.opts['configure']['output prefix']), os.path.basename(logger.parent.handlers[0].baseFilename))
    outfilename = logfilename.replace('.log','.yml')
    visymlfilename = logfilename.replace('.log','_vis.yml')
    
    fid = open(prob.opts['configure']['output prefix']+'_evalsensor.out', 'w')
    fid.write(evalsensor_output)
    fid.close()
    
    # Write output yml file
    config = wst_config.output_config()
    module_blocks = ("general", "sensor placement")
    template_options = {
        'general':{
            'cpu time': round(time.time() - startTime,3),
            'directory': os.path.dirname(logfilename),
            'log file': os.path.basename(logfilename)},
        'sensor placement': {
            'nodes': data.locations,
            'objective': data.objective,
            'lower bound': data.lower_bound,
            'upper bound': data.upper_bound,
            'greedy ranking': prefix+'_evalsensor.out',
            'stage 2': []} }
    if outfilename != None:
        prob.saveOutput(outfilename, config, module_blocks, template_options)
    
    # Write output visualization yml file
    shape = ["square", "circle", "triangle", "diamond"]
    color = ["navy", "maroon", "green", "yellow", "aqua", "lime", "magenta", "red", "blue", "black"]
    
    config = wst_config.master_config()
    module_blocks = ("network", "visualization", "configure")
    template_options = {
        'network':{
            'epanet file': "<REQUIRED INPUT>"},
        'visualization': {
            'layers': []},
        'configure': {
            'output prefix': os.path.abspath(prob.opts['configure']['output prefix'])}}
    i = 0
    for solution in data.locations: 
        if len(data.locations) == 1:
            label = "Sensor placement"
        else:
            label = "Sensor placement " + str(i + 1)
        template_options['visualization']['layers'].append({
                    'label': label,
                    'locations': "['sensor placement']['nodes'][" + str(i) + "][i]",
                    'file': os.path.abspath(outfilename),
                    'location type': 'node',  
                    'shape': shape[i % len(shape)], # Modulus        
                    'fill': {
                        'color': color[i % len(color)], # Modulus
                        'size': 15,
                        'opacity': 0},
                    'line': {
                        'color': color[i % len(color)], # Modulus
                        'size': 2,
                        'opacity': 0.6}})
        i += 1
    if visymlfilename != None:
        prob.saveVisOutput(visymlfilename, config, module_blocks, template_options)        
        
    # Run visualization
    cmd = ['wst', 'visualization', visymlfilename]
    p = pyutilib.subprocess.run(cmd) # logging information should not be printed to the screen
            
    # print solution to screen
    logger.info("\nWST normal termination")
    logger.info("---------------------------")
    dir_ = os.path.dirname(logfilename)
    if dir_ == "":
        dir_ = '.'
    logger.info("Directory: " + dir_)
    logger.info("Results file: "+os.path.basename(outfilename))
    logger.info("Log file: "+os.path.basename(logfilename))
    logger.info("Visualization configuration file: "+os.path.basename(visymlfilename))
    logger.info('WARNING: EPANET input file required to create HTML network graphic\n')
