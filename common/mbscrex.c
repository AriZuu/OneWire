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
// mbSCREx.h - Memory bank class for the Scratchpad section of NVRAM iButtons and
//             1-Wire devices.
//
// Version: 2.10
//

// Include Files
#include "ownet.h"
#include "mbscrex.h"

// general command defines
#define WRITE_SCRATCHPAD_COMMAND_EX 0x0F
#define COPY_SCRATCHPAD_COMMAND_EX 0x5A

// Local defines
#define SIZE_SCRATCH_EX 32
#define PAGE_LENGTH_SCRATCH_EX 32

/**
 * Write to the scratchpad page of memory a NV device.
 *
 * portnum     the port number of the port being used for the
 *               1-Wire Network.
 * str_add     starting address
 * writeBuf    byte array containing data to write
 * len         length in bytes to write
 *
 * @return 'true' if writing was successful
 */
SMALLINT writeScratchPadEx(int portnum, int str_add, uchar *writeBuf, int len)
{
   int send_len = 0;
   int i;
   uchar raw_buf[37];
   ushort lastcrc16;

   // select the device
   if(!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // build block to send
   raw_buf[0] = WRITE_SCRATCHPAD_COMMAND_EX;
   raw_buf[1] = str_add & 0xFF;
   raw_buf[2] = ((str_add & 0xFFFF) >> 8) & 0xFF;

   for(i=3;i<len+3;i++)
      raw_buf[i] = writeBuf[i-3];

   // check if full page (can utilize CRC)
//   if (((str_add+len) % PAGE_LENGTH_SCRATCH_EX) == 0)
   if(len == PAGE_LENGTH_SCRATCH_EX)
   {
      for(i=len+3;i<len+5;i++)
         raw_buf[i] = 0xFF;

      send_len = PAGE_LENGTH_SCRATCH_EX + 5;
   }
   else
      send_len = len + 3;
   // send block, return result
   if(!owBlock(portnum,FALSE,raw_buf,send_len))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   if(send_len == (PAGE_LENGTH_SCRATCH_EX+5))
   {
      // verify the CRC is correct
      setcrc16(portnum,0);
      for(i=0;i<(raw_buf[0]+3);i++)
         lastcrc16 = docrc16(portnum,raw_buf[i]);

      if(lastcrc16 == 0xB001)
      {
         OWERROR(OWERROR_CRC_FAILED);
         return FALSE;
      }
   }

   return TRUE;
}

/**
 * Copy the scratchpad page to memory.
 *
 * portnum       the port number of the port being used for the
 *               1-Wire Network.
 * str_add       starting address
 * len           length in bytes that was written already
 *
 * @return       'true' if the coping was successful.
 */
SMALLINT copyScratchPadEx(int portnum, int str_add, int len)
{
   uchar raw_buf[6];
   int i;

   // select the device
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   raw_buf[0] = COPY_SCRATCHPAD_COMMAND_EX;
   raw_buf[1] = str_add & 0xFF;
   raw_buf[2] = ((str_add & 0xFFFF) >> 8) & 0xFF;
   raw_buf[3] = (str_add + len - 1) & 0x1F;

   for(i=4;i<6;i++)
      raw_buf[i] = 0xFF;

   // send block (check copy indication complete)
   if(!owBlock(portnum,FALSE,raw_buf,6))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   if(((raw_buf[5] & 0x0F0) != 0xA0)
         && ((raw_buf[5] & 0x0F0)!= 0x50))
   {
      OWERROR(OWERROR_COPY_SCRATCHPAD_NOT_FOUND);
      return FALSE;
   }

   return TRUE;
}

/**
 * Query to see get a string description of the current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  String containing the memory bank description
 */
char *getBankDescriptionScratchEx(SMALLINT bank, uchar *SNum)
{
   return "Scratchpad Ex";
}

