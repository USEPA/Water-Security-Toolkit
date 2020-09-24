#  _________________________________________________________________________
#
#  Water Security Toolkit (WST)
#  Copyright (c) 2012 Sandia Corporation.
#  This software is distributed under the Revised BSD License.
#  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive 
#  license for use of this work by or on behalf of the U.S. government.
#  For more information, see the License Notice in the WST User Manual.
#  _________________________________________________________________________
#
import datetime
import os
import sys
from pyutilib.misc.config import ConfigValue, ConfigBlock, ConfigList
import pyutilib.services

try:
    import yaml
    using_yaml=True
except ImportError:
    using_yaml=False

none_list = ['','none','None','NONE', None, [], {}]

#valid_metrics = [ 'DEC', 'DMC', 'DPD', 'DPE', 'DPK', 'DTD', 'DVC', 'EC', 'MC', 'NFD', 'PD', 'PE', 'PK', 'TD', 'TEC', 'VC' ]
valid_metrics = [ 'EC', 'MC', 'NFD', 'PD', 'PE', 'PK', 'TD', 'VC' ] # removed TEC

USER_OPTION = 0
ADVANCED_OPTION = 1
DEVELOPER_OPTION = 2

def scalar_or_listOf(stype, data):
    if hasattr(data, "__iter__") and not isinstance(data, stype):
        return [ stype(i) for i in data ]
    else:
        return [ stype(data) ]

def str_or_listOfStr(data):
    #return scalar_or_listOf(str, x)
    if hasattr(data, "__iter__") and not isinstance(data, basestring):
        return [ str(i) for i in data ]
    else:
        return [ str(data) ]

def find_executable(exe):
    pyutilib.services.register_executable(exe)
    _exe = pyutilib.services.registered_executable(exe)
    if _exe is None:
        return None
    else:
        return _exe.get_path()

class Path(object):
    BasePath = None
    SuppressPathExpansion = False

    def __init__(self, basePath=None, preserveTrailingSep=False):
        self.basePath = basePath
        self.trailingSep = preserveTrailingSep

    def __call__(self, path):
        #print "normalizing path '%s' " % (path,),
        if path is None or Path.SuppressPathExpansion:
            return path 

        if self.basePath:
            base = self.basePath
        else:
            base = Path.BasePath
        if type(base) is ConfigValue:
            base = base.value()
        if base is None:
            base = ""
        else:
            base = str(base).lstrip()

        # We want to handle the CWD variable ourselves.  It should
        # always be in a known location (the beginning of the string)
        if len(path)>=6 and path[:6].lower() == '${cwd}':
            path = os.getcwd() + path[6:]

        ans = os.path.normpath(os.path.abspath(os.path.join(
            base, os.path.expandvars(os.path.expanduser(path)))))
        if self.trailingSep and path.endswith(os.path.sep):
            ans += os.path.sep
        #print "to '%s'" % (ans,)
        return ans

class PathList(Path):
    def __call__(self, data):
        if hasattr(data, "__iter__") and not isinstance(data, basestring):
            return [ super(PathList, self).__call__(i) for i in data ]
        else:
            return [ super(PathList, self).__call__(data) ]
        

