//
// Chatpad utility code that works with a KMDF USB filter driver, by
//   GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
// This particular file contains constants for various chatpad/controller
// keys and commands.  Many of these may result in configurable actions.
//
// Information for reading and writing to 360 controller:
//   http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller/UsbInfo
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//

#ifndef CHATPAD_CONTROL_H
#define CHATPAD_CONTROL_H

//***
// General global constants.
//***

// Name of the config file.
// TODO make configurable?
#define CHATPAD_DRIVER_CONFIG_FILE_NAME   "chatpad_config.txt"

// The HID keyboard report permits six keys to be held down simultaneously, if I understand correctly.
// IMPORTANT NOTE:  This number must match the number of simultaneous keys
//                  supported by the HID report in the virtual keyboard driver,
//                  and the number of simultaneous keys that the virtual
//                  keyboard device is expecting for a data IOCTL.
#define MAX_SIMULTANEOUS_KEYS                6

// This is the maximum number of simultaneous actions (i.e. button presses)
// allowed for the XBox 360 controller when managing button binds to things
// like simulated key actions and simulated mouse actions.
#define MAX_SIMULTANEOUS_CONTROLLER_ACTIONS  16

// This is the number of bytes that must be sent in each virtual mouse message.
#define VIRTUAL_MOUSE_MESSAGE_NUM_BYTES      4

// Define success and failure values.
#ifndef SUCCESS
#define SUCCESS    0
#endif

#ifndef FAILURE
#define FAILURE   -1
#endif

// This is a convenience shortcut for logging/printing messages.
#define LogMessage(...)    LogMessageAdvanced(false, __VA_ARGS__)

// This function is used for logging/printing messages.
void LogMessageAdvanced(bool bAddNewline, char *pMessageString, ...);


//***
// Constants representing chatpad keys.  These constants can be combined with modifiers.
// The values of these constants are not significant other than that they are unique
// and sequential from 0 to CHATPAD_NUM_KEYS.  This allows an array to be used to get
// mapping data for each key.
// The actual codes from the chatpad are mapped to these constants, after which
// they can be mapped to simulated keyboard keys under Windows.  The mapping for
// Windows can be configured to allow different key bind preferences.
//***

typedef enum ChatpadKey
{
   CHATPAD_KEY_NONE        = 0,
   CHATPAD_KEY_1,
   CHATPAD_KEY_2,
   CHATPAD_KEY_3,
   CHATPAD_KEY_4,
   CHATPAD_KEY_5,
   CHATPAD_KEY_6,
   CHATPAD_KEY_7,
   CHATPAD_KEY_8,
   CHATPAD_KEY_9,
   CHATPAD_KEY_0,
   CHATPAD_KEY_Q,
   CHATPAD_KEY_W,
   CHATPAD_KEY_E,
   CHATPAD_KEY_R,
   CHATPAD_KEY_T,
   CHATPAD_KEY_Y,
   CHATPAD_KEY_U,
   CHATPAD_KEY_I,
   CHATPAD_KEY_O,
   CHATPAD_KEY_P,
   CHATPAD_KEY_A,
   CHATPAD_KEY_S,
   CHATPAD_KEY_D,
   CHATPAD_KEY_F,
   CHATPAD_KEY_G,
   CHATPAD_KEY_H,
   CHATPAD_KEY_J,
   CHATPAD_KEY_K,
   CHATPAD_KEY_L,
   CHATPAD_KEY_COMMA,
   CHATPAD_KEY_SHIFT,
   CHATPAD_KEY_Z,
   CHATPAD_KEY_X,
   CHATPAD_KEY_C,
   CHATPAD_KEY_V,
   CHATPAD_KEY_B,
   CHATPAD_KEY_N,
   CHATPAD_KEY_M,
   CHATPAD_KEY_PERIOD,
   CHATPAD_KEY_ENTER,
   CHATPAD_KEY_GREEN,
   CHATPAD_KEY_PEOPLE,
   CHATPAD_KEY_LEFT,
   CHATPAD_KEY_SPACEBAR,
   CHATPAD_KEY_RIGHT,
   CHATPAD_KEY_BACKSPACE,
   CHATPAD_KEY_ORANGE,

   CHATPAD_NUM_KEYS
}; // end typedef enum ChatpadKey


