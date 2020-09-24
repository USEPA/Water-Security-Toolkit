from collections import namedtuple
from pyomo.environ import *
import pyutilib.subprocess
from random import randint
from mpi_util import *
import numpy as np
import itertools
import logging
import shutil
import copy
import sys
import os


logger = logging.getLogger(__name__)

SIGNALS_FORMAT = ".signals"
SIGNALS_IDX_FORMAT = ".idx"
INFONET_FORMAT = ".infonet"
INP_FORMAT = ".inp"
TSG_FORMAT = ".tsg"
INP_MAP_FILE = "list_scenarios"
EMPTY_LINE = ['\n', '\t\n', ' \n', '']
INPUTS_DIR = "INPUTS"
SIGNALS_DIR = "SIGNALS"
ERDS_DIR = "ERDS"
DATA_OUTPUT_DIR = "DATA_FOLDER"
LIST_IDS_FILE = "list_ids.dat"
ScenarioSet = namedtuple('ScenarioSet', ['sid', 'inp', 'tsg'])

class Scenario(object):

    # Keep the list of times outside of the class so that can be built faster. To use carefully
    def __init__(self,hid,cid,list_times, 
                 list_pointers=None, 
                 list_data=None,
                 inp_filemane=None, 
                 injection = None):
        n_steps = len(list_times)
        if list_pointers is None and list_data is None:
            list_sets = [set() for t in xrange(n_steps)]
            self.time_to_nodes = dict(zip(list_times,list_sets))
            self.flattened = False
        elif list_pointers is not None and list_data is not None:
            self.time_to_nodes = [list_times,list_pointers,list_data]
            self.flattened = True
        else:
            raise RuntimeError('Bad creation of scenario')
        self._injection = injection
        self._probability = None 
        self.n_meas_matches = 0
        self.n_meas_unmatches = 0
        self._n_matches = 0
        self._n_miss_matches = 0
        self._inp_filename = inp_filemane
        self._hid = hid
        self._cid = cid
        self._checked_measurements = set()
        
    @property
    def hid(self):
        return self._hid
    @property
    def cid(self):
        return self._cid
    @property
    def injection(self):
        return self._injection
    @property
    def probability(self):
        return self._probability
    @property
    def inp_filename(self):
        return self._inp_filename
    @property
    def matches(self):
        return self._n_matches
    @property
    def miss_matches(self):
        return self._n_miss_matches

    @cid.setter
    def cid(self,value):
        self._cid = value
    @hid.setter
    def hid(self,value):
        self._hid = value
    @injection.setter
    def injection(self,value):
        self._injection = value
    @probability.setter
    def probability(self,value):
        self._probability = value
    @inp_filename.setter
    def inp_filename(self,value):
        self._inp_filename = value
    @matches.setter
    def matches(self,value):
        self._n_matches = value
    @miss_matches.setter
    def miss_matches(self,value):
        self._n_miss_matches = value

    # time in minutes
    def write_measurements(self,filename,sample_tuples,idx_to_node_map,flip=0):
        #print sample_tuples
        with open(filename,'w') as f:
            counter = 0
            for nid,t in sample_tuples:
                if counter<flip:
                    if self.is_contaminated(nid,t):
                        outcome = 0 
                        f.write("{} {} {}\n".format(idx_to_node_map[nid],int(t*60),outcome))
                    else:
                        outcome = 1 
                        f.write("{} {} {}\n".format(idx_to_node_map[nid],int(t*60),outcome))
                else:
                    if self.is_contaminated(nid,t):
                        f.write("{} {} {}\n".format(idx_to_node_map[nid],int(t*60),1))
                    else:
                        f.write("{} {} {}\n".format(idx_to_node_map[nid],int(t*60),0))
                counter+=1
    def clear(self):
    	self._hid = None
        self._cid = None
        for t in self.time_to_nodes.iterkeys():
        	self.time_to_nodes[t].clear()

    def is_contaminated(self,nodeid,t):
        return nodeid in self.time_to_nodes[t]

    def get_contaminated_nodes_at_time(self,t,cumulative=False):
        if cumulative:
            nodes = set()
            sublist_time = (tt for tt in self.time_to_nodes.iterkeys() if tt<=t)
            for tt in sublist_time:
                nodes.update(self.time_to_nodes[tt])
            return nodes
        else:    
            return self.time_to_nodes[t]

    def EC(self,t,node_map,cumulative=True):
        nodes = self.get_contaminated_nodes_at_time(t,cumulative=cumulative)
        return sum(n*node_map[n] for n in nodes)
    
    def pprint(self, idx_to_node_map=None, detail=False):
        str_to_print = 'hid: {0} cid: {1}'.format(self._hid,self._cid)
        if self._probability is not None:
            str_to_print += ' prob: {}'.format(self._probability)
    	print str_to_print
        if self.inp_filename is not None:
            print self.inp_filename
        if self._injection is not None:
            if idx_to_node_map is not None:
                source = idx_to_node_map[self._injection.source]
            else:
                source = self._injection.source
            print 'Source: {0} \nType: {1}\nStrength: {2}\nStart: {3}\nEnd: {4}\n'.format(source,
            self._injection.type,self._injection.strength,self._injection.start_sec,self._injection.end_sec)
        else:
            print "No injection information"
        if detail:
        	if idx_to_node_map is not None:
                    ordered_times = sorted(self.time_to_nodes.keys())
                    for t in ordered_times:
                        nodes = self.time_to_nodes[t]
                        sys.stdout.write(str(t) + " [")
                        for n in sorted(nodes):
                            sys.stdout.write(idx_to_node_map[n] + " ")
                        sys.stdout.write("]\n")
        	else:
                    ordered_times = sorted(self.time_to_nodes.keys())
                    for t in ordered_times:
                        nodes =  self.time_to_nodes[t]
                        print t, sorted(nodes)             
    
    def flattened(self):
        if self.flattened == False:
            n_steps = len(self.time_to_nodes)
            list_times = np.arange(n_steps,dtype=np.uint32)
            list_data = list()
            list_pointer = list()
            accum = 0
            ordered_times = sorted(self.time_to_nodes.keys())
            for t,nodes in ordered_times:
                nodes = self.time_to_nodes[t]
                list_times.append(t)
                list_pointer.append(accum)
                accum+=len(nodes)
                for n in nodes:
                    list_data.append(n)
            array_pointers = np.array(list_pointer,dtype=np.uint32)
            array_data = np.array(list_data,dtype=np.uint32)
            self.time_to_nodes = [list_times,array_pointers,array_data]
            flat_injection = self._injection.get_flat_representation()
            self._injection = flat_injection
            self.flattened = True
    
    def get_flattened_arrays(self):
        if self.flattened == False:
            list_times = list()
            list_data = list()
            list_pointer = list()
            accum = 0
            ordered_times = sorted(self.time_to_nodes.keys())
            for t in ordered_times:
                nodes = self.time_to_nodes[t]
                list_times.append(t)
                list_pointer.append(accum)
                accum+=len(nodes)
                for n in nodes:
                    list_data.append(n)
            return list_pointer,list_data
        else:
            list_pointer = self.time_to_nodes[1]
            list_data = self.time_to_nodes[2]
            return list_pointer,list_data

    def get_flattened_arrays2(self):
        if self.flattened == False:
            list_times = list()
            list_data = list()
            list_pointer = list()
            accum = 0
            ordered_times = sorted(self.time_to_nodes.keys())
            for t in ordered_times:
                nodes = self.time_to_nodes[t]
                list_times.append(t)
                list_pointer.append(accum)
                accum+=len(nodes)
                for n in nodes:
                    list_data.append(n)
            array_pointers = np.array(list_pointer,dtype=np.uint32)
            array_data = np.array(list_data,dtype=np.uint32)
            return array_pointers,array_data
        else:
            list_pointer = self.time_to_nodes[1]
            list_data = self.time_to_nodes[2]
            return list_pointer,list_data

    def deflattend(self):
        if self.flattened == True:
            list_times = self.time_to_nodes[0]
            n_steps = len(list_times)
            list_pointer = self.time_to_nodes[1]
            list_data = self.time_to_nodes[2]
            contaminations = dict()
            for i,t in enumerate(list_times):
                start = list_pointer[i]
                if i<n_steps-1:
                    end = list_pointer[i+1]
                else:
                    end = len(list_data)
                indices = np.arange(start,end,dtype=np.uint32)
                values = np.take(list_data,indices)
                contaminations[t] = set(values)

            self.time_to_nodes = contaminations
            flat_injection = [a for a in self._injection]
            self._injection = Injection(*flat_injection)
            self.flattened = False

    def build_contaminations_from_flat_arrays(self,list_times,list_pointer,list_data):
        contaminations = dict()            
        n_steps = len(list_times)
        for i,t in enumerate(list_times):
            start = list_pointer[i]
            if i<n_steps-1:
                end = list_pointer[i+1]
            else:
                end = len(list_data)
            indices = np.arange(start,end,dtype=np.uint32)
            values = np.take(list_data,indices)
            contaminations[t] = set(values)
        self.time_to_nodes = contaminations
        self.flattened = False

