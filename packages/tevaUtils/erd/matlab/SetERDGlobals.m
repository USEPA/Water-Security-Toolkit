% This is all global data shared between the matlab functions for reading
% ERD databases

% Header
global  HEADER VALIDFILEVERSION FILEVERSION NBYTESFORNODEID ISSET_VELOCITY ISSET_FLOW ISSET_DEMAND ISSET_PRESSURE ISSET_DEMANDPROFILE;
% Potential causes of screwing up the database reads
global  FILEPOS MAXFNAME;
% Network size info
global  HYDSIMCOUNT QUALSIMCOUNT NNODES NLINKS NTANKS NJUNCTIONS NSTEPS NSPECIES QUALCODE NSTOREDSPECIES SIMDURATION REPORTSTART REPORTSTEP FLTMAX STEPSIZE;
% Control link info
global  NUMCONTROLLINKS CONTROLLINKS;
% Patterns
global  PATTERN PATTERNLENGTH;
% Network objects
global  TANKINDEX NODEID LINKID SPECIEIDX SPECIETYPE SPECIEID XNODE YNODE FROMNODE TONODE LENGTH NV VERTX VERTY;
% Database file names
global  HYDFILENAME QUALFILENAME;
% Hydraulic simulation data
global  D Q V P HYDFILEIDX HYDLENGTH HYDOFFSET;
% Quality simulatoin data
global  C QUALFILEIDX HYDSIMIDX QUALLENGTH QUALOFFSET QUALINPFNAME QUALMSXFNAME;
% Matlab file IDs
global  ERDprologueFID ERDindexFID ERDhydFID ERDqualFID;