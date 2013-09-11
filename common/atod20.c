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
//  atod20.c - Module(s) to do conversion, setup, read, and write
//             on the DS2450 - 1-Wire Quad A/D Converter.
//
//
//  Version: 2.00
//
//
// --------------------------------------------------------------------------

#include <stdio.h>
#include "ownet.h"
#include "atod20.h"

// -------------------------------------------------------------------------
// Setup A to D control data.  This is hardcoded to 5.12Volt scale at
// 8 bits, but it could be read from a file.
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'SerialNum'     - Serial Number of device
// 'ctrl'          - pointer to conrtol data to set (fixed info for now)
// 'msg'           - poniter to string to return message
//
int SetupAtoDControl(int portnum, uchar *SerialNum, uchar *ctrl, char *msg)
{
   uchar i;

   // set the device serial number to the DS2450 device
   owSerialNum(portnum,SerialNum,FALSE);

   // setup CONTROL register
   for (i = 0; i < 8; i += 2)
   {
      ctrl[i] = NO_OUTPUT | BITS_8;
      ctrl[i + 1] = RANGE_512 | ALARM_HIGH_DISABLE | ALARM_LOW_DISABLE;
   }

   // setup ALARM register
   for (i = 8; i < 16; i++)
      ctrl[i] = 0;

   // set return value
   sprintf(msg, "All channels set to 5.12V range at 8 bits");

   return TRUE;
}

//--------------------------------------------------------------------------
// Write A to D with provided buffer.
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'try_overdrive' - True(1) if want to try to use overdrive
// 'SerialNum'     - Serial Number of device
// 'ctrl'          - pointer to conrtol data to write
// 'start_address' - start address to write
// 'end_address'   - end of address to write
//
// Returns: TRUE, success, results read ok
//          FALSE, failure to read
//
int WriteAtoD(int portnum, int try_overdrive, uchar *SerialNum,
               uchar *ctrl, int start_address, int end_address)
{
   uchar send_block[50];
   uchar i;
   short send_cnt=0,flag,address;
   ushort lastcrc16;

   // build the first part of the packet to write
   // reset CRC
   setcrc16(portnum,0);
   send_cnt = 0;
   // write memory command
   send_block[send_cnt++] = 0x55;
   lastcrc16 = docrc16(portnum,0x55);
   // address
   send_block[send_cnt] = (uchar) (start_address & 0xFF);
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
   send_block[send_cnt] = (uchar)((start_address >> 8) & 0xFF);
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

   // set the device serial number to the DS2450 device
   owSerialNum(portnum,SerialNum,FALSE);

   // select the device
   if (Select(portnum,try_overdrive))
   {
      // loop to write each byte
      for (address = start_address; address <= end_address; address++)
      {
         // build the packet to write
         // init CRC on all but first pass
         if (address != start_address)
            setcrc16(portnum,address);
         // data to write
         send_block[send_cnt] = ctrl[address - start_address];
         lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
         // read CRC16
         send_block[send_cnt++] = 0xFF;
         send_block[send_cnt++] = 0xFF;
         // echo of byte
         send_block[send_cnt++] = 0xFF;

         // send the block
         flag = owBlock(portnum,FALSE, send_block, send_cnt);

         // check results of block
         if (flag == TRUE)
         {
            // perform crc16 on last 2 bytes
            for (i = (send_cnt - 3); i <= (send_cnt - 2); i++)
               lastcrc16 = docrc16(portnum,send_block[i]);

            // verify crc16 is correct
            if ((lastcrc16 != 0xB001) ||
                (send_block[send_cnt-1] != ctrl[address - start_address]))
               return FALSE;
         }
         else
            return FALSE;

         // reset the packet
         setcrc16(portnum,0);
         send_cnt = 0;
      }
   }
   // no device
   else
      return FALSE;

   return TRUE;
}

