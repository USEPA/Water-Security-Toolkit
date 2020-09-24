
from pyomo.environ import *


def pyomo_create_model(options):
    model = Model()

    model.Locations = Set()

    model.P = Param()

    model.Customers = Set()

    model.TuningLevels = Set()

    model.fp = Param(model.Locations, model.TuningLevels)

    model.fp_threshold = Param()

    model.far = Var()

    model.ValidIndices = Set(within=model.Locations*model.Customers*model.TuningLevels)

    model.d = Param(model.Locations, model.Customers, model.TuningLevels, within=Reals)

    model.x = Var(model.Locations, model.Customers, model.TuningLevels, bounds=(0.0,1.0))

    model.y = Var(model.Locations, model.TuningLevels, within=Binary)

    def obj_rule(model):
        return sum(model.d[n,m,l]*model.x[n,m,l] for (n,m,l) in model.ValidIndices)/len(model.Customers)
    model.obj = Objective(rule=obj_rule)

    def single_x_rule(m, model):
        return sum(model.x[n,m,l] for l in model.TuningLevels for n in model.Locations if (n,m,l) in model.ValidIndices) == 1.0
    model.single_x = Constraint(model.Customers, rule=single_x_rule)

    def bound_y_rule(n,m,l,model):
        return model.x[n,m,l] <= model.y[n,l]
    model.bound_y = Constraint(model.ValidIndices, rule=bound_y_rule)

    def num_facilities_rule(model):
        return sum(model.y[n,l] for l in model.TuningLevels for n in model.Locations if n != -1) <=  model.P
    model.num_facilities = Constraint(rule=num_facilities_rule)

    def location_rule(n,model):
        return sum(model.y[n,l] for l in model.TuningLevels) <= 1
    model.location = Constraint(model.Locations, rule=location_rule)

    def far_defn_rule(model):
        return model.far == sum(model.fp[n,l]*model.y[n,l] for n in model.Locations for l in model.TuningLevels if n != -1)
    model.far_defn = Constraint(rule=far_defn_rule)

    def fp_limit_rule(model):
        return model.far <= model.fp_threshold
    model.fp_limit = Constraint(rule=fp_limit_rule)

    return model

