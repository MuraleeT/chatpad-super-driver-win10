//
// Chatpad utility code that works with a KMDF USB filter driver, by
//   GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// Information for reading and writing to 360 controller:
//   http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller/UsbInfo
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//


//***
// Include files
//***

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <strsafe.h>

#include "chatpad_config.h"
#include "chatpad_control.h"
#include "chatpad_filter_ioctl.h"
#include "chatpad_keyboard_ioctl.h"
#include "chatpad_mouse_ioctl.h"


//***
// #defines and data structures
//***

// How many worker threads exist.
#define NUM_WORKER_THREADS          3

// How many pending reads should be used for USB endpoints.
#define NUM_PENDING_READS_TO_USE    2

// Table to convert 1-byte character codes to scan codes.
unsigned char chatpadScanCodeTable[256] =
{
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_7,                   // 0x11, '7'
   CHATPAD_KEY_6,                   // 0x12, '6'
   CHATPAD_KEY_5,                   // 0x13, '5'
   CHATPAD_KEY_4,                   // 0x14, '4'
   CHATPAD_KEY_3,                   // 0x15, '3'
   CHATPAD_KEY_2,                   // 0x16, '2'
   CHATPAD_KEY_1,                   // 0x17, '1'
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_U,                   // 0x21, 'u'
   CHATPAD_KEY_Y,                   // 0x22, 'y'
   CHATPAD_KEY_T,                   // 0x23, 't'
   CHATPAD_KEY_R,                   // 0x24, 'r'
   CHATPAD_KEY_E,                   // 0x25, 'e'
   CHATPAD_KEY_W,                   // 0x26, 'w'
   CHATPAD_KEY_Q,                   // 0x27, 'q'
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_J,                   // 0x31, 'j'
   CHATPAD_KEY_H,                   // 0x32, 'h'
   CHATPAD_KEY_G,                   // 0x33, 'g'
   CHATPAD_KEY_F,                   // 0x34, 'f'
   CHATPAD_KEY_D,                   // 0x35, 'd'
   CHATPAD_KEY_S,                   // 0x36, 's'
   CHATPAD_KEY_A,                   // 0x37, 'a'
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_N,                   // 0x41, 'n'
   CHATPAD_KEY_B,                   // 0x42, 'b'
   CHATPAD_KEY_V,                   // 0x43, 'v'
   CHATPAD_KEY_C,                   // 0x44, 'c'
   CHATPAD_KEY_X,                   // 0x45, 'x'
   CHATPAD_KEY_Z,                   // 0x46, 'z'
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_RIGHT,               // 0x51, right arrow
   CHATPAD_KEY_M,                   // 0x52, 'm'
   CHATPAD_KEY_PERIOD,              // 0x53, '.'
   CHATPAD_KEY_SPACEBAR,            // 0x54, spacebar
   CHATPAD_KEY_LEFT,                // 0x55, left arrow
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_COMMA,               // 0x62, ','
   CHATPAD_KEY_ENTER,               // 0x63, enter
   CHATPAD_KEY_P,                   // 0x64, 'p'
   CHATPAD_KEY_0,                   // 0x65, '0'
   CHATPAD_KEY_9,                   // 0x66, '9'
   CHATPAD_KEY_8,                   // 0x67, '8'
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_BACKSPACE,           // 0x71, backspace
   CHATPAD_KEY_L,                   // 0x72, 'l'
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_O,                   // 0x75, 'o'
   CHATPAD_KEY_I,                   // 0x76, 'i'
   CHATPAD_KEY_K,                   // 0x77, 'k'
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE,
   CHATPAD_KEY_NONE 
}; // end unsigned char chatpadScanCodeTable[256];

// This array is used to hold chatpad keyboard modifier mask values and their
// corresponding CHATPAD_KEY_ constants.
unsigned char chatpadModifierMaskTable[4][2] =
{
   { CHATPAD_MODIFIER_SHIFT,  CHATPAD_KEY_SHIFT    },
   { CHATPAD_MODIFIER_GREEN,  CHATPAD_KEY_GREEN    },
   { CHATPAD_MODIFIER_ORANGE, CHATPAD_KEY_ORANGE   },
   { CHATPAD_MODIFIER_PEOPLE, CHATPAD_KEY_PEOPLE   }
};


//***
// Global variables
//***

// TODO combine these into a state structure instead of using globals?

// Handles to worker threads.  These threads do operations like continuously
// reading from the chatpad and continuously reading controls data.
HANDLE hWorkerThreads[NUM_WORKER_THREADS] = { NULL, NULL, NULL };

// Event for sending control requests and other general IOCTLs that do not involve reading from USB endpoints.
HANDLE hControlRequestEvent = NULL;
// Mutex to protect the above event since it might be called from multiple
// threads.  The hControlsReadEvents and hChatpadReadEvents events are not
// protected since each of them is specific to a single thread.
HANDLE hControlRequestMutex = NULL;

// Events for reading controls data.
HANDLE hControlsReadEvents[NUM_PENDING_READS_TO_USE] = { NULL, NULL };

// Events for reading chatpad data.
HANDLE hChatpadReadEvents[NUM_PENDING_READS_TO_USE] = { NULL, NULL };

// *** Start of variables protected by hStatesMutex ***

// Handle to virtual keyboard device.
HANDLE hVirtualKeyboardDevice = INVALID_HANDLE_VALUE;
// Event for sending data to the virtual keyboard device.
HANDLE hVirtualKeyboardSendEvent = NULL;
// Current output keyboard modifiers.
BYTE currentKeyboardModifiers = KEYBOARD_MODIFIER_NONE;
// TODO eventually the below items might be combined into a structure.
// Previous output keyboard keys being held down, populated
// by KEYBOARD_KEY_ constants.
BYTE previousOutputKeysHeldDown[MAX_SIMULTANEOUS_KEYS] =
{
   KEYBOARD_KEY_NONE,
   KEYBOARD_KEY_NONE,
   KEYBOARD_KEY_NONE,
   KEYBOARD_KEY_NONE,
   KEYBOARD_KEY_NONE,
   KEYBOARD_KEY_NONE
};
// Current output keyboard keys being held down, populated
// by KEYBOARD_KEY_ constants.
BYTE currentOutputKeysHeldDown[MAX_SIMULTANEOUS_KEYS] =
{
   KEYBOARD_KEY_NONE,
   KEYBOARD_KEY_NONE,
   KEYBOARD_KEY_NONE,
   KEYBOARD_KEY_NONE,
   KEYBOARD_KEY_NONE,
   KEYBOARD_KEY_NONE
};
// Temporary keyboard modifiers to release (due to latched modifiers, and/or
// keys being bound to combinations like Alt+Tab), or KEYBOARD_MODIFIER_NONE
// if no modifier corresponds to the slot.
BYTE temporaryKeyboardModifiersToRelease[MAX_SIMULTANEOUS_KEYS] =
{
   KEYBOARD_MODIFIER_NONE,
   KEYBOARD_MODIFIER_NONE,
   KEYBOARD_MODIFIER_NONE,
   KEYBOARD_MODIFIER_NONE,
   KEYBOARD_MODIFIER_NONE,
   KEYBOARD_MODIFIER_NONE
};

// Handle to virtual mouse device.
HANDLE hVirtualMouseDevice = INVALID_HANDLE_VALUE;
// Event for sending data to the virtual mouse device.
HANDLE hVirtualMouseSendEvent = NULL;

// State data used for handling chatpad data.  This structure is
// protected by hStatesMutex since controller key binds may
// need to know the current state of chatpad modifiers.
ChatpadStateData* pChatpadStateData = NULL;

// Whether Windows Mode is currently active.
bool bWindowsModeActive = false;

// Array of pointers to action bindings triggered by the controller that are
// currently active.  Unused slots will be set to NULL.  If Windows Mode is
// deactivated and these bindings are still active, then StopBoundAction will
// be called on these bindings and the corresponding slots set back to NULL.
ChatpadDriverActionBinding* controllerTriggeredActions[MAX_SIMULTANEOUS_CONTROLLER_ACTIONS];

// Array of modifiers that track the modifiers from when each action in
// controllerTriggeredActions was triggered.
// TODO is there a more efficient way to do this that should be used here instead?
unsigned int controllerTriggeredActionModifiers[MAX_SIMULTANEOUS_CONTROLLER_ACTIONS];

// Array of booleans indicating whether the controller buttons are currently pressed if
// controls data is being filtered or intercepted.
bool controllerButtonsPressed[CONTROLLER_NUM_BUTTONS];

// *** End of variables protected by hStatesMutex ***

// Mutex to protect the above chatpad/controller/keyboard globals so that
// multiple threads can use them to trigger simulated mouse/keyboard/controller
// events.
HANDLE hStatesMutex = NULL;

// Unsigned integer representation of virtual mouse state.  This value AND'd
// with 0xFF000000 and shifted 24 bits to the right contains button
// information.  0x00FF0000 and 0x0000FF00 can be used, with 16-bit and 8-bit
// shifting, to obtain mouse X and Y offset information.  This variable AND'd
// with 0x000000FF produces mouse wheel offset information.
//
// This variable is protected by hMouseStateMutex.
unsigned int uVirtualMouseState = 0x00000000;

// Mutex to protect the above mouse state data so that multiple threads can
// update parts of it without causing problems.
HANDLE hMouseStateMutex = NULL;

// Configuration data for the driver, including data such as key mappings.
// This data is read-only after the configuration file is initially loaded,
// so it does not need to be protected by a mutex.
ChatpadDriverConfigStructure* pDriverConfigData = NULL;

// Event used to shut down worker threads.
HANDLE hWorkerThreadShutdownEvent = NULL;

// Special-case boolean used to shutdown the mouse worker thread since it
// currently does not pend on events.
// TODO is there a better way to handle shutting down the mouse worker thread?
//   (technically this may not even be required if the program just exits)
bool bShutdownMouseWorkerThread = false;

//***
// Functions
//***

// Logs a message.  This could be made configurable and made to log messages
// to something other than console output.
void LogMessageAdvanced(bool bAddNewline, char *pMessageString, ...)
{
   va_list formatList;

   va_start(formatList, pMessageString);
   vfprintf(stdout, pMessageString, formatList);
   if(true == bAddNewline)
   {
      fprintf(stdout, "\n");
   }
   va_end(formatList);
   fflush(stdout);
} // end LogMessageAdvanced

