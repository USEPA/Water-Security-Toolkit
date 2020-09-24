function [status] = OpenTSO(tsofile)
setglobals;

status = 0;
VALIDFILEVERSION = 7;

% Open threat simulation output file
TSOFID = fopen(tsofile,'r');
if TSOFID == -1
   status = 1;
   return
end

% Header
[HEADER] = fread(TSOFID,32,'char');
FILEVERSION = HEADER(2);
STORAGEMETHOD = HEADER(3);
NBYTESFORNODEID = HEADER(4);

if STORAGEMETHOD > 1 || FILEVERSION ~= VALIDFILEVERSION
    status = 2;
    return
end