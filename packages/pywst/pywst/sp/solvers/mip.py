import math
import os, sys
import os.path
import shutil
from string import Template

import pyutilib.subprocess
import pyutilib.services
from pyutilib.workflow import functor_api

from pywst.sp.models.GeneralSP import model
from pywst.sp.util import get_solvername

from pyomo.environ import *

spdir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))+os.sep
modeldir = spdir+'models'+os.sep


def createIPData(prob, data):
    """
    A function that creates the *.dat data file used with AMPL and Pyomo models.
    """
    #
    # TODO: how handle constraint-specific gamma values?
    #
    if pyutilib.services.registered_executable('createIPData') is None:
        raise RuntimeError, "Must have 'createIPData' executable to optimize an AMPL model"
    config_file = prob.create_wst_configfile(data.prefix)
    pyutilib.services.TempfileManager.add_tempfile(config_file, False)
    data_file = data.prefix+'.dat'
    pyutilib.services.TempfileManager.add_tempfile(data_file, False)
    cmd = pyutilib.services.registered_executable('createIPData').get_path() + " --output=" + data_file + " --gamma=0.05" + " " + config_file
    if prob.getConfigureOption('debug'):
        print "createIPData", cmd
    (rc, output) = pyutilib.subprocess.run(cmd)
    if rc != 0:
        raise RuntimeError, "Problem launching 'createIPData'.\nCurrent directory: %s\nExit Code: %s\nOutput: %s" % (os.getcwd(), str(rc), output)
    return data_file



@functor_api(namespace='pywst.sp')
def pico_ampl_GeneralSP(data, prob=None):
    """
    Perform optimization with a MIP modeled with AMPL.

    Required:
        prob:       Problem instance
    """
    ampl_GeneralSP(data, prob)

@functor_api(namespace='pywst.sp')
def pyomo_GeneralSP_default(data, prob=None):
    """
    Optimize default problem modeled with Pyomo with MIP solver.

    Required:
        prob:       Problem instance
    """
    pyomo_GeneralSP(data, prob)

@functor_api(namespace='pywst.sp')
def pyomo_GeneralSP_pmedian(data, prob=None):
    """
    Optimize p-median problem modeled with Pyomo with MIP solver.

    Required:
        prob:       Problem instance
    """
    pyomo_GeneralSP(data, prob)

@functor_api(namespace='pywst.sp')
def pyomo_GeneralSP_avgcase_perfect(data, prob=None):
    """
    Optimize average-case-perfect problem modeled with Pyomo with MIP solver.

    Required:
        prob:       Problem instance
    """
    pyomo_GeneralSP(data, prob)

@functor_api(namespace='pywst.sp')
def pyomo_GeneralSP_pcenter(data, prob=None):
    """
    Optimize p-center problem modeled with Pyomo with MIP solver.

    Required:
        prob:       Problem instance
    """
    pyomo_GeneralSP(data, prob)

@functor_api(namespace='pywst.sp')
def pyomo_GeneralSP_worstcase_perfect(data, prob=None):
    """
    Optimize worst-case-perfect problem modeled with Pyomo with MIP solver.

    Required:
        prob:       Problem instance
    """
    pyomo_GeneralSP(data, prob)

@functor_api(namespace='pywst.sp')
def pyomo_GeneralSP_robust_cvar_perfect(data, prob=None):
    """
    Optimize robust-cvar-perfect problem modeled with Pyomo with MIP solver.

    Required:
        prob:       Problem instance
    """
    pyomo_GeneralSP(data, prob)

@functor_api(namespace='pywst.sp')
def pyomo_GeneralSP_sc(data, prob=None):
    """
    Optimize side-constrained problem modeled with Pyomo with MIP solver.

    Required:
        prob:       Problem instance
    """
    pyomo_GeneralSP(data, prob)

@functor_api(namespace='pywst.sp')
def pyomo_GeneralSP_min_sensors(data, prob=None):
    """
    Optimize min-sensors problem modeled with Pyomo with MIP solver.

    Required:
        prob:       Problem instance
    """
    pyomo_GeneralSP(data, prob)


