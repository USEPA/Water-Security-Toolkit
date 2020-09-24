% Read link flows
[Q] = fread(ERDPrologueFID,[NLINKS,NSTEPS],'float');

% Read link velocities
[V] = fread(ERDPrologueFID,[NLINKS,NSTEPS],'float');
