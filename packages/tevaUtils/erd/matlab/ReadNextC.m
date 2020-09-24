function [status] = ReadNextC()

% This function reads concentrations from the .tso output file

SetTSOGlobals;

status = 0;

% Read number of contaminant sources (i.e. injection locations)
[NSOURCES,nr] = fread(TSOFID,1,'int');

% nr = 1 means data was read successfully and there is another scenario to read
if nr == 1,   
    for is=1:NSOURCES
        %Read source info
        [NODEINDEX(is)] = fread(TSOFID,1,'int'); 
        [SPECIESINDEX(is)] = fread(TSOFID,1,'int'); 
        [SOURCETYPE(is)] = fread(TSOFID,1,'int'); 
        [SOURCESTRENGTH(is)] = fread(TSOFID,1,'float'); 
        [SOURCESTART(is)] = fread(TSOFID,1,'long'); 
        [SOURCESTOP(is)] = fread(TSOFID,1,'long'); 
    end
    
   %Read concentrations
   C = zeros(NNODES,NSTEPS,NSTOREDSPECIES);
   for is=1:NSTOREDSPECIES
       if STORAGEMETHOD == 0
           [C(:,:,is)] = fread(TSOFID,[NNODES,NSTEPS],'float');
       elseif STORAGEMETHOD == 1
           [nnz] = fread(TSOFID,1,'int');
           for in=1:nnz
               [n] = fread(TSOFID,1,'int');
               [C(n,:,is)] = fread(TSOFID,NSTEPS,'float');
           end
       else
           disp('Storage method error in ReadNextC');
           status = 1;
           return
       end
       status = 0;
   end
else
   status = 1;
end   

