function [result] = istank(i)

global NTANKS TANKINDEX

result = 0;
for j=1:NTANKS
   if i == TANKINDEX(j)
      result = 1;
   end
end
