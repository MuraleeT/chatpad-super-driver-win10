@echo off

if "%1"== "" goto Error
if "%2"== "" goto Error

if not exist installer\install mkdir installer\install

if not exist installer\install\win7 mkdir installer\install\win7
if not exist installer\install\win7\data mkdir installer\install\win7\data
if not exist installer\install\win7\data\ia64 mkdir installer\install\win7\data\ia64
if not exist installer\install\win7\data\amd64 mkdir installer\install\win7\data\amd64
if not exist installer\install\win7\data\i386 mkdir installer\install\win7\data\i386

if not exist installer\install\winvista mkdir installer\install\winvista
if not exist installer\install\winvista\data mkdir installer\install\winvista\data
if not exist installer\install\winvista\data\ia64 mkdir installer\install\data\winvista\data\ia64
if not exist installer\install\winvista\data\amd64 mkdir installer\install\data\winvista\data\amd64
if not exist installer\install\winvista\data\i386 mkdir installer\install\data\winvista\data\i386

if not exist installer\install\winxp mkdir installer\install\winxp
if not exist installer\install\winxp\data mkdir installer\install\winxp\data
if not exist installer\install\winxp\data\i386 mkdir installer\install\winxp\data\i386

cd filter
call testbuild.bat %1 %2
pause

cd ..\hid_keyboard_interface
call testbuild.bat %1 %2
pause

cd ..\chatpad_keyboard_kmdf
call testbuild.bat %1 %2
pause

cd ..\hid_mouse_interface
call testbuild.bat %1 %2
pause

cd ..\chatpad_mouse_kmdf
call testbuild.bat %1 %2
pause

cd ..\installer
call testbuild.bat %1 %2
pause

cd ..\chatpad_control
call testbuild.bat %1 %2

cd ..

copy /Y LICENSE.TXT installer\install\%1

echo.
echo ***** Build finished *****
echo.
echo NOTE THAT THE WDF COINSTALLER FILES ARE NOT AUTOMATICALLY
echo COPIED INTO THE INSTALL DIRECTORIES!  YOU MAY NEED TO DO
echo THIS BY HAND IF THE INSTALL DIRECTORIES WERE DELETED!
echo.

goto End

:Error
echo.
echo Provide build arguments.  For example, "testbuild.bat winvista amd64"
echo The first argument can be winxp, winvista, or win7.
echo The second argument can be ia64, amd64, or i386.
echo.

:End
