//---------------------------------------------------------------------------
// Copyright (C) 2001 Dallas Semiconductor Corporation, All Rights Reserved.
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
//  swtoper.C - Menu-driven test of DS2406(DS2407) 1-Wire switch
//  version 3.00


// Include files
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "ownet.h"
#include "swt12.h"

// Constant definition
#define SWITCH_FAMILY      0x12
#define MAXDEVICES         15

// External subroutines
extern short ReadSwitch12(int,int);
extern int SetSwitch12(int,uchar *,SwitchProps);
extern int SwitchStateToString12(int,char *);
extern SMALLINT owAccess(int);
extern SMALLINT owAcquire(int,char *);
extern void owRelease(int);
extern void owSerialNum(int,uchar *,SMALLINT);
extern SMALLINT FindDevices(int,uchar FamilySN[][8],SMALLINT,SMALLINT);
extern void msDelay(int);
extern int EnterNum(char *,int,long *,long,long);

//---------------------------------------------------------------------------
// The main program that performs the operations on switches
//
int main(int argc, char **argv)
{
   short test;                     //info byte data
   short clear=0;                  //used to clear the button
   short done;                     //to tell when the user is done
   SwitchProps sw;                 //used to set Channel A and B
   uchar SwitchSN[MAXDEVICES][8];  //the serial number for the devices
   int num;                        //for the number of devices present
   int i,j,n,count,result;         //loop counters and indexes
   char out[140];                  //used for output of the info byte data
   int portnum=0;
   long select;                    //inputed number from user

   //----------------------------------------
   // Introduction header
   printf("\n/---------------------------------------------\n");
   printf("  Switch - V3.00\n"
          "  The following is a test to excersize the \n"
          "  setting of the state in a DS2406.\n");

   printf("  Press any CTRL-C to stop this program.\n\n");

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

   // this is to get the number of the devices and the serial numbers
   num = FindDevices(portnum, &SwitchSN[0], SWITCH_FAMILY, MAXDEVICES);

   // setting up the first print out for the frist device
   owSerialNum(portnum, SwitchSN[0], FALSE);

   printf("\n");
   n=0;
   if(owAccess(portnum))
   {
      // loop while not done
      do
      {
         test = ReadSwitch12(portnum, clear);

         for(i=7; i>=0; i--)
            printf("%02X", SwitchSN[n][i]);

         printf("\n");

         count = SwitchStateToString12(test, out);
         printf("%s", out);

         // print menu
         select = 1;
         if (!EnterNum("\n\n(1) Display the switch Info\n"
              "(2) Clear activity Latches\n"
              "(3) Set Flip Flop(s) on switch\n"
              "(4) Select different device\n"
              "(5) Quit\n"
              "Select a Number", 1, &select, 1, 5))
              break;
         printf("\n\n");

         // do something from the menu selection
         clear = FALSE;
         switch(select)
         {
            case 1: // Display the switch Info
               done = FALSE;
               break;
            case 2: // Clear activity Latches
               clear = TRUE;
               done = FALSE;
               break;
            case 3: // Set Flip Flop(s) on switch
               select = 0;
               if (EnterNum("Channel A Flip Flop (1 set, 0 clear)",
                    1, &select, 0, 1))
               {
                  sw.Chan_A = (uchar)select;

                  if(test & 0x40)
                  {
                     if (EnterNum("Channel B Flip Flop (1 set, 0 clear)",
                          1, &select, 0, 1))
                     {
                        sw.Chan_B = (uchar)select;
                        done = FALSE;
                     }
                  }
                  else
                  {
                    sw.Chan_B = 0;
                    done = FALSE;
                  }
                  printf("\n");
               }

               // proceed to setting switch state if not done
               if (!done)
               {
                  // loop to attempt to set the switch (try up to 5 times)
                  count = 0;
                  do
                  {
                     result = SetSwitch12(portnum, SwitchSN[n], sw);

                     // if failed then delay to let things settle
                     if (!result)
                        msDelay(50);
                  }
                  while ((count++ < 5) && (result != TRUE));

                  // if could not set then show error
                  if (!result)
                     printf("Could not set Switch!\n");
               }
               break;
            case 4: // Switch Devices
               // print the device list
               for(j=0; j < num; j++)
               {
                  printf("%d  ", j+1);
                  for(i=7; i>=0; i--)
                  {
                     printf("%02X", SwitchSN[j][i]);
                  }
                  printf("\n");
               }
               printf("\n");

               // select the device
               select = 0;
               if (EnterNum("Select Device",1, &select, 1, num))
               {
                  n = (int)(select - 1);
                  owSerialNum(portnum, SwitchSN[n], FALSE);
                  done = FALSE;
               }
               break;

            case 5: // Done
               done = TRUE;
               break;
               default:
            break;
         }
      }
      while (!done);
   }
   //One Wire Access
   owRelease(portnum);
   printf("Closing port %s.\n", argv[1]);
   exit(0);

   return 0;
}

