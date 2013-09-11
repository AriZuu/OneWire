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
//
//  tstfind_m.C - Test application to search for all 1-Wire devices on 1-Wire
//                Net.  Modified for use on embedded microcontrollers
//
//  Version: 2.00

#include "ownet.h"
#include "ser550.h"

// local functions
void DisplaySerialNum(int);

// serial init functions for I/O
extern void serial0_init(uchar);
extern void serial1_init(uchar);

//----------------------------------------------------------------------
//  Main for tstfind
//
void main(void)
{
   uchar rslt;
   int cnt;

   //use port for 1-wire
   uchar portnum = ONEWIRE_P;

   //initialize I/O port
#if STDOUT_P==0
   serial0_init(BAUD9600_TIMER_RELOAD_VALUE);
#else
   serial1_init(BAUD9600_TIMER_RELOAD_VALUE);
#endif

   printf("Beginning tstfindm\r\n");

   // attempt to acquire the 1-Wire Net
   if (!owAcquire(portnum,NULL))
   {
      printf("owAcquire failed\r\n");
      while(owHasErrors())
         printf("  - Error %d\r\n", owGetErrorNum());
      return;
   }

   //----------------------------------------
   // Introduction
   printf("\r\n/----------------\r\n");
   printf("  All iButtons.\r\n\r\n");

   do
   {
      printf("-- Start\r\n");
      cnt = 0;

      // find the first device (all devices not just alarming)
      rslt = owFirst(portnum, TRUE, FALSE);
      while (rslt)
      {
         // print the device number
         cnt++;
         printf("(%d) ",cnt);

         // get and print the Serial Number of the device just found
         DisplaySerialNum(portnum);

         // find the next device
         rslt = owNext(portnum, TRUE, FALSE);
      }
      printf("-- End\r\n\r\nPress any key to continue searching\r\n");

      rslt = getchar();
      //printf("key pressed: %c\r\n",rslt);
   }
   while (rslt!='q');

   // release the 1-Wire Net
   owRelease(portnum);

   return;
}

//----------------------------------------------------------------------
//  Read and print the Serial Number.
//
void DisplaySerialNum(int portnum)
{
   uchar serial_num[8];
   int i;

   owSerialNum(portnum,serial_num,TRUE);
   for (i = 7; i >= 0; i--)
      printf("%02X",(int)serial_num[i]);
   printf("\r\n");
}
