import pywst.common.wst_util as wst_util
from pywst.common.signals import *
from random import randint
import imp
            
class SamplingLocation(object):

    def __init__(self,list_scenarios,data_manager):
        self.data_manager = data_manager
        self.list_events = list_scenarios
        self.n_events = len(self.list_events)
	#self.list_event_idx = range(self.n_events)
	self.pair_wise_events = tuple(itertools.combinations(range(self.n_events), 2))
	self.pair_wise_to_idx = {pair:i for i,pair in enumerate(self.pair_wise_events)}

    def build_Dij_sets(self,time_min):
        Dij = dict()
        for pair in self.pair_wise_events:
            e1 = pair[0]
            e2 = pair[1]
            event1 = self.list_events[e1]
            event2 = self.list_events[e2]
            impacted_nodes_e1 = event1.time_to_nodes[time_min]
            impacted_nodes_e2 = event2.time_to_nodes[time_min]
            Dij[pair] = impacted_nodes_e1.symmetric_difference(impacted_nodes_e2)
        return Dij

    def compute_weights_dict(self,Dij_dict):
        weights = dict()
        node_to_metric = self.data_manager.node_to_metric_map
        if node_to_metric:
            for pair,nids in Dij_dict.iteritems():
                weights[pair] = sum(node_to_metric[nid] for nid in nids)
        else:
            for pair,nids in Dij_dict.iteritems():
                weights[pair] = len(nids)

        return weights
    
    def write_gs_datafile(self,filename,locations,time_min,n_samples,fixed_locations=[],
                          with_weights=False):
        with open(filename,'w') as f:
            f.write('set S_EVENTS := ')
            for i in xrange(self.n_events):
                f.write("{} ".format(i))
            f.write(";\n\n")

            f.write('set S_LOCATIONS := ')
            for nid in locations:
                f.write("{} ".format(nid))
            f.write(";\n\n")

            f.write('set S_PAIR_WISE := ')
            for e1,e2 in self.pair_wise_events:
                f.write("({},{}) ".format(e1,e2))
            f.write(";\n\n")

            Dij = self.build_Dij_sets(time_min)
            for pair in self.pair_wise_events:
                e1 = pair[0]
                e2 = pair[1]
                f.write('set S_DISTINGUISH_LOCATIONS[{},{}] := '.format(e1,e2))
                for nid in Dij[pair]:
                    f.write('{} '.format(nid))
                f.write(";\n")
            f.write("\n")

            f.write('set S_FIXED_LOCATIONS := ')
            for nid in fixed_locations:
                f.write('{} '.format(nid))
            f.write(";\n\n")

            f.write('param P_MAX_SAMPLES := {};\n\n'.format(n_samples))

            if with_weights:
                f.write('param P_WEIGHT_FACTORS :=\n')
                weights = self.compute_weights_dict(Dij)
                for pair,nids in Dij.iteritems():
                    e1 = pair[0]
                    e2 = pair[1]
                    #w = len(nids)
                    w = weights[pair]
                    f.write('{} {} {}\n'.format(e1,e2,w))
                f.write(";\n")


    def greedySelection(self,locations,time_min,n_samples,with_weights=False):

        Dij = self.build_Dij_sets(time_min)
        Nij = dict()
        for nid in locations:
            Nij[nid] = set()
            for pair,nids in Dij.iteritems():
                if nid in nids:
                    Nij[nid].add(self.pair_wise_to_idx[pair])

        if with_weights:
            weights = self.compute_weights_dict(Dij)
            
        i_sample = 1
        sampling_locations = list()
        objective = 0
        while i_sample<=n_samples:
            # get the node with maximum set of pairs
            if not with_weights:
                Nij_metric = {k:len(v) for k,v in Nij.iteritems()}
            else:
                Nij_metric = dict()
                for k,pairs in Nij.iteritems():
                    Nij_metric[k] = sum(weights[self.pair_wise_events[pair]] for pair in pairs)
                    
            max_nid, max_dist = max(Nij_metric.iteritems(), key=lambda x: x[1])
            # get distinguishable pairs for max_nid
            distinguished_pairs = copy.deepcopy(Nij[max_nid])
            sampling_locations.append(max_nid)
            objective+=max_dist
            
            #remove current max_nid from dictionary
            if i_sample<n_samples:
                del Nij[max_nid]
                for pair_idx in distinguished_pairs:
                    for pairs in Nij.itervalues():
                        pairs.discard(pair_idx)
        
            i_sample+=1

        #print objective
        sample_tuples = [(nid,time_min) for nid in sampling_locations]
        return sample_tuples,objective

    def concrete_model_distinguishability(self,locations,time_min,n_samples,fixed_locations=[],
                          with_weights=False):

        m = ConcreteModel()
        m.S_EVENTS = range(self.n_events)
        m.S_LOCATIONS = locations
        m.S_PAIR_WISE = self.pair_wise_events
        m.S_DISTINGUISH_LOCATIONS = self.build_Dij_sets(time_min)
        m.P_MAX_SAMPLES = n_samples
        
        m.s = Var(m.S_LOCATIONS, within=Binary)
        m.d = Var(m.S_PAIR_WISE, within=NonNegativeReals, bounds=(0.0,1.0))

        if with_weights:
            m.P_WEIGHT_FACTORS = self.compute_weights_dict(m.S_DISTINGUISH_LOCATIONS)
            #Objective function
            def obj(m):
                return sum(m.P_WEIGHT_FACTORS[i,j]*m.d[i,j] for (i,j) in m.S_PAIR_WISE)
        else:
            def obj(m):
                return sum(m.d[i,j] for (i,j) in m.S_PAIR_WISE)

        m.OBECTIVE = Objective(rule=obj, sense=maximize)
        #CONSTRAINS
        #Number of samples to be taken
        def num_samples(m):
            return sum(m.s[n] for n in m.S_LOCATIONS) <= m.P_MAX_SAMPLES
        m.MAX_SAMPLES = Constraint(rule=num_samples)

        # Last constraint
        def dij_constraint(m,i,j):
            return sum(m.s[n] for n in m.S_DISTINGUISH_LOCATIONS[i,j]) >= m.d[i,j]
        
        m.DIJ_CONST = Constraint(m.S_PAIR_WISE, rule=dij_constraint)
        
        return m
    
    def run_distinguishability_model(self,
                                     locations,
                                     time_min,
                                     n_samples,
                                     solver='glpk',
                                     solver_opts=dict(),
                                     with_weights=False,
                                     tee=False,
                                     concrete=True):

        if not concrete:
            pyomo_module = os.path.join(os.path.dirname(os.path.abspath(__file__)),'models','pyomo','gs_model')
            pm = imp.load_source(os.path.basename(pyomo_module),pyomo_module+".py")
            model = pm.model
            
            data='GSP.dat'
            wst_util.declare_tempfile('GSP.dat')
            self.write_gs_datafile(data,
                                   locations,
                                   time_min,
                                   n_samples,
                                   with_weights=with_weights)

            modeldata=DataPortal()
            modeldata.load(model=model, filename=data)
            GSmod=model.create_instance(modeldata)
            GSmod.preprocess()
        else:
            GSmod =self.concrete_model_distinguishability(locations,
                                                          time_min,
                                                          n_samples,
                                                          with_weights=with_weights)
        opt = SolverFactory(solver)
        for k,v in solver_opts.iteritems():
            opt.options[k] = v
        
        results = opt.solve(GSmod,tee=tee)
        
        nodes=[]
        for location in GSmod.S_LOCATIONS:
            if(value(GSmod.s[location])>0):
                nodes.append(location)
        solution=(float(value(GSmod.OBECTIVE)),nodes)
        
        objective = solution[0]
        opt_locations = [(int(nid),time_min) for nid in solution[1]]

        return opt_locations,objective
        
    def min_expected_np_model(self,locations,loc_probabilities,time_min,n_samples,weight_obj=False):
        
        model = ConcreteModel()
        model.locations = Set(initialize=locations)
        model.scenarios = Set(initialize=range(self.n_events))
        
        model.Ptilde = Var(model.scenarios,domain=Reals)
        model.Pmatch = Var(model.scenarios,domain=NonNegativeReals)
        model.x = Var(model.locations,domain=Binary)

        def rule_pmatch(m,s):
            accum = 0.0
            for nid in m.locations:
                event = self.list_events[s]
                alpha = 1.0-loc_probabilities[nid]
                if event.is_contaminated(nid,time_min):
                    alpha = loc_probabilities[nid]
                if alpha<1e-6:
                    alpha=1e-6
                accum+=m.x[nid]*log(alpha)
                #accum+=m.x[nid]*alpha
            return m.Ptilde[s] == accum
        
        model.P_match_constraint = Constraint(model.scenarios,rule=rule_pmatch)

        
        # add underestimators
        
        points = np.linspace(-13.86,1.0,20)
        model.cuts = ConstraintList()
        for s in model.scenarios:
            for x in points:
                m = exp(x)
                model.cuts.add(model.Pmatch[s] >= m*(model.Ptilde[s]+1.0-x))
        
        """
        tol=1e-3
        model.cuts = ConstraintList()
        for s in model.scenarios:
            count = 0
            m = 1.0
            x= 0.0
            npoints=100
            delta_x = 0.015
            while m>tol and count<npoints:
                m = np.exp(x)
                model.cuts.add(model.Pmatch[s] >= m*(model.Ptilde[s]+1.0-x))
                x = x - 1.0/m*delta_x
                count+=1
        """
        def samples_rule(m):
            return sum(m.x[n] for n in m.locations)==n_samples
        model.limit_samples = Constraint(rule=samples_rule)

        if weight_obj:
            obj_expr = sum(model.Pmatch[s]*self.list_events[s].probability for s in model.scenarios)
        else:
            obj_expr = sum(model.Pmatch[s] for s in model.scenarios)

        model.expected_matches = Objective(expr=obj_expr)

        return model

    def min_max_p_model_t(self,locations,loc_probabilities_l,times,n_samples):
        
        model = ConcreteModel()
        model.locations = locations
        model.times = times
        model.scenarios = range(self.n_events)
        
        model.Ptilde = Var(model.scenarios,domain=Reals)
        #model.x = Var(model.locations,domain=Binary)
        #model.y = Var(model.locations,model.times,bounds=(0.0,1.0))
        model.y = Var(model.locations,model.times,domain=Binary)
        model.q = Var(initialize=0.0)

        def max_p(m,s):
            return m.q >= m.Ptilde[s]
        model.max_p_constraint = Constraint(model.scenarios,rule=max_p)

        def rule_ptilde(m,s):
            accum = 0.0
            event = self.list_events[s]
            for nid in m.locations:
                for k,t in enumerate(m.times):
                    alpha = 1.0-loc_probabilities_l[k][nid]
                    if event.is_contaminated(nid,t):
                        alpha = loc_probabilities_l[k][nid]
                    if alpha<1e-6:
                        alpha=1e-6
                    accum+=m.y[nid,t]*log(alpha)
            return m.Ptilde[s] == accum
        
        model.P_tilde_constraint = Constraint(model.scenarios,rule=rule_ptilde)
        
        def samples_rule(m):
            return sum(m.y[n,t] for n in m.locations for t in m.times)<=n_samples
        model.limit_samples = Constraint(rule=samples_rule)

        #def upper_bound_y_rule(m,n,t):
        #    return m.y[n,t]<=m.x[n]
        #model.bound_y = Constraint(model.locations,model.times,rule=upper_bound_y_rule)

        model.expected_matches = Objective(expr=model.q)

        return model

    def min_max_p_model(self,locations,loc_probabilities,time_min,n_samples):
        
        model = ConcreteModel()
        model.locations = locations
        model.scenarios = range(self.n_events)
        
        model.Ptilde = Var(model.scenarios,domain=Reals)
        model.x = Var(model.locations,domain=Binary)
        model.q = Var(initialize=0.0)

        def max_p(m,s):
            return m.q >= m.Ptilde[s]
        model.max_p_constraint = Constraint(model.scenarios,rule=max_p)

        def rule_ptilde(m,s):
            accum = 0.0
            event = self.list_events[s]
            for nid in m.locations:
                alpha = 1.0-loc_probabilities[nid]
                if event.is_contaminated(nid,time_min):
                    alpha = loc_probabilities[nid]
                if alpha<1e-6:
                    alpha=1e-6
                accum+=m.x[nid]*log(alpha)
            return m.Ptilde[s] == accum
        
        model.P_tilde_constraint = Constraint(model.scenarios,rule=rule_ptilde)
        
        def samples_rule(m):
            return sum(m.x[n] for n in m.locations)==n_samples
        model.limit_samples = Constraint(rule=samples_rule)

        model.expected_matches = Objective(expr=model.q)

        return model
    
    def run_probability_model(self,
                        locations,
                        loc_probabilities,
                        time_min,
                        n_samples,
                        solver='glpk',
                        solver_opts=dict(),
                        model='probability1',
                        tee=False,
                        weight_obj=False):

        if model=='probability1':
            instance = self.min_expected_np_model(locations,loc_probabilities,time_min,n_samples,weight_obj=weight_obj)
        elif model=='probability2':
            instance = self.min_max_p_model(locations,loc_probabilities,time_min,n_samples)
        else:
            raise RuntimeError('No {} probability model. Possible choises probability1 or probability2'.format(model))
        opt = SolverFactory(solver)

        for key, val in solver_opts.iteritems():
            opt.options[key]=val

        solver_results = opt.solve(instance,tee=tee)
                
        objective_value = value(instance.expected_matches)
        tol = 1e-4
        nodes_to_sample = [(nid,time_min) for nid in instance.locations if abs(instance.x[nid].value-1.0)<tol]
        return nodes_to_sample,objective_value

    def run_probability_time_model(self,
                        locations,
                        loc_probability_list,
                        times,
                        n_samples,
                        solver='glpk',
                        solver_opts=dict(),
                        tee=False):
        instance = self.min_max_p_model_t(locations,loc_probability_list,times,n_samples)
        opt = SolverFactory(solver)

        for key, val in solver_opts.iteritems():
            opt.options[key]=val

        solver_results = opt.solve(instance,tee=tee)
                
        objective_value = value(instance.expected_matches)
        tol = 1e-4
        #instance.y.pprint()
        nodes_to_sample = [(nid,t) for nid in instance.locations  for t in times if abs(instance.y[nid,t].value-1.0)<tol]
        return nodes_to_sample,objective_value


    def enumerate_expected_matches(self,locations,loc_probabilities,time_min,n_samples):

        n_locations = len(locations)
        combinations = list(itertools.combinations(locations, n_samples))
        combination_to_Ematches = dict()
        for c in combinations:
            Ematches=0.0
            for s in self.list_events:
                Pmatch = 1.0
                for nid in c:
                    alpha = 1.0-loc_probabilities[nid]
                    if s.is_contaminated(nid,time_min):
                        alpha = loc_probabilities[nid]
                    if alpha<1e-6:
                        alpha=1e-6
                    Pmatch*=alpha
                Ematches+=Pmatch
            combination_to_Ematches[c] = Ematches

        return combination_to_Ematches

    def enumerate_min_max_match(self,locations,loc_probabilities,time_min,n_samples):

        n_locations = len(locations)
        combinations = list(itertools.combinations(locations, n_samples))
        combination_to_maxP = dict()
        for c in combinations:
            max_Pmatch = -10000.0
            for s in self.list_events:
                Pmatch = 0.0
                for nid in c:
                    alpha = 1.0-loc_probabilities[nid]
                    if s.is_contaminated(nid,time_min):
                        alpha = loc_probabilities[nid]
                    if alpha<1e-6:
                        alpha=1e-6
                    Pmatch+=log(alpha)
                if Pmatch>=max_Pmatch:
                    max_Pmatch=Pmatch
            combination_to_maxP[c] = max_Pmatch

        return combination_to_maxP

    
    def min_match_enumeration(self,locations,loc_probabilities,time_min,n_samples,model='E_number_s'):

        if model=='E_number_s':
            Ematches = self.enumerate_expected_matches(locations,loc_probabilities,time_min,n_samples)
        else:
            Ematches = self.enumerate_min_max_match(locations,loc_probabilities,time_min,n_samples)
        
        min_c, min_em  = min(Ematches.iteritems(), key=lambda x: x[1])

        return [(nid,time_min) for nid in min_c],min_em
        
    def random_locations(self,locations,time_min,n_samples):
        
        n_locations = len(locations)
        sample_tuples = [(locations[randint(0,n_locations-1)],time_min) for p in xrange(0,n_samples)]
        return sample_tuples,0.0

    def predefined_locations(self,locations,t):
        return  [(l,t) for l in locations],0.0


