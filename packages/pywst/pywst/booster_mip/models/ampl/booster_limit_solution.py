def create_solution_script(results_filename, objective_type):
    
    script = """
#Print the results to a yaml file
printf '"solve_result_num": %q\\n', solve_result_num >> """+results_filename+""";
printf '"solve_result": %q\\n', solve_result >> """+results_filename+""";
printf '"solve_exitcode": %q\\n', solve_exitcode >> """+results_filename+""";
printf '"booster ids": [' > """+results_filename+""";
for {b in S_BOOSTER_CANDIDATES} {
    if y_booster[b] > 0.9 then {
printf "%q,", b >> """+results_filename+""";
    }
}
printf ']\\n' >> """+results_filename+""";
printf '"expected mass consumed grams": %q\\n', MASS_CONSUMED_g >> """+results_filename+""";
printf '"mass consumed grams": {' > """+results_filename+""";
for {s in S_SCENARIOS} {
     printf '"%q":%q,', s, sum{n in S_NODES, t in S_TIMES: t >= P_SCEN_START_TIMESTEP[s] and  t <= P_SCEN_END_TIMESTEP[s] and P_DEMANDS_m3pmin[n,t] > 0}(P_DEMANDS_m3pmin[n,t]*c_toxin_gpm3[n,t,s])*P_MINUTES_PER_TIMESTEP > """+results_filename+""";
}
printf '}\\n' >> """+results_filename+""";
"""
    if objective_type == 'PD':
        script += """
printf '"expected population dosed": %q\\n', POPULATION_DOSED >> """+results_filename+""";
printf '"population dosed": {' > """+results_filename+""";
for {s in S_SCENARIOS} {
     printf '"%q":%q,', s, sum{n in S_NZP_NODES}(z_dosed[n,s]*P_NODE_POPULATION[n]) > """+results_filename+""";
}
printf '}\\n' >> """+results_filename+""";
"""
    elif objective_type != 'MC':
        raise ValueError

    script += """
# tank tox mass is the toxin mass left in tanks after reacting tank tox mass and tank chl mass
printf '"tank mass grams": {' > """+results_filename+""";
for {s in S_SCENARIOS} {
     printf '"%q":%q,', s, sum{n in S_TANKS}(max(0,c_toxin_gpm3[n,P_SCEN_END_TIMESTEP[s],s]-((1/P_ALPHA)*c_booster_gpm3[n,P_SCEN_END_TIMESTEP[s],s]))*P_TANK_VOLUMES_m3[n,P_SCEN_END_TIMESTEP[s]]) > """+results_filename+""";
}
printf '}\\n' >> """+results_filename+""";
"""
    return script

