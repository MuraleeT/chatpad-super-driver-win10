//
// Chatpad KMDF USB filter driver, by GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// This header file is shared between the WDF filter driver and the user mode application.
// TODO rename to chatpad_filter_common.h if that is a more accurate name?
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//

// The awesome guide at http://www.osronline.com/article.cfm?name=wdffltr_v10.zip&id=446 was
// used as a basis for how to put this driver together, but the code in this driver was all
// written by hand.

#ifndef CHATPAD_FILTER_IOCTL_H
#define CHATPAD_FILTER_IOCTL_H


// Arbitrary device type from the user-defined range.
#define FILE_DEVICE_CHATPAD_FILTER  41000

// IOCTL codes.  The range can be from 2048-4095 inclusive.

// This IOCTL checks whether the controller has been initialized by the Microsoft driver.
#define IOCTL_CHATPAD_IS_MS_INIT_DONE              CTL_CODE(FILE_DEVICE_CHATPAD_FILTER,   \
                                                            2049,                         \
                                                            METHOD_BUFFERED,              \
                                                            FILE_READ_DATA | FILE_WRITE_DATA)

// This IOCTL sends a control transfer to the device.
#define IOCTL_CHATPAD_SEND_CONTROL_TRANSFER        CTL_CODE(FILE_DEVICE_CHATPAD_FILTER,   \
                                                            2050,                         \
                                                            METHOD_BUFFERED,              \
                                                            FILE_READ_DATA | FILE_WRITE_DATA)

// This IOCTL writes data to the controls endpoint.  This can do things like flashing lights
// on the controller or activating the rumble motors.
#define IOCTL_CHATPAD_WRITE_TO_CONTROLS_ENDPOINT   CTL_CODE(FILE_DEVICE_CHATPAD_FILTER,   \
                                                            2051,                         \
                                                            METHOD_BUFFERED,              \
                                                            FILE_READ_DATA | FILE_WRITE_DATA)

// This IOCTL queues a read from the controls endpoint.  Data will only become available
// if controls data interception is enabled.  The data includes information about the
// thumbsticks and buttons.
#define IOCTL_CHATPAD_READ_FROM_CONTROLS_ENDPOINT  CTL_CODE(FILE_DEVICE_CHATPAD_FILTER,   \
                                                            2052,                         \
                                                            METHOD_BUFFERED,              \
                                                            FILE_READ_DATA | FILE_WRITE_DATA)

// This IOCTL queues a read from the chatpad endpoint.  The data includes information
// about keys being pressed.
#define IOCTL_CHATPAD_READ_FROM_CHATPAD_ENDPOINT   CTL_CODE(FILE_DEVICE_CHATPAD_FILTER,   \
                                                            2053,                         \
                                                            METHOD_BUFFERED,              \
                                                            FILE_READ_DATA | FILE_WRITE_DATA)

// This IOCTL sets options for controls mapping.  XBox 360 controller controls mappings
// to other buttons (i.e. swapping the A and B buttons) only take effect if the controls
// filter mode is set to filtered.
//
// This IOCTL can also be used to enable or disable thumbstick vertical axis inversion.
#define IOCTL_CHATPAD_SET_CONTROLS_MAPPINGS        CTL_CODE(FILE_DEVICE_CHATPAD_FILTER,   \
                                                            2054,                         \
                                                            METHOD_BUFFERED,              \
                                                            FILE_READ_DATA | FILE_WRITE_DATA)

// This IOCTL sets options for filtering controls data.  Controls data can be unfiltered
// (no bindings available, everything passes through), filtered (allows the guide button
// to be mapped even while playing a game), or intercepted (allows for Windows Mode
// functionality where the thumbsticks can control the mouse instead of affecting a game
// at all).
#define IOCTL_CHATPAD_SET_CONTROLS_FILTER_MODE     CTL_CODE(FILE_DEVICE_CHATPAD_FILTER,   \
                                                            2055,                         \
                                                            METHOD_BUFFERED,              \
                                                            FILE_READ_DATA | FILE_WRITE_DATA)



// Enum to be used with IOCTL_CHATPAD_SET_CONTROLS_FILTER_MODE to set the filter mode.
typedef enum _CHATPAD_FILTER_MODE
{
   // In unfiltered mode, controls data is not mapped or intercepted.
   FILTER_MODE_UNFILTERED        = 0,

   // In filtered mode, some controls may be remapped.
   FILTER_MODE_FILTERED          = 1,

   // In intercepted mode, controls may not be used for normal ingame functions.
   FILTER_MODE_INTERCEPTED       = 2
} CHATPAD_FILTER_MODE;


// Enum to be used with IOCTL_CHATPAD_SEND_CONTROL_TRANSFER
// to select USB interfaces.
typedef enum _XBOX_CONTROLLER_INTERFACE
{
   // This interface is used for some general commands.
   // TODO verify that the above description is accurate.
   MAIN_INTERFACE       = 0,

   // This interface is used to read data from thumbsticks, buttons, etc.
   // TODO verify that the above description is accurate.
   CONTROLS_INTERFACE   = 1,

   // This interface is used to interact with the chatpad.
   // TODO verify that the above description is accurate.
   CHATPAD_INTERFACE    = 2
} XBOX_CONTROLLER_INTERFACE;


// Structure definitions

// TODO NEXT delete below if these structures are not needed?

// Request types.
// TODO is there any restriction on these values?
// TODO do I need some standard ones, ala PnPRequest etc.?
typedef enum _REQUEST_TYPE
{
   ControlsWriteRequest    = 0x3000,
   ControlsReadRequest     = 0x3001,
   ChatpadReadRequest      = 0x3002,
   ChatpadMaxRequestType   = 0x3003
} REQUEST_TYPE;

// Request buffer entries "inherit" this header.
typedef struct _CHATPAD_REQUEST_ENTRY_HEADER
{
   REQUEST_TYPE requestType;
} CHATPAD_REQUEST_ENTRY_HEADER, *PCHATPAD_REQUEST_ENTRY_HEADER;

typedef struct _CHATPAD_INIT_REQUEST
{
   // 16-bit init code for chatpad, like 0x90 0x00.
   // TODO change the above if the code is wrong.
   UCHAR initCode[2];
} CHATPAD_INIT_REQUEST, *PCHATPAD_INIT_REQUEST;

// TODO NEXT create more structures for read, write, maybe others

// Union of request types, keyed by CHATPAD_REQUEST->RequestType.
// TODO this syntax looks like C++...is that normal for a kernel driver like this?
typedef struct _CHATPAD_REQUEST : CHATPAD_REQUEST_ENTRY_HEADER
{
   union
   {
      CHATPAD_INIT_REQUEST          initRequest;
      // TODO add read, write, maybe others
   };
} CHATPAD_REQUEST, *PCHATPAD_REQUEST;

// TODO is there a guideline about this limit?
#define MAX_CHATPAD_REQUESTS_PER_BUFFER   0x0200
typedef struct _CHATPAD_REQUEST_BUFFER
{
   LIST_ENTRY        listEntry;
   ULONG             validRequestCount;
   CHATPAD_REQUEST   chatpadRequests[MAX_CHATPAD_REQUESTS_PER_BUFFER];
} CHATPAD_REQUEST_BUFFER, *PCHATPAD_REQUEST_BUFFER;


#endif // CHATPAD_FILTER_IOCTL_H

