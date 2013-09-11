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
//  temp.c -   Application to find and read the 1-Wire Net
//             DS1920/DS1820/DS18S20 - temperature measurement.
//
//             This application uses the files from the 'Public Domain'
//             1-Wire Net libraries ('general' and 'userial').
//
//
//  Version: 2.00
//

#include "ownet.h"
#include "temp10.h"
#include "findtype.h"
#include "ser550.h"

// defines
#define MAXDEVICES         20

// local functions
void DisplaySerialNum(uchar sn[8]);
int WeakReadTemperature(int, uchar*, float*);

// serial init functions for I/O
extern void serial0_init(uchar);
extern void serial1_init(uchar);

//----------------------------------------------------------------------
//  Main Test for DS1920/DS1820 temperature measurement
//
void main(void)
{
   uchar FamilySN[MAXDEVICES][8];
   float current_temp;
   int i = 0;
   int j = 0;
   int NumDevices = 0;
   SMALLINT didRead = 0;

   //use port number for 1-wire
   uchar portnum = ONEWIRE_P;

   //initialize I/O port
#if STDOUT_P==0
   serial0_init(BAUD9600_TIMER_RELOAD_VALUE);
#else
   serial1_init(BAUD9600_TIMER_RELOAD_VALUE);
#endif

   //----------------------------------------
   // Introduction header
   printf("\r\nTemperature\r\n");

   // attempt to acquire the 1-Wire Net
   if (!owAcquire(portnum,NULL))
   {
      printf("Acquire failed\r\n");
      while(owHasErrors())
         printf("  - Error %d\r\n", owGetErrorNum());
      return;
   }

   do
   {
      j = 0;
      // Find the device(s)
      NumDevices = FindDevices(portnum, FamilySN, 0x10, MAXDEVICES);
      if (NumDevices>0)
      {
         printf("\r\n");
         // read the temperature and print serial number and temperature
         for (i = NumDevices; i; i--)
         {
            printf("(%d) ", j++);
            DisplaySerialNum(FamilySN[i-1]);
            if(owHasPowerDelivery(portnum))
            {
               didRead = ReadTemperature(portnum, FamilySN[i-1],&current_temp);
            }
            else
            {
               didRead = WeakReadTemperature(portnum, FamilySN[i-1],&current_temp);
            }
            
            if (didRead)
            {
               printf(" %5.1f Celsius\r\n", current_temp);
            }
            else
            {
               printf("  Convert failed.  Device is");
               if(!owVerify(portnum, FALSE))
                  printf(" not");
               printf(" present.\r\n");
               while(owHasErrors())
                  printf("  - Error %d\r\n", owGetErrorNum());
            }
            
         }
      }
      else
         printf("No temperature devices found!\r\n");
         
      printf("\r\nPress any key to continue\r\n");
      i = getchar();
   }
   while (i!='q');

   // release the 1-Wire Net
   owRelease(portnum);

   return;
}

// -------------------------------------------------------------------------------
// Read and print the serial number.
//
void DisplaySerialNum(uchar sn[8])
{
   int i;
   for (i = 7; i>=0; i--)
      printf("%02X", (int)sn[i]);
}

//----------------------------------------------------------------------
// Read the temperature of a DS1920/DS1820 without using Power Delivery
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number was provided to
//                 OpenCOM to indicate the port number.
// 'SerialNum'   - Serial Number of DS1920/DS1820 to read temperature from
// 'Temp '       - pointer to variable where that temperature will be
//                 returned
//
// Returns: TRUE(1)  temperature has been read and verified
//          FALSE(0) could not read the temperature, perhaps device is not
//                   in contact
//
int WeakReadTemperature(int portnum, uchar *SerialNum, float *Temp)
{
   uchar rt=FALSE;
   uchar send_block[30],lastcrc8;
   int send_cnt=0, tsht, i, loop=0;
   float tmp,cr,cpc;

   setcrc8(portnum,0);

   // set the device serial number to the counter device
   owSerialNum(portnum,SerialNum,FALSE);

   for (loop = 0; loop < 2; loop ++)
   {
      // access the device
      if (owAccess(portnum))
      {
         // send the convert temperature command
         owTouchByte(portnum,0x44);

         // sleep for 1 second
         msDelay(1000);

         // turn off the 1-Wire Net strong pull-up
         if (owLevel(portnum,MODE_NORMAL) != MODE_NORMAL)
            return FALSE;

         // access the device
         if (owAccess(portnum))
         {
            // create a block to send that reads the temperature
            // read scratchpad command
            send_block[send_cnt++] = 0xBE;
            // now add the read bytes for data bytes and crc8
            for (i = 0; i < 9; i++)
               send_block[send_cnt++] = 0xFF;

            // now send the block
            if (owBlock(portnum,FALSE,send_block,send_cnt))
            {
               // perform the CRC8 on the last 8 bytes of packet
               for (i = send_cnt - 9; i < send_cnt; i++)
                  lastcrc8 = docrc8(portnum,send_block[i]);

               // verify CRC8 is correct
               if (lastcrc8 == 0x00)
               {
                  // calculate the high-res temperature
                  tsht = send_block[1]/2;
                  if (send_block[2] & 0x01)
                     tsht |= -128;
                  tmp = (float)(tsht);
                  cr = send_block[7];
                  cpc = send_block[8];
                  if (((cpc - cr) == 1) && (loop == 0))
                     continue;
                  if (cpc == 0)
                     return FALSE;
                  else
                     tmp = tmp - (float)0.25 + (cpc - cr)/cpc;

                  *Temp = tmp;
                  // success
                  rt = TRUE;
               }
            }
         }
      }

   }

   // return the result flag rt
   return rt;
}
