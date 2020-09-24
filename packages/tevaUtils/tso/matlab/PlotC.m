function [cplot] = PlotC(NodeID,SpecieID)

setglobals;

Inode=strncmp(NODEID,NodeID,length(NodeID));
Ispecie=strncmp(SPECIEID,SpecieID,length(SpecieID));
cplot=C(Inode,:,Ispecie);
plot(cplot);
