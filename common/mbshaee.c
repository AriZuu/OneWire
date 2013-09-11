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
//  mbSHAEE.c - Reads and writes to memory locations for the SHAEE memory bank.
//  version 1.00
//

// Include Files
#include "ownet.h"
#include "mbshaee.h"

long KTN (int n);
long NLF (long B, long C, long D, int n);

// General command defines
#define READ_MEMORY_SHAEE       0xF0
#define WRITE_SCRATCHPAD_SHAEE  0x0F
#define READ_SCRATCHPAD_SHAEE   0xAA
#define LOAD_FIRST_SECRET_SHAEE 0x5A
#define COPY_SCRATCHPAD_SHAEE   0x55
#define READ_AUTH_PAGE_SHAEE    0xA5
#define NEXT_SECRET_SHAEE       0x33

// Local defines
#define PAGE_LENGTH_SHAEE 32
#define SIZE_SHAEE_MB_0_1 32
#define SIZE_SHAEE_MB_2   32

// Global variables
char    *bankDescriptionSHAEE     = "";
SMALLINT writeVerificationSHAEE    = TRUE;
SMALLINT generalPurposeMemorySHAEE = TRUE;
SMALLINT readWriteSHAEE            = TRUE;   // check memory status
SMALLINT writeOnceSHAEE            = FALSE;  // check memory status
SMALLINT readOnlySHAEE             = FALSE;  // check memory status
SMALLINT nonVolatileSHAEE          = TRUE;
SMALLINT needProgramPulseSHAEE     = FALSE;
SMALLINT needPowerDeliverySHAEE    = FALSE;
SMALLINT ExtraInfoSHAEE            = TRUE;   // memory status page doesn't, so check bank
SMALLINT extraInfoLengthSHAEE      = 20;
char    *extraInfoDescSHAEE       = "The MAC for the SHA Engine";
SMALLINT pageAutoCRCSHAEE          = TRUE;   // memory status page doesn't, so check bank

// Global variables to write and read from part
uchar    local_secret[8];
uchar    challenge[8];
uchar    currentSN[8];
int      checked = FALSE;
uchar    status[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};


