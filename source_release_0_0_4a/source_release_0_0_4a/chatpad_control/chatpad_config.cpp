//
// Chatpad utility code that works with a KMDF USB filter driver, by
//   GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
// This particular file contains code to read data from a configuration
// file into data structures.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//


//***
// Include files
//***

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chatpad_config.h"


//***
// Global variables
//***

// Table that matches config file mapping-related strings to their corresponding numerical constants.
//
// IMPORTANT NOTE:  The string values below (corresponding to pString) must not be NULL.
//
// IMPORTANT NOTE:  If the number of strings in this string table is changed,
// then uNumConfigStringTableStrings must be updated to match.
ChatpadDriverConfigStringMapping configStringTable[] =
{
   // Strings corresponding to chatpad keys.
   //
   // String count for this section:  47
   { "CHATPAD_KEY_1",                  CHATPAD_KEY_1,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_2",                  CHATPAD_KEY_2,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_3",                  CHATPAD_KEY_3,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_4",                  CHATPAD_KEY_4,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_5",                  CHATPAD_KEY_5,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_6",                  CHATPAD_KEY_6,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_7",                  CHATPAD_KEY_7,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_8",                  CHATPAD_KEY_8,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_9",                  CHATPAD_KEY_9,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_0",                  CHATPAD_KEY_0,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_Q",                  CHATPAD_KEY_Q,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_W",                  CHATPAD_KEY_W,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_E",                  CHATPAD_KEY_E,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_R",                  CHATPAD_KEY_R,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_T",                  CHATPAD_KEY_T,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_Y",                  CHATPAD_KEY_Y,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_U",                  CHATPAD_KEY_U,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_I",                  CHATPAD_KEY_I,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_O",                  CHATPAD_KEY_O,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_P",                  CHATPAD_KEY_P,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_A",                  CHATPAD_KEY_A,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_S",                  CHATPAD_KEY_S,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_D",                  CHATPAD_KEY_D,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_F",                  CHATPAD_KEY_F,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_G",                  CHATPAD_KEY_G,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_H",                  CHATPAD_KEY_H,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_J",                  CHATPAD_KEY_J,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_K",                  CHATPAD_KEY_K,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_L",                  CHATPAD_KEY_L,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_COMMA",              CHATPAD_KEY_COMMA,      CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_SHIFT",              CHATPAD_KEY_SHIFT,      CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_Z",                  CHATPAD_KEY_Z,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_X",                  CHATPAD_KEY_X,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_C",                  CHATPAD_KEY_C,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_V",                  CHATPAD_KEY_V,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_B",                  CHATPAD_KEY_B,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_N",                  CHATPAD_KEY_N,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_M",                  CHATPAD_KEY_M,          CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_PERIOD",             CHATPAD_KEY_PERIOD,     CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_ENTER",              CHATPAD_KEY_ENTER,      CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_GREEN",              CHATPAD_KEY_GREEN,      CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_PEOPLE",             CHATPAD_KEY_PEOPLE,     CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_LEFT",               CHATPAD_KEY_LEFT,       CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_SPACEBAR",           CHATPAD_KEY_SPACEBAR,   CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_RIGHT",              CHATPAD_KEY_RIGHT,      CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_BACKSPACE",          CHATPAD_KEY_BACKSPACE,  CHATPAD_KEY_STRING },
   { "CHATPAD_KEY_ORANGE",             CHATPAD_KEY_ORANGE,     CHATPAD_KEY_STRING },

   // Strings corresponding to chatpad key modifiers.
   //
   // String count for this section:  4
   { "CHATPAD_SHIFT",                  CHATPAD_MODIFIER_SHIFT,    CHATPAD_MODIFIER_STRING },
   { "CHATPAD_GREEN",                  CHATPAD_MODIFIER_GREEN,    CHATPAD_MODIFIER_STRING },
   { "CHATPAD_ORANGE",                 CHATPAD_MODIFIER_ORANGE,   CHATPAD_MODIFIER_STRING },
   { "CHATPAD_PEOPLE",                 CHATPAD_MODIFIER_PEOPLE,   CHATPAD_MODIFIER_STRING },

   // Strings corresponding to XBox 360 controller buttons.
   //
   // String count for this section:  11
   { "CONTROLLER_BUTTON_LB",                 CONTROLLER_BUTTON_LB,                  CONTROLLER_BUTTON_STRING },
   { "CONTROLLER_BUTTON_RB",                 CONTROLLER_BUTTON_RB,                  CONTROLLER_BUTTON_STRING },
   { "CONTROLLER_BUTTON_BACK",               CONTROLLER_BUTTON_BACK ,               CONTROLLER_BUTTON_STRING },
   { "CONTROLLER_BUTTON_GUIDE",              CONTROLLER_BUTTON_GUIDE,               CONTROLLER_BUTTON_STRING },
   { "CONTROLLER_BUTTON_START",              CONTROLLER_BUTTON_START,               CONTROLLER_BUTTON_STRING },
   { "CONTROLLER_BUTTON_LEFT_STICK_CLICK",   CONTROLLER_BUTTON_LEFT_STICK_CLICK,    CONTROLLER_BUTTON_STRING },
   { "CONTROLLER_BUTTON_RIGHT_STICK_CLICK",  CONTROLLER_BUTTON_RIGHT_STICK_CLICK,   CONTROLLER_BUTTON_STRING },
   { "CONTROLLER_BUTTON_A",                  CONTROLLER_BUTTON_A,                   CONTROLLER_BUTTON_STRING },
   { "CONTROLLER_BUTTON_B",                  CONTROLLER_BUTTON_B,                   CONTROLLER_BUTTON_STRING },
   { "CONTROLLER_BUTTON_X",                  CONTROLLER_BUTTON_X,                   CONTROLLER_BUTTON_STRING },
   { "CONTROLLER_BUTTON_Y",                  CONTROLLER_BUTTON_Y,                   CONTROLLER_BUTTON_STRING },

   // Strings corresponding to XBox 360 controller modifiers.
   //
   // String count for this section:  1
   { "CONTROLLER_WINDOWS_MODE",              CONTROLLER_MODIFIER_WINDOWS_MODE,      CONTROLLER_MODIFIER_STRING },

   // Strings corresponding to simulated keyboard keys.
   //
   // String count for this section:  121
   { "KEYBOARD_KEY_A",                 KEYBOARD_KEY_A,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_B",                 KEYBOARD_KEY_B,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_C",                 KEYBOARD_KEY_C,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_D",                 KEYBOARD_KEY_D,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_E",                 KEYBOARD_KEY_E,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F",                 KEYBOARD_KEY_F,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_G",                 KEYBOARD_KEY_G,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_H",                 KEYBOARD_KEY_H,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_I",                 KEYBOARD_KEY_I,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_J",                 KEYBOARD_KEY_J,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_K",                 KEYBOARD_KEY_K,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_L",                 KEYBOARD_KEY_L,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_M",                 KEYBOARD_KEY_M,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_N",                 KEYBOARD_KEY_N,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_O",                 KEYBOARD_KEY_O,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_P",                 KEYBOARD_KEY_P,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_Q",                 KEYBOARD_KEY_Q,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_R",                 KEYBOARD_KEY_R,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_S",                 KEYBOARD_KEY_S,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_T",                 KEYBOARD_KEY_T,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_U",                 KEYBOARD_KEY_U,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_V",                 KEYBOARD_KEY_V,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_W",                 KEYBOARD_KEY_W,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_X",                 KEYBOARD_KEY_X,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_Y",                 KEYBOARD_KEY_Y,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_Z",                 KEYBOARD_KEY_Z,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_1",                 KEYBOARD_KEY_1,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_2",                 KEYBOARD_KEY_2,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_3",                 KEYBOARD_KEY_3,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_4",                 KEYBOARD_KEY_4,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_5",                 KEYBOARD_KEY_5,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_6",                 KEYBOARD_KEY_6,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_7",                 KEYBOARD_KEY_7,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_8",                 KEYBOARD_KEY_8,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_9",                 KEYBOARD_KEY_9,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_0",                 KEYBOARD_KEY_0,                  KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_ENTER",             KEYBOARD_KEY_ENTER,              KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_ESCAPE",            KEYBOARD_KEY_ESCAPE,             KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_BACKSPACE",         KEYBOARD_KEY_BACKSPACE,          KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_TAB",               KEYBOARD_KEY_TAB,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_SPACEBAR",          KEYBOARD_KEY_SPACEBAR,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_MINUS",             KEYBOARD_KEY_MINUS,              KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_EQUALS",            KEYBOARD_KEY_EQUALS,             KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_OPEN_BRACKET",      KEYBOARD_KEY_OPEN_BRACKET,       KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_CLOSE_BRACKET",     KEYBOARD_KEY_CLOSE_BRACKET,      KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_BACKSLASH",         KEYBOARD_KEY_BACKSLASH,          KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_SEMICOLON",         KEYBOARD_KEY_SEMICOLON,          KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_APOSTROPHE",        KEYBOARD_KEY_APOSTROPHE,         KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_GRAVE_ACCENT",      KEYBOARD_KEY_GRAVE_ACCENT,       KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_COMMA",             KEYBOARD_KEY_COMMA,              KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_PERIOD",            KEYBOARD_KEY_PERIOD,             KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_FORWARDSLASH",      KEYBOARD_KEY_FORWARDSLASH,       KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_CAPS_LOCK",         KEYBOARD_KEY_CAPS_LOCK,          KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F1",                KEYBOARD_KEY_F1,                 KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F2",                KEYBOARD_KEY_F2,                 KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F3",                KEYBOARD_KEY_F3,                 KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F4",                KEYBOARD_KEY_F4,                 KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F5",                KEYBOARD_KEY_F5,                 KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F6",                KEYBOARD_KEY_F6,                 KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F7",                KEYBOARD_KEY_F7,                 KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F8",                KEYBOARD_KEY_F8,                 KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F9",                KEYBOARD_KEY_F9,                 KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F10",               KEYBOARD_KEY_F10,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F11",               KEYBOARD_KEY_F11,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F12",               KEYBOARD_KEY_F12,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_PRINTSCREEN",       KEYBOARD_KEY_PRINTSCREEN,        KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_SCROLL_LOCK",       KEYBOARD_KEY_SCROLL_LOCK,        KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_PAUSE",             KEYBOARD_KEY_PAUSE,              KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_INSERT",            KEYBOARD_KEY_INSERT,             KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_HOME",              KEYBOARD_KEY_HOME,               KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_PAGEUP",            KEYBOARD_KEY_PAGEUP,             KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_DELETE",            KEYBOARD_KEY_DELETE,             KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_END",               KEYBOARD_KEY_END,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_PAGEDOWN",          KEYBOARD_KEY_PAGEDOWN,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_RIGHT",             KEYBOARD_KEY_RIGHT,              KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_LEFT",              KEYBOARD_KEY_LEFT,               KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_DOWN",              KEYBOARD_KEY_DOWN,               KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_UP",                KEYBOARD_KEY_UP,                 KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_NUM_LOCK",      KEYBOARD_KEY_KEYPAD_NUM_LOCK,       KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_FORWARDSLASH",  KEYBOARD_KEY_KEYPAD_FORWARDSLASH,   KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_ASTERISK",      KEYBOARD_KEY_KEYPAD_ASTERISK,       KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_MINUS",      KEYBOARD_KEY_KEYPAD_MINUS,             KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_PLUS",       KEYBOARD_KEY_KEYPAD_PLUS,              KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_ENTER",      KEYBOARD_KEY_KEYPAD_ENTER,             KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_1",          KEYBOARD_KEY_KEYPAD_1,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_2",          KEYBOARD_KEY_KEYPAD_2,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_3",          KEYBOARD_KEY_KEYPAD_3,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_4",          KEYBOARD_KEY_KEYPAD_4,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_5",          KEYBOARD_KEY_KEYPAD_5,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_6",          KEYBOARD_KEY_KEYPAD_6,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_7",          KEYBOARD_KEY_KEYPAD_7,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_8",          KEYBOARD_KEY_KEYPAD_8,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_9",          KEYBOARD_KEY_KEYPAD_9,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_0",          KEYBOARD_KEY_KEYPAD_0,           KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_KEYPAD_PERIOD",     KEYBOARD_KEY_KEYPAD_PERIOD,      KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F13",               KEYBOARD_KEY_F13,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F14",               KEYBOARD_KEY_F14,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F15",               KEYBOARD_KEY_F15,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F16",               KEYBOARD_KEY_F16,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F17",               KEYBOARD_KEY_F17,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F18",               KEYBOARD_KEY_F18,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F19",               KEYBOARD_KEY_F19,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F20",               KEYBOARD_KEY_F20,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F21",               KEYBOARD_KEY_F21,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F22",               KEYBOARD_KEY_F22,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F23",               KEYBOARD_KEY_F23,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_F24",               KEYBOARD_KEY_F24,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_EXECUTE",           KEYBOARD_KEY_EXECUTE,            KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_HELP",              KEYBOARD_KEY_HELP,               KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_MENU",              KEYBOARD_KEY_MENU,               KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_SELECT",            KEYBOARD_KEY_SELECT,             KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_STOP",              KEYBOARD_KEY_STOP,               KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_AGAIN",             KEYBOARD_KEY_AGAIN,              KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_UNDO",              KEYBOARD_KEY_UNDO,               KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_CUT",               KEYBOARD_KEY_CUT,                KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_COPY",              KEYBOARD_KEY_COPY,               KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_PASTE",             KEYBOARD_KEY_PASTE,              KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_FIND",              KEYBOARD_KEY_FIND,               KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_MUTE",              KEYBOARD_KEY_MUTE,               KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_VOLUME_UP",         KEYBOARD_KEY_VOLUME_UP,          KEYBOARD_KEY_STRING },
   { "KEYBOARD_KEY_VOLUME_DOWN",       KEYBOARD_KEY_VOLUME_DOWN,        KEYBOARD_KEY_STRING },

   // Strings corresponding to simulated keyboard key modifiers.
   //
   // String count for this section:  8
   { "KEYBOARD_LEFT_CONTROL",          KEYBOARD_MODIFIER_LEFT_CONTROL,  KEYBOARD_MODIFIER_STRING },
   { "KEYBOARD_LEFT_SHIFT",            KEYBOARD_MODIFIER_LEFT_SHIFT,    KEYBOARD_MODIFIER_STRING },
   { "KEYBOARD_LEFT_ALT",              KEYBOARD_MODIFIER_LEFT_ALT,      KEYBOARD_MODIFIER_STRING },
   { "KEYBOARD_LEFT_GUI",              KEYBOARD_MODIFIER_LEFT_GUI,      KEYBOARD_MODIFIER_STRING },
   { "KEYBOARD_RIGHT_CONTROL",         KEYBOARD_MODIFIER_RIGHT_CONTROL, KEYBOARD_MODIFIER_STRING },
   { "KEYBOARD_RIGHT_SHIFT",           KEYBOARD_MODIFIER_RIGHT_SHIFT,   KEYBOARD_MODIFIER_STRING },
   { "KEYBOARD_RIGHT_ALT",             KEYBOARD_MODIFIER_RIGHT_ALT,     KEYBOARD_MODIFIER_STRING },
   { "KEYBOARD_RIGHT_GUI",             KEYBOARD_MODIFIER_RIGHT_GUI,     KEYBOARD_MODIFIER_STRING },

   // Strings corresponding to simulated mouse actions.
   //
   // String count for this section:  5
   { "MOUSE_CLICK_LEFT",               MOUSE_CLICK_LEFT,    MOUSE_ACTION_STRING },
   { "MOUSE_CLICK_MIDDLE",             MOUSE_CLICK_MIDDLE,  MOUSE_ACTION_STRING },
   { "MOUSE_CLICK_RIGHT",              MOUSE_CLICK_RIGHT,   MOUSE_ACTION_STRING },
   { "MOUSE_WHEEL_UP",                 MOUSE_WHEEL_UP,      MOUSE_ACTION_STRING },
   { "MOUSE_WHEEL_DOWN",               MOUSE_WHEEL_DOWN,    MOUSE_ACTION_STRING },

   // Strings corresponding to special actions.  Chatpad keys
   // might be bound to these keys, for instance.
   //
   // String count for this section:  23
   { "SPECIAL_ACTION_LEFT_CTRL",             SPECIAL_ACTION_LEFT_CTRL,           SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_LEFT_CTRL_LATCH",       SPECIAL_ACTION_LEFT_CTRL_LATCH,     SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_LEFT_SHIFT",            SPECIAL_ACTION_LEFT_SHIFT,          SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_LEFT_SHIFT_LATCH",      SPECIAL_ACTION_LEFT_SHIFT_LATCH,    SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_LEFT_ALT",              SPECIAL_ACTION_LEFT_ALT,            SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_LEFT_ALT_LATCH",        SPECIAL_ACTION_LEFT_ALT_LATCH,      SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_LEFT_GUI",              SPECIAL_ACTION_LEFT_GUI,            SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_LEFT_GUI_LATCH",        SPECIAL_ACTION_LEFT_GUI_LATCH,      SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_RIGHT_CTRL",            SPECIAL_ACTION_RIGHT_CTRL,          SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_RIGHT_CTRL_LATCH",      SPECIAL_ACTION_RIGHT_CTRL_LATCH,    SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_RIGHT_SHIFT",           SPECIAL_ACTION_RIGHT_SHIFT,         SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_RIGHT_SHIFT_LATCH",     SPECIAL_ACTION_RIGHT_SHIFT_LATCH,   SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_RIGHT_ALT",             SPECIAL_ACTION_RIGHT_ALT,           SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_RIGHT_ALT_LATCH",       SPECIAL_ACTION_RIGHT_ALT_LATCH,     SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_RIGHT_GUI",             SPECIAL_ACTION_RIGHT_GUI,           SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_RIGHT_GUI_LATCH",       SPECIAL_ACTION_RIGHT_GUI_LATCH,     SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_ORANGE",                SPECIAL_ACTION_ORANGE,              SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_ORANGE_LATCH",          SPECIAL_ACTION_ORANGE_LATCH,        SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_GREEN",                 SPECIAL_ACTION_GREEN,               SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_GREEN_LATCH",           SPECIAL_ACTION_GREEN_LATCH,         SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_PEOPLE",                SPECIAL_ACTION_PEOPLE,              SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_PEOPLE_LATCH",          SPECIAL_ACTION_PEOPLE_LATCH,        SPECIAL_ACTION_STRING },
   { "SPECIAL_ACTION_TOGGLE_WINDOWS_MODE",   SPECIAL_ACTION_TOGGLE_WINDOWS_MODE, SPECIAL_ACTION_STRING }
}; // end ChatpadDriverConfigStringMapping configStringTable[]

// Number of strings in configStringTable.
//
// IMPORTANT NOTE:  This value must exactly match the string table length.
// If the number of strings in the string table is changed, then this value
// must be updated to match.  Note that the individual values are from the
// individual "string count" comments for configStringTable to make the math
// easier.
static const unsigned int uNumConfigStringTableStrings = 47 + 4 + 11 + 1 + 121 + 8 + 5 + 23;

// This is a token buffer used while parsing config files.  Putting it as a global
// variable is probably poor style, but it means that it will not need to be allocated
// every time a new config file line is parsed.
#define CONFIG_FILE_TOKEN_BUFFER_LENGTH   (2048+1)
static char configFileTokenBuffer[CONFIG_FILE_TOKEN_BUFFER_LENGTH];

//***
// Functions
//***

// Converts a null-terminated string to uppercase.
void StringToUpper(char* pString)
{
   if(NULL != pString)
   {
      char* pStringPtr = pString;
      while('\0' != (*pStringPtr))
      {
         if(((*pStringPtr) >= 'a') &&
            ((*pStringPtr) <= 'z'))
         {
            (*pStringPtr) = ((*pStringPtr) - 'a') + 'A';
         }
         pStringPtr++;
      }
   }
} // end StringToUpper

// Skips over whitespace pointed to by pSeekPtr and returns a pointer to a non-whitespace character.
char* SkipWhiteSpace(char* pSeekPtr)
{
   char* pReturnPtr = pSeekPtr;

   if(NULL != pReturnPtr)
   {
      while((' ' == (*pReturnPtr))    ||
            ('\t' == (*pReturnPtr)))
      {
         pReturnPtr++;
      }
   }

   return pReturnPtr;
} // end SkipWhiteSpace

// Skips backwards over whitespace pointed to by pSeekPtr and returns a pointer
// to a non-whitespace character.  The pStringStart parameter must point to
// the first character of the string to serve as a minimum value that pSeekPtr
// can hold.  If the string is entirely whitespace then this function will
// return a pointer to the first character in the string.
char* SkipEndingWhiteSpace(char* pStringStart, char* pSeekPtr)
{
   char* pReturnPtr = pSeekPtr;

   if((NULL != pStringStart) &&
      (NULL != pReturnPtr))
   {
      if(pReturnPtr < pStringStart)
      {
         LogMessage("Error, seek pointer is less than start pointer in SkipWhiteSpace.\n");
         pReturnPtr = pStringStart;
      }

      while((pReturnPtr > pStringStart) &&
            ((' ' == (*pReturnPtr))             ||
             ('\t' == (*pReturnPtr))))
      {
         pReturnPtr--;
      }
   }

   return pReturnPtr;
} // end SkipEndingWhiteSpace

// This function checks if pConfigLineLeftSide and pConfigLineRightSide
// represent a valid config variable assignment.  If so, the assignment is
// processed and data is written into the structure pointed to by
// pConfigStructure.  This function then return SUCCESS.
//
// IMPORTANT NOTE:  pConfigLineLeftSide and pConfigLineRightSide must
// point to null-terminated strings since no string lengths are provided
// to this function.
//
// If the left and right side strings do not represent a valid config
// variable assignment, or if an error occurs, then FAILURE is returned.
int HandleConfigVariableAssignment(
  ChatpadDriverConfigStructure*  pConfigStructure,
  char*                          pConfigLineLeftSide,
  char*                          pConfigLineRightSide)
{
   // By default, assume that this is not a valid assignment.
   int nRetValue = FAILURE;

   if((NULL != pConfigStructure)       &&
      (NULL != pConfigLineLeftSide)    &&
      (NULL != pConfigLineRightSide))
   {
      if(0 == strcmp("ENABLE_WINDOWS_MODE", pConfigLineLeftSide))
      {
         if(0 == strcmp("TRUE", pConfigLineRightSide))
         {
            pConfigStructure->bEnableWindowsMode = true;
            nRetValue = SUCCESS;
         }
         else if(0 == strcmp("FALSE", pConfigLineRightSide))
         {
            pConfigStructure->bEnableWindowsMode = false;
            nRetValue = SUCCESS;
         }
         else
         {
            LogMessage("Invalid variable value \"%s\", must be TRUE or FALSE.\n", pConfigLineRightSide);
         }
      } // end if(0 == strcmp("ENABLE_WINDOWS_MODE", pConfigLineLeftSide))
      else if(0 == strcmp("CAPS_LOCK_CONTROLS_SHIFT_INDICATOR", pConfigLineLeftSide))
      {
         if(0 == strcmp("TRUE", pConfigLineRightSide))
         {
            pConfigStructure->bCapsLockControlsShiftIndicator = true;
            nRetValue = SUCCESS;
         }
         else if(0 == strcmp("FALSE", pConfigLineRightSide))
         {
            pConfigStructure->bCapsLockControlsShiftIndicator = false;
            nRetValue = SUCCESS;
         }
         else
         {
            LogMessage("Invalid variable value \"%s\", must be TRUE or FALSE.\n", pConfigLineRightSide);
         }
      } // end else if(0 == strcmp("CAPS_LOCK_CONTROLS_SHIFT_INDICATOR", pConfigLineLeftSide))
      else if(0 == strcmp("ENABLE_WINDOWS_MODE_PEOPLE_KEY_INDICATOR", pConfigLineLeftSide))
      {
         if(0 == strcmp("TRUE", pConfigLineRightSide))
         {
            pConfigStructure->bEnableWindowsModePeopleKeyIndicator = true;
            nRetValue = SUCCESS;
         }
         else if(0 == strcmp("FALSE", pConfigLineRightSide))
         {
            pConfigStructure->bEnableWindowsModePeopleKeyIndicator = false;
            nRetValue = SUCCESS;
         }
         else
         {
            LogMessage("Invalid variable value \"%s\", must be TRUE or FALSE.\n", pConfigLineRightSide);
         }
      } // end else if(0 == strcmp("ENABLE_WINDOWS_MODE_PEOPLE_KEY_INDICATOR", pConfigLineLeftSide))
      else if(0 == strcmp("ENABLE_INGAME_REMAPPED_BUTTONS", pConfigLineLeftSide))
      {
         if(0 == strcmp("TRUE", pConfigLineRightSide))
         {
            pConfigStructure->bEnableIngameRemappedButtons = true;
            nRetValue = SUCCESS;
         }
         else if(0 == strcmp("FALSE", pConfigLineRightSide))
         {
            pConfigStructure->bEnableIngameRemappedButtons = false;
            nRetValue = SUCCESS;
         }
         else
         {
            LogMessage("Invalid variable value \"%s\", must be TRUE or FALSE.\n", pConfigLineRightSide);
         }
      } // end else if(0 == strcmp("ENABLE_INGAME_REMAPPED_BUTTONS", pConfigLineLeftSide))
      else if(0 == strcmp("THUMBSTICK_MOUSE_SENSITIVITY", pConfigLineLeftSide))
      {
         int nNewVirtualMouseSensitivity = atoi(pConfigLineRightSide);
         if((nNewVirtualMouseSensitivity >= MINIMUM_VIRTUAL_MOUSE_SENSITIVITY) &&
            (nNewVirtualMouseSensitivity <= MAXIMUM_VIRTUAL_MOUSE_SENSITIVITY))
         {
            pConfigStructure->nVirtualMouseSensitivity = nNewVirtualMouseSensitivity;
            nRetValue = SUCCESS;
         }
         else
         {
            LogMessage("Invalid virtual mouse sensitivity %u (valid range %u min, %u max).\n",
                       nNewVirtualMouseSensitivity,
                       MINIMUM_VIRTUAL_MOUSE_SENSITIVITY,
                       MAXIMUM_VIRTUAL_MOUSE_SENSITIVITY);
         }
      } // end else if(0 == strcmp("ENABLE_INGAME_REMAPPED_BUTTONS", pConfigLineLeftSide))
   } // end if function parameters are valid

   return nRetValue;
} // end HandleConfigVariableAssignment

// This function examines the string pointed to by *pSeekPtr, skips
// any initial whitespace, copies a token, skips any final whitespace,
// skips over any '+' character, and leaves *pSeekPtr pointing to
// a non-whitespace character that should be the start of the next token,
// or '\0' if no more tokens are available.
//
// This function returns SUCCESS on success and FAILURE on failure.
int CopyNextToken(
  char** pSeekPtr,
  char*  pTokenBuffer,
  int    nTokenBufferLength)
{
   int   nRetValue            = SUCCESS;
   // This pointer is used to copy data into pTokenBuffer.
   char* pTokenBufferPtr      = NULL;
   // This pointer points to the last valid location in pTokenBuffer to prevent overruns.
   char* pTokenBufferEndPtr   = NULL;

   if((NULL == pSeekPtr)         ||
      (NULL == (*pSeekPtr))      ||
      (NULL == pTokenBuffer)     ||
      (0 == nTokenBufferLength))
   {
      LogMessage("Invalid parameters provided to CopyNextToken.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      pTokenBufferPtr = pTokenBuffer;
      pTokenBufferEndPtr = pTokenBufferPtr + (nTokenBufferLength - 1);

      // Skip initial whitespace.
      (*pSeekPtr) = SkipWhiteSpace(*pSeekPtr);

      // Copy the token.
      while((' ' != (**pSeekPtr))    &&
            ('\t' != (**pSeekPtr))   &&
            ('+' != (**pSeekPtr))    &&
            ('\0' != (**pSeekPtr))   &&
            (pTokenBufferPtr < pTokenBufferEndPtr))
      {
         (*pTokenBufferPtr) = (**pSeekPtr);
         pTokenBufferPtr++;
         (*pSeekPtr)++;
      }

      // Null-terminate the token.
      (*pTokenBufferPtr) = '\0';

      // Skip ending whitespace.
      while((' ' == (**pSeekPtr))    ||
            ('\t' == (**pSeekPtr)))
      {
         (*pSeekPtr)++;
      }

      // Make sure that the next character is a '+' or '\0', or else return an error.
      if('+' == (**pSeekPtr))
      {
         (*pSeekPtr)++;
      }
      else
      {
         if('\0' != (**pSeekPtr))
         {
            nRetValue = FAILURE;
         }
      }
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end CopyNextToken

// This function looks up the string pTokenString in the configStringTable
// structure.  If the string is found, then it writes the corresponding data to
// the structure pointed to by pTokenMapping.
//
// IMPORTANT NOTE:  This function copies a string pointer into the structure
// pointed to by pTokenMapping.  Since this pointer points to a constant
// string in global storage, calling code should NOT attempt to free it.
//
// If the string is not found, or if an error occurs, then this function
// returns FAILURE.  Otherwise, this function returns SUCCESS.
int GetTokenMapping(
  char*                             pTokenString,
  ChatpadDriverConfigStringMapping* pTokenMapping)
{
   int   nRetValue   = SUCCESS;

   if((NULL == pTokenString)     ||
      (NULL == pTokenMapping))
   {
      LogMessage("Invalid parameters provided to GetTokenMapping.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      bool bMatchFound = false;

      for(unsigned int i = 0;
          ((i < uNumConfigStringTableStrings) &&
           (false == bMatchFound));
          i++)
      {
         if(0 == strcmp(pTokenString, configStringTable[i].pString))
         {
            bMatchFound = true;
            pTokenMapping->pString     = configStringTable[i].pString;
            pTokenMapping->uConstant   = configStringTable[i].uConstant;
            pTokenMapping->stringType  = configStringTable[i].stringType;
         } // end if(0 == strcmp(pTokenString, configStringTable[i]))
      } // end looping through configStringTable

      if(false == bMatchFound)
      {
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
}

// This function handles pConfigLineLeftSide and pConfigLineRightSide as a
// config mapping line.  The line is parsed and data is written into the
// structure pointed to by pConfigStructure.  This function then return
// SUCCESS.
//
// IMPORTANT NOTE:  pConfigLineLeftSide and pConfigLineRightSide must
// point to null-terminated strings since no string lengths are provided
// to this function.
//
// If the left and right side strings do not represent a valid config
// mapping line, or if another error occurs, then FAILURE is returned.
int HandleConfigMappingLine(
  ChatpadDriverConfigStructure*  pConfigStructure,
  char*                          pConfigLineLeftSide,
  char*                          pConfigLineRightSide)
{
   int   nRetValue   = SUCCESS;
   char* pSeekPtr    = NULL;
   ChatpadDriverConfigStringMapping sourceTokenMapping;
   ChatpadDriverConfigStringMapping actionTokenMapping;
   ChatpadDriverConfigStringMapping modifierTokenMapping;
   ChatpadDriverActionBinding       actionBinding;

   if((NULL == pConfigStructure)       ||
      (NULL == pConfigLineLeftSide)    ||
      (NULL == pConfigLineRightSide))
   {
      LogMessage("Invalid parameters provided to HandleConfigMappingLine.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      // Parse tokens in the left side string first.
      pSeekPtr = pConfigLineLeftSide;

      // Copy the first token.
      configFileTokenBuffer[0] = '\0';
      nRetValue = CopyNextToken(&pSeekPtr, configFileTokenBuffer, CONFIG_FILE_TOKEN_BUFFER_LENGTH);
   }

   if(SUCCESS == nRetValue)
   {
      actionBinding.uSourceActionModifiers   = 0;
      actionBinding.bindingType              = BINDING_NONE;
      actionBinding.uActionModifiers         = 0;
      actionBinding.uActionCode              = 0;
      sourceTokenMapping.pString    = NULL;
      sourceTokenMapping.uConstant  = 0;
      sourceTokenMapping.stringType = INVALID_STRING;
      nRetValue = GetTokenMapping(configFileTokenBuffer, &sourceTokenMapping);
      if(SUCCESS != nRetValue)
      {
         LogMessage("Error, unknown config string \"%s\".\nSee README.TXT for valid strings.\n",
                    configFileTokenBuffer);
      }
   }

   if(SUCCESS == nRetValue)
   {
      // The first token must correspond to either a chatpad key or a controller button.
      //
      // Note that there is probably a more elegant way to do this than repeating so much
      // code, but repeating the code is easier for now.
      if(CHATPAD_KEY_STRING == sourceTokenMapping.stringType)
      {
         // Since this is a chatpad key binding, check for modifiers that are
         // compatible with chatpad keys.

         // Note that eventually, this code might be refactored to consolidate some of
         // the similar code below that loops through modifiers.
         while(('\0' != (*pSeekPtr))  &&
               (SUCCESS == nRetValue))
         {
            configFileTokenBuffer[0] = '\0';
            nRetValue = CopyNextToken(&pSeekPtr, configFileTokenBuffer, CONFIG_FILE_TOKEN_BUFFER_LENGTH);
            if(SUCCESS == nRetValue)
            {
               modifierTokenMapping.pString    = NULL;
               modifierTokenMapping.uConstant  = 0;
               modifierTokenMapping.stringType = INVALID_STRING;
               nRetValue = GetTokenMapping(configFileTokenBuffer, &modifierTokenMapping);
               // Check modifier compatibility.
               if(CHATPAD_MODIFIER_STRING == modifierTokenMapping.stringType)
               {
                  // Add the modifier to the current action binding.
                  actionBinding.uSourceActionModifiers |= modifierTokenMapping.uConstant;
               }
               else
               {
                  LogMessage("Error, expected chatpad key modifier but found \"%s\".\n",
                             configFileTokenBuffer);
                  nRetValue = FAILURE;
               }
            } // end if(SUCCESS == nRetValue)
         } // end while parsing through source action modifiers

         if(SUCCESS == nRetValue)
         {
            bool                          bBindingAlreadyExists   = false;
            ChatpadKeyMapping*            pKeyMapping             = NULL;
            ChatpadDriverActionBinding*   pNewBinding             = NULL;

            pKeyMapping = &(pConfigStructure->keyMappings[sourceTokenMapping.uConstant]);

            if(NULL != pKeyMapping->pBindings)
            {
               // Check whether there is already a binding for the source action.
               for(int nBindingIndex = 0; nBindingIndex < (pKeyMapping->nNumBindings); nBindingIndex++)
               {
                  if(pKeyMapping->pBindings[nBindingIndex].uSourceActionModifiers ==
                     actionBinding.uSourceActionModifiers)
                  {
                     bBindingAlreadyExists = true;
                     pNewBinding = &(pKeyMapping->pBindings[nBindingIndex]);
                     break;
                  }
               }
            }

            if(false == bBindingAlreadyExists)
            {
               ChatpadDriverActionBinding* pReallocatedBindings = NULL;

               // An intermediate pointer is used here since realloc could
               // fail and cause the original memory in pKeyMapping->pBindings
               // to leak.
               pReallocatedBindings =
                 static_cast<ChatpadDriverActionBinding*>(realloc(
                   pKeyMapping->pBindings,
                   ((pKeyMapping->nNumBindings + 1) * sizeof(ChatpadDriverActionBinding))));
               if(NULL != pReallocatedBindings)
               {
                  pNewBinding = &(pReallocatedBindings[pKeyMapping->nNumBindings]);

                  pKeyMapping->nNumBindings++;
                  pKeyMapping->pBindings = pReallocatedBindings;
               } // end if(NULL != pReallocatedBindings)
               else
               {
                  LogMessage("Error allocating memory in HandleConfigMappingLine.\n");
                  nRetValue = FAILURE;
               }
            } // end if(false == bBindingAlreadyExists)
            else
            {
               LogMessage("Warning, binding already exists for \"%s\".\n", pConfigLineLeftSide);
            }

            // If successful thus far, parse the right side string.
            if(SUCCESS == nRetValue)
            {
               pSeekPtr = pConfigLineRightSide;

               configFileTokenBuffer[0] = '\0';
               nRetValue = CopyNextToken(&pSeekPtr, configFileTokenBuffer, CONFIG_FILE_TOKEN_BUFFER_LENGTH);
            } // end if(SUCCESS == nRetValue)

            if(SUCCESS == nRetValue)
            {
               actionTokenMapping.pString    = NULL;
               actionTokenMapping.uConstant  = 0;
               actionTokenMapping.stringType = INVALID_STRING;
               nRetValue = GetTokenMapping(configFileTokenBuffer, &actionTokenMapping);
               if(SUCCESS != nRetValue)
               {
                  LogMessage("Error, unknown config string \"%s\".\nSee README.TXT for valid strings.\n",
                             configFileTokenBuffer);
               }
            } // end if(SUCCESS == nRetValue)

            if(SUCCESS == nRetValue)
            {
               if(KEYBOARD_KEY_STRING == actionTokenMapping.stringType)
               {
                  actionBinding.bindingType  = BINDING_SIMULATED_KEY;
                  actionBinding.uActionCode  = actionTokenMapping.uConstant;

                  // Since this is a binding to a simulated key, check for
                  // modifiers that are compatible with simulated keys.

                  while(('\0' != (*pSeekPtr))  &&
                        (SUCCESS == nRetValue))
                  {
                     configFileTokenBuffer[0] = '\0';
                     nRetValue = CopyNextToken(&pSeekPtr, configFileTokenBuffer, CONFIG_FILE_TOKEN_BUFFER_LENGTH);
                     if(SUCCESS == nRetValue)
                     {
                        modifierTokenMapping.pString    = NULL;
                        modifierTokenMapping.uConstant  = 0;
                        modifierTokenMapping.stringType = INVALID_STRING;
                        nRetValue = GetTokenMapping(configFileTokenBuffer, &modifierTokenMapping);
                        // Check modifier compatibility.
                        if(KEYBOARD_MODIFIER_STRING == modifierTokenMapping.stringType)
                        {
                           // Add the modifier to the current action binding.
                           actionBinding.uActionModifiers |= modifierTokenMapping.uConstant;
                        }
                        else
                        {
                           LogMessage("Error, expected keyboard key modifier but found \"%s\".\n",
                                      configFileTokenBuffer);
                           nRetValue = FAILURE;
                        }
                     } // end if(SUCCESS == nRetValue)
                  } // end while parsing through source action modifiers
               } // end if(KEYBOARD_KEY_STRING == actionTokenMapping.stringType)
               else if(MOUSE_ACTION_STRING == actionTokenMapping.stringType)
               {
                  actionBinding.bindingType  = BINDING_SIMULATED_MOUSE_ACTION;
                  actionBinding.uActionCode  = actionTokenMapping.uConstant;

                  if('\0' != (*pSeekPtr))
                  {
                     LogMessage("Error, no modifiers are supported for mouse actions.\n");
                     nRetValue = FAILURE;
                  }
               } // end else if(MOUSE_ACTION_STRING == actionTokenMapping.stringType)
               else if(SPECIAL_ACTION_STRING == actionTokenMapping.stringType)
               {
                  actionBinding.bindingType  = BINDING_SPECIAL;
                  actionBinding.uActionCode  = actionTokenMapping.uConstant;

                  if('\0' != (*pSeekPtr))
                  {
                     LogMessage("Error, no modifiers are supported for special actions.\n");
                     nRetValue = FAILURE;
                  }
               } // end else if(SPECIAL_ACTION_STRING == actionTokenMapping.stringType)
               else
               {
                  LogMessage("\"%s\" is not a keyboard key, mouse, or special binding.\n", configFileTokenBuffer);
                  nRetValue = FAILURE;
               }
            } // end if(SUCCESS == nRetValue)

            if((SUCCESS == nRetValue) &&
               (NULL != pNewBinding))
            {
               // Copy the binding data.
               pNewBinding->uSourceActionModifiers = actionBinding.uSourceActionModifiers;
               pNewBinding->bindingType            = actionBinding.bindingType;
               pNewBinding->uActionModifiers       = actionBinding.uActionModifiers;
               pNewBinding->uActionCode            = actionBinding.uActionCode;
/*
               LogMessage("New binding:  0x%x, 0x%x, 0x%x, 0x%x\n",
                          pNewBinding->uSourceActionModifiers,
                          pNewBinding->bindingType,
                          pNewBinding->uActionModifiers,
                          pNewBinding->uActionCode);
*/
            }
         } // end if(SUCCESS == nRetValue)
      } // end if(CHATPAD_KEY_STRING == sourceTokenMapping.stringType)
      else if(CONTROLLER_BUTTON_STRING == sourceTokenMapping.stringType)
      {
         // Since this is a controller button binding, check for modifiers that are
         // compatible with controller buttons.

         while(('\0' != (*pSeekPtr))  &&
               (SUCCESS == nRetValue))
         {
            configFileTokenBuffer[0] = '\0';
            nRetValue = CopyNextToken(&pSeekPtr, configFileTokenBuffer, CONFIG_FILE_TOKEN_BUFFER_LENGTH);
            if(SUCCESS == nRetValue)
            {
               modifierTokenMapping.pString    = NULL;
               modifierTokenMapping.uConstant  = 0;
               modifierTokenMapping.stringType = INVALID_STRING;
               nRetValue = GetTokenMapping(configFileTokenBuffer, &modifierTokenMapping);
               // Check modifier compatibility.
               if(CONTROLLER_MODIFIER_STRING == modifierTokenMapping.stringType)
               {
                  // Add the modifier to the current action binding.
                  actionBinding.uSourceActionModifiers |= modifierTokenMapping.uConstant;
               }
               else
               {
                  LogMessage("Error, expected controller button modifier but found \"%s\".\n",
                             configFileTokenBuffer);
                  nRetValue = FAILURE;
               }
            } // end if(SUCCESS == nRetValue)
         } // end while parsing through source action modifiers

         if(SUCCESS == nRetValue)
         {
            bool                          bBindingAlreadyExists   = false;
            ControllerButtonMapping*      pButtonMapping          = NULL;
            ChatpadDriverActionBinding*   pNewBinding             = NULL;

            pButtonMapping = &(pConfigStructure->buttonMappings[sourceTokenMapping.uConstant]);

            if(NULL != pButtonMapping->pBindings)
            {
               // Check whether there is already a binding for the source action.
               for(int nBindingIndex = 0; nBindingIndex < (pButtonMapping->nNumBindings); nBindingIndex++)
               {
                  if(pButtonMapping->pBindings[nBindingIndex].uSourceActionModifiers ==
                     actionBinding.uSourceActionModifiers)
                  {
                     bBindingAlreadyExists = true;
                     pNewBinding = &(pButtonMapping->pBindings[nBindingIndex]);
                     break;
                  }
               }
            }

            if(false == bBindingAlreadyExists)
            {
               ChatpadDriverActionBinding* pReallocatedBindings = NULL;

               // An intermediate pointer is used here since realloc could
               // fail and cause the original memory in pButtonMapping->pBindings
               // to leak.
               pReallocatedBindings =
                 static_cast<ChatpadDriverActionBinding*>(realloc(
                   pButtonMapping->pBindings,
                   ((pButtonMapping->nNumBindings + 1) * sizeof(ChatpadDriverActionBinding))));
               if(NULL != pReallocatedBindings)
               {
                  pNewBinding = &(pReallocatedBindings[pButtonMapping->nNumBindings]);

                  pButtonMapping->nNumBindings++;
                  pButtonMapping->pBindings = pReallocatedBindings;
               } // end if(NULL != pReallocatedBindings)
               else
               {
                  LogMessage("Error allocating memory in HandleConfigMappingLine.\n");
                  nRetValue = FAILURE;
               }
            } // end if(false == bBindingAlreadyExists)
            else
            {
               LogMessage("Warning, binding already exists for \"%s\".\n", pConfigLineLeftSide);
            }

            // If successful thus far, parse the right side string.
            if(SUCCESS == nRetValue)
            {
               pSeekPtr = pConfigLineRightSide;

               configFileTokenBuffer[0] = '\0';
               nRetValue = CopyNextToken(&pSeekPtr, configFileTokenBuffer, CONFIG_FILE_TOKEN_BUFFER_LENGTH);
            } // end if(SUCCESS == nRetValue)

            if(SUCCESS == nRetValue)
            {
               actionTokenMapping.pString    = NULL;
               actionTokenMapping.uConstant  = 0;
               actionTokenMapping.stringType = INVALID_STRING;
               nRetValue = GetTokenMapping(configFileTokenBuffer, &actionTokenMapping);
               if(SUCCESS != nRetValue)
               {
                  LogMessage("Error, unknown config string \"%s\".\nSee README.TXT for valid strings.\n",
                             configFileTokenBuffer);
               }
            } // end if(SUCCESS == nRetValue)

            if(SUCCESS == nRetValue)
            {
               if(CONTROLLER_BUTTON_STRING == actionTokenMapping.stringType)
               {
                  actionBinding.bindingType  = BINDING_CONTROLLER;
                  actionBinding.uActionCode  = actionTokenMapping.uConstant;

                  if('\0' != (*pSeekPtr))
                  {
                     LogMessage("Error, no modifiers are supported for output controller actions.\n");
                     nRetValue = FAILURE;
                  }
               } // end if(CONTROLLER_BUTTON_STRING == actionTokenMapping.stringType)
               else if(KEYBOARD_KEY_STRING == actionTokenMapping.stringType)
               {
                  actionBinding.bindingType  = BINDING_SIMULATED_KEY;
                  actionBinding.uActionCode  = actionTokenMapping.uConstant;

                  // Since this is a binding to a simulated key, check for
                  // modifiers that are compatible with simulated keys.

                  while(('\0' != (*pSeekPtr))  &&
                        (SUCCESS == nRetValue))
                  {
                     configFileTokenBuffer[0] = '\0';
                     nRetValue = CopyNextToken(&pSeekPtr, configFileTokenBuffer, CONFIG_FILE_TOKEN_BUFFER_LENGTH);
                     if(SUCCESS == nRetValue)
                     {
                        modifierTokenMapping.pString    = NULL;
                        modifierTokenMapping.uConstant  = 0;
                        modifierTokenMapping.stringType = INVALID_STRING;
                        nRetValue = GetTokenMapping(configFileTokenBuffer, &modifierTokenMapping);
                        // Check modifier compatibility.
                        if(KEYBOARD_MODIFIER_STRING == modifierTokenMapping.stringType)
                        {
                           // Add the modifier to the current action binding.
                           actionBinding.uActionModifiers |= modifierTokenMapping.uConstant;
                        }
                        else
                        {
                           LogMessage("Error, expected keyboard key modifier but found \"%s\".\n",
                                      configFileTokenBuffer);
                           nRetValue = FAILURE;
                        }
                     } // end if(SUCCESS == nRetValue)
                  } // end while parsing through source action modifiers
               } // end if(KEYBOARD_KEY_STRING == actionTokenMapping.stringType)
               else if(MOUSE_ACTION_STRING == actionTokenMapping.stringType)
               {
                  actionBinding.bindingType  = BINDING_SIMULATED_MOUSE_ACTION;
                  actionBinding.uActionCode  = actionTokenMapping.uConstant;

                  if('\0' != (*pSeekPtr))
                  {
                     LogMessage("Error, no modifiers are supported for mouse actions.\n");
                     nRetValue = FAILURE;
                  }
               } // end else if(MOUSE_ACTION_STRING == actionTokenMapping.stringType)
               else if(SPECIAL_ACTION_STRING == actionTokenMapping.stringType)
               {
                  actionBinding.bindingType  = BINDING_SPECIAL;
                  actionBinding.uActionCode  = actionTokenMapping.uConstant;

                  if('\0' != (*pSeekPtr))
                  {
                     LogMessage("Error, no modifiers are supported for special actions.\n");
                     nRetValue = FAILURE;
                  }
               } // end else if(SPECIAL_ACTION_STRING == actionTokenMapping.stringType)
               else
               {
                  LogMessage("\"%s\" is not a controller, keyboard key, mouse, or special binding.\n", configFileTokenBuffer);
                  nRetValue = FAILURE;
               }
            } // end if(SUCCESS == nRetValue)

            if((SUCCESS == nRetValue) &&
               (NULL != pNewBinding))
            {
               // Copy the binding data.
               pNewBinding->uSourceActionModifiers = actionBinding.uSourceActionModifiers;
               pNewBinding->bindingType            = actionBinding.bindingType;
               pNewBinding->uActionModifiers       = actionBinding.uActionModifiers;
               pNewBinding->uActionCode            = actionBinding.uActionCode;
/*
               LogMessage("New binding:  0x%x, 0x%x, 0x%x, 0x%x\n",
                          pNewBinding->uSourceActionModifiers,
                          pNewBinding->bindingType,
                          pNewBinding->uActionModifiers,
                          pNewBinding->uActionCode);
*/
            }
         } // end if(SUCCESS == nRetValue)
      } // end else if(CONTROLLER_BUTTON_STRING == sourceTokenMapping.stringType)
      else
      {
         LogMessage("\"%s\" is neither a chatpad key nor a controller button.\n", configFileTokenBuffer);
         nRetValue = FAILURE;
      }
   } // end if(SUCCESS == nRetValue)

   return nRetValue;
} // end HandleConfigMappingLine

// Initializes a config data structure to default values.
int InitDefaultDriverConfigDataValues(ChatpadDriverConfigStructure* pConfigStructure)
{
   int nRetValue = SUCCESS;

   if(NULL != pConfigStructure)
   {
      pConfigStructure->bEnableWindowsMode = true;
      pConfigStructure->bCapsLockControlsShiftIndicator = false;
      pConfigStructure->bEnableWindowsModePeopleKeyIndicator = true;
      pConfigStructure->bEnableIngameRemappedButtons = true;
      pConfigStructure->nVirtualMouseSensitivity = DEFAULT_VIRTUAL_MOUSE_SENSITIVITY;

      for(int i = 0; i < CHATPAD_NUM_KEYS; i++)
      {
         pConfigStructure->keyMappings[i].nNumBindings = 0;
         pConfigStructure->keyMappings[i].pBindings = NULL;
      }

      for(int i = 0; i < CONTROLLER_NUM_BUTTONS; i++)
      {
         pConfigStructure->buttonMappings[i].nNumBindings = 0;
         pConfigStructure->buttonMappings[i].pBindings = NULL;
      }
   }
   else
   {
      nRetValue = FAILURE;
   }

   return nRetValue;
} // end InitDefaultDriverConfigDataValues

// This function opens a config file, initializes pConfigStructure to a default
// state, parses the file, writes data into pConfigStructure as needed, and
// closes the file.
int LoadConfigFile(
  char*                          pFilename,
  ChatpadDriverConfigStructure*  pConfigStructure)
{
   int      nRetValue                  = SUCCESS;
   bool     bFinished                  = false;
   FILE*    pReadFile                  = NULL;
   // Allow for 2048 characters in a command string plus a null terminator.
   int      nConfigLineBufferSize      = 2048 + 1;
   char*    pConfigLine                = NULL;
   char*    pConfigLineLeftSide        = NULL;
   char*    pConfigLineRightSide       = NULL;
   char*    pReadResult                = NULL;
   char*    pSeekPtr                   = NULL;
   char*    pConfigLineLeftPtr         = NULL;
   char*    pConfigLineRightPtr        = NULL;

   if((SUCCESS == nRetValue) &&
      ((NULL == pFilename)             ||
       (NULL == pConfigStructure)))
   {
      LogMessage("Invalid parameters provided to LoadConfigFile.\n");
      nRetValue = FAILURE;
   }

   if(SUCCESS == nRetValue)
   {
      pReadFile = fopen(pFilename, "rb");
      if(NULL == pReadFile)
      {
         LogMessage("Failed to open config file \"%s\".\n", pFilename);
         nRetValue = FAILURE;
      }
   }

   if(SUCCESS == nRetValue)
   {
      nRetValue = InitDefaultDriverConfigDataValues(pConfigStructure);
   }

   if(SUCCESS == nRetValue)
   {
      pConfigLine = static_cast<char*>(malloc(nConfigLineBufferSize));
      pConfigLineLeftSide = static_cast<char*>(malloc(nConfigLineBufferSize));
      pConfigLineRightSide = static_cast<char*>(malloc(nConfigLineBufferSize));
      if((NULL == pConfigLine)            ||
         (NULL == pConfigLineLeftSide)    ||
         (NULL == pConfigLineRightSide))
      {
         LogMessage("Failed to allocate memory for config line buffers.\n");
         nRetValue = FAILURE;
      }
   }

   while((SUCCESS == nRetValue) &&
         (false == bFinished))
   {
      pConfigLine[0] = '\0';
      pReadResult = fgets(pConfigLine, nConfigLineBufferSize, pReadFile);

      if(NULL != pReadResult)
      {
         pSeekPtr = pConfigLine;

         // Skip initial whitespace.
         pSeekPtr = SkipWhiteSpace(pSeekPtr);

         // Ignore the line if it is empty or starts with '#'.
         if(('#' == (*pSeekPtr))    ||
            ('\n' == (*pSeekPtr))   ||
            ('\r' == (*pSeekPtr))   ||
            ('\0' == (*pSeekPtr)))
         {
            continue;
         }

         // Copy the left side of the '=' sign that should exist into a buffer.
         pConfigLineLeftSide[0] = '\0';
         pConfigLineLeftPtr = pConfigLineLeftSide;
         while(('=' != (*pSeekPtr))    &&
               ('\0' != (*pSeekPtr)))
         {
            (*pConfigLineLeftPtr) = (*pSeekPtr);
            pConfigLineLeftPtr++;
            pSeekPtr++;
         }

         if('=' == (*pSeekPtr))
         {
            // Skip backwards over any ending whitespace.
            if(pConfigLineLeftPtr > pConfigLineLeftSide)
            {
               pConfigLineLeftPtr--;
               pConfigLineLeftPtr = SkipEndingWhiteSpace(pConfigLineLeftSide, pConfigLineLeftPtr);
               pConfigLineLeftPtr++;
            }
            // Null-terminate the left-side string.
            (*pConfigLineLeftPtr) = '\0';
         }
         else
         {
            LogMessage("Error, missing '=' in the following line:\n%s", pConfigLine);
            nRetValue = FAILURE;
            continue;
         }

         //***
         // Copy the right side of the '=' sign into a buffer.  If this point
         // in the code was reached, then pSeekPtr currently points to '='.
         //***
         pSeekPtr++;

         // Now that we are past the '=' character, skip any initial whitespace.
         pSeekPtr = SkipWhiteSpace(pSeekPtr);

         pConfigLineRightSide[0] = '\0';
         pConfigLineRightPtr = pConfigLineRightSide;
         while(('\n' != (*pSeekPtr))    &&
               ('\r' != (*pSeekPtr))    &&
               ('\0' != (*pSeekPtr)))
         {
            (*pConfigLineRightPtr) = (*pSeekPtr);
            pConfigLineRightPtr++;
            pSeekPtr++;
         }

         // Skip backwards over any ending whitespace.
         if(pConfigLineRightPtr > pConfigLineRightSide)
         {
            pConfigLineRightPtr--;
            pConfigLineRightPtr = SkipEndingWhiteSpace(pConfigLineRightSide, pConfigLineRightPtr);
            pConfigLineRightPtr++;
         }
         // Null-terminate the right-side string.
         (*pConfigLineRightPtr) = '\0';

         // Convert both left and right sides to all-uppercase.
         StringToUpper(pConfigLineLeftSide);
         StringToUpper(pConfigLineRightSide);

         // Try to handle this as a config variable assignment.  If there is
         // a syntax error or the left side does not name a config variable,
         // then HandleConfigVariableAssignment will return FAILURE.
         // We can then try to handle it as a key/control mapping instead.
         if(FAILURE == HandleConfigVariableAssignment(pConfigStructure, pConfigLineLeftSide, pConfigLineRightSide))
         {
            nRetValue = HandleConfigMappingLine(pConfigStructure, pConfigLineLeftSide, pConfigLineRightSide);
            if(SUCCESS != nRetValue)
            {
               LogMessage("There was an error parsing the following config mapping line:\n%s", pConfigLine);
            }
         }
      } // end if(NULL != pReadResult)
      else
      {
         // This case indicates an error or the end of the file.
         bFinished = true;
         if(0 != ferror(pReadFile))
         {
            LogMessage("An error occurred while reading the config file.\n");
            nRetValue = FAILURE;
         }
      }
   } // end while successful and not finished

   if(NULL == pConfigLine)
   {
      free(pConfigLine);
      pConfigLine = NULL;
   }

   if(NULL == pConfigLineLeftSide)
   {
      free(pConfigLineLeftSide);
      pConfigLineLeftSide = NULL;
   }

   if(NULL == pConfigLineRightSide)
   {
      free(pConfigLineRightSide);
      pConfigLineRightSide = NULL;
   }

   if(NULL != pReadFile)
   {
      fclose(pReadFile);
      pReadFile = NULL;
   }

   if(SUCCESS != nRetValue)
   {
      LogMessage("There was an error while attempting to load config file \"%s\".\n", pFilename);
   }

   return nRetValue;
} // end LoadConfigFile

