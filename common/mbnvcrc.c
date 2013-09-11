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
// mbEE.h - Include memory bank EE functions.
//
// Version: 2.10
//

// Include files
#include "ownet.h"
#include "mbnv.h"
#include "mbnvcrc.h"

// general command defines 
#define READ_MEMORY_COMMAND      0xF0
#define WRITE_SCRATCHPAD_COMMAND 0x0F
#define READ_SCRATCHPAD_COMMAND  0xAA
#define COPY_SCRATCHPAD_COMMAND  0x55
#define READ_PAGE_WITH_CRC       0xA5

// Local defines
#define SIZE_NVCRC        32
#define PAGE_LENGTH_NVCRC 32

// Global variables
SMALLINT pageAutoCRCNVCRC     = TRUE;
SMALLINT readContinuePossible = TRUE;


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
SMALLINT readPageNVCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                       SMALLINT rd_cont, uchar *buff)
{
   uchar extra[8];

   return readCRC(bank,portnum,SNum,page,rd_cont,buff,
                  extra,getExtraInfoLengthNV(bank,SNum));
}


/**
 * Read  page with extra information in the current bank with no
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
 * buff     byte array containing data that was read
 * extra    the extra information
 *
 * @return - returns '0' if the read page wasn't completed with extra info.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageExtraNVCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                            SMALLINT rd_cont, uchar *buff, uchar *extra)
{
   if(!hasExtraInfoNV(bank,SNum))
   {
      OWERROR(OWERROR_EXTRA_INFO_NOT_SUPPORTED);
      return FALSE;
   }

   return readCRC(bank,portnum,SNum,page,rd_cont,buff,
                  extra,getExtraInfoLengthNV(bank,SNum));
}


/**
 * Read a complete memory page with CRC verification provided by the
 * device with extra information.  Not supported by all devices.
 *
 * bank      to tell what memory bank of the ibutton to use.
 * portnum   the port number of the port being used for the
 *           1-Wire Network.
 * SNum      the serial number for the part.
 * page      the page to read
 * read_buff byte array containing data that was read.
 * extra     the extra information
 *
 * @return - returns '0' if the read page wasn't completed with extra info.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageExtraCRCNVCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                               uchar *read_buff, uchar *extra)
{
   return readCRC(bank,portnum,SNum,page,FALSE,read_buff,
                  extra,getExtraInfoLengthNV(bank,SNum));
}

/**
 * Read a complete memory page with CRC verification provided by the
 * device.  Not supported by all devices.  
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to read
 * buff     byte array containing data that was read
 *
 * @return - returns '0' if the read page wasn't completed.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageCRCNVCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                          uchar *buff)
{
   uchar extra[8];

   return readCRC(bank,portnum,SNum,page,FALSE,buff,
                  extra,getExtraInfoLengthNV(bank,SNum));
}

/**
 * Read a Universal Data Packet and extra information.  
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
 * len      length of the packet
 * extra    extra information
 *
 * @return - returns '0' if the read page packet wasn't completed
 *                   '1' if the operation is complete.
 */
SMALLINT readPagePacketExtraNVCRC(SMALLINT bank, int portnum, uchar *SNum, 
                                  int page, SMALLINT rd_cont, uchar *buff,
                                  int *len, uchar *extra)
{
   uchar raw_buf[PAGE_LENGTH_NVCRC];
   int i;
   ushort lastcrc16;

   // read entire page with read page CRC
   if(!readCRC(bank,portnum,SNum,page,rd_cont,raw_buf,
               extra,getExtraInfoLengthNV(bank,SNum)))
      return FALSE;

   // check if length is realistic
   if (raw_buf[0] > getMaxPacketDataLengthNV(bank,SNum))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort) ((getStartingAddressNV(bank,SNum)/PAGE_LENGTH_NVCRC) + page));
   for(i=0;i<(raw_buf[0]+3);i++)
      lastcrc16 = docrc16(portnum,raw_buf[i]);

   if(lastcrc16 == 0xB001)
   {
      // extract the data out of the packet
      for(i=1;i<(raw_buf[0]+1);i++)
         buff[i-1] = raw_buf[i];

      // return the length
      *len = raw_buf[0];
   }
   else
   {
      OWERROR(OWERROR_CRC_FAILED);
      return FALSE;
   }

   return TRUE;
}

