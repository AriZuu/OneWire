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
//  mbEE.c - Reads and writes to memory locations for the EE memory bank.
//  version 1.00
//

// Include Files
#include "ownet.h"
#include "mbee.h"

// General command defines
#define READ_MEMORY_COMMAND      0xF0
#define WRITE_SCRATCHPAD_COMMAND 0x0F
#define READ_SCRATCHPAD_COMMAND  0xAA
#define COPY_SCRATCHPAD_COMMAND  0x55
// Local defines
#define SIZE        32
#define PAGE_LENGTH 32

// Global variables
char     *bankDescription     = "Main Memory";
SMALLINT  writeVerification    = TRUE;
SMALLINT  generalPurposeMemory = TRUE;
SMALLINT  readWrite            = TRUE;
SMALLINT  writeOnce            = FALSE;
SMALLINT  readOnly             = FALSE;
SMALLINT  nonVolatile          = TRUE;
SMALLINT  needsProgramPulse    = FALSE;
SMALLINT  needsPowerDelivery   = TRUE;
SMALLINT  hasExtraInfo         = FALSE;
SMALLINT  extraInfoLength      = 0;
char     *extraInfoDesc       = "";
SMALLINT  pageAutoCRC          = FALSE;


/**
 * Read  memory in the current bank with no CRC checking (device or
 * data). The resulting data from this API may or may not be what is on
 * the 1-Wire device.  It is recommends that the data contain some kind
 * of checking (CRC) like in the readPagePacketEE() method or have
 * the 1-Wire device provide the CRC as in readPageCRCEE().  readPageCRCEE()
 * however is not supported on all memory types, see 'hasPageAutoCRCEE()'.
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
SMALLINT readEE(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                SMALLINT rd_cont, uchar *buff, int len)
{
   int    i;
   uchar  raw_buf[2];

   // check if read exceeds memory
   if((str_add + len)>SIZE)
   {
      OWERROR(OWERROR_READ_OUT_OF_RANGE);
      return FALSE;
   }

   owSerialNum(portnum,SNum,FALSE);

   // select the device
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // build start reading memory block
   raw_buf[0] = READ_MEMORY_COMMAND;
   raw_buf[1] = (uchar)str_add & 0xFF;

   // do the first block for command, address
   if(!owBlock(portnum,FALSE,raw_buf,2))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   // pre-fill readBuf with 0xFF
   for(i=0;i<len;i++)
      buff[i] = 0xFF;

   // send the block
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
 * to provide error free reading back with readEE().  Or the
 * method 'writePagePacketEE()' could be used which automatically
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
SMALLINT writeEE(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                 uchar *buff, int len)
{
   int i;
   uchar raw_buf[64];

   // return if nothing to do
   if (len == 0)
      return TRUE;

   // check if power delivery is available
   if(!owHasPowerDelivery(portnum))
   {
      OWERROR(OWERROR_POWER_NOT_AVAILABLE);
      return FALSE;
   }

   // check if write exceeds memory
   if ((str_add + len) > SIZE)
   {
      OWERROR(OWERROR_WRITE_OUT_OF_RANGE);
      return FALSE;
   }

   owSerialNum(portnum,SNum,FALSE);

   // write the page of data to scratchpad
   if(!writeScratchpad(portnum,str_add,buff,len))
      return FALSE;

   // read the data back for verification
   if(!readScratchpad(portnum,raw_buf,str_add,PAGE_LENGTH))
      return FALSE;

   // check to see if the same
   for (i = 0; i < len; i++)
      if (raw_buf[i] != buff[i])
      {
         OWERROR(OWERROR_READ_SCRATCHPAD_VERIFY);
         return FALSE;
      }

   // do the copy
   if(!copyScratchpad(portnum))
      return FALSE;

   // check on write verification
   if (writeVerification)
   {

      // read back to verify
      if(!readEE(bank,portnum,SNum,str_add,FALSE,buff,len))
         return FALSE;

      for (i=0;i<len;i++)
         if (raw_buf[i] != buff[i])
         {
            OWERROR(OWERROR_READ_VERIFY_FAILED);
            return FALSE;
         }
   }

   return TRUE;
}

/**
 * Read  page in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketEE() method or have the 1-Wire device provide the
 * CRC as in readPageCRCEE().  readPageCRCEE() however is not
 * supported on all memory types, see 'hasPageAutoCRCEE()'.
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
SMALLINT readPageEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                    SMALLINT rd_cont, uchar *buff)
{
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   return readEE(bank,portnum,SNum,page,rd_cont,buff,PAGE_LENGTH);
}

/**
 * Read  page with extra information in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketEE() method or have the 1-Wire device provide the
 * CRC as in readPageCRCEE().  readPageCRCEE() however is not
 * supported on all memory types, see 'hasPageAutoCRCEE()'.
 * If neither is an option then this method could be called more
 * then once to at least verify that the same thing is read consistantly.
 * See the method 'hasExtraInfoEE()' for a description of the optional
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
SMALLINT readPageExtraEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                         SMALLINT rd_cont, uchar *buff, uchar *extra)
{
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
SMALLINT readPageExtraCRCEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                            uchar *read_buff, uchar *extra)
{
   OWERROR(OWERROR_CRC_EXTRA_INFO_NOT_SUPPORTED);
   return FALSE;
}
/**
 * Read a complete memory page with CRC verification provided by the
 * device.  Not supported by all devices.  See the method
 * 'hasPageAutoCRCEE()'.
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
SMALLINT readPageCRCEE(SMALLINT bank, int portnum, uchar *SNum, int page, uchar *buff)
{
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
SMALLINT readPagePacketEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                          SMALLINT rd_cont, uchar *buff, int *len)
{
   uchar raw_buf[PAGE_LENGTH];
   ushort lastcrc16;
   int i;

   // read the scratchpad, discard extra information
   if(!readEE(bank,portnum,SNum,page,rd_cont,raw_buf,PAGE_LENGTH))
      return FALSE;

   // check if length is realistic
   if((raw_buf[0] > (PAGE_LENGTH - 3)) || (raw_buf[0] <= 0))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort) ((getStartingAddressEE(bank,SNum)/PAGE_LENGTH) + page));
   for(i=0;i<raw_buf[0]+3;i++)
      lastcrc16 = docrc16(portnum,raw_buf[i]);

   if(lastcrc16 == 0xB001)
   {

      // extract the data out of the packet
      for(i=1;i<raw_buf[0]+1;i++)
         buff[i-1] = raw_buf[i];

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
 * Read a Universal Data Packet and extra information.  See the
 * method 'readPagePacketEE()' for a description of the packet structure.
 * See the method 'hasExtraInfoEE()' for a description of the optional
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
SMALLINT readPagePacketExtraEE(SMALLINT bank, int portnum, uchar *SNum,
                               int page, SMALLINT rd_cont, uchar *buff,
                               int *len, uchar *extra)
{
   OWERROR(OWERROR_PG_PACKET_WITHOUT_EXTRA);
   return FALSE;
}

/**
 * Write a Universal Data Packet.  See the method 'readPagePacketEE()'
 * for a description of the packet structure.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page the packet is being written to.
 * rd_cont  if 'true' then device read is continued without
 *          re-selecting.  This can only be used if the new
 *          read() continious where the last one led off
 *          and it is inside a 'beginExclusive/endExclusive'
 *          block.
 * buff     byte array containing data that to write.
 * len      length of the packet
 *
 * @return - returns '0' if the write page packet wasn't completed
 *                   '1' if the operation is complete.
 */
