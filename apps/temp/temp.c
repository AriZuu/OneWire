//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
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
//---------------------------------------------------------------------------
//
//  temp.c -   Application to find and read the 1-Wire Net
//             DS1920/DS1820/DS18S20 - temperature measurement.
//
//             This application uses the files from the 'Public Domain'
//             1-Wire Net libraries ('general' and 'userial').
//
//
//  Version: 2.00
//

#include <stdlib.h>
#include <stdio.h>
#include "ownet.h"
#include "temp10.h"
#include "findtype.h"

// defines
#define MAXDEVICES         20

// global serial numbers
uchar FamilySN[MAXDEVICES][8];

// variables
int family_code;

//----------------------------------------------------------------------
//  Main Test for DS1920/DS1820 temperature measurement
//
int main(int argc, char **argv)
{
   float current_temp;
   int i = 0;
   int NumDevices=0;
   int portnum = 0;

   //----------------------------------------
   // Introduction header
   printf("\n/---------------------------------------------\n");
   printf("  Temperature application DS1920/DS1820 - Version 1.00 \n"
          "  The following is a test to excersize a DS1920/DS1820.\n"
          "  Temperature Find and Read from a: \n"
          "  DS1920/DS1820 (at least 1)\n\n");

   printf("  Press any CTRL-C to stop this program.\n\n");
   printf("  Output [Serial Number(s) ........ Temp1(F)] \n\n");

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

   // success
   printf("Port opened: %s\n",argv[1]);

   // Find the device(s)
   NumDevices = FindDevices(portnum, &FamilySN[0], 0x10, MAXDEVICES);
   if (NumDevices>0)
   {
      printf("\n");
      printf("Device(s) Found: \n");
      for (i = 0; i < NumDevices; i++)
      {
         PrintSerialNum(FamilySN[i]);
         printf("\n");
      }
      printf("\n\n");

      // (stops on CTRL-C)
      do
      {
         // read the temperature and print serial number and temperature
         for (i = 0; i < NumDevices; i++)
         {

            if (ReadTemperature(portnum, FamilySN[i],&current_temp))
            {
               PrintSerialNum(FamilySN[i]);
               printf("     %5.1f \n", current_temp * 9 / 5 + 32);
              // converting temperature from Celsius to Fahrenheit
            }
            else
               printf("     Error reading temperature, verify device present:%d\n",
                       (int)owVerify(portnum, FALSE));
         }
         printf("\n");
      }
      while (!key_abort());
   }
   else
      printf("\n\n\nERROR, device DS1920/DS1820 not found!\n");

   // release the 1-Wire Net
   owRelease(portnum);
   printf("Closing port %s.\n", argv[1]);
   exit(0);

   return 0;
}

