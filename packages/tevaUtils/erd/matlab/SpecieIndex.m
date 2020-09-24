function [index] = SpecieIndex(speciename)

SetTSOGlobals;

Ispecie=strcmp(SPECIEID,speciename);
index=find(Ispecie==1);
