echo off
set bindir=..\..\..\bin
set moddir=..\mod

if not exist test1_ec.impact call test1MakeImpact

echo "Running SP solver..."

python %bindir%\sp --path=%bindir% --path=%moddir% --network=test1  --objective=mc --ub=ns,5 --solver=pico --invalid-ids=test1-invalid-junctions