// Opens the controller device.  For now, this will probably only work with a single chatpad filter driver on a
// system, i.e. it will probably always try to open the first one.
int OpenController(PHANDLE pChatpadHandle)
{
   int nRetValue = SUCCESS;
   BOOL bAPIResult = TRUE;

   if((SUCCESS == nRetValue) &&
      (NULL == pChatpadHandle))
   {
      LogMessage("Invalid pointer provided to OpenController.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      LogMessage("Opening chatpad filter driver.\n");
      // Open the filter device, using the path \\.\ChatpadFilter
      // TODO do a little research into how such paths work, and/or look a little more closely at the CreateFile API.
      (*pChatpadHandle) = CreateFile(
        "\\\\.\\ChatpadFilter",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
      if(INVALID_HANDLE_VALUE != (*pChatpadHandle))
      {
         LogMessage("Successfully opened chatpad filter driver.\n");
      }
      else
      {
         LogMessage("Couldn't open chatpad filter:  %lu\n", GetLastError());
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end OpenController

// Opens the virtual keyboard device.  There should be only one virtual
// keyboard using the chatpad_keyboard.sys and chatpad_keyboard_kmdf.sys
// drivers on a system at a time.
int OpenVirtualKeyboard(PHANDLE pNewVirtualKeyboardHandle)
{
   int nRetValue = SUCCESS;
   BOOL bAPIResult = TRUE;

   if((SUCCESS == nRetValue) &&
      (NULL == pNewVirtualKeyboardHandle))
   {
      LogMessage("Invalid pointer provided to OpenVirtualKeyboard.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      LogMessage("Opening connection to virtual keyboard device.\n");
      // Open the filter device, using the path \\.\ChatpadKeyboardKMDF
      (*pNewVirtualKeyboardHandle) = CreateFile(
        "\\\\.\\ChatpadKeyboardKMDF",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
      if(INVALID_HANDLE_VALUE != (*pNewVirtualKeyboardHandle))
      {
         LogMessage("Successfully opened connection to virtual keyboard device.\n");
      }
      else
      {
         LogMessage("Couldn't open connection to virtual keyboard device:  %lu\n", GetLastError());
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end OpenVirtualKeyboard

// Opens the virtual mouse device.  There should be only one virtual
// mouse using the chatpad_mouse.sys and chatpad_mouse_kmdf.sys
// drivers on a system at a time.
int OpenVirtualMouse(PHANDLE pNewVirtualMouseHandle)
{
   int nRetValue = SUCCESS;
   BOOL bAPIResult = TRUE;

   if((SUCCESS == nRetValue) &&
      (NULL == pNewVirtualMouseHandle))
   {
      LogMessage("Invalid pointer provided to OpenVirtualMouse.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      LogMessage("Opening connection to virtual mouse device.\n");
      // Open the filter device, using the path \\.\ChatpadMouseKMDF
      (*pNewVirtualMouseHandle) = CreateFile(
        "\\\\.\\ChatpadMouseKMDF",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
      if(INVALID_HANDLE_VALUE != (*pNewVirtualMouseHandle))
      {
         LogMessage("Successfully opened connection to virtual mouse device.\n");
      }
      else
      {
         LogMessage("Couldn't open connection to virtual mouse device:  %lu\n", GetLastError());
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end OpenVirtualMouse

// Executes a control IOCTL, using hControlRequestMutex for synchronization and hControlRequestEvent 
// to detect when the IOCTL has finished executing.  This function returns when the IOCTL is completed.
// Due to the hControlRequestMutex synchronization, it should be safe to call this function from
// multiple threads.
//
// This function returns SUCCESS on success and FAILURE on failure.
int ExecuteControlIOCTL(
  HANDLE    hChatpadDevice,
  DWORD     ioctlCode,
  PVOID     dataToDevice,
  DWORD     bytesToSend,
  PVOID     pReceiveBuffer,
  DWORD     receiveBufferLength,
  PDWORD    pBytesTransferred)
{
   int         nRetValue            = SUCCESS;
   DWORD       waitResult           = 0;
   OVERLAPPED  overlappedStructure;

   // Get the control request mutex to ensure that only one control request is ongoing at a time,
   // and that two threads do not try to use the event at the same time.
   waitResult = WaitForSingleObject(hControlRequestMutex, INFINITE);
   // Make sure the wait actually succeeded.
   if(WAIT_OBJECT_0 != waitResult)
   {
      LogMessage("Wait for mutex failed:  %u\n", waitResult);
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) &&
      (INVALID_HANDLE_VALUE == hChatpadDevice))
   {
      LogMessage("Invalid handle provided to ExecuteControlIOCTL.\n");
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) && 
      (NULL == pBytesTransferred))
   {
      LogMessage("Invalid bytes transferred pointer provided to ExecuteControlIOCTL.\n");
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) &&
      (NULL == hControlRequestEvent))
   {
      LogMessage("Error, control request event handle is NULL.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      DWORD lastError            = 0;
      BOOL  operationComplete    = FALSE;

      // Initialize the OVERLAPPED structure.
      memset(&overlappedStructure, 0x00, sizeof(OVERLAPPED));
      overlappedStructure.hEvent = hControlRequestEvent;

      // Call the IOCTL to submit the control transfer through the filter driver.
      operationComplete = DeviceIoControl(
        hChatpadDevice,
        ioctlCode,
        dataToDevice,
        bytesToSend,
        pReceiveBuffer,
        receiveBufferLength,
        pBytesTransferred,
        &overlappedStructure);

      if(FALSE == operationComplete)
      {
         // If the request failed, it is hopefully pending.
         lastError = GetLastError();

         if(ERROR_IO_PENDING == lastError)
         {
            // Wait for the request to finish or the thread shutdown signal to be sent.
            DWORD ioctlWaitResult = 0;
            HANDLE waitHandles[2];

            waitHandles[0] = hControlRequestEvent;
            waitHandles[1] = hWorkerThreadShutdownEvent;
            ioctlWaitResult = WaitForMultipleObjects(
              2,
              waitHandles,
              FALSE,       // Stop waiting after any of the specified events finishes.
              INFINITE);   // No timeout.
            if(WAIT_OBJECT_0 == ioctlWaitResult)
            {
               // Make sure that the IOCTL actually completed successfully.
               operationComplete = GetOverlappedResult(
                 hChatpadDevice,
                 &overlappedStructure,
                 pBytesTransferred,
                 FALSE);
               if(FALSE == operationComplete)
               {
                  lastError = GetLastError();
               }
            }
            else if((WAIT_OBJECT_0 + 1) == ioctlWaitResult)
            {
               // The worker thread shutdown event was signaled.
               nRetValue      = FAILURE;
               lastError      = 0;
               LogMessage("Thread shutdown detected while in ExecuteControlIOCTL.\n");
            }
            else
            {
               LogMessage("WaitForMultipleObjects failed with return value %u\n", ioctlWaitResult);
               nRetValue      = FAILURE;
               lastError      = GetLastError();
            }
         } // end if(ERROR_IO_PENDING == lastError)

         //***
         // Note that if lastError is not ERROR_IO_PENDING and the operation
         // does not subsequently complete successfully, then it will be caught
         // in code below.
         //***

      } // end if(FALSE == operationComplete)

      // Note that even if operationComplete is FALSE above, it should be set to TRUE later in
      // the above code after the operation actually finishes.
      if(TRUE == operationComplete)
      {
         // For now, I am only printing this if a nonzero number of bytes were
         // transferred.  Otherwise, a line would be printed for every 0x001F
         // and 0x001E chatpad keep-alive control request.
         if((*pBytesTransferred) > 0)
         {
            LogMessage("Successfully sent IOCTL.  Bytes transferred:  %lu\n", *pBytesTransferred);
         }
      }
      else
      {
         LogMessage("IOCTL failed, lastError == %lu\n", lastError);
         nRetValue = FAILURE;
      }

      // Reset the event in case it was signaled.  If it was not signaled, this
      // call should simply have no effect.
      ResetEvent(hControlRequestEvent);
   } // end if(SUCCESS == nRetValue)

   // Release the mutex if we obtained it.
   if(WAIT_OBJECT_0 == waitResult)
   {
      if(0 == ReleaseMutex(hControlRequestMutex))
      {
         LogMessage("Control IOCTL mutex release failed:  %lu\n", GetLastError());
         nRetValue = FAILURE;
      }
   }

   return nRetValue;
} // end ExecuteControlIOCTL

// Sends a control request to the chatpad.
//
// To send additional data, include it in pExtraSendBuffer and specify the length of the data.
// To receive data, include it in pReceiveBuffer and specify the length of the buffer.
int SendControlRequest(
  HANDLE hChatpadDevice,
  XBOX_CONTROLLER_INTERFACE whichInterface,
  unsigned char requestType,
  unsigned char request,
  unsigned int value,
  unsigned int index,
  unsigned int length,
  unsigned char* pExtraSendBuffer = NULL,
  unsigned int extraSendBufferLength = 0,
  unsigned char* pReceiveBuffer = NULL,
  unsigned int receiveBufferLength = 0)
{
   int         nRetValue            = SUCCESS;
   DWORD       bytesToSend          = 0;
   DWORD       bytesTransferred     = 0;
   DWORD       dataToDeviceLength   = 0;
   PUCHAR      dataToDevice         = NULL;

   if((SUCCESS == nRetValue) &&
      (INVALID_HANDLE_VALUE == hChatpadDevice))
   {
      LogMessage("Invalid handle provided to SendControlRequest.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      // Allocate enough space for an interface number, the control transfer
      // parameters (more space than we need), and any extra data that is
      // to be sent.
      dataToDeviceLength = 1 + 32 + extraSendBufferLength;
      dataToDevice = static_cast<PUCHAR>(malloc(dataToDeviceLength));
      if(NULL == dataToDevice)
      {
         LogMessage("Memory allocation failure in SendControlRequest.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      memset(dataToDevice, 0x00, dataToDeviceLength);
      if(NULL != pReceiveBuffer)
      {
         memset(pReceiveBuffer, 0x00, receiveBufferLength);
      }

      // The first byte specifies which interface to use.
      // Technically, everything is being sent to the first interface right now, as
      // far as I know, but it seems to work so I'm leaving it like that.
      dataToDevice[bytesToSend++] = whichInterface;

      // Set up the USB control transfer.
      // See http://www.beyondlogic.org/usbnutshell/usb6.shtml for setup packet details.
      dataToDevice[bytesToSend++] = requestType;
      dataToDevice[bytesToSend++] = request;
      dataToDevice[bytesToSend++] = value & 0x000000FF;          // Value low.
      dataToDevice[bytesToSend++] = (value & 0x0000FF00) >> 8;   // Value high.
      dataToDevice[bytesToSend++] = index & 0x000000FF;          // Index low.
      dataToDevice[bytesToSend++] = (index & 0x0000FF00) >> 8;   // Index high.
      dataToDevice[bytesToSend++] = length & 0x000000FF;         // Length low.
      dataToDevice[bytesToSend++] = (length & 0x0000FF00) >> 8;  // Length high.

      // Additional bytes contain additional data to be sent to the device.
      for(unsigned int i = 0; i < extraSendBufferLength; i++)
      {
         dataToDevice[bytesToSend++] = pExtraSendBuffer[i];
      }

      // Execute the IOCTL.
      nRetValue = ExecuteControlIOCTL(
        hChatpadDevice,
        IOCTL_CHATPAD_SEND_CONTROL_TRANSFER,
        dataToDevice,
        bytesToSend,
        pReceiveBuffer,
        receiveBufferLength,
        &bytesTransferred);
   } // end if(SUCCESS == nRetValue)

   if(NULL != dataToDevice)
   {
      free(dataToDevice);
      dataToDevice = NULL;
   }

   // Sleep at least 12 milliseconds after each request, since sending them too close together
   // seems to potentially cause problems, at least for turning on individual key lights like
   // the green and the orange inidicators.
   Sleep(12);

   return nRetValue;
} // end SendControlRequest

// Sets the filter mode.
int SetFilterMode(
  HANDLE hChatpadDevice,
  CHATPAD_FILTER_MODE eNewFilterMode)
{
   int         nRetValue            = SUCCESS;
   DWORD       bytesTransferred     = 0;
   UCHAR       dataToDevice[1]      = { 0 };
   DWORD       bytesToSend          = 1;

   if((SUCCESS == nRetValue) &&
      (INVALID_HANDLE_VALUE == hChatpadDevice))
   {
      LogMessage("Invalid handle provided to SetFilterMode.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      dataToDevice[0] = eNewFilterMode;

      nRetValue = ExecuteControlIOCTL(
        hChatpadDevice,
        IOCTL_CHATPAD_SET_CONTROLS_FILTER_MODE,
        dataToDevice,
        bytesToSend,
        NULL,
        0,
        &bytesTransferred);
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end SetFilterMode

// Sets controls mappings.
// TODO NEXT make the parameters match the options once the options are implemented in the kernel driver.
int SetControlsMappings(
  HANDLE hChatpadDevice,
  PUCHAR writeBuffer,
  DWORD  bytesToWrite)
{
   int         nRetValue            = SUCCESS;
   DWORD       bytesTransferred     = 0;

   if((SUCCESS == nRetValue) &&
      ((INVALID_HANDLE_VALUE == hChatpadDevice)    ||
       (NULL == writeBuffer)                       ||
       (0 == bytesToWrite)))
   {
      LogMessage("Invalid parameters to SetControlsMappings.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      nRetValue = ExecuteControlIOCTL(
        hChatpadDevice,
        IOCTL_CHATPAD_SET_CONTROLS_MAPPINGS,
        writeBuffer,
        bytesToWrite,
        NULL,
        0,
        &bytesTransferred);
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end SetControlsMappings

// Writes data to the controls USB endpoint.
int WriteToControlsEndpoint(
  HANDLE hChatpadDevice,
  PUCHAR writeBuffer,
  DWORD  bytesToWrite)
{
   int         nRetValue            = SUCCESS;
   DWORD       bytesTransferred     = 0;

   if((SUCCESS == nRetValue) &&
      ((INVALID_HANDLE_VALUE == hChatpadDevice)    ||
       (NULL == writeBuffer)                       ||
       (0 == bytesToWrite)))
   {
      LogMessage("Invalid parameters to WriteToControlsEndpoint.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      nRetValue = ExecuteControlIOCTL(
        hChatpadDevice,
        IOCTL_CHATPAD_WRITE_TO_CONTROLS_ENDPOINT,
        writeBuffer,
        bytesToWrite,
        NULL,
        0,
        &bytesTransferred);
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end WriteToControlsEndpoint

// Executes an IOCTL to send keyboard data to the virtual keyboard device.
//
// IMPORTANT NOTE:  This function attempts to synchronize via hStatesMutex,
// so the calling thread should not hold that mutex (though it actually
// might work properly to acquire it twice).
//
// The enableValue parameter must be CHATPAD_KEYBOARD_KMDF_ENABLE or
// CHATPAD_KEYBOARD_KMDF_DISABLE.
//
// This function returns when the IOCTL is completed.
//
// This function returns SUCCESS on success and FAILURE on failure.
int SetVirtualKeyboardDeviceEnabled(UCHAR enableValue)
{
   int         nRetValue            = SUCCESS;
   DWORD       waitResult           = 0;
   OVERLAPPED  overlappedStructure;

   // Get the states mutex for synchronization.
   waitResult = WaitForSingleObject(hStatesMutex, INFINITE);
   // Make sure the wait actually succeeded.
   if(WAIT_OBJECT_0 != waitResult)
   {
      LogMessage("Wait for mutex failed:  %u\n", waitResult);
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) &&
      (INVALID_HANDLE_VALUE == hVirtualKeyboardDevice))
   {
      LogMessage("Invalid virtual keyboard device handle in SetVirtualKeyboardDeviceEnabled.\n");
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue)                          && 
      (CHATPAD_KEYBOARD_KMDF_ENABLE != enableValue)   &&
      (CHATPAD_KEYBOARD_KMDF_DISABLE != enableValue))
   {
      LogMessage("Invalid parameter provided to SetVirtualKeyboardDeviceEnabled.\n");
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) &&
      (NULL == hVirtualKeyboardSendEvent))
   {
      LogMessage("Error, virtual keyboard send event handle is NULL.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      DWORD    lastError            = 0;
      DWORD    bytesReturned        = 0;
      BOOL     operationComplete    = FALSE;
      UCHAR    sendBuffer[1];
      DWORD    sendBufferLength     = 0;

      sendBuffer[0]                 = enableValue;
      sendBufferLength = 1;

      // Initialize the OVERLAPPED structure.
      memset(&overlappedStructure, 0x00, sizeof(OVERLAPPED));
      overlappedStructure.hEvent = hVirtualKeyboardSendEvent;

      // Call the IOCTL to send data to the virtual keyboard control device.
      operationComplete = DeviceIoControl(
        hVirtualKeyboardDevice,
        IOCTL_CHATPAD_KEYBOARD_KMDF_SET_ENABLED,
        sendBuffer,
        sendBufferLength,
        NULL,
        0,
        &bytesReturned,
        &overlappedStructure);

      if(FALSE == operationComplete)
      {
         // If the request failed, it is hopefully pending.
         lastError = GetLastError();

         if(ERROR_IO_PENDING == lastError)
         {
            // Wait for the request to finish or the thread shutdown signal to be sent.
            DWORD ioctlWaitResult = 0;
            HANDLE waitHandles[2];

            waitHandles[0] = hVirtualKeyboardSendEvent;
            waitHandles[1] = hWorkerThreadShutdownEvent;
            ioctlWaitResult = WaitForMultipleObjects(
              2,
              waitHandles,
              FALSE,       // Stop waiting after any of the specified events finishes.
              INFINITE);   // No timeout.
            if(WAIT_OBJECT_0 == ioctlWaitResult)
            {
               // Make sure that the IOCTL actually completed successfully.
               operationComplete = GetOverlappedResult(
                 hVirtualKeyboardDevice,
                 &overlappedStructure,
                 &bytesReturned,
                 FALSE);
               if(FALSE == operationComplete)
               {
                  lastError = GetLastError();
               }
            }
            else if((WAIT_OBJECT_0 + 1) == ioctlWaitResult)
            {
               // The worker thread shutdown event was signaled.
               nRetValue      = FAILURE;
               lastError      = 0;
               LogMessage("Thread shutdown detected while in SetVirtualKeyboardDeviceEnabled.\n");
            }
            else
            {
               LogMessage("WaitForMultipleObjects failed with return value %u\n", ioctlWaitResult);
               nRetValue      = FAILURE;
               lastError      = GetLastError();
            }
         } // end if(ERROR_IO_PENDING == lastError)

         //***
         // Note that if lastError is not ERROR_IO_PENDING and the operation
         // does not subsequently complete successfully, then it will be caught
         // in code below.
         //***

      } // end if(FALSE == operationComplete)

      // Note that even if operationComplete is FALSE above, it should be set to TRUE later in
      // the above code after the operation actually finishes.
      if(TRUE != operationComplete)
      {
         LogMessage("Enable/disable virtual keyboard device IOCTL failed, lastError == %lu\n", lastError);
         nRetValue = FAILURE;
      }

      // Reset the event in case it was signaled.  If it was not signaled, this
      // call should simply have no effect.
      ResetEvent(hVirtualKeyboardSendEvent);
   } // end if(SUCCESS == nRetValue)

   // Release the mutex if we obtained it.
   if(WAIT_OBJECT_0 == waitResult)
   {
      if(0 == ReleaseMutex(hStatesMutex))
      {
         LogMessage("States mutex release failed:  %lu\n", GetLastError());
         nRetValue = FAILURE;
      }
   }

   return nRetValue;
} // end SetVirtualKeyboardDeviceEnabled

// Executes an IOCTL to send keyboard data to the virtual keyboard device.
//
// IMPORTANT NOTE:  This function must only be called if hStatesMutex
// is held by the calling thread!  This function does not attempt
// to synchronize on its own!
//
// The sendBuffer pointer must point to a buffer that has exactly the right
// amount of data to work with the IOCTL_CHATPAD_KEYBOARD_KMDF_SEND_DATA IOCTL,
// and sendBufferLength must match this length.
//
// This function returns when the IOCTL is completed.
//
// This function returns SUCCESS on success and FAILURE on failure.
int SendVirtualKeyboardData(
  PUCHAR sendBuffer,
  DWORD  sendBufferLength)
{
   int         nRetValue            = SUCCESS;
   OVERLAPPED  overlappedStructure;

   if((SUCCESS == nRetValue) &&
      (INVALID_HANDLE_VALUE == hVirtualKeyboardDevice))
   {
      LogMessage("Invalid virtual keyboard device handle in SendVirtualKeyboardData.\n");
      nRetValue = FAILURE;
   }

   // TODO make 7 a constant somewhere instead of hardcoded here...maybe add it to the IOCTL header?
   if((SUCCESS == nRetValue) && 
      ((NULL == sendBuffer)         ||
       (7 != sendBufferLength)))
   {
      LogMessage("Invalid parameters provided to SendVirtualKeyboardData.\n");
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) &&
      (NULL == hVirtualKeyboardSendEvent))
   {
      LogMessage("Error, virtual keyboard send event handle is NULL.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      DWORD lastError            = 0;
      DWORD bytesReturned        = 0;
      BOOL  operationComplete    = FALSE;

      // Save these keys as the previously held-down keys.
      memcpy(previousOutputKeysHeldDown, &(sendBuffer[1]), MAX_SIMULTANEOUS_KEYS);

      // Initialize the OVERLAPPED structure.
      memset(&overlappedStructure, 0x00, sizeof(OVERLAPPED));
      overlappedStructure.hEvent = hVirtualKeyboardSendEvent;

/*
      LogMessage(
        "[keyboard]:  [ 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ]\n",
        sendBuffer[0],
        sendBuffer[1],
        sendBuffer[2],
        sendBuffer[3],
        sendBuffer[4],
        sendBuffer[5],
        sendBuffer[6]);
*/
      // Call the IOCTL to send data to the virtual keyboard control device.
      operationComplete = DeviceIoControl(
        hVirtualKeyboardDevice,
        IOCTL_CHATPAD_KEYBOARD_KMDF_SEND_DATA,
        sendBuffer,
        sendBufferLength,
        NULL,
        0,
        &bytesReturned,
        &overlappedStructure);

      if(FALSE == operationComplete)
      {
         // If the request failed, it is hopefully pending.
         lastError = GetLastError();

         if(ERROR_IO_PENDING == lastError)
         {
            // Wait for the request to finish or the thread shutdown signal to be sent.
            DWORD ioctlWaitResult = 0;
            HANDLE waitHandles[2];

            waitHandles[0] = hVirtualKeyboardSendEvent;
            waitHandles[1] = hWorkerThreadShutdownEvent;
            ioctlWaitResult = WaitForMultipleObjects(
              2,
              waitHandles,
              FALSE,       // Stop waiting after any of the specified events finishes.
              INFINITE);   // No timeout.
            if(WAIT_OBJECT_0 == ioctlWaitResult)
            {
               // Make sure that the IOCTL actually completed successfully.
               operationComplete = GetOverlappedResult(
                 hVirtualKeyboardDevice,
                 &overlappedStructure,
                 &bytesReturned,
                 FALSE);
               if(FALSE == operationComplete)
               {
                  lastError = GetLastError();
               }
            }
            else if((WAIT_OBJECT_0 + 1) == ioctlWaitResult)
            {
               // The worker thread shutdown event was signaled.
               nRetValue      = FAILURE;
               lastError      = 0;
               LogMessage("Thread shutdown detected while in SendVirtualKeyboardData.\n");
            }
            else
            {
               LogMessage("WaitForMultipleObjects failed with return value %u\n", ioctlWaitResult);
               nRetValue      = FAILURE;
               lastError      = GetLastError();
            }
         } // end if(ERROR_IO_PENDING == lastError)

         //***
         // Note that if lastError is not ERROR_IO_PENDING and the operation
         // does not subsequently complete successfully, then it will be caught
         // in code below.
         //***

      } // end if(FALSE == operationComplete)

      // Note that even if operationComplete is FALSE above, it should be set to TRUE later in
      // the above code after the operation actually finishes.
      if(TRUE != operationComplete)
      {
         LogMessage("Send virtual keyboard data IOCTL failed, lastError == %lu\n", lastError);
         nRetValue = FAILURE;
      }

      // Reset the event in case it was signaled.  If it was not signaled, this
      // call should simply have no effect.
      ResetEvent(hVirtualKeyboardSendEvent);
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end SendVirtualKeyboardData

// This function sends a keyboard event with the previous held-down keys and the new modifiers,
// then sends a keyboard event with the new modifiers and current held-down keys.  This should
// allow keys bound to key combinations (such as Alt+Tab) to work properly with Windows and
// not make normal keyboard Alt keys (for example) act weird afterwards.
//
// IMPORTANT NOTE:  This function must only be called if hStatesMutex
// is held by the calling thread!  This function does not attempt
// to synchronize on its own!
int SendVirtualKeyboardDataWithNewModifiers(
  PUCHAR sendBuffer,
  DWORD  sendBufferLength)
{
   int            nRetValue               = SUCCESS;
   unsigned char  keyboardData[MAX_SIMULTANEOUS_KEYS + 1];

   if((NULL == sendBuffer) ||
      ((MAX_SIMULTANEOUS_KEYS + 1) != sendBufferLength))
   {
      LogMessage("Invalid parameters provided to SendVirtualKeyboardDataWithNewModifiers.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      // Add in modifiers and send an event.
      keyboardData[0] = sendBuffer[0];
      memcpy(&(keyboardData[1]), previousOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
      nRetValue = SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
   }

   if(SUCCESS == nRetValue)
   {
      // This sleep seems to make things involving alt-tab work well, so I am keeping it.
      // It should not make keypresses slow down unless, maybe, they were pressed really
      // rapidly and bound key modifiers were involved.
      Sleep(20);
      // Send an event with the new set of keys held down.
      nRetValue = SendVirtualKeyboardData(sendBuffer, MAX_SIMULTANEOUS_KEYS + 1);
   }

   // Note that at this point, the most recent SendVirtualKeyboardData call should
   // have updated previousOutputKeysHeldDown to the proper values.

   return nRetValue;
} // end SendVirtualKeyboardDataWithNewModifiers

// Executes an IOCTL to send mouse data to the virtual mouse device.
//
// IMPORTANT NOTE:  This function attempts to synchronize via hStatesMutex,
// so the calling thread should not hold that mutex (though it actually
// might work properly to acquire it twice).
//
// The enableValue parameter must be CHATPAD_MOUSE_KMDF_ENABLE or
// CHATPAD_MOUSE_KMDF_DISABLE.
//
// This function returns when the IOCTL is completed.
//
// This function returns SUCCESS on success and FAILURE on failure.
int SetVirtualMouseDeviceEnabled(UCHAR enableValue)
{
   int         nRetValue            = SUCCESS;
   DWORD       waitResult           = 0;
   OVERLAPPED  overlappedStructure;

   // Get the states mutex for synchronization.
   waitResult = WaitForSingleObject(hStatesMutex, INFINITE);
   // Make sure the wait actually succeeded.
   if(WAIT_OBJECT_0 != waitResult)
   {
      LogMessage("Wait for mutex failed:  %u\n", waitResult);
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) &&
      (INVALID_HANDLE_VALUE == hVirtualMouseDevice))
   {
      LogMessage("Invalid virtual mouse device handle in SetVirtualMouseDeviceEnabled.\n");
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue)                          && 
      (CHATPAD_MOUSE_KMDF_ENABLE != enableValue)      &&
      (CHATPAD_MOUSE_KMDF_DISABLE != enableValue))
   {
      LogMessage("Invalid parameter provided to SetVirtualMouseDeviceEnabled.\n");
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) &&
      (NULL == hVirtualMouseSendEvent))
   {
      LogMessage("Error, virtual mouse send event handle is NULL.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      DWORD    lastError            = 0;
      DWORD    bytesReturned        = 0;
      BOOL     operationComplete    = FALSE;
      UCHAR    sendBuffer[1];
      DWORD    sendBufferLength     = 0;

      sendBuffer[0]                 = enableValue;
      sendBufferLength = 1;

      // Initialize the OVERLAPPED structure.
      memset(&overlappedStructure, 0x00, sizeof(OVERLAPPED));
      overlappedStructure.hEvent = hVirtualMouseSendEvent;

      // Call the IOCTL to send data to the virtual mouse control device.
      operationComplete = DeviceIoControl(
        hVirtualMouseDevice,
        IOCTL_CHATPAD_MOUSE_KMDF_SET_ENABLED,
        sendBuffer,
        sendBufferLength,
        NULL,
        0,
        &bytesReturned,
        &overlappedStructure);

      if(FALSE == operationComplete)
      {
         // If the request failed, it is hopefully pending.
         lastError = GetLastError();

         if(ERROR_IO_PENDING == lastError)
         {
            // Wait for the request to finish or the thread shutdown signal to be sent.
            DWORD ioctlWaitResult = 0;
            HANDLE waitHandles[2];

            waitHandles[0] = hVirtualMouseSendEvent;
            waitHandles[1] = hWorkerThreadShutdownEvent;
            ioctlWaitResult = WaitForMultipleObjects(
              2,
              waitHandles,
              FALSE,       // Stop waiting after any of the specified events finishes.
              INFINITE);   // No timeout.
            if(WAIT_OBJECT_0 == ioctlWaitResult)
            {
               // Make sure that the IOCTL actually completed successfully.
               operationComplete = GetOverlappedResult(
                 hVirtualMouseDevice,
                 &overlappedStructure,
                 &bytesReturned,
                 FALSE);
               if(FALSE == operationComplete)
               {
                  lastError = GetLastError();
               }
            }
            else if((WAIT_OBJECT_0 + 1) == ioctlWaitResult)
            {
               // The worker thread shutdown event was signaled.
               nRetValue      = FAILURE;
               lastError      = 0;
               LogMessage("Thread shutdown detected while in SetVirtualMouseDeviceEnabled.\n");
            }
            else
            {
               LogMessage("WaitForMultipleObjects failed with return value %u\n", ioctlWaitResult);
               nRetValue      = FAILURE;
               lastError      = GetLastError();
            }
         } // end if(ERROR_IO_PENDING == lastError)

         //***
         // Note that if lastError is not ERROR_IO_PENDING and the operation
         // does not subsequently complete successfully, then it will be caught
         // in code below.
         //***

      } // end if(FALSE == operationComplete)

      // Note that even if operationComplete is FALSE above, it should be set to TRUE later in
      // the above code after the operation actually finishes.
      if(TRUE != operationComplete)
      {
         LogMessage("Enable/disable virtual mouse device IOCTL failed, lastError == %lu\n", lastError);
         nRetValue = FAILURE;
      }

      // Reset the event in case it was signaled.  If it was not signaled, this
      // call should simply have no effect.
      ResetEvent(hVirtualMouseSendEvent);
   } // end if(SUCCESS == nRetValue)

   // Release the mutex if we obtained it.
   if(WAIT_OBJECT_0 == waitResult)
   {
      if(0 == ReleaseMutex(hStatesMutex))
      {
         LogMessage("States mutex release failed:  %lu\n", GetLastError());
         nRetValue = FAILURE;
      }
   }

   return nRetValue;
} // end SetVirtualMouseDeviceEnabled

// Executes an IOCTL to send mouse data to the virtual mouse device.
//
// IMPORTANT NOTE:  This function must only be called if hStatesMutex
// is held by the calling thread!  This function does not attempt
// to synchronize on its own!
// TODO does this function even need the synchronization, as long as it
// is the sole user of the virtual mouse device?
//
// The sendBuffer pointer must point to a buffer that has exactly the right
// amount of data to work with the IOCTL_CHATPAD_MOUSE_KMDF_SEND_DATA IOCTL,
// and sendBufferLength must match this length.
//
// This function returns when the IOCTL is completed.
//
// This function returns SUCCESS on success and FAILURE on failure.
int SendVirtualMouseData(
  PUCHAR sendBuffer,
  DWORD  sendBufferLength)
{
   int         nRetValue            = SUCCESS;
   OVERLAPPED  overlappedStructure;

   if((SUCCESS == nRetValue) &&
      (INVALID_HANDLE_VALUE == hVirtualMouseDevice))
   {
      LogMessage("Invalid virtual mouse device handle in SendVirtualMouseData.\n");
      nRetValue = FAILURE;
   }

   // TODO make 4 a constant somewhere instead of hardcoded here...maybe add it to the IOCTL header?
   if((SUCCESS == nRetValue) && 
      ((NULL == sendBuffer)         ||
       (4 != sendBufferLength)))
   {
      LogMessage("Invalid parameters provided to SendVirtualMouseData.\n");
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) &&
      (NULL == hVirtualMouseSendEvent))
   {
      LogMessage("Error, virtual mouse send event handle is NULL.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      DWORD lastError            = 0;
      DWORD bytesReturned        = 0;
      BOOL  operationComplete    = FALSE;

      // Initialize the OVERLAPPED structure.
      memset(&overlappedStructure, 0x00, sizeof(OVERLAPPED));
      overlappedStructure.hEvent = hVirtualMouseSendEvent;

/*
      LogMessage(
        "[mouse]:  [ 0x%02x 0x%02x 0x%02x 0x%02x ]\n",
        sendBuffer[0],
        sendBuffer[1],
        sendBuffer[2],
        sendBuffer[3]);
*/
      // Call the IOCTL to send data to the virtual mouse control device.
      operationComplete = DeviceIoControl(
        hVirtualMouseDevice,
        IOCTL_CHATPAD_MOUSE_KMDF_SEND_DATA,
        sendBuffer,
        sendBufferLength,
        NULL,
        0,
        &bytesReturned,
        &overlappedStructure);

      if(FALSE == operationComplete)
      {
         // If the request failed, it is hopefully pending.
         lastError = GetLastError();

         if(ERROR_IO_PENDING == lastError)
         {
            // Wait for the request to finish or the thread shutdown signal to be sent.
            DWORD ioctlWaitResult = 0;
            HANDLE waitHandles[2];

            waitHandles[0] = hVirtualMouseSendEvent;
            waitHandles[1] = hWorkerThreadShutdownEvent;
            ioctlWaitResult = WaitForMultipleObjects(
              2,
              waitHandles,
              FALSE,       // Stop waiting after any of the specified events finishes.
              INFINITE);   // No timeout.
            if(WAIT_OBJECT_0 == ioctlWaitResult)
            {
               // Make sure that the IOCTL actually completed successfully.
               operationComplete = GetOverlappedResult(
                 hVirtualMouseDevice,
                 &overlappedStructure,
                 &bytesReturned,
                 FALSE);
               if(FALSE == operationComplete)
               {
                  lastError = GetLastError();
               }
            }
            else if((WAIT_OBJECT_0 + 1) == ioctlWaitResult)
            {
               // The worker thread shutdown event was signaled.
               nRetValue      = FAILURE;
               lastError      = 0;
               LogMessage("Thread shutdown detected while in SendVirtualMouseData.\n");
            }
            else
            {
               LogMessage("WaitForMultipleObjects failed with return value %u\n", ioctlWaitResult);
               nRetValue      = FAILURE;
               lastError      = GetLastError();
            }
         } // end if(ERROR_IO_PENDING == lastError)

         //***
         // Note that if lastError is not ERROR_IO_PENDING and the operation
         // does not subsequently complete successfully, then it will be caught
         // in code below.
         //***

      } // end if(FALSE == operationComplete)

      // Note that even if operationComplete is FALSE above, it should be set to TRUE later in
      // the above code after the operation actually finishes.
      if(TRUE != operationComplete)
      {
         LogMessage("Send virtual mouse data IOCTL failed, lastError == %lu\n", lastError);
         nRetValue = FAILURE;
      }

      // Reset the event in case it was signaled.  If it was not signaled, this
      // call should simply have no effect.
      ResetEvent(hVirtualMouseSendEvent);
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end SendVirtualMouseData

// Forward declaration of StopBoundAction() so that StartBoundAction() can call it.
int StopBoundAction(
  HANDLE                      hChatpadDevice,
  ChatpadDriverActionBinding* pActionBinding);

// Function to start an action, such as a simulated key event.
//
// IMPORTANT NOTE:  This function must only be called if hStatesMutex
// is held by the calling thread!  This function does not attempt
// to synchronize on its own!
int StartBoundAction(
  HANDLE                      hChatpadDevice,
  ChatpadDriverActionBinding* pActionBinding)
{
   int            nRetValue                        = SUCCESS;
   bool           bActionNeedsStarted              = true;
   bool           bWindowsModeWasJustToggled       = false;
   unsigned char  keyboardData[MAX_SIMULTANEOUS_KEYS + 1];

//   LogMessage("StartBoundAction was called.\n");

   if(NULL == pActionBinding)
   {
      LogMessage("Invalid pointer provided to StartBoundAction.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      if(BINDING_SIMULATED_KEY == pActionBinding->bindingType)
      {
         int slotIndex = 0;

         // Check whether the action even needs started, i.e. whether the
         // simulated key is already down.
         for(slotIndex = 0; slotIndex < MAX_SIMULTANEOUS_KEYS; slotIndex++)
         {
            if(currentOutputKeysHeldDown[slotIndex] == pActionBinding->uActionCode)
            {
               bActionNeedsStarted = false;
               break;
            }
         }

         if(true == bActionNeedsStarted)
         {
            // Find a slot to store this simulated key's information.
            for(slotIndex = 0; slotIndex < MAX_SIMULTANEOUS_KEYS; slotIndex++)
            {
               if(currentOutputKeysHeldDown[slotIndex] == KEYBOARD_KEY_NONE)
               {
                  // Update the keys held down structure with a KEYBOARD_KEY_ constant.
                  currentOutputKeysHeldDown[slotIndex] = static_cast<BYTE>(pActionBinding->uActionCode & 0x000000FF);
                  break;
               }
            }

            if(MAX_SIMULTANEOUS_KEYS == slotIndex)
            {
               LogMessage("Error, too many keys currently held down.\n");
               nRetValue = FAILURE;
            }

            if(SUCCESS == nRetValue)
            {
               // Whether new modifiers (i.e. latched, or bound to the output action) are used.
               bool bUsingNewModifiers = false;

               // Simulate the keyboard event.

               // Start with the current set of keyboard modifiers.
               keyboardData[0] = currentKeyboardModifiers;

               // Notice that more than one of these may be active at a time, so no "else if" blocks are used.
               if(true == pChatpadStateData->leftControlLatchActive)
               {
                  keyboardData[0] |= KEYBOARD_MODIFIER_LEFT_CONTROL;
                  pChatpadStateData->leftControlLatchActive = false;
               }

               if(true == pChatpadStateData->leftAltLatchActive)
               {
                  keyboardData[0] |= KEYBOARD_MODIFIER_LEFT_ALT;
                  pChatpadStateData->leftAltLatchActive = false;
               }

               if(true == pChatpadStateData->leftGuiLatchActive)
               {
                  keyboardData[0] |= KEYBOARD_MODIFIER_LEFT_GUI;
                  pChatpadStateData->leftGuiLatchActive = false;
               }

               if(true == pChatpadStateData->rightControlLatchActive)
               {
                  keyboardData[0] |= KEYBOARD_MODIFIER_RIGHT_CONTROL;
                  pChatpadStateData->rightControlLatchActive = false;
               }

               if(true == pChatpadStateData->rightShiftLatchActive)
               {
                  keyboardData[0] |= KEYBOARD_MODIFIER_RIGHT_SHIFT;
                  pChatpadStateData->rightShiftLatchActive = false;
               }

               if(true == pChatpadStateData->rightAltLatchActive)
               {
                  keyboardData[0] |= KEYBOARD_MODIFIER_RIGHT_ALT;
                  pChatpadStateData->rightAltLatchActive = false;
               }

               if(true == pChatpadStateData->rightGuiLatchActive)
               {
                  keyboardData[0] |= KEYBOARD_MODIFIER_RIGHT_GUI;
                  pChatpadStateData->rightGuiLatchActive = false;
               }

               // Note that the left shift keyboard modifier is ignored here unless
               // it is in the binding itself.  This might change some day, but for
               // now it seems simplest to require a long configuration file that
               // will provide bindings for each "shift + whatever" combination
               // instead of using all of the keyboard modifiers directly.  Doing
               // things this way allows one to map a "shift + 1" combination to
               // F1, without the result being interpreted by Windows as "shift-F1".
               keyboardData[0] = ((keyboardData[0] & (~KEYBOARD_MODIFIER_LEFT_SHIFT)) |
                                  pActionBinding->uActionModifiers);

               // Detect whether the modifiers have changed.
               if(keyboardData[0] != (currentKeyboardModifiers & (~KEYBOARD_MODIFIER_LEFT_SHIFT)))
               {
                  bUsingNewModifiers = true;
               }

               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               if(true == bUsingNewModifiers)
               {
//                  LogMessage("Using new modifiers!\n");
                  temporaryKeyboardModifiersToRelease[slotIndex] = keyboardData[0];
                  SendVirtualKeyboardDataWithNewModifiers(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               }
               else
               {
                  temporaryKeyboardModifiersToRelease[slotIndex] = KEYBOARD_MODIFIER_NONE;
                  SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               }

               // Handle the special case of caps lock turning the shift light on and off.
               if((KEYBOARD_KEY_CAPS_LOCK == pActionBinding->uActionCode) &&
                  (true == pDriverConfigData->bCapsLockControlsShiftIndicator))
               {
                  if(false == pChatpadStateData->capsLockActive)
                  {
                     pChatpadStateData->capsLockActive = true;
                     SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0008, 0x0002, 0x0000);
                  }
                  else
                  {
                     pChatpadStateData->capsLockActive = false;
                     SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0000, 0x0002, 0x0000);
                  }
               }
            } // end if(SUCCESS == nRetValue)
         } // end if(true == bActionNeedsStarted)
   else
   {
   // TODO NEXT REMOVE after checking that this message works with controller button keybinds
   LogMessage("Action does not need started.\n");
   }
      } // end if(BINDING_SIMULATED_KEY == pActionBinding->bindingType)
      else if(BINDING_CONTROLLER == pActionBinding->bindingType)
      {
         // These bindings are simply ignored since they are handled by the chatpad filter driver itself.
      } // end else if(BINDING_CONTROLLER == pActionBinding->bindingType)
      else if(BINDING_SIMULATED_MOUSE_ACTION == pActionBinding->bindingType)
      {
         // Get the mouse state mutex for synchronization.
         DWORD mouseStateWaitResult = WaitForSingleObject(hMouseStateMutex, INFINITE);
         // Make sure the wait actually succeeded.
         if(WAIT_OBJECT_0 != mouseStateWaitResult)
         {
            LogMessage("Wait for mouse state mutex failed:  %u\n", mouseStateWaitResult);
            nRetValue = FAILURE;
         }

         switch(pActionBinding->uActionCode)
         {
            // TODO eventually add constants for the mouse bits instead of hardcoding values?

            case MOUSE_CLICK_LEFT:
            {
               uVirtualMouseState |= (1 << 24);
               break;
            } // end case MOUSE_CLICK_LEFT

            case MOUSE_CLICK_MIDDLE:
            {
               uVirtualMouseState |= (1 << 26);
               break;
            } // end case MOUSE_CLICK_MIDDLE

            case MOUSE_CLICK_RIGHT:
            {
               uVirtualMouseState |= (1 << 25);
               break;
            } // end case MOUSE_CLICK_RIGHT

            case MOUSE_WHEEL_UP:
            {
               // If the wheel byte was sent over and over when the mouse moved,
               // Windows would keep trying to continuously scroll.  So, we simply
               // send our own single scroll event here.

               UCHAR          mouseDataBuffer[VIRTUAL_MOUSE_MESSAGE_NUM_BYTES];
               mouseDataBuffer[0] = (uVirtualMouseState & 0xFF000000) >> 24;
               mouseDataBuffer[1] = (uVirtualMouseState & 0x00FF0000) >> 16;
               mouseDataBuffer[2] = (uVirtualMouseState & 0x0000FF00) >> 8;

               // Set the wheel byte to 1.
               mouseDataBuffer[3] = 0x01;

               nRetValue = SendVirtualMouseData(mouseDataBuffer, VIRTUAL_MOUSE_MESSAGE_NUM_BYTES);
               if(SUCCESS != nRetValue)
               {
                  LogMessage("Error sending wheel up mouse message.\n");
               }
               break;
            } // end case MOUSE_WHEEL_UP

            case MOUSE_WHEEL_DOWN:
            {
               // If the wheel byte was sent over and over when the mouse moved,
               // Windows would keep trying to continuously scroll.  So, we simply
               // send our own single scroll event here.

               UCHAR          mouseDataBuffer[VIRTUAL_MOUSE_MESSAGE_NUM_BYTES];
               mouseDataBuffer[0] = (uVirtualMouseState & 0xFF000000) >> 24;
               mouseDataBuffer[1] = (uVirtualMouseState & 0x00FF0000) >> 16;
               mouseDataBuffer[2] = (uVirtualMouseState & 0x0000FF00) >> 8;

               // Set the wheel byte to -1.
               mouseDataBuffer[3] = 0xFF;

               nRetValue = SendVirtualMouseData(mouseDataBuffer, VIRTUAL_MOUSE_MESSAGE_NUM_BYTES);
               if(SUCCESS != nRetValue)
               {
                  LogMessage("Error sending wheel down mouse message.\n");
               }
               break;
            } // end case MOUSE_WHEEL_DOWN

            default:
            {
               // This should never happen.
               LogMessage("Unknown mouse action %d.\n", pActionBinding->uActionCode);
               nRetValue = FAILURE;
               break;
            }
         } // end switch(pActionBinding->uActionCode)

         // Release the mouse state mutex if we obtained it.
         if(WAIT_OBJECT_0 == mouseStateWaitResult)
         {
            if(0 == ReleaseMutex(hMouseStateMutex))
            {
               LogMessage("Mouse state mutex release failed:  %lu\n", GetLastError());
               nRetValue = FAILURE;
            }
         }
      } // end else if(BINDING_SIMULATED_MOUSE_ACTION == pActionBinding->bindingType)
      else if(BINDING_SPECIAL == pActionBinding->bindingType)
      {
         switch(pActionBinding->uActionCode)
         {
            case SPECIAL_ACTION_LEFT_CTRL:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_LEFT_CONTROL;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_LEFT_CTRL_LATCH:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_LEFT_CONTROL;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               if(false == pChatpadStateData->leftControlLatchActive)
               {
                  pChatpadStateData->leftControlLatchActive = true;
               }
               else
               {
                  pChatpadStateData->leftControlLatchActive = false;
               }
               break;
            }

            // Only the left shift key is currently supported as a chatpad key modifier
            // (plus, it uses the shift key indicator light).  There is a note in
            // README.TXT that talks about this.

            case SPECIAL_ACTION_LEFT_SHIFT:
            {
               pChatpadStateData->currentModifiers |= CHATPAD_MODIFIER_SHIFT;
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_LEFT_SHIFT;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_LEFT_SHIFT_LATCH:
            {
               if(CHATPAD_MODIFIER_SHIFT != (pActionBinding->uSourceActionModifiers & CHATPAD_MODIFIER_SHIFT))
               {
                  pChatpadStateData->currentModifiers |= CHATPAD_MODIFIER_SHIFT;
                  currentKeyboardModifiers |= KEYBOARD_MODIFIER_LEFT_SHIFT;
                  // Send an update message so that Windows knows that the key is held down.
                  keyboardData[0] = currentKeyboardModifiers;
                  memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
                  SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
                  if(false == pChatpadStateData->leftShiftLatchActive)
                  {
                     pChatpadStateData->leftShiftLatchActive = true;
                     // Turn the shift light on unless caps lock is configured to control that light.
                     if(false == pDriverConfigData->bCapsLockControlsShiftIndicator)
                     {
                        SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0008, 0x0002, 0x0000);
                     }
                  }
                  else
                  {
                     pChatpadStateData->leftShiftLatchActive = false;
                     // Turn the shift light off unless caps lock is configured to control that light.
                     if(false == pDriverConfigData->bCapsLockControlsShiftIndicator)
                     {
                        SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0000, 0x0002, 0x0000);
                     }
                  }
               }
               break;
            }

            case SPECIAL_ACTION_LEFT_ALT:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_LEFT_ALT;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_LEFT_ALT_LATCH:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_LEFT_ALT;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               if(false == pChatpadStateData->leftAltLatchActive)
               {
                  pChatpadStateData->leftAltLatchActive = true;
               }
               else
               {
                  pChatpadStateData->leftAltLatchActive = false;
               }
               break;
            }

            case SPECIAL_ACTION_LEFT_GUI:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_LEFT_GUI;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_LEFT_GUI_LATCH:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_LEFT_GUI;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               if(false == pChatpadStateData->leftGuiLatchActive)
               {
                  pChatpadStateData->leftGuiLatchActive = true;
               }
               else
               {
                  pChatpadStateData->leftGuiLatchActive = false;
               }
               break;
            }

            case SPECIAL_ACTION_RIGHT_CTRL:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_RIGHT_CONTROL;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_CTRL_LATCH:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_RIGHT_CONTROL;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               if(false == pChatpadStateData->rightControlLatchActive)
               {
                  pChatpadStateData->rightControlLatchActive = true;
               }
               else
               {
                  pChatpadStateData->rightControlLatchActive = false;
               }
               break;
            }

            case SPECIAL_ACTION_RIGHT_SHIFT:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_RIGHT_SHIFT;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_SHIFT_LATCH:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_RIGHT_SHIFT;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               if(false == pChatpadStateData->rightShiftLatchActive)
               {
                  pChatpadStateData->rightShiftLatchActive = true;
               }
               else
               {
                  pChatpadStateData->rightShiftLatchActive = false;
               }
               break;
            }

            case SPECIAL_ACTION_RIGHT_ALT:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_RIGHT_ALT;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_ALT_LATCH:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_RIGHT_ALT;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               if(false == pChatpadStateData->rightAltLatchActive)
               {
                  pChatpadStateData->rightAltLatchActive = true;
               }
               else
               {
                  pChatpadStateData->rightAltLatchActive = false;
               }
               break;
            }

            case SPECIAL_ACTION_RIGHT_GUI:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_RIGHT_GUI;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_GUI_LATCH:
            {
               currentKeyboardModifiers |= KEYBOARD_MODIFIER_RIGHT_GUI;
               // Send an update message so that Windows knows that the key is held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               if(false == pChatpadStateData->rightGuiLatchActive)
               {
                  pChatpadStateData->rightGuiLatchActive = true;
               }
               else
               {
                  pChatpadStateData->rightGuiLatchActive = false;
               }
               break;
            }

            case SPECIAL_ACTION_ORANGE:
            {
               pChatpadStateData->currentModifiers |= CHATPAD_MODIFIER_ORANGE;
               break;
            }

            // For the chatpad latch handlers below, note that they do not do
            // anything if their corresponding modifier was used as a source
            // modifier.  This is so that pressing (for instance) the orange
            // toggle once, letting go, and pressing it again will work.  The
            // toggle will be released automatically when the key is pressed,
            // and thus this code should not attempt to set it again, assuming
            // that things are simplest that way.

            case SPECIAL_ACTION_ORANGE_LATCH:
            {
               if(CHATPAD_MODIFIER_ORANGE != (pActionBinding->uSourceActionModifiers & CHATPAD_MODIFIER_ORANGE))
               {
                  pChatpadStateData->currentModifiers |= CHATPAD_MODIFIER_ORANGE;
                  if(false == pChatpadStateData->orangeLatchActive)
                  {
                     pChatpadStateData->orangeLatchActive = true;
                     // Turn the orange light on.
                     SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x000a, 0x0002, 0x0000);
                  }
                  else
                  {
                     pChatpadStateData->orangeLatchActive = false;
                     // Turn the orange light off.
                     SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0002, 0x0002, 0x0000);
                  }
               }
               break;
            }

            case SPECIAL_ACTION_GREEN:
            {
               pChatpadStateData->currentModifiers |= CHATPAD_MODIFIER_GREEN;
               break;
            }

            case SPECIAL_ACTION_GREEN_LATCH:
            {
               if(CHATPAD_MODIFIER_GREEN != (pActionBinding->uSourceActionModifiers & CHATPAD_MODIFIER_GREEN))
               {
                  pChatpadStateData->currentModifiers |= CHATPAD_MODIFIER_GREEN;
                  if(false == pChatpadStateData->greenLatchActive)
                  {
                     pChatpadStateData->greenLatchActive = true;
                     // Turn the green light on.
                     SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0009, 0x0002, 0x0000);
                  }
                  else
                  {
                     pChatpadStateData->greenLatchActive = false;
                     // Turn the green light off.
                     SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0001, 0x0002, 0x0000);
                  }
               }
               break;
            }

            case SPECIAL_ACTION_PEOPLE:
            {
               pChatpadStateData->currentModifiers |= CHATPAD_MODIFIER_PEOPLE;
               break;
            }

            case SPECIAL_ACTION_PEOPLE_LATCH:
            {
               if(CHATPAD_MODIFIER_PEOPLE != (pActionBinding->uSourceActionModifiers & CHATPAD_MODIFIER_PEOPLE))
               {
                  pChatpadStateData->currentModifiers |= CHATPAD_MODIFIER_PEOPLE;
                  if(false == pChatpadStateData->peopleLatchActive)
                  {
                     pChatpadStateData->peopleLatchActive = true;
                     // Turn the people light on.
                     SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x000b, 0x0002, 0x0000);
                  }
                  else
                  {
                     pChatpadStateData->peopleLatchActive = false;
                     // Turn the people light off.
                     SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0003, 0x0002, 0x0000);
                  }
               }
               break;
            }

            case SPECIAL_ACTION_TOGGLE_WINDOWS_MODE:
            {
               if(true == pDriverConfigData->bEnableWindowsMode)
               {
                  if(false == bWindowsModeActive)
                  {
                     // Enable controls data interception.
                     SetFilterMode(hChatpadDevice, FILTER_MODE_INTERCEPTED);

                     bWindowsModeActive            = true;
                     bWindowsModeWasJustToggled    = true;

                     // Turn the people light on if the people light is being used to indicate Windows Mode.
                     if(true == pDriverConfigData->bEnableWindowsModePeopleKeyIndicator)
                     {
                        SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x000b, 0x0002, 0x0000);
                     }
                  } // end case where Windows Mode is turned on
                  else
                  {
                     // Enable filtering unless the passthrough mode should be used.
                     if(true == pDriverConfigData->bEnableIngameRemappedButtons)
                     {
                        SetFilterMode(hChatpadDevice, FILTER_MODE_FILTERED);
                     }
                     else
                     {
                        SetFilterMode(hChatpadDevice, FILTER_MODE_UNFILTERED);
                     }

                     bWindowsModeActive            = false;
                     bWindowsModeWasJustToggled    = true;

                     // Turn the people light off if the people light is being used to indicate Windows Mode.
                     if(true == pDriverConfigData->bEnableWindowsModePeopleKeyIndicator)
                     {
                        SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0003, 0x0002, 0x0000);
                     }

                     // Send messages to clear any virtual key state or mouse state that might exist to prevent
                     // potential problems like the mouse cursor getting stuck moving left after leaving Windows Mode.

                     int            nSendResult = SUCCESS;
                     UCHAR          keyboardDataBuffer[MAX_SIMULTANEOUS_KEYS + 1];
                     memset(keyboardDataBuffer, 0x00, MAX_SIMULTANEOUS_KEYS + 1);
                     nSendResult = SendVirtualKeyboardData(keyboardDataBuffer, MAX_SIMULTANEOUS_KEYS + 1);
                     if(SUCCESS != nSendResult)
                     {
                        LogMessage("Error sending no action key message.\n");
                     }

                     // Get the mouse state mutex for synchronization.
                     DWORD mouseStateWaitResult = WaitForSingleObject(hMouseStateMutex, INFINITE);
                     // Make sure the wait actually succeeded.
                     if(WAIT_OBJECT_0 != mouseStateWaitResult)
                     {
                        LogMessage("Wait for mouse state mutex failed:  %u\n", mouseStateWaitResult);
                        nRetValue = FAILURE;
                     }

                     UCHAR          mouseDataBuffer[VIRTUAL_MOUSE_MESSAGE_NUM_BYTES];

                     uVirtualMouseState = 0x00000000;
                     memset(mouseDataBuffer, 0x00, VIRTUAL_MOUSE_MESSAGE_NUM_BYTES);
                     nSendResult = SendVirtualMouseData(mouseDataBuffer, VIRTUAL_MOUSE_MESSAGE_NUM_BYTES);
                     if(SUCCESS != nSendResult)
                     {
                        LogMessage("Error sending no action mouse message.\n");
                     }

                     // Release the mouse state mutex if we obtained it.
                     if(WAIT_OBJECT_0 == mouseStateWaitResult)
                     {
                        if(0 == ReleaseMutex(hMouseStateMutex))
                        {
                           LogMessage("Mouse state mutex release failed:  %lu\n", GetLastError());
                           nRetValue = FAILURE;
                        }
                     }
                  } // end case where Windows Mode is turned off
               }
               else
               {
                  LogMessage("Windows mode not supported due to config file setting.\n");
               }
               break;
            }

            default:
            {
               // This should never happen.
               LogMessage("Unknown special action %d.\n", pActionBinding->uActionCode);
               nRetValue = FAILURE;
               break;
            }
         } // end switch(pActionBinding->uActionCode)
      } // end else if(BINDING_SPECIAL == pActionBinding->bindingType)
      else
      {
         // This should never happen.
         LogMessage("Unknown binding type %d.\n", pActionBinding->bindingType);
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   // If Windows Mode was just toggled, then stop any active
   // controller-triggered actions, and clear the array that tracks which
   // filtered or intercepted buttons are pressed.  Otherwise, toggling
   // Windows Mode might make actions repeat until Windows Mode is toggled
   // again or the utility exits.
   if(true == bWindowsModeWasJustToggled)
   {
      for(int i = 0; i < MAX_SIMULTANEOUS_CONTROLLER_ACTIONS; i++)
      {
         if(NULL != controllerTriggeredActions[i])
         {
            StopBoundAction(hChatpadDevice, controllerTriggeredActions[i]);
            controllerTriggeredActions[i]          = NULL;
            controllerTriggeredActionModifiers[i]  = NULL;
         }
      }

      for(int i = 0; i < CONTROLLER_NUM_BUTTONS; i++)
      {
         controllerButtonsPressed[i] = false;
      }
   }

   return nRetValue;
} // end StartBoundAction

// Function to stop an action, such as a simulated key event.
//
// IMPORTANT NOTE:  This function must only be called if hStatesMutex
// is held by the calling thread!  This function does not attempt
// to synchronize on its own!
int StopBoundAction(
  HANDLE                      hChatpadDevice,
  ChatpadDriverActionBinding* pActionBinding)
{
   int            nRetValue            = SUCCESS;
   bool           bActionNeedsStopped  = false;
   unsigned char  keyboardData[MAX_SIMULTANEOUS_KEYS + 1];

//   LogMessage("StopBoundAction was called.\n");

   if(NULL == pActionBinding)
   {
      LogMessage("Invalid pointer provided to StopBoundAction.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      if(BINDING_SIMULATED_KEY == pActionBinding->bindingType)
      {
         int slotIndex = 0;

         // Check whether the action even needs stopped, i.e. whether the
         // key code exists in the array of currently held-down keys.
         for(slotIndex = 0; slotIndex < MAX_SIMULTANEOUS_KEYS; slotIndex++)
         {
            if(currentOutputKeysHeldDown[slotIndex] == pActionBinding->uActionCode)
            {
               bActionNeedsStopped = true;
               break;
            }
         }

         if(true == bActionNeedsStopped)
         {
// TODO NEXT test Alt-Tab via two different keys
// TODO NEXT test Alt-Tab from a single bound key and make sure releasing it does not keep alt held down
            if(SUCCESS == nRetValue)
            {
               // Simulate the keyboard event.

               // Clear the slot in the keys held down structure.
               currentOutputKeysHeldDown[slotIndex] = KEYBOARD_KEY_NONE;

               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);

               if(KEYBOARD_MODIFIER_NONE != temporaryKeyboardModifiersToRelease[slotIndex])
               {
                  // If any latched modifiers or bound combination modifiers were active, send one extra
                  // message with those modifiers before clearing them so that everything will be handled
                  // properly by Windows.
                  keyboardData[0] = currentKeyboardModifiers | temporaryKeyboardModifiersToRelease[slotIndex];
                  SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);

                  temporaryKeyboardModifiersToRelease[slotIndex] = KEYBOARD_MODIFIER_NONE;
               } // end if(KEYBOARD_MODIFIER_NONE != temporaryKeyboardModifiersToRelease[slotIndex])

               keyboardData[0] = currentKeyboardModifiers;
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
            } // end if(SUCCESS == nRetValue)
         } // end if(true == bActionNeedsStopped)
   else
   {
   // TODO NEXT REMOVE after checking that this message works with controller button keybinds
   LogMessage("Action does not need stopped.\n");
   }
      } // end if(BINDING_SIMULATED_KEY == pActionBinding->bindingType)
      else if(BINDING_CONTROLLER == pActionBinding->bindingType)
      {
         // These bindings are simply ignored since they are handled by the chatpad filter driver itself.
      } // end else if(BINDING_CONTROLLER == pActionBinding->bindingType)
      else if(BINDING_SIMULATED_MOUSE_ACTION == pActionBinding->bindingType)
      {
         // Get the mouse state mutex for synchronization.
         DWORD mouseStateWaitResult = WaitForSingleObject(hMouseStateMutex, INFINITE);
         // Make sure the wait actually succeeded.
         if(WAIT_OBJECT_0 != mouseStateWaitResult)
         {
            LogMessage("Wait for mouse state mutex failed:  %u\n", mouseStateWaitResult);
            nRetValue = FAILURE;
         }

         switch(pActionBinding->uActionCode)
         {
            // TODO eventually add constants for the mouse bits instead of hardcoding values?

            case MOUSE_CLICK_LEFT:
            {
               uVirtualMouseState &= (~(1 << 24));
               break;
            } // end case MOUSE_CLICK_LEFT

            case MOUSE_CLICK_MIDDLE:
            {
               uVirtualMouseState &= (~(1 << 26));
               break;
            } // end case MOUSE_CLICK_MIDDLE

            case MOUSE_CLICK_RIGHT:
            {
               uVirtualMouseState &= (~(1 << 25));
               break;
            } // end case MOUSE_CLICK_RIGHT

            case MOUSE_WHEEL_UP:
            {
               // Nothing needs to be done for the current wheel implementation.
               break;
            } // end case MOUSE_WHEEL_UP

            case MOUSE_WHEEL_DOWN:
            {
               // Nothing needs to be done for the current wheel implementation.
               break;
            } // end case MOUSE_WHEEL_DOWN

            default:
            {
               // This should never happen.
               LogMessage("Unknown mouse action %d.\n", pActionBinding->uActionCode);
               nRetValue = FAILURE;
               break;
            }
         } // end switch(pActionBinding->uActionCode)

         // Release the mouse state mutex if we obtained it.
         if(WAIT_OBJECT_0 == mouseStateWaitResult)
         {
            if(0 == ReleaseMutex(hMouseStateMutex))
            {
               LogMessage("Mouse state mutex release failed:  %lu\n", GetLastError());
               nRetValue = FAILURE;
            }
         }
      } // end else if(BINDING_SIMULATED_MOUSE_ACTION == pActionBinding->bindingType)
      else if(BINDING_SPECIAL == pActionBinding->bindingType)
      {
         switch(pActionBinding->uActionCode)
         {
            case SPECIAL_ACTION_LEFT_CTRL:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_LEFT_CONTROL);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_LEFT_CTRL_LATCH:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_LEFT_CONTROL);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_LEFT_SHIFT:
            {
               pChatpadStateData->currentModifiers &= (~CHATPAD_MODIFIER_SHIFT);
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_LEFT_SHIFT);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_LEFT_SHIFT_LATCH:
            {
               pChatpadStateData->currentModifiers &= (~CHATPAD_MODIFIER_SHIFT);
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_LEFT_SHIFT);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_LEFT_ALT:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_LEFT_ALT);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_LEFT_ALT_LATCH:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_LEFT_ALT);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_LEFT_GUI:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_LEFT_GUI);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_LEFT_GUI_LATCH:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_LEFT_GUI);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_CTRL:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_RIGHT_CONTROL);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_CTRL_LATCH:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_RIGHT_CONTROL);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_SHIFT:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_RIGHT_SHIFT);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_SHIFT_LATCH:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_RIGHT_SHIFT);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_ALT:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_RIGHT_ALT);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_ALT_LATCH:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_RIGHT_ALT);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_GUI:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_RIGHT_GUI);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_RIGHT_GUI_LATCH:
            {
               currentKeyboardModifiers &= (~KEYBOARD_MODIFIER_RIGHT_GUI);
               // Send an update message so that Windows knows that the key is no longer held down.
               keyboardData[0] = currentKeyboardModifiers;
               memcpy(&(keyboardData[1]), currentOutputKeysHeldDown, MAX_SIMULTANEOUS_KEYS);
               SendVirtualKeyboardData(keyboardData, MAX_SIMULTANEOUS_KEYS + 1);
               break;
            }

            case SPECIAL_ACTION_ORANGE:
            {
               pChatpadStateData->currentModifiers &= (~CHATPAD_MODIFIER_ORANGE);
               break;
            }

            case SPECIAL_ACTION_ORANGE_LATCH:
            {
               pChatpadStateData->currentModifiers &= (~CHATPAD_MODIFIER_ORANGE);
               break;
            }

            case SPECIAL_ACTION_GREEN:
            {
               pChatpadStateData->currentModifiers &= (~CHATPAD_MODIFIER_GREEN);
               break;
            }

            case SPECIAL_ACTION_GREEN_LATCH:
            {
               pChatpadStateData->currentModifiers &= (~CHATPAD_MODIFIER_GREEN);
               break;
            }

            case SPECIAL_ACTION_PEOPLE:
            {
               pChatpadStateData->currentModifiers &= (~CHATPAD_MODIFIER_PEOPLE);
               break;
            }

            case SPECIAL_ACTION_PEOPLE_LATCH:
            {
               pChatpadStateData->currentModifiers &= (~CHATPAD_MODIFIER_PEOPLE);
               break;
            }

            case SPECIAL_ACTION_TOGGLE_WINDOWS_MODE:
            {
               // Nothing needs to be done here.
               break;
            }

            default:
            {
               // This should never happen.
               LogMessage("Unknown special action %d.\n", pActionBinding->uActionCode);
               nRetValue = FAILURE;
               break;
            }
         } // end switch(pActionBinding->uActionCode)
      } // end else if(BINDING_SPECIAL == pActionBinding->bindingType)
      else
      {
         // This should never happen.
         LogMessage("Unknown binding type %d.\n", pActionBinding->bindingType);
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end StopBoundAction

// Function to handle when a controller button is pressed.
//
// Returns SUCCESS on success and FAILURE on failure.
//
// IMPORTANT NOTE:  This function must only be called if hStatesMutex
// is held by the calling thread!  This function does not attempt
// to synchronize on its own!
int HandleControlsButtonPress(
  HANDLE             hChatpadDevice,
  ControllerButton   eWhichButton,
  bool               bWindowsModeModifierActive)
{
   int                           nRetValue                     = SUCCESS;
   ChatpadDriverActionBinding*   pActionBinding                = NULL;
   int                           nSavedActionsSearchIndex      = 0;
   unsigned int                  uControllerModifiers          = CONTROLLER_MODIFIER_NONE;

   if((eWhichButton < CONTROLLER_FIRST_BUTTON_VALUE) ||
      (eWhichButton >= CONTROLLER_NUM_BUTTONS))
   {
      LogMessage("Invalid button code %d provided to HandleControlsButtonPress.\n", eWhichButton);
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      for(nSavedActionsSearchIndex = 0;
          nSavedActionsSearchIndex < MAX_SIMULTANEOUS_CONTROLLER_ACTIONS;
          nSavedActionsSearchIndex++)
      {
         if(NULL == controllerTriggeredActions[nSavedActionsSearchIndex])
         {
            break;
         }
      }
      
      if(MAX_SIMULTANEOUS_CONTROLLER_ACTIONS == nSavedActionsSearchIndex)
      {
         LogMessage("Error, too many controller actions are currently active.\n");
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   if(SUCCESS == nRetValue)
   {
      if(true == bWindowsModeModifierActive)
      {
         uControllerModifiers |= CONTROLLER_MODIFIER_WINDOWS_MODE;
      }

      // Try to find a matching action binding.
      for(int bindingIndex = 0;
          bindingIndex < pDriverConfigData->buttonMappings[eWhichButton].nNumBindings;
          bindingIndex++)
      {
         // If the number of bindings is greater than 0 here, pBindings should
         // never be NULL, so it is safe not to check it.
         pActionBinding = &(pDriverConfigData->buttonMappings[eWhichButton].pBindings[bindingIndex]);
         if(uControllerModifiers == pActionBinding->uSourceActionModifiers)
         {
            break;
         }
         else
         {
            // The binding did not match, so do not save the pointer.
            pActionBinding = NULL;
         }
      }

      // If an action binding was found, start the action.
      if(NULL != pActionBinding)
      {
         controllerTriggeredActions[nSavedActionsSearchIndex]           = pActionBinding;
         controllerTriggeredActionModifiers[nSavedActionsSearchIndex]   = uControllerModifiers;
         StartBoundAction(hChatpadDevice, pActionBinding);
      }
      else
      {
// TODO maybe add a no-action option to avoid warnings for unbound controller buttons?
/*
         LogMessage("No action binding found for combination %d, %d.\n",
                    eWhichButton,
                    (true == bWindowsModeModifierActive) ? 1 : 0);
*/
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end HandleControlsButtonPress

// Function to handle when a controller button is released.
//
// Returns SUCCESS on success and FAILURE on failure.
//
// IMPORTANT NOTE:  This function must only be called if hStatesMutex
// is held by the calling thread!  This function does not attempt
// to synchronize on its own!
int HandleControlsButtonRelease(
  HANDLE             hChatpadDevice,
  ControllerButton   eWhichButton,
  bool               bWindowsModeModifierActive)
{
   int                           nRetValue                     = SUCCESS;
   ChatpadDriverActionBinding*   pActionBinding                = NULL;
   int                           nSavedActionsSearchIndex      = 0;
   unsigned int                  uControllerModifiers          = CONTROLLER_MODIFIER_NONE;

   if((eWhichButton < CONTROLLER_FIRST_BUTTON_VALUE) ||
      (eWhichButton >= CONTROLLER_NUM_BUTTONS))
   {
      LogMessage("Invalid button code %d provided to HandleControlsButtonRelease.\n", eWhichButton);
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      if(true == bWindowsModeModifierActive)
      {
         uControllerModifiers |= CONTROLLER_MODIFIER_WINDOWS_MODE;
      }

      // Try to find a matching action binding.
      for(int bindingIndex = 0;
          bindingIndex < pDriverConfigData->buttonMappings[eWhichButton].nNumBindings;
          bindingIndex++)
      {
         // If the number of bindings is greater than 0 here, pBindings should
         // never be NULL, so it is safe not to check it.
         pActionBinding = &(pDriverConfigData->buttonMappings[eWhichButton].pBindings[bindingIndex]);
         if(pActionBinding->uSourceActionModifiers == uControllerModifiers)
         {
            break;
         }
         else
         {
            // The binding did not match, so do not save the pointer.
            pActionBinding = NULL;
         }
      }

      // If an action binding was found, stop the action.
      if(NULL != pActionBinding)
      {
         // Find a matching binding and deactivate it.
         for(nSavedActionsSearchIndex = 0;
             nSavedActionsSearchIndex < MAX_SIMULTANEOUS_CONTROLLER_ACTIONS;
             nSavedActionsSearchIndex++)
         {
            if((pActionBinding         == controllerTriggeredActions[nSavedActionsSearchIndex])          &&
               (uControllerModifiers   == controllerTriggeredActionModifiers[nSavedActionsSearchIndex]))
            {
               break;
            }
         }

         if(nSavedActionsSearchIndex < MAX_SIMULTANEOUS_CONTROLLER_ACTIONS)
         {
            StopBoundAction(hChatpadDevice, pActionBinding);
            controllerTriggeredActions[nSavedActionsSearchIndex]           = NULL;
            controllerTriggeredActionModifiers[nSavedActionsSearchIndex]   = CONTROLLER_MODIFIER_NONE;
         }
         else
         {
            LogMessage("Action binding does not seem to be active.\n");
            nRetValue = FAILURE;
         }
      }
      else
      {
// TODO maybe add a no-action option to avoid warnings for unbound controller buttons?
/*
         LogMessage("No action binding found for combination %d, %d.\n",
                    eWhichButton,
                    (true == bWindowsModeModifierActive) ? 1 : 0);
*/
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end HandleControlsButtonRelease

// Function to handle XBox 360 controller controls data.
//
// Returns SUCCESS on success and FAILURE on failure.
int HandleControlsData(
  HANDLE hChatpadDevice,
  PUCHAR readBuffer,
  DWORD  dataLength)
{
   int nRetValue     = SUCCESS;
   DWORD waitResult  = 0;

   // Get the states mutex for synchronization.
   // TODO will using the mutex here and in HandleChatpadData cause performance problems?
   waitResult = WaitForSingleObject(hStatesMutex, INFINITE);
   // Make sure the wait actually succeeded.
   if(WAIT_OBJECT_0 != waitResult)
   {
      LogMessage("Wait for mutex failed:  %u\n", waitResult);
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) &&
      ((INVALID_HANDLE_VALUE == hChatpadDevice)    ||
       (NULL == readBuffer)                        ||
       (0 == dataLength)))
   {
      LogMessage("Invalid parameters to HandleControlsData.\n");
      nRetValue = FAILURE;
   }

   // Currently, only normal 20-byte controls reports are supported.
   // For information on the format:
   // http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller/UsbInfo
   if(dataLength != 20)
   {
      LogMessage("Unsupported data length %lu passed to HandleControlsData.\n", dataLength);

      // I am keeping the below code around as an example, even if I comment it out,
      // since occasionally I may want to see the raw hex data for debugging
      // or development purposes.

      // This buffer should be more than long enough to hold the message and a representation
      // of 32 bytes of control data.  Technically, the buffer would only need to be around
      // (25 + (5 * 32) + 2) == 187 bytes long.
      size_t tempLength = 1024;
      char tempMessage[1024];

      // Start the string.
      sprintf_s(tempMessage, tempLength, "[controls] Length %u:  [ ", dataLength);
      for(DWORD i = 0; i < dataLength; i++)
      {
         // Append data bytes to the string.
         sprintf_s(tempMessage, tempLength, "%s0x%02x ", tempMessage, readBuffer[i]);
      }
      // End the string.
      sprintf_s(tempMessage, tempLength, "%s]", tempMessage);
      LogMessage("%s\n", tempMessage);

      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) &&
      (true == bWindowsModeActive))
   {
      int leftStickX    = 0;
      int leftStickY    = 0;
      int rightStickX   = 0;
      int rightStickY   = 0;

      // Read 16-bit signed, little-endian, values for the thumbsticks.
      leftStickX  = (((static_cast<int>(readBuffer[7]) << 24) >> 16) +
                    (static_cast<int>(readBuffer[6])));
      leftStickY  = (((static_cast<int>(readBuffer[9]) << 24) >> 16) +
                    (static_cast<int>(readBuffer[8])));
      rightStickX = (((static_cast<int>(readBuffer[11]) << 24) >> 16) +
                    (static_cast<int>(readBuffer[10])));
      rightStickY = (((static_cast<int>(readBuffer[13]) << 24) >> 16) +
                    (static_cast<int>(readBuffer[12])));
//      LogMessage("Left %d, %d     Right %d, %d\n", leftStickX, leftStickY, rightStickX, rightStickY);

      // Generate a mouse event.
      unsigned int uNewMouseState = 0;
      int nScaledXValue = 0;
      int nScaledYValue = 0;
      // Compute a value used to implement virtual mouse sensitivity.
      int nStickAdjustValue =
        (1000 * ((MAXIMUM_VIRTUAL_MOUSE_SENSITIVITY + 1) - pDriverConfigData->nVirtualMouseSensitivity));
      DWORD mouseStateWaitResult  = 0;

      nScaledXValue += (leftStickX / nStickAdjustValue);
      nScaledXValue += (rightStickX / nStickAdjustValue);
      // Make sure the sum does not overflow, and use an even positive and negative limit.
      if(nScaledXValue < (-127))
      {
         nScaledXValue = -127;
      }
      else if(nScaledXValue > 127)
      {
         nScaledXValue = 127;
      }

      // Note that the Y stick value is apparently inverted compared to the normal Windows desktop coordinate scheme.
      nScaledYValue += ((-leftStickY) / nStickAdjustValue);
      nScaledYValue += ((-rightStickY) / nStickAdjustValue);
      // Make sure the sum does not overflow, and use an even positive and negative limit.
      if(nScaledYValue < (-127))
      {
         nScaledYValue = -127;
      }
      else if(nScaledYValue > 127)
      {
         nScaledYValue = 127;
      }

/*
      LogMessage("nScaledXValue:  0x%08x     nScaledYValue:  0x%08x\n",
                 static_cast<unsigned int>(nScaledXValue),
                 static_cast<unsigned int>(nScaledYValue));
*/

      // Get the mouse state mutex for synchronization.
      mouseStateWaitResult = WaitForSingleObject(hMouseStateMutex, INFINITE);
      // Make sure the wait actually succeeded.
      if(WAIT_OBJECT_0 != mouseStateWaitResult)
      {
         LogMessage("Wait for mouse state mutex failed:  %u\n", mouseStateWaitResult);
         nRetValue = FAILURE;
      }

      if(SUCCESS == nRetValue)
      {
         // Update the global state variable by masking out the X and Y offset bytes and then updating them.
         uVirtualMouseState = (uVirtualMouseState & 0xFF0000FF);
         uVirtualMouseState += (static_cast<unsigned int>(nScaledXValue) & 0x000000FF) << 16;
         uVirtualMouseState += (static_cast<unsigned int>(nScaledYValue) & 0x000000FF) << 8;
//         LogMessage("0x%08x\n", uVirtualMouseState);
      } // end if(SUCCESS == nRetValue)

      // Release the mouse state mutex if we obtained it.
      if(WAIT_OBJECT_0 == mouseStateWaitResult)
      {
         if(0 == ReleaseMutex(hMouseStateMutex))
         {
            LogMessage("Mouse state mutex release failed:  %lu\n", GetLastError());
            nRetValue = FAILURE;
         }
      }
   } // end if successful and Windows Mode is active

   if(SUCCESS == nRetValue)
   {
      // For each of the following cases, a pointer to the action binding is
      // saved so that the action can be stopped if Windows Mode is
      // deactivated.
      //
      // Return values are currently ignored in case bindings are not found
      // causing errors to be returned, since we do not really care about that.
      //
      // Note that HandleControlsButtonPress() automatically finds an action
      // binding pointer slot in controllerTriggeredActions, saves a pointer
      // to it, and starts a bound action.
      // TODO eventually make tables of constants instead of hardcoding things here?

      if((0x01 == (0x01 & readBuffer[3]))                            &&
         (false == controllerButtonsPressed[CONTROLLER_BUTTON_LB]))
      {
         HandleControlsButtonPress(hChatpadDevice, CONTROLLER_BUTTON_LB, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_LB] = true;
      }
      else if((0x00 == (0x01 & readBuffer[3]))                            &&
              (true == controllerButtonsPressed[CONTROLLER_BUTTON_LB]))
      {
         HandleControlsButtonRelease(hChatpadDevice, CONTROLLER_BUTTON_LB, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_LB] = false;
      }

      if((0x02 == (0x02 & readBuffer[3]))                            &&
         (false == controllerButtonsPressed[CONTROLLER_BUTTON_RB]))
      {
         HandleControlsButtonPress(hChatpadDevice, CONTROLLER_BUTTON_RB, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_RB] = true;
      }
      else if((0x00 == (0x02 & readBuffer[3]))                            &&
              (true == controllerButtonsPressed[CONTROLLER_BUTTON_RB]))
      {
         HandleControlsButtonRelease(hChatpadDevice, CONTROLLER_BUTTON_RB, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_RB] = false;
      }

      if((0x20 == (0x20 & readBuffer[2]))                            &&
         (false == controllerButtonsPressed[CONTROLLER_BUTTON_BACK]))
      {
         HandleControlsButtonPress(hChatpadDevice, CONTROLLER_BUTTON_BACK, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_BACK] = true;
      }
      else if((0x00 == (0x20 & readBuffer[2]))                            &&
              (true == controllerButtonsPressed[CONTROLLER_BUTTON_BACK]))
      {
         HandleControlsButtonRelease(hChatpadDevice, CONTROLLER_BUTTON_BACK, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_BACK] = false;
      }

      if((0x04 == (0x04 & readBuffer[3]))                            &&
         (false == controllerButtonsPressed[CONTROLLER_BUTTON_GUIDE]))
      {
         HandleControlsButtonPress(hChatpadDevice, CONTROLLER_BUTTON_GUIDE, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_GUIDE] = true;
      }
      else if((0x00 == (0x04 & readBuffer[3]))                            &&
              (true == controllerButtonsPressed[CONTROLLER_BUTTON_GUIDE]))
      {
         HandleControlsButtonRelease(hChatpadDevice, CONTROLLER_BUTTON_GUIDE, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_GUIDE] = false;
      }

      if((0x10 == (0x10 & readBuffer[2]))                            &&
         (false == controllerButtonsPressed[CONTROLLER_BUTTON_START]))
      {
         HandleControlsButtonPress(hChatpadDevice, CONTROLLER_BUTTON_START, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_START] = true;
      }
      else if((0x00 == (0x10 & readBuffer[2]))                            &&
              (true == controllerButtonsPressed[CONTROLLER_BUTTON_START]))
      {
         HandleControlsButtonRelease(hChatpadDevice, CONTROLLER_BUTTON_START, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_START] = false;
      }

      if((0x40 == (0x40 & readBuffer[2]))                            &&
         (false == controllerButtonsPressed[CONTROLLER_BUTTON_LEFT_STICK_CLICK]))
      {
         HandleControlsButtonPress(hChatpadDevice, CONTROLLER_BUTTON_LEFT_STICK_CLICK, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_LEFT_STICK_CLICK] = true;
      }
      else if((0x00 == (0x40 & readBuffer[2]))                            &&
              (true == controllerButtonsPressed[CONTROLLER_BUTTON_LEFT_STICK_CLICK]))
      {
         HandleControlsButtonRelease(hChatpadDevice, CONTROLLER_BUTTON_LEFT_STICK_CLICK, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_LEFT_STICK_CLICK] = false;
      }

      if((0x80 == (0x80 & readBuffer[2]))                            &&
         (false == controllerButtonsPressed[CONTROLLER_BUTTON_RIGHT_STICK_CLICK]))
      {
         HandleControlsButtonPress(hChatpadDevice, CONTROLLER_BUTTON_RIGHT_STICK_CLICK, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_RIGHT_STICK_CLICK] = true;
      }
      else if((0x00 == (0x80 & readBuffer[2]))                            &&
              (true == controllerButtonsPressed[CONTROLLER_BUTTON_RIGHT_STICK_CLICK]))
      {
         HandleControlsButtonRelease(hChatpadDevice, CONTROLLER_BUTTON_RIGHT_STICK_CLICK, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_RIGHT_STICK_CLICK] = false;
      }

      if((0x10 == (0x10 & readBuffer[3]))                            &&
         (false == controllerButtonsPressed[CONTROLLER_BUTTON_A]))
      {
         HandleControlsButtonPress(hChatpadDevice, CONTROLLER_BUTTON_A, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_A] = true;
      }
      else if((0x00 == (0x10 & readBuffer[3]))                            &&
              (true == controllerButtonsPressed[CONTROLLER_BUTTON_A]))
      {
         HandleControlsButtonRelease(hChatpadDevice, CONTROLLER_BUTTON_A, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_A] = false;
      }

      if((0x20 == (0x20 & readBuffer[3]))                            &&
         (false == controllerButtonsPressed[CONTROLLER_BUTTON_B]))
      {
         HandleControlsButtonPress(hChatpadDevice, CONTROLLER_BUTTON_B, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_B] = true;
      }
      else if((0x00 == (0x20 & readBuffer[3]))                            &&
              (true == controllerButtonsPressed[CONTROLLER_BUTTON_B]))
      {
         HandleControlsButtonRelease(hChatpadDevice, CONTROLLER_BUTTON_B, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_B] = false;
      }

      if((0x40 == (0x40 & readBuffer[3]))                            &&
         (false == controllerButtonsPressed[CONTROLLER_BUTTON_X]))
      {
         HandleControlsButtonPress(hChatpadDevice, CONTROLLER_BUTTON_X, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_X] = true;
      }
      else if((0x00 == (0x40 & readBuffer[3]))                            &&
              (true == controllerButtonsPressed[CONTROLLER_BUTTON_X]))
      {
         HandleControlsButtonRelease(hChatpadDevice, CONTROLLER_BUTTON_X, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_X] = false;
      }

      if((0x80 == (0x80 & readBuffer[3]))                            &&
         (false == controllerButtonsPressed[CONTROLLER_BUTTON_Y]))
      {
         HandleControlsButtonPress(hChatpadDevice, CONTROLLER_BUTTON_Y, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_Y] = true;
      }
      else if((0x00 == (0x80 & readBuffer[3]))                            &&
              (true == controllerButtonsPressed[CONTROLLER_BUTTON_Y]))
      {
         HandleControlsButtonRelease(hChatpadDevice, CONTROLLER_BUTTON_Y, bWindowsModeActive);
         controllerButtonsPressed[CONTROLLER_BUTTON_Y] = false;
      }
   } // end if(SUCCESS == nRetValue)

   // Release the mutex if we obtained it.
   if(WAIT_OBJECT_0 == waitResult)
   {
      if(0 == ReleaseMutex(hStatesMutex))
      {
         LogMessage("States mutex release failed:  %lu\n", GetLastError());
         nRetValue = FAILURE;
      }
   }

   return nRetValue;
} // end HandleControlsData

// Worker read code to continuously read controller data.
DWORD WINAPI ControllerReadFunction(LPVOID lpParam)
{
   int         nStatus                                      = SUCCESS;
   HANDLE      hChatpadDevice                               = static_cast<HANDLE>(lpParam);
   BOOLEAN     workerThreadFinished                         = FALSE;

   HANDLE      waitHandles[NUM_PENDING_READS_TO_USE + 1];

   BOOL        readPending[NUM_PENDING_READS_TO_USE];
   DWORD       readBufferLength[NUM_PENDING_READS_TO_USE];

   OVERLAPPED  overlappedStructure[NUM_PENDING_READS_TO_USE];
   UCHAR       readBuffer[NUM_PENDING_READS_TO_USE][32];
   DWORD       bytesRead[NUM_PENDING_READS_TO_USE];

   LogMessage("Reading thread started for controls data.\n");

   if(INVALID_HANDLE_VALUE == hChatpadDevice)
   {
      LogMessage("Error, invalid device handle provided to ControllerReadFunction.\n");
      nStatus = FAILURE;
   }

   if(SUCCESS == nStatus)
   {
      // Set up wait handles and mark the reads as not pending.
      for(int controlsReadIndex = 0; controlsReadIndex < NUM_PENDING_READS_TO_USE; controlsReadIndex++)
      {
         waitHandles[controlsReadIndex] = hControlsReadEvents[controlsReadIndex];
         readPending[controlsReadIndex] = FALSE;
         readBufferLength[controlsReadIndex] = 32;
      }

      // Set up the wait handle for the worker thread shutdown event.
      waitHandles[NUM_PENDING_READS_TO_USE] = hWorkerThreadShutdownEvent;
   }

   // Main loop for this worker thread.
   while((SUCCESS == nStatus) &&
         (FALSE == workerThreadFinished))
   {
      // Send any read requests that are not already pending.
      for(int controlsReadIndex = 0;
          ((SUCCESS == nStatus) &&
           (controlsReadIndex < NUM_PENDING_READS_TO_USE));
          controlsReadIndex++)
      {
         if(FALSE == readPending[controlsReadIndex])
         {
            // Mark the read as pending so it will not be sent again until it finishes.
            readPending[controlsReadIndex] = TRUE;

            // Initialize the OVERLAPPED structure.
            memset(&(overlappedStructure[controlsReadIndex]), 0x00, sizeof(OVERLAPPED));
            overlappedStructure[controlsReadIndex].hEvent = hControlsReadEvents[controlsReadIndex];
            // Reset the event in case it is already signaled.  If it is not
            // currently signaled, this call should simply have no effect.
            ResetEvent(hControlsReadEvents[controlsReadIndex]);

            // Clear read buffer and the variable tracking the number of bytes read.
            memset(readBuffer[controlsReadIndex],
                   0x00,
                   readBufferLength[controlsReadIndex]);
            bytesRead[controlsReadIndex] = 0;

            // Call the IOCTL.
            BOOL operationComplete = DeviceIoControl(
              hChatpadDevice,
              IOCTL_CHATPAD_READ_FROM_CONTROLS_ENDPOINT,
              NULL,
              0,
              readBuffer[controlsReadIndex],
              readBufferLength[controlsReadIndex],
              &bytesRead[controlsReadIndex],
              &overlappedStructure[controlsReadIndex]);
            if(FALSE == operationComplete)
            {
               DWORD lastError = GetLastError();

               // At this point, lastError should be ERROR_IO_PENDING.
               if(ERROR_IO_PENDING != lastError)
               {
                  LogMessage("DeviceIoControl failed:  %lu\n", lastError);
                  nStatus = FAILURE;
               }
            } // end if(FALSE == operationComplete)
         } // end if(FALSE == readPending[controlsReadIndex])
      } // end looping to send read requests that need sent

      // Now that all read requests should be pending, wait for any of them to finish, or
      // for worker thread shutdown to be signaled.

      if(SUCCESS == nStatus)
      {
         DWORD ioctlWaitResult = 0;

//         LogMessage("Waiting on controls input...\n");
         ioctlWaitResult = WaitForMultipleObjects(
           (NUM_PENDING_READS_TO_USE + 1),
           waitHandles,
           FALSE,       // Stop waiting after any of the specified events finishes.
           INFINITE);   // No timeout.
         if((WAIT_OBJECT_0 + NUM_PENDING_READS_TO_USE) == ioctlWaitResult)
         {
            // The worker thread shutdown event was signaled.
            LogMessage("Thread shutdown detected while in ControllerReadFunction.\n");
            workerThreadFinished = TRUE;
            continue;
         }
         else
         {
            int numReadsFinished = 0;

            // Find which controls read(s) completed.
            for(int controlsReadIndex = 0;
                ((SUCCESS == nStatus) &&
                 (controlsReadIndex < NUM_PENDING_READS_TO_USE));
                controlsReadIndex++)
            {
               if((WAIT_OBJECT_0 + controlsReadIndex) == ioctlWaitResult)
               {
                  numReadsFinished++;

                  // Check if the IOCTL actually completed successfully.
                  BOOL operationComplete = GetOverlappedResult(
                    hChatpadDevice,
                    &overlappedStructure[controlsReadIndex],
                    &(bytesRead[controlsReadIndex]),
                    FALSE);
                  if(TRUE == operationComplete)
                  {
                     // Process the controls data.
                     // TODO add some sort of state parameter, and pass a pointer here?  Probably
                     //   a struct.
                     int nControlsReturnValue = HandleControlsData(
                       hChatpadDevice, readBuffer[controlsReadIndex], bytesRead[controlsReadIndex]);
                     // I am anticipating that I may have some unexpected data-processing failures,
                     // so I am just printing an error if one occurs, but not stopping overall processing.
                     if(SUCCESS != nControlsReturnValue)
                     {
                        LogMessage("Error, HandleControlsData returned FAILURE.\n");
                     }

                     // Mark the read as no longer pending.
                     readPending[controlsReadIndex] = FALSE;
                  }
                  else
                  {
                     LogMessage("Controls data read completed but a failure occurred:  %lu\n", GetLastError());
                     nStatus = FAILURE;
                  }
               } // end if((WAIT_OBJECT_0 + controlsReadIndex) == ioctlWaitResult)
            } // end looping to find which controls read(s) completed

            // If no controls reads seem to have finished, then an error presumably occurred.
            if(0 == numReadsFinished)
            {
               LogMessage("WaitForMultipleObjects failed with return value %u\n", ioctlWaitResult);
               LogMessage("Read from controls endpoint failed:  %lu\n", GetLastError());
               nStatus = FAILURE;
            }
         } // end handling if the worker thread shutdown event was not signaled
      } // end if(SUCCESS == nStatus)
   } // end while the worker thread is successful and has not been shutdown

   // Cancel any I/O this worker thread has pending.
   CancelIo(hChatpadDevice);

   // Exit this worker thread.
   LogMessage("Reading thread for controls data has ended, with nStatus == %d\n", nStatus);
   return 0;
} // end ControllerReadFunction

// Function to handle chatpad data.
int HandleChatpadData(
  HANDLE hChatpadDevice,
  PUCHAR readBuffer,
  DWORD  dataLength)
{
   int   nRetValue   = SUCCESS;
   DWORD waitResult  = 0;

   // Get the states mutex for synchronization.
   waitResult = WaitForSingleObject(hStatesMutex, INFINITE);
   // Make sure the wait actually succeeded.
   if(WAIT_OBJECT_0 != waitResult)
   {
      LogMessage("Wait for mutex failed:  %u\n", waitResult);
      nRetValue = FAILURE;
   }

   // See http://www.mp3car.com/vbulletin/input-devices/108554-xbox360-chatpad-awsome-backlit-mini-keyboard-16.html#post1255117
   // for information on chatpad codes.

   // Note that for each of the below commands, a Sleep(12) afterwards seems like a good idea since otherwise
   // commands sent too close together (closer than 12 milliseconds, or however long Windows waits?) seem
   // not to work.  I have included this delay as part of the SendControlRequest function so it will happen
   // automatically.
   //
   // TODO are there shortcut commands to turn off more than one light at once?
   //
   // Note, sending (0x41, 0x00, 0x0008, 0x0002, 0x0000) makes the chatpad keyboard caps lock light turn on.
   // Note, sending (0x41, 0x00, 0x0000, 0x0002, 0x0000) makes the chatpad keyboard caps lock light turn off.
   // Note, sending (0x41, 0x00, 0x0009, 0x0002, 0x0000) makes the chatpad keyboard green square light turn on.
   // Note, sending (0x41, 0x00, 0x0001, 0x0002, 0x0000) makes the chatpad keyboard green square light turn off.
   // Note, sending (0x41, 0x00, 0x000a, 0x0002, 0x0000) makes the chatpad keyboard orange circle light turn on.
   // Note, sending (0x41, 0x00, 0x0002, 0x0002, 0x0000) makes the chatpad keyboard orange circle light turn off.
   // Note, sending (0x41, 0x00, 0x000b, 0x0002, 0x0000) makes the chatpad keyboard people light turn on.
   // Note, sending (0x41, 0x00, 0x0003, 0x0002, 0x0000) makes the chatpad keyboard people light turn off.
   // 
   // Note, sending (0x41, 0x00, 0x0004, 0x0002, 0x0000) seems to make the chatpad backlight instantly turn off.
   // TODO is there a way to completely disable the backlight but have chatpad keys still send data?

/*
   if(SUCCESS == nRetValue)
   {
      // These messages should always be length 5 so we are just printing that much data.
      // The buffer is currently always length 32, so we will not overrun that with this code.
      // I am not printing the repeated 0xf0 messages, since I do not seem to need them
      // and I do not know what they do.
      if(0xf0 != readBuffer[0])
      {
         LogMessage(
           "[chatpad] Length %u:  [ 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ]\n",
           dataLength,
           readBuffer[0],
           readBuffer[1],
           readBuffer[2],
           readBuffer[3],
           readBuffer[4]);
      } // end if(0xf0 != readBuffer[0])
   } // end if(SUCCESS == nRetValue)
*/

   if((SUCCESS == nRetValue) &&
      (0xf0 != readBuffer[0]))
   {
      // Note that if a modifier is held down, then only one normal key can be
      // held down.  If two modifiers are held down, then no normal key can be
      // held down.  No more than any two key are registered at a time by the
      // chatpad, apparently.
      // This array is used to store CHATPAD_KEY_ constant values for any
      // chatpad keys that are held down according to this message.
      unsigned char newKeysDown[2];
      ChatpadDriverActionBinding* actionBinding = NULL;
      unsigned char previousKeyCode = 0;
      unsigned int previousKeyModifiers = 0;
      unsigned int chatpadModifierAfterLatches = 0;
      int searchIndex = 0;

      newKeysDown[0] = CHATPAD_KEY_NONE;
      newKeysDown[1] = CHATPAD_KEY_NONE;

      // Check whether any special keys are held down.
      for(searchIndex = 0; searchIndex < 4; searchIndex++)
      {
         if(chatpadModifierMaskTable[searchIndex][0] == (readBuffer[1] & chatpadModifierMaskTable[searchIndex][0]))
         {
            if(CHATPAD_KEY_NONE == newKeysDown[0])
            {
               newKeysDown[0] = chatpadModifierMaskTable[searchIndex][1];
            }
            else if(CHATPAD_KEY_NONE == newKeysDown[1])
            {
               newKeysDown[1] = chatpadModifierMaskTable[searchIndex][1];
            }
            else
            {
               // This should never happen.
               LogMessage("Error, no slot is available for a pressed chatpad key.\n");
            }
         } // end if modifier mask matches
      } // end for(int searchIndex = 0; searchIndex < 4; searchIndex++)

      // Check whether any normal keys are held down.
      for(searchIndex = 0; searchIndex < 2; searchIndex++)
      {
         if(0x00 != readBuffer[2 + searchIndex])
         {
            if(CHATPAD_KEY_NONE == newKeysDown[0])
            {
               newKeysDown[0] = chatpadScanCodeTable[readBuffer[2 + searchIndex]];
            }
            else if(CHATPAD_KEY_NONE == newKeysDown[1])
            {
               newKeysDown[1] = chatpadScanCodeTable[readBuffer[2 + searchIndex]];
            }
            else
            {
               // This should never happen.
               LogMessage("Error, no slot is available for a pressed chatpad key.\n");
            }
         } // end if(0x00 != readBuffer[2 + searchIndex])
      } // end for(int searchIndex = 0; searchIndex < 2; searchIndex++)

      // Ignore this data if it is repeated and the boolean to ignore repeated data is true.
      if((readBuffer[0] == pChatpadStateData->previousChatpadRawData[0])   &&
         (readBuffer[1] == pChatpadStateData->previousChatpadRawData[1])   &&
         (readBuffer[2] == pChatpadStateData->previousChatpadRawData[2])   &&
         (readBuffer[3] == pChatpadStateData->previousChatpadRawData[3])   &&
         (readBuffer[4] == pChatpadStateData->previousChatpadRawData[4])   &&
         (true == pChatpadStateData->ignoreNextRepeatedData))
      {
//         LogMessage("Ignoring repeated data.\n");
         pChatpadStateData->ignoreNextRepeatedData = false;
      }
      else
      {
         pChatpadStateData->ignoreNextRepeatedData = true;

         // If any chatpad keys were held down the last time this code executed,
         // but are not now, then generate stop actions if a binding exists.
         for(searchIndex = 0; searchIndex < 2; searchIndex++)
         {
            previousKeyCode      = pChatpadStateData->currentKeysDown[searchIndex];
            previousKeyModifiers = pChatpadStateData->currentKeysDownModifiers[searchIndex];

            if(CHATPAD_KEY_NONE != previousKeyCode)
            {
               // Check whether chatpad key has been released, trying to handle repeated
               // codes properly.
               //
               // === Cases where an existing event has stopped: ===
               //
               // * The leftmost chatpad data slot had a nonzero value last message, and
               //     the same value is no longer there.
               // * The leftmost chatpad data slot has the same nonzero value as last message, and
               //     the rightmost data slot has a zero value, and
               //     the rightmost data slot had a zero value in the last message.
               // * The rightmost chatpad data slot had a nonzero value last message, and
               //     the same value is not now in the leftmost chatpad data slot.

               if(((0 == searchIndex) &&
                   ((previousKeyCode != newKeysDown[0])        ||
                    ((CHATPAD_KEY_NONE == newKeysDown[1]) &&
                     (CHATPAD_KEY_NONE == pChatpadStateData->currentKeysDown[1]))))    ||
                  ((1 == searchIndex) &&
                   (previousKeyCode != newKeysDown[0])))
               {
                  // Find the binding.
                  // TODO a pointer could be saved to this binding for a slight
                  //   increase in efficiency if desired.
                  actionBinding = NULL;

                  for(int bindingIndex = 0;
                      bindingIndex < pDriverConfigData->keyMappings[previousKeyCode].nNumBindings;
                      bindingIndex++)
                  {
                     // If the number of bindings is greater than 0 here, pBindings should never be NULL,
                     // so it is safe not to check it.
                     actionBinding = &(pDriverConfigData->keyMappings[previousKeyCode].pBindings[bindingIndex]);
                     if(actionBinding->uSourceActionModifiers == previousKeyModifiers)
                     {
                        break;
                     }
                     else
                     {
                        // The binding did not match, so do not save the pointer.
                        actionBinding = NULL;
                     }
                  }

                  if(NULL != actionBinding)
                  {
                     StopBoundAction(hChatpadDevice, actionBinding);
                  }
                  else
                  {
                     LogMessage("Error, no action binding found for 0x%x + 0x%x.\n",
                                previousKeyCode,
                                previousKeyModifiers);
                  }

                  // Reset the variables since the key is no longer down.
                  pChatpadStateData->currentKeysDown[searchIndex]          = CHATPAD_KEY_NONE;
                  pChatpadStateData->currentKeysDownModifiers[searchIndex] = CHATPAD_MODIFIER_NONE;
               } // end if the key is no longer held down
            } // end if(CHATPAD_KEY_NONE != previousKeyCode)
         } // end for(int searchIndex = 0; searchIndex < 2; searchIndex++)

         chatpadModifierAfterLatches = pChatpadStateData->currentModifiers;

         // If any chatpad keys are held down now, but were not held down the last
         // time this code executed, then send start actions if a binding exists.
         for(searchIndex = 0; searchIndex < 2; searchIndex++)
         {
            if(CHATPAD_KEY_NONE != newKeysDown[searchIndex])
            {
               // Check whether a new chatpad key press event has occurred, trying
               // to handle repeated codes properly.
               //
               // Note that at this point, pChatpadStateData->currentKeysDown may contain
               // CHATPAD_KEY_NONE (zero) in either because no key had been pressed down
               // for that slot when this message was received, or else because a key
               // had been pressed down for that slot but was released due to the message
               // contents.
               //
               // === Cases where a new event has happened: ===
               //
               // * The leftmost chatpad data slot does not have a key marked as already held down, and
               //     a nonzero value is there now, and
               //     the new nonzero value does not match a key marked as already held down for
               //       the rightmost data slot.
               // * A nonzero value is in the rightmost data slot.
               if(((0 == searchIndex) &&
                   ((CHATPAD_KEY_NONE == pChatpadStateData->currentKeysDown[0]) &&
                    (newKeysDown[0] != pChatpadStateData->currentKeysDown[1])))                    ||
                  (1 == searchIndex))
               {
                  actionBinding = NULL;

                  // If this is not one of the special latch keys, apply and release any latched keyboard modifiers.
                  // The green, people, and orange latch modifiers may all be active at once.  The shift key is
                  // affected by modifiers so that caps lock can be triggered properly.
                  if((CHATPAD_KEY_GREEN != newKeysDown[searchIndex])    &&
                     (CHATPAD_KEY_PEOPLE != newKeysDown[searchIndex])   &&
                     (CHATPAD_KEY_ORANGE != newKeysDown[searchIndex]))
                  {
                     // Notice that more than one of these modifiers may be active at a time,
                     // so no "else if" blocks are used.
                     if(true == pChatpadStateData->leftShiftLatchActive)
                     {
                        chatpadModifierAfterLatches |= CHATPAD_MODIFIER_SHIFT;
                        pChatpadStateData->leftShiftLatchActive = false;
                        // Turn the shift light off unless caps lock is configured to control that light.
                        if(false == pDriverConfigData->bCapsLockControlsShiftIndicator)
                        {
                           SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0000, 0x0002, 0x0000);
                        }
                     }

                     if(true == pChatpadStateData->orangeLatchActive)
                     {
                        chatpadModifierAfterLatches |= CHATPAD_MODIFIER_ORANGE;
                        pChatpadStateData->orangeLatchActive = false;
                        // Turn the orange light off.
                        SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0002, 0x0002, 0x0000);
                     }

                     if(true == pChatpadStateData->peopleLatchActive)
                     {
                        chatpadModifierAfterLatches |= CHATPAD_MODIFIER_PEOPLE;
                        pChatpadStateData->peopleLatchActive = false;
                        // Turn the people light off.
                        SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0003, 0x0002, 0x0000);
                     }

                     if(true == pChatpadStateData->greenLatchActive)
                     {
                        chatpadModifierAfterLatches |= CHATPAD_MODIFIER_GREEN;
                        pChatpadStateData->greenLatchActive = false;
                        // Turn the green light off.
                        SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0001, 0x0002, 0x0000);
                     }
                  } // end if the key pressed was not a special latch key (green, people, or orange)

                  // Find the binding.
                  for(int bindingIndex = 0;
                      bindingIndex < pDriverConfigData->keyMappings[newKeysDown[searchIndex]].nNumBindings;
                      bindingIndex++)
                  {
                     // If the number of bindings is greater than 0 here, pBindings should never be NULL,
                     // so it is safe not to check it.
                     actionBinding = &(pDriverConfigData->keyMappings[newKeysDown[searchIndex]].pBindings[bindingIndex]);
                     if(actionBinding->uSourceActionModifiers == chatpadModifierAfterLatches)
                     {
                        break;
                     }
                     else
                     {
                        // The binding did not match, so do not save the pointer.
                        actionBinding = NULL;
                     }
                  }

                  if(NULL != actionBinding)
                  {
                     // Save the variables to indicate that the key is currently down.
                     if(CHATPAD_KEY_NONE == pChatpadStateData->currentKeysDown[0])
                     {
                        pChatpadStateData->currentKeysDown[0]           = newKeysDown[searchIndex];
                        pChatpadStateData->currentKeysDownModifiers[0]  = chatpadModifierAfterLatches;
                     }
                     else if(CHATPAD_KEY_NONE == pChatpadStateData->currentKeysDown[1])
                     {
                        pChatpadStateData->currentKeysDown[1]           = newKeysDown[searchIndex];
                        pChatpadStateData->currentKeysDownModifiers[1]  = chatpadModifierAfterLatches;
                     }
                     else
                     {
                        // This should never happen.
                        LogMessage("Error, no state save slot available for a pressed chatpad key.");
                     }

                     // Start the action.
                     StartBoundAction(hChatpadDevice, actionBinding);
                  } // end if(NULL != actionBinding)
                  else
                  {
                     // TODO comment this message out eventually.  It is not necessarily an error,
                     //   and could just indicate keys that the user has not bound.
                     LogMessage("No action binding found for 0x%x + 0x%x.\n",
                                newKeysDown[searchIndex],
                                chatpadModifierAfterLatches);
                  }
               } // end if a new chatpad key press event occurred
            } // end if(CHATPAD_KEY_NONE != newKeysDown[searchIndex])
         } // end for(int searchIndex = 0; searchIndex < 2; searchIndex++)
      } // end if not ignoring repeated data
   } // end if successful and the first readBuffer byte is not 0xf0

   // Save the raw data from this message if it did not start with 0xf0.
   if(0xf0 != readBuffer[0])
   {
      for(int i = 0; i < 5; i++)
      {
         pChatpadStateData->previousChatpadRawData[i] = readBuffer[i];
      }
   }

   // Release the mutex if we obtained it.
   if(WAIT_OBJECT_0 == waitResult)
   {
      if(0 == ReleaseMutex(hStatesMutex))
      {
         LogMessage("States mutex release failed:  %lu\n", GetLastError());
         nRetValue = FAILURE;
      }
   }

   return nRetValue;
} // end HandleChatpadData

// Worker read code to continuously read chatpad data.
DWORD WINAPI ChatpadReadFunction(LPVOID lpParam)
{
   int         nStatus                                      = SUCCESS;
   HANDLE      hChatpadDevice                               = static_cast<HANDLE>(lpParam);
   BOOLEAN     workerThreadFinished                         = FALSE;

   HANDLE      waitHandles[NUM_PENDING_READS_TO_USE + 1];

   BOOL        readPending[NUM_PENDING_READS_TO_USE];
   DWORD       readBufferLength[NUM_PENDING_READS_TO_USE];

   OVERLAPPED  overlappedStructure[NUM_PENDING_READS_TO_USE];
   UCHAR       readBuffer[NUM_PENDING_READS_TO_USE][32];
   DWORD       bytesRead[NUM_PENDING_READS_TO_USE];

   LogMessage("Reading thread started for chatpad data.\n");

   if(INVALID_HANDLE_VALUE == hChatpadDevice)
   {
      LogMessage("Error, invalid device handle provided to ChatpadReadFunction.\n");
      nStatus = FAILURE;
   }

   if(SUCCESS == nStatus)
   {
      // Set up wait handles and mark the reads as not pending.
      for(int chatpadReadIndex = 0; chatpadReadIndex < NUM_PENDING_READS_TO_USE; chatpadReadIndex++)
      {
         waitHandles[chatpadReadIndex] = hChatpadReadEvents[chatpadReadIndex];
         readPending[chatpadReadIndex] = FALSE;
         readBufferLength[chatpadReadIndex] = 32;
      }

      // Set up the wait handle for the worker thread shutdown event.
      waitHandles[NUM_PENDING_READS_TO_USE] = hWorkerThreadShutdownEvent;
   }

   // Main loop for this worker thread.
   while((SUCCESS == nStatus) &&
         (FALSE == workerThreadFinished))
   {
      // Send any read requests that are not already pending.
      for(int chatpadReadIndex = 0;
          ((SUCCESS == nStatus) &&
           (chatpadReadIndex < NUM_PENDING_READS_TO_USE));
          chatpadReadIndex++)
      {
         if(FALSE == readPending[chatpadReadIndex])
         {
            // Mark the read as pending so it will not be sent again until it finishes.
            readPending[chatpadReadIndex] = TRUE;

            // Initialize the OVERLAPPED structure.
            memset(&(overlappedStructure[chatpadReadIndex]), 0x00, sizeof(OVERLAPPED));
            overlappedStructure[chatpadReadIndex].hEvent = hChatpadReadEvents[chatpadReadIndex];
            // Reset the event in case it is already signaled.  If it is not
            // currently signaled, this call should simply have no effect.
            ResetEvent(hChatpadReadEvents[chatpadReadIndex]);

            // Clear read buffer and the variable tracking the number of bytes read.
            memset(readBuffer[chatpadReadIndex],
                   0x00,
                   readBufferLength[chatpadReadIndex]);
            bytesRead[chatpadReadIndex] = 0;

            // Call the IOCTL.
            BOOL operationComplete = DeviceIoControl(
              hChatpadDevice,
              IOCTL_CHATPAD_READ_FROM_CHATPAD_ENDPOINT,
              NULL,
              0,
              readBuffer[chatpadReadIndex],
              readBufferLength[chatpadReadIndex],
              &bytesRead[chatpadReadIndex],
              &overlappedStructure[chatpadReadIndex]);
            if(FALSE == operationComplete)
            {
               DWORD lastError = GetLastError();

               // At this point, lastError should be ERROR_IO_PENDING.
               if(ERROR_IO_PENDING != lastError)
               {
                  LogMessage("DeviceIoControl failed:  %lu\n", lastError);
                  nStatus = FAILURE;
               }
            } // end if(FALSE == operationComplete)
         } // end if(FALSE == readPending[chatpadReadIndex])
      } // end looping to send read requests that need sent

      // Now that all read requests should be pending, wait for any of them to finish, or
      // for worker thread shutdown to be signaled.

      if(SUCCESS == nStatus)
      {
         DWORD ioctlWaitResult = 0;

//         LogMessage("Waiting on chatpad input...\n");
         ioctlWaitResult = WaitForMultipleObjects(
           (NUM_PENDING_READS_TO_USE + 1),
           waitHandles,
           FALSE,       // Stop waiting after any of the specified events finishes.
           INFINITE);   // No timeout.
         if((WAIT_OBJECT_0 + NUM_PENDING_READS_TO_USE) == ioctlWaitResult)
         {
            // The worker thread shutdown event was signaled.
            LogMessage("Thread shutdown detected while in ChatpadReadFunction.\n");
            workerThreadFinished = TRUE;
            continue;
         }
         else
         {
            int numReadsFinished = 0;

            // Find which chatpad read(s) completed.
            for(int chatpadReadIndex = 0;
                ((SUCCESS == nStatus) &&
                 (chatpadReadIndex < NUM_PENDING_READS_TO_USE));
                chatpadReadIndex++)
            {
               if((WAIT_OBJECT_0 + chatpadReadIndex) == ioctlWaitResult)
               {
                  numReadsFinished++;

                  // Check if the IOCTL actually completed successfully.
                  BOOL operationComplete = GetOverlappedResult(
                    hChatpadDevice,
                    &overlappedStructure[chatpadReadIndex],
                    &(bytesRead[chatpadReadIndex]),
                    FALSE);
                  if(TRUE == operationComplete)
                  {
                     // Enable the chatpad keyboard backlight, which will activate when keys are pressed.
                     // This command seems to be required for chatpad key data to be received at all,
                     // so there may not even be a way for the backlight to be disabled but keys to
                     // still operate.
                     // Only sending this command once seems to work, but it needs to be after
                     // at least one chatpad data message has been received, and so it
                     // is done here.
                     // 
                     // No mutex protection is needed for backlightCommandSent since this is the only
                     // thread in which it will ever be read or set.
                     if(false == pChatpadStateData->backlightCommandSent)
                     {
                        pChatpadStateData->backlightCommandSent = true;
                        // TODO research at some point to see if the backlight can be disabled?
                        SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x001b, 0x0002, 0x0000);
                     }

/*
if(0xf0 != readBuffer[chatpadReadIndex][0])
{
   LogMessage(
     "[chatpad] Length %u:  [ 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ]\n",
     bytesRead[chatpadReadIndex],
     readBuffer[chatpadReadIndex][0],
     readBuffer[chatpadReadIndex][1],
     readBuffer[chatpadReadIndex][2],
     readBuffer[chatpadReadIndex][3],
     readBuffer[chatpadReadIndex][4]);
}
*/
                     // Process the chatpad data.
                     // TODO add some sort of state parameter, and pass a pointer here?  Probably
                     //   a struct.
                     int nChatpadReturnValue = HandleChatpadData(
                       hChatpadDevice, readBuffer[chatpadReadIndex], bytesRead[chatpadReadIndex]);
                     // I am anticipating that I may have some unexpected data-processing failures,
                     // so I am just printing an error if one occurs, but not stopping overall processing.
                     if(SUCCESS != nChatpadReturnValue)
                     {
                        LogMessage("Error, HandleChatpadData returned FAILURE.\n");
                     }

                     // Mark the read as no longer pending.
                     readPending[chatpadReadIndex] = FALSE;
                  }
                  else
                  {
                     LogMessage("Chatpad data read completed but a failure occurred:  %lu\n", GetLastError());
                     nStatus = FAILURE;
                  }
               } // end if((WAIT_OBJECT_0 + chatpadReadIndex) == ioctlWaitResult)
            } // end looping to find which chatpad read(s) completed

            // If no chatpad reads seem to have finished, then an error presumably occurred.
            if(0 == numReadsFinished)
            {
               LogMessage("WaitForMultipleObjects failed with return value %u\n", ioctlWaitResult);
               LogMessage("Read from chatpad endpoint failed:  %lu\n", GetLastError());
               nStatus = FAILURE;
            }
         } // end handling if the worker thread shutdown event was not signaled
      } // end if(SUCCESS == nStatus)
   } // end while the worker thread is successful and has not been shutdown

   // Cancel any I/O this worker thread has pending.
   CancelIo(hChatpadDevice);

   // Exit this worker thread.
   LogMessage("Reading thread for chatpad data has ended, with nStatus == %d\n", nStatus);
   return 0;
} // end ChatpadReadFunction

// Worker thread to send simulated mouse events.
DWORD WINAPI MouseWorkerThreadFunction(LPVOID lpParam)
{
   int            nStatus                                   = SUCCESS;
   HANDLE         hChatpadDevice                            = static_cast<HANDLE>(lpParam);
   BOOLEAN        workerThreadFinished                      = FALSE;
   // TODO make this 4 a predefined constant somewhere instead of hardcoding it
   UCHAR          mouseDataBuffer[4]                        = { 0x00, 0x00, 0x00, 0x00 };
   DWORD          mouseDataBufferLength                     = 4;
   // This variable is used to make a copy of the uVirtualMouseState variable since
   // no mutex is currently used to read it.
   unsigned int   uVirtualMouseStateCopy                    = 0;
   // This variable is used to avoid sending the same event over and over unnecessarily.
   unsigned int   uLastVirtualMouseState                    = 0;
   // Whether the mouse is currently active (according to the most recently sent event),
   // i.e. whether buttons are held down or it is being moved.
   BOOLEAN        mouseActive                               = FALSE;

   LogMessage("Worker thread started for mouse data.\n");

   // TODO get rid of this handle if we do not need it?
   if(INVALID_HANDLE_VALUE == hChatpadDevice)
   {
      LogMessage("Error, invalid device handle provided to MouseWorkerThreadFunction.\n");
      nStatus = FAILURE;
   }

   // Send a no-action mouse event to begin with.  This will allow uLastVirtualMouseState to
   // be correct.
   nStatus = SendVirtualMouseData(mouseDataBuffer, mouseDataBufferLength);
   mouseActive = FALSE;

   // Main loop for this worker thread.
   while((SUCCESS == nStatus) &&
         (FALSE == workerThreadFinished))
   {
      // TODO should I use a mutex here?  I am currently not using one with the
      //   thought that using one might slightly hurt performance, but it might
      //   not make much of a difference.

      // Hopefully this is an atomic operation.
      uVirtualMouseStateCopy = uVirtualMouseState;

      // Do not perform any processing if Windows Mode is not active.
      //
      // If the above check is met, send a virtual mouse event if the
      // mouse is active, or send an inactive event only once if the mouse
      // controls have gone into the deadzones.
      if((true == bWindowsModeActive)                          &&
         ((0 != uVirtualMouseStateCopy)   ||
          (TRUE == mouseActive)))
      {
         mouseDataBuffer[0] = (uVirtualMouseStateCopy & 0xFF000000) >> 24;
         mouseDataBuffer[1] = (uVirtualMouseStateCopy & 0x00FF0000) >> 16;
         mouseDataBuffer[2] = (uVirtualMouseStateCopy & 0x0000FF00) >> 8;
         mouseDataBuffer[3] = (uVirtualMouseStateCopy & 0x000000FF);
         nStatus = SendVirtualMouseData(mouseDataBuffer, mouseDataBufferLength);
         // TODO NEXT fix this if configurable deadzones are implemented
         if(0 == uVirtualMouseStateCopy)
         {
            mouseActive = FALSE;
         }
         else
         {
            mouseActive = TRUE;
         }
         uLastVirtualMouseState = uVirtualMouseStateCopy;
      }
      // TODO try measuring how long a cycle takes if a mutex gets involved
      // TODO do I need to worry about different machines reacting too differently to this
      //   sleep timing?  There are presumably other ways to time things, maybe some sort
      //   of periodic timer, but I don't want to go through the trouble to use one if
      //   I don't need it.
      Sleep(4);

      if(true == bShutdownMouseWorkerThread)
      {
         workerThreadFinished = TRUE;
      }
   } // end while the worker thread is successful and has not been shutdown

   // Cancel any I/O this worker thread has pending.
   CancelIo(hChatpadDevice);

   // Exit this worker thread.
   LogMessage("Reading thread for chatpad data has ended, with nStatus == %d\n", nStatus);
   return 0;
} // end ChatpadReadFunction

// This function launchers worker threads to read chatpad and controller data.
void LaunchWorkerThreads(
  HANDLE hChatpadDevice)
{
   // Based on example from http://msdn.microsoft.com/en-us/library/ms682516(VS.85).aspx.
   HANDLE dataArray[NUM_WORKER_THREADS];
   DWORD threadIDArray[NUM_WORKER_THREADS];

   dataArray[0] = hChatpadDevice;
   dataArray[1] = hChatpadDevice;
   dataArray[2] = hChatpadDevice;

   hWorkerThreads[0] = CreateThread(
     NULL,
     0,
     ControllerReadFunction,
     dataArray[0],
     0,
     &(threadIDArray[0]));
   hWorkerThreads[1] = CreateThread(
     NULL,
     0,
     ChatpadReadFunction,
     dataArray[1],
     0,
     &(threadIDArray[1]));
   hWorkerThreads[2] = CreateThread(
     NULL,
     0,
     MouseWorkerThreadFunction,
     dataArray[2],
     0,
     &(threadIDArray[2]));

   // Make sure the threads were successfully created.
   for(int workerIndex = 0; workerIndex < NUM_WORKER_THREADS; workerIndex++)
   {
      if(NULL == hWorkerThreads[workerIndex])
      {
         LogMessage("Fatal error creating worker thread with index %d.\n", workerIndex);
         exit(-1);
      }
   }
} // end LaunchWorkerThreads

// Initializes the chatpad.
int InitChatpad(
  HANDLE hChatpadDevice)
{
   int         nRetValue            = SUCCESS;
   DWORD       waitResult           = 0;
   DWORD       bytesTransferred     = 0;
   OVERLAPPED  overlappedStructure;

   // Get the control request mutex to ensure that only one control request is ongoing at a time,
   // and that two threads do not try to use the event at the same time.
   waitResult = WaitForSingleObject(hControlRequestMutex, INFINITE);
   // Make sure the wait actually succeeded.
   if(WAIT_OBJECT_0 != waitResult)
   {
      LogMessage("Wait for mutex failed:  %u\n", waitResult);
      nRetValue = FAILURE;
   }

   if((SUCCESS == nRetValue) &&
      (NULL == hControlRequestEvent))
   {
      LogMessage("Error, control request event handle is NULL.\n");
      nRetValue = FAILURE;
   }

   // Initialize the OVERLAPPED structure.
   memset(&overlappedStructure, 0x00, sizeof(OVERLAPPED));
   overlappedStructure.hEvent = hControlRequestEvent;

   if(SUCCESS == nRetValue)
   {
      DWORD lastError = 0;
      // Note that bytesTransferred is needed according to
      // http://msdn.microsoft.com/en-us/library/aa363216%28VS.85%29.aspx even
      // though it is not used here.
      BOOL operationComplete = DeviceIoControl(
        hChatpadDevice,
        IOCTL_CHATPAD_IS_MS_INIT_DONE,
        NULL,
        0,
        NULL,
        0,
        &bytesTransferred,
        &overlappedStructure);

      if(FALSE == operationComplete)
      {
         // If the request failed, it is hopefully pending.
         lastError = GetLastError();

         if(ERROR_IO_PENDING == lastError)
         {
            // Wait for the request to finish.
            // TODO should I include timeout functionality?  Could these sorts of waits ever cause lockups?
            operationComplete = GetOverlappedResult(
              hChatpadDevice,
              &overlappedStructure,
              &bytesTransferred,
              TRUE);
         }
      }

      if(TRUE == operationComplete)
      {
         LogMessage("Microsoft initialization has been finished, so we can init the chatpad now.\n");
      }
      else
      {
         LogMessage("Microsoft initialization is not finished, or detection has failed:  %d\n", lastError);
         LogMessage("Please contact GAFBlizzard to find a solution to this problem.\n");
         nRetValue = FAILURE;
      }

      // Reset the event in case it was signaled.  If it was not signaled, this
      // call should simply have no effect.
      ResetEvent(hControlRequestEvent);
   } // end if(SUCCESS == nRetValue)

   // Release the mutex if we obtained it.
   //
   // The mutex is released now because the rest of the initialization is not
   // raw DeviceIoControl calls, but instead calls to other functions which
   // themselves will try to obtain the mutex.
   if(WAIT_OBJECT_0 == waitResult)
   {
      if(0 == ReleaseMutex(hControlRequestMutex))
      {
         LogMessage("Mutex release failed:  %lu\n", GetLastError());
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      unsigned char sendMessageBuffer[8];
      unsigned char receiveMessageBuffer[32];

      memset(sendMessageBuffer, 0x00, 8);
      memset(receiveMessageBuffer, 0x00, 32);

      // Note that the XBox 360 controller security interface communication is
      // presumably not needed (the Mac driver doesn't do it as far as I can tell),
      // so it is not done here either.

      // These three calls produce stalls/failures.  At any rate, if I do not send these
      // three, even with the failures they return, then the commands below to make chatpad
      // messages start will fail.
      // Apparently they are required and I do not know where they come from.  Maybe check the
      // code that I got the 0x09 0x00 data from.
      // TODO do I need to automatically reset/unstall?  Things seem to work without doing that.
      // TODO comment out the IOCTL failure message for release, since it is expected?  Or add
      //   an option to disable it for SendControlRequest?
      SendControlRequest(hChatpadDevice, MAIN_INTERFACE, 0x40, 0xa9, 0xa30c, 0x4423, 0x0000);
      SendControlRequest(hChatpadDevice, MAIN_INTERFACE, 0x40, 0xa9, 0x2344, 0x7f03, 0x0000);
      SendControlRequest(hChatpadDevice, MAIN_INTERFACE, 0x40, 0xa9, 0x5839, 0x6832, 0x0000);

      // Send control codes to make the chatpad start sending data (or to make
      // the 360 controller let the chatpad data come through, who knows how
      // it works internally :P).
      if(SUCCESS == nRetValue)
      {
         nRetValue = SendControlRequest(
           hChatpadDevice, CHATPAD_INTERFACE, 0xc0, 0xa1, 0x0000, 0xe416, 0x0002, NULL, 0, receiveMessageBuffer, 32);
         LogMessage("Control response data received:  0x%02x 0x%02x\n", receiveMessageBuffer[0], receiveMessageBuffer[1]);
      }

      if(SUCCESS == nRetValue)
      {
         // The working chatpad activation hex values are here thanks to
         // http://mbed.org/users/AjK/programs/SOWB/lg489e/docs/xbox360gamepad_8c_source.html.
         // The notes from the author there seem to imply that the security channel is required to handshake
         // with the controller in order for the chatpad to work.  That does not seem to be the case.
         // Neither this code nor the Mac driver uses the security channel to handshake, as far as I know,
         // unless the three mystery commands above (0x40, 0xa9, etc.) relate to security handshaking.
//sendMessageBuffer[0] = 0x01;
//sendMessageBuffer[1] = 0x02;
// TODO note that the above value may be necessary on some controllers
// TODO NEXT WIP try other codes here if the initial attempt fails.
         sendMessageBuffer[0] = 0x09;
         sendMessageBuffer[1] = 0x00;
         nRetValue = SendControlRequest(
           hChatpadDevice, CHATPAD_INTERFACE, 0x40, 0xa1, 0x0000, 0xe416, 0x0002, sendMessageBuffer, 2);
         LogMessage("Control response data sent:  0x%02x 0x%02x\n", sendMessageBuffer[0], sendMessageBuffer[1]);
      }

      if(SUCCESS == nRetValue)
      {
         memset(receiveMessageBuffer, 0x00, 32);
         nRetValue = SendControlRequest(
           hChatpadDevice, CHATPAD_INTERFACE, 0xc0, 0xa1, 0x0000, 0xe416, 0x0002, NULL, 0, receiveMessageBuffer, 32);
         LogMessage("Control response data received:  0x%02x 0x%02x\n", receiveMessageBuffer[0], receiveMessageBuffer[1]);
      }
   }

   // TODO NEXT remove/modify below?  At least update the comment once it is no longer "test" code,
   //   and clean up/indent etc. the code.
   // Code to test writing to the controls endpoint.
   if(SUCCESS == nRetValue)
   {
      // Thanks to http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller/UsbInfo
      // for information on the control data format.

      // This sequence of bytes should make light #1 on the controller be lit.
      // Hopefully it does not actually affect the operation of the controller.
      UCHAR writeBuffer[3] = { 0x01, 0x03, 0x02 };
      DWORD bytesToWrite = 3;

      nRetValue = WriteToControlsEndpoint(
        hChatpadDevice,
        writeBuffer,
        bytesToWrite);

      // Turn the caps lock light, green square light, orange circle light, and people light off.
      SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0000, 0x0002, 0x0000);
      SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0001, 0x0002, 0x0000);
      SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0002, 0x0002, 0x0000);
      SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0003, 0x0002, 0x0000);
      // Turn the backlight off.
      SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x0004, 0x0002, 0x0000);

      // This sequence should make the lights on the controller flash diagonally and then
      // go back to the most recently set light setting (i.e. #1 since we set it above).
      // Note that if we used the solid command instead of the blink command above, the
      // lights would ALL go to a blinking state after the alternating blink finishes.
