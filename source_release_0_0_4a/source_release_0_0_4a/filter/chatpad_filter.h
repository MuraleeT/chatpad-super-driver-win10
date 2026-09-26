//
// Chatpad KMDF USB filter driver, by GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//

// The awesome guide at http://www.osronline.com/article.cfm?name=wdffltr_v10.zip&id=446 was
// used as a basis for how to put this driver together, but the code in this driver was all
// written by hand.

#ifndef CHATPAD_FILTER_H
#define CHATPAD_FILTER_H

// TODO REMOVE if debug output is not desired
#define CHATPAD_DBG

extern "C"
{
#include <ntddk.h>
#include <wdf.h>
}

#include <usb.h>
#include <usbioctl.h>
#include <Wdfusb.h>

#include "chatpad_filter_ioctl.h"


// Support debug tracing.
#ifdef CHATPAD_DBG
// TODO add this above the other DbgPrint if desired for more information in DebugView.  Make sure to add a '\' at the end.
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

// As far as I can tell, this is just an arbitrary number that I chose when I was following an example.
// TODO REMOVE?
#define DEVICE_CONTEXT_MAGIC_NUMBER                         0x91727388

// Arbitrary number of supported interfaces.  This should be no greater than 255.
#define CHATPAD_DRIVER_MAX_SUPPORTED_NUM_INTERFACES         16

// Arbitrary number of supported endpoints for a device.  This should be no greater than 255.
#define CHATPAD_DRIVER_MAX_SUPPORTED_NUM_PIPES              16

// Timeout length in seconds for synchronous requests.
#define CHATPAD_DRIVER_SYNCHRONOUS_REQUEST_TIMEOUT_LENGTH   10

// Length in bytes of the write buffer used for controls data.
#define CONTROLS_WRITE_BUFFER_LENGTH                        32

// Length in bytes of the read buffers used for controls data.
#define CONTROLS_READ_BUFFER_LENGTH                         32

// Length in bytes of the read buffers used for chatpad data.
#define CHATPAD_READ_BUFFER_LENGTH                          32

// How many pending requests will be used for reading from a USB endpoint.  2 or 3 is apparently normal.
#define CHATPAD_MAX_NUM_PENDING_USB_REQUESTS                2

// How many pending requests are used fror reading from a USB endpoint by the top-level driver.
// It seems to use 2, which makes sense.
#define TOP_LEVEL_DRIVER_NUM_PENDING_READ_REQUESTS          2


// Structure definitions


// IMPORTANT NOTE:
//
// Last time I checked (11/25/2010), the below structure was less than 1 KB in
// size.  If this structure gets much over a kilobyte in size, I may want to
// refactor it so that it includes one or more pointers to allocated data
// instead of putting so much data in this structure itself.
//
// See http://thedailyreviewer.com/windowsos/view/size-of-device-context-limit-107123784
// for information that suggests there may be a device context limit that may not even
// be documented, somewhere less than 64 KB.

// Structure for a device context for each device.
typedef struct _DEVICE_CONTEXT
{
   // TODO remove magicNumber and serialNumber if unused?
   ULONG                         magicNumber;
   ULONG                         serialNumber;
   // TODO add descriptive comments for these two variables
   WDFIOTARGET                   targetForRequests;
   WDFUSBDEVICE                  usbDevice;
   // Whether the USB device configuration has been selected yet.
   BOOLEAN                       usbConfigSelected;
   // Whether we think the Microsoft driver has finished initializing the 360 controller.
   BOOLEAN                       msInitFinished;
   // Whether we have finished initializing the chatpad successfully.
   // TODO eventually remove chatpadInitFinished if is not used and needed
   BOOLEAN                       chatpadInitFinished;
   // The current filter mode.  This value must be one of the FILTER_MODE_ constants.
   ULONG                         filterMode;
   // Whether the controls continuous reader is started.
   BOOLEAN                       controlsReaderStarted;

   // Queue used to park controls data read requests from the top-level driver.
   WDFQUEUE                      controlsParkedDriverReadRequestQueue;
   // Queue used to park controls data read requests from the user.
   WDFQUEUE                      controlsParkedUserReadRequestQueue;

   // Queue used to park chatpad data read requests from the user.
   WDFQUEUE                      chatpadParkedUserReadRequestQueue;

   // Number of valid interface information structures in usbdInterfaceInformation.
   BYTE                          numInterfaces;
   // Array of dynamically-allocated structures containing saved information
   // about interfaces so that URB_FUNCTION_SELECT_INTERFACE from the top-level
   // driver can be handled (as long as only the default 0 alternate interface
   // is selected).
   PUSBD_INTERFACE_INFORMATION   usbdInterfaceInformation[CHATPAD_DRIVER_MAX_SUPPORTED_NUM_INTERFACES];

   // Number of valid pipe information structures in wdfUsbPipeInfo and WDF pipe handles in wdfUsbPipe.
   BYTE                          numPipes;
   // Pipe information for each pipe.
   WDF_USB_PIPE_INFORMATION      wdfUsbPipeInfo[CHATPAD_DRIVER_MAX_SUPPORTED_NUM_PIPES];
   // Pipe handles for use in reading and writing data.
   WDFUSBPIPE                    wdfUsbPipe[CHATPAD_DRIVER_MAX_SUPPORTED_NUM_PIPES];

   // For the fields below, the "controls" endpoint refers to the endpoint that returns data
   // about (potentially among other things) XBox 360 controller thumbstick and button data.

   // TODO combine some of the below values for chatpad and top-level requests into
   //   custom structures of their own to make things cleaner here?

   // Memory containing a write buffer for writing data to the controls endpoint.
   WDFMEMORY                     controlsWriteBufferMemory;
   // Memory containing an URB for writing data to the controls endpoint.
   WDFMEMORY                     controlsWriteUrbMemory;
   // Request for writing data to the controls endpoint.
   WDFREQUEST                    controlsWriteRequest;

   // Memory containing read buffers for reading controls endpoint data from the XBox 360 controller.
   WDFMEMORY                     controlsReadBufferMemory[CHATPAD_MAX_NUM_PENDING_USB_REQUESTS];
   // Memory containing URBs for reading controls endpoint data from the XBox 360 controller.
   WDFMEMORY                     controlsReadUrbMemory[CHATPAD_MAX_NUM_PENDING_USB_REQUESTS];
   // Requests for reading controls endpoint data from the XBox 360 controller.
   WDFREQUEST                    controlsReadRequests[CHATPAD_MAX_NUM_PENDING_USB_REQUESTS];
   // Tracks whether each controls endpoint data read request is in use.  If this value is FALSE
   // for a particular request, then that request can be formatted and sent.
   // TODO do I need some sort of synchronization around this, or is the way I have callbacks
   //   set up sufficient?
   BOOLEAN                       controlsReadRequestsInUse[CHATPAD_MAX_NUM_PENDING_USB_REQUESTS];

   // Memory containing read buffers for reading chatpad endpoint data from the XBox 360 controller.
   WDFMEMORY                     chatpadReadBufferMemory[CHATPAD_MAX_NUM_PENDING_USB_REQUESTS];
   // Memory containing URBs for reading chatpad endpoint data from the XBox 360 controller.
   WDFMEMORY                     chatpadReadUrbMemory[CHATPAD_MAX_NUM_PENDING_USB_REQUESTS];
   // Requests for reading chatpad endpoint data from the XBox 360 controller.
   WDFREQUEST                    chatpadReadRequests[CHATPAD_MAX_NUM_PENDING_USB_REQUESTS];
   // Tracks whether each chatpad endpoint data read request is in use.  If this value is FALSE
   // for a particular request, then that request can be formatted and sent.
   BOOLEAN                       chatpadReadRequestsInUse[CHATPAD_MAX_NUM_PENDING_USB_REQUESTS];
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

#define IS_DEVICE_CONTEXT(_DC_) (((_DC_)->magicNumber) == DEVICE_CONTEXT_MAGIC_NUMBER)

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
  DEVICE_CONTEXT,
  ChatpadFilterGetDeviceContext)