//***
// Chatpad modifier codes
//***

#define CHATPAD_MODIFIER_NONE          0x00000000
#define CHATPAD_MODIFIER_SHIFT         0x00000001
#define CHATPAD_MODIFIER_GREEN         0x00000002
#define CHATPAD_MODIFIER_ORANGE        0x00000004
#define CHATPAD_MODIFIER_PEOPLE        0x00000008


//***
// Constants representing binding types.  Keys/buttons on the chatpad/controller can
// be bound to nothing, a simulated output (optionally modified) virtual keyboard action,
// a controller action, a simulated mouse action, or a special action.
//
// Only controller buttons can be bound to trigger controller buttons, however.
//***

typedef enum BindingType
{
   BINDING_NONE                     = 0,
   BINDING_SIMULATED_KEY,
   BINDING_CONTROLLER,
   BINDING_SIMULATED_MOUSE_ACTION,
   BINDING_SPECIAL
}; // end typedef enum BindingType


//***
// Constants representing keyboard keys.  These constants can be combined with modifiers.
// Note that these values match the specification for USB HID scan codes.
// (see USB HID Usage Tables, version 1.12, section 10)
//***
#define KEYBOARD_KEY_NONE           0x00
#define KEYBOARD_KEY_A              0x04
#define KEYBOARD_KEY_B              0x05
#define KEYBOARD_KEY_C              0x06
#define KEYBOARD_KEY_D              0x07
#define KEYBOARD_KEY_E              0x08
#define KEYBOARD_KEY_F              0x09
#define KEYBOARD_KEY_G              0x0A
#define KEYBOARD_KEY_H              0x0B
#define KEYBOARD_KEY_I              0x0C
#define KEYBOARD_KEY_J              0x0D
#define KEYBOARD_KEY_K              0x0E
#define KEYBOARD_KEY_L              0x0F
#define KEYBOARD_KEY_M              0x10
#define KEYBOARD_KEY_N              0x11
#define KEYBOARD_KEY_O              0x12
#define KEYBOARD_KEY_P              0x13
#define KEYBOARD_KEY_Q              0x14
#define KEYBOARD_KEY_R              0x15
#define KEYBOARD_KEY_S              0x16
#define KEYBOARD_KEY_T              0x17
#define KEYBOARD_KEY_U              0x18
#define KEYBOARD_KEY_V              0x19
#define KEYBOARD_KEY_W              0x1A
#define KEYBOARD_KEY_X              0x1B
#define KEYBOARD_KEY_Y              0x1C
#define KEYBOARD_KEY_Z              0x1D
#define KEYBOARD_KEY_1              0x1E
#define KEYBOARD_KEY_2              0x1F
#define KEYBOARD_KEY_3              0x20
#define KEYBOARD_KEY_4              0x21
#define KEYBOARD_KEY_5              0x22
#define KEYBOARD_KEY_6              0x23
#define KEYBOARD_KEY_7              0x24
#define KEYBOARD_KEY_8              0x25
#define KEYBOARD_KEY_9              0x26
#define KEYBOARD_KEY_0              0x27
#define KEYBOARD_KEY_ENTER          0x28
#define KEYBOARD_KEY_ESCAPE         0x29
#define KEYBOARD_KEY_BACKSPACE      0x2A
#define KEYBOARD_KEY_TAB            0x2B
#define KEYBOARD_KEY_SPACEBAR       0x2C
#define KEYBOARD_KEY_MINUS          0x2D
#define KEYBOARD_KEY_EQUALS         0x2E
#define KEYBOARD_KEY_OPEN_BRACKET   0x2F
#define KEYBOARD_KEY_CLOSE_BRACKET  0x30
#define KEYBOARD_KEY_BACKSLASH      0x31
// "Keyboard Non-US # and ~" skipped here
#define KEYBOARD_KEY_SEMICOLON      0x33
#define KEYBOARD_KEY_APOSTROPHE     0x34
#define KEYBOARD_KEY_GRAVE_ACCENT   0x35
#define KEYBOARD_KEY_COMMA          0x36
#define KEYBOARD_KEY_PERIOD         0x37
#define KEYBOARD_KEY_FORWARDSLASH   0x38
#define KEYBOARD_KEY_CAPS_LOCK      0x39
#define KEYBOARD_KEY_F1             0x3A
#define KEYBOARD_KEY_F2             0x3B
#define KEYBOARD_KEY_F3             0x3C
#define KEYBOARD_KEY_F4             0x3D
#define KEYBOARD_KEY_F5             0x3E
#define KEYBOARD_KEY_F6             0x3F
#define KEYBOARD_KEY_F7             0x40
#define KEYBOARD_KEY_F8             0x41
#define KEYBOARD_KEY_F9             0x42
#define KEYBOARD_KEY_F10            0x43
#define KEYBOARD_KEY_F11            0x44
#define KEYBOARD_KEY_F12            0x45
#define KEYBOARD_KEY_PRINTSCREEN    0x46
#define KEYBOARD_KEY_SCROLL_LOCK    0x47
#define KEYBOARD_KEY_PAUSE          0x48
#define KEYBOARD_KEY_INSERT         0x49
#define KEYBOARD_KEY_HOME           0x4A
#define KEYBOARD_KEY_PAGEUP         0x4B
#define KEYBOARD_KEY_DELETE         0x4C
#define KEYBOARD_KEY_END            0x4D
#define KEYBOARD_KEY_PAGEDOWN       0x4E
#define KEYBOARD_KEY_RIGHT          0x4F
#define KEYBOARD_KEY_LEFT           0x50
#define KEYBOARD_KEY_DOWN           0x51
#define KEYBOARD_KEY_UP             0x52
#define KEYBOARD_KEY_KEYPAD_NUM_LOCK      0x53
#define KEYBOARD_KEY_KEYPAD_FORWARDSLASH  0x54
#define KEYBOARD_KEY_KEYPAD_ASTERISK      0x55
#define KEYBOARD_KEY_KEYPAD_MINUS         0x56
#define KEYBOARD_KEY_KEYPAD_PLUS          0x57
#define KEYBOARD_KEY_KEYPAD_ENTER         0x58
#define KEYBOARD_KEY_KEYPAD_1             0x59
#define KEYBOARD_KEY_KEYPAD_2             0x5A
#define KEYBOARD_KEY_KEYPAD_3             0x5B
#define KEYBOARD_KEY_KEYPAD_4             0x5C
#define KEYBOARD_KEY_KEYPAD_5             0x5D
#define KEYBOARD_KEY_KEYPAD_6             0x5E
#define KEYBOARD_KEY_KEYPAD_7             0x5F
#define KEYBOARD_KEY_KEYPAD_8             0x60
#define KEYBOARD_KEY_KEYPAD_9             0x61
#define KEYBOARD_KEY_KEYPAD_0             0x62
#define KEYBOARD_KEY_KEYPAD_PERIOD        0x63
// "Keyboard Non-US \ and |" skipped here
// "Keyboard Application" skipped here
// "Keyboard Power" skipped here
// "Keypad =" skipped here
#define KEYBOARD_KEY_F13                  0x68
#define KEYBOARD_KEY_F14                  0x69
#define KEYBOARD_KEY_F15                  0x6A
#define KEYBOARD_KEY_F16                  0x6B
#define KEYBOARD_KEY_F17                  0x6C
#define KEYBOARD_KEY_F18                  0x6D
#define KEYBOARD_KEY_F19                  0x6E
#define KEYBOARD_KEY_F20                  0x6F
#define KEYBOARD_KEY_F21                  0x70
#define KEYBOARD_KEY_F22                  0x71
#define KEYBOARD_KEY_F23                  0x72
#define KEYBOARD_KEY_F24                  0x73
#define KEYBOARD_KEY_EXECUTE              0x74
#define KEYBOARD_KEY_HELP                 0x75
#define KEYBOARD_KEY_MENU                 0x76
#define KEYBOARD_KEY_SELECT               0x77
#define KEYBOARD_KEY_STOP                 0x78
#define KEYBOARD_KEY_AGAIN                0x79
#define KEYBOARD_KEY_UNDO                 0x7A
#define KEYBOARD_KEY_CUT                  0x7B
#define KEYBOARD_KEY_COPY                 0x7C
#define KEYBOARD_KEY_PASTE                0x7D
#define KEYBOARD_KEY_FIND                 0x7E
#define KEYBOARD_KEY_MUTE                 0x7F
#define KEYBOARD_KEY_VOLUME_UP            0x80
#define KEYBOARD_KEY_VOLUME_DOWN          0x81
// No more keys from the USB HID specification seem useful after this point.


