@echo off
REM
REM A batch file that calls the TEVA-SPOT sp script.
REM
REM This batch file is customized to work within a virtualenv 
REM environment.  This script can be called without using the virtualenv
REM activate.bat command.
REM

@"%WST_PYTHON%"\python.exe "%WST_PYTHON%\\Scripts\\sp" --path="%WST_PYTHON%\\Scripts" --path="%WST_HOME%\\bin" --path="%WST_HOME%\\etc\\mod" %* & exit /b
