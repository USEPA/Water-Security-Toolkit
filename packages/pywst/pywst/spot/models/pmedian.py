
from pyomo.environ import *


def pyomo_create_model(options):
    model = Model()

    model.Locations = Set()

    model.P = Param()

    model.Customers = Set()

    model.ValidIndices = Set(within=model.Locations*model.Customers)

    model.d = Param(model.Locations, model.Customers, within=Reals)

    model.x = Var(model.Locations, model.Customers, bounds=(0.0,1.0))

    model.y = Var(model.Locations, within=Binary)

    def obj_rule(model):
        return sum(model.d[n,m]*model.x[n,m] for (n,m) in model.ValidIndices)/len(model.Customers)
    model.obj = Objective(rule=obj_rule)

    def single_x_rule(m, model):
        return sum(model.x[n,m] for n in model.Locations if (n,m) in model.ValidIndices) == 1.0
    model.single_x = Constraint(model.Customers, rule=single_x_rule)

    def bound_y_rule(n,m,model):
        return model.x[n,m] <= model.y[n]
    model.bound_y = Constraint(model.ValidIndices, rule=bound_y_rule)

    def num_facilities_rule(model):
        return sum(model.y[n] for n in model.Locations if n != -1) <=  model.P
    model.num_facilities = Constraint(rule=num_facilities_rule)

    return model