// TODO uncomment to blink lights
/*
      writeBuffer[0] = 0x01;
      writeBuffer[1] = 0x03;
      writeBuffer[2] = 0x0D;
      bytesToWrite = 3;

      nRetValue = WriteToControlsEndpoint(
        hChatpadDevice,
        writeBuffer,
        bytesToWrite);
*/

// TODO uncomment to enable rumble
/*
UCHAR rumbleWriteBuffer[8] = { 0x00, 0x08, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00 };
bytesToWrite = 8;
nRetValue = WriteToControlsEndpoint(
  hChatpadDevice,
  rumbleWriteBuffer,
  bytesToWrite);
Sleep(500);
rumbleWriteBuffer[3] = 0x00;
rumbleWriteBuffer[4] = 0x00;
bytesToWrite = 8;
nRetValue = WriteToControlsEndpoint(
  hChatpadDevice,
  rumbleWriteBuffer,
  bytesToWrite);
Sleep(1000);
*/
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end InitChatpad

// Shuts down worker threads so that global handles can safely be deleted.
// IMPORTANT NOTE:  This function should only be called after InitializeGlobalVariables() has been called.
void ShutdownWorkerThreads(void)
{
   DWORD waitResult = 0;

   if(NULL != hWorkerThreadShutdownEvent)
   {
      // Trigger the worker thread shutdown event to stop all child threads.
      if(0 == SetEvent(hWorkerThreadShutdownEvent))
      {
         // Hopefully this never happens.
         LogMessage("Error triggering shutdown event:  %lu\n", GetLastError());
         LogMessage("Forcing an exit since normal thread shutdown failed.\n");
         exit(-1);
      }
      // Set the special-case boolean to shut down the mouse worker thread.
      bShutdownMouseWorkerThread = true;

      // Wait for the worker threads to shutdown.
      waitResult = WaitForMultipleObjects(
        NUM_WORKER_THREADS,
        hWorkerThreads,
        TRUE,     // Wait for all of the handles to be signaled.
        5000);    // Wait 5 seconds and then timeout.

      if(WAIT_OBJECT_0 != waitResult)
      {
         LogMessage("Error while waiting on threads to finish, waitResult == 0x%x\n", waitResult);
         if(WAIT_FAILED == waitResult)
         {
            LogMessage("WaitForMultipleObjects failed.  GetLastError() returns %lu.\n", GetLastError());
         }
         LogMessage("Forcing an exit since normal thread shutdown failed.\n");
         exit(-1);
      }
   } // end if(NULL != hWorkerThreadShutdownEvent)
   else
   {
      LogMessage("Error while shutting down:  Shutdown event handle is NULL.\n");
      LogMessage("Forcing an exit since normal thread shutdown failed.\n");
      exit(-1);
   }

   LogMessage("Worker threads successfully stopped.\n");
} // end ShutdownWorkerThreads