// --------------------------------------------------------------------------
// Attempt to do an A to D conversion
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'try_overdrive' - True(1) if want to try to use overdrive
// 'SerialNum'     - Serial Number of device
//
//
// Returns: TRUE, success, conversion ok
//          FALSE, failure to do conversion
//
int DoAtoDConversion(int portnum, int try_overdrive, uchar *SerialNum)
{
   uchar send_block[50];
   int i;
   short send_cnt=0;
   ushort lastcrc16;

   // set the device serial number to the DS2450 device
   owSerialNum(portnum,SerialNum,FALSE);

   // access the device
   if (Select(portnum,try_overdrive))
   {
      setcrc16(portnum,0);
      // create a block to send
      send_block[send_cnt] = 0x3C;
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

      // input select mask (all channels)
      send_block[send_cnt] = 0x0F;
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

      // read-out control
      send_block[send_cnt] = 0x00;
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

      // read CRC16
      send_block[send_cnt++] = 0xFF;
      send_block[send_cnt++] = 0xFF;

      // now send the block
      if (owBlock(portnum,FALSE, send_block, (send_cnt)))
      {
         // check the CRC
         for (i = send_cnt - 2; i < (send_cnt); i++)
            lastcrc16 = docrc16(portnum,send_block[i]);

         // verify CRC16 is correct
         if (lastcrc16 != 0xB001)
         {
            OWERROR(OWERROR_CRC_FAILED);
            return FALSE;
         }

         // if success then apply the strong pullup
         if(!owWriteBytePower(portnum,((send_cnt-1) & 0x1F)))
            return FALSE;
         // delay the max time, 6ms
         msDelay(6);
         // set the pullup back to normal
         if(MODE_NORMAL != owLevel(portnum,MODE_NORMAL))
         {
            OWERROR(OWERROR_LEVEL_FAILED);
            return FALSE;
         }

         // check conversion over
         if (owReadByte(portnum) == 0xFF)
            return TRUE;
      }
   }

   return FALSE;
}

//--------------------------------------------------------------------------
// Read A to D results
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'try_overdrive' - True(1) if want to try to use overdrive
// 'SerialNum'     - Serial Number of device
// 'prslt'         - pointer to array of 4 floats to return voltage results
// 'ctrl'          - pointer to conrtol data for reference when calculating
//                   results.
//
// Returns: TRUE, success, results read ok
//          FALSE, failure to read
//
int ReadAtoDResults(int portnum, int try_overdrive, uchar *SerialNum,
                         float *prslt, uchar *ctrl)
{
   uchar i, send_cnt=0;
   ulong templong;
   uchar rt=FALSE;
   uchar send_block[30];
   ushort lastcrc16;

   setcrc16(portnum,0);

   // set the device serial number to the DS2450 device
   owSerialNum(portnum,SerialNum,FALSE);

   // access the device
   if (Select(portnum,try_overdrive))
   {
      // create a block to send that reads the DS2450
      send_block[send_cnt++] = 0xAA;
      lastcrc16 = docrc16(portnum,0xAA);

      // address block
      send_block[send_cnt++] = 0x00;
      send_block[send_cnt++] = 0x00;
      lastcrc16 = docrc16(portnum,0x00);
      lastcrc16 = docrc16(portnum,0x00);

      // read the bytes
      for (i = 0; i < 10; i++)
         send_block[send_cnt++] = 0xFF;

      // send the block
      if (owBlock(portnum,FALSE, send_block, send_cnt))
      {
         // perform CRC16
         for (i = send_cnt - 10; i < send_cnt; i++)
            lastcrc16 = docrc16(portnum,send_block[i]);

         // verify CRC16 is correct
         if (lastcrc16 == 0xB001)
         {
            // success
            rt = TRUE;
         }

      }

      // verify read ok
      if (rt == TRUE)
      {
         // convert the value read to floats
         for (i = 3; i < 11; i += 2)  // (1.01)
         {
            templong = ((send_block[i + 1] << 8) | send_block[i]) & 0x0000FFFF;
            prslt[(i - 3) / 2] = (float)((float)(templong / 65535.0) *
               ((ctrl[(i - 3) + 1] & 0x01) ? 5.12 : 2.56)); // (1.01)
         }
      }
   }

   return rt;
}

//--------------------------------------------------------------------------
// Select the current device and attempt overdrive if possible.
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
//
// 'try_overdrive' - True(1) if want to try to use overdrive
//
//  Returns: TRUE(1)  current device selected
//           FALSE(0) device not present
//
int Select(int portnum, int try_overdrive)
{
   static uchar current_speed = MODE_NORMAL;

   // attempt to do overdrive
   if (try_overdrive)
   {
      // verify device is in overdrive
      if (current_speed == MODE_OVERDRIVE)
      {
         if (owAccess(portnum))
            return TRUE;
      }

      if (owOverdriveAccess(portnum))
         current_speed = MODE_OVERDRIVE;
      else
         current_speed = MODE_NORMAL;
   }

   return (owAccess(portnum));
}