class Injection:
    def __init__(self, source_location, type_inj, strength, start, end):
        self.source = source_location
        self.type = type_inj
        self.strength = strength
        self.start_sec = start
        self.end_sec = end

    def tsg_line(self,node_id_to_name):
        return '{0} {1} {2} {3} {4}\n'.format(node_id_to_name[self.source],
            self.type,self.strength,self.start_sec,self.end_sec)
        
    def get_flat_representation(self):
        return[self.source,self.type,self.strength,self.start_sec,self.end_sec]
        
    def __str__(self):
        return 'Source: {0} \nType: {1}\nStrength: {2}\nStart: {3}\nEnd: {4}\n'.format(self.source,
            self.type,self.strength,self.start_sec,self.end_sec)

class Simulator(object):

    def __init__(self, scenarios_file,
                 storage_dir=DATA_OUTPUT_DIR):

        validate_list_scenarios_file(scenarios_file)
        self.scenario_sets = read_list_scenarios_file(scenarios_file)
        self.storage_dir = storage_dir
        if rank == 0:
            self.setup_output_folders()
        if size>1 and found_mpi:
            comm.barrier() # all to wait until storage directories are built
        if rank == 0:
            copy_input_files_to_folder(self.storage_dir,
                                       self.scenario_sets)
        if size>1 and found_mpi:
            comm.barrier() # all to wait until storage directories are built
            
    def setup_output_folders(self):
        output_dir = self.storage_dir
        if rank==0:
            if os.path.exists(output_dir):
                shutil.rmtree(output_dir)
                print "WARNING: Reseting folder {}".format(output_dir)
            os.mkdir(output_dir)
            
            inputs_folder = os.path.join(output_dir,INPUTS_DIR)
            if not os.path.exists(inputs_folder):
                os.mkdir(inputs_folder)

            erds_folder = os.path.join(output_dir,ERDS_DIR)
            if not os.path.exists(erds_folder):
                os.mkdir(erds_folder)

            signals_folder = os.path.join(output_dir,SIGNALS_DIR)
            if not os.path.exists(signals_folder):
                os.mkdir(signals_folder)

            
    def create_signals(self, 
                       signals_filename, 
                       threshold, 
                       erd_filename, 
                       with_infonet=True,
                       start_time = -1,
                       report_time = -1):
        cmd = "erd2Signals "
        if not with_infonet:
            cmd += "--no-nodemap-file "  
        if start_time>0:
            cmd += "--startTime={} ".format(start_time)
        if report_time>=0:
            cmd += "--reportTime={} ".format(report_time)

        cmd += signals_filename + " " + str(threshold) + " " + erd_filename
        #print cmd
        p = pyutilib.subprocess.run(cmd)
        if (p[0]):
            print 'An error occured when running the erd2Signals executable.\n Error Message: ' + p[1] + '\n Command: ' + cmd + '\n'
        return p[0]

    def run_tevasim(self, options):
        if options['merlion']:
            cmd = "tevasim --merlion --tsg=" + options['tsg'] + " " + options[
                'inp'] + " " + options['out_prefix'] + ".epa " + options['out_prefix']
        else:
            cmd = "tevasim --tsg=" + options['tsg'] + " " + options[
                'inp'] + " " + options['out_prefix'] + ".epa " + options['out_prefix']
        #print cmd
        p = pyutilib.subprocess.run(cmd)
        if (p[0]):
            print 'An error occured when running the erd2Signals executable.\n Error Message: ' + p[1] + '\n Command: ' + cmd + '\n'
        return p[0]

    def run_simulation_kernel(self, 
                              local_scenario_sets, 
                              erd_folder, 
                              signals_folder, 
                              threshold, 
                              **kwds):
        
        #options
        with_merlion = kwds.pop('with_merlion',False)
        with_infonet = kwds.pop('with_infonet',True)
        del_erds = kwds.pop('del_erds',True)
        message = kwds.pop('message','')
        start_time = kwds.pop('start_time',-1)
        report_time = kwds.pop('report_time',-1)

        N_scenarios = len(local_scenario_sets)
        clean_run = 1
        for scenario_set in local_scenario_sets:
            inpfile = scenario_set.inp
            tsgfile = scenario_set.tsg
            sset_id = str(scenario_set.sid)
            out_prefix = os.path.join(erd_folder, sset_id)
            options = {'inp': inpfile,
                       'tsg': tsgfile,
                       'merlion': with_merlion,
                       'out_prefix': out_prefix,
                       'ignore_warnings': False}

            success_tevasim = self.run_tevasim(options)

            erd_filename = os.path.join(erd_folder, sset_id + ".erd")
            signals_filename = os.path.join(signals_folder, sset_id)
            if(success_tevasim == 0):
                success_signals = self.create_signals(signals_filename,
                                                      threshold,
                                                      erd_filename,
                                                      with_infonet=with_infonet,
                                                      start_time=start_time,
                                                      report_time=report_time)
            else:
                logger.error('RANK {} failed tevasim of set {} {}'.format(rank, sset_id,message))
                success_signals = 1
                clean_run = 0
            if success_signals == 0:
                logger.debug('RANK {} done simulating set {} {}'.format(rank,sset_id,message))
            else:
                logger.debug('RANK {} failed erd2Signals of set {} {}'.format(rank, sset_id,message))
                clean_run = 0
            
            if del_erds and success_tevasim==0:
                basename,extension = os.path.splitext(erd_filename)
                os.remove(erd_filename)
                os.remove(basename+'.epa')
                os.remove(basename+'-1.qual.erd')
                os.remove(basename+'-1.hyd.erd')
                os.remove(basename+'.index.erd')

        return clean_run

    def run_simulations(self, threshold, **kwds):

        # options
        with_merlion = kwds.pop('with_merlion',False)
        with_infonet = kwds.pop('with_infonet',True)
        del_erds = kwds.pop('del_erds',True)
        message = kwds.pop('message','')
        start_time = kwds.pop('start_time',-1)
        report_time = kwds.pop('report_time',-1)
        parallel = kwds.pop('parallel', False)

        scenarios_sets = self.scenario_sets
        files_container = self.storage_dir
        erd_folder = os.path.join(files_container, ERDS_DIR)
        signals_folder = os.path.join(files_container, SIGNALS_DIR)

        N_scenarios = len(scenarios_sets)
        if parallel:
            r_start, r_end = distributeN(comm, N_scenarios)
        else:
            r_start = 0
            r_end = N_scenarios

        local_scenario_sets = [scenarios_sets[j]
                               for j in xrange(r_start, r_end)]

        clean_run = self.run_simulation_kernel(local_scenario_sets,
                                               erd_folder,
                                               signals_folder,
                                               threshold,
                                               with_merlion=with_merlion,
                                               with_infonet=with_infonet,
                                               del_erds=del_erds,
                                               message=message,
                                               start_time=start_time,
                                               report_time=report_time)
        if clean_run:
            if len(local_scenario_sets)>1:
                logger.debug('RANK {} done simulating sets [{}-{}] {}'.format(rank,r_start,r_end-1,message))
        else:
            logger.error('RANK {} failed one or more simulations of set [{}-{}] {}'.format(rank,r_start,r_end-1,message))
            raise RuntimeError('processor {0} didnt finish simulation and parsing {}'.format(rank, message))


