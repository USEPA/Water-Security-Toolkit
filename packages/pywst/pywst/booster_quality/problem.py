#
#
import os, sys, datetime
import pyutilib.subprocess
import yaml
import json
import itertools
import pprint
import subprocess
import copy

try:
    import pyepanet
except ImportError:
    pass

def _assert_(value):
    if not value:
        raise ValueError('booster_quality: option with invalid value was found in configuration file')

class Problem():
    filename = 'booster_mip.yml'

    opts = {\
        'network': {
            'epanet file': 'Net3.inp',
            'water quality timestep': 'INP',
            'simulation duration': 'INP',
            'ignore merlion warnings':False}, 
        'booster': {
            'model format': 'AMPL',
            'objective': 'MASS', 
            'decay coefficient': 'INP',
            'min quality': 0.2,
            'max quality': 'AUTO',
            'setpoint quality': 1.0,
            'source quality': 1.0,
            'source nodes': [],
            'max boosters': 5,
            'evaluate': False,
            'feasible nodes': 'NZD',
            'infeasible nodes': 'NONE',
            'fixed nodes': [],
            'strength': 'FLOWPACED 4',
            'quality start': 1440},
        'solver': {
            'executable': 'cplexamp',
            'options': {},
            'problem writer': 'nl'},
        'configure': {
            'ampl executable': 'ampl64', 
            'pyomo executable': 'pyomo',
            'output prefix': 'wst'},    
        'internal': {
            'nodeNames':[]}}
    
    # for development purposes
    #opts['boosterquality'] = {}
    #opts['boosterquality']['options string'] = ''

    results = { 'dateOfLastRun': '',
                'nodesToBooster': [],
                'finalMetric': -999 }

    # Trying handle all possible ways we may encounter None coming from the yaml parser
    none_list = ['none','','None','NONE', None, [], {}] 
    defLocs = {}
    epanetOkay = False

    def __init__(self):
        self.loadPreferencesFile()
        self.validateEPANET()
        return

    def validateEPANET(self):
        try:
            enData = pyepanet.ENepanet()
        except:
            raise RuntimeError("EPANET DLL is missing or corrupt. Please reinstall PyEPANET.")
        self.epanetOkay = True
        return

    def loadPreferencesFile(self):
        if os.name in ['nt','win','win32','win64','dos']:
            rcPath = os.path.join(os.path.abspath(os.environ['APPDATA']),
                                  '.wstrc')
        else:
            rcPath = os.path.join(os.path.abspath(os.environ['HOME']),
                                  '.wstrc')
        print rcPath
        if os.path.exists(rcPath) and os.path.isfile(rcPath):
            fid = open(rcPath,'r')
            defLocs = yaml.load(fid)
            fid.close()
            self.defLocs = defLocs
            for key in defLocs.keys():
                if key == 'ampl':
                    self.opts['configure']['ampl executable'] = defLocs[key]
                if key == 'pyomo':
                    self.opts['configure']['pyomo executable'] = defLocs[key]
        return

    def load(self, filename):
        if filename is None: return False
        try:
            fid = open(filename,'r')
            options = yaml.load(fid)
            for key in options.keys():
                suboptions = options[key]
                for subkey in suboptions.keys():
                    self.opts[key][subkey] = options[key][subkey]
            fid.close()
            (path, file) = os.path.split(filename)
        except:
            print 'Error reading "%s"'%filename
            return False
        return True

    def save(self, filename=None):
        '''
        Save the project as a YAML formatted file. Uses the filename stored in
        opts['configure']['output prefix'], use setConfigureOption('output prefix',f)
        to set this name.
        '''
        if filename is None:
            filename = self.getConfigureOption('output prefix')+'.yaml'
        try:
            fid = open(filename,'wt')
            fid.write('# YML template: booster_mip\n')
            fid.write('# Units: time [min], mass [g], volume [m^3]\n')
            fid.write('\n')
            # Avoid using yaml.dump so we have control over order and comments in the template
            fid.write('network:\n')
            fid.write('  epanet file: '+str(self.opts['network']['epanet file'])+'            # EPANET network inp file\n')
            fid.write('  water quality timestep: '+str(self.opts['network']['water quality timestep'])+'      # Water quality (and reporting) stepsize. INP or Numerical Value(min)\n')
            fid.write('  simulation duration: '+str(self.opts['network']['simulation duration'])+'         # Simulation duration. INP or Numerical Value(min)\n')
            fid.write('  ignore merlion warnings: '+str(self.opts['network']['ignore merlion warnings'])+'   # True or False\n')
            fid.write('\n')
            fid.write('booster:\n')
            fid.write('  model format: '+str(self.opts['booster']['model format'])+'               # AMPL \n')
            fid.write('  objective: '+str(self.opts['booster']['objective'])+'                  # MASS (injected mass, MILP) or SETPOINT (deviation from source quality, MIQP) \n')
            fid.write('  decay coefficient: '+str(self.opts['booster']['decay coefficient'])+'           # Coefficient for first-order decay of booster agent. INP or Numerical Value(1/min). \n')
            fid.write('  min quality: '+str(self.opts['booster']['min quality'])+'                 # Minimum allowable water quality [g/m3] (Numerical Value). \n')
            fid.write('  max quality: '+str(self.opts['booster']['max quality'])+'                # Maximum allowable water quality [g/m3] (Numerical Value) or AUTO (determines lowest possible value). \n')
            fid.write('  setpoint quality: '+str(self.opts['booster']['setpoint quality'])+'            # Ideal water quality setpoint [g/m3] (Numerical Value). \n')
            fid.write('  source quality: '+str(self.opts['booster']['source quality'])+'              # water quality from source nodes [g/m3] (Numerical Value). \n')
            fid.write('  source nodes: '+str(self.opts['booster']['source nodes'])+'                 # source nodes (List). \n')
            fid.write('  feasible nodes: '+str(self.opts['booster']['feasible nodes'])+'              # ALL, NZD, NONE, list or filename\n')
            fid.write('  infeasible nodes: '+str(self.opts['booster']['infeasible nodes'])+'           # ALL, NZD, NONE, list or filename\n')
            fid.write('  fixed nodes: '+str(self.opts['booster']['fixed nodes'])+'                  # list\n')
            fid.write('  evaluate: '+str(self.opts['booster']['evaluate'])+'                  # True or False \n')
            fid.write('  max boosters: '+str(self.opts['booster']['max boosters'])+'                  # Maximum number of booster stations\n')
            fid.write('  strength: '+str(self.opts['booster']['strength'])+'            # FLOWPACED [g/m3] or MASS [g/min]\n')
            fid.write('  quality start: '+str(self.opts['booster']['quality start'])+'              # start time [minutes] of quality concentration bounds\n')
            fid.write('\n')
            fid.write('solver:\n')
            fid.write('  executable: '+str(self.opts['solver']['executable'])+'             # Path to the optimization solver executable\n')
            fid.write('  options:\n')
            fid.write('    #mipgap: 0.01                  # Example option 1 for optimization solver\n')
            fid.write('    #threads: 8                    # Example option 2 for optimization solver\n')
            fid.write('  problem writer: '+str(self.opts['solver']['problem writer'])+'               # Problem writer used for Pyomo: nl, lp, python, ...\n')
            fid.write('\n')
            fid.write('configure:\n')
            fid.write('  output prefix: '+str(self.opts['configure']['output prefix'])+'               # All created filenames will begin with \'output prefix-\'\n')
            fid.write('  ampl executable: '+str(self.opts['configure']['ampl executable'])+'          # Path to the AMPL executable\n')
            fid.write('  pyomo executable: '+str(self.opts['configure']['pyomo executable'])+'          # Path to the Pyomo executable\n')
            fid.write('\n')
            fid.close()
        except:
            print 'Error saving "%s"'%filename
            return False
        return True
    
    def saveAll(self, filename=None):
        if filename is None:
            filename = self.getConfigureOption('output prefix')+'.yml'
        try:
            fid = open(filename,'wt')
            fid.write('# YML template: sp\n')
            fid.write('\n')
            fid.write(yaml.dump(self.opts,default_flow_style=False))
            fid.close()
        except:
            print 'Error saving "%s"'%filename
            return False
        return True 

    def runBoosterQuality(self):

        out_prefix = ''
        if self.getConfigureOption('output prefix') not in self.none_list:
            out_prefix += self.getConfigureOption('output prefix')+'-'

        cmd = 'boosterquality '

        # The file defining the water quality model
        cmd += '--inp '+self.getNetworkOption('epanet file')+' '
        if(self.getBoosterOption('decay coefficient') not in ['INP','Inp','inp']):
            cmd += '--decay-const '+str(self.getBoosterOption('decay coefficient'))+' '
        
        # Optional arguments are simulation duration and water quality timestep, which will
        # override what is in the EPANET input file.
        if self.getNetworkOption('simulation duration') not in ['INP','Inp','inp']:
            cmd += '--simulation-duration-minutes='+str(self.getNetworkOption('simulation duration'))+' '
        if self.getNetworkOption('water quality timestep') not in ['INP','Inp','inp']:
            cmd += '--quality-timestep-minutes='+str(self.getNetworkOption('water quality timestep'))+' '

        # Dissable merlion warnings
        if self.getNetworkOption('ignore merlion warnings') is True:
            cmd += '--ignore-merlion-warnings '

        # substitude integers for node names in output files
        cmd += '--output-merlion-labels '
        # the above command will produce the following file
        label_map_file = "MERLION_LABEL_MAP.txt"

        # Prepend all output file names with this
        cmd += '--output-prefix='+out_prefix+' '

        cmd += '--epanet-rpt-file='+out_prefix+'booster-epanet.rpt '
 
       #
        # Eventually we will use these options to write the data files in a more 
        # efficient format for pyomo or pysp
        #if self.getBoosterOption('model format') == 'PYOMO':
        #    cmd += '--output-PYOMO '
        #

        # Pass in any user user specified command line options directly to boosterquality
        # This is useful for testing
        try:
            cmd += self.opts['boosterquality']['options string']+' '
        except KeyError:
            pass
        except TypeError:
            pass

        # Create the booster spec file
        booster_spec_file = pyutilib.services.TempfileManager.create_tempfile(suffix = '.boosterquality.boosters.spec') 
        f = open(booster_spec_file,'w')
        #INPUT VALIDATION
        _assert_(self.getBoosterOption('strength') not in self.none_list)
        _assert_(self.opts['internal']['nodeNames'] not in self.none_list)
        print >> f, 'DELAY_MINUTES:', 0
        print >> f, 'INJ_DURATION_MINUTES:', 60
        print >> f, 'SIM_DURATION_MINUTES:', 60
        print >> f, 'INJ_TYPE:', self.getBoosterOption('strength').split()[0]
        print >> f, 'INJ_STRENGTH:', self.getBoosterOption('strength').split()[1]
        print >> f, 'FEASIBLE_NODES:'
        if len(self.getBoosterNodesList()) == 0:
            # The boosterquality code will default to using NZD if the 'FEASIBLE_NODES'
            # list is not specified in this file. However, we will still write this
            # parameter to the file to make things more explicit
            print >> f, 'NZD'
        else: 
            for node_name in self.getBoosterNodesList():
                print >> f, node_name
        f.close()
        cmd += booster_spec_file+' '


        f = open(out_prefix+'boosterquality.run','w')
        f.write(cmd+'\n')
        f.close()
        print "Launching boosterquality executable ..."
        out = out_prefix+'boosterquality.out' 
        sim_timelimit = None
        fout = open(out,'w')
        p = pyutilib.subprocess.run(cmd,timelimit=sim_timelimit,stderr=sys.stderr,stdout=fout)
        fout.close()
        if (p[0]):
            raise RuntimeError("\nAn error occured when running the boosterquality executable\n"
                "Error Message: "+p[1]+"\n"
                "Command: "+cmd+ "\n")

        id_to_name_nodemap = {}
        name_to_id_nodemap = {}
        f = open(out_prefix+label_map_file,'r')
        for line in f:
            t = line.split()
            id_to_name_nodemap[int(t[1])] = t[0]
            name_to_id_nodemap[t[0]] = int(t[1])
        f.close()
        
        return id_to_name_nodemap, name_to_id_nodemap

    def run(self):

        self.validate()

        self.setFeasibleNodes()

        id_to_name_nodemap, name_to_id_nodemap = self.runBoosterQuality()

        results = []

        # After initial data files are created with boosterquality, we can re-solve the optimization problem
        # for a range of different max_stations values. This may save a significant amount of
        # computational time.
        solution_files = []
        failed_solves = []
        for max_stations in self.getBoosterOption('max boosters'):
            try:
                solution_files.append(self.runOptimization(id_to_name_nodemap, 
                                                           name_to_id_nodemap,
                                                           max_stations))
            except RuntimeWarning as warning:
                failed_solves.append((max_stations,warning))

        if failed_solves != []:
            print >> sys.stderr, ' '
            print >> sys.stderr, 'WARNING: One or more failures occurred'          
            for (max_boosters, warning) in failed_solves:
                print >> sys.stderr, '\t"'+str(warning)+'": Max Boosters = '+str(max_boosters)
            print >> sys.stderr, ' '

        return solution_files

    def runOptimization(self, id_to_name_nodemap, name_to_id_nodemap, max_stations):
        print '\nRunning Optimization with:'
        print '\tMAX BOOSTERS = ', max_stations
        
        solve_timelimit = None
        p = (1,"There was a problem with the 'model type' or 'model format' options")
        cmd = None
        Solution = {}
        Solution['detected scenarios'] = {}
        if self.getBoosterOption('model format') == 'AMPL':
            exe = self.getConfigureOption('ampl executable')
            inp = os.path.join(os.path.abspath(os.curdir),self.getConfigureOption('output prefix')+'-ampl-b'+str(max_stations)+'.run')
            out = os.path.join(os.path.abspath(os.curdir),self.getConfigureOption('output prefix')+'-ampl-b'+str(max_stations)+'.out')
            results_file, concentrations_file, mass_injections_file = self.createAmplRun(name_to_id_nodemap, max_stations, inp)
            cmd = '%s %s'%(exe,inp)
            # Delete the possibly existing results file with the same name before-hand,
            # that way we'll know if AMPL failed
            try:
                os.remove(results_file)
            except OSError:
                # the file did not exist
                pass
            try:
                os.remove(concentrations_file)
            except OSError:
                # the file did not exist
                pass
            try:
                os.remove(mass_injections_file)
            except OSError:
                # the file did not exist
                pass
            print 'Launching AMPL ...'
            p = pyutilib.subprocess.run(cmd,timelimit=solve_timelimit,outfile=out)
            if (p[0] or not os.path.isfile(results_file)):
                print >> sys.stderr, '\nAn error occured when running the optimization problem'
                print >> sys.stderr, 'Error Message: ', p[1]
                print >> sys.stderr, 'Command: ', cmd, '\n'
                raise RuntimeWarning('Optimization Failed')
            #try to load the results file
            f = open(results_file,'r')
            ampl_sol = yaml.load(f)
            f.close()
            if (ampl_sol['solve_result_num'] >= 100) or (ampl_sol['solve_result'] != 'solved') or (ampl_sol['solve_exitcode'] != 0):
                print >> sys.stderr, ' '
                print >> sys.stderr, 'WARNING: AMPL solver statuses indicate possible errors.'
                print >> sys.stderr, '         solve_result_num =', ampl_sol['solve_result_num']
                print >> sys.stderr, '         solve_result     =', ampl_sol['solve_result']
                print >> sys.stderr, '         solve_exitcode   =', ampl_sol['solve_exitcode']
                print >> sys.stderr, ' '
            

            min_conc_actual = None
            max_conc_actual = None
            try:
                import json
                f = open(concentrations_file,'r')
                concentrations = json.load(f)
                f.close()
                qs_idx = concentrations['quality start']
                min_conc_actual = min(min(_c for _c in c[qs_idx:] if _c >= concentrations['min quality']) for c in concentrations['nodes'].values())
                max_conc_actual = max(max(c[qs_idx:]) for c in concentrations['nodes'].values())
                import pylab
                from matplotlib.backends.backend_pdf import PdfPages
                pylab.figure()
                avg = concentrations['min quality']*0.5 + concentrations['max quality']*0.5
                pp = PdfPages(os.path.join(os.path.abspath(os.curdir),self.getConfigureOption('output prefix')+'-concentrations-b'+str(max_stations)+'.pdf'))
                for node in concentrations['nodes']:
                    pylab.clf()
                    pylab.title('Node - '+id_to_name_nodemap[int(node)])
                    pylab.plot(concentrations['nodes'][node])
                    pylab.xlabel('Timestep')
                    pylab.ylabel('Concentration g/m3')
                    pylab.ylim([max(0,concentrations['min quality']-avg*0.1),concentrations['max quality']+avg*0.1])
                    pylab.axhline(concentrations['min quality'],color='k',linestyle='--')
                    pylab.axhline(concentrations['max quality'],color='k',linestyle='--')
                    pylab.axvline(concentrations['quality start'],color='k')
                    pylab.savefig(pp,format='pdf')
                pp.close()

                f = open(mass_injections_file,'r')
                mass_injections = json.load(f)
                f.close()
                pylab.figure()
                pp = PdfPages(os.path.join(os.path.abspath(os.curdir),self.getConfigureOption('output prefix')+'-mass-injections-b'+str(max_stations)+'.pdf'))
                for node in mass_injections['nodes']:
                    if max(mass_injections['nodes'][node]) > 0.0:
                        pylab.clf()
                        pylab.title('Node - '+id_to_name_nodemap[int(node)])
                        pylab.plot(mass_injections['nodes'][node])
                        pylab.xlabel('Timestep')
                        pylab.ylabel('Mass Injected (Scaled by flow) g/m3')
                        pylab.ylim([0,1.1*max(mass_injections['nodes'][node][k] for k in range(mass_injections['quality start'],len(mass_injections['nodes'][node])))])
                        pylab.axvline(concentrations['quality start'],color='k')
                        pylab.savefig(pp,format='pdf')
                pp.close()

            except:
                print >> sys.stderr, ' '
                print >> sys.stderr, 'Failed to generate plot pdf'
                print >> sys.stderr, ' '
                raise

            Solution['objective'] = ampl_sol['objective']
            Solution['injected mass grams'] = ampl_sol['injected mass grams']
            Solution['average setpoint deviation'] = ampl_sol['average setpoint deviation']
            Solution['quality period minutes'] = ampl_sol['quality period minutes']
            Solution['min quality bound'] = ampl_sol['min quality']
            Solution['min quality actual'] = min_conc_actual
            Solution['max quality bound'] = ampl_sol['max quality']
            Solution['max quality actual'] = max_conc_actual
            Solution['booster nodes'] = [id_to_name_nodemap[name] for name in ampl_sol['booster ids']]
            Solution['booster nodes'].sort()
        else:
            raise RuntimeError("Bad model format option")

        # Summarize yaml options
        Solution['yaml options'] = copy.deepcopy(self.opts)
        Solution['yaml options']['booster']['max boosters'] = max_stations

        self.printSolutionSummary(Solution)
        
        out_prefix = ""
        if self.getConfigureOption('output prefix') not in self.none_list:
            out_prefix += self.getConfigureOption('output prefix')+'-'         
        results_fname = os.path.join(os.path.abspath(os.curdir),out_prefix+"booster_quality-results-b"+str(max_stations)+'.json')
        f = open(results_fname,'w')
        json.dump(Solution,f,indent=2)
        f.close()
        
        return results_fname

    def printSolutionSummary(self,Solution):
        print 
        print "Solution Summary:"
        print "  (1) Mass injected during quality period (grams): ", Solution['injected mass grams']
        print "  (2) Average squared setpoint deviation during quality period: ", Solution['average setpoint deviation']
        print "  (3) Length of quality period (minutes): ", Solution['quality period minutes']
        print "  (4) Number of booster stations: ", len(Solution['booster nodes'])
        print "  (5) Booster nodes: ", Solution['booster nodes']
        print "  (6) Source nodes : ", self.getBoosterOption('source nodes')
        print "  (7) Min Quality Bound:    ", Solution['min quality bound']
        print "  (8) Min Quality Actual:   ", Solution['min quality actual']
        print "  (9) Max Quality Bound:    ", Solution['max quality bound']
        print " (10) Max Quality Actual:   ", Solution['max quality actual']

        if (len(Solution['booster nodes']) != Solution['yaml options']['booster']['max boosters']):
            print >> sys.stderr, ' '
            print >> sys.stderr, "WARNING: The list of optimal booster nodes returned had"
            print >> sys.stderr, "         fewer locations than what was specified by"
            print >> sys.stderr, "         'max boosters'. Errors may have occurred."
            print >> sys.stderr, ' '
        print

    def validateExecutables(self):
        amplExe = self.getConfigureOption('ampl executable')
        pyomoExe = self.getConfigureOption('pyomo executable')
        solverExe = self.getSolverOption('executable')
        if not os.path.exists(amplExe):
            if 'ampl' in self.defLocs.keys():
                amplExe = self.defLocs['ampl']
            else:
                amplExe = os.path.split(amplExe)[1]
                for p in os.sys.path:
                    f = os.path.join(p,amplExe)
                    if os.path.exists(f) and os.path.isfile(f):
                        amplExe = f
                        break
                    f = os.path.join(p,amplExe+'.exe')
                    if os.path.exists(f) and os.path.isfile(f):
                        amplExe = f
                        break

        if not os.path.exists(pyomoExe):
            if 'pyomo' in self.defLocs.keys():
                pyomoExe = self.defLocs['pyomo']
            else:
                pyomoExe = os.path.split(pyomoExe)[1]
                for p in os.sys.path:
                    f = os.path.join(p,pyomoExe)
                    if os.path.exists(f) and os.path.isfile(f):
                        pyomoExe = f
                        break
                    f = os.path.join(p,pyomoExe+'.exe')
                    if os.path.exists(f) and os.path.isfile(f):
                        pyomoExe = f
                        break
        if not os.path.exists(solverExe):
            solverExe = os.path.split(solverExe)[1]
            for p in os.sys.path:
                f = os.path.join(p,solverExe)
                if os.path.exists(f) and os.path.isfile(f):
                    solverExe = f
                    break
                f = os.path.join(p,solverExe+'.exe')
                if os.path.exists(f) and os.path.isfile(f):
                    solverExe = f
                    break
        self.setConfigureOption('ampl executable',amplExe)
        self.setConfigureOption('pyomo executable',pyomoExe)
        self.setSolverOption('executable',solverExe)        

    def validate(self):
        
        output_prefix = self.getConfigureOption('output prefix')
        self.validateExecutables()

        if output_prefix == '':
            output_prefix = 'boost'
            self.setConfigureOption('output prefix',output_prefix)

        if self.getBoosterOption('model format') not in ['AMPL']:
            raise IOError("Invalid model format: "+self.getBoosterOption('model format'))
        
        objective = self.getBoosterOption('objective')
        _assert_(objective not in self.none_list)
        _assert_(objective in ['MASS','Mass','mass','SETPOINT','Setpoint','setpoint'])

        decay_coef = self.getBoosterOption('decay coefficient')
        _assert_(decay_coef not in self.none_list)
        _assert_((decay_coef in ['INP','Inp','inp']) or (decay_coef >= 0.0))

        min_qual = self.getBoosterOption('min quality')
        _assert_(min_qual not in self.none_list)
        _assert_(min_qual >= 0.0)
        self.setBoosterOption('min quality', float(min_qual))

        max_qual = self.getBoosterOption('max quality')
        _assert_(max_qual not in self.none_list)
        _assert_((max_qual in ['AUTO','Auto','auto']) or (max_qual >= 0.0))
        if max_qual not in ['AUTO','Auto','auto']:
            self.setBoosterOption('max quality', float(max_qual))

        source_qual = self.getBoosterOption('source quality')
        _assert_(source_qual not in self.none_list)
        _assert_(source_qual >= 0.0)
        self.setBoosterOption('source quality', float(source_qual))

        setp_qual = self.getBoosterOption('setpoint quality')
        _assert_(setp_qual not in self.none_list)
        _assert_(setp_qual >= 0.0)
        self.setBoosterOption('setpoint quality', float(setp_qual))

        source_nodes = self.getBoosterOption('source nodes')
        _assert_(source_nodes.__class__ is list)
        self.setBoosterOption('source nodes',[str(i) for i in source_nodes])

        quality_start = self.getBoosterOption('quality start')
        _assert_(quality_start not in self.none_list)
        _assert_(quality_start >= 0.0)

        _assert_(self.getBoosterOption('max boosters') not in self.none_list)
        Max_Stations = self.getBoosterOption('max boosters')
        if Max_Stations.__class__ in [tuple,list]:
            if not all([(i.__class__ is int) and (i >= 0) for i in Max_Stations]):
                raise TypeError, "'max boosters' parameter must be a non-negative integer or a list of non-negative integers."               
        elif (Max_Stations.__class__ is int) and (Max_Stations >= 0):
            Max_Stations = [Max_Stations]
        else:
            raise TypeError, "'max boosters' parameter must be a non-negative integer or a list of non-negative integers."
        
        self.opts['booster']['max boosters'] = Max_Stations        

        return

    def createAmplRun(self,name_to_id_nodemap,max_stations,filename):

        ampl_model = os.path.join(os.path.dirname(os.path.abspath(__file__)),'ampl','booster_quality.mod')
        commands_ampl_model = os.path.join(os.path.dirname(os.path.abspath(__file__)),'ampl','booster_quality_fixing.mod')
        objective = self.getBoosterOption('objective')

        data_file_prefix = ''
        if self.getConfigureOption('output prefix') not in self.none_list:
            data_file_prefix += self.getConfigureOption('output prefix')+'-'

        fid = open(filename,'wt')
        fid.write('option presolve 0;\n')
        fid.write('option substout 0;\n')
        fid.write('\n')

        fid.write('# Define solver options\n')
        fid.write('option solver '+self.getSolverOption('executable')+';\n')
        # HACK: Not sure what the correct label is for solvers other than
        # cplex and gurobi so I will throw an error if I encounter options.
        # The alternative is to ask the user for the solver executable and this
        # ampl specific label which would be weird. The solver configuration system
        # will likely be updated in the future so this should work for now.
        options_label = ''
        if self.getSolverOption('executable') == 'cplexamp':
            options_label += 'cplex_options'
        elif self.getSolverOption('executable') == 'gurobi_ampl':
            options_label += 'gurobi_options'
        
        if self.getSolverOption('options') not in self.none_list:
            if options_label != '':
                fid.write('option '+options_label+' \'')
                for (key,value) in self.getSolverOption('options').iteritems():                
                    if value in self.none_list:
                        # this is the case where an option does not accept a value
                        fid.write(key+' ')
                    else:
                        fid.write(key+'='+str(value)+' ')
                fid.write('\';\n')
            else:
                print >> sys.stderr, ' '
                print >> sys.stderr, "WARNING: Solver options in AMPL are currently not handled for"
                print >> sys.stderr, "         the specified solver: ", self.getSolverOption('executable')
                print >> sys.stderr, "         All solver options will be ignored."
                print >> sys.stderr, ' '

        fid.write('# Booster placement model\n')
        fid.write('model %s;\n'%ampl_model)
        fid.write('\n')

        fid.write('# Booster model data\n')
        fid.write('data '+os.path.join(os.path.abspath(os.curdir),data_file_prefix+'WQM_HEADERS.dat')+'\n')
        fid.write('data '+os.path.join(os.path.abspath(os.curdir),data_file_prefix+'WQM_TYPES.dat')+'\n')
        fid.write('data '+os.path.join(os.path.abspath(os.curdir),data_file_prefix+'WQM.dat')+'\n')
        fid.write('data '+os.path.join(os.path.abspath(os.curdir),data_file_prefix+'BOOSTER_CANDIDATES.dat')+'\n')
        fid.write('data '+os.path.join(os.path.abspath(os.curdir),data_file_prefix+'BOOSTER_INJECTION.dat')+'\n')
        fid.write('\n')

        if self.getBoosterOption('evaluate') is True:
            fid.write('# Fixing booster nodes\n')
            fid.write('for {b in S_BOOSTER_CANDIDATES} {\n')
            fid.write('  fix y_booster[b] := 1;\n')
            fid.write('}\n')
            fid.write('\n')
        elif len(self.getBoosterOption('fixed nodes')) != 0:
            fid.write('# Fixing booster nodes\n')
            for node in self.getBoosterOption('fixed nodes'):
                fid.write('fix y_booster[{0!r}] := 1;\n'.format(name_to_id_nodemap[str(node)]))
            fid.write('\n')

        fid.write('# Max number of stations\n')
        fid.write('let P_MAX_STATIONS := %d;\n'%max_stations)
        fid.write('\n')
        fid.write('# Minimum allowable water quality\n')
        fid.write('let P_MIN_CONC_BOOSTER_gpm3 := {0!r};\n'.format(self.getBoosterOption('min quality')))
        fid.write('\n')

        fid.write('# water sources \n')
        source_nodes = self.getBoosterOption('source nodes')
        fid.write('let S_SOURCES_BOOSTER := { ')
        for i in range(len(source_nodes)):
            fid.write(str(name_to_id_nodemap[str(source_nodes[i])]))
            if (i+1) < len(source_nodes):
                fid.write(', ')
            else:
                fid.write(' ')
        fid.write('};\n\n')
        fid.write('# water quality from sources \n')
        fid.write('let P_SOURCE_CONC_BOOSTER_gpm3 := {0!r};\n'.format(self.getBoosterOption('source quality')))
        fid.write('\n')
        fid.write('# setpoint water quality\n')
        fid.write('let P_SETPOINT_CONC_BOOSTER_gpm3 := {0!r};\n'.format(self.getBoosterOption('setpoint quality')))
        fid.write('\n')
        fid.write('# Global initial water quality (constant for reservoirs)\n')
        fid.write('let P_QUALITY_START_BOOSTER_min := {0!r};\n'.format(self.getBoosterOption('quality start')))
        fid.write('\n')

        fid.write('# Booster placement model\n')
        fid.write('commands %s;\n'% (commands_ampl_model))
        fid.write('\n')

        if self.getBoosterOption('max quality') in ['AUTO','Auto','auto']:
            fid.write('# Solve the feasibility problem\n')
            fid.write('objective FEASIBILITY;\n')
            fid.write('solve;\n')
            fid.write('# Fix Maximum allowable water quality\n')
            fid.write('fix c_max_booster_gpm3;\n')
            fid.write('\n')
        else:
            fid.write('# Fix Maximum allowable water quality\n')
            fid.write('fix c_max_booster_gpm3 := {0!r};\n'.format(self.getBoosterOption('max quality')))
            fid.write('\n')
            
        fid.write("# define the objective to use\n")
        if objective in ['MASS','Mass','mass']:
            fid.write('objective MASS;\n')
        elif objective in ['SETPOINT','Setpoint','setpoint']:
            fid.write('objective SETPOINT;\n')
        else:
            raise ValueError('Bad objective tag encountered')

        fid.write('# Solve the problem\n')
        fid.write('solve;\n')
        
        fid.write('option omit_zero_rows 1;\n')
        
        fid.write('# Actual total mass consumed\n')

        if objective in ['MASS','Mass','mass']:
            fid.write('display MASS;\n\n')
        elif objective in ['SETPOINT','Setpoint','setpoint']:
            fid.write('display SETPOINT;\n\n')
        else:
            raise ValueError('Bad objective tag encountered')
        fid.write('# Booster nodes\n')
        fid.write('display y_booster;\n\n')
        
        fid.write('# Important exit codes to check\n')
        fid.write('display solve_result_num;\n')
        fid.write('display solve_result;\n')
        fid.write('display solve_exitcode;\n\n')
        
        results_file = os.path.join(os.path.abspath(os.curdir),data_file_prefix+'ampl-results-b'+str(max_stations)+'.yml')
        fid.write('\n')
        fid.write('#Print the results to a yaml file\n')
        fid.write('printf \'\\\"solve_result_num\\\": %q\\n\', solve_result_num > '+results_file+';\n')
        fid.write('printf \'\\\"solve_result\\\": %q\\n\', solve_result >> '+results_file+';\n')
        fid.write('printf \'\\\"solve_exitcode\\\": %q\\n\', solve_exitcode >> '+results_file+';\n')
        fid.write('printf \'\\\"booster ids\\\": [\' >> '+results_file+';\n')
        fid.write('for {b in S_BOOSTER_CANDIDATES} {\n')
        fid.write('    if y_booster[b] > 0.9 then {\n')
        fid.write('        printf "%q,", b >> '+results_file+';\n')
        fid.write('    }\n')
        fid.write('}\n')
        fid.write('printf \"]\\n\" >> '+results_file+';\n')
        if objective in ['MASS','Mass','mass']:
            fid.write('printf \'\\\"objective\\\": %q\\n\', MASS >> '+results_file+';\n')
        elif objective in ['SETPOINT','Setpoint','setpoint']:
            fid.write('printf \'\\\"objective\\\": %q\\n\', SETPOINT >> '+results_file+';\n')
        else:
            raise ValueError('Bad objective tag encountered')
        fid.write('printf \'\\\"injected mass grams\\\": %q\\n\', MASS >> '+results_file+';\n')
        fid.write('printf \'\\\"average setpoint deviation\\\": %q\\n\', SETPOINT >> '+results_file+';\n')
        fid.write('printf \'\\\"quality period minutes\\\": %q\\n\', P_MINUTES_PER_TIMESTEP*card(S_TIMES_QUALITY) >> '+results_file+';\n')
        fid.write('printf \'\\\"min quality\\\": %q\\n\', P_MIN_CONC_BOOSTER_gpm3 >> '+results_file+';\n')
        fid.write('printf \'\\\"max quality\\\": %q\\n\', c_max_booster_gpm3 >> '+results_file+';\n')
        fid.write('\n')
        concentrations_file = os.path.join(os.path.abspath(os.curdir),data_file_prefix+'ampl-concentrations-b'+str(max_stations)+'.json')
        fid.write('\n')
        fid.write('#Print the concentrations to a json file\n')
        fid.write('printf \'{\' > '+concentrations_file+';\n')
        fid.write('printf \'\\\"quality start\\\": %q,\\n\', first(S_TIMES_QUALITY) >> '+concentrations_file+';\n')
        fid.write('printf \'\\\"min quality\\\": %q,\\n\', P_MIN_CONC_BOOSTER_gpm3 >> '+concentrations_file+';\n')
        fid.write('printf \'\\\"max quality\\\": %q,\\n\', c_max_booster_gpm3 >> '+concentrations_file+';\n')
        fid.write('printf \'\\\"nodes\\\": {\\n\' >> '+concentrations_file+';\n')
        fid.write('for {n in S_NODES} {\n')
        fid.write('    printf \'\\\"%q\\\": [\', n >> '+concentrations_file+';\n')
        fid.write('    for {t in S_TIMES} {\n')
        fid.write('        if (t != last(S_TIMES)) then {\n')
        fid.write('            printf "%q,", c_booster_gpm3[n,t] >> '+concentrations_file+';\n')
        fid.write('        } else {\n')
        fid.write('            if (n != last(S_NODES)) then {\n')
        fid.write('                printf "%q],\\n", c_booster_gpm3[n,t] >> '+concentrations_file+';\n')
        fid.write('            } else {\n')
        fid.write('                printf "%q]\\n", c_booster_gpm3[n,t] >> '+concentrations_file+';\n')
        fid.write('            }\n')
        fid.write('        }\n')
        fid.write('    }\n')
        fid.write('}\n')
        fid.write('printf \'}\' > '+concentrations_file+';\n')
        fid.write('printf \"}\\n\" >> '+concentrations_file+';\n')
        fid.write('\n')
        mass_injected_file = os.path.join(os.path.abspath(os.curdir),data_file_prefix+'ampl-mass-injected-b'+str(max_stations)+'.json')
        fid.write('\n')
        fid.write('#Print the mass injected to a json file\n')
        fid.write('printf \'{\' > '+mass_injected_file+';\n')
        fid.write('printf \'\\\"quality start\\\": %q,\\n\', first(S_TIMES_QUALITY) >> '+mass_injected_file+';\n')
        fid.write('printf \'\\\"nodes\\\": {\\n\' >> '+mass_injected_file+';\n')
        fid.write('for {n in S_BOOSTER_CANDIDATES} {\n')
        fid.write('    printf \'\\\"%q\\\": [\', n >> '+mass_injected_file+';\n')
        fid.write('    for {t in S_TIMES} {\n')
        fid.write('        if (t != last(S_TIMES)) then {\n')
        fid.write('            printf "%q,", m_booster_gpmin_[n] >> '+mass_injected_file+';\n')
        fid.write('        } else {\n')
        fid.write('            if (n != last(S_BOOSTER_CANDIDATES)) then {\n')
        fid.write('                printf "%q],\\n", m_booster_gpmin_[n] >> '+mass_injected_file+';\n')
        fid.write('            } else {\n')
        fid.write('                printf "%q]\\n", m_booster_gpmin_[n] >> '+mass_injected_file+';\n')
        fid.write('            }\n')
        fid.write('        }\n')
        fid.write('    }\n')
        fid.write('}\n')
        fid.write('printf \'}\' > '+mass_injected_file+';\n')
        fid.write('printf \"}\\n\" >> '+mass_injected_file+';\n')
        fid.write('\n')
        
        fid.close()

        return results_file, concentrations_file, mass_injected_file

    # General Option SET functions
    def setNetworkOption(self, name, value):
        self.opts['network'][name] = value
        return
    
    def setSolverOption(self, name, value):
        self.opts['solver'][name] = value
        return

    def setBoosterOption(self, name, value):
        self.opts['booster'][name] = value
        return

    def setConfigureOption(self, name, value):
        if name == 'output prefix' and value != '':
            output_prefix = os.path.splitext(os.path.split(value)[1])[0]
            value = output_prefix
        self.opts['configure'][name] = value
        return

    # Node / Pipe List Functions
    def setFeasibleNodes(self):

        # check that the give epanet file exists, otherwise epanet will just cause a segfault
        try:
            fp = open(self.getNetworkOption('epanet file'))
            fp.close()
        except IOError:
            raise

        try:
            enData = pyepanet.ENepanet()
            epanet_rpt_file = pyutilib.services.TempfileManager.create_tempfile(suffix = '.epanet.rpt')     
            enData.ENopen(self.getNetworkOption('epanet file'),epanet_rpt_file)
        except:
            raise RuntimeError("EPANET inp file not loaded using pyepanet")
        
        nnodes = enData.ENgetcount(pyepanet.EN_NODECOUNT)

        # set feasible locations  
        list_feas = []
        feasible = self.opts['booster']['feasible nodes']

        if feasible == 'ALL':
            for i in range(nnodes):
                if enData.ENgetnodetype(i+1) != pyepanet.EN_TANK:
                    if enData.ENgetnodeid(i+1) not in self.getBoosterOption('source nodes'):
                        list_feas.append(enData.ENgetnodeid(i+1))
        elif feasible == 'NZD':
            for i in range(nnodes):
                if enData.ENgetnodetype(i+1) != pyepanet.EN_TANK:
                    dem = enData.ENgetnodevalue(i+1,pyepanet.EN_BASEDEMAND)
                    if dem > 0:
                        list_feas.append(enData.ENgetnodeid(i+1))
        elif feasible.__class__ is list:
            for i in feasible:
                list_feas.append(str(i))
        elif feasible in self.none_list:
            # prevents entering next 'elif' block
            pass
        elif feasible.__class__ is str:
            try:
                fid = open(feasible,'r')
            except:
                raise RuntimeError("feasible nodes file did not load")
            list_feas = fid.read()
            fid.close()
            list_feas = list_feas.splitlines()
        else:
            print >> sys.stderr, "Unsupported feasible nodes, setting option to None"
            self.setBoosterOption('feasible nodes',None)
        
        # set infeasible locations 
        list_infeas = []
        infeasible = self.opts['booster']['infeasible nodes']
        if infeasible == 'ALL':
            for i in range(nnodes):
                list_infeas.append(enData.ENgetnodeid(i+1))
        elif infeasible == 'NZD':
            for i in range(nnodes):
                dem = enData.ENgetnodevalue(i+1,pyepanet.EN_BASEDEMAND)
                if dem > 0:
                    list_infeas.append(enData.ENgetnodeid(i+1))            
        elif infeasible.__class__ is list:
            for i in infeasible:
                list_infeas.append(str(i))
        elif infeasible in self.none_list:
            # prevents entering next 'elif' block
            pass
        elif infeasible.__class__ is str:
            try:
                fid = open(infeasible,'r')
            except:
                raise RuntimeError("Infeasible nodes file did not load")
            list_infeas = fid.read()
            fid.close()
            list_infeas = list_infeas.splitlines()
        else:
            print "Unsupported infeasible nodes, setting option to None"
            self.setBoosterOption('infeasible nodes',None)
        
        # remove infeasible from feasible
        final_list = []
        for i in list_feas:
            if i not in list_infeas:
                final_list.append(i)
        
        # make sure the list consists of strings
        self.setBoosterOption('fixed nodes', [str(node) for node in self.getBoosterOption('fixed nodes')])
        for node in self.getBoosterOption('fixed nodes'):
            if str(node) not in final_list:
                raise ValueError("Fixed node with name %s is not in the list of feasible nodes." % (node))

        # assign to internal parameters
        self.opts['internal']['nodeNames'] = list(final_list) #copy

        if len(final_list) == 0:
            raise RuntimeError("\nERROR: List of feasible booster node locations is empty.\n")
        elif self.getBoosterOption('evaluate') is True:
            print 
            print "Evaluation Mode: Fixing the booster station solution to the set of candidate booster station nodes."
            print
            self.setBoosterOption( 'max boosters', [len(self.opts['internal']['nodeNames'])] )
        elif len(final_list) < max(self.getBoosterOption('max boosters')):
            print >> sys.stderr, ' '
            print >> sys.stderr, "WARNING: max boosters is larger than the number of candidate booster nodes.\n"
            print >> sys.stderr, ' ' 
                
        enData.ENclose()
        return

    # General Option GET functions
    def getBoosterNodesList(self):
        return self.opts['internal']['nodeNames']

    def getConfigureOption(self, name):
        return self.opts['configure'][name]

    def getBoosterOption(self, name):
        return self.opts['booster'][name]

    def getNetworkOption(self, name):
        return self.opts['network'][name]

    def getSolverOption(self, name):
        return self.opts['solver'][name]

    def getInternalOption(self, name):
        return self.opts['internal'][name]