// Initializes a chatpad state data structure to default values.
int InitDefaultChatpadStateDataValues(ChatpadStateData* pStateStructure)
{
   int nRetValue = SUCCESS;

   if(NULL != pStateStructure)
   {
      pStateStructure->backlightCommandSent           = false;
      pStateStructure->capsLockActive                 = false;
      pStateStructure->currentModifiers               = CHATPAD_MODIFIER_NONE;

      pStateStructure->orangeLatchActive              = false;
      pStateStructure->greenLatchActive               = false;
      pStateStructure->peopleLatchActive              = false;
      pStateStructure->leftControlLatchActive         = false;
      pStateStructure->leftShiftLatchActive           = false;
      pStateStructure->leftAltLatchActive             = false;
      pStateStructure->leftGuiLatchActive             = false;
      pStateStructure->rightControlLatchActive        = false;
      pStateStructure->rightShiftLatchActive          = false;
      pStateStructure->rightAltLatchActive            = false;
      pStateStructure->rightGuiLatchActive            = false;

      for(int i = 0; i < 5; i++)
      {
         pStateStructure->previousChatpadRawData[i]   = 0x00;
      }

      pStateStructure->currentKeysDown[0]             = CHATPAD_KEY_NONE;
      pStateStructure->currentKeysDownModifiers[0]    = CHATPAD_MODIFIER_NONE;
      pStateStructure->currentKeysDown[1]             = CHATPAD_KEY_NONE;
      pStateStructure->currentKeysDownModifiers[1]    = CHATPAD_MODIFIER_NONE;
      pStateStructure->ignoreNextRepeatedData         = false;
   }
   else
   {
      nRetValue = FAILURE;
   }

   return nRetValue;
} // end InitDefaultChatpadStateDataValues