def read_list_scenarios_file(filename):
    f = open(filename, 'r')
    scenario_sets = list()
    for i, line in enumerate(f):
        if line not in EMPTY_LINE and '#' not in line:
            l = line.split()
            if len(l) == 3:
                s = ScenarioSet(sid=int(l[0]), inp=l[1], tsg=l[2])
                scenario_sets.append(s)
            else:
                raise RuntimeError(
                    'Error in line {} of {}'.format(i, filename))
    f.close()
    return scenario_sets


def validate_list_scenarios_file(filename):
    f = open(filename, 'r')
    try:
        for counter, line in enumerate(f):
            if '#' not in line:
                if line not in EMPTY_LINE:
                    l = line.split()
                    if len(l) != 3:
                        raise RuntimeError(
                            'Error in line {} of list_scenarios file'.format(counter))
                    for j in xrange(1, 3):
                        name = l[j]
                        if not os.path.exists(name):
                            raise RuntimeError(
                                'Error in line {} of list_scenarios file. File {} cannot be found'.format(counter, name))
    except Exception, e:
        raise
    else:
        pass
    finally:
        f.close()


def copy_input_files_to_folder(folder, scenario_sets):
    scenario_sets_file = os.path.join(folder, INP_MAP_FILE)
    inputs_folder = os.path.join(folder, INPUTS_DIR)
    with open(scenario_sets_file, 'w') as f:
        f.write('#sid inp tsg\n')
        for scenario_set in scenario_sets:
            inp = scenario_set.inp
            tsg = scenario_set.tsg
            sid = scenario_set.sid
            f.write("{} {} {}\n".format(sid, inp, tsg))
            shutil.copy(inp, inputs_folder)
            shutil.copy(tsg, inputs_folder)


