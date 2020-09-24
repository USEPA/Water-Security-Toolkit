rem   This script creates the python zips in the directory "spot_py_nodata".  It MUST
rem   be run from the base directory (such as trunk), i.e., the parent of the "packages" 
rem   directory, as it will *NOT* know any different.

python packages\tevaspot\util\spot_install --clear-config --config=https://software.sandia.gov/svn/teva/spot/vpy/instlight.ini --preinstall --trunk --proxy=%http_proxy% spot_py_nodata

xcopy /C /R /Y spot_py_nodata\tevaspot.zip  .
move  tevaspot.zip tevaspot_py_win.zip

python packages\tevaspot\util\spot_install --preinstall --trunk --proxy=%http_proxy% spot_py_withdata

xcopy /C /R /Y spot_py_withdata\tevaspot.zip .
move  tevaspot.zip tevaspot_py_data_win.zip

