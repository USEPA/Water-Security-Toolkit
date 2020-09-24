
import json
import os, sys
import os.path
import shutil
from string import lower

import pyutilib.subprocess
from pyutilib.workflow import functor_api
from pywst.sp.util import get_solvername
from pyomo.environ import SolverFactory

spdir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))+os.sep
modeldir = spdir+'models'+os.sep


@functor_api(namespace='pywst.sp')
def pyomo_multi_stage(data, prob=None):
    """
    Perform multi-stage optimization with PySP.

    Required:
        prob:       Problem instance
    """
    #
    # Error Checking
    #
    solvername = get_solvername(prob)
    prob_name = prob.getProblemOption('name',0)
    con_name = prob.getProblemOption('constraint', prob_name)
    if len(con_name) > 1:
        raise RuntimeError, "We don't know how to handle side-constraints within PySP"
    else:
        con_name = con_name[0]
    aggr_name = prob.getProblemOption('aggregate', prob_name)
    goal_name = prob.getConstraintOption('goal', con_name)
    if lower(goal_name) != 'ns':
        cost_name = prob.getCostOption('cost file', goal_name)
        if not cost_name is None:
            raise RuntimeError, "Cannot use a cost constraint goal with the %s solver" % solvername
    if aggr_name is not None:
        raise RuntimeError, "Cannot use a aggregation with the %s solver" % solvername
    #
    # Create the Pyomo data files
    #
    create_multistage_data(prob, data)
    #
    # Setup the PySP command-line
    #
    solvername = get_solvername(prob)
    #
    seed = prob.getSolverOption('seed')
    if solvername == 'ef':
        cmd = ['runef']
        cmd.append('--output-solver-log')
    elif solvername == 'ph':
        cmd = ['runph']
        cmd.append('-r')
        cmd.append('1.0')
    cmd.append('--solver')
    #
    # TODO: add an 'ef' solver option that specifies a sub-solver name
    #
    opt = SolverFactory('pico')
    if opt.available(False) and (not opt.executable() is None):
        cmd.append('pico')
    else:
        opt = SolverFactory('cbc')
        if opt.available(False) and (not opt.executable() is None):
            cmd.append('cbc')
        else:
            opt = SolverFactory('glpk')
            if opt.available(False) and (not opt.executable() is None):
                cmd.append('glpk')
    #
    cmd.append('-m')
    cmd.append('%s/multistage' % modeldir)
    cmd.append('-i')
    cmd.append('%s' % data.datadir)
    if not seed is None:
        cmd.append('--scenario-tree-seed')
        cmd.append(str(seed))
    cmd.append('--solution-writer=pywst.sp.pyspsolutionwriter')
    cmd.append('--traceback')
    if solvername == 'ef':
        cmd.append('--solve')
    else:
        cmd.append('--linearize-nonbinary-penalty-terms=5')
    data.command_line = ' '.join(cmd)
    #
    # Run the solver
    #
    print "Running command ..."
    print ' '.join(cmd)
    rc, output = pyutilib.subprocess.run(cmd, tee=True)
    if rc != 0:
        print "ERROR executing the '%s' command" % solvername
        print output
        raise RuntimeError, "Bad return code for multi-stage solver: %d\n%s" % (rc, output)
    #
    # Read the JSON file
    #
    INPUT = open(solvername+'.json','r')
    results = json.load(INPUT)
    INPUT.close()
    stage2 = {}
    soln = []
    for i in results:
        for j in results[i]:
            for k in results[i][j]:
                for l in results[i][j][k]:
                    #print i,j,k,l
                    val = results[i][j][k][l]
                    if i == 'FirstStage' and j == 'RootNode' and k == 'Stage1Sensors' and val == 1.0:
                        soln.append( eval(l) )
                    elif i == 'SecondStage':
                        if not j in stage2:
                            stage2[ j ] = []
                        if k == 'Stage2Sensors' and val == 1.0:
                            loc = eval(l)
                            if not loc in soln:
                                stage2[ j ].append( loc )
    data.solutions = [soln]
    data.stage2_solutions = stage2
    #
    # Remove the data directory
    #
    if not prob.getConfigureOption('keepfiles'):
        shutil.rmtree(data.datadir)


