if exist test1b.tso goto IMPACT

echo "Creating bio tso file..."
%bindir%\tevasim --tsg test1b.tsg --tsi test1b.tsi --tso test1b.tso Net3.inp Net3.out

if exist test1c.tso goto IMPACT

echo "Creating bio tso file..."
%bindir%\tevasim --tsg test1c.tsg --tsi test1c.tsi --tso test1c.tso Net3.inp Net3.out

:IMPACT

echo "Creating impact file..."
%bindir%\tso2Impact --pe test1 test1b.tso test1c.tso ..\data\Net3\bio.tai ..\data\Net3\chem.tai
