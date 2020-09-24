def create_solution_script(results_filename, objective_type):
    script = """
#Print the results to a yaml file
printf '"booster ids": [' > """+results_filename+""";
for {b in S_BOOSTER_CANDIDATES} {
    if y_booster[b] > 0.9 then {
        printf "%q,", b >> """+results_filename+""";
    }
}
printf ']\\n' >> """+results_filename+""";
"""
    if objective_type == 'MC':
        script += """
printf '"expected mass consumed grams": %q\\n', MASS_CONSUMED_g >> """+results_filename+""";
"""
    elif objective_type == 'PD':
        script += """
printf '"expected population dosed": %q\\n', POPULATION_DOSED >> """+results_filename+""";
"""
    script += """
printf '"solve_result_num": %q\\n', solve_result_num >> """+results_filename+""";
printf '"solve_result": %q\\n', solve_result >> """+results_filename+""";
printf '"solve_exitcode": %q\\n', solve_exitcode >> """+results_filename+""";
    """
    return script
