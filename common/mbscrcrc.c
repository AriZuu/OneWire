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
// mbSCRCRC.h - Memory bank class for the Scratchpad section of NVRAM iButtons and
//              1-Wire devices.
//
// Version: 2.10
//

// Include Files
#include "ownet.h"
#include "mbscrcrc.h"
#include "mbscr.h"

// general command defines 
#define READ_SCRATCHPAD_COMMAND_CRC 0xAA
#define COPY_SCRATCHPAD_COMMAND_CRC 0x55

// Local defines
#define SIZE_SCRATCH_CRC 32
#define PAGE_LENGTH_SCRATCH_CRC 32

/**
 * Read  page in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC).
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to read
 * rd_cont  if 'true' then device read is continued without
 *          re-selecting.  This can only be used if the new
 *          read() continious where the last one led off
 *          and it is inside a 'beginExclusive/endExclusive'
 *          block.
 * buff     byte array containing data that was read.
 *
 * @return - returns '0' if the read page wasn't completed.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageScratchCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                            SMALLINT rd_cont, uchar *buff)
{
   uchar extra[3];

   if(!readPageExtraScratchCRC(bank,portnum,SNum,page,rd_cont,buff,&extra[0]))
      return FALSE;

   return TRUE;
}

/**
 * Read  page with extra information in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to read
 * rd_cont  if 'true' then device read is continued without
 *          re-selecting.  This can only be used if the new
 *          read() continious where the last one led off
 *          and it is inside a 'beginExclusive/endExclusive'
 *          block.
 * buff     byte array containing data that was read
 * extra    the extra information
 *
 * @return - returns '0' if the read page wasn't completed with extra info.
 *                   '1' if the operation is complete.
 */

SMALLINT readPageExtraScratchCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                                 SMALLINT rd_cont, uchar *buff, uchar *extra)
{
   // only needs to be implemented if supported by hardware
   if(!hasPageAutoCRCScratch(bank,SNum))
   {
      OWERROR(OWERROR_CRC_NOT_SUPPORTED);
      return FALSE;
   }

   // check if read exceeds memory
   if (page > getNumberPagesScratch(bank,SNum))
   {
      OWERROR(OWERROR_READ_OUT_OF_RANGE);
      return FALSE;
   }

   owSerialNum(portnum,SNum,FALSE);

   // read the scratchpad
   if(!readScratchPadCRC(portnum, buff, PAGE_LENGTH_SCRATCH_CRC, extra))
      return FALSE;

   return TRUE;
}

/**
 * Read the scratchpad page of memory from a NV device
 * This method reads and returns the entire scratchpad after the byte
 * offset regardless of the actual ending offset.
 *
 * NOTE:  If tring to just read the last byte of the scratch pad the
 *        read will not work.  It is an error in the hardware.  It is
 *        currently being looked at and will be resolved soon.
 *
 * portnum       the port number of the port being used for the
 *               1-Wire Network.
 * buff          byte array to place read data into
 *               length of array is always pageLength.
 * len           length in bytes to read
 * extra         byte array to put extra info read into
 *               (TA1, TA2, e/s byte)
 *               length of array is always extraInfoLength.
 *               Can be 'null' if extra info is not needed.
 *
 * @return  'true' if reading scratch pad was successfull.
 */
SMALLINT readScratchPadCRC(int portnum, uchar *buff, int len, uchar *extra)
{
   int i,num_crc,addr;
   uchar raw_buf[38];
   ushort lastcrc16;

   // select the device
   if(!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // build block
   raw_buf[0] = READ_SCRATCHPAD_COMMAND_CRC;

   for(i=1;i<38;i++)
      raw_buf[i] = 0xFF;

   // send block, command + (extra) + page data + CRC
   if(!owBlock(portnum,FALSE,raw_buf,len+6))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   // get the starting offset to see when the crc will show up
   addr = raw_buf[1];

   addr = (addr | ((raw_buf[2] << 8) & 0xFF00)) & 0xFFFF;

   num_crc = 35 - (addr & 0x001F) + 3;

   // check crc of entire block
   setcrc16(portnum,0);
   for(i=0;i<num_crc;i++)
      lastcrc16 = docrc16(portnum,raw_buf[i]);

   if(lastcrc16 != 0xB001)
   {
      OWERROR(OWERROR_CRC_FAILED);
      return FALSE;
   }

   // extract the extra info
   for(i=1;i<4;i++)
      extra[i-1] = raw_buf[i];

   // extract the page data
   for(i=4;i<36;i++)
      buff[i-4] = raw_buf[i];

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
SMALLINT copyScratchPadCRC(int portnum, int str_add, int len)
{
   uchar raw_buf[6];
   int i;

   // select the device
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   raw_buf[0] = COPY_SCRATCHPAD_COMMAND_CRC;
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
char *getBankDescriptionScratchCRC(SMALLINT bank, uchar *SNum)
{
   return "Scratchpad with CRC";
}

/**
 * Query to see if current memory bank pages can be read with
 * the contents being verified by a device generated CRC.
 * This is used to see if the 'ReadPageCRCScratch()' can be used.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can be read with self
 *          generated CRC.
 */
SMALLINT hasPageAutoCRCScratchCRC(SMALLINT bank, uchar *SNum)
{
   return TRUE;
}
