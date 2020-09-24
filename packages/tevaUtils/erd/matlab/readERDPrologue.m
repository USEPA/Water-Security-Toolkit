function [err] = readERDPrologue()

% This function reads in the prologue of the ERD database

err = 0;

SetERDGlobals;

% Number of simulations in database
[HYDSIMCOUNT] = fread(ERDprologueFID,1,'int');
[QUALSIMCOUNT] = fread(ERDprologueFID,1,'int');

% Read network size
[NNODES] = fread(ERDprologueFID,1,'int');
[NLINKS] = fread(ERDprologueFID,1,'int');
[NTANKS] = fread(ERDprologueFID,1,'int');
[NJUNCTIONS] = fread(ERDprologueFID,1,'int');
[NSPECIES] = fread(ERDprologueFID,1,'int');
[NSTEPS] = fread(ERDprologueFID,1,'int');
[STEPSIZE] = fread(ERDprologueFID,1,'float');
[FLTMAX] = fread(ERDprologueFID,1,'float');
[QUALCODE] = fread(ERDprologueFID,1,'int');
[SIMDURATION] = fread(ERDprologueFID,1,'int');
[REPORTSTART] = fread(ERDprologueFID,1,'int');
[REPORTSTEP] = fread(ERDprologueFID,1,'int');

% Control link information
[NUMCONTROLLINKS] = fread(ERDprologueFID,1,'int');
[CONTROLLINKS] = fread(ERDprologueFID,NUMCONTROLLINKS,'int');

% Read Tank indices
[TANKINDEX] = fread(ERDprologueFID,NTANKS,'int');

% Read Node IDs
[X] = fread(ERDprologueFID,[NBYTESFORNODEID,NNODES],'char');
NODEID = char(X');
NODEID = cellstr(NODEID);

% Read Link IDs
[X] = fread(ERDprologueFID,[NBYTESFORNODEID,NLINKS],'char');
LINKID = char(X');
LINKID = cellstr(LINKID);

% Read Specie IDs
SPECIEIDX = zeros(1,NSPECIES);
SPECIETYPE = zeros(1,NSPECIES);
X = zeros(NBYTESFORNODEID,NSPECIES);
for i=1:NSPECIES
    [SPECIEIDX(i)] = fread(ERDprologueFID,1,'int');
    [SPECIETYPE(i)] = fread(ERDprologueFID,1,'int');
    [X(:,i)] = fread(ERDprologueFID,NBYTESFORNODEID,'char');
end
SPECIEID = char(X');
SPECIEID = cellstr(SPECIEID);

% Read Node coordinates
[XNODE] = fread(ERDprologueFID,NNODES,'float');
[YNODE] = fread(ERDprologueFID,NNODES,'float');
noXY = sum(XNODE==FLTMAX);
if noXY > 0
   fprintf(1,'Warning: %d nodes have no coordinates.\n',noXY);
end

% Read link/node topology
[FROMNODE] = fread(ERDprologueFID,NLINKS,'int');
[TONODE] = fread(ERDprologueFID,NLINKS,'int');

% Read link lengths
[LENGTH] = fread(ERDprologueFID,NLINKS,'float');

% Read Link Vertices
VERTX = sparse(NLINKS,10);
VERTY = sparse(NLINKS,10);
for i=1:NLINKS
    NV(i) = fread(ERDprologueFID,1,'int');
    if NV(i) > 0
        VERTX(i,1:NV(i)) = fread(ERDprologueFID,NV(i),'float');
        VERTY(i,1:NV(i)) = fread(ERDprologueFID,NV(i),'float');
    end
end

% Read node demand patterns
if ISSET_DEMANDPROFILE
    PATTERNLENGTH = zeros(1,NNODES);
    PATTERN = sparse(NNODES,24);
    for i=1:NNODES
         PATTERNLENGTH(i) = fread(ERDprologueFID,1,'int');
         if PATTERNLENGTH(i) > 0
             PATTERN(i,1:PATTERNLENGTH(i)) = fread(ERDprologueFID,PATTERNLENGTH(i),'float');
         end
    end
end

% Read hydraulic and quality database file names
eof = 0;
nhydfile = 0;
nqualfile = 0;
while ~eof
    % Read in the length of the next filename
    [len] = fread(ERDprologueFID,1,'int');
    eof = feof(ERDprologueFID);
    if eof
        % No more filenames
        break
    else
        % Read in the next filename
        [X] = fread(ERDprologueFID,len,'char');
        cX = char(X');
    end
    % Determine whether its a hydraulic or quality database file
    if strfind(cX, 'hyd.erd')
        % hydraulics database file
        nhydfile = nhydfile + 1;
        HYDFILENAME{nhydfile} = cX;
    elseif strfind(cX, 'qual.erd')
        % quality database file
        nqualfile = nqualfile + 1;
        QUALFILENAME{nqualfile} = cX;
    else
        err = 1;
        return
    end
end
