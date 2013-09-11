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
//  mbNV.c - Reads and writes to memory locations for the NV memory bank.
//  version 1.00
//

// Include Files
#include "ownet.h"
#include "mbnv.h"
#include "mbscr.h"
#include "mbscree.h"
#include "mbscrex.h"
#include "mbscrcrc.h"
#include "mbsha.h"
#include "mbnvcrc.h"

// General command defines
#define READ_MEMORY_COMMAND_NV      0xF0

// Local defines
#define PAGE_LENGTH_NV 32
#define SIZE_NV        512

// Global variables
char    *bankDescriptionNV     = "Main Memory";
SMALLINT writeVerificationNV    = TRUE;
SMALLINT generalPurposeMemoryNV = TRUE;
SMALLINT readWriteNV            = TRUE;
SMALLINT writeOnceNV            = FALSE;
SMALLINT readOnlyNV             = FALSE;
SMALLINT nonVolatileNV          = TRUE;
SMALLINT needProgramPulseNV     = FALSE;
SMALLINT needPowerDeliveryNV    = FALSE;
SMALLINT ExtraInfoNV            = FALSE;
SMALLINT extraInfoLengthNV      = 0;
char    *extraInfoDescNV       = "";
SMALLINT pageAutoCRCNV          = FALSE;


/**
 * Read  memory in the current bank with no CRC checking (device or
 * data). The resulting data from this API may or may not be what is on
 * the 1-Wire device.  It is recommends that the data contain some kind
 * of checking (CRC) like in the readPagePacketNV() method or have
 * the 1-Wire device provide the CRC as in readPageCRCNV().  readPageCRCNV()
 * however is not supported on all memory types, see 'hasPageAutoCRCNV()'.
 * If neither is an option then this method could be called more
 * then once to at least verify that the same thing is read consistantly.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part that the read is
 *          to be done on.
 * str_add  starting physical address
 * rd_cont  if 'true' then device read is continued without
 *          re-selecting.  This can only be used if the new
 *          read() continious where the last one led off
 *          and it is inside a 'beginExclusive/endExclusive'
 *          block.
 * buff     byte array to place read data into
 * len      length in bytes to read
 *
 * @return 'true' if the read was complete
 */
SMALLINT readNV(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                SMALLINT rd_cont, uchar *buff, int len)
{

   int i,addr,pgs,extra;
   uchar raw_buf[PAGE_LENGTH_NV];

   // check if read exceeds memory
   if ((str_add + len) > getSizeNV(bank,SNum))
   {
      OWERROR(OWERROR_READ_OUT_OF_RANGE);
      return FALSE;
   }

   // see if need to access the device
   if (!rd_cont)
   {

      owSerialNum(portnum,SNum,FALSE);

      // select the device
      if (!owAccess(portnum))
      {
         OWERROR(OWERROR_DEVICE_SELECT_FAIL);
         return FALSE;
      }

      // build start reading memory block
      addr    = str_add + getStartingAddressNV(bank,SNum);

      raw_buf[0] = READ_MEMORY_COMMAND_NV;
      raw_buf[1] = addr & 0xFF;
      raw_buf[2] = ((addr & 0xFFFF) >> 8) & 0xFF;

      // do the first block for command, address
      if(!owBlock(portnum,FALSE,raw_buf,3))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

   }

   // pre-fill readBuf with 0xFF 
   pgs   = len / PAGE_LENGTH_NV;
   extra = len % PAGE_LENGTH_NV;

   for(i=0;i<(pgs*PAGE_LENGTH_NV)+extra;i++)
      buff[i] = 0xFF;

   // send second block to read data, return result
   if(!owBlock(portnum,FALSE,buff,len))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   return TRUE;
}

/**
 * Write  memory in the current bank.  It is recommended that
 * when writing  data that some structure in the data is created
 * to provide error free reading back with readNV().  Or the
 * method 'writePagePacketNV()' could be used which automatically
 * wraps the data in a length and CRC.
 *
 * When using on Write-Once devices care must be taken to write into
 * into empty space.  If write() is used to write over an unlocked
 * page on a Write-Once device it will fail.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part that the write is
 *          to be done on.
 * str_add  starting address
 * buff     byte array containing data to write
 * len      length in bytes to write
 *
 * @return 'true' if the write was complete.
 */