############################# Data Manager ###################################

class DataManager(object):

    def __init__(self, 
                 data_folder,
                 report_time_step_min=None,
                 report_start_time=0,
                 report_end_time=-1):

        validate_data_folder(data_folder)

        self._signals_n_steps = None
        self._signals_time_step_min = None
        self._signals_start_time = None
        
        self._report_step_min = report_time_step_min
        self._report_start_time = report_start_time
        self._report_end_time = report_end_time

        self.idx_to_node_name = dict()
        self.node_name_to_idx = dict()
        self.data_folder = data_folder
        signals_dir = os.path.join(data_folder, SIGNALS_DIR)
        self.signals_dir = signals_dir
        inputs_dir = os.path.join(data_folder, INPUTS_DIR)
        self.inputs_dir = inputs_dir
        all_infofiles = [os.path.join(signals_dir, filename) for filename in os.listdir(
            signals_dir) if filename.endswith(INFONET_FORMAT)]
        infonet_file = all_infofiles[0]  # all should have the same

        # parse time data from infonet files
        self.read_info_file(infonet_file)
        
        # check if the time parameters passed agree with signals
        if self._report_step_min is not None:
            if self._report_step_min%self._signals_time_step_min!=0:
                raise RuntimeError('The specified report time step is not multiple of signals time step')
        else:
            self._report_step_min = self._signals_time_step_min


        # adjust start time report
        self._report_start_time -= self._report_start_time%self._signals_time_step_min
        if self._report_start_time < self._signals_start_time:
            if self._report_start_time>0:
                print "WARNING: report start time less than start time in signals data. Adjusting to signals start time"
            self._report_start_time = self._signals_start_time
    
        if self._report_start_time>self._signals_start_time:
            self.start_time = self._report_start_time
        else:
            self.start_time = self._signals_start_time

        self.time_step_min = self._report_step_min

        # adjust end time
        last_signals = self._signals_start_time + self._signals_time_step_min*(self._signals_n_steps-1)
        if self._report_end_time<0:
            self._report_end_time = last_signals
        else:
            if self._report_end_time%self._report_step_min!=0:
                self._report_end_time -= self._report_end_time%self._report_step_min
                self._report_end_time += self._report_step_min
            if self._report_end_time >= last_signals:
                self._report_end_time = last_signals

        # signal times
        self._signals_list_times = [self._signals_start_time + t * self._signals_time_step_min for t in xrange(self._signals_n_steps)]
        last_time_signals = self._signals_list_times[-1]

        self._reported_times = list()
        rt = self.start_time
        while rt<=last_time_signals:
            if rt<=self._report_end_time:
                self._reported_times.append(rt)
            rt += self._report_step_min
            
        self._reported_times_set = set(self._reported_times)
            
        if self._reported_times[-1]>last_time_signals:
            del self._reported_times[-1]

        self.times = copy.deepcopy(self._reported_times)

        self.n_time_steps = len(self.times)
        self.node_to_metric_map = None

    def get_node_map_idx_to_name(self):
        return copy.deepcopy(self.idx_to_node_name)

    def get_n_time_steps(self):
        return copy.copy(self.n_time_steps)

    def get_time_step_duration_min(self):
        return copy.copy(self.time_step_min)

    def get_inpfiles(self):
        all_inpfiles = [os.path.join(self.inputs_dir, filename) for filename in os.listdir(
            self.inputs_dir) if filename.endswith(INP_FORMAT)]
        return all_inpfiles

    def get_tsgfiles(self):
        all_tsgfiles = [os.path.join(self.inputs_dir, filename) for filename in os.listdir(
            self.inputs_dir) if filename.endswith(TSG_FORMAT)]
        return all_tsgfiles
    
    def get_signals_ids(self):
        signals_ids = list()
        for filename in os.listdir(self.signals_dir):
            if filename.endswith(SIGNALS_FORMAT):
                name = os.path.basename(filename)
                signals_id = os.path.splitext(name)[0]
                signals_ids.append(int(signals_id))
        return signals_ids

    def read_info_file(self, filename):
        info_file = filename
        f = open(info_file, 'r')
        try:
            header = f.readline()
            if header not in EMPTY_LINE:
                l = header.split()
                if len(l) == 2:
                    self._signals_n_steps = int(l[0])
                    self._signals_time_step_min = int(l[1])
                    self._signals_start_time = 0
                elif len(l) == 3:
                    self._signals_n_steps = int(l[0])
                    self._signals_start_time = int(l[1])
                    self._signals_time_step_min = int(l[2])
                else:
                    raise RuntimeError(
                        'The header line of infonet file does not follow the standard. It should have two entries. First the number of time steps and sceond the time step in mniutes')
            else:
                raise RuntimeError(
                    'The header line of infonet file does not follow the standard. It should have two entries. First the number of time steps and sceond the time step in mniutes')

            for line in f:
                if line not in EMPTY_LINE:
                    l = line.split()
                    key = int(l[0])
                    value = l[1]
                    found = self.idx_to_node_name.has_key(key)
                    if not found:
                        self.idx_to_node_name[key] = value
                        self.node_name_to_idx[value] = key
                    else:
                        raise RuntimeError(
                            'Index {} already in map'.format(key))
            f.close()
        except IndexError:
            print "ERROR wrong {} format".format(INFONET_FORMAT)

    def read_measurement_file(self, filename):

        measurements = dict()
        measurements['positive'] = dict()
        measurements['negative'] = dict()
        positive_times = set()
        negative_times = set()
        try:
            f = open(filename, 'r')
            for line in f:
                if line not in EMPTY_LINE:
                    l = line.split()
                    n = self.node_name_to_idx[l[0]]
                    t = int(int(l[1]) / 60.0)
                    meas = int(l[2])
                    if t in self._reported_times_set:
                        if meas:
                            if t in positive_times:
                                measurements['positive'][t].add(n)
                            else:
                                measurements['positive'][t] = set()
                                measurements['positive'][t].add(n)
                            positive_times.add(t)
                        else:
                            if t in negative_times:
                                measurements['negative'][t].add(n)
                            else:
                                measurements['negative'][t] = set()
                                measurements['negative'][t].add(n)
                            negative_times.add(t)
                    else:
                        print "WARNING: Ignored measurement at node ", n," at time ", t, " since time is not in simulated data"
            return measurements
        except IOError:
            print "ERROR reading measurement file"

    def read_node_to_metric_map(self,filename):
        self.node_to_metric_map = dict()
        with open(filename,'r') as f:
            for line in f:
                if line not in EMPTY_LINE and '#' not in line:
                    l = line.split()
                    nid = self.node_name_to_idx[l[0]]
                    self.node_to_metric_map[nid] = float(l[1])
        
    def _get_bytes_map_from_idx(self,sid):
        index_filename = os.path.join(
            self.signals_dir, str(sid) + SIGNALS_IDX_FORMAT)
        have_idx = False
        cid_to_bytes = dict()
        cid_to_injection = dict()
        idx_file = open(index_filename, 'r')
        try:
            header = idx_file.readline()
            if header not in EMPTY_LINE:
                have_idx = not int(header)
                if have_idx:
                    for line in idx_file:
                        if line not in EMPTY_LINE:
                            l = line.split()
                            key = int(l[0])
                            value = int(l[1])
                            cid_to_bytes[key] = value
            else:
                print "TODO WARNING first line should not be empty..asume no index"
        except IndexError:
            logger.error('Wrong format in index file {} {}'.format(index_filename,message))
            raise IndexError, "Wrong format in index file"
        finally:
            idx_file.close()
        return cid_to_bytes


    def read_signals_file_kernel(self, sid, contaminations_ids, scenarios_container,flattened=False,message=''):

        index_filename = os.path.join(
            self.signals_dir, str(sid) + SIGNALS_IDX_FORMAT)
        signals_filename = os.path.join(
            self.signals_dir, str(sid) + SIGNALS_FORMAT)
        basename=os.path.basename(signals_filename)
        have_idx = False
        cid_to_bytes = dict()
        cid_to_injection = dict()
        idx_file = open(index_filename, 'r')
        try:
            header = idx_file.readline()
            if header not in EMPTY_LINE:
                have_idx = not int(header)
                for line in idx_file:
                    if line not in EMPTY_LINE:
                        l = line.split()
                        key = int(l[0])
                        value = int(l[1])
                        cid_to_bytes[key] = value
                        source_location = int(l[3])
                        type_inj = l[4]
                        strength = float(l[5])
                        start = int(l[6])
                        end = int(l[7])
                        injection = Injection(
                            source_location, type_inj, strength, start, end)
                        cid_to_injection[key] = injection
            else:
                print "TODO WARNING first line should not be empty..asume no index"
        except IndexError:
            logger.error('Wrong format in index file {} {}'.format(index_filename,message))
            raise IndexError, "Wrong format in index file"
        finally:
            idx_file.close()
            
        # signal times
        n_steps = len(self._signals_list_times)
        last_time_signals = self._signals_list_times[-1]
        
        # reported times
        report_list_times = self._reported_times
        
        last_time_reported = report_list_times[-1]
        tmp_scenario = Scenario(int(sid),-1, report_list_times)

        # start reading
        signals_file = open(signals_filename, 'r')
        if not have_idx:
            count_lines = 0
            for line in signals_file:
                if line not in EMPTY_LINE:
                    l = line.split()
                    if count_lines % (n_steps + 1) == 0:
                        cid = int(l[0])
                        tmp_scenario.clear()
                        tmp_scenario.hid = int(sid)
                        tmp_scenario.cid = cid
                        injection = cid_to_injection.get(cid)
                        if injection is None:
                            raise RuntimeError(
                                'No contamination found for cid {0} check idx file {1}'.format(cid, index_filename))
                        tmp_scenario.injection = injection
                    else:
                        if cid in contaminations_ids:
                            t = int(l[0])
                            ts = int(t / self._signals_time_step_min)
                            if t in self._reported_times_set:
                                for j in xrange(1, len(l)):
                                    tmp_scenario.time_to_nodes[t].add(int(l[j]))
                            if t == last_time_signals:
                                scenarios_container.append(copy.deepcopy(tmp_scenario))
                    count_lines += 1
        else:
            for cid in contaminations_ids:
                off_set = cid_to_bytes.get(cid)
                if off_set is not None:
                    signals_file.seek(off_set)
                    line = signals_file.readline()
                    injection = cid_to_injection.get(cid)
                    if injection is None:
                        raise RuntimeError(
                            'No contamination found for cid {0} check idx file{1}'.format(cid, index_filename))
                    if not flattened:
                        S = Scenario(int(sid),cid, report_list_times,injection=injection)
                        for j in xrange(n_steps):
                            line = signals_file.readline()
                            l = line.split()
                            t = int(l[0])
                            ts = j
                            if t in self._reported_times_set:
                                for k in xrange(1, len(l)):
                                    S.time_to_nodes[t].add(int(l[k]))
                        scenarios_container.append(S)
                    else:
                        list_data = []
                        list_pointers = []
                        for j in xrange(n_steps):
                            line = signals_file.readline()
                            l = line.split()
                            t = int(l[0])
                            ts = j
                            if t in self._reported_times_set:
                                list_pointers.append(len(list_data))
                                for k in xrange(1, len(l)):
                                    list_data.append(int(l[k]))
                        array_pointers = np.array(list_pointers,dtype=np.uint32)
                        array_data = np.array(list_data,dtype=np.uint32)
                        S = Scenario(int(sid),cid, report_list_times,
                                     list_data=array_data,
                                     list_pointers=array_pointers,
                                     injection=injection)
                        scenarios_container.append(S)
                else:
                    raise RuntimeError(
                        'No offset found for cid {} check {} {}'.format(cid, index_filename,message))
        

        logger.debug('RANK {} read file {} {}'.format(
            rank, basename, message))
        signals_file.close()
    
    def read_signals_files(self, dict_scenarios, parallel=False, root=0,message=''):
        if not parallel:
            scenarios_container = list()
            for hid, cids in dict_scenarios.iteritems():
                self.read_signals_file_kernel(
                    hid, cids, scenarios_container,message=message)

            # sets initial probability as uniform and checks for tanks and
            # reservoir contaminations
            if len(scenarios_container)==0:
                raise RuntimeError('The number of scenarios that match the measurements is zero')

            prob = 1 / float(len(scenarios_container))
            for s in scenarios_container:
                s.probability = prob
                start_time = int(s.injection.start_sec / 60)
                stop_time = int(s.injection.end_sec / 60)
                source = s.injection.source
                for t, nodes in s.time_to_nodes.iteritems():
                    if t >= start_time and t <= stop_time:
                        if source not in nodes:
                            s.time_to_nodes[t].add(source)
        else:
            n_hids = len(dict_scenarios)
            r_start, r_end = distributeN(comm, n_hids)
            local_scenario_container = list()
            r_keys = sorted([k for k in dict_scenarios.keys()])

            sub_list = [r_keys[j] for j in xrange(r_start, r_end)]
            for hid in sub_list:
                cids = dict_scenarios[hid]
                self.read_signals_file_kernel(hid, cids, local_scenario_container,
                                              flattened=True,message=message)
            
            if r_start<r_end:
                logger.debug('RANK {} done reading file [{}-{}]'.format(rank,r_start,r_end))

            local_hids = np.array([s.hid for s in local_scenario_container],dtype=np.uint32)
            local_cids = np.array([s.cid for s in local_scenario_container],dtype=np.uint32)
            n_scenarios = len(local_hids)

            global_count_scenarios = comm.allgather(n_scenarios)
            
            N_scenarios = sum(global_count_scenarios)
            counts = global_count_scenarios
            disps = [sum([counts[j] for j in xrange(i)]) for i in xrange(size)]
            
            # send all hydaulic ids
            all_hids = np.zeros(N_scenarios,dtype=np.uint32)
            comm.Gatherv([local_hids,MPI.UINT32_T],[all_hids,counts,disps,MPI.UINT32_T],root=0)
            if r_start<r_end:
                logger.debug('RANK {} done passing hids [{}-{}]'.format(rank,r_start,r_end))

            # send all contamination ids
            all_cids = np.zeros(N_scenarios,dtype=np.uint32)
            comm.Gatherv([local_cids,MPI.UINT32_T],[all_cids,counts,disps,MPI.UINT32_T],root=0)
            if r_start<r_end:
                logger.debug('RANK {} done passing cids [{}-{}]'.format(rank,r_start,r_end))

            # send flattened injections
            local_injections = [s._injection for s in local_scenario_container]
            all_injections = comm.gather(local_injections,root=0)
            if r_start<r_end:
                logger.debug('RANK {} done passing injections [{}-{}]'.format(rank,r_start,r_end))

            processor_int = 1
            scenario_int = 0

            # flatten processor pointers
            processor_pointers_to_spointers = np.zeros(n_scenarios,dtype=np.uint32)
            list_processor_spointers = []
            processor_pointers_to_sdata = np.zeros(n_scenarios,dtype=np.uint32)
            list_processor_sdata = []
            accum_pointers = 0
            accum_data = 0
            for i,s in enumerate(local_scenario_container):
                # get scenario flattened arrays
                scenario_pointers,scenario_data = s.get_flattened_arrays()
                
                #if rank==processor_int and i==scenario_int:
                #    print "POINTERS",list(scenario_pointers),"\n"
                #    print "DATA",list(scenario_data)
                
                #flattened processor pointers
                processor_pointers_to_spointers[i] = accum_pointers
                for p in scenario_pointers:
                    list_processor_spointers.append(p)
                accum_pointers+=len(scenario_pointers)
                #flattened processor data
                processor_pointers_to_sdata[i] = accum_data
                for d in scenario_data:
                    list_processor_sdata.append(d)
                accum_data+=len(scenario_data)

            # send processor pointers
            all_processor_pointers_to_spointers = comm.gather(processor_pointers_to_spointers,root=0)
            all_processor_pointers_to_sdata = comm.gather(processor_pointers_to_sdata,root=0)

            processor_spointers = np.array(list_processor_spointers,dtype=np.uint32)
            n_pointers = accum_pointers
            global_count_pointers = comm.allgather(n_pointers)
            N_pointers = sum(global_count_pointers)
            counts = global_count_pointers
            disps = [sum([counts[j] for j in xrange(i)]) for i in xrange(size)]
            
            # send all pointers
            all_pointers = np.zeros(N_pointers,dtype=np.uint32)
            comm.Gatherv([processor_spointers,MPI.UINT32_T],[all_pointers,counts,disps,MPI.UINT32_T],root=0)
            if r_start<r_end:
                logger.debug('RANK {} done passing pointers [{}-{}]'.format(rank,r_start,r_end))

            processor_sdata = np.array(list_processor_sdata,dtype=np.uint32)
            n_data = accum_data
            global_count_data = comm.allgather(n_data)
            N_data = sum(global_count_data)
            counts = global_count_data
            disps = [sum([counts[j] for j in xrange(i)]) for i in xrange(size)]
        
            # send all data
            all_data = np.zeros(N_data,dtype=np.uint32)
            comm.Gatherv([processor_sdata,MPI.UINT32_T],[all_data,counts,disps,MPI.UINT32_T],root=0)
            if r_start<r_end:
                logger.debug('RANK {} done passing data [{}-{}]'.format(rank,r_start,r_end))
                
            scenarios_container = []
            if rank==root:
                """
                print all_hids
                print all_cids
                print global_count_scenarios
                print global_count_pointers
                print global_count_data
                print all_processor_pointers_to_spointers
                print all_processor_pointers_to_sdata
                """

                # signal times
                signals_list_times = self._signals_list_times
                last_time_signals = signals_list_times[-1]

                # reported times
                report_list_times = self._reported_times

                # build scenarios from data
                offset = 0
                sorted_times = sorted(report_list_times)
                offset_pointers = 0
                offset_data = 0
                offset_scenarios = 0
                count_build = 0
                for processor_i,n_scenarios in enumerate(global_count_scenarios):
                    # get injections
                    injections = all_injections[processor_i]
                    # get scenario pointers
                    processor_pointers_to_spointers = all_processor_pointers_to_spointers[processor_i]
                    processor_pointers_to_sdata = all_processor_pointers_to_sdata[processor_i]
                    for j in xrange(n_scenarios):
                        # getting hydraulic id
                        hid = all_hids[offset_scenarios+j]
                        # getting contamination id
                        cid = all_cids[offset_scenarios+j]
                        # getting back pointers of individual scenarios
                        processor_pointers_pointers = all_processor_pointers_to_spointers[processor_i]
                        start = offset_pointers+processor_pointers_to_spointers[j]
                        if j<n_scenarios-1:
                            end = offset_pointers+processor_pointers_to_spointers[j+1]
                        else:
                            end = offset_pointers+global_count_pointers[processor_i]
                        #scenario_pointers=[]
                        # this could be done with slice from numpy
                        #for k in xrange(start,end):
                        #    scenario_pointers.append(all_pointers[k])
                        
                        indices = np.arange(start,end,dtype=np.uint32)
                        scenario_pointers = np.take(all_pointers,indices)

                        # getting back data of individual scenarios
                        processor_pointers_to_sdata = all_processor_pointers_to_sdata[processor_i]
                        start = offset_data+processor_pointers_to_sdata[j]
                        if j<n_scenarios-1:
                            end = offset_data+processor_pointers_to_sdata[j+1]
                        else:
                            end = offset_data+global_count_data[processor_i]
                        #scenario_data=[]
                        # this could be done with slice from numpy
                        #for k in xrange(start,end):
                        #    scenario_data.append(all_data[k])
                        
                        indices = np.arange(start,end,dtype=np.uint32)
                        scenario_data = np.take(all_data,indices)
                        
                        #if processor_i==processor_int and j==scenario_int:
                        #    print "POINTERS",list(scenario_pointers),"\n"
                        #    print "DATA",list(scenario_data)

                        # Add scenario
                        scenario = Scenario(hid,cid,sorted_times,injection=injections[j])
                        #scenario.build_contaminations_from_flat_lists(sorted_times,scenario_pointers,scenario_data)
                        scenario.build_contaminations_from_flat_arrays(sorted_times,scenario_pointers,scenario_data)
                        scenarios_container.append(scenario)
                        count_build+=1
                        logger.debug('RANK {} done building scenario {} of {}'.format(rank,count_build,N_scenarios))
                    offset_pointers+=global_count_pointers[processor_i]
                    offset_data+=global_count_data[processor_i]
                    offset_scenarios+=global_count_scenarios[processor_i]
                
            #l_container = comm.gather(local_scenario_container,root=root)
            #scenarios_container = []
            #if rank == root:
            #    for l in l_container:
            #        for s in l:                        
            #            scenarios_container.append(s)
                        
                if len(scenarios_container)==0:
                    raise RuntimeError('The number of scenarios that match the measurements is zero')

                prob = 1.0 / float(len(scenarios_container))
                for s in scenarios_container:
                    s.probability = prob
                    start_time = int(s.injection.start_sec / 60)
                    stop_time = int(s.injection.end_sec / 60)
                    source = s.injection.source
                    for t, nodes in s.time_to_nodes.iteritems():
                        if t >= start_time and t <= stop_time:
                            if source not in nodes:
                                s.time_to_nodes[t].add(source)
        #for s in scenarios_container:
        #    s.pprint()
        return scenarios_container

    def _list_scenario_ids_kernel(self, hydraulic_id, measurements,message=''):
        n_timesteps = self._signals_n_steps
        signals_filename = os.path.join(self.signals_dir,
                                        str(hydraulic_id) + SIGNALS_FORMAT)
        basename=os.path.basename(signals_filename)
        signals_file = open(signals_filename, 'r')
        cids = set()
        cid_to_bytes = self._get_bytes_map_from_idx(hydraulic_id)
        n_steps = self._signals_n_steps
        if len(cid_to_bytes)!=0:
            for cid,offset in cid_to_bytes.iteritems():
                if offset is not None:
                    signals_file.seek(offset)
                    line = signals_file.readline()
                    for j in xrange(n_steps):
                        line = signals_file.readline()
                        l = line.split()
                        t = int(l[0])
                        contaminated_nodes = measurements['positive'].get(t)
                        found_match = False
                        if contaminated_nodes is not None:
                            for k in xrange(1, len(l)):
                                nid = int(l[k])
                                if nid in contaminated_nodes:
                                    found_match = True
                                    break
                        if found_match:
                            cids.add(cid)
                            break
                            
        else:
            count_lines = 0
            cid = -1
            for line in signals_file:
                if line not in EMPTY_LINE:
                    l = line.split()
                    if count_lines % (n_timesteps + 1) == 0:
                        cid = int(l[0])
                        added = False
                    else:
                        if not added:
                            t = int(l[0])
                            n_nodes_s = len(l)
                            discarded = False
                            contaminated_nodes = measurements['positive'].get(t)
                            if contaminated_nodes is not None:
                                num_nodes_covereage = 1
                                count_nodes = 0
                                for j in xrange(1, n_nodes_s):
                                    node = int(l[j])
                                    if node in contaminated_nodes:
                                        count_nodes += 1
                                        if count_nodes == num_nodes_covereage:
                                            cids.add(cid)
                                            added = True
                                            break
                    count_lines += 1
        logger.debug('RANK {} done listing file {} {}'.format(rank, basename, message))
        signals_file.close()
        return cids

    def _list_all_scenarios_kernell(self, hydraulic_id,message=''):

        index_filename = os.path.join(self.signals_dir,
                                      str(hydraulic_id) + SIGNALS_IDX_FORMAT)
        basename = os.path.basename(index_filename)
        cids = list()
        idx_file = open(index_filename, 'r')
        try:
            header = idx_file.readline()
            if header not in EMPTY_LINE:
                have_idx = not int(header)
                counter = 0
                for line in idx_file:
                    if line not in EMPTY_LINE:
                        cids.append(counter)
                        counter += 1
            else:
                print "TODO WARNING first line should not be empty..asume no index"
        except IndexError:
            print "TODO ERROR wrong format in index file"
            logger.error('RANK {} failed listing file {} {}'.format(rank, basename,message))
        finally:
            idx_file.close()
            logger.debug('RANK {} done listing file {} {}'.format(rank, basename,message))

        return cids

    def list_scenario_ids(self, signals_ids = None, measurements=None, parallel=False,message=''):

        time_to_contaminated_nodes = measurements
        if signals_ids is None:
            list_signal_ids = self.get_signals_ids()
        else:
            list_signal_ids = signals_ids
        N_scenarios = len(list_signal_ids)
        if parallel:
            #setup_mpi_logger(size, host, rank)
            r_start, r_end = distributeN(comm, N_scenarios)
        else:
            r_start = 0
            r_end = N_scenarios

        local_hids = [list_signal_ids[j]
                      for j in xrange(r_start, r_end)]

        local_list_scenarios = list()
        for hid in local_hids:
            if time_to_contaminated_nodes is not None:
                if time_to_contaminated_nodes['positive']:
                    cids = self._list_scenario_ids_kernel(
                        hid, time_to_contaminated_nodes,message=message)
                else:
                    cids = self._list_all_scenarios_kernell(hid,message=message)
                if len(cids)>0:
                    local_list_scenarios.append((hid, cids))
            else:
                cids = self._list_all_scenarios_kernell(hid,message=message)
                local_list_scenarios.append((hid, cids))

        if parallel:
            list_scenarios = list()
            tmp_list = comm.allgather(local_list_scenarios)
            for l in tmp_list:
                for ll in l:
                    list_scenarios.append(ll)
        else:
            list_scenarios = local_list_scenarios

        return list_scenarios

    def read_list_ids_file(self,filename):
        with open(LIST_IDS_FILE,'r') as f:
            hid_to_cids = dict()
            for line in f:
                if line not in EMPTY_LINE:
                    l = line.split()
                    hid = int(l[0])
                    cid = int(l[1])
                    if hid_to_cids.has_key(hid):
                        hid_to_cids[hid].add(cid)
                    else:
                        hid_to_cids[hid] = set()
                        hid_to_cids[hid].add(cid)
        ids = list()
        for hid,cids in hid_to_cids.iteritems():
            ids.append((hid,list(cids)))
        return ids

