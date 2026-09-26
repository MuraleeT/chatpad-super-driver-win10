//
// Chatpad utility program that works with a KMDF USB filter driver, by
//   GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//
// The awesome guide at http://www.osronline.com/article.cfm?id=446 was used as a basis
// for how to put this driver together, but the code in this driver was all written
// by hand.
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "chatpad_control.h"

// TODO move elsewhere
#define CHATPAD_CONTROL_UTILITY_VERSION      "0.1a"


//***
// Functions
//***

int __cdecl main(int argc, char* argv[])
{
   int nRetValue = SUCCESS;

   printf("\n*****\n");
   printf("Chatpad control utility version %s started.\n", CHATPAD_CONTROL_UTILITY_VERSION);
   printf("*****\n");

   if((2 == argc) && (0 == strcmp(argv[1], "--nodelay")))
   {
      printf("Control utility started with no delay.\n");
   }
   else
   {
      printf("Please wait...\n");
      printf("\n");
      Sleep(5000);
      printf("Control utility started.\n");
   }

   nRetValue = ChatpadControlMainLoop();

   printf("Done.  The exit status is %d.\n", nRetValue);
   return nRetValue;
}

