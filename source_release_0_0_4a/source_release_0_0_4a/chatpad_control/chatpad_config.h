//
// Chatpad utility code that works with a KMDF USB filter driver, by
//   GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
// This particular file contains header information for structures and
// functions that are used to load configuration information from a file
// and use it to map keys and buttons.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//

#ifndef CHATPAD_CONFIG_H
#define CHATPAD_CONFIG_H

#include "chatpad_control.h"

// General constants.
enum ChatpadConfigConstants
{
   // Default virtual mouse sensitivity.
   DEFAULT_VIRTUAL_MOUSE_SENSITIVITY            = 8,
   // Minimum virtual mouse sensitivity.
   MINIMUM_VIRTUAL_MOUSE_SENSITIVITY            = 1,
   // Maximum virtual mouse sensitivity.
   MAXIMUM_VIRTUAL_MOUSE_SENSITIVITY            = 20
};

// Action types.
// TODO NEXT delete if unused, but keep this around until control mapping is done
typedef enum ConfigStringType
{
   INVALID_STRING                = 0,
   CHATPAD_KEY_STRING,
   CHATPAD_MODIFIER_STRING,
   CONTROLLER_BUTTON_STRING,
   CONTROLLER_MODIFIER_STRING,
   KEYBOARD_KEY_STRING,
   KEYBOARD_MODIFIER_STRING,
   MOUSE_ACTION_STRING,
   SPECIAL_ACTION_STRING,
};

// Structure used for a global string table to map strings like
// "CHATPAD_KEY_A" to the corresponding numerical constants.
typedef struct ChatpadDriverConfigStringMapping_t
{
   // Null-terminated string.
   char*             pString;
   // Corresponding numerical constant.
   unsigned int      uConstant;
   // Config string type, i.e. chatpad key, simulated mouse click, etc.
   ConfigStringType  stringType;
} ChatpadDriverConfigStringMapping;

//***
// Data structures
//***

// Structure to specify a bound action, such as a simulated key press.
typedef struct ChatpadDriverActionBinding_t
{
   // Modifiers to the source action (i.e. shift, or orange).  If there is more
   // than one modifier, they are combined with binary OR.
   unsigned int   uSourceActionModifiers;

   // Type of binding (simulated key, simulated mouse click, etc.).
   BindingType    bindingType;
   // Modifiers to the bound action (i.e. shift, or alt).  If there is more
   // than one modifier, they are combined with binary OR.
   unsigned int   uActionModifiers;
   // Numerical action code, i.e. KEYBOARD_KEY_B.
   unsigned int   uActionCode;
} ChatpadDriverActionBinding;

// Structure to contain information on the mapping(s) for a particular chatpad key.
typedef struct ChatpadKeyMapping_t
{
   // Number of bindings that are applied to this key.  If nNumBindings is 0,
   // then pressing this key will have no effect.
   // TODO NEXT test to make sure keys have no effect if not bound
   int                           nNumBindings;
   // Dynamically allocated array of length nNumBindings containing the action
   // bindings for the key.  This can be NULL if no bindings exist for the key.
   ChatpadDriverActionBinding*   pBindings;
} ChatpadKeyMapping;

// Structure to contain information on the mapping(s) for a particular controller button.
typedef struct ControllerButtonMapping_t
{
   // Number of bindings that are applied to this button.  If nNumBindings is 0,
   // then pressing this button will have no effect.
   // TODO NEXT test to make sure it has no effect if not bound...may want to reconsider
   int                           nNumBindings;
   // Dynamically allocated array of length nNumBindings containing the action
   // bindings for the button.  This can be NULL if no bindings exist for the button.
   ChatpadDriverActionBinding*   pBindings;
} ControllerButtonMapping;

// Structure to contain driver config data such as key mappings.
typedef struct ChatpadDriverConfigStructure_t
{
   // Whether Windows Mode is enabled in the current configuration.
   // If it is not enabled, then it cannot be activated.
   bool                       bEnableWindowsMode;

   // Whether the "people" chatpad key light is used to indicate that Windows
   // Mode is active.
   bool                       bEnableWindowsModePeopleKeyIndicator;

   // Whether the caps lock state is used to control the chatpad shift
   // key indicator light (if true, the left shift latched special action
   // will NOT control the same light).
   bool                       bCapsLockControlsShiftIndicator;

   // Whether mappings of controller buttons to other controller buttons
   // are allowed.  If they are allowed, the remapping will only apply
   // when Windows Mode is not active.
   bool                       bEnableIngameRemappedButtons;

   // Virtual mouse sensitivity.  The higher this number is, the faster the
   // mouse will move when the thumbsticks are moved in Windows mode.
   int                        nVirtualMouseSensitivity;

   // Mapping structures for each chatpad key and controller button.
   // The keyMappings array is indexed by CHATPAD_KEY_ constants, and
   // the buttonMappings array is indexed by the CONTROLLER_BUTTON_
   // constants.
   ChatpadKeyMapping          keyMappings[CHATPAD_NUM_KEYS];
   ControllerButtonMapping    buttonMappings[CONTROLLER_NUM_BUTTONS];
} ChatpadDriverConfigStructure;


//***
// Function prototypes
//***

// This function initializes default driver config data values.
int InitDefaultDriverConfigDataValues(ChatpadDriverConfigStructure* pConfigStructure);

// Loads data from the configuration file with the given filename.
//
// This function opens the file, initializes the structure pointed to by
// pConfigStructure to a default state, parses the configuration data from the
// config file, writes it into the structure pointed to by pConfigStructure,
// and closes the file.  If pFilename is NULL, pConfigStructure is NULL, a
// syntax error is encountered, or another problem occurs, then this function
// fails.
//
// Return SUCCESS on success and FAILURE on failure.
int LoadConfigFile(
  char*                          pFilename,
  ChatpadDriverConfigStructure*  pConfigStructure);


#endif // CHATPAD_CONFIG_H