SMALLINT writeNV(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                 uchar *buff, int len)
{ 
   int i, room_left, startx, nextx, addr, pl;
   uchar raw_buf[PAGE_LENGTH_NV];
   uchar extra[PAGE_LENGTH_NV];

   // return if nothing to do
   if (len == 0)
      return TRUE;

   owSerialNum(portnum,SNum,FALSE);

   // check if write exceeds memory
   if ((str_add + len) > getSizeNV(bank,SNum))
   {
      OWERROR(OWERROR_WRITE_OUT_OF_RANGE);
      return FALSE;
   }

   // check if trying to write read only bank
   if(isReadOnlyNV(bank,portnum,SNum))
   {
      OWERROR(OWERROR_READ_ONLY);
      return FALSE;
   }

   // loop while still have pages to write
   startx = 0;
   nextx  = 0;   
   addr   = getStartingAddressNV(bank,SNum) + str_add;
   pl     = PAGE_LENGTH_NV;

   do
   {
      // calculate room left in current page
      room_left = pl - ((addr + startx) % pl);

      // check if block left will cross end of page
      if ((len - startx) > room_left)
         nextx = startx + room_left;
      else
         nextx = len;

      // write the page of data to scratchpad
      switch(SNum[0] & 0x7F)
      {
         case 0x18:
            if(bank == 3)
            {
               if(!writeScratchpd(portnum,addr+startx,&buff[startx],nextx-startx))
                  return FALSE;
            }
            else if(!writeScratchPadSHA(portnum,addr+startx,&buff[startx],nextx-startx))
               return FALSE;
            break;

         case 0x21: 
            if(!writeScratchPadEx(portnum,addr+startx,&buff[startx],nextx-startx))
               return FALSE;
            break;
         
         case 0x23:
            if(!writeScratchPadEE(portnum,addr+startx,&buff[startx],nextx-startx))
               return FALSE;
            break;

         default:
            if(!writeScratchpd(portnum,addr+startx,&buff[startx],nextx-startx))
               return FALSE;
            break;
      }

      // read to verify ok
      switch(SNum[0] & 0x7F)
      {
         case 0x18:
            if(bank == 3)
            {
               if(!readScratchpdExtra(portnum,raw_buf,pl,extra))
                  return FALSE;
            }
            else if(!readScratchPadCRC(portnum,raw_buf,pl,extra))
               return FALSE;
            break;

         case 0x21:
            if(!readScratchPadCRC(portnum,raw_buf,pl,extra))
               return FALSE;
            break;

         default:
            if(!readScratchpdExtra(portnum,raw_buf,pl,extra))
               return FALSE;
            break;
      }

      // check to see if the same
      for (i = 0; i < (nextx - startx); i++)
         if (raw_buf[i] != buff[i + startx])
         {
            OWERROR(OWERROR_READ_SCRATCHPAD_VERIFY);
            return FALSE;
         }

      // check to make sure that the address is correct // added the first unsigned jpe 
      if ((unsigned) (((extra[0] & 0x00FF) | ((extra[1] << 8) & 0x00FF00))
              & 0x00FFFF) != (unsigned) (addr + startx))
      {
         OWERROR(OWERROR_ADDRESS_READ_BACK_FAILED);
         return FALSE;
      }

      // do the copy
      switch(SNum[0] & 0x7F)
      {
         case 0x18:
            if(bank == 3)
            {
               if(!copyScratchpd(portnum,addr+startx,nextx-startx))
                  return FALSE;
            }
            else if(!copyScratchPadSHA(portnum,addr+startx,nextx-startx))
               return FALSE;
            break;

         case 0x1A:
            if(!copyScratchPadEx(portnum,addr+startx,nextx-startx))
               return FALSE;
            break;

         case 0x21:
            if(!copyScratchPadCRC(portnum,addr+startx,nextx-startx))
               return FALSE;
            break;

         case 0x23:
            if(!copyScratchPadEE(portnum,addr+startx,nextx-startx))
               return FALSE;
            break;

         default:
            if(!copyScratchpd(portnum,addr+startx,nextx-startx))
               return FALSE;
            break;
      }

      // point to next index
      startx = nextx;
   } while (nextx < len);

   return TRUE;
}

