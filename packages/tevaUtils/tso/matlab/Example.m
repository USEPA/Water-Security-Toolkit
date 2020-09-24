% Example use of TSO matlab functions to animate concentrations stored in
% a TSO file.
% The example animates the scenario in file 'network.tso' with injection
% node ID 'N4000'

% Matlab TSO variables are shared globally
setglobals;

% Open the tso file and read the prologue
% Note that matlab TSO can only handle TSO storage scheme 0 or 1
% (i.e. use '-s 0' or '-s 1' argument on tevasim command line)
OpenTSO('network.tso');
ReadPrologue;

% This while loop finds the right scenario if it exists
snodeid='N4000';
found=0;
while ReadNextC==0
    % NODEID is a global array of node IDs and NODEINDEX is a list of
    % injection node indices
    if strcmp(NODEID(NODEINDEX),snodeid) 
        % Found a match
        found=1;
        break;
    end
end
if ~found
    disp 'Scenario not found'
    return;
end

% We're going to plot the concentrations for the 'Unknown' specie ID
is=0;
for i=1:NSPECIES
    if strcmp(SPECIEID(i),'Unknown')
        is=i;
        break;
    end
end
if is==0
    disp 'Did not find specie ID'
    return
end

% Animate starting with time step 50 time steps
tframe = [50 length(C(1,:,is))]

% Min and Max value of Specie concentrations for plotting
cmin=min(min(C(:,tframe(1):tframe(2),is)));
cmax=max(max(C(:,tframe(1):tframe(2),is)));

% Set up a cell array of Node IDs where we want to plot special symbols
sampleid = [{'Res2'} {'1531_AB'} {'2002_AC'} {'1711_AD'} {'1570_AE'} {'6668_AF'} {'1473_AG'} {'1324_AH'} {'5948_AI'}];
% And set the corresponding symbols, symbol sizes, and symbol colors
symbol=['v' '^' '^' '^' '^' '^' '^' '^' '^'];
ssize=[8 8 8 8 8 8 8 8 8];
scolor=['r' 'g' 'g' 'g' 'g' 'g' 'g' 'g' 'g'];

% The AVI movie file name - avi extension
mfilename='networkmovie';

% This makes the movie.  See NetworkMovie for argument definitions (hope
% they are all actually there.  You'll be shown a network diagram with the
% first frame, and afforded the opportunity to size/magnify the picture as
% you wish.  Then press any key in the command window and don't obscure the
% figure window until all frames are drawn (else the obstruction will
% appear in the movie frames).
NetworkMovie([],mfilename,80,10,'hot','Unknown','n',cmin,cmax,2,3,3,'y',tframe,...
    sampleid,symbol,ssize,scolor);

