function [status] = ERDopen(erdbasename)
% ERDopen(erdbasename)
% Opens the erd database with basename 'erdbasename' and processes the
% prologue, storing results in global variables defined by SetERDGlobals.

SetERDGlobals;
initERDGlobals;
SetERDGlobals;

status = 0;

% File version we know how to read
VALIDFILEVERSION = 11;
% Size of __file_pos_t on computer the database was written on 
% (probably 64 bit int)
FILEPOS = 'int64';
% Maximum filename length used by ERD
MAXFNAME = 1024;

% Open ERD prologue
ERDprologueFile = [erdbasename '.prologue.erd'];
ERDprologueFID = fopen(ERDprologueFile,'r');
if ERDprologueFID == -1
   status = 1;
   return
end

% Header
[HEADER] = fread(ERDprologueFID,32,'uint8');
FILEVERSION = HEADER(2);
NBYTESFORNODEID = HEADER(4) + 1;  % Add the NULL terminator here
velocity_on = uint8(hex2dec('04'));
flow_on = uint8(hex2dec('02'));
demand_on = uint8(hex2dec('01'));
pressure_on = uint8(hex2dec('08'));
profile_on = uint8(hex2dec('10'));
ISSET_VELOCITY = logical(bitand(HEADER(6),velocity_on));
ISSET_FLOW = logical(bitand(HEADER(6),flow_on));
ISSET_DEMAND = logical(bitand(HEADER(6),demand_on));
ISSET_PRESSURE = logical(bitand(HEADER(6),pressure_on));
ISSET_DEMANDPROFILE = logical(bitand(HEADER(6),profile_on));

if FILEVERSION ~= VALIDFILEVERSION
    status = 2;
    return
end

% Read rest of prologue
readERDPrologue();

% Open ERD Index
ERDindexFile = [erdbasename '.index.erd'];
ERDindexFID = fopen(ERDindexFile,'r');
if ERDindexFID == -1
   status = 1;
   return
end

% Read index
readERDIndex();