//***
// Keyboard modifier codes.  Note that these values match the specification for the USB HID modifier byte.
// (see HID Device Class Definition, version 1.11, section 8.3)
//***
// TODO note in documentation that the "GUI" keys would presumably correspond
//   to the Windows key(s)
// TODO is it even possible to display odd symbols and international letters for an English HID keyboard?
//   Maybe through AltGr?  Need to do more research.
#define KEYBOARD_MODIFIER_NONE            0x00
#define KEYBOARD_MODIFIER_LEFT_CONTROL    0x01
#define KEYBOARD_MODIFIER_LEFT_SHIFT      0x02
#define KEYBOARD_MODIFIER_LEFT_ALT        0x04
#define KEYBOARD_MODIFIER_LEFT_GUI        0x08
#define KEYBOARD_MODIFIER_RIGHT_CONTROL   0x10
#define KEYBOARD_MODIFIER_RIGHT_SHIFT     0x20
#define KEYBOARD_MODIFIER_RIGHT_ALT       0x40
#define KEYBOARD_MODIFIER_RIGHT_GUI       0x80


//***
// Constants representing simulated mouse actions.
// The values of these constants are not significant other than that they are unique.
//***

typedef enum SimulatedMouseAction
{
   MOUSE_CLICK_LEFT           = 0,        // left click
   MOUSE_CLICK_MIDDLE,                    // middle click
   MOUSE_CLICK_RIGHT,                     // right click

   MOUSE_WHEEL_UP,                        // vertical scroll wheel up
   MOUSE_WHEEL_DOWN,                      // vertical scroll wheel down

   MOUSE_NUM_ACTIONS
}; // end typedef enum SimulatedMouseAction


