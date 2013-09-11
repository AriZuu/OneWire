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
//  coupler.C - Goes through the testing of the DS2409 device
//  Version 2.00
//

// Include files
#include <stdio.h>
#include <stdlib.h>
#include "ownet.h"
#include "findtype.h"
#include "swt1f.h"

// Constant definitions
#define MAXDEVICES         15

//--------------------------------------------------------------------------
// This is the begining of the program that test the commands for the
// DS2409
//
int main(int argc, char **argv)
{
   uchar SwitchSN[MAXDEVICES][8];  //the serial numbers for the devices
   short i,j;                      //loop counters
   int num;                        //for the number of DS2409s
   int NumDevices=0;               //used for the number of devices on the branch
   uchar BranchSN[20][8];          //used for the devices on the main and aux.
                                   //branch
   uchar a[3];                     //used for storing info data on the read/write
   char out[240];                  //used for output of the info data
   int charnum;                    //for the number of charactes in a string
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
   if ((portnum = owAcquireEx(argv[1])) < 0)
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

   // this is to get the number of the devices and the serial numbers
	num = FindDevices(portnum, &SwitchSN[0], SWITCH_FAMILY, MAXDEVICES);

   // this is the main branch device list
   NumDevices = FindBranchDevice(portnum, SwitchSN[0], &BranchSN[0], MAXDEVICES, TRUE);
   printf("The device on the main branch\n");
   for(j=0; j<NumDevices; j++)
   {
      printf("%d", j+1);
      for(i=7; i>=0; i--)
         printf("%02X", BranchSN[j][i]);
      printf("\n");
   }
   printf("\n");

   // this is the auxilary branch device list
   NumDevices = FindBranchDevice(portnum, SwitchSN[0], &BranchSN[0], MAXDEVICES, FALSE);
   printf("The device on the Auxiliary branch\n");
   for(j=0; j<NumDevices; j++)
   {
      printf("%d", j+1);
      for(i=7; i>=0; i--)
         printf("%02X", BranchSN[j][i]);
      printf("\n");
   }
   printf("\n");

   // this is to test the all lines off command
   if(SetSwitch1F(portnum, SwitchSN[0], ALL_LINES_OFF, 0, a, TRUE))
      printf("All lines off worked\n");
   else
      printf("All lines off didn't work\n");

   // this is to test the direct on main command
   if(SetSwitch1F(portnum, SwitchSN[0], DIRECT_MAIN_ON, 0, a, TRUE))
      printf("Direct main on worked\n");
   else
      printf("Direct main on didn't work\n");

   // this is to test the smart auxilary on
   if(SetSwitch1F(portnum, SwitchSN[0], AUXILARY_ON, 2, a, TRUE))
      printf("Auxilary on worked\n");
   else
      printf("Auxilary didn't work\n");

   // this is to test the read/write command and print the info.
   if(SetSwitch1F(portnum, SwitchSN[0], STATUS_RW, 1, a, TRUE))
   {
      printf("Status read write worked and the results are below\n");
      charnum = SwitchStateToString1F((short) a[1], out);
      printf("%s", out);
   }
   else
      printf("Status read write didn't work");

   owRelease(portnum);
   printf("Closing port %s.\n", argv[1]);
   exit(0);

   return 0;
}