/**
 * Read  page in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketNV() method or have the 1-Wire device provide the
 * CRC as in readPageCRCNV().  readPageCRCNV() however is not
 * supported on all memory types, see 'hasPageAutoCRCNV()'.
 * If neither is an option then this method could be called more
 * then once to at least verify that the same thing is read consistantly.
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
SMALLINT readPageNV(SMALLINT bank, int portnum, uchar *SNum, int page,
                    SMALLINT rd_cont, uchar *buff)
{
   if(((SNum[0] == 0x18) && (bank != 3))|| (SNum[0] == 0x1A) || (SNum[0] == 0x1D) ||
      (SNum[0] == 0x21))
      return readPageNVCRC(bank,portnum,SNum,page,rd_cont,buff);

   if((page < 0) || (page > getNumberPagesNV(bank,SNum)))
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   return readNV(bank,portnum,SNum,page*PAGE_LENGTH_NV,
                 rd_cont,buff,PAGE_LENGTH_NV);
}

/**
 * Read  page with extra information in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketNV() method or have the 1-Wire device provide the
 * CRC as in readPageCRCNV().  readPageCRCNV() however is not
 * supported on all memory types, see 'hasPageAutoCRCNV()'.
 * If neither is an option then this method could be called more
 * then once to at least verify that the same thing is read consistantly.
 * See the method 'hasExtraInfoNV()' for a description of the optional
 * extra information some devices have.
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
SMALLINT readPageExtraNV(SMALLINT bank, int portnum, uchar *SNum, int page,
                    SMALLINT rd_cont, uchar *buff, uchar *extra)
{
   if(((SNum[0] == 0x18) && (bank != 3)) || (SNum[0] == 0x1A) || (SNum[0] == 0x1D) ||
      (SNum[0] == 0x21))
      return readPageExtraNVCRC(bank,portnum,SNum,page,rd_cont,buff,extra);

   OWERROR(OWERROR_EXTRA_INFO_NOT_SUPPORTED);
   return FALSE;
}

/**
 * Read a complete memory page with CRC verification provided by the
 * device with extra information.  Not supported by all devices.
 * See the method 'hasPageAutoCRC()'.
 * See the method 'haveExtraInfo()' for a description of the optional
 * extra information.
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
SMALLINT readPageExtraCRCNV(SMALLINT bank, int portnum, uchar *SNum, int page,
                            uchar *read_buff, uchar *extra)
{
   if(((SNum[0] == 0x18) && (bank != 3))|| (SNum[0] == 0x1A) || (SNum[0] == 0x1D) ||
      (SNum[0] == 0x21))
      return readPageExtraCRCNVCRC(bank,portnum,SNum,page,read_buff,extra);

   OWERROR(OWERROR_CRC_EXTRA_INFO_NOT_SUPPORTED);
   return FALSE;
}

/**
 * Read a complete memory page with CRC verification provided by the
 * device.  Not supported by all devices.  See the method
 * 'hasPageAutoCRCNV()'.
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
SMALLINT readPageCRCNV(SMALLINT bank, int portnum, uchar *SNum, int page, uchar *buff)
{
   if(((SNum[0] == 0x18) && (bank != 3)) || (SNum[0] == 0x1A) || (SNum[0] == 0x1D) ||
      (SNum[0] == 0x21))
      return readPageCRCNVCRC(bank,portnum,SNum,page,buff);

   OWERROR(OWERROR_CRC_NOT_SUPPORTED);
   return FALSE;
}

/**
 * Read a Universal Data Packet.
 *
 * The Universal Data Packet always starts on page boundaries but
 * can end anywhere in the page.  The structure specifies the length of
 * data bytes not including the length byte and the CRC16 bytes.
 * There is one length byte. The CRC16 is first initialized to
 * the page number.  This provides a check to verify the page that
 * was intended is being read.  The CRC16 is then calculated over
 * the length and data bytes.  The CRC16 is then inverted and stored
 * low byte first followed by the high byte.  This is structure is
 * used by this method to verify the data but is not returned, only
 * the data payload is returned.
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
 *
 * @return - returns '0' if the read page packet wasn't completed
 *                   '1' if the operation is complete.
 */
