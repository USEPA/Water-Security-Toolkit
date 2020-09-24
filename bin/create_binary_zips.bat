cd build\packages\nmake
nmake /F makefile zip
xcopy /C /R /Y tevaspot_bin.zip ..\..\..
cd ..\..\..
move tevaspot_bin.zip tevaspot_bin_win.zip
