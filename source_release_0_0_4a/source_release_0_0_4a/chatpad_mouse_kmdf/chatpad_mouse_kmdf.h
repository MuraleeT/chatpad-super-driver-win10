//
// Chatpad virtual mouse KMDF driver, by GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//

// This driver was hand-written by following the hidkmdf example from the 7600.16385.1 WinDDK.

#ifndef CHATPAD_MOUSE_KMDF_H
#define CHATPAD_MOUSE_KMDF_H

// TODO REMOVE if debug output is not desired
#define CHATPAD_MOUSE_KMDF_DBG

extern "C"
{
#include <ntddk.h>
#include <wdf.h>
}

#include <hidport.h>

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#include "chatpad_mouse_ioctl.h"


// Support debug tracing.
#ifdef CHATPAD_MOUSE_KMDF_DBG
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

#define CHATPADMOUSEKMDF_POOL_TAG                  static_cast<ULONG>('zilB')
#define CHATPADMOUSEKMDF_HARDWARE_IDS              L"HID\\ChatpadMouse\0\0"
#define CHATPADMOUSEKMDF_HARDWARE_IDS_LENGTH       sizeof(CHATPADMOUSEKMDF_HARDWARE_IDS)

// Control device names.
#define CHATPAD_MOUSE_KMDF_CONTROL_NAME            L"\\Device\\ChatpadMouseKMDF"
#define CHATPAD_MOUSE_KMDF_CONTROL_LINK            L"\\DosDevices\\ChatpadMouseKMDF"


// Structure definitions


// Structure for a device context for the device.
typedef struct _DEVICE_CONTEXT
{
   // Whether this virtual mouse is enabled.
   BOOLEAN                       mouseEnabled;

   // Queue for read IOCTLs from hidclass that will be satisfied with virtual mouse data.
   WDFQUEUE                      hidReadRequestQueue;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
  DEVICE_CONTEXT,
  ChatpadMouseKMDFGetDeviceContext)

// Control device extension data.
typedef struct _CHATPAD_MOUSE_KMDF_CONTROL_DEVICE_EXTENSION
{
   // TODO is this safe?  I am doing this so I have access to hidReadRequestQueue
   //   in the close handler, and can generate a HID response indicating that no
   //   simulated mouse events are now occurring.
   PDEVICE_CONTEXT               mouseDeviceContext;
} CHATPAD_MOUSE_KMDF_CONTROL_DEVICE_EXTENSION, *PCHATPAD_MOUSE_KMDF_CONTROL_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
  CHATPAD_MOUSE_KMDF_CONTROL_DEVICE_EXTENSION,
  ChatpadMouseKMDFControlGetData);

#endif // CHATPAD_MOUSE_KMDF_H

