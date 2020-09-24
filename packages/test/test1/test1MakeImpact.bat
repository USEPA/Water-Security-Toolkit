if exist test1.tso goto IMPACT

echo "Creating tso file..."
%bindir%\tevasim --tsg test1.tsg --tsi test1.tsi --tso test1.tso Net3.inp Net3.out

:IMPACT

echo "Creating impact file..."
%bindir%\tso2Impact --mc --vc --td --nfd --ec --epanetin Net3.inp test1 test1.tso
