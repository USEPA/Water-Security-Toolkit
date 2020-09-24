function [cs] = SpecieC(SpecieID)

setglobals;

Ispecie=SpecieIndex(SpecieID);
cs=C(:,:,Ispecie);
