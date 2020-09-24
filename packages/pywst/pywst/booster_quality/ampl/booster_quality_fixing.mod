# 
# All parameter and variables names are appended
# with unit where appropriate. 
#  _gpmin  = grams per minute
#  _gpm3   = grams per meter cubed (mg/L)
#  _m3pmin = meters cubed per minute
#  _g      = grams
#

# Junctions not in the list of booster candidates must inject zero
for {n in (S_NODES diff (S_BOOSTER_CANDIDATES union S_SOURCES_BOOSTER)), t in S_TIMES_QUALITY} {
    fix m_booster_gpmin[n,t] := 0;
}

# No flow fix mass injected to zero 
for {n in S_NODES, t in S_TIMES} {
    if (P_INJ_MATRIX_DIAG[n,t] == 0) then {
       fix m_booster_gpmin[n,t] := 0;
    }
}

# source initial quality, **stays constant
for {n in S_SOURCES_BOOSTER, t in S_TIMES_QUALITY} {
    fix c_booster_gpm3[n,t] := P_SOURCE_CONC_BOOSTER_gpm3;
}

# don't try to fix source concentrations at no flow regions
# where the merlion linear system implicitly forces the concentration to zero
for {n in S_NODES, t in S_TIMES} {
    if ((P_INJ_MATRIX_DIAG[n,t] == 0) && (card(S_CONC_MATRIX_CSR_INDEX[n,t]) == 1)) then {
       #fix c_booster_gpm3[n,t] := 0;
       if (t in S_TIMES_QUALITY) then {
           drop CONCENTRATION_BOUNDS_NODES_MIN[n,t];
       }
    }
}

