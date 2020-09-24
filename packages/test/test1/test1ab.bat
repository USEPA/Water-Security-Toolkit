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

if not exist test1_mc.impact call test1MakeImpact

echo "Running SP solver..."

python %bindir%\sp --path=%bindir% --path=%moddir% --network=test1 --objective=mc --ub=ns,5 --solver=picoamp