/**
 * Read a complete memory page with CRC verification provided by the
 * device with extra information.  Not supported by all devices.
 * If not extra information available then just call with extraLength=0.
 *
 * bank          to tell what memory bank of the ibutton to use.
 * portnum       the port number of the port being used for the
 *               1-Wire Network.
 * SNum          the serial number for the part.
 * page          page number to read
 * rd_cont       if 'true' then device read is continued without
 *               re-selecting.  This can only be used if the new
 *               readPagePacket() continious where the last one
 *               stopped and it is inside a
 *               'beginExclusive/endExclusive' block.
 * readBuf       byte array to put data read. Must have at least
 *               'getMaxPacketDataLength()' elements.
 * extra         byte array to put extra info read into
 * extra_len     length of extra information
 *
 * return TRUE if the read is successful.
 */
SMALLINT readCRC(SMALLINT bank, int portnum, uchar *SNum, int page, SMALLINT rd_cont,
                 uchar *readBuf, uchar *extra, int extra_len)
{
   int    i;
   int addr;
   uchar raw_buf[3];
   uchar read_buf[50];
   ushort lastcrc16 = 0;

   owSerialNum(portnum,SNum,FALSE);

   // only needs to be implemented if supported by hardware
   if (!hasPageAutoCRCNV(bank,SNum))
   {
      OWERROR(OWERROR_CRC_NOT_SUPPORTED);
      return FALSE;
   }

   // check if read exceeds memory
   if (page > getNumberPagesNV(bank,SNum))
   {
      OWERROR(OWERROR_READ_OUT_OF_RANGE);
      return FALSE;
   }

   // see if need to access the device
   if(!rd_cont || !readPossible(bank,SNum))
   {

      // select the device
      if (!owAccess(portnum))
      {
         OWERROR(OWERROR_DEVICE_SELECT_FAIL);
         return FALSE;
      }

      // build start reading memory block
      raw_buf[0] = READ_PAGE_WITH_CRC;

      addr = page * PAGE_LENGTH_NVCRC + getStartingAddressNV(bank,SNum);

      raw_buf[1] = addr & 0xFF;
      raw_buf[2] = ((addr & 0xFFFF) >> 8) & 0xFF;

      // perform CRC16 on first part
      setcrc16(portnum,lastcrc16);
      for(i=0;i<3;i++)
         lastcrc16 = docrc16(portnum,raw_buf[i]);

      // do the first block for command, TA1, TA2
      if(!owBlock(portnum,FALSE,raw_buf,3))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }
   }

   // pre-fill with 0xFF 
   for(i=0;i<50;i++)
      read_buf[i] = 0xFF;

   // send block to read data + extra info? + crc
   if(!owBlock(portnum,FALSE,read_buf,PAGE_LENGTH_NVCRC + extra_len +
              2 + numVerifyBytes(bank,SNum)))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }


   // check the CRC
   setcrc16(portnum,lastcrc16);
   for(i=0;i<(PAGE_LENGTH_NVCRC+extra_len+2);i++)
      lastcrc16 = docrc16(portnum,read_buf[i]);

   if(lastcrc16 != 0xB001)
   {
      OWERROR(OWERROR_CRC_FAILED);
      return FALSE;
   }

   // extract the page data
   for(i=0;i<PAGE_LENGTH_NVCRC;i++)
      readBuf[i] = read_buf[i];

   // optional extract the extra info
   if (extra_len != 0)
      for(i=0;i<extra_len;i++)
         extra[i] = read_buf[i+PAGE_LENGTH_NVCRC];   
      
   return TRUE;
}

/**
 * Checks to see if a read is possible of a certian area of a device
 *
 * bank          to tell what memory bank of the ibutton to use.
 * SNum          the serial number for the part.
 *
 * return TRUE if the read can be done
 */
SMALLINT readPossible(SMALLINT bank, uchar *SNum)
{
   SMALLINT possible;
   
   switch(SNum[0])
   {
      case 0x18:
         if(bank == 2)
            possible = FALSE;
         else if(bank == 1)
            possible = FALSE;
         break;

      case 0x1D:
         if(bank == 2)
            possible = FALSE;
         break;

      default:
         possible = TRUE;
         break;
   }

   return possible;
}

/**
 * Gives the number of verification bytes for the device.
 *
 * bank          to tell what memory bank of the ibutton to use.
 * SNum          the serial number for the part.
 *
 * return the number of verification bytes a certain device has
 */
SMALLINT numVerifyBytes(SMALLINT bank, uchar *SNum)
{
   SMALLINT verify;

   switch(SNum[0])
   {
      case 0x18:
         if(bank == 2)
            verify = 8;
         else if(bank == 1)
            verify = 8;
         break;

      case 0x1D:
         if(bank == 2)
            verify = 8;
         else if(bank == 1)
            verify = 8;
         break;

      default: 
         verify = 0;
         break;
   }

   return verify;
}