SMALLINT readPagePacketNV(SMALLINT bank, int portnum, uchar *SNum, int page,
                          SMALLINT rd_cont, uchar *buff, int *len)
{

   uchar raw_buf[PAGE_LENGTH_NV];
   ushort lastcrc16;
   int i;

   // read the  page
   if(!readPageNV(bank,portnum,SNum,page,rd_cont,&raw_buf[0]))
      return FALSE;

   // check if length is realistic
   if ((raw_buf[0] > getMaxPacketDataLengthNV(bank,SNum)) || (raw_buf[0] <= 0))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort) ((getStartingAddressNV(bank,SNum)/PAGE_LENGTH_NV) + page));
   for(i=0;i<(raw_buf[0]+3);i++)
      lastcrc16 = docrc16(portnum,raw_buf[i]);

   if(lastcrc16 == 0xB001)
   {
      for(i=1;i<raw_buf[0]+1;i++)
         buff[i-1] = raw_buf[i];

      // return the length
      *len = (int) raw_buf [0];
   }
   else
   {
      OWERROR(OWERROR_CRC_FAILED);
      return FALSE;
   }

   return TRUE;
}

/**
 * Read a Universal Data Packet and extra information.  See the
 * method 'readPagePacketNV()' for a description of the packet structure.
 * See the method 'hasExtraInfoNV()' for a description of the optional
 * extra information some devices have.
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
SMALLINT readPagePacketExtraNV(SMALLINT bank, int portnum, uchar *SNum,
                               int page, SMALLINT rd_cont, uchar *buff,
                               int *len, uchar *extra)
{
   if(((SNum[0] == 0x18) && (bank != 3)) || (SNum[0] == 0x1A) || (SNum[0] == 0x1D) ||
      (SNum[0] == 0x21))
      return readPagePacketExtraNVCRC(bank,portnum,SNum,page,rd_cont,buff,
                                      len,extra);

   OWERROR(OWERROR_PG_PACKET_WITHOUT_EXTRA);
   return FALSE;
}

/**
 * Write a Universal Data Packet.  See the method 'readPagePacketNV()'
 * for a description of the packet structure.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page the packet is being written to.
 * buff     byte array containing data that to write.
 * len      length of the packet
 *
 * @return - returns '0' if the write page packet wasn't completed
 *                   '1' if the operation is complete.
 */
SMALLINT writePagePacketNV(SMALLINT bank, int portnum, uchar *SNum, int page,
                           uchar *buff, int len)
{
   uchar raw_buf[PAGE_LENGTH_NV];
   int i;
   ushort crc;

   // make sure length does not exceed max
   if (len > getMaxPacketDataLengthNV(bank,SNum))
   {
      OWERROR(OWERROR_PACKET_LENGTH_EXCEEDS_PAGE);
      return FALSE;
   }

   // see if this bank is general read/write
   if (!isGeneralPurposeMemoryNV(bank,SNum))
   {
      OWERROR(OWERROR_NOT_GENERAL_PURPOSE);
      return FALSE;
   }

   // construct the packet to write
   raw_buf[0] = (uchar) len;

   for(i=1;i<(len+1);i++)
      raw_buf[i] = buff[i-1];

   setcrc16(portnum,(ushort) ((getStartingAddressNV(bank,SNum)/PAGE_LENGTH_NV) + page));
   for(i=0;i<(len+1);i++)
      crc = docrc16(portnum,raw_buf[i]);

   raw_buf[len + 1] = (uchar) ~crc & 0xFF;
   raw_buf[len + 2] = (uchar) ((~crc & 0xFFFF) >> 8) & 0xFF;

   // write the packet, return result
   if(!writeNV(bank,portnum,SNum,page*PAGE_LENGTH_NV,&raw_buf[0],len+3))
      return FALSE;

   return TRUE;
}

/**
 * Query to get the number of pages in current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  number of pages in current memory bank
 */
