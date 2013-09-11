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
//  swtloop.C - Goes through the testing of the DS2406(DS2407) switch
//  Version 2.00


// Include files
#include <stdio.h>
#include <stdlib.h>
#include "ownet.h"
#include "swt12.h"
#include "findtype.h"

// Constant definition
#define SWITCH_FAMILY      0x12
#define MAXDEVICES         15

// local
int getNumber (int min, int max);


//--------------------------------------------------------------------------
// This is the begining of the program that tests the different Channels
int main(int argc, char **argv)
{
   int i,j,k,n;                    //loop counters
   short test=0;                   //info byte data
   short clear=0;                  //used to clear the button
   SwitchProps sw;                 //used to set Channel A and B
   uchar SwitchSN[MAXDEVICES][8];  //the serial numbers for the devices
   int num;                        //for the number of devices present
   char out[140];                  //used for output of the info byte data
   short done = FALSE;             //used to indicate the end of the input loop from user
   int portnum=0;

   //----------------------------------------
   // Introduction header
   printf("\n/---------------------------------------------\n");
   printf("  swtest - V2.00\n"
          "  The following is a test to excersize the\n"
          "  different channels on the DS2406.\n");
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
   if((portnum = owAcquireEx(argv[1])) < 0)
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

   j=1;
   n=0;
   do
   {
      printf("\n\n");
      for(k=0; k < num; k++)
      {
         printf("%d  ", k);
         for(i=7; i>=0; i--)
         {
            printf("%02X", SwitchSN[k][i]);
         }
         printf("\n");
      }
      printf("%d To quit.\n", k);

      printf("\n");
      printf("Pick a device\n");

      n = getNumber(0,num);

      if(n == num)
      {
         n = 0;        //used to finish off the loop
         done = TRUE;
         break;
      }

      owSerialNum(portnum, SwitchSN[n], FALSE);
      j = 1;

      printf("\n");

      test = ReadSwitch12(portnum,clear);

      // This looks at the info byte to determine if it is a
      // two or one channel device.
      if(test & 0x40)
      {

         switch(j)
         {
            case 1:
               sw.Chan_A = 0;
               sw.Chan_B = 0;
               break;
            case 2:
               sw.Chan_A = 0;
               sw.Chan_B = 1;
               break;
            case 3:
               sw.Chan_A = 1;
               sw.Chan_B = 0;
               break;
            case 4:
               sw.Chan_A = 1;
               sw.Chan_B = 1;
               break;
            default:
               sw.Chan_A = 1;
               sw.Chan_B = 1;
               j=0;
               break;
            }
      }
      else
      {
         switch(j)
         {
            case 1:
               sw.Chan_B = 0;
               sw.Chan_A = 0;
               break;
            case 2:
               sw.Chan_B = 0;
               sw.Chan_A = 1;
               break;
            default:
               sw.Chan_B = 0;
               sw.Chan_A = 1;
               j = 0;
               break;
         }
      }

      if(!SetSwitch12(portnum, SwitchSN[n], sw))
      {
         msDelay(50);
         if(SetSwitch12(portnum, SwitchSN[n], sw))
            msDelay(50);
         else
            printf("Switch not set\n");
      }

      test = ReadSwitch12(portnum,clear);

      printf("\n");

      for(i=7; i>=0; i--)
      {
         printf("%02X", SwitchSN[n][i]);
      }
      printf("\n");

      SwitchStateToString12(test, out);
      printf("%s", out);

      j++;

   }
   while(!done);

   owRelease(portnum);
   printf("Closing port %s.\n", argv[1]);
   exit(0);

   return 0;
}

/**
 * Retrieve user input from the console.
 *
 * min  minimum number to accept
 * max  maximum number to accept
 *
 * @return numeric value entered from the console.
 */
int getNumber (int min, int max)
{
   int value = min,cnt;
   int done = FALSE;
   do
   {
      cnt = scanf("%d",&value);
      if(cnt>0 && (value>max || value<min))
      {
         printf("Value (%d) is outside of the limits (%d,%d)\n",value,min,max);
         printf("Try again:\n");
      }
      else
         done = TRUE;

   }
   while(!done);

   return value;

}