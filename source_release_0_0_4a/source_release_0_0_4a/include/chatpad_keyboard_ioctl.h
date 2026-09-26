//
// Chatpad virtual keyboard KMDF driver, by GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//
// This header file is shared between the virtual keyboard filter driver and the user mode application.
//

#ifndef CHATPAD_KEYBOARD_IOCTL_H
#define CHATPAD_KEYBOARD_IOCTL_H


// Constants that can be used with IOCTL_CHATPAD_KEYBOARD_KMDF_SET_ENABLED.
#define CHATPAD_KEYBOARD_KMDF_DISABLE        0
#define CHATPAD_KEYBOARD_KMDF_ENABLE         1


// Arbitrary function code that should be okay for non-Microsoft use.
// (see http://msdn.microsoft.com/en-us/library/ms902086.aspx).
#define FILE_DEVICE_CHATPAD_KEYBOARD_FILTER  41001

// IOCTL codes.  The range can be from 2048-4095 inclusive.

// This IOCTL enables or disables the virtual keyboard device.
#define IOCTL_CHATPAD_KEYBOARD_KMDF_SET_ENABLED    CTL_CODE(FILE_DEVICE_CHATPAD_KEYBOARD_FILTER,   \
                                                            3000,                         \
                                                            METHOD_BUFFERED,              \
                                                            FILE_READ_DATA | FILE_WRITE_DATA)

// This IOCTL sends data to the virtual keyboard device.  This can cause a key to be simulated, for instance.
#define IOCTL_CHATPAD_KEYBOARD_KMDF_SEND_DATA      CTL_CODE(FILE_DEVICE_CHATPAD_KEYBOARD_FILTER,   \
                                                            3001,                         \
                                                            METHOD_BUFFERED,              \
                                                            FILE_READ_DATA | FILE_WRITE_DATA)

#endif // CHATPAD_KEYBOARD_IOCTL_H