def validate_data_folder(folder_name):
    # check structure of scenario folder
    folder_contents = os.listdir(folder_name)
    if SIGNALS_DIR not in folder_contents: 
        raise RuntimeError('Data folder needs to contain a {} folder'.format(SIGNALS_DIR))
    if INP_MAP_FILE not in folder_contents:
        raise RuntimeError('Data folder needs to contain a {} file'.format(INP_MAP_FILE))
    
    if rank==0:
        signals_dir = os.path.join(folder_name,SIGNALS_DIR)
        all_infofiles = [os.path.join(signals_dir, filename) for filename in os.listdir(
            signals_dir) if filename.endswith(INFONET_FORMAT)]
        infonet_file = all_infofiles[0]  # all should have the same
        if len(all_infofiles)==0:
            raise RuntimeError('file {} not found in {}.'.format(INFONET_FORMAT,signals_dir))
        #TODO: add check that all infonet files have the same network features


def write_tsg(filename, scenarios,node_id_to_name):
    with open(filename,'w') as f:
        for s in scenarios:
            f.write(s.injection.tsg_line(node_id_to_name))

def write_list_ids_file(list_scenarios):
    with open(LIST_IDS_FILE,'w') as f:
        for s in list_scenarios:
            f.write("{} {}\n".format(s.hid,s.cid))
            
