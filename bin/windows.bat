echo on
call "%MSVC_PATH%\vcvarsall.bat" %MSVC_ARCH%

@echo =****= =****= WE ARE BUILDING A %BUILD% TYPE BUILD =****= =****=
@echo ====== CLEANING UP FROM LAST BUILD                                  =INFO=

rmdir /Q /S output
rmdir /Q /S build
rmdir /Q /S tmpspot
rmdir /Q /S tmplight
rmdir /Q /S offlinespot
mkdir output
xcopy checkout build /E /Q /Y /I

@echo ====== BUILDING TEVA-SPOT BINARIES                                  =INFO=
@echo ======   Building UTILIB binaries                                   =INFO=
cd %WORKSPACE%\build/tpl/acro/packages/utilib/mvs/nmake
nmake all

@echo ======   Building TEVA binaries                                     =INFO=
cd %WORKSPACE%\build\packages\nmake
nmake JDK_ROOT=%JAVA_HOME% /F makefile all 

@echo ======   Creating ZIP file: tevaspot_bin.zip                        =INFO=
nmake /F makefile zip 
xcopy /C /R /Y %WORKSPACE%\build\packages\nmake\tevaspot_bin.zip %WORKSPACE%\output
move %WORKSPACE%\output\tevaspot_bin.zip %WORKSPACE%\output\tevaspot_bin_%SPOT_PLATFORM%.zip
cd %WORKSPACE%

@echo ====== BUILDING TEVA-SPOT PYTHON LIGHT ============================ =INFO=
@echo ======   Creating ZIP file: tevaspot_py.zip                         =INFO=
cd %WORKSPACE%
set ORIGPATH=%PATH%
set PATH=%ORIGPATH%;%SVNPATH%;%PYTHONPATH%
python %WORKSPACE%\checkout\packages\tevaspot\util\spot_install --clear-config --config=https://software.sandia.gov/svn/teva/spot/vpy/instlight.ini --preinstall --%BUILD% --proxy=http://wwwproxy.sandia.gov tmplight
set PATH=%ORIGPATH%
xcopy /C /R /Y tmplight\tevaspot.zip %WORKSPACE%\output
move %WORKSPACE%\output\tevaspot.zip %WORKSPACE%\output\tevaspot_py_%SPOT_PLATFORM%.zip
rmdir /Q /S %WORKSPACE%\tmplight

@echo ====== BUILDING TEVA-SPOT PYTHON FULL ============================= =INFO=
@echo ======   Creating ZIP file: tevaspot_pydata.zip                     =INFO=
cd %WORKSPACE%
set ORIGPATH=%PATH%
set PATH=%ORIGPATH%;%SVNPATH%;%PYTHONPATH%
python build\packages\tevaspot\util\spot_install --preinstall --%BUILD% --proxy=http://wwwproxy.sandia.gov tmpspot
set PATH=%ORIGPATH%
xcopy /C /R /Y %WORKSPACE%\tmpspot\tevaspot.zip %WORKSPACE%\output
move %WORKSPACE%\output\tevaspot.zip %WORKSPACE%\output\tevaspot_py_data_%SPOT_PLATFORM%.zip

@echo ====== RUNNING TEVA-SPOT INSTALL TEST ============================= =INFO=
@echo ======   Installing binaries to local test directory                =INFO=
@echo =****=   YOU MUST HAVE tevaspot_acro.zip PRESENT IN C:\ RIGHT NOW   =WARN=
cd %WORKSPACE%

python build\packages\tevaspot\util\spot_install --%BUILD% --offline --zip=%WORKSPACE%\build\packages\nmake\tevaspot_bin.zip --zip=%WORKSPACE%\tevaspot_acro.zip --zip=%WORKSPACE%\tmpspot\tevaspot.zip offlinespot
@echo ======   Running TEVA-SPOT binary acceptance tests                  =INFO=
cd %WORKSPACE%\offlinespot\test
..\bin\python runtests
rmdir /Q /S %WORKSPACE%\tmpspot