SMALLINT getNumberPagesNV(SMALLINT bank, uchar *SNum)
{
   int pages = 16;

   switch(SNum[0] & 0x7F)
   {
      case 0x04:
         if(bank == 1)
            pages = 16;
         else if(bank == 2)
            pages = 1;
         else if(bank == 3)
            pages = 3; 
         break;

      case 0x08:
         pages = 4;
         break;

      case 0x0A:
         pages = 64;
         break;

      case 0x0C:
         pages = 256;
         break;

      case 0x18:
         if(bank == 3)
            pages = 3;
         else if(bank == 2)
            pages = 8;
         else if(bank == 1)
            pages = 8;
         break;

      case 0x1A:
         if(bank == 2)
            pages = 4;
         else if(bank == 1)
            pages = 12;
         break;

      case 0x1D:
         if(bank >= 2)
            pages = 2;
         else if(bank == 1)
            pages = 12;
         break;

      case 0x21:
         if(bank == 5)
            pages = 64;
         else if(bank == 4)
            pages = 4;
         else if(bank == 3)
            pages = 3;
         else if(bank == 2)
            pages = 1;
         break;

      case 0x23:
         if(bank == 1)
            pages = 16;
         break;

      default:
         pages = 16;
         break;
   }

   return pages;
}

/**
 * Query to get the memory bank size in bytes.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  memory bank size in bytes.
 */
int getSizeNV(SMALLINT bank, uchar *SNum)
{
   int size = 512;

   switch(SNum[0] & 0x7F)
   {
      case 0x04:
         if(bank == 1)
            size = 512;
         else if(bank == 2)
            size = 32;
         else if(bank == 3)
            size = 96;
         break;

      case 0x06:
         size = 512;
         break;

      case 0x08:
         size = 128;
         break;

      case 0x0A:
         size = 2048;
         break;

      case 0x0C:
         size = 8192;
         break;

      case 0x18:
         if(bank == 3)
            size = 96;
         else if(bank == 2)
            size = 256;
         else if(bank == 1)
            size = 256;
         break;

      case 0x1A:
         if(bank == 2)
            size = 128;
         else if(bank == 1)
            size = 384;
         break;

      case 0x1D:
         if(bank <= 3)
            size = 64;
         else if(bank == 1)
            size = 384;
         break;

      case 0x21:
         if(bank == 5)
            size = 2048;
         else if(bank == 4)
            size = 128;
         else if(bank == 3)
            size = 96;
         else if(bank == 2)
            size = 32;
         break;

      case 0x23:
         if(bank == 1)
            size = 512;
         else if(bank == 2)
            size = 32;
         break;

      default:
         size = 512;
         break;
   }

   return size;
}

/**
 * Query to get the starting physical address of this bank.  Physical
 * banks are sometimes sub-divided into logical banks due to changes
 * in attributes.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  physical starting address of this logical bank.
 */
int getStartingAddressNV(SMALLINT bank, uchar *SNum)
{
   int start = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x04:
         if(bank == 2)
            start = 512;
         break;

      case 0x18:
         if(bank == 3)
            start = 608;
         else if(bank == 2)
            start = 256;
         break;

      case 0x1A:
         if(bank == 2)
            start = 384;
         break;

      case 0x1D:
         if(bank == 3)
            start = 448;
         else if(bank == 2)
            start = 384;
         break;

      case 0x21:
         if(bank == 5)
            start = 4096;
         else if(bank == 4)
            start = 2048;
         else if(bank == 3)
            start = 544;
         else if(bank == 2)
            start = 512;
         break;

      default:
         start = 0;
         break;
   }

   return start;
}

/**
 * Query to get page length in bytes in current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return   page length in bytes in current memory bank
 */
SMALLINT getPageLengthNV(SMALLINT bank, uchar *SNum)
{
   return PAGE_LENGTH_NV;
}

/**
 * Query to see get a string description of the current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  String containing the memory bank description
 */
char *getBankDescriptionNV(SMALLINT bank, uchar *SNum)
{
   switch(SNum[0] & 0x7F)
   {
      case 0x04:
         if(bank == 2)
            return "Clock/Alarm registers.";
         break;
   
      case 0x18:
         if(bank == 3)
            return "Write cycle counters and PRNG counter.";
         else if(bank == 2)
            return "Memory with write cycle counter.";
         break;

      case 0x1A:
         if(bank == 2)
            return "Memory with write cycle counter.";
         break;

      case 0x1D:
         if(bank == 3)
            return "Memory with externally triggered counter.";
         else if(bank == 2)
            return "Memory with write cycle counter.";
         break;

      case 0x21:
         if(bank == 5)
            return "Temperature log.";
         else if(bank == 4)
            return "Temperature Histogram.";
         else if(bank == 3)
            return "Alarm time stamps.";
         else if(bank == 2)
            return "Register control.";
         break;

      default:
         return bankDescriptionNV;
   }
   
   return bankDescriptionNV;
}

