@echo off

if "%1"== "" goto Error
if "%2"== "" goto Error

build -ceZ

REM copy /Y %2\chatpad_keyboard_kmdf.sys ..\hid_keyboard_interface\install
REM copy /Y %2\chatpad_keyboard_kmdf.pdb ..\hid_keyboard_interface\install

copy /Y %2\chatpad_keyboard_kmdf.sys ..\installer\install\%1\data\%2

goto End

:Error
echo.
echo Provide build arguments.  For example, "testbuild.bat winvista amd64"
echo The first argument can be winxp, winvista, or win7.
echo The second argument can be ia64, amd64, or i386.
echo.

:End