//***
// Constants representing controller buttons.  These constants can be combined with a modifier
// for mapping buttons in windows mode as opposed to buttons when windows mode is not active.
// The values of these constants are not significant other than that they are unique
// and sequential from 0 to CONTROLLER_NUM_BUTTONS.  This allows an array to be used to get
// mapping data for each button.
//***

typedef enum ControllerButton
{
   CONTROLLER_FIRST_BUTTON_VALUE = 0,

   CONTROLLER_BUTTON_LB          = 0,     // left bumper
   CONTROLLER_BUTTON_RB,                  // right bumper
   CONTROLLER_BUTTON_BACK,                // back button
   CONTROLLER_BUTTON_GUIDE,               // guide button
   CONTROLLER_BUTTON_START,               // start button
   CONTROLLER_BUTTON_LEFT_STICK_CLICK,    // left stick click
   CONTROLLER_BUTTON_RIGHT_STICK_CLICK,   // right stick click
   CONTROLLER_BUTTON_A,                   // A button
   CONTROLLER_BUTTON_B,                   // B button
   CONTROLLER_BUTTON_X,                   // X button
   CONTROLLER_BUTTON_Y,                   // Y button

   CONTROLLER_NUM_BUTTONS
}; // end typedef enum ControllerButton


//***
// Special actions.  Chatpad keys can be mapped to these actions.
//***
#define SPECIAL_ACTION_LEFT_CTRL             0x01
#define SPECIAL_ACTION_LEFT_CTRL_LATCH       0x02
#define SPECIAL_ACTION_LEFT_SHIFT            0x03
#define SPECIAL_ACTION_LEFT_SHIFT_LATCH      0x04
#define SPECIAL_ACTION_LEFT_ALT              0x05
#define SPECIAL_ACTION_LEFT_ALT_LATCH        0x06
#define SPECIAL_ACTION_LEFT_GUI              0x07
#define SPECIAL_ACTION_LEFT_GUI_LATCH        0x08
#define SPECIAL_ACTION_RIGHT_CTRL            0x09
#define SPECIAL_ACTION_RIGHT_CTRL_LATCH      0x0A
#define SPECIAL_ACTION_RIGHT_SHIFT           0x0B
#define SPECIAL_ACTION_RIGHT_SHIFT_LATCH     0x0C
#define SPECIAL_ACTION_RIGHT_ALT             0x0D
#define SPECIAL_ACTION_RIGHT_ALT_LATCH       0x0E
#define SPECIAL_ACTION_RIGHT_GUI             0x0F
#define SPECIAL_ACTION_RIGHT_GUI_LATCH       0x10
#define SPECIAL_ACTION_ORANGE                0x11
#define SPECIAL_ACTION_ORANGE_LATCH          0x12
#define SPECIAL_ACTION_GREEN                 0x13
#define SPECIAL_ACTION_GREEN_LATCH           0x14
#define SPECIAL_ACTION_PEOPLE                0x15
#define SPECIAL_ACTION_PEOPLE_LATCH          0x16
#define SPECIAL_ACTION_TOGGLE_WINDOWS_MODE   0x17