/**
 * Query to see if the current memory bank is general purpose
 * user memory.  If it is NOT then it is Memory-Mapped and writing
 * values to this memory will affect the behavior of the 1-Wire
 * device.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank is general purpose
 */
SMALLINT isGeneralPurposeMemoryNV(SMALLINT bank, uchar *SNum)
{
   SMALLINT gp = generalPurposeMemoryNV;

   switch(SNum[0] & 0x7F)
   {
      case 0x04:
         if(bank == 2)
            gp = FALSE;
         break;

      case 0x18:
         if(bank == 3)
            return FALSE;
         break;

      case 0x21:
         if(bank == 5)
            gp = FALSE;
         else if(bank == 4)
            gp = FALSE;
         else if(bank == 3)
            gp = FALSE;
         else if(bank == 2)
            gp = FALSE;
         break;

      default:
         gp = generalPurposeMemoryNV;
         break;
   }

   return gp;
}

/**
 * Query to see if current memory bank is read/write.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank is read/write
 */
SMALLINT isReadWriteNV(SMALLINT bank, int portnum, uchar *SNum)
{
   SMALLINT rw = readWriteNV;

   switch(SNum[0] & 0x7F)
   {
      case 0x18:
         if(bank == 3)
            rw = FALSE;
         break;

      case 0x21:
         if(bank == 5)
            rw = FALSE;
         else if(bank == 4)
            rw = FALSE;
         else if(bank == 3)
            rw = FALSE;
         break;

      default:
         rw = readWriteNV;
   }

   return rw;
}

/**
 * Query to see if current memory bank is write write once such
 * as with EPROM technology.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can only be written once
 */
SMALLINT isWriteOnceNV(SMALLINT bank, int portnum, uchar *SNum)
{
   return writeOnceNV;
}

/**
 * Query to see if current memory bank is read only.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can only be read
 */
SMALLINT isReadOnlyNV(SMALLINT bank, int portnum, uchar *SNum)
{
   SMALLINT ro = readOnlyNV;

   switch(SNum[0] & 0x7F)
   {
      case 0x18:
         if(bank == 3)
            ro = TRUE;
         break;

      case 0x21:
         if(bank == 5)
            ro = TRUE;
         else if(bank == 4)
            ro = TRUE;
         else if(bank == 3)
            ro = TRUE;
         break;

      default:
         ro = readOnlyNV;
         break;
   }

   return ro;
}

/**
 * Query to see if current memory bank non-volatile.  Memory is
 * non-volatile if it retains its contents even when removed from
 * the 1-Wire network.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank non volatile.
 */
SMALLINT isNonVolatileNV(SMALLINT bank, uchar *SNum)
{
   return nonVolatileNV;
}

/**
 * Query to see if current memory bank pages need the adapter to
 * have a 'ProgramPulse' in order to write to the memory.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if writing to the current memory bank pages
 *                 requires a 'ProgramPulse'.
 */
SMALLINT needsProgramPulseNV(SMALLINT bank, uchar *SNum)
{
   return needProgramPulseNV;
}

/**
 * Query to see if current memory bank pages need the adapter to
 * have a 'PowerDelivery' feature in order to write to the memory.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if writing to the current memory bank pages
 *                 requires 'PowerDelivery'.
 */
SMALLINT needsPowerDeliveryNV(SMALLINT bank, uchar *SNum)
{
   return needPowerDeliveryNV;
}

/**
 * Checks to see if this memory bank's pages deliver extra
 * information outside of the normal data space,  when read.  Examples
 * of this may be a redirection byte, counter, tamper protection
 * bytes, or SHA-1 result.  If this method returns true then the
 * methods with an 'extraInfo' parameter can be used.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  true if reading the this memory bank's
 *               pages provides extra information
 */