/**
 * Read  memory in the current bank with no CRC checking (device or
 * data). The resulting data from this API may or may not be what is on
 * the 1-Wire device.  It is recommends that the data contain some kind
 * of checking (CRC) like in the readPagePacketSHAEE() method or have
 * the 1-Wire device provide the CRC as in readPageCRCSHAEE().  readPageCRCSHAEE()
 * however is not supported on all memory types, see 'hasPageAutoCRCSHAEE()'.
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
SMALLINT readSHAEE(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                   SMALLINT rd_cont, uchar *buff, int len)
{
   int i;
   int addr;
   uchar raw_buf[3];

   // check if read exceeds memory
   if ((str_add + len) > (PAGE_LENGTH_SHAEE * getNumberPagesSHAEE(bank,SNum)))
   {
      OWERROR(OWERROR_READ_OUT_OF_RANGE);
      return FALSE;
   }

   // see if need to access the device
   if (!rd_cont)
   {
      // select the device
      if (!owAccess(portnum))
      {
         OWERROR(OWERROR_DEVICE_SELECT_FAIL);
         return FALSE;
      }

      // build start reading memory block
      addr    = str_add + getStartingAddressSHAEE(bank,SNum);

      raw_buf[0] = READ_MEMORY_SHAEE;
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
   for(i=0;i<len;i++)
      buff[i] = 0xFF;

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
 * to provide error free reading back with readSHAEE().  Or the
 * method 'writePagePacketSHAEE()' could be used which automatically
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
SMALLINT writeSHAEE(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                    uchar *buff, int len)
{
   int i, room_left;
   int startx, nextx, abs_addr, pl;
   uchar raw_buf[8];
   uchar extra[20];
   uchar memory[64];

   // return if nothing to do
   if (len == 0)
      return TRUE;

   // check if write exceeds memory
   if ((str_add + len) > getSizeSHAEE(bank,SNum))
   {
      OWERROR(OWERROR_WRITE_OUT_OF_RANGE);
      return FALSE;
   }

   // check if trying to write read only bank
   if(isReadOnlySHAEE(bank,portnum,SNum))
   {
      OWERROR(OWERROR_READ_ONLY);
      return FALSE;
   }

   // loop while still have pages to write
   startx   = 0;
   nextx    = 0;   // (start and next index into writeBuf)
   pl       = 8;
   abs_addr = getStartingAddressSHAEE(bank,SNum) + str_add;

   if(!readSHAEE(bank,portnum,SNum,(str_add/PAGE_LENGTH_SHAEE)*PAGE_LENGTH_SHAEE,
                 FALSE,memory,PAGE_LENGTH_SHAEE))
      return FALSE;

   do
   {
      // calculate room left in current page
      room_left = pl - ((abs_addr + startx) % pl);

      // check if block left will cross end of page
      if ((len - startx) > room_left)
         nextx = startx + room_left;
      else
         nextx = len;

      if((str_add+startx) >= PAGE_LENGTH_SHAEE)
      {
         if(!readSHAEE(bank,portnum,SNum,((str_add+startx)/PAGE_LENGTH_SHAEE)*PAGE_LENGTH_SHAEE,
                       FALSE,memory,PAGE_LENGTH_SHAEE))
            return FALSE;
      }

      if((str_add+startx) >= PAGE_LENGTH_SHAEE)
         for(i=0;i<8;i++)
            raw_buf[i] = memory[((((startx+str_add)/8)*8)-32)+i];
      else
         for(i=0;i<8;i++)
            raw_buf[i] = memory[(((startx+str_add)/8)*8)+i];

      if((nextx-startx) == 8)
         for(i=0;i<8;i++)
            raw_buf[i] = buff[startx+i];
      else
         if(((str_add+nextx)%8) == 0)
            for(i=0;i<(8-((str_add+startx)%8));i++)
               raw_buf[((str_add+startx)%8)+i] = buff[startx+i];
         else
            for(i=0;i<(((str_add+nextx)%8)-((str_add+startx)%8));i++)
               raw_buf[((str_add+startx)%8)+i] = buff[startx+i];

      // write the page of data to scratchpad
      if(!writeSpad(portnum,abs_addr+startx+room_left-8,raw_buf,8))
         return FALSE;

      // Copy data from scratchpad into memory
      if(abs_addr > 127)
         for(i=0;i<8;i++)
            memory[i] = local_secret[i];

      if(!copySpad(portnum,abs_addr+startx+room_left-8,SNum,extra,memory))
         return FALSE;

      // the status page starts on 136
      if((abs_addr+startx+room_left-8) == 136)
      {
         for(i=0;i<8;i++)
            status[i] = memory[i];
         for(i=0;i<8;i++)
            currentSN[i] = SNum[i];
         checked = TRUE;
      }

      if(str_add >= PAGE_LENGTH_SHAEE)
         for(i=0;i<8;i++)
            memory[((((startx+str_add)/8)*8)-32)+i] = raw_buf[i];
      else
         for(i=0;i<8;i++)
            memory[(((startx+str_add)/8)*8)+i] = raw_buf[i];

      // point to next index
      startx = nextx;
   }
   while (nextx < len);

   return TRUE;
}

/**
 * Read  page in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketSHAEE() method or have the 1-Wire device provide the
 * CRC as in readPageCRCSHAEE().  readPageCRCSHAEE() however is not
 * supported on all memory types, see 'hasPageAutoCRCSHAEE()'.
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
SMALLINT readPageSHAEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                       SMALLINT rd_cont, uchar *buff)
{
   return readSHAEE(bank,portnum,SNum,page*PAGE_LENGTH_SHAEE,FALSE,buff,PAGE_LENGTH_SHAEE);
}

/**
 * Read  page with extra information in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketSHAEE() method or have the 1-Wire device provide the
 * CRC as in readPageCRCSHAEE().  readPageCRCSHAEE() however is not
 * supported on all memory types, see 'hasPageAutoCRCSHAEE()'.
 * If neither is an option then this method could be called more
 * then once to at least verify that the same thing is read consistantly.
 * See the method 'hasExtraInfoSHAEE()' for a description of the optional
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
SMALLINT readPageExtraSHAEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                            SMALLINT rd_cont, uchar *buff, uchar *extra)
{
   int addr;

   if(!hasExtraInfoSHAEE(bank,SNum))
   {
      OWERROR(OWERROR_EXTRA_INFO_NOT_SUPPORTED);
      return FALSE;
   }

   // build start reading memory block
   addr = page*PAGE_LENGTH_SHAEE;
   if(!ReadAuthPage(bank,portnum,addr,SNum,buff,extra))
      return FALSE;

   return TRUE;
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
SMALLINT readPageExtraCRCSHAEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                               uchar *read_buff, uchar *extra)
{
   int addr;

   if(!hasPageAutoCRCSHAEE(bank,SNum))
   {
      OWERROR(OWERROR_CRC_EXTRA_INFO_NOT_SUPPORTED);
      return FALSE;
   }

   // build start reading memory block
   addr = page * PAGE_LENGTH_SHAEE;
   if(!ReadAuthPage(bank,portnum,addr,SNum,read_buff,extra))
      return FALSE;

   return TRUE;
}

/**
 * Read a complete memory page with CRC verification provided by the
 * device.  Not supported by all devices.  See the method
 * 'hasPageAutoCRCSHAEE()'.
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
SMALLINT readPageCRCSHAEE(SMALLINT bank, int portnum, uchar *SNum, int page, uchar *buff)
{
   return readSHAEE(bank,portnum,SNum,page*PAGE_LENGTH_SHAEE,FALSE,buff,PAGE_LENGTH_SHAEE);
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
 * the length and data bytes.  The CRC16 is then iNVerted and stored
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
SMALLINT readPagePacketSHAEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                             SMALLINT rd_cont, uchar *buff, int *len)
{
   uchar raw_buf[PAGE_LENGTH_SHAEE];
   ushort lastcrc16;
   int i;

   // read the  page
   if(!readPageSHAEE(bank,portnum,SNum,page,rd_cont,&raw_buf[0]))
      return FALSE;
      
   // check if length is realistic
   if((raw_buf[0] & 0x00FF) > getMaxPacketDataLengthSHAEE(bank,SNum))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }
   else if(raw_buf[0] == 0)
   {
      *len = raw_buf[0];
      return TRUE;
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort)((getStartingAddressSHAEE(bank,SNum)/PAGE_LENGTH_SHAEE) + page));
   for(i=0;i<(raw_buf[0]+3);i++)
      lastcrc16 = docrc16(portnum,raw_buf[i]);

   if(lastcrc16 == 0x0000B001)
   {
      // extract the data out of the packet
      for(i=0;i<raw_buf[0];i++)
         buff[i] = raw_buf[i+1];

      // extract the length
      *len = (int) raw_buf[0];
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
 * method 'readPagePacketSHAEE()' for a description of the packet structure.
 * See the method 'hasExtraInfoSHAEE()' for a description of the optional
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
SMALLINT readPagePacketExtraSHAEE(SMALLINT bank, int portnum, uchar *SNum,
                                  int page, SMALLINT rd_cont, uchar *buff,
                                  int *len, uchar *extra)
{
   uchar raw_buf[PAGE_LENGTH_SHAEE];
   int addr;
   int i;
   ushort lastcrc16;

   if(!hasExtraInfoSHAEE(bank,SNum))
   {
      OWERROR(OWERROR_EXTRA_INFO_NOT_SUPPORTED);
      return FALSE;
   }

   addr = page * PAGE_LENGTH_SHAEE;

   // read the  page
   if(!ReadAuthPage(bank,portnum,addr,SNum,&raw_buf[0],extra))
      return FALSE;
      
   // check if length is realistic
   if ((raw_buf[0] & 0x00FF) > getMaxPacketDataLengthSHAEE(bank,SNum))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }
   else if(raw_buf[0] == 0)
   {
      *len = raw_buf[0];
      return TRUE;
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort)((getStartingAddressSHAEE(bank,SNum)/PAGE_LENGTH_SHAEE) + page));
   for(i=0;i<(raw_buf[0]+3);i++)
      lastcrc16 = docrc16(portnum,raw_buf[i]);

   if(lastcrc16 == 0x0000B001)
   {
      // extract the data out of the packet
      for(i=0;i<raw_buf[0];i++)
         buff[i] = raw_buf[i+1];

      // return the length
      *len = (int) raw_buf[0];
   }
   else
   {
      OWERROR(OWERROR_CRC_FAILED);
      return FALSE;
   }

   return TRUE;

}

/**
 * Write a Universal Data Packet.  See the method 'readPagePacketSHAEE()'
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
SMALLINT writePagePacketSHAEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                              uchar *buff, int len)
{
   uchar raw_buf[PAGE_LENGTH_SHAEE];
   ushort crc;
   int i;

   // make sure length does not exceed max
   if(len > getMaxPacketDataLengthSHAEE(bank,SNum))
   {
      OWERROR(OWERROR_PACKET_LENGTH_EXCEEDS_PAGE);
      return FALSE;
   }

   // see if this bank is general read/write
   if (!isGeneralPurposeMemorySHAEE(bank,SNum))
   {
      OWERROR(OWERROR_NOT_GENERAL_PURPOSE);
      return FALSE;
   }

   // construct the packet to write
   raw_buf[0] = (uchar) len;

   for(i=0;i<len;i++)
      raw_buf[i+1] = buff[i];

   // calculate crc
   setcrc16(portnum,(ushort)((getStartingAddressSHAEE(bank,SNum)/PAGE_LENGTH_SHAEE) + page));
   for(i=0;i<(len+1);i++)
      crc = docrc16(portnum,raw_buf[i]);

   raw_buf[len + 1] = (uchar) ~crc & 0xFF;
   raw_buf[len + 2] = (uchar) ((~crc & 0xFFFF) >> 8) & 0xFF;

   // write the packet, return result
   if(!writeSHAEE(bank,portnum,SNum,page*PAGE_LENGTH_SHAEE,raw_buf,len+3))
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
SMALLINT getNumberPagesSHAEE(SMALLINT bank, uchar *SNum)
{
   SMALLINT pages;

   if(bank == 2)
      pages = 2;
   else
      pages = 1;

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
int getSizeSHAEE(SMALLINT bank, uchar *SNum)
{
   int size;

   if(bank == 2)
      size = 64;
   else
      size = 32;

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
int getStartingAddressSHAEE(SMALLINT bank, uchar *SNum)
{
   int start = 0;

   switch(bank)
   {
      case 0:
         start = 0;
         break;

      case 1:
         start = 32;
         break;

      case 2:
         start = 64;
         break;

      case 3:
         start = 128;
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
SMALLINT getPageLengthSHAEE(SMALLINT bank, uchar *SNum)
{
   return PAGE_LENGTH_SHAEE;
}

/**
 * Query to see get a string description of the current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  String containing the memory bank description
 */