// Initializes global variables.
// IMPORTANT NOTE:  LaunchWorkerThreads() must be called after this function is called,
//   since this function sets the global thread handles to NULL.
int InitializeGlobalVariables(void)
{
   int nRetValue = SUCCESS;

   // Initialize handles to NULL since the worker threads have not yet been created.
   for(int workerIndex = 0; workerIndex < NUM_WORKER_THREADS; workerIndex++)
   {
      hWorkerThreads[workerIndex] = NULL;
   }

   // Note that in failure cases, all creation calls are still made so that all
   // globals will be initialized to something by the end of this function.

   hControlRequestEvent = CreateEvent(
     NULL,     // Inherit security attributes.
     TRUE,     // Manual reset.
     FALSE,    // Initially nonsignaled.
     NULL);    // No name.
   if(NULL == hControlRequestEvent)
   {
      LogMessage("Error, control request event creation failed.\n");
      nRetValue = FAILURE;
   }

   hControlRequestMutex = CreateMutex(NULL, FALSE, NULL);
   if(NULL == hControlRequestMutex)
   {
      LogMessage("Error, control request mutex creation failed.\n");
      nRetValue = FAILURE;
   }

   for(int eventIndex = 0; eventIndex < NUM_PENDING_READS_TO_USE; eventIndex++)
   {
      hControlsReadEvents[eventIndex] = CreateEvent(
        NULL,     // Inherit security attributes.
        TRUE,     // Manual reset.
        FALSE,    // Initially nonsignaled.
        NULL);    // No name.
      if(NULL == hControlsReadEvents[eventIndex])
      {
         LogMessage("Error, controls read event creation failed.\n");
         nRetValue = FAILURE;
      }

      hChatpadReadEvents[eventIndex] = CreateEvent(
        NULL,     // Inherit security attributes.
        TRUE,     // Manual reset.
        FALSE,    // Initially nonsignaled.
        NULL);    // No name.
      if(NULL == hChatpadReadEvents[eventIndex])
      {
         LogMessage("Error, chatpad read event creation failed.\n");
         nRetValue = FAILURE;
      }
   } // end for(int eventIndex = 0; eventIndex < NUM_PENDING_READS_TO_USE; eventIndex++)

   hVirtualKeyboardDevice = INVALID_HANDLE_VALUE;
   hVirtualKeyboardSendEvent = CreateEvent(
     NULL,     // Inherit security attributes.
     TRUE,     // Manual reset.
     FALSE,    // Initially nonsignaled.
     NULL);    // No name.
   if(NULL == hVirtualKeyboardSendEvent)
   {
      LogMessage("Error, virtual keyboard send event creation failed.\n");
      nRetValue = FAILURE;
   }

   currentKeyboardModifiers = KEYBOARD_MODIFIER_NONE;
   for(int i = 0; i < MAX_SIMULTANEOUS_KEYS; i++)
   {
      previousOutputKeysHeldDown[i]          = KEYBOARD_KEY_NONE;
      currentOutputKeysHeldDown[i]           = KEYBOARD_KEY_NONE;
      temporaryKeyboardModifiersToRelease[i] = KEYBOARD_MODIFIER_NONE;
   }

   hVirtualMouseDevice = INVALID_HANDLE_VALUE;
   hVirtualMouseSendEvent = CreateEvent(
     NULL,     // Inherit security attributes.
     TRUE,     // Manual reset.
     FALSE,    // Initially nonsignaled.
     NULL);    // No name.
   if(NULL == hVirtualMouseSendEvent)
   {
      LogMessage("Error, virtual mouse send event creation failed.\n");
      nRetValue = FAILURE;
   }

   hStatesMutex = CreateMutex(NULL, FALSE, NULL);
   if(NULL == hStatesMutex)
   {
      LogMessage("Error, states mutex creation failed.\n");
      nRetValue = FAILURE;
   }

   uVirtualMouseState = 0x00000000;

   hMouseStateMutex = CreateMutex(NULL, FALSE, NULL);
   if(NULL == hMouseStateMutex)
   {
      LogMessage("Error, mouse state mutex creation failed.\n");
      nRetValue = FAILURE;
   }

   pDriverConfigData = static_cast<ChatpadDriverConfigStructure*>(malloc(sizeof(ChatpadDriverConfigStructure)));
   if(NULL != pDriverConfigData)
   {
      nRetValue = InitDefaultDriverConfigDataValues(pDriverConfigData);
   }
   else
   {
      LogMessage("Error allocating memory for driver config structure.\n");
      nRetValue = FAILURE;
   }

   pChatpadStateData = static_cast<ChatpadStateData*>(malloc(sizeof(ChatpadStateData)));
   if(NULL != pChatpadStateData)
   {
      nRetValue = InitDefaultChatpadStateDataValues(pChatpadStateData);
   }
   else
   {
      LogMessage("Error allocating memory for chatpad state data structure.\n");
      nRetValue = FAILURE;
   }

   bWindowsModeActive = false;

   for(int i = 0; i < MAX_SIMULTANEOUS_CONTROLLER_ACTIONS; i++)
   {
      controllerTriggeredActions[i]          = NULL;
      controllerTriggeredActionModifiers[i]  = CONTROLLER_MODIFIER_NONE;
   }

   for(int i = 0; i < CONTROLLER_NUM_BUTTONS; i++)
   {
      controllerButtonsPressed[i] = false;
   }

   hWorkerThreadShutdownEvent = CreateEvent(
     NULL,     // Inherit security attributes.
     TRUE,     // Manual reset.
     FALSE,    // Initially nonsignaled.
     NULL);    // No name.
   if(NULL == hWorkerThreadShutdownEvent)
   {
      LogMessage("Error, worker thread shutdown event creation failed.\n");
      nRetValue = FAILURE;
   }

   bShutdownMouseWorkerThread = false;

   return nRetValue;
} // end InitializeGlobalVariables

