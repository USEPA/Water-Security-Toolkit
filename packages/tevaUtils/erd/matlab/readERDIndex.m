function [err] = readERDIndex()

% This function reads in the index of the ERD database
% Matlab is a base-1 array index system, so we add 1 to all of the indices
% stored in the database.

err = 0;

SetERDGlobals;

% Read Quality Simulation Index data
QUALFILEIDX = zeros(1,QUALSIMCOUNT);
HYDSIMIDX = zeros(1,QUALSIMCOUNT);
QUALLENGTH = zeros(1,QUALSIMCOUNT);
QUALOFFSET = uint64(zeros(1,QUALSIMCOUNT));
QUALINPFNAME = {};
QUALMSXFNAME = {};
for i=1:QUALSIMCOUNT
    [QUALFILEIDX(i)] = fread(ERDindexFID,1,'int');
    [HYDSIMIDX(i)] = fread(ERDindexFID,1,'int');
    [QUALLENGTH(i)] = fread(ERDindexFID,1,'int');
    [QUALOFFSET(i)] = fread(ERDindexFID,1,'uint64');
    len = fread(ERDindexFID,1,'int');
    [X] = fread(ERDindexFID,len,'char');
    QUALINPFNAME{i} = char(X');
    len = fread(ERDindexFID,1,'int');
    [X] = fread(ERDindexFID,len,'char');
    QUALMSXFNAME{i} = char(X');
end
% Matlab is a base-1 array index system, so we add 1 to all of the indices
% stored in the database.
for i=1:QUALSIMCOUNT
    QUALFILEIDX(i) = QUALFILEIDX(i) + 1;
    HYDSIMIDX(i) = HYDSIMIDX(i) + 1;
end

% Read Hydraulics Simulation Index data
HYDFILEIDX = zeros(1,HYDSIMCOUNT);
HYDLENGTH = zeros(1,HYDSIMCOUNT);
HYDOFFSET = uint64(zeros(1,HYDSIMCOUNT));
for i=1:HYDSIMCOUNT
    [HYDFILEIDX(i)] = fread(ERDindexFID,1,'int');
    [HYDLENGTH(i)] = fread(ERDindexFID,1,'int');
    [HYDOFFSET(i)] = fread(ERDindexFID,1,'uint64');
end
% Matlab is a base-1 array index system, so we add 1 to all of the indices
% stored in the database.
for i=1:HYDSIMCOUNT
    HYDFILEIDX(i) = HYDFILEIDX(i) + 1;
end
