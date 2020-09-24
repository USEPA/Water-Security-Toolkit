@echo off
REM the purpose of this script is to determine if nmake was launched from a 32 or 64 bit
REM Visual Studio Environment.  This should have been possible in the makefile by using
REM the preprocessing directive (the syntax below evaluates the command and tests the return code):
REM   !if [@echo "%LIBPATH%" ^| find /I "lib\amuuud64"] != 0
REM   BITS=32
REM   !else
REM   BITS=64
REM   !endif
REM
REM  But the return code was apparently always 0 even though testing in the command prompt showed otherwise

@echo "%LIBPATH%" | %WINDIR%\System32\find /I "lib\amd64"
