//
// Chatpad driver uninstallation program that works with a KMDF USB filter driver, by
//   GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//
// All source code in this file was written by hand.
//

#include <windows.h>
#include <tchar.h>
#include <newdev.h>
#include <stdio.h>
#include <stdlib.h>

#include "uninstall_virtual_devices.h"

// General values for success and failure.
// TODO move these to somewhere common?
#define SUCCESS    0
#define FAILURE   -1


// TODO move this to a common/header file instead?
// TODO change this so printf syntax can be used eventually?
// TODO make more advanced to support yes/no as well?  Or just move to a custom dialog/window?
extern void ShowMessage(char* message);

int UninstallVirtualKeyboardDevice(void)
{
   int nRetValue = SUCCESS;

   // TODO NEXT WIP implement

   return nRetValue;
} // end UninstallVirtualKeyboardDevice

int UninstallVirtualMouseDevice(void)
{
   int nRetValue = SUCCESS;

   // TODO NEXT WIP implement

   return nRetValue;
} // end UninstallVirtualMouseDevice

