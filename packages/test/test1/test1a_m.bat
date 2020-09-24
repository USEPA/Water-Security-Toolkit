echo off
REM Paths depend are whether we are running in a binary installation
REM directory or in a source/build/dist directory

if exist ..\..\python goto BINARY_INSTALL

set bindir=..\..\..\bin
set moddir=..\..\..\etc\mod

goto MAKE_IMPACT

:BINARY_INSTALL
set bindir=..\..\bin
set moddir=..\..\mod

:MAKE_IMPACT

if not exist test1_pe.impact call test1mMakeImpact

echo "Running SP solver..."

python %bindir%\sp --path=%bindir% --path=%moddir% --network=test1 --objective=mc --ub=ns,5 --solver=lagrangian

%bindir%\scenarioAggr --numEvents=472 test1_pe.impact

copy test1.nodemap aggrtest1.nodemap

python %bindir%\sp --path=%bindir% --path=%moddir% --network=aggrtest1 --objective=pe --ub=ns,5 --solver=lagrangian