// Release global objects.
// IMPORTANT NOTE:  ShutdownWorkerThreads() must be called before this function is
//   called, since I assume that it is a bad idea to close handles while
//   other threads may be using them.
void CleanupGlobalVariables(void)
{
   // The worker threads should have already shut down, so we close the handles here.
   for(int workerIndex = 0; workerIndex < NUM_WORKER_THREADS; workerIndex++)
   {
      if(NULL != hWorkerThreads[workerIndex])
      {
         CloseHandle(hWorkerThreads[workerIndex]);
         hWorkerThreads[workerIndex] = NULL;
      }
   }

   if(NULL != hControlRequestEvent)
   {
      CloseHandle(hControlRequestEvent);
      hControlRequestEvent = NULL;
   }

   if(NULL != hControlRequestMutex)
   {
      CloseHandle(hControlRequestMutex);
      hControlRequestMutex = NULL;
   }

   for(int eventIndex = 0; eventIndex < NUM_PENDING_READS_TO_USE; eventIndex++)
   {
      if(NULL != hControlsReadEvents[eventIndex])
      {
         CloseHandle(hControlsReadEvents[eventIndex]);
         hControlsReadEvents[eventIndex] = NULL;
      }

      if(NULL != hChatpadReadEvents[eventIndex])
      {
         CloseHandle(hChatpadReadEvents[eventIndex]);
         hChatpadReadEvents[eventIndex] = NULL;
      }
   }

   if(NULL != hVirtualKeyboardSendEvent)
   {
      CloseHandle(hVirtualKeyboardSendEvent);
      hVirtualKeyboardSendEvent = NULL;
   }

   if(NULL != hVirtualMouseSendEvent)
   {
      CloseHandle(hVirtualMouseSendEvent);
      hVirtualMouseSendEvent = NULL;
   }

   if(NULL != hStatesMutex)
   {
      CloseHandle(hStatesMutex);
      hStatesMutex = NULL;
   }

   if(NULL != hMouseStateMutex)
   {
      CloseHandle(hMouseStateMutex);
      hMouseStateMutex = NULL;
   }

   if(NULL != pDriverConfigData)
   {
      free(pDriverConfigData);
      pDriverConfigData = NULL;
   }

   if(NULL != pChatpadStateData)
   {
      free(pChatpadStateData);
      pChatpadStateData = NULL;
   }
   
   bWindowsModeActive = false;

   if(NULL != hWorkerThreadShutdownEvent)
   {
      CloseHandle(hWorkerThreadShutdownEvent);
      hWorkerThreadShutdownEvent = NULL;
   }
} // end CleanupGlobalVariables

