function [Q,err] = ERDgetQualResultsByFile(specieID, hydfilename, qualfilename)
% Gets quality results for a particular specie ID, hydraulics input
% filename, and quality msx input filename

SetERDGlobals;

err = 0;

if isempty(hydfilename) 
    err = 1;
    return
end
if isempty(qualfilename) & ~isempty(QUALMSXFNAME)
    err = 1;
    return
end

% Find the quality simulation index, else return an error
% Get the indices of simulations using the hydraulics input file
simidx = 0;
hydusedidx = strmatch(hydfilename, QUALINPFNAME, 'exact');
if isempty(hydusedidx)
    err = 1;
    return
end
% Find the indices of simulations that also use the quality msx file
if ~isempty(qualfilename)
    qualusedidx = strmatch(qualfilename, QUALMSXFNAME, 'exact');
    if isempty(qualusedidx)
        err = 1;
        return
    end
    for i=1:length(hydusedidx)
        h = hydusedidx(i);
        for j=1:length(qualusedidx)
            if qualusedidx(j) == h
                simidx = h;
            end
        end
    end
else
    simidx = hydusedidx(1);
end
if simidx == 0
    err = 1;
    return
end

% Open the quality simulation database file and set the file pointer
% to the beginning of this simulation
qualfilename = QUALFILENAME{QUALFILEIDX(simidx)};
offset = QUALOFFSET(simidx);
ERDqualFID = fopen(qualfilename,'r');
if ERDqualFID == -1
   status = 1;
   return
end
% matlab apparently just understands 32 bit ints here
if fseek(ERDqualFID, uint32(offset), 'bof')
    err = 1;
    return
end

% find the index of the specie - results for each set are stored in this
% order.
specieidx = strmatch(specieID, SPECIEID, 'exact');
if isempty(specieidx)
    err = 1;
    return
end

% Get the quality information for the entire simulation first
[qsimidx] = fread(ERDqualFID,1,'int');