//***
// Controller modifier codes.
//***
#define CONTROLLER_MODIFIER_NONE          0x00000000
// Windows mode is where the thumbsticks can be used
// to control the mouse, etc.
#define CONTROLLER_MODIFIER_WINDOWS_MODE  0x00000001


//***
// Data structures
//***

// State information used for handling chatpad data.
typedef struct ChatpadStateData_t
{
   // Whether the 0x001b backlight command has been sent yet.  It must be sent
   // in the proper sequence.  See chatpad_control_code.cpp in the
   // HandleChatpadData function for details.
   bool backlightCommandSent;

   // Whether caps lock is currently active.  Caps lock is different from normal
   // modifiers and latch keys, so it is its own boolean.  Caps lock does not
   // provide modifiers to keys but instead just corresponds to the caps lock
   // state that Windows tracks for keyboards.
   bool capsLockActive;

   // Currently active modifiers.  This value is set by using binary OR
   // operations to combine CHATPAD_MODIFIER_ constants.
   // If no modifiers are currently active, this value will be set to
   // CHATPAD_MODIFIER_NONE.
   unsigned int currentModifiers;

   // Whether various special latch actions are active.  For each of these
   // actions that are active, a chatpad or keyboard modifier will be applied
   // to the next chatpad key press or simulated key event that occurs,
   // unless the latch action triggers again, which will turn the latch back
   // off.

   // Chatpad event latches.
   bool orangeLatchActive;
   bool greenLatchActive;
   bool peopleLatchActive;
   // Simulated keyboard event latches.
   bool leftControlLatchActive;
   bool leftShiftLatchActive;
   bool leftAltLatchActive;
   bool leftGuiLatchActive;
   bool rightControlLatchActive;
   bool rightShiftLatchActive;
   bool rightAltLatchActive;
   bool rightGuiLatchActive;

   // The previous raw message received from the chatpad, used for detecting duplicate messages.
   // Note that this does NOT include messages starting with 0xf0 in the first byte.  Those
   // messages are currently not used by the chatpad control code.
   unsigned char previousChatpadRawData[5];

   // Which keys are currently held down, identified by CHATPAD_KEY_ constants.
   // If either of the slots does not correspond to a key, then it will be
   // set to CHATPAD_KEY_NONE.
   unsigned char currentKeysDown[2];
   // Modifiers that were active for each of the keys that were pressed.
   // These values are needed to detect when certain combinations were
   // released.
   unsigned int currentKeysDownModifiers[2];
   // If this value is true, the next set of chatpad data that exactly matches
   // the last set will be ignored.
   bool ignoreNextRepeatedData;
} ChatpadStateData;


//***
// Function prototypes.
//***

// This function provides the main loop for chatpad control.
int ChatpadControlMainLoop(void);


#endif // CHATPAD_CONTROL_H

