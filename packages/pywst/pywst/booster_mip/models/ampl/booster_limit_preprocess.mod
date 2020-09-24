
for {n in S_NZP_NODES, s in S_SCENARIOS} {
    if (P_MAX_DOSE_g[n,s]*(1+EPS) < P_DOSE_THRESHOLD_g) then {
        fix z_dosed[n,s] := 0;
    }
}
