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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
//  atodtst.c - Application for the DS2450 - 1-Wire Quad A/D Converter.
//
//             This application uses the files from the 'Public Domain' MicroLAN
//             libraries ('general' and 'userial').
//
//  Version: 2.00
//

#include <stdio.h>
#include <stdlib.h>
#include "ownet.h"
#include "atod20.h"
#include "findtype.h"

// defines
#define MAXDEVICES           20

// global serial numbers
uchar FamilySN[MAXDEVICES][8];

//----------------------------------------------------------------------
//  Main Test for the DS2450 - 1-Wire Quad A/D Converter
//
int main(int argc, char **argv)
{
   char msg[45];
   int NumDevices = 0;
   int i = 0;
   int start_address = 0x8;
   int end_address = 0x11;
   float prslt[4];
   uchar ctrl[16];
   int try_overdrive=0;
   int portnum=0;

   //------------------------------------------------------
   // Introduction header
   printf("\n/---------------------------------------------\n");
   printf("  Channels A to D Application - V2.00\n"
          "  The following is a test to excersize a\n"
          "  DS2450 - 1-Wire Quad A/D Converter \n\n");

   printf("  Press any CTRL-C to stop this program.\n\n");
   printf("  Output   [Serial Number(s) ... Channel 'A' Value ... Channel 'B' Value ... \n"
                         "                             ... Channel 'C' Value ... Channel 'D' Value] \n\n");

   // check for required port name
   if (argc != 2)
   {
      printf("1-Wire Net name required on command line!\n"
             " (example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" "
             "(Linux DS2480),\"1\" (Win32 TMEX)\n");
      exit(1);
   }

   // attempt to acquire the 1-Wire Net
   if ((portnum = owAcquireEx(argv[1])) < 0)
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

   // success
   printf("Port opened: %s\n",argv[1]);

   // Find the device(s)
   NumDevices = FindDevices(portnum, &FamilySN[0], 0x20, MAXDEVICES);
   if (NumDevices>0)
   {
      printf("\n");
      printf("Device(s) Found: \n");

      for (i = 0; i < NumDevices; i++)
      {

         PrintSerialNum(FamilySN[i]);
         printf("\n");

         if (SetupAtoDControl(portnum, FamilySN[i], &ctrl[0], &msg[0]))
         {
            printf("A/D settings found\n %s\n", msg);
         }
         else
            printf("\n\n\n ERROR, device set up unsuccessful!\n");


         if (WriteAtoD(portnum, try_overdrive, FamilySN[i], &ctrl[0], start_address, end_address))
         {
            printf("\nA/D settings written");

         }
         else
            printf("\n\n\n ERROR, device not found!\n");
      }
   }

   // (stops on CTRL-C)
   do
   {
      // read the current channels
      for (i = 0; i < NumDevices; i++)
      {
         printf("\n\n");
         PrintSerialNum(FamilySN[i]);

         if (!DoAtoDConversion(portnum, try_overdrive, FamilySN[i]))
         {
            printf("\nError doing conversion, verify device present: %d\n",
            (int)owVerify(portnum,FALSE));
         }
         if (ReadAtoDResults(portnum, try_overdrive, FamilySN[i], &prslt[0], &ctrl[0]))
         {
            int  c = 0;
            for (c = 0; c < 4; c++)
            {
               printf("  %1.3f ", prslt[c]);
            }
         }
         else
         {
            printf("\nError reading channel, verify device present: %d\n",
            (int)owVerify(portnum,FALSE));
         }

      }
   }
   while (!key_abort());

   // release the 1-Wire Net
   owRelease(portnum);
   printf("Closing port %s.\n", argv[1]);
   exit(0);

   return 0;
}