char *getBankDescriptionSHAEE(SMALLINT bank, uchar *SNum)
{
   switch(bank)
   {
      case 0:
         return "Page Zero with write protection.";
         break;
   
      case 1:
         return "Page One with EPROM mode and write protection.";
         break;

      case 2:
         return "Page Two and Three with write protection.";
         break;

      case 3:
         return "Status Page that contains the secret and the status.";
         break;

      default:
         return bankDescriptionSHAEE;
         break;
   }
   
   return bankDescriptionSHAEE;
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
SMALLINT isGeneralPurposeMemorySHAEE(SMALLINT bank, uchar *SNum)
{
   SMALLINT gp = generalPurposeMemorySHAEE;

   if(bank == 3)
      gp = FALSE;

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
SMALLINT isReadWriteSHAEE(SMALLINT bank, int portnum, uchar *SNum)
{
   SMALLINT rw = readWriteSHAEE;
   int      i;

   for(i=0;i<8;i++)
   {
      if(SNum[i] != currentSN[i])
      {
         checked = FALSE;
         break;
      }
   }

   if(!checked)
   {
      for(i=0;i<8;i++)
         currentSN[i] = SNum[i];

      if(!readSHAEE(3,portnum,SNum,8,FALSE,&status[0],8))
         return FALSE;

      checked = TRUE;
   }

   switch(bank)
   {
      case 0:
         if( ((status[1] == 0xAA) || (status[1] == 0x55)) ||
             ((status[5] == 0xAA) || (status[5] == 0x55))    )
             rw = FALSE;
         break;

      case 1:
         if((status[5] == 0xAA) || (status[5] == 0x55))
            rw = FALSE;
         break;

      case 2:
         if((status[5] == 0xAA) || (status[5] == 0x55))
            rw = FALSE;
         break;

      case 3:
         rw = FALSE;
         break;

      default:
         rw = readWriteSHAEE;
         break;
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
SMALLINT isWriteOnceSHAEE(SMALLINT bank, int portnum, uchar *SNum)
{
   SMALLINT once = writeOnceSHAEE;
   SMALLINT same = FALSE;
   int i;

   if(bank == 1)
   {
      for(i=0;i<8;i++)
      {
         if(SNum[i] == currentSN[i])
         {
            if(i == 7)
               same = TRUE;
         }
         else
         {
            checked = FALSE;
            break;
         }
      }


      if(!checked && !same)
      {
         for(i=0;i<8;i++)
            currentSN[i] = SNum[i];

         if(!readSHAEE(3,portnum,SNum,8,FALSE,&status[0],8))
            return FALSE;

         if((status[4] == 0xAA) || (status[4] == 0x55))
         {
            checked = TRUE;
            once = TRUE;
         }
      }
      else
      {
         if((status[4] == 0xAA) || (status[4] == 0x55))
            once = TRUE;
      }
   }

   return once;
}

/**
 * Query to see if current memory bank is read only.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can only be read
 */
SMALLINT isReadOnlySHAEE(SMALLINT bank, int portnum, uchar *SNum)
{
   SMALLINT ro = readOnlySHAEE;
   int i;

   for(i=0;i<8;i++)
   {
      if(SNum[i] != currentSN[i])
      {
         checked = FALSE;
         break;
      }
   }

   if(!checked)
   {
      for(i=0;i<8;i++)
         currentSN[i] = SNum[i];

      if(!readSHAEE(3,portnum,SNum,8,FALSE,&status[0],8))
         return FALSE;

      checked = TRUE;
   }

   switch(bank)
   {
      case 0:         
         if( ((status[1] == 0xAA) || (status[1] == 0x55)) ||
             ((status[5] == 0xAA) || (status[5] == 0x55)) )
            ro = TRUE;
         break;

      case 1:
         if((status[1] == 0xAA) || (status[1] == 0x55))
            ro = TRUE;
         break;

      case 2:
         if((status[1] == 0xAA) || (status[1] == 0x55))
            ro = TRUE;
         break;

      default:
         ro = readOnlySHAEE;
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
SMALLINT isNonVolatileSHAEE(SMALLINT bank, uchar *SNum)
{
   return nonVolatileSHAEE;
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
SMALLINT needsProgramPulseSHAEE(SMALLINT bank, uchar *SNum)
{
   return needProgramPulseSHAEE;
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
SMALLINT needsPowerDeliverySHAEE(SMALLINT bank, uchar *SNum)
{
   return needPowerDeliverySHAEE;
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
SMALLINT hasExtraInfoSHAEE(SMALLINT bank, uchar *SNum)
{
   SMALLINT extra = ExtraInfoSHAEE;

   if(bank == 3)
      extra = FALSE;

   return extra;
}

/**
 * Query to get the length in bytes of extra information that
 * is read when read a page in the current memory bank.  See
 * 'hasExtraInfoSHAEE()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  number of bytes in Extra Information read when reading
 *          pages in the current memory bank.
 */
SMALLINT getExtraInfoLengthSHAEE(SMALLINT bank, uchar *SNum)
{
   SMALLINT len = extraInfoLengthSHAEE;

   if(bank == 3)
      len = 0;

   return len;
}

/**
 * Query to get a string description of what is contained in
 * the Extra Informationed return when reading pages in the current
 * memory bank.  See 'hasExtraInfoSHAEE()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return string describing extra information.
 */
char *getExtraInfoDescSHAEE(SMALLINT bank, uchar *SNum)
{
   if(bank == 3)
      return "";

   return extraInfoDescSHAEE;
}

/**
 * Query to see if current memory bank pages can be read with
 * the contents being verified by a device generated CRC.
 * This is used to see if the 'ReadPageCRCSHAEE()' can be used.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can be read with self
 *          generated CRC.
 */
SMALLINT hasPageAutoCRCSHAEE(SMALLINT bank, uchar *SNum)
{
   SMALLINT autoCRC = pageAutoCRCSHAEE;

   if(bank == 3)
      autoCRC = FALSE;

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
SMALLINT getMaxPacketDataLengthSHAEE(SMALLINT bank, uchar *SNum)
{
   return PAGE_LENGTH_SHAEE - 3;
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
SMALLINT canRedirectPageSHAEE(SMALLINT bank, uchar *SNum)
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
SMALLINT canLockPageSHAEE(SMALLINT bank, uchar *SNum)
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
SMALLINT canLockRedirectPageSHAEE(SMALLINT bank, uchar *SNum)
{
   return FALSE;
}


// ------------------------
// Extras
// ------------------------

/**
 * Write to the Scratch Pad, which is a max of 8 bytes.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * addr     the address to write the data to
 * out_buf  byte array to write into scratch pad
 * len      length of the write data
 *
 * @return - returns 'TRUE' if scratch pad was written correctly
 *                   and 'FALSE' if not.
 */
SMALLINT writeSpad(int portnum, int addr, uchar *out_buf, int len)
{
   uchar send_block[64];
   ushort lastcrc16;
   int i, cnt;
      
   cnt = 0;

   // access the device 
   if(owAccess(portnum))
   { 
      send_block[cnt++] = WRITE_SCRATCHPAD_SHAEE;
      send_block[cnt++] = addr & 0xFF;
      send_block[cnt++] = ((addr & 0xFFFF) >> 8) & 0xFF;

      for(i=0;i<len;i++)
         send_block[cnt++] = out_buf[i];

      send_block[cnt++] = 0xFF;
      send_block[cnt++] = 0xFF;


      if(!owBlock(portnum,FALSE,send_block,cnt))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

      // verify the CRC is correct
      setcrc16(portnum,0);
      for(i=0;i<cnt;i++)
         lastcrc16 = docrc16(portnum,send_block[i]);

      if(lastcrc16 != 0x0000B001)
      {
         OWERROR(OWERROR_CRC_FAILED);
         return FALSE;
      }
   }
   else
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   return TRUE;   
}

/**
 * Copy what is in the Sratch Pad, which is a max of 8 bytes to a certain
 * address in memory.
 *
 * portnum       the port number of the port being used for the
 *                  1-Wire Network.
 * addr          the address to copy the data to
 * SNum          the serial number for the part.
 * extra_buf     byte array to write the calculated MAC
 * memory        byte array for memory of the page to be copied to
 *
 * @return - returns 'TRUE' if the scratch pad was copied correctly 
 *                   and 'FALSE' if not.
 */
SMALLINT copySpad(int portnum, int addr, uchar *SNum, uchar *extra_buf, uchar *memory)
{
   short send_cnt=0;
   ushort tmpadd;
   uchar send_block[80];
   int i;
   uchar scratch[8],es,test;
   unsigned MT[64];
   long A,B,C,D,E;
   long Temp;

   for(i=0;i<4;i++)
      MT[i] = local_secret[i];

   for(i=4;i<32;i++)
      MT[i] = memory[i-4];

   if(readSpad(portnum, &tmpadd, &es, scratch))
      for(i=32;i<40;i++)
         MT[i] = scratch[i-32];
   else
      return FALSE;

   MT[40] = (addr & 0xf0) >> 5;

   for(i=41;i<48;i++)
      MT[i] = SNum[i-41];

   for(i=48;i<52;i++)
      MT[i] = local_secret[i-44];

   for(i=52;i<55;i++)
      MT[i] = 0xff;

   MT[55] = 0x80;

   for(i=56;i<62;i++)
      MT[i] = 0x00;

   MT[62] = 0x01;
   MT[63] = 0xB8;

   ComputeSHA(MT,&A,&B,&C,&D,&E);

   // access the device
   if(owAccess(portnum))
   {
      // Copy command
      send_block[send_cnt++] = 0x55;

      // address 1
      send_block[send_cnt++] = (uchar)(tmpadd & 0xFF);
      // address 2
      send_block[send_cnt++] = (uchar)((tmpadd >> 8) & 0xFF);

      // ending address with data status
      send_block[send_cnt++] = es;

      if(owBlock(portnum,FALSE,send_block,send_cnt))
      {
         send_cnt = 0;

         // sending MAC
         Temp = E;
         for(i=0;i<4;i++)
         {
            send_block[send_cnt++] = (uchar) (Temp & 0x000000FF);
            extra_buf[i] = send_block[send_cnt-1];
            Temp >>= 8;
         }

         Temp = D;
         for(i=0;i<4;i++)
         {
            send_block[send_cnt++] = (uchar) (Temp & 0x000000FF);
            extra_buf[i] = send_block[send_cnt-1];
            Temp >>= 8;
         }

         Temp = C;
         for(i=0;i<4;i++)
         {
            send_block[send_cnt++] = (uchar) (Temp & 0x000000FF);
            extra_buf[i] = send_block[send_cnt-1];
            Temp >>= 8;
         }

         Temp = B;
         for(i=0;i<4;i++)
         {
            send_block[send_cnt++] = (uchar) (Temp & 0x000000FF);
            extra_buf[i] = send_block[send_cnt-1];
            Temp >>= 8;
         }

         Temp = A;
         for(i=0;i<4;i++)
         {
            send_block[send_cnt++] = (uchar) (Temp & 0x000000FF);
            extra_buf[i] = send_block[send_cnt-1];
            Temp >>= 8;
         }

         msDelay(2);

         if(owBlock(portnum,FALSE,send_block,send_cnt))
         {
            msDelay(10);

            test = owReadByte(portnum);

            if((test == 0xAA) || (test == 0x55))
               return TRUE;
            else
            {
               if(test == 0xFF)
               {
                  OWERROR(OWERROR_WRITE_PROTECTED);
                  return FALSE;
               }
               else if(test == 0x00)
               {
                  OWERROR(OWERROR_NONMATCHING_MAC);
                  return FALSE;
               }
            }
         }

         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;

      }
      else
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

   }
   else
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }


   return FALSE;
}

/**
 * Read from the Scratch Pad, which is a max of 8 bytes.
 *
 * portnum       number 0 to MAX_PORTNUM-1.  This number is provided to
 *                  indicate the symbolic port number.
 * addr          the address to read the data
 * es            offset byte read from scratchpad
 * data          data buffer read from scratchpad
 *
 * @return - returns 'TRUE' if the scratch pad was read correctly
 *                   and 'FALSE' if not
 */
SMALLINT readSpad(int portnum, ushort *addr, uchar *es, uchar *data)
{
   short send_cnt=0;
   uchar send_block[50];
   int i;
   ushort lastcrc16;

   // access the device
   if(owAccess(portnum))
   {
      // read scratchpad command
      send_block[send_cnt++] = 0xAA;
      // now add the read bytes for data bytes and crc16
      for(i=0; i<13; i++)
         send_block[send_cnt++] = 0xFF;

      // now send the block
      if (owBlock(portnum,FALSE,send_block,send_cnt))
      {
         // copy data to return buffers
         *addr = (send_block[2] << 8) | send_block[1];
         *es = send_block[3];

         // calculate CRC16 of result
         setcrc16(portnum,0);
         for (i = 0; i < send_cnt ; i++)
            lastcrc16 = docrc16(portnum,send_block[i]);

         // verify CRC16 is correct
         if (lastcrc16 == 0xB001)
         {
            for (i = 0; i < 8; i++)
               data[i] = send_block[4 + i];
            // success
            return TRUE;
         }
         else
         {
            OWERROR(OWERROR_CRC_FAILED);
            return FALSE;
         }
      }
      else
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

   }
   else
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }


   return FALSE;
}

/**
 *  Reads authenticated page.
 *
 * portnum       number 0 to MAX_PORTNUM-1.  This number is provided to
 *                  indicate the symbolic port number.
 * addr          address of the data to be read along with Physical address
 * SNum          the serial number for the part.
 * data          the data read from the address
 * extra         the MAC calculated for this function
 *
 * @return - returns 'TRUE' if the page was read and had the correct MAC
 *                   and 'FALSE' if not.
 */
SMALLINT ReadAuthPage(SMALLINT bank, int portnum, int addr, uchar *SNum, 
                      uchar *data, uchar *extra)
{
   short send_cnt=0;
   uchar send_block[80];
   int i;
   ushort lastcrc16;

   if(!writeSpad(portnum,addr,&challenge[0],8))
      return FALSE;


   // access the device
   if(owAccess(portnum))
   {
      setcrc16(portnum,0);

      // Read Authenticated Command
      send_block[send_cnt] = 0xA5;
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

      // address 1
      send_block[send_cnt] = (uchar)(addr & 0xFF);
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

      // address 2
      send_block[send_cnt] = (uchar)((addr >> 8) & 0xFF);
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

      // data + FF byte
      for (i = 0; i < 35; i++)
         send_block[send_cnt++] = 0xFF;

      // now send the block
      if (owBlock(portnum,FALSE,send_block,send_cnt))
      {
         // calculate CRC16 of result
         for (i = 3; i < send_cnt; i++)
            lastcrc16 = docrc16(portnum,send_block[i]);

         // verify CRC16 is correct
         if (lastcrc16 != 0xB001)
         {
            OWERROR(OWERROR_CRC_FAILED);
            return FALSE;
         }

         for(i=3;i<35;i++)
            data[i-3] = send_block[i];

         send_cnt = 0;
         for(i=0;i<22;i++)
            send_block[send_cnt++] = 0xFF;

         // Delay for MAC
         msDelay(2);

         if(owBlock(portnum,FALSE,send_block,send_cnt))
         {
            // calculate CRC16 of result
            setcrc16(portnum,0);
            for (i = 0; i < send_cnt ; i++)
               lastcrc16 = docrc16(portnum,send_block[i]);

            // verify CRC16 is correct
            if (lastcrc16 != 0xB001)
            {
               OWERROR(OWERROR_CRC_FAILED);
               return FALSE;
            }
         
            for(i=0;i<20;i++)
               extra[i] = send_block[i];

            return TRUE;
         }
         else
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }

      }
      else
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

   }
   else
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }


   return FALSE;
}

/**
 * Loads data into memory without a copy scratchpad
 *
 * portnum       number 0 to MAX_PORTNUM-1.  This number is provided to
 *                  indicate the symbolic port number.
 * addr          address of the data to be read along with Physical address
 * SNum          the serial number for the part.
 * secret        the load first secret data
 * data_len      the length of the secret.
 *
 * @return - returns 'TRUE' if load first secret worked 
 *                   and 'FALSE' if not.
 */
SMALLINT loadFirstSecret(int portnum, int addr, uchar *SNum, uchar *secret, int data_len)
{
   ushort tempadd;
   uchar  es,data[8],test;
   uchar  send_block[32];
   int send_cnt=0;
   uchar mem[32];
   int i;

   if(readSHAEE(3,portnum,SNum,0,FALSE,mem,16))
   {
      if((mem[8] == 0xAA) || (mem[8] == 0x55))
      {
         OWERROR(OWERROR_WRITE_PROTECT_SECRET);
         return FALSE;
      }

      if(((addr < 32)) &&
         (((mem[9] == 0xAA) || (mem[9] == 0x55)) ||
          ((mem[13] == 0xAA) || (mem[13] == 0x55))))
      {
         OWERROR(OWERROR_WRITE_PROTECT);
         return FALSE;
      }

      if(((addr < 128)) &&
         ((mem[9] == 0xAA) || (mem[9] == 0x55)))
      {
         OWERROR(OWERROR_WRITE_PROTECT);
         return FALSE;
      }
   }


   if(writeSpad(portnum,addr,secret,data_len) &&
      readSpad(portnum,&tempadd,&es,data))
   {

      // access the device
      if(owAccess(portnum))
      {
         // write scratchpad command
         send_block[send_cnt++] = 0x5A;
         // address 1
         send_block[send_cnt++] = (uchar)(tempadd & 0xFF);
         // address 2
         send_block[send_cnt++] = (uchar)((tempadd >> 8) & 0xFF);
         // ending address with data status
         send_block[send_cnt++] = es;

         if(owBlock(portnum,FALSE,send_block,send_cnt))
         {
            // 10ms delay for writing first secret
            msDelay(10);

            test = owReadByte(portnum);

            if((test == 0xAA) || (test == 0x55))
            {
               if(addr == 128)
               {
                  for(i=0;i<8;i++)
                     local_secret[i] = secret[i];
               }

               return TRUE;
            }
            else
            {
               OWERROR(OWERROR_LOAD_FIRST_SECRET);
               return FALSE;
            }
         }
         else
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }
      }  
      else
      {
         OWERROR(OWERROR_DEVICE_SELECT_FAIL);
         return FALSE;
      }

   }

   return FALSE;
}

/**
 * Calculate the next secret using the current one.
 *
 * portnum       number 0 to MAX_PORTNUM-1.  This number is provided to
 *                  indicate the symbolic port number.
 * addr          address of the data to be read along with Physical address
 * SNum          the serial number for the part.
 * secret        the load first secret data
 *
 * @return - returns 'TRUE' if the next secret was calculated correctly
 *                   and 'FALSE' if not.
 */
SMALLINT computeNextSecret(int portnum, int addr, uchar *SNum, uchar *secret)
{
   int i;
   unsigned MT[64];
   long A,B,C,D,E;
   long Temp;
   uchar memory[32],scratch[8],es;
   ushort tmpadd;
   uchar send_block[32];
   int send_cnt=0;
   SMALLINT bank = 0;

   for(i=0;i<8;i++)
      scratch[i] = 0x00;

   if(!writeSpad(portnum,addr,scratch,8))
      return FALSE;

   for(i=0;i<4;i++)
      MT[i] = local_secret[i];

   if(addr < 32)
      bank = 0;
   else if((addr > 31) && (addr < 64))
      bank = 1;
   else if((addr > 63) && (addr < 128))
      bank = 2;
 
   if(readSHAEE(bank,portnum,SNum,(addr/((ushort)32))*32,FALSE,memory,32))
      for(i=4;i<36;i++)
         MT[i] = memory[i-4];
   else
      return FALSE;

   for(i=36;i<40;i++)
      MT[i] = 0xFF;

   if(readSpad(portnum, &tmpadd, &es, scratch))
   {
      MT[40] = scratch[0] & 0x3F;

      for(i=41;i<48;i++)
         MT[i] = scratch[i-40];
      for(i=48;i<52;i++)
         MT[i] = local_secret[i-44];
   }
   else
      return FALSE;

   for(i=52;i<55;i++)
      MT[i] = 0xFF;

   MT[55] = 0x80;

   for(i=56;i<62;i++)
      MT[i] = 0x00;

   MT[62] = 0x01;
   MT[63] = 0xB8;

   ComputeSHA(MT,&A,&B,&C,&D,&E);

   Temp = E;
   for(i=0;i<4;i++)
   {
      secret[i] = (uchar) (Temp & 0x000000FF);
//\\//\\//\\//\\//  DEBUG:  printing out what the next secret is.
//      printf("%02X",secret[i]);
//\\//\\//\\//\\//  DEBUG:  printing out what the next secret is.
      Temp >>= 8;
   }
//\\//\\//\\//\\//  DEBUG:  printing out what the next secret is.
//   printf("\n");
//\\//\\//\\//\\//  DEBUG:  printing out what the next secret is.

   Temp = D;
   for(i=0;i<4;i++)
   {
      secret[i+4] = (uchar) (Temp & 0x000000FF);
//\\//\\//\\//\\//  DEBUG:  printing out what the next secret is.
//      printf("%02X",secret[i+4]);
//\\//\\//\\//\\//  DEBUG:  printing out what the next secret is.
      Temp >>= 8;
   }
//\\//\\//\\//\\//  DEBUG:  printing out what the next secret is.
//   printf("\n");
//\\//\\//\\//\\//  DEBUG:  printing out what the next secret is.

   // access the device
   if(owAccess(portnum))
   {
      // Next Secret command
      send_block[send_cnt++] = 0x33;

      // address 1
      send_block[send_cnt++] = (uchar)(addr & 0xFF);
      // address 2
      send_block[send_cnt++] = (uchar)((addr >> 8) & 0xFF);


      if(owBlock(portnum,FALSE,send_block,send_cnt))
      {
         msDelay(12);

         if(readSpad(portnum, &tmpadd, &es, scratch))
         {
            for(i=0;i<8;i++)
               if(scratch[i] != 0xAA)
               {
                  OWERROR(OWERROR_COMPUTE_NEXT_SECRET);
                  return FALSE;
               }

            for(i=0;i<8;i++)
               local_secret[i] = secret[i];

            return TRUE;
         }
      }
      else
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

   }
   else
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   return FALSE;

}

/**
 * This function set the bus master secret, which is used in 
 * calculating the MAC that is sent to the part for different
 * operations.
 *
 * new_secret  is the new bus master secret to be set.
 */
void setBusMasterSecret(uchar *new_secret)
{
   int i;

   for(i=0;i<8;i++)
      local_secret[i] = new_secret[i];
}

/**
 * This function returns what the current bus master secret is.
 *
 * return_secret  is used to copy what the current bus master
 *                secret is.
 */
void returnBusMasterSecret(uchar *return_secret)
{
   int i;

   for(i=0;i<8;i++)
      return_secret[i] = local_secret[i];
}

/**
 * This function set the challenge, which resides in the 
 * scratch pad for doing a read authenticate page command.
 *
 * new_challenge  this is what the new challenge is going 
 *                to be set to.
 */
void setChallenge(uchar *new_challenge)
{
   int i;

   for(i=0;i<8;i++)
      challenge[i] = new_challenge[i];
}

//----------------------------------------------------------------------
// Compute the 160-bit MAC
//
// 'MT'  - input data
// 'A'   - part of the 160 bits
// 'B'   - part of the 160 bits
// 'C'   - part of the 160 bits
// 'D'   - part of the 160 bits
// 'E'   - part of the 160 bits
//
//
void ComputeSHA(unsigned int *MT,long *A,long *B, long *C, long *D,long *E)
{
   unsigned long MTword[80];
   int i;
   long ShftTmp;
   long Temp;

   for(i=0;i<16;i++)
      MTword[i] = (MT[i*4] << 24) | (MT[i*4+1] << 16) |
                  (MT[i*4+2] << 8) | MT[i*4+3];

   for(i=16;i<80;i++)
   {
      ShftTmp = MTword[i-3] ^ MTword[i-8] ^ MTword[i-14] ^ MTword[i-16];
      MTword[i] = ((ShftTmp << 1) & 0xfffffffe) |
                  ((ShftTmp >> 31) & 0x00000001);
   }

   *A=0x67452301;
   *B=0xefcdab89;
   *C=0x98badcfe;
   *D=0x10325476;
   *E=0xc3d2e1f0;

   for(i=0;i<80;i++)
   {
      ShftTmp = ((*A << 5) & 0xffffffe0) | ((*A >> 27) & 0x0000001f);
      Temp = NLF(*B,*C,*D,i) + *E + KTN(i) + MTword[i] + ShftTmp;
      *E = *D;
      *D = *C;
      *C = ((*B << 30) & 0xc0000000) | ((*B >> 2) & 0x3fffffff);
      *B = *A;
      *A = Temp;
   }
}

// calculation used for the MAC
long KTN (int n)
{
   if(n<20)
      return 0x5a827999;
   else if (n<40)
      return 0x6ed9eba1;
   else if (n<60)
      return 0x8f1bbcdc;
   else
      return 0xca62c1d6;
}

// calculation used for the MAC
long NLF (long B, long C, long D, int n)
{
   if(n<20)
      return ((B&C)|((~B)&D));
   else if(n<40)
      return (B^C^D);
   else if(n<60)
      return ((B&C)|(B&D)|(C&D));
   else
      return (B^C^D);
}