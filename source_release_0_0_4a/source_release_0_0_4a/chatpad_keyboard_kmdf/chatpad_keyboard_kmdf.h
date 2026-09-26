//
// Chatpad virtual keyboard KMDF driver, by GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//

// This driver was hand-written by following the hidkmdf example from the 7600.16385.1 WinDDK.

#ifndef CHATPAD_KEYBOARD_KMDF_H
#define CHATPAD_KEYBOARD_KMDF_H

// TODO REMOVE if debug output is not desired
#define CHATPAD_KBD_KMDF_DBG

extern "C"
{
#include <ntddk.h>
#include <wdf.h>
}

#include <hidport.h>

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#include "chatpad_keyboard_ioctl.h"


// Support debug tracing.
#ifdef CHATPAD_KBD_KMDF_DBG
// TODO add this above the other DbgPrint if desired for more information in DebugView.
//   Make sure to add a '\' at the end if this is done.
/*
   DbgPrint("CHATPAD!"__FUNCTION__ ": ");
*/
#define ChatpadTrace(_MSG_) \
{ \
   DbgPrint _MSG_; \
}
#else
#define ChatpadTrace(__MSG__) \
{ \
}
#endif


// Constants


// TODO change the names of some of the constants below so that they are less generic and less
//   likely to ever have a collision?

#define CHATPADKEYBOARDKMDF_POOL_TAG               static_cast<ULONG>('zilB')
#define CHATPADKEYBOARDKMDF_HARDWARE_IDS           L"HID\\ChatpadKbd\0\0"
#define CHATPADKEYBOARDKMDF_HARDWARE_IDS_LENGTH    sizeof(CHATPADKEYBOARDKMDF_HARDWARE_IDS)

// Control device names.
#define CHATPAD_KEYBOARD_KMDF_CONTROL_NAME         L"\\Device\\ChatpadKeyboardKMDF"
#define CHATPAD_KEYBOARD_KMDF_CONTROL_LINK         L"\\DosDevices\\ChatpadKeyboardKMDF"


// Structure definitions


// Structure for a device context for the device.
typedef struct _DEVICE_CONTEXT
{
   // Whether this virtual keyboard is enabled.
   BOOLEAN                       keyboardEnabled;

   // Queue for read IOCTLs from hidclass that will be satisfied with virtual keyboard data.
   WDFQUEUE                      hidReadRequestQueue;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
  DEVICE_CONTEXT,
  ChatpadKeyboardKMDFGetDeviceContext)

// Control device extension data.
typedef struct _CHATPAD_KEYBOARD_KMDF_CONTROL_DEVICE_EXTENSION
{
   // TODO is this safe?  I am doing this so I have access to hidReadRequestQueue
   //   in the close handler, and can generate a HID response indicating that no
   //   keys are now pressed.
   PDEVICE_CONTEXT               keyboardDeviceContext;
} CHATPAD_KEYBOARD_KMDF_CONTROL_DEVICE_EXTENSION, *PCHATPAD_KEYBOARD_KMDF_CONTROL_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
  CHATPAD_KEYBOARD_KMDF_CONTROL_DEVICE_EXTENSION,
  ChatpadKeyboardKMDFControlGetData);

#endif // CHATPAD_KEYBOARD_KMDF_H