def master_config():

    config = ConfigBlock("Master config file for wst" )

    config.declare('__config_file_location__', ConfigValue(
        '', Path(),
        'DEVELOPER OPTION',
        """The directory where the YAML configuration file lives""",
        visibility=DEVELOPER_OPTION ) )

    # network block
    network = config.declare('network', ConfigBlock())
    network.declare( 'epanet file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'EPANET network file name',
                """ The name of the EPANET input (INP) file that defines the water distribution
                network model.
                
                Required input.""" ) )
    network.declare( 'water quality timestep', ConfigValue(
                'INP', None,
                'DEVELOPER OPTION',
                """The water quality time step (min) will override the value in the
                EPANET input file unless the value is set to null. If the EPANET
                time step is not in minutes, the value is converted. This parameter will
                be ignored when using a Merlion water quality model wqm file.
                Default = INP.""",
                visibility=DEVELOPER_OPTION ) )
    network.declare( 'simulation duration', ConfigValue(
                'INP', None,
                'DEVELOPER OPTION',
                """The simulation duration (min) will override the value in the
                EPANET input file unless the value is set to null. If the EPANET
                time step is not in minutes, the value is converted. This parameter will
                be ignored when using a Merlion water quality model wqm file.
                Default = INP.""",
                visibility=DEVELOPER_OPTION ) )

    scenario = config.declare('scenario', ConfigBlock())
    scenario.declare( 'location', ConfigValue(
                None, str_or_listOfStr,
                'Injection location: ALL, NZD or EPANET ID',
                """A list that describes the injection locations for the contamination scenarios.
                The options are: (1) ALL, which denotes all nodes (excluding tanks and reservoirs)
                as contamination injection locations; (2) NZD, which denotes all nodes with
                non-zero demands as contamination injection locations; or (3) an EPANET node ID, 
                which identifies a node as the contamination injection location. This allows 
                for an easy specification of single or multiple contamination scenarios.
                
                Required input unless a TSG or TSI file is specified.""" ) )
    scenario.declare( 'type', ConfigValue(
                None, str,
                'Injection type: MASS, CONCEN, FLOWPACED, or SETPOINT',
                """The injection type for the contamination scenarios. The options are MASS, CONCEN, FLOWPACED or SETPOINT. 
                See the EPANET manual for additional information about source types \\citep{EPANETusermanual}.
                
                Required input unless a TSG or TSI file is specified.""" ) )
    scenario.declare( 'strength', ConfigValue(
                None, float,
                'Injection strength [mg/min or mg/L depending on type]',
                """The amount of contaminant injected into the network for the contamination scenarios.  
                If the type option is MASS, then the units for the strength are in mg/min. 
                If the type option is CONCEN, FLOWPACED or SETPOINT, then units are in mg/L.
                
                Required input unless a TSG or TSI file is specified.""" ) )
    scenario.declare( 'species', ConfigValue(
                None, str,
                'Injection species, required for EPANET-MSX',
                """The name of the contaminant species injected into the network. This is the name of a single species. 
                It is required when using EPANET-MSX, since multiple species might be simulated, but
                only one is injected into the network. For cases where multiple contaminants are injected,
                a TSI file must be used.
                
                Required input for EPANET-MSX unless a TSG or TSI file is specified.""" ) )
    scenario.declare( 'start time', ConfigValue(
                None, int,
                'Injection start time [min]',
                """The injection start time that defines when the contaminant injection begins. 
                The time is given in minutes and is measured from the start of the simulation. 
                For example, a value of 60 represents an injection that starts at hour 1 of the simulation.
                
                Required input unless a TSG or TSI file is specified.""" ) )
    scenario.declare( 'end time', ConfigValue(
                None, int,
                'Injection end time [min]',
                """The injection end time that defines when the contaminant injection stops.				
                The time is given in minutes and is measured from the start of the simulation.
                For example, a value of 120 represents an injection that ends at hour 2 of the simulation.
                
                Required input unless a TSG or TSI file is specified.""" ) )
    scenario.declare( 'tsg file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'TSG file name, overrides injection parameters above',
                """The name of the TSG scenario file that defines the ensemble of contamination
                scenarios to be simulated. Specifying a TSG file will
                override the location, type, strength, species, start and end times options specified in
                the WST configuration file. The TSG file format is documented in File Formats Section \\ref{formats_tsgFile}.
                
                Optional input.""" ) )
    scenario.declare( 'tsi file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'TSI file name, overrides TSG file',
                """The name of the TSI scenario file that defines the ensemble of contamination
                scenarios to be simulated. Specifying a TSI file will
                override the TSG file, as well as the location, type, strength, species, start and end time options specified in
                the WST configuration file. The TSI file format is documented in File Formats Section \\ref{formats_tsiFile}.
                
                Optional input.""" ) )
    scenario.declare( 'signals', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'Signal files, overrides TSG or tsi files',
                """Name of file or directory with information to generate 
                or load signals. If a file is provided the list of inp tsg tuples
                 will be simulated and the information stored in signals files. If
                a directory with the signals files is specified, the signal files will
                be read and loaded in memory. The idea of loading signals is to avoid 
                rerunning simulation of scenarios if they have been run already. 
                This input is only valid for the uq, subcommand, and the grabsample 
                subcommand with probability based formulations.

                Required input.""") )
    
    scenario.declare( 'scn file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'DEVELOPER OPTION',
                """Similar to the TSG file, this file gives more
                flexibility by providing the ability to define each
                injection as a time pattern. Currently used only by the
                manual sampling module.""",
                visibility=DEVELOPER_OPTION ) )
    scenario.declare( 'dvf file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'DEVELOPER OPTION',
                """The DVF decision variable file that allows the user to simulate
                various operational controls. For the \code{tevasim} subcommand, 
                the DVF file adds a flushing rate to demands at specified nodes, 
                and/or closes specified pipes. The DVF file format is documented 
                in Section \\ref{formats_dvfFile}.""",
                visibility=DEVELOPER_OPTION ) )
    scenario.declare( 'msx file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'Multi-species extension file name',
                """The name of the EPANET-MSX multi-species file that defines the multi-species reactions to
                be simulated using EPANET-MSX.
                
                Required input for EPANET-MSX.""" ) )
    scenario.declare( 'msx species', ConfigValue(
                None, str,
                'MSX species to save',
                """The name of the MSX species whose concentration profile will be saved by the EPANET-MSX simulation
                and used for later calculations.
                
                Required input for EPANET-MSX.""" ) )
    scenario.declare( 'merlion', ConfigValue(
                False, bool,
                'Use Merlion as WQ simulator, true or false',
                """A flag to indicate if the Merlion water quality
                simulator should be used. The options are true or false. 
                If an MSX file is provided, EPANET-MSX will be used.
                
                Required input, default = false.""" ) )
    scenario.declare( 'erd compression', ConfigValue(
                'LZMA', str,
                'DEVELOPER OPTION',
                """The compression option for the output ERD database.
                Options = RLE or LZMA. RLE compression is recommended
                when using Merlion. 
                Default = LZMA.""",
                visibility=DEVELOPER_OPTION ) )
    scenario.declare( 'merlion nsims', ConfigValue(
                100, int,
                'DEVELOPER OPTION',
                """**Developer option, Recommended merlion nsims = number 
                of scenarios""",
                visibility=DEVELOPER_OPTION ) )
    scenario.declare( 'ignore merlion warnings', ConfigValue(
                True, bool,
                'DEVELOPER OPTION',
                visibility=DEVELOPER_OPTION ) )

    # impact block
    impact = config.declare('impact', ConfigBlock())
    impact.declare( 'erd file', ConfigValue(
                None, PathList(config.get('__config_file_location__')),
                'ERD database file name',
                """The name of the ERD database file that contains the 
                contaminant transport simulation results. It is 
                created by running the \code{tevasim} subcommand.
                Multiple ERD files (entered as a list, i.e. [<file1>, <file2>]) can be combined to
                generate a single impact file. This can be used to combine
                simulation results from different types of contaminants, in
                which the ERD files were generated from different
                TSG files.
                
                Required input.""" ) )
    impact.declare( 'metric', ConfigValue(
                None, str_or_listOfStr,
                'Impact metric',
                """The impact metric used to compute the impact file. Options
                include EC, MC, NFD, PD, PE, PK, TD, or VC. One impact file 
                is created for each metric selected. These metrics are 
                defined in Section \\ref{impact_measures}.
                
                Required input.""") )
    impact.declare( 'tai file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'Health impact file name, required for public health metrics',
                """The name of the TAI file that contains health impact information. 
                The TAI file format is documented in File Formats Section \\ref{formats_taiFile}.
                
                Required input if a public health metric is used (PD, PE or PK).""") )
    impact.declare( 'dvf file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'DEVELOPER OPTION',
                """The DVF decision variable file name that allows the user to simulate
                various operational controls. These controls include flushing
                at nodes and/or closing pipes. For the \code{sim2Impact} subcommand, the DVF option 
                specifies that the demands added due to flushing must be subtracted 
                out prior to calculating the impact metrics. This ensures that 
                flushed water is not consumed, and therefore not included in 
                impact metrics related to demand. The DVF file format is documented in 
                Section \\ref{formats_dvfFile}.""",
                visibility=DEVELOPER_OPTION) )
    impact.declare( 'response time', ConfigValue(
                0, int,
                'Time [min] needed to respond',
                """The number of minutes that are needed to respond to the
                detection of a contaminant. This represents the time that it takes
                a water utility to stop the spread of the contaminant in the network and 
                eliminate the consumption of contaminated water. As the response time increases,
                the impact increases because the contaminant affects the network
                for a greater length of time.  
                
                Required input, default = 0 minutes.""") )
    impact.declare( 'detection limit', ConfigValue(
                [0], lambda x: scalar_or_listOf(float, x),
                'Thresholds needed to perform detection',
                """The minimum concentration that must be exceeded before a sensor can detect a contaminant.
                There must be one threshold for each ERD file. The units of
                these detection limits depend on the units of the contaminant
                simulated for each ERD file (e.g., number of cells of a
                biological agent).  
                
                Required input, default = 0.""") )
    impact.declare( 'detection confidence', ConfigValue(
                1, int,
                'Number of sensors for detection',
                """The number of sensors that must detect an incident before
                the impacts are calculated.  
                
                Required input, default = 1 sensor.""") )
    impact.declare( 'msx species', ConfigValue(
                None, str,
                'MSX species used to compute impact',
                'MSX species used to compute impact',
                """The name of the MSX species tracked in the EPANET-MSX simulation.
                This parameter is required for multi-species contamination
                incidents created by \code{tevasim} subcommand.
                
                Required input for EPANET-MSX, default = first species listed in the ERD file""") )



    # sp
    impact_data_ = ConfigBlock()
    impact_data = config.declare( 'impact data', ConfigList([], domain=impact_data_) )
    impact_data_.declare( 'name', ConfigValue(
                None, str,
                'Impact block name',
                """The name of the impact block that is used in the objective or constraint block.
                
                Required input.""") )
    impact_data_.declare( 'impact file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'Impact file name',
                """The name of the impact file that is created by \code{sim2Impact} and 
                contains the detection time and the total
                impact given a sensor at that node is the first to detect
                contamination from that scenario. 
                The impact file format is documented in File Formats Section \\ref{formats_impactFile}.
                
                Required input.""") )
    impact_data_.declare( 'nodemap file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'Nodemap file name',
                """The name of the nodemap file that is created by \code{sim2Impact} and 
                maps sensor placement ids to the network node labels. 
                The nodemap file format is documented in File Formats Section \\ref{formats_nodeFile}.
                
                Required input.""") )
    impact_data_.declare( 'weight file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'Weight file name',
                """The name of the weight file that specifies the weights for contamination
                incidents. This file supports the optimization of weighted
                impact metrics. 
                The weight file format is documented in File Formats Section \\ref{formats_weightFile}.
                
                Optional input, by default, incidents are optimized with weight 1.""") )
    #impact_data_.declare( 'directory', ConfigValue(
    #            None, str, #Path(config.get('__config_file_location__')),
    #            'Impact data directory',
    #            """The name of the directory where the impact file, nodemap file and weight file are located.
    #            
    #            Optional input, default = working directory.""") )
    
    impact_data_.declare( 'original impact file', ConfigValue(
                None, None,
                '', '', 
                visibility=DEVELOPER_OPTION ) )
    impact_data_.declare( 'original nodemap file', ConfigValue(
                None, None,
                '', '', 
                visibility=DEVELOPER_OPTION ) )
                
    cost_ = ConfigBlock()
    cost = config.declare( 'cost', ConfigList([], domain=cost_) )
    cost_.declare( 'name', ConfigValue(
                None, str,
                'Cost block name',
                """The name of the cost block that is used in the objective or constraint block.
                
                Optional input.""") )
    cost_.declare( 'cost file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'Cost file name',
                """The name of the cost file that contains the costs for the installation of
                sensors throughout the distribution network. This file contains
                EPANET ID/cost pairs.
                The cost file format is documented in File Formats Section \\ref{formats_costFile}.
                
                Optional input.""") )
    #cost_.declare( 'directory', ConfigValue(
    #            None, str, #Path(config.get('__config_file_location__')),
    #            'Cost data directory',
    #            """The name of the directory where the cost file is located.
    #            
    #            Optional input, default = working directory.""") )

    objective_ = ConfigBlock()
    objective = config.declare( 'objective', ConfigList([], domain=objective_) )
    objective_.declare( 'name', ConfigValue(
                None, str,
                'Objective block name',
                """The name of the objective block that is used in sensor placement block.
                
                Required input.""") )
    objective_.declare( 'goal', ConfigValue(
                None, str,
                'Optimization objective',
                """The objective of the optimization process that defines what is going to minimized. 
                The options are the name of the impact block, the name of the cost block, 
                the number of sensors (NS) or the number of failed detections (NFD).
				
				Required input.""" ) )
    objective_.declare( 'statistic', ConfigValue(
                None, str,
                'Objective statistic',
			    """The objective statistic. The TOTAL
			    statistic is used when the goal is NS
			    or NFD. When the goal is to compute a
			    statistic of an impact block, the options
			    are MEAN, MEDIAN, VAR, TCE, CVAR, TOTAL
			    or WORST. For example, MEAN will minimize
			    the mean impacts over all of the
			    contamination scenarios, while WORST
			    will only minimize the worst impacts
			    from the ensemble of contamination
			    scenarios.

				Required input.""" ) )
    objective_.declare( 'gamma', ConfigValue(
		0.05, float, 'Gamma, required with statistics VAR or CVAR', 
        """The value of gamma that specifies the
		fraction of the distribution of impacts that will
		be used to compute the VAR, CVAR and TCE statistics.
		Gamma is assumed to be in the interval (0,1], which means 
		that gamma can be greater than zero but less than or equal to one. It
		can be interpreted as specifying the 100*gamma
		percent of the worst contamination incidents that
		are used for these calculations.

		Required input for VAR or CVAR objective statistics,
		default = 0.05.""") )

    constraint_ = ConfigBlock()
    constraint = config.declare( 'constraint', ConfigList([], domain=constraint_) )
    constraint_.declare( 'name', ConfigValue(
                None, str,
                'Constraint block name',
                """The name of the constraint block that is used in sensor placement block.
                
                Required input.""") )
    constraint_.declare( 'goal', ConfigValue(
                None, str,
                'Constraint goal',
                """The constraint goal. The options are the name of the impact block name, 
                the name of the cost block, the number of sensors (NS) or the number of failed detections (NFD).
                
                Required input.""" ) )
    constraint_.declare( 'statistic', ConfigValue(
                None, str,
                'Constraint statistic',
                """The constraint statistic. The TOTAL
                statistic is used when the goal is NS
                or NFD. When the goal is to compute a
                statistic of an impact block, the options
                are MEAN, MEDIAN, VAR, TCE, CVAR, TOTAL
                or WORST. For example, MEAN will constrain
                the mean impacts over all of the
                contamination scenarios, while WORST
                will only constrain the worst impacts
                from the ensemble of contamination
                scenarios.

                Required input.""", ) )
    constraint_.declare( 'gamma', ConfigValue(
                0.05, float,
                'Gamma, required with statistics VAR or CVAR',
                """The value of gamma that specifies the fraction of the distribution of impacts that
                will be used to compute the VAR, CVAR and TCE statistics. Gamma
                is assumed to be in the interval (0,1], which means that gamma 
				can be greater than zero but less than or equal to one. It can be interpreted
                as specifying the 100*gamma percent of the worst contamination
                incidents that are used for these calculations.  
                
                Required input for VAR or CVAR objective statistics, default = 0.05.""") )
    constraint_.declare( 'bound', ConfigValue(
                None, None,
                'Constraint upper bound',
                """The upper bound on the constraint.
                
                Optional input.""") )

    constraint_scenario_ = ConfigBlock(visibility=DEVELOPER_OPTION)
    constraint_scenario = constraint_.declare( "scenario", ConfigList([], domain=constraint_scenario_, visibility=DEVELOPER_OPTION) )
    constraint_scenario_.declare( 'name', ConfigValue(
                None, str,
                'sensor placement name',
                "TODO") )
    constraint_scenario_.declare( 'probability', ConfigValue(
                1.0, float,
                'Scenario probability',
                "TODO") )
    constraint_scenario_.declare( 'bound', ConfigValue(
                0, float,
                'Scenario bound',
                "TODO") )

    imperfect_ = ConfigBlock()
    imperfect = config.declare( 'imperfect', ConfigList([], domain=imperfect_) )
    imperfect_.declare( 'name', ConfigValue(
                None, str,
                'Imperfect block name',
                """The name of the imperfect block that is used in sensor placement block.
                
                Optional input.""") )
    imperfect_.declare( 'sensor class file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'Imperfect sensor class file',
                """The name of the imperfect sensor class file that defines the detection probabilities
                for all sensor categories. It is used with the imperfect-sensor model 
                and must be specified in conjunction with a imperfect junction class file.
                The imperfect sensor class file format is documented in File Formats Section \\ref{formats_sensorClass}.
                
                Optional input.""") )
    imperfect_.declare( 'junction class file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'Imperfect junction class file',
                """The name of the imperfect junction class file that defines a sensor category for
                each network node. It is used with the imperfect-sensor model and
                must be specified in conjunction with a imperfect sensor class file.
                The imperfect junction class file format is documented in File Formats Section \\ref{formats_junctionClass}.
                
                Optional input.""") )
    #imperfect_.declare( 'directory', ConfigValue(
    #            None, str, #Path(config.get('__config_file_location__')),
    #            'Imperfect file directory',
    #            """The name of the directory where the imperfect junction class and sensor class files are located.
    #            
    #            Optional input.""") )

    aggregate_ = ConfigBlock()
    aggregate = config.declare( 'aggregate', ConfigList([], domain=aggregate_) )
    aggregate_.declare( 'name', ConfigValue(
                None, str,
                'Aggregation block name',
                """The name of the aggregation block that is used in sensor placement block.
                
                Optional input.""") )
    aggregate_.declare( 'type', ConfigValue(
                None, str,
                'Aggregation type: THRESHOLD, PERCENT or RATIO',
                """The type of aggregation used to reduce the size of the sensor placement problem.  
                The options are THRESHOLD, PERCENT or RATIO.

                THRESHOLD is used to aggregate similar impacts by specifying a goal and a value.  
                This is used to reduce the total size of the sensor placement formulation (for large problems).
                Solutions generated with non-zero thresholds are not guaranteed
                to be globally optimal.

                PERCENT is an alternative method to compute the aggregation threshold 
                in which the value (of the goal-value pair) is a real number between 0.0 and 1.0. 
                Over all contamination incidents, compute the maximum difference, d, between the impact of the
                contamination incident if it is not detected and the impact if it is detected
                at the earliest possible feasible location and set the aggregation threshold to 
                d times the aggregation percent. If both THRESHOLD and PERCENT are set to valid values, 
                then PERCENT takes priority.

                RATIO is also specified with a goal-value pair in which value is a real number between
                0.0 and 1.0.
                
                Optional input.""") )
    aggregate_.declare( 'goal', ConfigValue(
                None, str,
                'Aggregation goal',
                """The aggregation goal for the aggregation type.
                
                Optional input.""") )
    aggregate_.declare( 'value', ConfigValue(
                None, str,
                'Aggregation value',
                """The aggregation value for the aggregation type. If the aggregation type is PERCENT or RATIO,
                then this value is a real number between 0.0 and 1.0.
                
                Optional input.""") )
    aggregate_.declare( 'conserve memory', ConfigValue(
                0, int,
                'Aggregation conserve memory',
                """The maximum number of impact files that should be read into memory 
                at any one time. This option allows impact files to be processed in 
                a memory conserving mode if location aggregation is chosen and the original impact
                files are very large. For example, a conserve memory value of 10000 requests
                that no more than 10000 impacts should be read into memory at any one 
                time while the original impact files are being processed into smaller
                aggregated files.  
				
				Optional input, default = zero to turn off this option.""") )
    aggregate_.declare( 'distinguish detection', ConfigValue(
                0, int,
                'Detection goal',
                """A goal for which aggregation should not allow incidents to
                become trivial. If the aggregation threshold is so large that all
                locations, including the dummy, would form a single superlocation,
                this forces the dummy to be in a superlocation by itself. Thus,
                the sensor placement will distinguish between detecting and not
                detecting. This option can be listed multiple times, to specify
                multiple goals. 
                
                Optional input, default = 0.""") )
    aggregate_.declare( 'disable aggregation', ConfigValue(
                [0], lambda x: scalar_or_listOf(int, x),
                'Aggregation disable aggregation',
                """Disable aggregation for this goal, even at value zero, which
                would incur no error. Each witness incident will be in a separate
                superlocation. This option can be listed multiple times to
                specify multiple goals. ALL can be used to specify
                all goals. 
                
                Optional input, default = 0.""") )

    problem = ConfigBlock()
    config.declare( 'sensor placement', ConfigList([], domain=problem))
    #problem = config.declare( 'sensor placement', ConfigBlock() )
    problem.declare( 'name', ConfigValue(
                None, str,
                'Sensor placement block name',
                """The sensor placement block name.""",
                visibility=DEVELOPER_OPTION ) )
    problem.declare( 'type', ConfigValue(
                'default', str,
                'Sensor placement problem type',
                """The sensor placement problem type. The command \code{wst sp ---help-problems} 
                provides a list of problem types for sensor placement. For example, average-case perfect-sensor
                is the standard problem type for sensor placement, since it uses the mean statistic, 
                zero constraints, single objective and perfect sensors. 
                
                Required option, default = average-case perfect-sensor.""") )
    problem.declare( 'modeling language', ConfigValue(
                'NONE', str,
                'Modeling language: NONE, PYOMO or AMPL, default = NONE',
                """The modeling language to generate the sensor placement optimization 
                problem. The options are NONE, PYOMO or AMPL. 

                Required input, default = NONE.""") )
    problem.declare( 'objective', ConfigValue(
                None, str,
                'Objective block name used in sensor placement',
                """The name of the objective block previously defined to be used in sensor placement.
                
                Required input.""") )
    problem.declare( 'constraint', ConfigValue(
                None, str_or_listOfStr,
                'Name of constraint block(s) used in sensor placement',
                """The name of the constraint block previously defined to be used in sensor placement.
                
                Required input.""") )
    problem.declare( 'imperfect', ConfigValue(
                None, str,
                'Imperfect block name used in sensor placement',
                """The name of the imperfect block previously defined to be used in sensor placement.
                
                Optional input.""") )
    problem.declare( 'aggregate', ConfigValue(
                None, str,
                'Aggregate block name used in sensor placement',
                """The name of the aggregate block previously defined to be used in sensor placement.
                
                Optional input.""") )
    problem.declare( 'compute bound', ConfigValue(
                False, bool,
                'Compute bounds: true or false, default = false',
                """A flag to indicate if bounds should be computed on the sensor placement
                solution. The options are true or false. 
                
                Optional input, default = false.""") )
    problem.declare( 'presolve', ConfigValue(
                True, bool,
                'Presolve problem: true or false, default = true',
                """A flag to indicate if the sensor placement problem should be presolved. 
                The options are true or false. 
                
                Optional input, default = true.""") )
    problem.declare( 'compute greedy ranking', ConfigValue(
                False, bool,
                'Compute greedy ranking of sensor locations, default = false',
                """A flag to indicate if a greedy ranking of the sensor locations should be calculated. 
                The options are true or false. 
                
                Optional input, default = false.""") )

    location_ = ConfigBlock()
    location_.declare( 'feasible nodes', ConfigValue(
                'ALL', None,
                'Feasible sensor nodes',
                """A list that defines nodes that can be considered for the sensor placement problem.
                The options are:
                (1) ALL, which specifies all nodes as feasible sensor locations;
                (2) NZD, which specifies all non-zero demand nodes as feasible sensor locations;
                (3) a list of EPANET node IDs, which identifies specific nodes as feasible sensor locations; 
                (4) a filename, which references a space or comma separated file containing a list of 
                specific nodes as feasible sensor locations; or
                (5) NONE, which indicates this option is ignored.
                
                Required input, default = ALL.""") )
    location_.declare( 'infeasible nodes', ConfigValue(
                'NONE', None,
                'Infeasible sensor nodes',
                """A list that defines nodes that cannot be considered for the sensor placement problem.
                The options are:
                (1) ALL, which specifies all nodes as infeasible sensor locations;
                (2) NZD, which specifies non-zero demand nodes as infeasible sensor locations;
                (3) a list of EPANET node IDs, which identifies specific nodes as infeasible sensor locations;
                (4) a filename, which references a space or comma separated file containing a list of 
                specific nodes as infeasible sensor locations; or
                (5) NONE, which indicates this option is ignored.
                
                Optional input, default = NONE.""") )
    location_.declare( 'fixed nodes', ConfigValue(
                'NONE', None,
                'Fixed sensor nodes',
                """A list that defines nodes that are already sensor locations.
                The options are:
                (1) ALL, which specifies all nodes as fixed sensor locations;
                (2) NZD, which specifies non-zero demand nodes as fixed sensor locations;
                (3) a list of EPANET node IDs, which identifies specific nodes as fixed sensor locations; 
                (4) a filename, which references a space or comma separated file containing a list of 
                specific nodes as fixed sensor locations; or
                (5) NONE, which indicates this option is ignored.
                
                Optional input, default = NONE.""") )
    location_.declare( 'unfixed nodes', ConfigValue(
                'NONE', None,
                'Unfixed sensor nodes',
                """A list that defines nodes that are unfixed sensor locations.
                The options are:
                (1) ALL, which specifies all nodes as unfixed sensor locations;
                (2) NZD, which specifies non-zero demand nodes as unfixed sensor locations;
                (3) a list of EPANET node IDs, which identifies specific nodes as unfixed sensor locations; 
                (4) a filename, which references a space or comma separated file containing a list of 
                specific nodes as unfixed sensor locations; or
                (5) NONE, which indicates this option is ignored.
                
                Optional input, default = NONE.""") )
    location = problem.declare( 'location', ConfigList([], domain=location_) )

    # flushing
    flushing = config.declare( 'flushing', ConfigBlock() )
    flushing.declare( 'detection', ConfigValue(
                None, None,
                "Sensor locations to detect contamination scenarios",
                """The sensor network design used to detect contamination scenarios. The
                sensor locations are used to compute a detection time for each 
                contamination scenario. The options are a list of 
                EPANET node IDs or a file name which contains a list of EPANET node IDs.
                
                Required input.""") )
    flushing.declare( 'allow location removal', ConfigValue(
                True, bool,
                "DEVELOPER OPTION",
                "Default = True",
                visibility=DEVELOPER_OPTION ) )
    n = flushing.declare( 'flush nodes', ConfigBlock() )
    n.declare( 'feasible nodes', ConfigValue(
                'ALL', None,
                'Feasible flushing nodes',
                """A list that defines the nodes in the network that can be flushed. 
                The options are: (1) ALL, which specifies all nodes as feasible flushing locations;
                (2) NZD, which specifies all non-zero demand nodes as feasible flushing locations;
                (3) NONE, which specifies no nodes as feasible flushing locations;
                (4) a list of EPANET node IDs, which identifies specific nodes as feasible flushing locations; or
                (5) a filename, which references a space or comma separated file containing a list of 
                specific nodes as feasible flushing locations.
                
                Required input, default = ALL.""") )
    n.declare( 'infeasible nodes', ConfigValue(
                'NONE', None,
                'Infeasible flushing nodes',
                """A list that defines the nodes in the network that cannot be flushed. 
                The options are: (1) ALL, which specifies all nodes as infeasible flushing locations;
                (2) NZD, which specifies all non-zero demand nodes as infeasible flushing locations;
                (3) NONE, which specifies no nodes as infeasible flushing locations;
                (4) a list of EPANET node IDs, which identifies specific nodes as infeasible flushing locations; or
                (5) a filename, which references a space or comma separated file containing a list of 
                specific nodes as infeasible flushing locations. 
                
                Optional input, default = NONE.""") )
    n.declare( 'max nodes', ConfigValue(
                0, int,
                'Maximum number of nodes to flush',
                """The maximum number of node locations that can be flushed simultaneously in the
                network. The value is a nonnegative integer or a list of
                nonnegative integers. When a list is specified, the optimization
                will be performed for each number in this list. For example, a value of 
                3 means that a maximum of 3 node will be identified as flushing locations 
                during the optimization process.
                
                Required input.""") ) 
    n.declare( 'rate', ConfigValue(
                0, float,
                'Flushing rate [gallons/min]',
                """The flushing rate for each node location to be flushed. A new demand pattern 
                will be created using this rate for the node. The value is a nonnegative integer. 
                For example, a value of 800 means that an additional demand of 800 gpm is applied 
                at a particular node. This rate is applied to all flushing locations identified 
                in the optimization process.
                
                Required input.""") ) 
    n.declare( 'response time', ConfigValue(
                0, float,
                'Time [min] between detection and flushing',
                """The time in minutes between the detection of a contamination incident and 
                the start of flushing. The value is a nonnegative integer. For example, 
                a value of 120 represents a 120 minutes or a 2 hour delay between 
                the time of detection and the start of flushing.
                
                Required input.""") )
    n.declare( 'duration', ConfigValue(
                0, float,
                'Flushing duration [min]',
                """The length of time in minutes that flushing will be simulated at a particular node. 
                The value is a nonnegative integer. For example, a value of 240 means that  
                flushing would be simulated at a particular node for 4 hours. This duration 
                is applied to all flushing locations identified in the optimization process.
                
                Required input.""") )
    v = flushing.declare( 'close valves', ConfigBlock() )
    v.declare( 'feasible pipes', ConfigValue(
                'ALL', None,
                'Feasible pipes to close',
                """A list that defines the pipes in the network that can be closed. 
                The options are: (1) ALL, which specifies all pipes as feasible pipes to close;
                (2) DIAM min max, which specifies all pipes with a specific diameter as feasible pipes to close;
                (3) NONE, which specifies no pipes as feasible pipes to close;
                (4) a list of EPANET pipe IDs, which identifies specific pipes as feasible pipes to close; or
                (5) a filename, which references a space or comma separated file containing a list of 
                specific pipes as feasible pipes to close. 
                
                Required input, default = ALL.""") )
    v.declare( 'infeasible pipes', ConfigValue(
                'NONE', None,
                'Infeasible pipes to close',
                """A list that defines the pipes in the network that cannot be closed. 
                The options are: (1) ALL, which specifies all pipes as infeasible pipes to close;
                (2) DIAM min max, which specifies all pipes with a specific diameter as infeasible pipes to close;
                (3) NONE, which specifies no pipes as infeasible pipes to close;
                (4) a list of EPANET pipe IDs, which identifies specific pipes as infeasible pipes to close; or
                (5) a filename, which references a space or comma separated file containing a list of 
                specific pipes as infeasible pipes to close. 
                
                Optional input, default = NONE.""") )
    v.declare( 'max pipes', ConfigValue(
                0, int,
                'Maximum number of pipes to close',
                """The maximum number of pipes that can be closed simultaneously in the
                network. The value must be a nonnegative integer or a list of
                nonnegative integers. When a list is specified, the optimization
                will be performed for each number in this list. For example, a value of 
                2 means that a maximum of 2 pipes to close will be identified 
                during the optimization process.
                
                Required input.""") ) 
    v.declare( 'response time', ConfigValue(
                0, float,
                'Time [min] between detection and closing pipes',
                """The time in minutes between the detection of a contamination incident and 
                closing pipes. The value is a nonnegative integer. For example, 
                a value of 120 would represent a 120 minutes or a 2 hour delay between 
                the time of detection and the start of pipe closures.
                
                Required input.""") )

    # booster_msx
    booster_msx = config.declare( 'booster msx', ConfigBlock() )
    booster_msx.declare( 'detection', ConfigValue(
                None, None,
                "Sensor locations to detect contamination scenarios",
                """The sensor network design used to detect contamination scenarios. The
                sensor locations are used to compute a detection time for each 
                contamination scenario in the TSG file. The options are a list of 
                EPANET node IDs or a file name which contains a list of EPANET node IDs.
                
                Required input.""") )
    booster_msx.declare( 'toxin species', ConfigValue(
                None, str,
                'Toxin species injected in each contaminant scenario',
                """The name of the contaminant species that is injected in each
                contamination scenario. This is the species that interacts with the 
                injected disinfectant and whose impact is going to be minimized.
                
                Required input.""") )
    booster_msx.declare( 'decon species', ConfigValue(
                None, str,
                'Decontaminant injected from booster station',
                """The name of the decontaminant or disinfectant species that is injected from the 
                booster stations.
                
                Required input.""") )
    booster_msx.declare( 'feasible nodes', ConfigValue(
                'ALL', None,
                'Feasible booster nodes',
                """A list that defines nodes that can be considered for the booster station placement problem.
                The options are: (1) ALL, which specifies all nodes as feasible booster station locations;
                (2) NZD, which specifies all non-zero demand nodes as feasible booster station locations;
                (3) NONE, which specifies no nodes as feasible booster station locations;
                (4) a list of EPANET node IDs, which identifies specific nodes as feasible booster station locations; or
                (5) a filename, which references a space or comma separated file containing a list of 
                specific nodes as feasible booster station locations.
                
                Required input, default = ALL.""") )
    booster_msx.declare( 'infeasible nodes', ConfigValue(
                'NONE', None,
                'Infeasible booster nodes',
                """A list that defines nodes that cannot be considered for the booster station placement problem.
                The options are: (1) ALL, which specifies all nodes as infeasible booster station locations;
                (2) NZD, which specifies non-zero demand nodes as infeasible booster station locations;
                (3) NONE, which specifies no nodes as infeasible booster station locations;
                (4) a list of EPANET node IDs, which identifies specific nodes as infeasible booster station locations; or
                (5) a filename, which references a space or comma separated file containing a list of 
                specific nodes as infeasible booster station locations. 
                
                Optional input, default = NONE.""") )
    booster_msx.declare( 'max boosters', ConfigValue(
                None, int,
                'Maximum number of booster stations',
                """The maximum number of booster stations that can be placed in the
                network. The value must be a nonnegative integer or a list of
                nonnegative integers. When a list is specified, the optimization
                will be performed for each number in this list.
                
                Required input.""" ) ) 
    booster_msx.declare( 'type', ConfigValue( 
                'FLOWPACED', str,
                'Booster source type: FLOWPACED',
                """The injection type for the disinfectant at the booster stations. The option is FLOWPACED. 
                See the EPANET manual for additional information about source types \\cite{EPANETusermanual}.
                
                Required input.""" ) )
    booster_msx.declare( 'strength', ConfigValue( 
                None, float,
                'Booster source strength [mg/L]',
                """The amount of disinfectant injected into the network from the booster stations. 
                For the source type FLOWPACED, the strength units are in mg/L.
            
                Required input.""" ) )
    booster_msx.declare( 'response time', ConfigValue(
                None, float,
                'Time [min] between detection and booster injection',
                """The time in minutes between the detection of a contamination incident and 
                the start of injecting disinfectants from the booster stations. The value 
                is a nonnegative integer. For example, a value of 120 represents 
                a 120 minutes or a 2 hour delay between the time of detection and 
                the start of booster injections.
                
                Required input.""" ) )
    booster_msx.declare( 'duration', ConfigValue(
                None, float,
                'Time [min] for booster injection',
                """The length of time in minutes that disinfectant will be injected at the booster 
                stations during the simulation.	The value is a nonnegative integer. For example, 
                a value of 240 means that a booster would simulate injection of disinfectant 
                at a particular node for 4 hours. This duration is applied to all booster 
                station locations identified in the optimization process.
                
                Required input.""" ) )

    # booster_mip
    booster_mip = config.declare( 'booster mip', ConfigBlock() )
    booster_mip.declare( 'detection', ConfigValue(
                None, None,
                "Sensor locations to detect contamination scenarios",
                """The sensor network design used to detect contamination scenarios. The
                sensor locations are used to compute a detection time for each 
                contamination scenario in the TSG file. The options are a list of 
                EPANET node IDs or a file name which contains a list of EPANET node IDs.
                
                Required input.""" ) )
    booster_mip.declare( 'model type', ConfigValue(
                'NEUTRAL', str,
                'Booster model type: NEUTRAL or LIMIT',
                """The model type used to determine optimal booster station
                locations. Options include NEUTRAL (complete neutralization)
                or LIMIT (limiting reagent). 
                
                Required input, default = NEUTRAL.""" ) )
    booster_mip.declare( 'model format', ConfigValue(
                'PYOMO', str,
                'Booster optimization model: AMPL or PYOMO',
                """The modeling language used to build the formulation specified
                by the model type option. The options are AMPL and PYOMO. 
				AMPL is a third party package that must be installed by 
				the user if this option is specified. PYOMO is an open source 
				software package that is distributed with WST. 
                
                Required input, default = PYOMO.""" ) )
    booster_mip.declare( 'stoichiometric ratio', ConfigValue(
                [0], lambda x: scalar_or_listOf(float,x),
                'Stoichiometric ratio [decon/toxin], LIMIT model only',
                """The stoichiometric ratio used by the limiting reagent
                model (LIMIT) represents the mass of disinfectant removed per 
                mass of contaminant removed. The units for disinfectant mass 
                and contaminant mass are determined by the type of injection used 
                for each species (mg for chemical and CFU for biological).  
                This can be a number or a list of
                numbers greater than 0.0. When a list is specified, the
                optimization will be performed for each number in this list. As
                the stoichiometric ratio approaches 0, the LIMIT model converges 
                to the NEUTRAL model.
                
                Required input if the model type = LIMIT.""" ) )
    booster_mip.declare( 'objective', ConfigValue(
                'MC', str,
                'Objective to minimize',
                """The impact metric used to place the booster stations.
                In the current version, all models support MC metric
                (mass of toxin consumed through the node demands). The
                PD metric is only supported in the LIMIT Pyomo model.                
                
                Required input, default = MC.""" ) )
    booster_mip.declare( 'PD dose threshold', ConfigValue(
                1e-8, float, 
		'DEVELOPER OPTION',
                """The contaminant dose above which a person is
                considered dosed. Required input for PD objective.
                Equivalent to 'DOSE\\_THRESHOLDS <num>' TAI file
                option.""",
                visibility=DEVELOPER_OPTION) )
    booster_mip.declare( 'PD demand per capita', ConfigValue(
                0.139, float, 
                'DEVELOPER OPTION',
                """Per capita usage rate used to determine per-node
                population (flow units/person). Flow units match those
                of the EPANET input file, which is likely
                GPM. Required input for PD objective. Equivalent to
                the 'POPULATION demand <num>' TAI file option.""",
                visibility=DEVELOPER_OPTION) )
    booster_mip.declare( 'PD ingestion rate', ConfigValue(
                2, float, 
                'DEVELOPER OPTION',
                """Daily volumetric ingestion rate per person
                (liters/day). Required input for PD
                objective. Equivalent to the 'INGESTIONRATE <num>' TAI
                file option.""",
                visibility=DEVELOPER_OPTION) )
    booster_mip.declare( 'PD ingestion type', ConfigValue(
                'demand', str, 
                'DEVELOPER OPTION',
                """Equivalent to the 'INGESTIONTYPE <name>' TAI file
                option. Only 'demand' is supported.""",
                visibility=DEVELOPER_OPTION) )
    booster_mip.declare( 'PD dose type', ConfigValue(
                'total', str, 
                'DEVELOPER OPTION',
                """Equivalent to the 'DOSETYPE <name>' TAI file
                option. Only 'total' is supported.""",
                visibility=DEVELOPER_OPTION) )
    booster_mip.declare( 'toxin decay coefficient', ConfigValue(
                0, None,
                'Toxin decay coeffienct: None, INP or number',
                """The contaminant (toxin) decay coefficient. The options are 
				(1) None, which runs the simulations without first-order decay, 
				(2) INP, which runs the simulations with first-order decay using the
                coefficient specified in the EPANET INP file or (3) a number, which 
				runs the simulation with first-order decay and the specified first-order
				decay coefficient in units of (1/min) (overrides the decay coefficient 
				in the EPANET INP file).
                
                Required input, default = 0.""" ) )
    booster_mip.declare( 'decon decay coefficient', ConfigValue(
                0, None, 
                'Decontaminant decay coefficient: None, INP or number', 
                """The disinfectant (decontaminant) decay coefficient. The options are 
				(1) None, which runs the simulations without first-order decay, 
				(2) INP, which runs the simulations with first-order decay using the
                coefficient specified in the EPANET INP file or (3) a number, which 
				runs the simulation with first-order decay and the specified first-order
				decay coefficient in units of (1/min) (overrides the decay coefficient 
				in the EPANET INP file).
                
                Required input, default = 0.""" ) )
    booster_mip.declare( 'feasible nodes', ConfigValue(
                'ALL', None,
                'Feasible booster nodes',
                """A list that defines nodes that can be considered for the booster station placement problem.
                The options are: (1) ALL, which specifies all nodes as feasible booster station locations;
                (2) NZD, which specifies all non-zero demand nodes as feasible booster station locations;
                (3) NONE, which specifies no nodes as feasible booster station locations;
                (4) a list of EPANET node IDs, which identifies specific nodes as feasible booster station locations; or
                (5) a filename, which references a space or comma separated file containing a list of 
                specific nodes as feasible booster station locations. 
                
                Required input, default = ALL.""") )
    booster_mip.declare( 'infeasible nodes', ConfigValue(
                'NONE', None,
                'Infeasbile booster nodes',
                """A list that defines nodes that cannot be considered for the booster station placement problem.
                The options are: (1) ALL, which specifies all nodes as infeasible booster station locations;
                (2) NZD, which specifies non-zero demand nodes as infeasible booster station locations;
                (3) NONE, which specifies no nodes as infeasible booster station locations;
                (4) a list of EPANET node IDs, which identifies specific nodes as infeasible booster station locations; or
                (5) a filename, which references a space or comma separated file containing a list of 
                specific nodes as infeasible booster station locations. 
                
                Optional input, default = NONE.""") )
    booster_mip.declare( 'fixed nodes', ConfigValue(
                [], str_or_listOfStr,
                'DEVELOPER OPTION',
                """A list of fixed nodes for boosters. Default = [].""",
                visibility=DEVELOPER_OPTION) )
    booster_mip.declare( 'max boosters', ConfigValue(
                None, lambda x: scalar_or_listOf(int, x),
                'Maximum number of booster stations',
                """The maximum number of booster stations that can be placed in the
                network. The value must be a nonnegative integer or a list of
                nonnegative integers. When a list is specified, the optimization
                will be performed for each number in this list.
                
                Required input.""" ) )
    booster_mip.declare( 'type', ConfigValue(
                None, str,
                'Booster source type: MASS or FLOWPACED',
                """The injection type for the disinfectant at the booster stations. 
                The options are MASS or FLOWPACED. 
                See the EPANET manual for additional information about source types \\cite{EPANETusermanual}.
                
                Required input.""" ) )
    booster_mip.declare( 'strength', ConfigValue(
                None, float,
                'Booster source strength [mg/min or mg/L depending on type]',
                """The amount of disinfectant injected into the network from the booster stations.  
                If the source type option is MASS, then the units for the strength are in mg/min.  
                If the source type option is FLOWPACED, then units are in mg/L.
                
                Required input.""" ) )
    booster_mip.declare( 'response time', ConfigValue(
                None, float,
                'Time [min] between detection and booster injection',
                """The time in minutes between the detection of a contamination incident and 
                the start of injecting disinfectants from the booster stations. The value 
                is a nonnegative integer. For example, a value of 120 represents 
                a 120 minutes or a 2 hour delay between the time of detection and 
                the start of booster injections.
				
                Required input.""" ) )
    booster_mip.declare( 'duration', ConfigValue(
                None, float,
                'Time [min] for booster injection',
                """The length of time in minutes that disinfectant will be injected at the booster 
                stations during the simulation.	The value is a nonnegative integer. For example, 
                a value of 240 means that a booster would simulate injection of disinfectant 
                at a particular node for 4 hours. This duration is applied to all booster 
                station locations identified in the optimization process.
				
                Required input.""" ) )
    booster_mip.declare( 'evaluate', ConfigValue(
                False, bool,
                'Evaluate booster placement: true or false,  default = false',
                """The option to evaluate the booster station placement created from 
				the optimization process.
				
				Optional input, default = false.""") )
    
    # developer options used in booster_mip
    boostersim = config.declare( 'boostersim', ConfigBlock() )
    boostersim.declare( 'options string', ConfigValue(
                None, None,'DEVELOPER OPTION',None,
                visibility=DEVELOPER_OPTION) )
    eventDetection = config.declare( 'eventDetection', ConfigBlock() )
    eventDetection.declare( 'options string', ConfigValue(
                None, None,'DEVELOPER OPTION',None,
                visibility=DEVELOPER_OPTION) )
    boosterimpact = config.declare( 'boosterimpact', ConfigBlock() )
    boosterimpact.declare( 'options string', ConfigValue(
                None, None,'DEVELOPER OPTION',None,
                visibility=DEVELOPER_OPTION) )
    
    # booster_quality
    booster_quality = config.declare( 'booster quality', ConfigBlock() )

    # inversion
    inversion = config.declare( 'inversion', ConfigBlock() )
    inversion.declare( 'algorithm', ConfigValue(
                'optimization', str,
                'Source inversion algorithm: optimization, bayesian, or csa',
                """The algorithm used to perform source inversion. The options are 
				optimization, bayesian, or csa. The optimization algorithm requires 
				AMPL or PYOMO along with a MIP solver. The bayesian algorithm 
				uses Bayes\' Rule to update probability of a particular node 
				being the contaminant source node. The CSA is the Contaminant
                                Status Algorithm by \\citet{csa}.
                
                Required input, default = optimization.""" ) )
    inversion.declare('formulation', ConfigValue(
                'MIP_discrete', str, 
                'Optimization formulation type, optimization only',
                """The formulation used by the optimization algorithm. The options are 
                LP\\_discrete (discrete LP), MIP\\_discrete (discrete MIP), 
                MIP\\_discrete\\_nd (discrete MIP with no decrease) or 
                MIP\\_discrete\\_step (discrete MIP for step injection).
                
                Required input for optimization algorithm, default = MIP\\_discrete."""))
    inversion.declare('model format', ConfigValue(
                'PYOMO', str, 
                'Source inversion optimization formulation: AMPL or PYOMO',
                """The modeling language used to build the formulation specified
                by the formulation option. The options are AMPL and PYOMO. 
				AMPL is a third party package that must be installed by 
				the user if this option is specified. PYOMO is an open source 
				software package that is distributed with WST.
                
                Required input for optimization algorithm, default = PYOMO."""))
    inversion.declare('merlion water quality model', ConfigValue(
                True, bool, 
                'Use Merlion water quality model for Bayesian algorithm',
                """This option is set to true to use the Merlion 
                water quality model for simulating the candidate injections
                in the Bayesian probability-based method. It can be set to false
                to use EPANET for these simulations. Note that the Merlion water quality
                model is required in either case to generate the initial list of candidate injections.
 
                Optional input, default = true."""))
    inversion.declare('horizon', ConfigValue(
                None, float,
                'Amount of past measurement data to use (min)',
                """The minutes over which the past measurement
                data is used for source inversion. It is calculated backwards from
                the latest measurement time in the measurements file. 
                All measurements in the measurements file that are within the horizon 
                are used (both negative and positive). In the case of the CSA algorithm, 
                the implementation assumes fixed sensors only, and all measurements at 
                these sensors are assumed to be negative prior to the horizon.
                If the horizon is longer
                than the time between the latest measurement and simulation start time,
                then all the measurements are used for source inversion.
                
                Required input, default = None (Start of simulation)."""))
    inversion.declare('num injections', ConfigValue(
                1, float, 
                'No. of possible injections',
                """The number of possible injections to consider when
                performing inversion. Multiple injections are only supported by
                the MIP formulation. This value must be set to 1 for the LP model
                or the probability algorithm.
                
                Required input for optimization algorithm, default = 1."""))
    inversion.declare('measurement failure', ConfigValue(
                0.05, float, 
                'Probability that a sensor fails',
                """The probability that a sensors gives an incorrect reading. Must be between 0 and 1. 
                
                Required input for the bayesian algorithm, default = 0.05."""))
    inversion.declare('positive threshold', ConfigValue(
                100, float,
                'Sensor threshold for positive contamination measurement',
                """The concentration threshold used by the sensors to flag a positive 
                detection measurement. This is a parameter in the optimization algorithm (Equation \\ref{eqn.milp_positive_set}).
                
                Required input for optimization algorithm, default = 100 mg/L. """))
    inversion.declare('negative threshold', ConfigValue(
                0.0, float,
                'Sensor threshold for negative contamination measurement',
                """The concentration threshold used by the sensors to flag
                a negative detection measurement. This is a parameter in the
                optimization algorithm (Equation \\ref{eqn.milp_negative_set}).
                
                Required input for optimization algorithm, default = 0.0 mg/L."""))
    inversion.declare('feasible nodes', ConfigValue(
                None, str, 
                'Feasible source nodes',
                """A list that defines nodes that can be considered for the source inversion problem.
                The options are: (1) ALL, which specifies all nodes as feasible source locations;
                (2) NZD, which specifies all non-zero demand nodes as feasible source locations;
                (3) a list of EPANET node IDs, which identifies specific nodes as feasible source locations; or
                (4) a filename, which is a space or comma separated file containing a list of 
                specific nodes as feasible source locations. 

                Optional input."""))
    inversion.declare('candidate threshold', ConfigValue(
                0.20, float,
                'Objective cut-off for candidate nodes.',
                """The objective cut-off value for candidate contamination incidents 
                using the optimization algorithm. The objective value represents the
                likelihood of a particular node being the injection node (See Equation \\ref{eq_transform_obj}).  
                The objective values are normalized to 1 and only the nodes having 
                their objective values greater or equal to the threshold are reported
                in the inversion results. 
                
                Required input for optimization algorithm, default = 0.20."""))
    inversion.declare('confidence', ConfigValue(
                0.95, float,
                'Probability confidence for candidate nodes.',
                """The probability cut-off value for candidate contamination incidents
                using the bayesian algorithm. The value is between 0 and 1. 
                
                Required for the bayesian algorithm, default = 0.95."""))
    inversion.declare('output impact nodes', ConfigValue(
                False, bool, 
                'Print likely injection nodes file',
                """A option to output a Likely\_Nodes.dat file that contains only
                the node IDs of the possible contaminant injection nodes obtained from the 
                \code{inversion} subcommand. This file can be used as the feasible nodes for the next 
                iteration of the \code{inversion} subcommand to only consider this set of possible contaminant 
                injection nodes.
                
                Optional input, default = false."""))
    inversion.declare('wqm file', ConfigValue(
                None, Path(config.get('__config_file_location__')), 
                'DEVELOPER OPTION',
                """This option can be used to provide a Merlion water quality model file. 
                The wqm file can be used in place of the EPANET INP file when running multiple
                cycles of source inversion. This speeds up inversion since recalculation of 
                hydraulics and rebuilding of the Merlion model is not required. """,
                visibility=DEVELOPER_OPTION))
    inversion.declare( 'ignore merlion warnings', ConfigValue(
                True, bool,
                'DEVELOPER OPTION',
                None,
                visibility=DEVELOPER_OPTION ) )

    # measurements file options
    measurements = config.declare( 'measurements', ConfigBlock() )
    measurements.declare('grab samples', ConfigValue(
                None, Path(config.get('__config_file_location__')), 
                'Measurements file name',
                """The name of the file that contains all the measurements from 
                the manual grab samples and the fixed sensors. The measurement file 
                format is documented in File Formats Section \\ref{formats_measFile}.

                Required input for source identification. Optional for uncertainty quantification."""))
 
    # grabsample
    grabsample = config.declare( 'grabsample', ConfigBlock() )
    grabsample.declare( 'model format', ConfigValue(
                'PYOMO', str,
                'Grab sample model format: AMPL or PYOMO',
                """The modeling language used to build the formulation specified
                by the model format option. The options are AMPL and PYOMO. 
                AMPL is a third party package that must be installed by 
                the user if this option is specified. PYOMO is an open source 
                software package that is distributed with WST.
       
                Required input, default = PYOMO.""" ) )

    grabsample.declare( 'sample criteria', ConfigValue(
                'distinguish', str,
                'Criteria to sample: distinguish, probability1, probability2',
                """ Determines which optimization model to solve. This option is 
                only checked when running the problem with signal files. By default
                the optimization is based on distinguishability of pair-wise scenarios.
       
                Optional input.""" ) )

    grabsample.declare( 'sample time', ConfigValue(
                720, float,
                'Sampling time (min)',
                """The time at which the manual grab sample should be taken. 
                The algorithm determines the best possible manual grab sample location(s)
                based upon this time. Units: Minutes from the simulation start time in the
                EPANET INP file. 

                Required input.""" ) )
    grabsample.declare( 'threshold', ConfigValue(
                None, float,
                'Contamination threshold. Default 0.001',
                """This threshold determines whether or not an incident impacts a candidate
                sample location.

                Required input, default = 0.001.""" ) )
    grabsample.declare( 'fixed sensors', ConfigValue(
                None, None,
                'Fixed sensor nodes',
                """A list that defines nodes that are already fixed continuous sensor locations.
                The options are: (1) ALL, which specifies all nodes as fixed sensor locations;
                (2) NZD, which specifies non-zero demand nodes as fixed sensor locations;
                (3) NONE, which specifies no nodes as fixed sensor locations;
                (4) a list of EPANET node IDs, which identifies specific nodes as fixed sensor locations; or
                (5) a filename, which references a space or comma separated file containing a list of 
                specific nodes as fixed sensor locations. 

                Optional input.""" ) )

    grabsample.declare( 'nodes metric', ConfigValue(
                None, None,
                'Map of node to metric (eg. EC, PI)',
                """ File containing a map of node to metric. The map is used for determining weighting factors
                in the objective of the distinguishability optimization formulation.
                Each line in the file has the node name separated by the corresponding metric.  

                Optional input.""" ) )
    
    grabsample.declare( 'list scenario ids', ConfigValue(
                None, None,
                'List of scenario ids considered from the signals folder',
                """ File containing list of scenarios to considered from the signals folder.
                Each line in the file has the signals id and the contamination id separeted by a space.
                Optional input.""" ) )
    
    grabsample.declare( 'feasible nodes', ConfigValue(
                None, None,
                'Feasible sampling nodes',
                """A list that defines nodes that can be considered as potential sampling locations 
                for the optimal sample location problem.
                The options are: (1) ALL, which specifies all nodes as feasible sampling locations;
                (2) NZD, which specifies all non-zero demand nodes as feasible sampling locations;
                (3) a list of EPANET node IDs, which identifies specific nodes as feasible sampling locations; or
                (4) a filename, which references a space or comma separated file containing a list of 
                specific nodes as feasible sampling locations. 

                Optional input.""" ) )
    grabsample.declare( 'num samples', ConfigValue(
                None, float,
                'Maximum number of grab samples, default = 1',
                """The maximum number of locations that can be sampled at one time. This is usually equal
                to the number of sampling teams that are available.

                Required input, default = 1.""" ) )
    grabsample.declare( 'greedy selection', ConfigValue(
                False, bool,
                'Perform greedy selection. No optimization',
                """The option to select manual grab sample locations based upon a greedy search.
                This does not require any optimization.

                Optional input.""" ) )
    grabsample.declare( 'with weights', ConfigValue(
                False, bool,
                'Perform optimization with weights in the objective function',
                """The option add weights in the objective function of the distinguishability 
                optimization formulation.

                Optional input.""" ) )
    grabsample.declare('filter scenarios', ConfigValue(
                False, bool,
                'Filters scenarios that match measurements',
                """ This options enables filtering scenarios. Only those scenarios 
                that match at least one of the measurements are considered
                in the optimal sampling analysis, default = False.""" ))

    grabsample.declare('wqm file', ConfigValue(
                None, Path(config.get('__config_file_location__')), 
                'DEVELOPER OPTION',
                """This option can be used to provide a Merlion water quality model file. 
                The wqm file can be used in place of the EPANET INP file when running multiple
                cycles of grab sample module. This speeds up cycling since recalculation of 
                hydraulics and rebuilding of the Merlion model is not required. """,
                visibility=DEVELOPER_OPTION) )

    # uncertainty quantification
    uq = config.declare( 'uq', ConfigBlock() )
    
    uq.declare( 'analysis time', ConfigValue(
                None, int,
                'analysis time (min)',
                """The time at which the manual grab sample is should be taken. 
                The algorithm determines the best possible manual grab sample location(s)
                based upon this time. Units: Minutes from the simulation start time in the
                EPANET INP file."""))

    uq.declare( 'threshold', ConfigValue(
                0.01, float,
                'Contamination threshold. Default 0.01 (mg/L)',
                """This threshold determines whether or not an incident impacts a location."""))

    uq.declare('filter scenarios', ConfigValue(
                False, bool,
                'Filters scenarios that match measurements',
                """ This options enables filtering scenarios. Only those scenarios 
                that match at least one of the measurements are considered
                in the optimal sampling analysis, default = False.""" ))

    uq.declare('measurement failure', ConfigValue(
                0.05, float, 
                'Probability that a sensor fails',
                """The probability that a sensors gives an incorrect reading. Must be between 0 and 1. 
                
                Required input for the bayesian algorithm, default = 0.05."""))
    uq.declare('confidence', ConfigValue(
                0.90, float,
                'Probability confidence for candidate nodes.',
                """The probability cut-off value for classifying nodes as certain to be contaminated, 
                uncertain to be contaminated, and certain to not be contaminated. The value is 
                between 0 and 1. Nodes with probability greater than (1-confidence/2) are 
                classified Likely to be contaminated or like yes LY, 
                Nodes with probability less than confidence/2 are classified Like not contaminated
                LN and nodes with probability in between are uncertain nodes UN
                
                Required for node classification, default = 0.95 (unitless)."""))
    #######################################################################################################
    # visualization
    visualization = config.declare('visualization', ConfigBlock())
    screen = visualization.declare('screen', ConfigBlock())
    screen.declare('color', ConfigValue(
                'white', str,
                'Screen color, HEX or predefined code',
                """The screen background color defined using a HEX color code or predefined color name.
                
                Optional input, default = white"""))
    screen.declare('size', ConfigValue(
                [1000,600], list,
                'Screen size [width, height] in pixels',
                """The screen size [width, height] in pixels.
                
                Optional input, default = [1000,600]"""))        
    legend = visualization.declare('legend', ConfigBlock())   
    legend.declare('color', ConfigValue(
                'white', str,
                'Legend color, HEX or predefined code',
                """The legend background color defined using a HEX color code or predefined color name.
                
                Optional input, default = white"""))
    legend.declare('scale', ConfigValue(
                1, float,
                'Legend text size multiplier, real number',
                """The legend text size multiplier, real number.
                
                Optional input, default = 1.0"""))
    legend.declare('location', ConfigValue(
                [10, 10], list,
                'Legend location [left, bottom] in pixels',
                """The legend location [left, top] in pixels.
                
                Optional input, default = [10,10] (upper left)"""))
    legend.declare('show legend', ConfigValue(
                True, bool,
                '','',visibility=DEVELOPER_OPTION))
    legend.declare('use EPANet symbols', ConfigValue(
                True, bool,
                '','',visibility=DEVELOPER_OPTION))
    nodes = visualization.declare('nodes', ConfigBlock())
    nodes.declare('color', ConfigValue(
                None, str,
                'Node color, HEX or predefined code',
                """The node color defined using HEX color code or predefined color name.
                The color will apply to junctions, reservoirs and tanks. If the 
                color is left blank, then junctions are black, reservoirs are 
                blue and tanks are green.
                
                Optional input, default = None"""))
    nodes.declare('size', ConfigValue(
                None, float,
                'Node size, real number',
                """The node size, real number.
                
                Optional input."""))
    nodes.declare('opacity', ConfigValue(
                0.6, float,
                'Node opacity, real number',
                """The node opacity, real number between 0.0 (transparent) and 1.0 (opaque).
                
                Optional input, default = 0.6"""))
    nodes.declare('show in legend', ConfigValue(
                False, bool,
                '','',visibility=DEVELOPER_OPTION))
    links = visualization.declare('links', ConfigBlock())
    links.declare('color', ConfigValue(
                None, str,
                'Link color, HEX or predefined code',
                """The link color defined using HEX color code or predefined color name.
                The color will apply to pipes, pumps and valves. If the 
                color is left blank, then pipes are black, pumps are 
                yellow and valves are turquoise.
                
                Optional input, default = None"""))
    links.declare('size', ConfigValue(
                None, float,
                'Link size, real number',
                """The link size, real number.
                
                Optional input."""))
    links.declare('opacity', ConfigValue(
                0.6, float,
                'Link opacity, real number',
                """The link opacity, real number between 0.0 (transparent) and 1.0 (opaque).
                
                Optional input, default = 0.6"""))
    links.declare('show in legend', ConfigValue(
                False, bool,
                '','',visibility=DEVELOPER_OPTION))
    reservoirs = visualization.declare('reservoirs', ConfigBlock(visibility=DEVELOPER_OPTION))
    reservoirs.declare('color', ConfigValue(
                None, str,
                '',))
    reservoirs.declare('size', ConfigValue(
                None, float,
                '',))
    reservoirs.declare('opacity', ConfigValue(
                0.6, float,
                '',))
    reservoirs.declare('show in legend', ConfigValue(
                None, bool,
                '',))
    tanks = visualization.declare('tanks', ConfigBlock(visibility=DEVELOPER_OPTION))
    tanks.declare('color', ConfigValue(
                None, str,
                '',))
    tanks.declare('size', ConfigValue(
                None, float,
                '',))
    tanks.declare('opacity', ConfigValue(
                0.6, float,
                '',))
    tanks.declare('show in legend', ConfigValue(
                None, bool,
                '',))
    junctions = visualization.declare('junctions', ConfigBlock(visibility=DEVELOPER_OPTION))
    junctions.declare('color', ConfigValue(
                None, str,
                '',))
    junctions.declare('size', ConfigValue(
                None, float,
                '',))
    junctions.declare('opacity', ConfigValue(
                0.6, float,
                '',))
    junctions.declare('show in legend', ConfigValue(
                None, bool,
                '',))
    pumps = visualization.declare('pumps', ConfigBlock(visibility=DEVELOPER_OPTION))
    pumps.declare('color', ConfigValue(
                None, str,
                '',))
    pumps.declare('size', ConfigValue(
                None, float,
                '',))
    pumps.declare('opacity', ConfigValue(
                0.6, float,
                '',))
    pumps.declare('show in legend', ConfigValue(
                None, bool,
                '',))
    valves = visualization.declare('valves', ConfigBlock(visibility=DEVELOPER_OPTION))
    valves.declare('color', ConfigValue(
                None, str,
                '',))
    valves.declare('size', ConfigValue(
                None, float,
                '',))
    valves.declare('opacity', ConfigValue(
                0.6, float,
                '',))
    valves.declare('show in legend', ConfigValue(
                None, bool,
                '',))
    pipes = visualization.declare('pipes', ConfigBlock(visibility=DEVELOPER_OPTION))
    pipes.declare('color', ConfigValue(
                None, str,
                '',))
    pipes.declare('size', ConfigValue(
                None, float,
                '',))
    pipes.declare('opacity', ConfigValue(
                0.6, float,
                '',))
    pipes.declare('show in legend', ConfigValue(
                None, bool,
                '',))
    layers = ConfigBlock()
    visualization.declare('layers', ConfigList([], domain=layers))
    layers.declare('label', ConfigValue(
                None, str,
                'Label used in legend',
                """The layer label used in the legend. 
                
                Optional input, default = None"""))
    layers.declare('locations', ConfigValue(
                None, str_or_listOfStr,
                'Data locations, list of EPANET IDs',
                """
                The data locations to plot over the network. Locations are specified
                using a list of EPANET IDs.
                
                Required input unless an external file is specified."""))
    layers.declare('file', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'Locations from file, yaml format',
                """The name of an external file that contains data to be used in the visualization.
                The file is in YAML format. 
                
                Required input unless 'locations' are specified.  
                Data from a file overrides data specified in 'locations'"""))
    layers.declare('location type', ConfigValue(
                None, str,
                'Location type, node or link',
                """The location type is used to indicate if the EPANET ID is of type 'node' 
                (junction, reservoir, tank) or 'link' (pipe, pump, valve).
                
                Optional input. If left blank, node is tested before link"""))
    layers.declare('shape', ConfigValue(
                'circle', str_or_listOfStr,
                'Marker shape, predefined name',
                """The marker shape, used only for node type layers. The shape can
                be a single string, or a list of strings which is the same 
                length as the location data.
                
				Optional input, only used for node type layers, default = circle"""))
    fill = layers.declare('fill', ConfigBlock())
    fill.declare('color', ConfigValue(
                None, None,
                'Fill color, HEX or predefined code',
                """The fill color defined using HEX color code or predefined color name.
                
                Optional input, default = None"""))
    fill.declare('size', ConfigValue(
                None, None,
                'Fill size, real number',
                """The fill size, real number.
                
                Optional input."""))
    fill.declare('opacity', ConfigValue(
                0.6, None,
                'Fill opacity, real number',
                """The fill opacity, real number between 0.0 (transparent) and 1.0 (opaque).
                
                Optional input, default = 0.6"""))
    fill.declare('color range', ConfigValue(
                None, list,
                'Fill color range [min, max]',
                """The fill color range used to scale line data.
                
                Optional input, default = [data min, data max]"""))
    fill.declare('size range', ConfigValue(
                None, list,
                'Fill size range [min, max]',
                """The fill size range used to scale line data.
                
                Optional input, default = [data min, data max]"""))
    fill.declare('opacity range', ConfigValue(
                None, list,
                'Fill opacity range [min, max]',
                """The fill opacity range used to scale line data.
                
                Optional input, default = [data min, data max]"""))
    line = layers.declare('line', ConfigBlock())
    line.declare('color', ConfigValue(
                None, None,
                'Line color, HEX or predefined code',
                """The line color defined using HEX color code or predefined color name.
                
                Optional input, default = None"""))
    line.declare('size', ConfigValue(
                None, None,
                'Line size, real number',
                """The line size, real number.
                
                Optional input."""))
    line.declare('opacity', ConfigValue(
                0.6, None,
                'Line opacity, real number',
                """The fill opacity, real number between 0.0 (transparent) and 1.0 (opaque).
                
                Optional input, default = 0.6"""))
    line.declare('color range', ConfigValue(
                None, list,
                'Line color range [min, max]',
                """The line color range used to scale line data.
                
                Optional input, default = [data min, data max]"""))
    line.declare('size range', ConfigValue(
                None, list,
                'Line size range [min, max]',
                """The line size range used to scale line data.
                
                Optional input, default = [data min, data max]"""))
    line.declare('opacity range', ConfigValue(
                None, list,
                'Line opacity range [min, max]',
                """The line opacity range used to scale line data.
                
                Optional input, default = [data min, data max]"""))
    layers.declare('hide', ConfigValue(
                False, bool,
                '','',visibility=DEVELOPER_OPTION))

    #######################################################################################################
    # solver
    solver = config.declare('solver', ConfigBlock())
    solver.declare( 'name', ConfigValue(
                'solver1', str,
                'DEVELOPER OPTION',
                "Solver name",
                visibility=DEVELOPER_OPTION) )
    solver.declare( 'type', ConfigValue(
                '', str,
                'Solver type',
                """The solver type. Each component of WST
				(e.g., sensor placement, flushing response, booster 
				placement, source identification and grab sample) has different 
				solvers available and more specific details are provided in 
				the subcommand's chapter.
                
                Required input.""") )
    solver.declare( 'options', ConfigBlock(
            'A dictionary of solver options',
            """A list of options associated with a specific solver type. More
            information on the options available for a specific solver
            is provided in the solver's documentation. The Getting
            Started Section \\ref{dependencies} provides links to the
            different solvers.
            
            Optional input.""", implicit=True ) )
    solver.declare( 'problem writer', ConfigValue(
                'none', str,
                'DEVELOPER OPTION',
                "Options = nl, lp, none.  Default = none",
                visibility=DEVELOPER_OPTION) )
    solver.declare( 'threads', ConfigValue(
                1, int,
                'Number of concurrent threads or function evaluations',
                """The maximum number of threads or function evaluations the solver is
                allowed to use.  This option is not available to all solvers or all analyses.
                
                Optional input.""") )
    solver.declare( 'logfile', ConfigValue(
                None, Path(config.get('__config_file_location__')),
                'Redirect solver output to a logfile',
                """The name of a file to output the results of the solver.
                
                Optional input.""") )
    solver.declare( 'verbose', ConfigValue(
                0, int,
                'Solver verbosity level',
                """The solver verbosity level.
                
                Optional input, default = 0 (lowest level).""") )

    initialpoints_ = ConfigBlock()
    solver.declare( 'initial points', ConfigList([], domain=initialpoints_) )
    initialpoints_.declare( 'nodes', ConfigValue(
        [], str_or_listOfStr,
        'Initial nodes to begin optimization',
        """A list of node locations (EPANET IDs) to begin the optimization
        process.  Currently, this option is only supported for the
        network solver used in the flushing response and booster\\_msx
        placement.
        
        Optional input.""") )
    initialpoints_.declare( 'pipes', ConfigValue(
        [], str_or_listOfStr,
        'Initial pipes to begin optimization',
        """A list of pipe locations (EPANET IDs) to begin the optimization
        process.  Currently, this option is only supported for the
        network solver used in the flushing response.
        
        Optional input.""") )

    # configure
    configure = config.declare('configure', ConfigBlock())
    configure.declare( 'output prefix', ConfigValue(
                None, Path(config.get('__config_file_location__'),
                           preserveTrailingSep=True),
                'Output file prefix',
                """The prefix used for all output files.
                
                Required input.""") )

    configure.declare( 'output directory', ConfigValue(
                'SIGNALS_DATA_FOLDER', Path(config.get('__config_file_location__'),
                           preserveTrailingSep=True),
                'Output directory',
                """The output directory to store the results.""") )

    configure.declare( 'keepfiles', ConfigValue(
                None, str,
                'DEVELOPER OPTION',
                None,
                visibility=DEVELOPER_OPTION) )
    configure.declare( 'temp directory', ConfigValue(
                None, str,
                'DEVELOPER OPTION',
                None,
                visibility=DEVELOPER_OPTION) )
    #
    # WEH - These values should not be needed by the wst executable.
    #
    configure.declare( 'ampl executable', ConfigValue(
                find_executable('ampl'), 
                Path(config.get('__config_file_location__')),
                'DEVELOPER OPTION',
                None,
                visibility=DEVELOPER_OPTION) )
    configure.declare( 'pyomo executable', ConfigValue(
                find_executable('pyomo'), 
                Path(config.get('__config_file_location__')),
                'DEVELOPER OPTION',
                None,
                visibility=DEVELOPER_OPTION) )
    configure.declare( 'tevasim executable', ConfigValue(
                find_executable('tevasim'), 
                Path(config.get('__config_file_location__')),
                'DEVELOPER OPTION',
                None,
                visibility=DEVELOPER_OPTION) )
    configure.declare( 'sim2Impact executable', ConfigValue(
                find_executable('tso2Impact'), 
                Path(config.get('__config_file_location__')),
                'DEVELOPER OPTION',
                None,
                visibility=DEVELOPER_OPTION) )
    configure.declare( 'sp executable', ConfigValue(
                find_executable('sp'), 
                Path(config.get('__config_file_location__')),
                'DEVELOPER OPTION',
                None,
                visibility=DEVELOPER_OPTION) )
    configure.declare( 'coliny executable', ConfigValue(
                find_executable('coliny'), 
                Path(config.get('__config_file_location__')),
                'DEVELOPER OPTION',
                None,
                visibility=DEVELOPER_OPTION) )
    configure.declare( 'dakota executable', ConfigValue(
                find_executable('dakota'), 
                Path(config.get('__config_file_location__')),
                'DEVELOPER OPTION',
                None,
                visibility=DEVELOPER_OPTION) )
    #configure.declare( 'solver', ConfigValue(
    #            'solver1', str,
    #            None,
    #            None) )
    #configure.declare( 'problem', ConfigValue(
    #            'problem1', str,
    #            None,
    #            None) )
    configure.declare( 'debug', ConfigValue(
                0, int,
                'Debugging level, default = 0',
                """The debugging level (0 or 1) that indicates the amount of debugging 
                information printed to the screen, log file, and output yml file. 
                
                Optional input, default = 0 (lowest level).""") )

    return config               

