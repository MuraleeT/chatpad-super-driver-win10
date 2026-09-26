@echo off

if "%1"=="" goto Error
if "%2"=="" goto Error

build -ceZ

if "%1"=="winxp" goto CopyExe

REM Add a manifest to request admin access.
REM Is there a better way to get the path to this?  This might not match the path on other people's systems.
"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0a\Bin\mt.exe" -manifest chatpad_control.exe.manifest -outputresource:"%2\chatpad_control.exe";#1

:CopyExe

copy /Y %2\chatpad_control.exe ..\installer\install\%1\chatpad_control_%2.exe
copy /Y chatpad_config.txt ..\installer\install\%1
copy /Y README.TXT ..\installer\install\%1

goto End

:Error
echo.
echo Provide build arguments.  For example, "testbuild.bat winvista amd64"
echo The first argument can be winxp, winvista, or win7.
echo The second argument can be ia64, amd64, or i386.
echo.

:End
