function [index] = NodeIndex(nodename)

setglobals;

Inode=strcmp(NODEID,nodename);
index=find(Inode==1);
