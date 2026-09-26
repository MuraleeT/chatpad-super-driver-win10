//
// Chatpad driver installation program that works with a KMDF USB filter driver, by
//   GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//
// The operation of this installer is based on how the devcon example utility works.
// All source code in this file was written by hand.
//

#include <windows.h>
#include <tchar.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <newdev.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>

#include "resource.h"
#include "uninstall_virtual_devices.h"

// General values for success and failure.
#define SUCCESS    0
#define FAILURE   -1

// Chatpad filter driver INF file name.
#define CHATPAD_FILTER_DRIVER_INF            "data\\chatpad_filter.inf"
// Virtual keyboard driver INF file name.
#define CHATPAD_VIRTUAL_KEYBOARD_DRIVER_INF  "data\\chatpad_keyboard.inf"
// Virtual mouse driver INF file name.
#define CHATPAD_VIRTUAL_MOUSE_DRIVER_INF     "data\\chatpad_mouse.inf"

// Hardware IDs
#define WIRED_CONTROLLER_HW_ID               "USB\\VID_045E&PID_028E"
#define VIRTUAL_KEYBOARD_HW_ID               "HID\\ChatpadKbd"
#define VIRTUAL_MOUSE_HW_ID                  "HID\\ChatpadMouse"

// Strings for use with GetProcAddress.
#ifdef _UNICODE
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES    "UpdateDriverForPlugAndPlayDevicesW"
#else
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES    "UpdateDriverForPlugAndPlayDevicesA"
#endif


// Ugly global variable to make it easy to update the dialog box install status.
HWND hwndInstallDialog = NULL;


// Prototype for use with GetProcAddress.
typedef BOOL (WINAPI *UpdateDriverForPlugAndPlayDevicesProto)(
  __in HWND       hwndParent,
  __in LPCTSTR    hardwareID,
  __in LPCTSTR    fullINFPath,
  __in DWORD      installFlags,
  __out_opt PBOOL pRebootRequired);


// Pops up a message dialog box.  It is okay if hwndInstallDialog is NULL for this call.
// TODO change this so printf syntax can be used eventually?
void ShowMessage(char* message)
{
   if(NULL != message)
   {
      MessageBox(hwndInstallDialog, message, "Chatpad Installer Message", MB_OK);
   }
} // end ShowMessage

// Sets the status text in the dialog box.  It is NOT okay if hwndInstallDialog is NULL for this call.
// TODO change this so printf syntax can be used eventually?
void SetStatus(char* message)
{
   if((NULL != message) &&
      (NULL != hwndInstallDialog))
   {
      SetDlgItemText(hwndInstallDialog, IDC_UTILITY_STATUS, message);
   }
} // end ShowMessage