SMALLINT writePagePacketEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                           uchar *buff, int len)
{
   uchar raw_buf[64];
   int i;
   ushort crc;

   // make sure length does not exceed max
   if ((len > (PAGE_LENGTH - 3)) || (len <= 0))
   {
      OWERROR(OWERROR_PACKET_LENGTH_EXCEEDS_PAGE);
      return FALSE;
   }

   // construct the packet to write
   raw_buf[0] = (uchar) len;

   for(i=1;i<len+1;i++)
      raw_buf[i] = buff[i-1];

   setcrc16(portnum,(ushort) ((getStartingAddressEE(bank,SNum)/PAGE_LENGTH) + page));
   for(i=0;i<len+1;i++)
      crc = docrc16(portnum,raw_buf[i]);

   raw_buf[len + 1] = (uchar) ~crc & 0xFF;
   raw_buf[len + 2] = (uchar) ((~crc & 0xFFFF) >> 8) & 0xFF;

   // write the packet, return result
   if(!writeEE(bank,portnum,SNum,page * PAGE_LENGTH,raw_buf,len + 3))
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
SMALLINT getNumberPagesEE(SMALLINT bank, uchar *SNum)
{
   return 1;
}

/**
 * Query to get the memory bank size in bytes.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  memory bank size in bytes.
 */
int getSizeEE(SMALLINT bank, uchar *SNum)
{
   return SIZE;
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
int getStartingAddressEE(SMALLINT bank, uchar *SNum)
{
   return 0;
}

/**
 * Query to get page length in bytes in current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return   page length in bytes in current memory bank
 */
SMALLINT getPageLengthEE(SMALLINT bank, uchar *SNum)
{
   return PAGE_LENGTH;
}

/**
 * Query to see get a string description of the current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  String containing the memory bank description
 */
char *getBankDescriptionEE(SMALLINT bank, uchar *SNum)
{
   return bankDescription;
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
SMALLINT isGeneralPurposeMemoryEE(SMALLINT bank, uchar *SNum)
{
   return generalPurposeMemory;
}

/**
 * Query to see if current memory bank is read/write.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank is read/write
 */
SMALLINT isReadWriteEE(SMALLINT bank, int portnum, uchar *SNum)
{
   return readWrite;
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
SMALLINT isWriteOnceEE(SMALLINT bank, int portnum, uchar *SNum)
{
   return writeOnce;
}

/**
 * Query to see if current memory bank is read only.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can only be read
 */
SMALLINT isReadOnlyEE(SMALLINT bank, int portnum, uchar *SNum)
{
   return readOnly;
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
SMALLINT isNonVolatileEE(SMALLINT bank, uchar *SNum)
{
   return nonVolatile;
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
SMALLINT needsProgramPulseEE(SMALLINT bank, uchar *SNum)
{
   return needsProgramPulse;
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
SMALLINT needsPowerDeliveryEE(SMALLINT bank, uchar *SNum)
{
   return needsPowerDelivery;
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
SMALLINT hasExtraInfoEE(SMALLINT bank, uchar *SNum)
{
   return hasExtraInfo;
}

/**
 * Query to get the length in bytes of extra information that
 * is read when read a page in the current memory bank.  See
 * 'hasExtraInfoEE()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  number of bytes in Extra Information read when reading
 *          pages in the current memory bank.
 */
SMALLINT getExtraInfoLengthEE(SMALLINT bank, uchar *SNum)
{
   return extraInfoLength;
}

/**
 * Query to get a string description of what is contained in
 * the Extra Informationed return when reading pages in the current
 * memory bank.  See 'hasExtraInfoEE()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return string describing extra information.
 */
char *getExtraInfoDescEE(SMALLINT bank, uchar *SNum)
{
   return extraInfoDesc;
}

/**
 * Query to see if current memory bank pages can be read with
 * the contents being verified by a device generated CRC.
 * This is used to see if the 'ReadPageCRCEE()' can be used.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can be read with self
 *          generated CRC.
 */
SMALLINT hasPageAutoCRCEE(SMALLINT bank, uchar *SNum)
{
   return pageAutoCRC;
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
SMALLINT getMaxPacketDataLengthEE(SMALLINT bank, uchar *SNum)
{
   return PAGE_LENGTH - 3;
}

//--------
//-------- Bank specific methods
//--------


/**
 * Read the scratchpad page of memory from the device
 * This method reads and returns the entire scratchpad after the byte
 * offset regardless of the actual ending offset
 *
 * portnum       the port number used to reference the port
 * readBuf       byte array to place read data into
 *                       length of array is always pageLength.
 * startAddr     address to start to read from scratchPad
 * len           length in bytes to read
 *
 * @return       'true' if read scratch pad was successful
 */
SMALLINT readScratchpad (int portnum, uchar *readBuf, int startAddr, int len)
{
   int i;
   uchar raw_buf[2];

   // select the device
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // build first block
   raw_buf[0] = READ_SCRATCHPAD_COMMAND;
   raw_buf[1] = startAddr;

   // do the first block for address
   if(!owBlock(portnum,FALSE,raw_buf,2))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   // build the next block
   for(i=0;i<len;i++)
      readBuf[i] = 0xFF;

   // send second block to read data, return result
   if(!owBlock(portnum,FALSE,readBuf,len))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   return TRUE;
}

/**
 * Write to the scratchpad page of memory device.
 *
 * portnum       the port number used to reference the port
 * startAddr     starting address
 * writeBuf      byte array containing data to write
 * len           length in bytes to write
 *
 * @return       'true' if the write to scratch pad was successful
 */
SMALLINT writeScratchpad (int portnum,int startAddr, uchar *writeBuf, int len)
{
   int i;
   uchar raw_buf[64];

   // select the device
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // build block to send
   raw_buf[0] = WRITE_SCRATCHPAD_COMMAND;
   raw_buf[1] = startAddr & 0xFF;

   for(i=2;i<len+2;i++)
      raw_buf[i] = writeBuf[i-2];

   // send second block to read data, return result
   if(!owBlock(portnum,FALSE,raw_buf,len+2))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   return TRUE;
}

/**
 * Copy the scratchpad page to memory.
 *
 * portnum       the port number used to reference the port
 *
 * @return       'true' if the copy scratchpad was successful
 */
SMALLINT copyScratchpad(int portnum)
{

   // select the device
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // copy scratch
   if(!owWriteByte(portnum,COPY_SCRATCHPAD_COMMAND))
   {
      OWERROR(OWERROR_WRITE_BYTE_FAILED);
      return FALSE;
   }

   // send command and setup strong pullup
   if(!owWriteBytePower(portnum,0xA5))
   {
      OWERROR(OWERROR_WRITE_BYTE_FAILED);
      return FALSE;
   }

   // delay for 10ms
   msDelay(10);

   // disable power
   if(MODE_NORMAL != owLevel(portnum,MODE_NORMAL))
   {
      OWERROR(OWERROR_LEVEL_FAILED);
      return FALSE;
   }

   return TRUE;
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
SMALLINT canRedirectPageEE(SMALLINT bank, uchar *SNum)
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
SMALLINT canLockPageEE(SMALLINT bank, uchar *SNum)
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
SMALLINT canLockRedirectPageEE(SMALLINT bank, uchar *SNum)
{
   return FALSE;
}

