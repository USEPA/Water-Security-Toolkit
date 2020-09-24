function [index] = NodeIndex(nodename)

SetTSOGlobals;

Inode=strcmp(NODEID,nodename);
index=find(Inode==1);
