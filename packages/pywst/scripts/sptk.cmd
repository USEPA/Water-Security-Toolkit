@echo off
REM
REM A batch file that calls the TEVA-SPOT sptk script.
REM
REM This batch file is customized to work within a virtualenv 
REM environment.  This script can be called without using the virtualenv
REM activate.bat command.
REM

REM set VIRTUAL_ENV=__VIRTUAL_ENV__
REM set PATH=%VIRTUAL_ENV%\bin;%PATH%
REM set SP_BIN=%VIRTUAL_ENV%\bin
REM set SP_PATH=%VIRTUAL_ENV%

@"%WST_PYTHON%"\python.exe "%WST_PYTHON%\\Scripts\\sptk" %* & exit /b
