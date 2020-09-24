echo on

@echo =****= =****= WE ARE BUILDING A %BUILD% TYPE BUILD =****= =****=
@echo ====== CLEANING UP FROM LAST BUILD                                  =INFO=
rmdir /Q /S output
rmdir /Q /S tmpspot
rmdir /Q /S tmplight
rmdir /Q /S offlinespot
mkdir output

@echo ====== BUILDING TEVA-SPOT PYTHON FULL ============================= =INFO=
@echo ======   Creating ZIP file: tevaspot_py_data.zip                    =INFO=
cd %WORKSPACE%
set ORIGPATH=%PATH%
set PATH=%ORIGPATH%;%SVNPATH%;%PYTHONPATH%
python %WORKSPACE%\checkout\packages\tevaspot\util\spot_install --preinstall --%BUILD% --proxy=http://wwwproxy.sandia.gov tmpspot
set PATH=%ORIGPATH%
xcopy /C /R /Y %WORKSPACE%\tmpspot\tevaspot.zip %WORKSPACE%\output
move %WORKSPACE%\output\tevaspot.zip %WORKSPACE%\output\tevaspot_py_data.zip

@echo ====== TESTING TEVA-SPOT PYTHON =================================== =INFO=
@echo =****=   Currently disabled                                         =WARN=
