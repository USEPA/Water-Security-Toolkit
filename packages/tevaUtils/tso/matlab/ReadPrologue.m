function [] = ReadPrologue()

% This function reads in the prologue of the .tso output file

setglobals;

% Read network size
[NNODES] = fread(TSOFID,1,'int');
[NLINKS] = fread(TSOFID,1,'int');
[NTANKS] = fread(TSOFID,1,'int');
[NJUNCTIONS] = fread(TSOFID,1,'int');
[NSPECIES] = fread(TSOFID,1,'int');
[NSTEPS] = fread(TSOFID,1,'int');
[STEPSIZE] = fread(TSOFID,1,'float');
[FLTMAX] = fread(TSOFID,1,'float');
[QUALCODE] = fread(TSOFID,1,'int');
[NBULKSPECIES] = fread(TSOFID,1,'int');
[SIMDURATION] = fread(TSOFID,1,'long');
[REPORTSTART] = fread(TSOFID,1,'long');
[REPORTSTEP] = fread(TSOFID,1,'long');

% Read Tank indices
[TANKINDEX] = fread(TSOFID,NTANKS,'int');

% Read Node IDs
[X] = fread(TSOFID,[NBYTESFORNODEID,NNODES],'char');
NODEID = char(X');
NODEID = cellstr(NODEID);

% Read Link IDs
[X] = fread(TSOFID,[NBYTESFORNODEID,NLINKS],'char');
LINKID = char(X');
LINKID = cellstr(LINKID);

% Read Specie IDs
[X] = fread(TSOFID,[16,NSPECIES],'char');
SPECIEID = char(X');
SPECIEID = cellstr(SPECIEID);

% Read Node coordinates
[XNODE] = fread(TSOFID,NNODES,'float');
[YNODE] = fread(TSOFID,NNODES,'float');
noXY = sum(XNODE==FLTMAX);
if noXY > 0
   fprintf(1,'Warning: %d nodes have no coordinates.\n',noXY);
end

% Read link/node topology
[FROMNODE] = fread(TSOFID,NLINKS,'int');
[TONODE] = fread(TSOFID,NLINKS,'int');

% Read link lengths
[LENGTH] = fread(TSOFID,NLINKS,'float');

% Read Link Vertices
VERTX = sparse(NLINKS,10);
VERTY = sparse(NLINKS,10);
for i=1:NLINKS
    NV(i) = fread(TSOFID,1,'int');
    if NV(i)>0
        VERTX(i,1:NV(i)) = fread(TSOFID,NV(i),'float');
        VERTY(i,1:NV(i)) = fread(TSOFID,NV(i),'float');
    end
end

% Read node demands
[D] = fread(TSOFID,[NNODES,NSTEPS],'float');

% Read link flows
[Q] = fread(TSOFID,[NLINKS,NSTEPS],'float');

% Read link velocities
[V] = fread(TSOFID,[NLINKS,NSTEPS],'float');