// Updates/installs the chatpad filter driver.
int UpdateChatpadFilterDriver(void)
{
   int      nRetValue            = SUCCESS;

   HMODULE  newDevModule         = NULL;
   DWORD    requiredPathLength   = 0;
   TCHAR    infPath[MAX_PATH];
   UpdateDriverForPlugAndPlayDevicesProto UpdateFunction;

   // Get a full INF path name.
   requiredPathLength = GetFullPathName(CHATPAD_FILTER_DRIVER_INF, MAX_PATH, infPath, NULL);
   if((requiredPathLength >= MAX_PATH) ||
      (0 == requiredPathLength))
   {
      ShowMessage("Error finding chatpad filter INF file.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      if(INVALID_FILE_ATTRIBUTES == GetFileAttributes(infPath))
      {
         nRetValue = FAILURE;
         char errorString[80];
         sprintf_s(errorString, 80, "Error getting chatpad filter INF file attributes:  0x%08x\n", GetLastError());
         ShowMessage(errorString);

         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      // TODO why does devcon do things this way, i.e. loading the DLL instead
      //   of linking to a library?  Does it reduce the number of different
      //   executables required or something?
      // Load newdev.dll so that UpdateDriverForPlugAndPlayDevices can be used.
      newDevModule = LoadLibrary(TEXT("newdev.dll"));
      if(NULL == newDevModule)
      {
         ShowMessage("Error loading newdev.dll.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      // TODO use an explicit C++ cast instead?
      UpdateFunction =
        (UpdateDriverForPlugAndPlayDevicesProto)
          GetProcAddress(newDevModule, UPDATEDRIVERFORPLUGANDPLAYDEVICES);
      if(NULL == UpdateFunction)
      {
         ShowMessage("GetProcAddress returned NULL.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      BOOL rebootRequired = FALSE;

      if(FALSE == UpdateFunction(NULL, WIRED_CONTROLLER_HW_ID, infPath, INSTALLFLAG_FORCE, &rebootRequired))
      {
         char errorString[80];
         sprintf_s(errorString, 80, "Failed to update chatpad filter device driver:  0x%08x\n", GetLastError());
         ShowMessage(errorString);

         nRetValue = FAILURE;
      }

      if(TRUE == rebootRequired)
      {
         ShowMessage(
           "WARNING:  Windows thinks a reboot is required.\n"
           "Reboot if you want, once the installer has finished running.\n");
      }
   }

   if(NULL != newDevModule)
   {
      FreeLibrary(newDevModule);
      newDevModule = NULL;
   }

   return nRetValue;
} // end UpdateChatpadFilterDriver

// Installs/updates the virtual keyboard driver.
int UpdateVirtualKeyboardDriver(void)
{
   int      nRetValue            = SUCCESS;

   HDEVINFO deviceInfoSet        = INVALID_HANDLE_VALUE;
   SP_DEVINFO_DATA deviceInfoData;
   GUID     classGUID;
   TCHAR    className[MAX_CLASS_NAME_LEN];
   // TODO what does LINE_LEN represent?  This is what devcon uses.  And why +4?  For two wide 0's?
   TCHAR    hardwareIDList[LINE_LEN+4];

   HMODULE  newDevModule         = NULL;
   DWORD    requiredPathLength   = 0;
   TCHAR    infPath[MAX_PATH];
   UpdateDriverForPlugAndPlayDevicesProto UpdateFunction;

   // Get a full INF path name.
   requiredPathLength = GetFullPathName(CHATPAD_VIRTUAL_KEYBOARD_DRIVER_INF, MAX_PATH, infPath, NULL);
   if((requiredPathLength >= MAX_PATH) ||
      (0 == requiredPathLength))
   {
      ShowMessage("Error finding virtual keyboard INF file.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      if(INVALID_FILE_ATTRIBUTES == GetFileAttributes(infPath))
      {
         ShowMessage("Error getting virtual keyboard INF file attributes.\n");
         nRetValue = FAILURE;
      }
   }

   //***
   // First, create the device node.
   //***

   if(SUCCESS == nRetValue)
   {
      // List of hardware IDs must be double zero-terminated, according to devcon source.
      ZeroMemory(hardwareIDList, sizeof(hardwareIDList));
      if(S_OK != StringCchCopy(hardwareIDList, LINE_LEN, VIRTUAL_KEYBOARD_HW_ID))
      {
         ShowMessage("Error copying virtual keyboard driver hardware ID.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      // Extract the class GUID.
      if(FALSE == SetupDiGetINFClass(infPath, &classGUID, className, MAX_CLASS_NAME_LEN, NULL))
      {
         ShowMessage("Error extracting virtual keyboard class GUID.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      // Create a container for a device information element. 
      // TODO read up on how this all works...for now, I am just following the devcon example flow
      deviceInfoSet = SetupDiCreateDeviceInfoList(&classGUID, NULL);
      if(INVALID_HANDLE_VALUE == deviceInfoSet)
      {
         ShowMessage("Error creating device info list for virtual keyboard device.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      // Create the device information element.
      deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
      if(FALSE == SetupDiCreateDeviceInfo(
        deviceInfoSet,
        className,
        &classGUID,
        _T("Chatpad Virtual Keyboard Device"),
        NULL,
        DICD_GENERATE_ID,
        &deviceInfoData))
      {
         char errorString[80];
         sprintf_s(errorString, 80, "Failed to create virtual keyboard device:  0x%08x\n", GetLastError());
         ShowMessage(errorString);

         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   if(SUCCESS == nRetValue)
   {
      // Add the hardware ID to the device's HardwareID property.
      if(FALSE == SetupDiSetDeviceRegistryProperty(
        deviceInfoSet,
        &deviceInfoData,
        SPDRP_HARDWAREID,
        reinterpret_cast<BYTE*>(hardwareIDList),
        (lstrlen(hardwareIDList) + 1 + 1) * sizeof(TCHAR)))
      {
         ShowMessage("Error setting HardwareID property for virtual keyboard device.\n");
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   if(SUCCESS == nRetValue)
   {
      // Transform the registry element into a device node in the plug-and-play hardware tree.
      if(FALSE == SetupDiCallClassInstaller(
        DIF_REGISTERDEVICE,
        deviceInfoSet,
        &deviceInfoData))
      {
         char errorString[80];
         sprintf_s(errorString, 80, "Failed to create virtual keyboard device node:  0x%08x\n", GetLastError());
         ShowMessage(errorString);

         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   //***
   // Now, "update" the driver.
   //***

   if(SUCCESS == nRetValue)
   {
      // Load newdev.dll so that UpdateDriverForPlugAndPlayDevices can be used.
      newDevModule = LoadLibrary(TEXT("newdev.dll"));
      if(NULL == newDevModule)
      {
         ShowMessage("Error loading newdev.dll.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      // TODO use an explicit C++ cast instead?
      UpdateFunction =
        (UpdateDriverForPlugAndPlayDevicesProto)
          GetProcAddress(newDevModule, UPDATEDRIVERFORPLUGANDPLAYDEVICES);
      if(NULL == UpdateFunction)
      {
         ShowMessage("GetProcAddress returned NULL.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      BOOL rebootRequired = FALSE;

      if(FALSE == UpdateFunction(NULL, VIRTUAL_KEYBOARD_HW_ID, infPath, INSTALLFLAG_FORCE, &rebootRequired))
      {
         char errorString[80];
         sprintf_s(errorString, 80, "Failed to update virtual keyboard device driver:  0x%08x\n", GetLastError());
         ShowMessage(errorString);

         nRetValue = FAILURE;
      }

      if(TRUE == rebootRequired)
      {
         ShowMessage(
           "WARNING:  Windows thinks a reboot is required.\n"
           "Reboot if you want, once the installer has finished running.\n");
      }
   }

   if(NULL != newDevModule)
   {
      FreeLibrary(newDevModule);
      newDevModule = NULL;
   }

   if(INVALID_HANDLE_VALUE != deviceInfoSet)
   {
      SetupDiDestroyDeviceInfoList(deviceInfoSet);
      deviceInfoSet = INVALID_HANDLE_VALUE;
   }

   return nRetValue;
} // end UpdateVirtualKeyboardDriver

// Installs/updates the virtual mouse driver.
int UpdateVirtualMouseDriver(void)
{
   int      nRetValue            = SUCCESS;

   HDEVINFO deviceInfoSet        = INVALID_HANDLE_VALUE;
   SP_DEVINFO_DATA deviceInfoData;
   GUID     classGUID;
   TCHAR    className[MAX_CLASS_NAME_LEN];
   // TODO what does LINE_LEN represent?  This is what devcon uses.  And why +4?  For two wide 0's?
   TCHAR    hardwareIDList[LINE_LEN+4];

   HMODULE  newDevModule         = NULL;
   DWORD    requiredPathLength   = 0;
   TCHAR    infPath[MAX_PATH];
   UpdateDriverForPlugAndPlayDevicesProto UpdateFunction;

   // Get a full INF path name.
   requiredPathLength = GetFullPathName(CHATPAD_VIRTUAL_MOUSE_DRIVER_INF, MAX_PATH, infPath, NULL);
   if((requiredPathLength >= MAX_PATH) ||
      (0 == requiredPathLength))
   {
      ShowMessage("Error finding virtual mouse INF file.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      if(INVALID_FILE_ATTRIBUTES == GetFileAttributes(infPath))
      {
         ShowMessage("Error getting virtual mouse INF file attributes.\n");
         nRetValue = FAILURE;
      }
   }

   //***
   // First, create the device node.
   //***

   if(SUCCESS == nRetValue)
   {
      // List of hardware IDs must be double zero-terminated, according to devcon source.
      ZeroMemory(hardwareIDList, sizeof(hardwareIDList));
      if(S_OK != StringCchCopy(hardwareIDList, LINE_LEN, VIRTUAL_MOUSE_HW_ID))
      {
         ShowMessage("Error copying virtual mouse driver hardware ID.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      // Extract the class GUID.
      if(FALSE == SetupDiGetINFClass(infPath, &classGUID, className, MAX_CLASS_NAME_LEN, NULL))
      {
         ShowMessage("Error extracting virtual mouse class GUID.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      // Create a container for a device information element. 
      // TODO read up on how this all works...for now, I am just following the devcon example flow
      deviceInfoSet = SetupDiCreateDeviceInfoList(&classGUID, NULL);
      if(INVALID_HANDLE_VALUE == deviceInfoSet)
      {
         ShowMessage("Error creating device info list for virtual mouse device.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      // Create the device information element.
      deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
      if(FALSE == SetupDiCreateDeviceInfo(
        deviceInfoSet,
        className,
        &classGUID,
        _T("Chatpad Virtual Mouse Device"),
        NULL,
        DICD_GENERATE_ID,
        &deviceInfoData))
      {
         char errorString[80];
         sprintf_s(errorString, 80, "Failed to create virtual mouse device:  0x%08x\n", GetLastError());
         ShowMessage(errorString);

         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   if(SUCCESS == nRetValue)
   {
      // Add the hardware ID to the device's HardwareID property.
      if(FALSE == SetupDiSetDeviceRegistryProperty(
        deviceInfoSet,
        &deviceInfoData,
        SPDRP_HARDWAREID,
        reinterpret_cast<BYTE*>(hardwareIDList),
        (lstrlen(hardwareIDList) + 1 + 1) * sizeof(TCHAR)))
      {
         ShowMessage("Error setting HardwareID property for virtual mouse device.\n");
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   if(SUCCESS == nRetValue)
   {
      // Transform the registry element into a device node in the plug-and-play hardware tree.
      if(FALSE == SetupDiCallClassInstaller(
        DIF_REGISTERDEVICE,
        deviceInfoSet,
        &deviceInfoData))
      {
         char errorString[80];
         sprintf_s(errorString, 80, "Failed to create virtual mouse device node:  0x%08x\n", GetLastError());
         ShowMessage(errorString);

         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   //***
   // Now, "update" the driver.
   //***

   if(SUCCESS == nRetValue)
   {
      // Load newdev.dll so that UpdateDriverForPlugAndPlayDevices can be used.
      newDevModule = LoadLibrary(TEXT("newdev.dll"));
      if(NULL == newDevModule)
      {
         ShowMessage("Error loading newdev.dll.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      // TODO use an explicit C++ cast instead?
      UpdateFunction =
        (UpdateDriverForPlugAndPlayDevicesProto)
          GetProcAddress(newDevModule, UPDATEDRIVERFORPLUGANDPLAYDEVICES);
      if(NULL == UpdateFunction)
      {
         ShowMessage("GetProcAddress returned NULL.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      BOOL rebootRequired = FALSE;

      if(FALSE == UpdateFunction(NULL, VIRTUAL_MOUSE_HW_ID, infPath, INSTALLFLAG_FORCE, &rebootRequired))
      {
         char errorString[80];
         sprintf_s(errorString, 80, "Failed to update virtual mouse device driver:  0x%08x\n", GetLastError());
         ShowMessage(errorString);

         nRetValue = FAILURE;
      }

      if(TRUE == rebootRequired)
      {
         ShowMessage(
           "WARNING:  Windows thinks a reboot is required.\n"
           "Reboot if you want, once the installer has finished running.\n");
      }
   }

   if(NULL != newDevModule)
   {
      FreeLibrary(newDevModule);
      newDevModule = NULL;
   }

   if(INVALID_HANDLE_VALUE != deviceInfoSet)
   {
      SetupDiDestroyDeviceInfoList(deviceInfoSet);
      deviceInfoSet = INVALID_HANDLE_VALUE;
   }

   return nRetValue;
} // end UpdateVirtualMouseDriver

// Uninstalls the chatpad filter driver.
// TODO is there eventually a better way to uninstall, i.e. by deleting the driver store entry or
//   removing the lower filters etc.?  This way requires the user to go into device manager and
//   manually update to get the old driver back.
int UninstallChatpadFilterDriver(void)
{
   int      nRetValue               = SUCCESS;
   UINT     requiredPathLength      = 0;
   // TODO:  Depending on the build system, these might be wide characters, and it might
   //   break the functionality of some code below.  I need to go through the code
   //   eventually and make sure I'm handling strings properly.
   TCHAR    windowsPath[MAX_PATH];
   TCHAR    pathToDelete[MAX_PATH];

   // Get the Windows directory path.  Make sure there is enough space to append the driver path.
   requiredPathLength = GetSystemWindowsDirectory(windowsPath, MAX_PATH);
   // TODO eventually don't use an arbitrary constant that's long enough here?  At least make
   //   it configurable via a #define or something?
   if((requiredPathLength >= (MAX_PATH - 50)) ||
      (0 == requiredPathLength))
   {
      ShowMessage("Error getting Windows directory.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      // Add a '\' to the path if necessary.
      if('\\' != windowsPath[requiredPathLength])
      {
         windowsPath[requiredPathLength]     = '\\';
         windowsPath[requiredPathLength+1]   = '\0';
      }
   }

   if(SUCCESS == nRetValue)
   {
      // Delete the main chatpad filter driver.
      _stprintf_s(pathToDelete, MAX_PATH, "%sSystem32\\Drivers\\chatpad_filter.sys", windowsPath);
      if(0 == DeleteFile(pathToDelete))
      {
         char errorString[80];
         sprintf_s(errorString, 80, "Failed to delete filter driver:  0x%08x\n", GetLastError());
         ShowMessage(errorString);

         nRetValue = FAILURE;
      }
   }

   return nRetValue;
} // end UninstallChatpadFilterDriver

// Installs or upgrades the chatpad drivers.
int DoInstallOrUpgrade(void)
{
   int nRetValue = SUCCESS;

   ShowMessage(
     "Preparing to install chatpad driver.\n"
     "WARNING:  Make sure the controller is plugged in before clicking OK!");

   if(SUCCESS == nRetValue)
   {
      SetStatus("Installing controller filter driver...");
      nRetValue = UpdateChatpadFilterDriver();
   }

   if(SUCCESS == nRetValue)
   {
      SetStatus("Uninstalling virtual keyboard device drivers...");
      // Try to uninstall the virtual keyboard driver in case it was installed already.
      // TODO eventually make update actually update instead of requiring the uninstall first?
      nRetValue = UninstallVirtualKeyboardDevice();
   }

   if(SUCCESS == nRetValue)
   {
      SetStatus("Updating virtual keyboard device drivers...");
      nRetValue = UpdateVirtualKeyboardDriver();
   }

   if(SUCCESS == nRetValue)
   {
      SetStatus("Uninstalling virtual mouse device drivers...");
      // Try to uninstall the virtual mouse driver in case it was installed already.
      // TODO eventually make update actually update instead of requiring the uninstall first?
      nRetValue = UninstallVirtualMouseDevice();
   }

   if(SUCCESS == nRetValue)
   {
      SetStatus("Updating virtual mouse device drivers...");
      nRetValue = UpdateVirtualMouseDriver();
   }

   if(SUCCESS == nRetValue)
   {
      SetStatus("Install/upgrade succeeded.");
   }
   else
   {
      SetStatus("Install/upgrade failed.");
   }

   return nRetValue;
} // end DoInstallOrUpgrade

// Uninstalls the chatpad drivers.
int DoUninstall(void)
{
   int nRetValue = SUCCESS;

   ShowMessage(
     "Preparing to delete the chatpad drivers.\n"
     "WARNING:  Make sure the controller is unplugged before clicking OK!");

   if(SUCCESS == nRetValue)
   {
      SetStatus("Uninstalling controller filter driver...");
      nRetValue = UninstallChatpadFilterDriver();
   }

   if(SUCCESS == nRetValue)
   {
// TODO NEXT WIP uncomment this when uninstalling the virtual devices is implemented
//      SetStatus("Uninstalling virtual keyboard device drivers...");
      nRetValue = UninstallVirtualKeyboardDevice();
   }

   if(SUCCESS == nRetValue)
   {
// TODO NEXT WIP uncomment this when uninstalling the virtual devices is implemented
//      SetStatus("Uninstalling virtual mouse device drivers...");
      nRetValue = UninstallVirtualMouseDevice();
   }

   if(SUCCESS == nRetValue)
   {
      SetStatus("Uninstall succeeded.");
   }
   else
   {
      SetStatus("Uninstall failed.");
   }

   return nRetValue;
} // end DoUninstall

// Dialog box callback.
LRESULT CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   LRESULT retValue = FALSE;

   switch(msg)
   {
      case WM_INITDIALOG:
      {
         hwndInstallDialog = hwnd;
         SetStatus("Ready.");
         retValue = TRUE;
      }

      case WM_COMMAND:
      {
         switch(wParam)
         {
            case IDC_INSTALL_BUTTON:
            {
               if(IDYES == MessageBox(hwnd, "Install/upgrade chatpad drivers?", "Install", MB_YESNO))
               {
                  HWND dialogButtons[NUM_DIALOG_BUTTONS];
                  dialogButtons[0] = GetDlgItem(hwnd, IDC_INSTALL_BUTTON);
                  dialogButtons[1] = GetDlgItem(hwnd, IDC_UNINSTALL_BUTTON);
                  dialogButtons[2] = GetDlgItem(hwnd, IDC_EXIT_BUTTON);

                  // Disable the buttons.
                  for(int i = 0; i < NUM_DIALOG_BUTTONS; i++)
                  {
                     if(NULL != dialogButtons[i])
                     {
                        EnableWindow(dialogButtons[i], FALSE);
                     }
                  }

                  // Do the install/upgrade.
                  DoInstallOrUpgrade();

                  // Enable the buttons.
                  for(int i = 0; i < NUM_DIALOG_BUTTONS; i++)
                  {
                     if(NULL != dialogButtons[i])
                     {
                        EnableWindow(dialogButtons[i], TRUE);
                     }
                  }
               } // end if user confirmed install/upgrade
               retValue = TRUE;
               break;
            } // end case IDC_INSTALL_BUTTON

            case IDC_UNINSTALL_BUTTON:
            {
               if(IDYES == MessageBox(hwnd, "Uninstall chatpad drivers?", "Uninstall", MB_YESNO))
               {
                  HWND dialogButtons[NUM_DIALOG_BUTTONS];
                  dialogButtons[0] = GetDlgItem(hwnd, IDC_INSTALL_BUTTON);
                  dialogButtons[1] = GetDlgItem(hwnd, IDC_UNINSTALL_BUTTON);
                  dialogButtons[2] = GetDlgItem(hwnd, IDC_EXIT_BUTTON);

                  // Disable the buttons.
                  for(int i = 0; i < NUM_DIALOG_BUTTONS; i++)
                  {
                     if(NULL != dialogButtons[i])
                     {
                        EnableWindow(dialogButtons[i], FALSE);
                     }
                  }

                  // Do the uninstall.
                  DoUninstall();

                  // Enable the buttons.
                  for(int i = 0; i < NUM_DIALOG_BUTTONS; i++)
                  {
                     if(NULL != dialogButtons[i])
                     {
                        EnableWindow(dialogButtons[i], TRUE);
                     }
                  }
               } // end if user confirmed uninstall
               retValue = TRUE;
               break;
            } // end case IDC_UNINSTALL_BUTTON

            case IDC_EXIT_BUTTON:   // fall-through
            case IDCANCEL:
            {
               EndDialog(hwnd, 0);
               retValue = TRUE;
               break;
            }

            default:
            {
               break;
            }
         }
      }
      default:
      {
         break;
      }
   } // end switch(msg)

   return FALSE;
}

// Application entry point.
// Windows code based on example at http://www.functionx.com/win32/Lesson04.htm.
int WINAPI WinMain(
  HINSTANCE hInst,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nShowCmd)
{
   int nRetValue = SUCCESS;

   hwndInstallDialog = NULL;
   DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, reinterpret_cast<DLGPROC>(DlgProc));

   return nRetValue;
} // end WinMain

