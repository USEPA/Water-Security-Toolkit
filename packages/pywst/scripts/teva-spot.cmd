@echo off
REM
REM A batch file that creates a DOS shell that is configured to 
REM have the TEVA-SPOT path pre-initialized.
REM
REM This batch file is customized to work within a virtualenv
REM environment.  This script can be called without using the virtualenv
REM activate.bat command.
REM

cd "%WST_HOME%"
set PATH=%WST_PYTHON%;"%WST_HOME%"\bin;%PATH%

echo Preparing to run TEVA-SPOT
echo If the following has spaces, please change directories
echo so that you are using ~ notation: e.g., "C:\progra~1" 
echo instead of "C:\Program Files"

cd

cmd & exit /b
