function [cplot] = PlotC(NodeID,SpecieID)

SetTSOGlobals;

Inode=strncmp(NODEID,NodeID,length(NodeID));
Ispecie=strncmp(SPECIEID,SpecieID,length(SpecieID));
cplot=C(Inode,:,Ispecie);
plot(cplot);
