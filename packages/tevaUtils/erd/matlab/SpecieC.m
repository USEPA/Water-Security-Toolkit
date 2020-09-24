function [cs] = SpecieC(SpecieID)

SetTSOGlobals;

Ispecie=SpecieIndex(SpecieID);
cs=C(:,:,Ispecie);
