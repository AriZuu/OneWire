//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//--------------------------------------------------------------------------
//
//  swtsngl.C - Goes through the testing of the DS2405 device
//  Version 2.00
//

// Include files
#include <stdio.h>
#include <stdlib.h>
#include "ownet.h"
#include "swt05.h"
#include "findtype.h"

// Constant definitions
#define MAXDEVICES         15

//--------------------------------------------------------------------------
// This is the begining of the program that tests the commands for the
// DS2405
//
int main(int argc, char **argv)
{
   uchar SwitchSN[MAXDEVICES][8];  //the serial numbers for the devices
   short i,j;                      //loop counters
   int num;                        //for the number of DS2405s
   int lev;
   int portnum=0;

   // check for required port name
   if (argc != 2)
   {
      printf("1-Wire Net name required on command line!\n"
             " (example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" "
             "(Linux DS2480),\"1\" (Win32 TMEX)\n");
      exit(1);
   }

   // attempt to acquire the 1-Wire Net
   if((portnum = owAcquireEx(argv[1])) < 0)
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

   // this is to get the number of the devices and the serial numbers
	num = FindDevices(portnum, &SwitchSN[0], SWITCH_FAMILY, MAXDEVICES);

   for( i=0; i<num; i++)
   {
      if(SetSwitch05(portnum, SwitchSN[i], 1))
      {
         printf("Device ");

         for(j=7; j>=0; j--)
            printf("%02X", SwitchSN[i][j]);

         printf(" is active\n");
      }
      else
         printf("Error setting device on\n");

      if(ReadSwitch05(portnum, SwitchSN[i], &lev))
      {
         printf("Device ");

         for(j=7; j>=0; j--)
            printf("%02X", SwitchSN[i][j]);

         if(lev)
            printf(" is active and is high.\n");
         else
            printf(" is active and is low.\n");
      }
      else
         printf("Error reading active device\n");

      if(SetSwitch05(portnum, SwitchSN[i], 0))
      {
         printf("Device ");

         for(j=7; j>=0; j--)
            printf("%02X", SwitchSN[i][j]);

         printf(" is not active\n");
      }
      else
         printf("Error setting device off\n");

      if(!ReadSwitch05(portnum, SwitchSN[i], &lev))
      {
         printf("Device ");

         for(j=7; j>=0; j--)
            printf("%02X", SwitchSN[i][j]);

         if(lev)
            printf(" is not active and is high.\n");
         else
            printf(" is not active and is low.\n");
      }
      else
         printf("Error reading nonactive device\n");
   }

   if(num == 0)
      printf("DS2405 not found on the 1-Wire Network.\n");

   owRelease(portnum);
   printf("Closing port %s.\n", argv[1]);
   exit(0);

   return 0;
}