def pyomo_GeneralSP(data, prob):
    """
    Perform optimization with a MIP modeled with Pyomo.
    """
    global model
    #
    relax_integrality = prob.getProblemOption('compute bound')
    #
    # Create the AMPL *.dat data file
    #
    data_file = createIPData(prob, data)
    #
    solvername = get_solvername(prob)
    #
    # Create model instance
    #
    if relax_integrality:
        TransformationFactory('core.relax_integrality').apply_to(model)
    instance = model.create_instance(data_file)
    #
    # Perform optimization
    #
    opt = SolverFactory(solvername)
    if prob.getConfigureOption('debug') or prob.getSolverOption('verbose'):
        sol = opt.solve(instance,
                        tee=True,
                        load_solutions=False,
                        logfile=prob.getSolverOption('logfile'))
    else:
        sol = opt.solve(instance,
                        load_solutions=False,
                        logfile=prob.getSolverOption('logfile'))
    #
    # Collect the objective value (for the first solution)
    #
    ##print sol
    #
    # TODO: should we evaluate the objective instead?  This doesn't give us the same answer.
    # This is due to (a) the way that we formulated or objective and (b) solver-specific peculiarities.
    #
    # Note: loading the solution avoids inconsistencies in Pyomo.  The following command
    #
    #  data.objective = sol.solution[0].objective['obj'].value
    #
    # does not work for all solvers (e.g. PICO).
    #
    #
    # Collect a list of sensors
    #
    solutions = []
    if not relax_integrality:
        for i in range(len(sol.solution)):
            instance.solutions.load_from(sol, id=i)
            s = []
            for j in instance.s:
                #
                # NOTE: this avoids an apparent bug in Pyomo.  If value != 1, then it happens
                # to be None, instead of zero.
                #
                if not instance.s[j].value is None and math.fabs(instance.s[j].value - 1.0) < 1e-7:
                    s.append(j)
            solutions.append(s)
    else:
        instance.solutions.load_from(sol)
    
    data.objective = value(instance.obj)
    data.solutions = solutions
    #
    data.command_line = 'Optimization performed with the Pyomo solver interface to %s' % solvername
    if prob.getProblemOption('aggregate') in prob.none_list:
        if relax_integrality:
            data.lower_bound = sol.problem[0].lower_bound
            data.upper_bound = None
            data.objective = None
        else:
            data.lower_bound = sol.problem[0].lower_bound
            data.upper_bound = sol.problem[0].upper_bound
        if relax_integrality and (data.lower_bound == float('inf') or data.lower_bound is None):
            data.lower_bound = data.objective
    else:
        #
        # TODO: can we adapt the lower bound values based on aggregation?
        #
        # No bound values are printed, since these may not be accurate
        pass


ampl_script_template=Template("""
##
## AMPL script that solves the GeneralSP.mod model.
##
#
# Read in the model and data files
#
model ${model_file} ;
data ${data_file} ;
#
# Tells ampl to show model statistics
#
option showstats 1;
#
# Manage whether AMPL preprocesses the model.
#
option presolve ${presolve};
#
# Other options
#
${other_options};
#
# Solver
#
option solver ${solvername};
option ${solveroptionname} '${solveroptions}';
#
# Solve
#
solve;
#
# Post-solve
#
${postprocess}
#
# Print end-of-job tag
#
print "# SUCCESSFUL_TERMINATION";
""")


