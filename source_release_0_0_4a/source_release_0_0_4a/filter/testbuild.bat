@echo off

if "%1"== "" goto Error
if "%2"== "" goto Error

build -ceZ

REM copy /Y %2\chatpad_filter.sys install 
REM copy /Y %2\chatpad_filter.pdb install 

copy /Y %2\chatpad_filter.sys ..\installer\install\%1\data\%2
if "%1"=="winxp" copy /Y install\winxp_chatpad_filter.inf ..\installer\install\%1\data\chatpad_filter.inf
if "%1"=="winvista" copy /Y install\winvista_chatpad_filter.inf ..\installer\install\%1\data\chatpad_filter.inf
if "%1"=="win7" copy /Y install\win7_chatpad_filter.inf ..\installer\install\%1\data\chatpad_filter.inf

goto End

:Error
echo.
echo Provide build arguments.  For example, "testbuild.bat winvista amd64"
echo The first argument can be winxp, winvista, or win7.
echo The second argument can be ia64, amd64, or i386.
echo.

:End