def create_multistage_data(prob, data):
    #
    # The data directory
    #
    data.datadir = data.prefix+'_data'+os.sep
    os.mkdir(data.datadir)
    ##
    ## Create the root data file
    ##
    root_file = data.datadir + 'RootNode.dat'
    OUTPUT = open(root_file, 'w')
    #
    # Get the impact filename for the objective
    #
    prob_name = prob.getProblemOption('name',0)
    obj_name = prob.getProblemOption('objective', prob_name)
    impact_name = prob.getObjectiveOption('goal', obj_name)
    #dir = prob.getImpactOption('directory', impact_name)
    #if dir in prob.none_list:
    #    dir = ""
    fname = prob.getImpactOption('impact file', impact_name)
    #
    # Get the constraint on the number of sensors
    #
    con = prob.getProblemOption('constraint', prob_name)
    if lower(prob.getConstraintOption('goal', con[0])) == 'ns':
        print >>OUTPUT, "param NumStage1Sensors := %s ;" % prob.getConstraintOption('bound', con[0]) 
    #
    # Read the impact file and write the data file
    #
    INPUT = open(fname, 'r')
    line = INPUT.readline()
    line = line.strip()
    num_nodes = int(line.split(" ")[0])
    #print >>OUTPUT, "param N := %d ;" % num_nodes
    INPUT.readline()
    print >>OUTPUT, "param : EventsXJunctions : Impact :="
    for line in INPUT:
        line = line.strip()
        tokens = line.split(' ')
        print >>OUTPUT, tokens[0], tokens[1], tokens[3]
    print >>OUTPUT, ";"
    INPUT.close()
    OUTPUT.close()
    ##
    ## Create the ReferenceModel.dat file
    ##
    reference_file = data.datadir + 'ReferenceModel.dat'
    OUTPUT = open(reference_file, 'w')
    INPUT = open(root_file, 'r')
    for line in INPUT:
        print >>OUTPUT, line,
    INPUT.close()
    # TODO: Does this need to be defined differently?
    print >>OUTPUT, "param NumStage2Sensors := 0 ;"
    OUTPUT.close()
    ##
    ## Create the scenario data files
    ##
    ## TODO: double-check that conditional probabilities sum to one.
    ##
    scenarios = prob.getConstraintOption('scenario', prob.getProblemOption('constraint')[0])
    for i in range(len(scenarios)):
        if scenarios[i].get('name',None) is None:
            scenarios[i]['name'] = 'Node%d' % i
    for i in range(len(scenarios)):
        OUTPUT = open(data.datadir+'%s.dat' % scenarios[i].get('name',None), 'w')
        print >>OUTPUT, "param NumStage2Sensors := %d ;" % scenarios[i]['bound']
        OUTPUT.close()
    ##
    ## Create the scenario data file
    ##
    root_file = data.datadir + 'ScenarioStructure.dat'
    OUTPUT = open(root_file, 'w')
    print >>OUTPUT, "set Stages := FirstStage SecondStage ;\n"
    #
    print >>OUTPUT, "set Nodes :="
    print >>OUTPUT, "RootNode"
    for i in range(len(scenarios)):
        print >>OUTPUT, scenarios[i]['name']
    print >>OUTPUT, ";\n"
    #
    print >>OUTPUT, "param NodeStage :="
    print >>OUTPUT, "RootNode FirstStage"
    for i in range(len(scenarios)):
        print >>OUTPUT, "%s SecondStage" % scenarios[i]['name']
    print >>OUTPUT, ";\n"
    #
    print >>OUTPUT, "set Children[RootNode] :="
    for i in range(len(scenarios)):
        print >>OUTPUT, scenarios[i]['name']
    print >>OUTPUT, ";\n"
    #
    print >>OUTPUT, "param ConditionalProbability :="
    print >>OUTPUT, "RootNode 1.0"
    for i in range(len(scenarios)):
        print >>OUTPUT, "%s %f" % (scenarios[i]['name'], scenarios[i]['probability'])
    print >>OUTPUT, ";\n"
    #
    print >>OUTPUT, "set Scenarios :="
    for i in range(len(scenarios)):
        print >>OUTPUT, "%sScenario" % scenarios[i]['name']
    print >>OUTPUT, ";\n"
    #
    print >>OUTPUT, "param ScenarioLeafNode :="
    for i in range(len(scenarios)):
        print >>OUTPUT, "%sScenario %s" % (scenarios[i]['name'], scenarios[i]['name'])
    print >>OUTPUT, ";\n"
    #
    print >>OUTPUT, "set StageVariables[FirstStage] :="
    print >>OUTPUT, "x1[*,*]"
    print >>OUTPUT, "Stage1Sensors[*]"
    print >>OUTPUT, ";\n"
    #
    print >>OUTPUT, "set StageVariables[SecondStage] :="
    print >>OUTPUT, "x2[*,*]"
    print >>OUTPUT, "Stage2Sensors[*]"
    print >>OUTPUT, ";\n"
    #
    print >>OUTPUT, "param StageCostVariable :="
    print >>OUTPUT, "FirstStage FirstStageCost"
    print >>OUTPUT, "SecondStage SecondStageCost"
    print >>OUTPUT, ";\n"
    #
    print >>OUTPUT, "param ScenarioBasedData := False"
    print >>OUTPUT, ";\n"
    OUTPUT.close()