def output_config():
    
    config = ConfigBlock()
    
    general = config.declare('general', ConfigBlock())
    general.declare( 'version', ConfigValue(
                1.5, None,
                'WST version', 
                None ) )
    general.declare( 'date', ConfigValue(
                datetime.datetime.now().strftime("%Y-%m-%d"), str,
                'Run date', 
                None ) )
    general.declare( 'cpu time', ConfigValue(
                None, float,
                'CPU time (sec)',
                None ) )
    general.declare( 'directory', ConfigValue(
                None, str,
                'Results directory',
                None ) )
    general.declare( 'log file', ConfigValue(
                None, str,
                'Log file',
                """This file contains general, debug, warning and error messages.""" ) )    
    
    # tevasim output block
    tevasim = config.declare('tevasim', ConfigBlock())
    tevasim.declare( 'report file', ConfigValue(
                None, str,
                'EPANET report file',
                """This file provides information on the EPANET simulations. The 
                EPANET report file format is described in Appendix C.3 of the 
                EPANET Users Manual \\citep{EPANETusermanual}.""" ) )            
    tevasim.declare( 'header file', ConfigValue(
                None, str,
                'ERD header file',
                None) )            
    tevasim.declare( 'hydraulic file', ConfigValue(
                None, str,
                'ERD hydraulic file',
                None) ) 
    tevasim.declare( 'water quality file', ConfigValue(
                None, str,
                'ERD water quality file',
                None) ) 
    tevasim.declare( 'index file', ConfigValue(
                None, str,
                'ERD index file',
                None) ) 
    
    # sim2Impact output block
    sim2Impact = config.declare('sim2Impact', ConfigBlock())
    sim2Impact.declare( 'impact file', ConfigValue(
                None, str_or_listOfStr,
                'Impact file',
                None ) )            
    sim2Impact.declare( 'id file', ConfigValue(
                None, str_or_listOfStr,
                'ID file',
                None) )            
    sim2Impact.declare( 'nodemap file', ConfigValue(
                None, str,
                'Nodemap file',
                None) ) 
    sim2Impact.declare( 'scenariomap file', ConfigValue(
                None, str,
                'Scenariomap file',
                None) ) 
                
     # sp output block
    sp = config.declare('sensor placement', ConfigBlock())
    sp.declare( 'nodes', ConfigValue(
                None, None,
                'List of sensor nodes',
                None ) )            
    sp.declare( 'objective', ConfigValue(
                None, str_or_listOfStr,
                'Objective value',
                None) )    
    sp.declare( 'lower bound', ConfigValue(
                None, None,
                'Lower bound',
                None) )
    sp.declare( 'upper bound', ConfigValue(
                None, None,
                'Upper bound',
                None) )
    sp.declare( 'greedy ranking', ConfigValue(
                None, None,
                'Upper bound',
                None) )
    sp.declare( 'stage 2', ConfigValue(
                None, None,
                'Upper bound',
                None) )
    
    # flushing output block
    flushing = config.declare('flushing', ConfigBlock())
    flushing.declare( 'nodes', ConfigValue(
                None, str_or_listOfStr,
                'List of nodes to flush',
                None ) )            
    flushing.declare( 'pipes', ConfigValue(
                None, str_or_listOfStr,
                'List of pipes to close',
                None ) )
    flushing.declare( 'objective', ConfigValue(
                None, float,
                'Objective value',
                None) )      
                
    # booster_mip and booster_msx output block
    booster = config.declare('booster', ConfigBlock())
    booster.declare( 'nodes', ConfigValue(
                None, str_or_listOfStr,
                'List of booster nodes',
                None ) )            
    booster.declare( 'objective', ConfigValue(
                None, float,
                'Objective value',
                None) )  

    # grabsample output block
    grabsample = config.declare('grabsample', ConfigBlock())
    grabsample.declare( 'nodes', ConfigValue(
                None, str_or_listOfStr,
                'List of grabsample nodes',
                None ) )            
    grabsample.declare( 'objective', ConfigValue(
                None, float,
                'Objective value',
                None) ) 
    grabsample.declare( 'threshold', ConfigValue(
                None, float,
                'Threshold',
                None) )
    grabsample.declare( 'count', ConfigValue(
                None, int,
                'Count',
                None) ) 
    grabsample.declare( 'time', ConfigValue(
                None, float,
                'Time',
                None) ) 
                
    # inversion output block
    inversion = config.declare('inversion', ConfigBlock())
    inversion.declare( 'tsg file', ConfigValue(
                None, str,
                '',
                None ) )            
    inversion.declare( 'likely nodes file', ConfigValue(
                None, str,
                '',
                None) ) 
    inversion.declare( 'candidate nodes', ConfigValue(
                None, str_or_listOfStr,
                'List of candidate injection nodes',
                None) ) 
    inversion.declare( 'node likeliness', ConfigValue(
                None,lambda x: scalar_or_listOf(float,x),
                'Likeliness measure of each node being true injection node.',
                None) ) 
    
    # scenarios output block  
    scenarios_ = ConfigBlock()
    config.declare( 'scenario results', ConfigList([], domain=scenarios_) )
    scenarios_.declare( 'definition', ConfigValue(
                None, str,
                'Scenario input definition',
                None) )
    scenarios_.declare( 'more', ConfigValue(
                None, None,
                None, None) )
                
              
    scenarios = config.declare('scenarios', ConfigBlock())
    scenarios.declare( 'injection node', ConfigValue(
                None, None,
                None,
                None) ) 
    scenarios.declare( 'start time', ConfigValue(
                None, None,
                None,
                None) ) 
    scenarios.declare( 'end time', ConfigValue(
                None, None,
                'Injection node',
                None) ) 
    discard = scenarios.declare( 'discarded', ConfigBlock() )
    discard.declare( 'count', ConfigValue(
                None, int,
                'Number of discarded scenarios',
                None) ) 
    nondetect = scenarios.declare( 'non-detected', ConfigBlock() )
    nondetect.declare( 'count', ConfigValue(
                None, int,
                'Number of non-detected scenarios',
                None) )   
    nondetect.declare( 'mass injected', ConfigValue(
                None, None, 
                'Mass injected (grams)', 
                None) )
    detect = scenarios.declare( 'detected', ConfigBlock() )
    detect.declare( 'count', ConfigValue(
                None, int,
                'Number of detected scenarios',
                None) )   
    detect.declare( 'detection time', ConfigValue(
                None, None,
                'Detection time (min)',
                None) )   
    detect.declare( 'mass consumed', ConfigValue(
                None, None, 
                'Mass consumed (grams)', 
                None) )
    detect.declare( 'mass injected', ConfigValue(
                None, None, 
                'Mass injected (grams)',
                None) )
    detect.declare( 'pre-booster mass injected', ConfigValue(
                None, None, 
                'Mass injected before booster activate(grams)', 
                None) )
    detect.declare( 'mass in tanks', ConfigValue(
                None, None, 
                'Mass remaining in tanks (grams)', 
                None) )
    detect.declare( 'mass balence', ConfigValue(
                None, None, 
                'Mass balence', 
                None) )
                
    return config   
