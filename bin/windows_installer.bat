echo on

@echo =****= =****= WE ARE BUILDING A %BUILD% TYPE BUILD =****= =****=
@echo ====== CLEANING UP FROM LAST BUILD                                  =INFO=

rmdir /Q /S output
rmdir /Q /S build
rmdir /Q /S tevaspot
mkdir build
mkdir output

set SPOT_VERSION=%MAJOR%.%MINOR%.%BUILD_TYPE%.%BUILD_NUMBER%

IF NOT EXIST tevaspot_bin_win32.zip "%GNUWIN32%"\bin\wget -nv http://hudson.sandia.gov:8080/job/spot_%BUILD%_win32/lastSuccessfulBuild/artifact/output/tevaspot_bin_win32.zip
IF NOT EXIST tevaspot_py_win32.zip "%GNUWIN32%"\bin\wget -nv http://hudson.sandia.gov:8080/job/spot_%BUILD%_win32/lastSuccessfulBuild/artifact/output/tevaspot_py_win32.zip
IF NOT EXIST tevaspot_py_data_win32.zip "%GNUWIN32%"\bin\wget -nv http://hudson.sandia.gov:8080/job/spot_%BUILD%_win32/lastSuccessfulBuild/artifact/output/tevaspot_py_data_win32.zip
IF NOT EXIST tevaspot_bin_win64.zip "%GNUWIN32%"\bin\wget -nv http://hudson.sandia.gov:8080/job/spot_%BUILD%_win64/lastSuccessfulBuild/artifact/output/tevaspot_bin_win64.zip
IF NOT EXIST tevaspot_py_win64.zip "%GNUWIN32%"\bin\wget -nv http://hudson.sandia.gov:8080/job/spot_%BUILD%_win64/lastSuccessfulBuild/artifact/output/tevaspot_py_win64.zip
IF NOT EXIST tevaspot_py_data_win64.zip "%GNUWIN32%"\bin\wget -nv http://hudson.sandia.gov:8080/job/spot_%BUILD%_win64/lastSuccessfulBuild/artifact/output/tevaspot_py_data_win64.zip
IF NOT EXIST tevaspot_acro.zip COPY C:\tevaspot_acro.zip %WORKSPACE%

xcopy /C /R /Y %WORKSPACE%\tevaspot_py_win32.zip %WORKSPACE%\build
xcopy /C /R /Y %WORKSPACE%\tevaspot_py_data_win32.zip %WORKSPACE%\build
xcopy /C /R /Y %WORKSPACE%\tevaspot_bin_win32.zip %WORKSPACE%\build
xcopy /C /R /Y %WORKSPACE%\tevaspot_py_win64.zip %WORKSPACE%\build
xcopy /C /R /Y %WORKSPACE%\tevaspot_py_data_win64.zip %WORKSPACE%\build
xcopy /C /R /Y %WORKSPACE%\tevaspot_bin_win64.zip %WORKSPACE%\build
xcopy /C /R /Y %WORKSPACE%\tevaspot_acro.zip %WORKSPACE%\build

del /Q /F %WORKSPACE%\*.zip

@echo ====== BUILDING TEVA-SPOT WINDOWS INSTALLER ======================= =INFO=
@echo ======   Creating exe files                                         =INFO=
cd %WORKSPACE%\admin
"%INNOSETUP%"\ISCC.exe teva-spot-win-debug.iss /dMySetupVersion=%SETUP_VERSION% /dMyAppVersion=%SPOT_VERSION% /dMyAppRevision="(%SVN_REVISION%) %BUILD%"
"%INNOSETUP%"\ISCC.exe teva-spot-win.iss /dMySetupVersion=%SETUP_VERSION% /dMyAppVersion=%SPOT_VERSION% /dMyAppRevision="(%SVN_REVISION%) %BUILD%"

@echo ====== TESTING TEVA-SPOT WINDOWS INSTALLER ======================== =INFO=
REM %WORKSPACE%\output\teva-spot-%MAJOR%.%MINOR%-debug.exe /SP- /SILENT /DIR="%WORKSPACE\tevaspot"
REM cd %WORKSPACE%\tevaspot\test
REM ..\bin\python runtests