def ampl_GeneralSP(data, prob):
    """
    Perform optimization with a MIP modeled with AMPL.
    """
    #
    # Create the AMPL *.dat data file
    #
    data_file = createIPData(prob, data)
    #
    # Error checking for modeling tools and solvers
    #
    using_ampl = not pyutilib.services.registered_executable('ampl') is None
    using_glpsol = not pyutilib.services.registered_executable('glpsol') is None
    if not using_ampl and not using_glpsol:
        raise RuntimeError, "Must have 'ampl' or 'glpsol' available to optimize an AMPL model."
    solvername = get_solvername(prob)
    if using_ampl and not solvername in ['pico', 'cplex']:
            raise RuntimeError, "Bad solver '%s'.  When using 'ampl', the solver must be 'pico' or 'cplex'." % solvername
    if using_glpsol and not solvername in ['glpk', 'pico']:
            raise RuntimeError, "Bad solver '%s'.  When using 'glpsol', the solver must be 'glpk'." % solvername
    #
    # Create a temporary model file
    #
    model_file = data.prefix+'.mod'
    pyutilib.services.TempfileManager.add_tempfile(model_file, exists=False)
    shutil.copyfile(modeldir+"GeneralSP.mod", model_file)
    #
    # Create string that generates output
    #
    tlist = [""]
    tlist.append("printf \"\\nObjective Information: %s\\n\", objectiveGoal & \" \" & objectiveMeasure;")
    tlist.append("printf(\"\\nSuperLocation Information:\\n\");")
    tlist.append("for {g in ActiveGoals} printf \"%s\\n\",  g & \" \" & slThreshold[g] & \" \" & slAggregationRatio[g];")
    tlist.append("printf(\";\\n\\n\");")
    if using_ampl:
        tlist.append("display s;")
        tlist.append("printf \"\\tObjective= %f\\n\", Objective;")
    postprocess_steps = '\n'.join(tlist)
    #
    # Collect solver options
    #
    if using_ampl:
        postprocess = postprocess_steps
    else: 
        postprocess = ''
    if prob.getProblemOption('presolve') is True:
        presolve=1
    else:
        presolve=0
    other_options = prob.getProblemOption('other')
    if other_options in prob.none_list:
        other_options = ''
    if solvername == 'pico':
        solvername = 'PICO'
        solveroptionname = 'pico'
        solveroptions = " --RRTrialsPerCall=8 --RRDepthThreshold=-1 --feasibilityPump=false --usingCuts=true "
        # TODO: add --absTolerance=0.99999 when minimizing # sensors
        if not prob.getSolverOption('timelimit',0) in prob.none_list:
            minutes = prob.getSolverOption('timelimit',0)/60.0
            solveroptions += "--maxWallMinutes= %f" % minutes
        solver_path = pyutilib.services.registered_executable('PICO').get_path()
    elif solvername == 'cplex':
        solvername = 'cplexamp'
        solveroptionname = 'cplex'
        solveroptions = ''
    elif solvername == 'glpk':
        solveroptions = ''
    #
    # Perform optimization
    #
    logfile = data.prefix+'.log'
    pyutilib.services.TempfileManager.add_tempfile(logfile, exists=False)
    #print "HERE", using_ampl, logfile
    if using_ampl:
        #
        # Create AMPL script
        #
        cmd = ampl_script_template.substitute(model_file=model_file, data_file=data_file, presolve=presolve, other_options=other_options, solvername=solvername, solveroptionname=solveroptionname, solveroptions=solveroptions, postprocess=postprocess)
        #cmd = ampl_script_template.substitute(model_file=model_file, data_file=data_file, presolve=presolve, file_type=file_type, other_options=other_options, solvername=solvername, solveroptionname=solveroptionname, solveroptions=solveroptions, postprocess=postprocess)
        #
        ampl_script = data.prefix+'_ampl.in'
        #pyutilib.services.TempfileManager.add_tempfile(ampl_script, exists=False)
        OUTPUT=open(ampl_script,"w")
        print >>OUTPUT, cmd
        OUTPUT.close()
        #
        # Perform optimization
        #
        cmd = pyutilib.services.registered_executable('ampl').get_path()+' '+ampl_script
        (rc, output) = pyutilib.subprocess.run( cmd, outfile=logfile)
        data.rc = rc
        data.command_line = cmd
    else:
        #
        # Append postprocessing steps to the model
        #
        OUTPUT = open(model_file,"a")
        print >>OUTPUT, postprocess_steps
        print >>OUTPUT, 'end;'
        OUTPUT.close()
        #
        # Generate MPS file from AMPL model
        #
        # TODO: use Pyomo model conversion utility?
        #
        mps_file = data.prefix+'.mps'
        pyutilib.services.TempfileManager.add_tempfile(mps_file, exists=False)
        cmd = "%s --check -m %s -d  %s --wfreemps %s" % (pyutilib.services.registered_executable('glpsol').get_path(), model_file, data_file, mps_file)
        if prob.getConfigureOption('debug'):
            print "Running glpsol..."
            print cmd
        (rc, output) = pyutilib.subprocess.run(cmd)
        if rc != 0:
            raise RuntimeError, "Problem launching 'glpsol' to generate an MPS file.\nCurrent directory: %s\nExit Code: %d\nOutput: %s" % (os.getcwd(), rc, output)
        #
        # Optimize with PICO
        # 
        cmd = '%s --debug=1 --lpType=clp %s %s' % (solver_path, solveroptions, mps_file)
        if prob.getConfigureOption('debug'):
            print "Running PICO ..."
            print cmd
        (rc, output) = pyutilib.subprocess.run(cmd)
        data.command_line = cmd
        data.rc = rc
        if rc != 0:
            raise RuntimeError, "Problem launching 'PICO'.\nCurrent directory: %s\nExit Code: %d\nOutput: %s" % (os.getcwd(), rc, output)
        #
        # Process the PICO solution file to get the sensor placements
        #
        sensors = []
        INPUT = open(data.prefix + ".sol.txt","r")
        for line in INPUT.xreadlines():
            tokens = line.split(" ")
            #
            # Process a line to see if its a sensor variable
            #
            item = tokens[0].split("[")
            if (item[0] == "s"):
                val = eval(item[1].split("]")[0])
                sensors = sensors + [ val ]
        INPUT.close()
        data.solutions = []
        data.solutions.append(sensors)