// Main loop for chatpad control code.
int ChatpadControlMainLoop(void)
{
   int               nRetValue      = SUCCESS;
   BOOL              bAPIResult     = TRUE;
   BOOL              bFinished      = FALSE;
   HANDLE            hChatpadDevice = INVALID_HANDLE_VALUE;

   // This message should show up in DebugView.
   OutputDebugString(("Chatpad control main loop started.\n"));

   //***
   // Initialize global variables.
   //***
   nRetValue = InitializeGlobalVariables();
   if(SUCCESS != nRetValue)
   {
      LogMessage("Failed to initialize global variables.\n");
   }

   //***
   // Load configuration data.
   //***
   if(SUCCESS == nRetValue)
   {
      nRetValue = LoadConfigFile(CHATPAD_DRIVER_CONFIG_FILE_NAME, pDriverConfigData);
   }

   //***
   // Open device.
   //***

   if(SUCCESS == nRetValue)
   {
      nRetValue = OpenController(&hChatpadDevice);
      if(INVALID_HANDLE_VALUE != hChatpadDevice)
      {
         LogMessage("Successfully connected to XBox 360 controller.\n");
      }
      else
      {
         LogMessage("Failed to open connection to XBox 360 controller.\n");
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      nRetValue = OpenVirtualKeyboard(&hVirtualKeyboardDevice);
      if(INVALID_HANDLE_VALUE != hVirtualKeyboardDevice)
      {
         LogMessage("Successfully connected to the virtual keyboard device.\n");
         // Enable the virtual keyboard device (even though this currently
         // happens automatically when the application connects).
         SetVirtualKeyboardDeviceEnabled(CHATPAD_KEYBOARD_KMDF_ENABLE);
      }
      else
      {
         LogMessage("Failed to open connection to the virtual keyboard device.\n");
         nRetValue = FAILURE;
      }
   }

   // TODO eventually don't even try to connect to the mouse device, and provide an option
   //   not to install it?  It shouldn't do anything if it is not installed, however.
   if(SUCCESS == nRetValue)
   {
      nRetValue = OpenVirtualMouse(&hVirtualMouseDevice);
      if(INVALID_HANDLE_VALUE != hVirtualMouseDevice)
      {
         LogMessage("Successfully connected to the virtual mouse device.\n");
         // Enable the virtual mouse device (even though this currently
         // happens automatically when the application connects).
         SetVirtualMouseDeviceEnabled(CHATPAD_MOUSE_KMDF_ENABLE);
      }
      else
      {
         LogMessage("Failed to open connection to the virtual mouse device.\n");
         nRetValue = FAILURE;
      }
   }

   //***
   // Initialize chatpad.
   //***

   if(SUCCESS == nRetValue)
   {
      nRetValue = InitChatpad(hChatpadDevice);
      if(SUCCESS != nRetValue)
      {
         LogMessage("Error initializing chatpad.\n");
      }
   }

   if(SUCCESS == nRetValue)
   {
      // This code helps make sure controls data is not initially intercepted.
      // Filtering is enabled unless the passthrough mode should be used.
      if(true == pDriverConfigData->bEnableIngameRemappedButtons)
      {
         SetFilterMode(hChatpadDevice, FILTER_MODE_FILTERED);
      }
      else
      {
         SetFilterMode(hChatpadDevice, FILTER_MODE_UNFILTERED);
      }
   }

   //***
   // Start main loop to send messages that keep the chatpad alive.
   //***

   if(SUCCESS == nRetValue)
   {
// TODO NEXT move elsewhere, and implement eventually
/*
{
   UCHAR writeBuffer[4] = { 0x00, 0x00, 0x00, 0x00 };
   DWORD bytesToWrite = 4;

   nRetValue = SetControlsMappings(
     hChatpadDevice,
     writeBuffer,
     bytesToWrite);
}
*/

      // Launch threads to handle controller and chatpad data.
      LaunchWorkerThreads(hChatpadDevice);

      // Send command messages to keep the chatpad alive.
      while(FALSE == bFinished)
      {
         nRetValue = SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x001f, 0x0002, 0x0000);
         if(SUCCESS != nRetValue)
         {
            bFinished = TRUE;
            continue;
         }
         Sleep(1000);

         nRetValue = SendControlRequest(hChatpadDevice, CHATPAD_INTERFACE, 0x41, 0x00, 0x001e, 0x0002, 0x0000);
         if(SUCCESS != nRetValue)
         {
            bFinished = TRUE;
            continue;
         }
         Sleep(1000);
      }
   } // end if(SUCCESS == nRetValue)

// TODO add Control-C handler, or a different method of running and quitting things?  Currently,
//   none of this cleanup code gets called if Control-C is used to just kill the control utility.

   //***
   // Clean things up
   //***

   // Make sure the driver stops intercepting or filtering controls data when this utility exits.
   if(INVALID_HANDLE_VALUE != hChatpadDevice)
   {
      SetFilterMode(hChatpadDevice, FILTER_MODE_UNFILTERED);
   }

// TODO NEXT add code to set mapping options back to normal when the utility
//   exits, and make sure the equivalent code is also in the driver itself,
//   like the way the filtering stuff already works

   // Shutdown the worker threads so that we can safely close handles.
   // Note that this will not get called if one of the threads is created
   // and the other one fails for some reason.  In that case, any remaining
   // threads will presumably just be terminated by Windows when the program
   // exits.
   bool bShutdownWorkerThreads = true;
   for(int i = 0; i < NUM_WORKER_THREADS; i++)
   {
      if(NULL == hWorkerThreads[i])
      {
         bShutdownWorkerThreads = false;
         break;
      }
   }
   if(true == bShutdownWorkerThreads)
   {
      ShutdownWorkerThreads();
   }

   // Disable and close the virtual keyboard device.
   if(INVALID_HANDLE_VALUE != hVirtualKeyboardDevice)
   {
      // Disable the virtual keyboard device (even though this currently
      // happens automatically when the application disconnects).
      SetVirtualKeyboardDeviceEnabled(CHATPAD_KEYBOARD_KMDF_DISABLE);

      LogMessage("Closing connection to virtual keyboard device.\n");
      CloseHandle(hVirtualKeyboardDevice);
      hVirtualKeyboardDevice = INVALID_HANDLE_VALUE;
   }

   // Disable and close the virtual mouse device.
   if(INVALID_HANDLE_VALUE != hVirtualMouseDevice)
   {
      // Disable the virtual mouse device (even though this currently
      // happens automatically when the application disconnects).
      SetVirtualMouseDeviceEnabled(CHATPAD_MOUSE_KMDF_DISABLE);

      LogMessage("Closing connection to virtual mouse device.\n");
      CloseHandle(hVirtualMouseDevice);
      hVirtualMouseDevice = INVALID_HANDLE_VALUE;
   }

   // Close the chatpad control device.
   if(INVALID_HANDLE_VALUE != hChatpadDevice)
   {
      LogMessage("Closing chatpad filter driver.\n");
      CloseHandle(hChatpadDevice);
      hChatpadDevice = INVALID_HANDLE_VALUE;
   }

   // Cleanup global objects.
   CleanupGlobalVariables();

   // This message should show up in DebugView.
   OutputDebugString(("Chatpad control main loop finished.\n"));

	return nRetValue;
} // end ChatpadControlMainLoop