SMALLINT hasExtraInfoNV(SMALLINT bank, uchar *SNum)
{
   SMALLINT extra = ExtraInfoNV;

   switch(SNum[0] & 0x7F)
   {
      case 0x18:
         if((bank == 2) || (bank == 1))
            extra = TRUE;
         break;

      case 0x1A:
         if((bank == 2) || (bank == 1))
            extra = TRUE;
         break;

      case 0x1D:
         if((bank == 3) || (bank == 2))
            extra = TRUE;
         break;

      default:
         extra = ExtraInfoNV;
         break;
   }

   return extra;
}

/**
 * Query to get the length in bytes of extra information that
 * is read when read a page in the current memory bank.  See
 * 'hasExtraInfoNV()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  number of bytes in Extra Information read when reading
 *          pages in the current memory bank.
 */
SMALLINT getExtraInfoLengthNV(SMALLINT bank, uchar *SNum)
{
   SMALLINT len = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x18:
         if(bank == 2)
            len = 8;
         else if(bank == 1)
            len = 8;
         break;

      case 0x1A:
         if(bank == 2)
            len = 8;
         else if(bank == 1)
            len = 8;
         break;

      case 0x1D:
         if(bank == 3)
            len = 8;
         else if(bank == 2)
            len = 8;
         break;

      default:
         len = extraInfoLengthNV;
         break;
   }

   return len;
}

/**
 * Query to get a string description of what is contained in
 * the Extra Informationed return when reading pages in the current
 * memory bank.  See 'hasExtraInfoNV()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return string describing extra information.
 */
char *getExtraInfoDescNV(SMALLINT bank, uchar *SNum)
{
   switch(SNum[0] & 0x7F)
   {
      case 0x18:
         if(bank == 2)
            return "Write cycle counter.";
         break;

      case 0x1A:
         if(bank == 2)
            return "Write cycle counter.";
         break;

      case 0x1D:
         if(bank == 2)
            return "Write cycle counter.";
         else if(bank == 3)
            return "Externally triggered counter.";
         break;

      default:
         return extraInfoDescNV;
   }

   return extraInfoDescNV;
}

/**
 * Query to see if current memory bank pages can be read with
 * the contents being verified by a device generated CRC.
 * This is used to see if the 'ReadPageCRCNV()' can be used.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can be read with self
 *          generated CRC.
 */
SMALLINT hasPageAutoCRCNV(SMALLINT bank, uchar *SNum)
{
   SMALLINT autoCRC = pageAutoCRCNV;

   switch(SNum[0] & 0x7F)
   {
      case 0x18:
         if((bank == 1) || (bank == 2))
            autoCRC = TRUE;
         break;

      case 0x1A:
         if((bank == 1) || (bank == 2))
            autoCRC = TRUE;
         break;

      case 0x1D:
         if((bank > 0) && (bank < 4))
            autoCRC = TRUE;
         break;

      case 0x21:
         if((bank > 0) && (bank < 6))
            autoCRC = TRUE;
         break;

      default:
         autoCRC = pageAutoCRCNV;
         break;
   }

   return autoCRC;
}

/**
 * Query to get Maximum data page length in bytes for a packet
 * read or written in the current memory bank.  See the 'ReadPagePacket()'
 * and 'WritePagePacket()' methods.  This method is only usefull
 * if the current memory bank is general purpose memory.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  max packet page length in bytes in current memory bank
 */
SMALLINT getMaxPacketDataLengthNV(SMALLINT bank, uchar *SNum)
{
   return PAGE_LENGTH_NV - 3;
}

/**
 * Query to see if current memory bank pages can be redirected
 * to another pages.  This is mostly used in Write-Once memory
 * to provide a means to update.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank pages can be redirected
 *          to a new page.
 */
SMALLINT canRedirectPageNV(SMALLINT bank, uchar *SNum)
{
   return FALSE;
}

/**
 * Query to see if current memory bank pages can be locked.  A
 * locked page would prevent any changes to the memory.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank pages can be redirected
 *          to a new page.
 */
SMALLINT canLockPageNV(SMALLINT bank, uchar *SNum)
{
   return FALSE;
}

/**
 * Query to see if current memory bank pages can be locked from
 * being redirected.  This would prevent a Write-Once memory from
 * being updated.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank pages can be locked from
 *          being redirected to a new page.
 */
SMALLINT canLockRedirectPageNV(SMALLINT bank, uchar *SNum)
{
   return FALSE;
}