// Structure for extension data for the control device.
typedef struct _CHATPAD_CONTROL_DEVICE_EXTENSION
{
   // TODO is this safe?  I am doing this so I have access to the
   //   DEVICE_CONTEXT filter mode in the close handler.
   PDEVICE_CONTEXT               chatpadFilterDeviceContext;
} CHATPAD_CONTROL_DEVICE_EXTENSION, *PCHATPAD_CONTROL_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
  CHATPAD_CONTROL_DEVICE_EXTENSION,
  ChatpadControlGetData);


// Global variables.
extern LONG                            chatpadDeviceCount;
extern WDFDEVICE                       chatpadControlDevice;
extern BOOLEAN                         chatpadControlDeviceOpen;

// Control device names.
#define CHATPAD_FILTER_CONTROL_NAME    L"\\Device\\ChatpadFilter"
#define CHATPAD_FILTER_CONTROL_LINK    L"\\DosDevices\\ChatpadFilter"

// Function prototypes.

// chatpad_filter.cpp

EVT_WDF_DRIVER_UNLOAD DriverUnload;

VOID
DriverUnload(
  IN WDFDRIVER driver);

EVT_WDF_DRIVER_DEVICE_ADD ChatpadFilterEvtDeviceAdd;

NTSTATUS
ChatpadFilterEvtDeviceAdd(
  IN WDFDRIVER       driver,
  IN PWDFDEVICE_INIT deviceInit);

NTSTATUS
ChatpadCreateControlDevice(
  WDFDEVICE device);

VOID
ChatpadDeleteControlDevice(VOID);

EVT_WDF_DEVICE_CONTEXT_CLEANUP ChatpadFilterEvtDeviceContextCleanup;

VOID
ChatpadFilterEvtDeviceContextCleanup(
  IN WDFOBJECT       object);

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL ChatpadFilterInternalIOCTLHandler;

VOID
ChatpadFilterInternalIOCTLHandler(
  IN WDFQUEUE     queue,
  IN WDFREQUEST   request,
  IN size_t       outputBufferLength,
  IN size_t       inputBufferLength,
  IN ULONG        ioControlCode);

/*
EVT_WDF_IO_QUEUE_IO_DEFAULT ChatpadFilterDefaultHandler;

VOID
ChatpadFilterDefaultHandler(
  IN WDFQUEUE     queue,
  IN WDFREQUEST   request);
  */

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL ChatpadFilterIOCTLHandler;

VOID
ChatpadFilterIOCTLHandler(
  IN WDFQUEUE     queue,
  IN WDFREQUEST   request,
  IN size_t       outputBufferLength,
  IN size_t       inputBufferLength,
  IN ULONG        ioControlCode);

EVT_WDF_IO_QUEUE_IO_READ ChatpadFilterReadHandler;

VOID
ChatpadFilterReadHandler(
  IN WDFQUEUE     queue,
  IN WDFREQUEST   request,
  IN size_t       length);

EVT_WDF_IO_QUEUE_IO_WRITE ChatpadFilterWriteHandler;

VOID
ChatpadFilterWriteHandler(
  IN WDFQUEUE     queue,
  IN WDFREQUEST   request,
  IN size_t       length);

#endif // CHATPAD_FILTER_H

