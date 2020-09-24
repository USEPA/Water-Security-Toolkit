function [index] = SpecieIndex(speciename)

setglobals;

Ispecie=strcmp(SPECIEID,speciename);
index=find(Ispecie==1);
