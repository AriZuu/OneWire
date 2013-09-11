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
//  mbAppReg.c - Reads and writes to memory locations for the AppReg memory bank.
//  version 1.00
//

// Include Files
#include "ownet.h"
#include "mbappreg.h"

// General command defines
#define READ_MEMORY_COMMAND  0xC3
#define WRITE_MEMORY_COMMAND 0x99
#define COPY_LOCK_COMMAND    0x5A
#define READ_STATUS_COMMAND  0x66
#define VALIDATION_KEY       0xA5
#define LOCKED_FLAG          0xFC
// Local defines
#define SIZE                 8
#define PAGE_LENGTH          8

// Global variables
char     *bankDescriptionAR     = "Application register, non-volatile when locked.";
SMALLINT  writeVerificationAR    = TRUE;
SMALLINT  generalPurposeMemoryAR = TRUE;
SMALLINT  readWriteAR            = TRUE;
SMALLINT  writeOnceAR            = FALSE;
SMALLINT  readOnlyAR             = FALSE;
SMALLINT  nonVolatileAR          = FALSE;
SMALLINT  needsProgramPulseAR    = FALSE;
SMALLINT  needsPowerDeliveryAR   = TRUE;
SMALLINT  hasExtraInfoAR         = TRUE;
SMALLINT  extraInfoLengthAR      = 1;
char     *extraInfoDescAR       = "Page Locked Flag";
SMALLINT  pageAutoCRCAR          = FALSE;
SMALLINT  canRedirectPageAR      = FALSE;
SMALLINT  canLockPageAR          = TRUE;
SMALLINT  canLockRedirecPageAR   = FALSE;


/**
 * Read  memory in the current bank with no CRC checking (device or
 * data). The resulting data from this API may or may not be what is on
 * the 1-Wire device.  It is recommends that the data contain some kind
 * of checking (CRC) like in the readPagePackeAppRegt() method or have
 * the 1-Wire device provide the CRC as in readPageCRCAppReg().  readPageCRCAppReg()
 * however is not supported on all memory types, see 'hasPageAutoCRCAppReg()'.
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
SMALLINT readAppReg(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                    SMALLINT rd_cont, uchar *buff, int len)
{
   uchar raw_buf[2];
   int i;

   // check if read exceeds memory
   if((str_add + len) > PAGE_LENGTH)
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

   // start the read
   raw_buf[0] = READ_MEMORY_COMMAND;
   raw_buf[1] = str_add & 0xFF;

   if(!owBlock(portnum,FALSE,raw_buf,2))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

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
 * to provide error free reading back with readAppReg().  Or the
 * method 'writePagePacketAppReg()' could be used which automatically
 * wraps the data in a length and CRC.
 *
 * When using on Write-Once devices care must be taken to write into
 * into empty space.  If writeAppReg() is used to write over an unlocked
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
SMALLINT writeAppReg(SMALLINT bank, int portnum, uchar *SNum,
                     int str_add, uchar *buff, int len)
{
   uchar raw_buf[64];
   int i;

   // return if nothing to do
   if (len == 0)
      return TRUE;

   // check if power delivery is available
/*   if(!owDeliverPower())
   {
      OWERROR(OWERROR_NO_POWER_DELIVERY);
      return FALSE;
   }*/

   // check if write exceeds memory
   if ((str_add + len) > PAGE_LENGTH)
   {
      OWERROR(OWERROR_WRITE_OUT_OF_RANGE);
      return FALSE;
   }

   owSerialNum(portnum,SNum,FALSE);

   // select the device
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // start the write
   raw_buf[0] = WRITE_MEMORY_COMMAND;
   raw_buf[1] = str_add & 0xFF;

   for(i=0;i<len;i++)
      raw_buf[i+2] = buff[i];

   if(!owBlock(portnum,FALSE,raw_buf,len+2))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   // check for write verification
   if(writeVerificationAR)
   {
      // read back
      if(!readAppReg(bank,portnum,SNum,str_add,TRUE,raw_buf,len))
      {
         return FALSE;
      }

      // compare
      for(i=0;i<len;i++)
      {
         if(raw_buf[i] != buff[i])
         {
            OWERROR(OWERROR_READ_BACK_INCORRECT);
            return FALSE;
         }
      }
   }

   return TRUE;
}

/**
 * Read  page in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketAppReg() method or have the 1-Wire device provide the
 * CRC as in readPageCRCAppReg().  readPageCRCAppReg() however is not
 * supported on all memory types, see 'hasPageAutoCRCAppReg()'.
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
SMALLINT readPageAppReg(SMALLINT bank, int portnum, uchar *SNum, int page,
                        SMALLINT rd_cont, uchar *buff)
{
   // check if for valid page
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   // do the read
   if(!readAppReg(bank,portnum,SNum,page*PAGE_LENGTH,
       TRUE,buff,PAGE_LENGTH))
      return FALSE;

   return TRUE;
}

/**
 * Read  page with extra information in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketAppReg() method or have the 1-Wire device provide the
 * CRC as in readPageCRCAppReg().  readPageCRCAppReg() however is not
 * supported on all memory types, see 'hasPageAutoCRCAppReg()'.
 * If neither is an option then this method could be called more
 * then once to at least verify that the same thing is read consistantly.
 * See the method 'hasExtraInfoAppReg()' for a description of the optional
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
SMALLINT readPageExtraAppReg(SMALLINT bank, int portnum, uchar *SNum, int page,
                             SMALLINT rd_cont, uchar *buff, uchar *extra)
{
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   // read the page data
   if(!readAppReg(bank,portnum,SNum,page*PAGE_LENGTH,
       TRUE,buff,PAGE_LENGTH))
      return FALSE;

   // read the extra information (status)
   if(!readStatus(portnum,extra))
   {
      OWERROR(OWERROR_READ_STATUS_NOT_COMPLETE);
      return FALSE;
   }

   return TRUE;
}

/**
 * Read  page with extra information in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacket() method or have the 1-Wire device provide the
 * CRC as in readPageCRC().  readPageCRC() however is not
 * supported on all memory types, see 'hasPageAutoCRC()'.
 * If neither is an option then this method could be called more
 * then once to at least verify that the same thing is read consistantly.
 * See the method 'haveExtraInfo()' for a description of the optional
 * extra information some devices have.
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
SMALLINT readPageExtraCRCAppReg(SMALLINT bank, int portnum, uchar *SNum, int page,
                                uchar *read_buff, uchar *extra)
{
   // read the page data
   if(!readAppReg(bank,portnum,SNum,page*PAGE_LENGTH,
                  TRUE,read_buff,PAGE_LENGTH))
      return FALSE;

   // read the extra information (status)
   if(!readStatus(portnum,extra))
   {
      OWERROR(OWERROR_READ_STATUS_NOT_COMPLETE);
      return FALSE;
   }

   return TRUE;
}

/**
 * Read a complete memory page with CRC verification provided by the
 * device.  Not supported by all devices.  See the method
 * 'hasPageAutoCRCAppReg()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to be read
 * buff     byte array containing data that was read
 *
 * @return - returns '0' if the read page wasn't completed.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageCRCAppReg(SMALLINT bank, int portnum, uchar *SNum,
                           int str_add, uchar *buff)
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
SMALLINT readPagePacketAppReg(SMALLINT bank, int portnum, uchar *SNum, int page,
                              SMALLINT rd_cont, uchar *buff, int *len)
{
   uchar raw_buf[PAGE_LENGTH];
   ushort lastcrc16;
   int i;

   // read the entire page data
   if(!readAppReg(bank,portnum,SNum,page,rd_cont,raw_buf,PAGE_LENGTH))
   {
      return FALSE;
   }

   // check if length is realistic
   if((raw_buf[0] > (PAGE_LENGTH - 2)) || (raw_buf[0] <= 0))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort)((getStartingAddressAppReg(bank,SNum)/PAGE_LENGTH) + page));
   for(i=0;i<(raw_buf[0]+3);i++)
      lastcrc16 = docrc16(portnum,raw_buf[i]);

   if(lastcrc16 == 0xB001)
   {
      *len = (int) raw_buf[0];

      // extract the data out of the packet
      for(i=1;i<raw_buf[0]+1;i++)
         buff[i-1] = raw_buf[i];
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
 * method 'readPagePacketAppReg()' for a description of the packet structure.
 * See the method 'hasExtraInfoAppReg()' for a description of the optional
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
SMALLINT readPagePacketExtraAppReg(SMALLINT bank, int portnum, uchar *SNum,
                                   int page, SMALLINT rd_cont, uchar *buff,
                                   int *len, uchar *extra)
{
   uchar raw_buf[32];
   int i;
   ushort lastcrc16;

   // check if current bank is not scratchpad bank, or not page 0
   if(!hasExtraInfoAppReg(bank,SNum))
   {
      OWERROR(OWERROR_EXTRA_INFO_NOT_SUPPORTED);
      return FALSE;
   }

   // read the entire page data
   if(!readAppReg(bank,portnum,SNum,page*PAGE_LENGTH,TRUE,raw_buf,PAGE_LENGTH))
   {
      return FALSE;
   }

   // check if length is realistic
   if((raw_buf[0] > (PAGE_LENGTH - 2)) || (raw_buf[0] <= 0))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort) ((getStartingAddressAppReg(bank,SNum)/PAGE_LENGTH) + page));
   for(i=0;i<(raw_buf[0]+3);i++)
      lastcrc16 = docrc16(portnum,raw_buf[i]);

   if(lastcrc16 == 0xB001)
   {
      *len = (int) raw_buf[0];

      if(!readStatus(portnum,extra))
      {
         OWERROR(OWERROR_READ_STATUS_NOT_COMPLETE);
         return FALSE;
      }

      // extract the data out of the packet
      for(i=1;i<raw_buf[0]+1;i++)
         buff[i-1] = raw_buf[i];
   }
   else
   {
      OWERROR(OWERROR_CRC_FAILED);
      return FALSE;
   }

   return TRUE;
}

/**
 * Write a Universal Data Packet.  See the method 'readPagePacketAppReg()'
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
SMALLINT writePagePacketAppReg(SMALLINT bank, int portnum, uchar *SNum,
                               int page, uchar *buff, int len)
{
   uchar raw_buf[PAGE_LENGTH];
   ushort crc;
   int i;

   // make sure length does not exceed max
   if((len > (PAGE_LENGTH - 2)) || (len <= 0))
   {
      OWERROR(OWERROR_PACKET_LENGTH_EXCEEDS_PAGE);
      return FALSE;
   }

   raw_buf[0] = (uchar) len;

   for(i=0;i<len;i++)
      raw_buf[i+1] = buff[i];

   setcrc16(portnum,(ushort) ((getStartingAddressAppReg(bank,SNum)/PAGE_LENGTH) + page));
   for(i=0;i<len+1;i++)
      crc = docrc16(portnum,raw_buf[i]);

   raw_buf[len + 1] = (uchar) ~crc & 0xFF;
   raw_buf[len + 2] = (uchar) ((~crc & 0xFFFF) >> 8) & 0xFF;

   // write the packet, return result
   if(!writeAppReg(bank,portnum,SNum,page*PAGE_LENGTH,raw_buf,len + 3))
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
SMALLINT getNumberPagesAppReg(SMALLINT bank, uchar *SNum)
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
int getSizeAppReg(SMALLINT bank, uchar *SNum)
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
int getStartingAddressAppReg(SMALLINT bank, uchar *SNum)
{
   return 0;
}

/**
 * Query to get  page length in bytes in current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return   page length in bytes in current memory bank
 */
SMALLINT getPageLengthAppReg(SMALLINT bank, uchar *SNum)
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
char *getBankDescriptionAppReg(SMALLINT bank, uchar *SNum)
{
   return bankDescriptionAR;
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
SMALLINT isGeneralPurposeMemoryAppReg(SMALLINT bank, uchar *SNum)
{
   return generalPurposeMemoryAR;
}

/**
 * Query to see if current memory bank is read/write.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank is read/write
 */
SMALLINT isReadWriteAppReg(SMALLINT bank, int portnum, uchar *SNum)
{
   return readWriteAR;
}

/**
 * Query to see if current memory bank is read only.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can only be read
 */
SMALLINT isReadOnlyAppReg(SMALLINT bank, int portnum, uchar *SNum)
{
   return readOnlyAR;
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
SMALLINT isWriteOnceAppReg(SMALLINT bank, int portnum, uchar *SNum)
{
   return writeOnceAR;
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
SMALLINT isNonVolatileAppReg(SMALLINT bank, uchar *SNum)
{
   return nonVolatileAR;
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
SMALLINT needsProgramPulseAppReg(SMALLINT bank, uchar *SNum)
{
   return needsProgramPulseAR;
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
SMALLINT needsPowerDeliveryAppReg(SMALLINT bank, uchar *SNum)
{
   return needsPowerDeliveryAR;
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
SMALLINT hasExtraInfoAppReg(SMALLINT bank, uchar *SNum)
{
   return hasExtraInfoAR;
}

/**
 * Query to get the length in bytes of extra information that
 * is read when read a page in the current memory bank.  See
 * 'hasExtraInfoAppReg()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  number of bytes in Extra Information read when reading
 *          pages in the current memory bank.
 */
SMALLINT getExtraInfoLengthAppReg(SMALLINT bank, uchar *SNum)
{
   return extraInfoLengthAR;
}

/**
 * Query to get a string description of what is contained in
 * the Extra Informationed return when reading pages in the current
 * memory bank.  See 'hasExtraInfoAppReg()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return string describing extra information.
 */
char *getExtraInfoDescAppReg(SMALLINT bank, uchar *SNum)
{
   return extraInfoDescAR;
}

/**
 * Query to see if current memory bank pages can be read with
 * the contents being verified by a device generated CRC.
 * This is used to see if the 'ReadPageCRCAppReg()' can be used.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can be read with self
 *          generated CRC.
 */
SMALLINT hasPageAutoCRCAppReg(SMALLINT bank, uchar *SNum)
{
   return pageAutoCRCAR;
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
SMALLINT getMaxPacketDataLengthAppReg(SMALLINT bank, uchar *SNum)
{
   return PAGE_LENGTH - 3;
}

//--------
//-------- OTPMemoryBank query methods
//--------

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
SMALLINT canRedirectPageAppReg(SMALLINT bank, uchar *SNum)
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
SMALLINT canLockPageAppReg(SMALLINT bank, uchar *SNum)
{
   return TRUE;
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
SMALLINT canLockRedirectPageAppReg(SMALLINT bank, uchar *SNum)
{
   return FALSE;
}

//--------
//-------- OTPMemoryBank I/O methods
//--------

/**
 * Lock the specifed page in the current memory bank.  Not supported
 * by all devices.  See the method 'canLockPage()'.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to be used.
 *
 * @return 'true' if page is locked.
 */
SMALLINT lockPageAppReg(int portnum, int page, uchar *SNum)
{

   // select the device
   owSerialNum(portnum,SNum,FALSE);
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // do the copy/lock sequence
   if(!(owWriteByte(portnum,COPY_LOCK_COMMAND) &&
        owWriteByte(portnum,VALIDATION_KEY)))
   {
      OWERROR(OWERROR_WRITE_BYTE_FAILED);
      return FALSE;
   }

   // read back to verify
   if(!isPageLockedAppReg(portnum,page,SNum))
   {
      OWERROR(OWERROR_READ_BACK_NOT_VALID);
      return FALSE;
   }

   return TRUE;
}

/**
 * Query to see if the specified page is locked.
 * See the method 'canLockPage()'.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to be used.
 *
 * @return 'true' if page is locked.
 */
SMALLINT isPageLockedAppReg(int portnum, int page, uchar *SNum)
{
   uchar extra[2];

   // check if for valid page
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   // read status and return result
   if(!readStatus(portnum,&extra[0]))
   {
      OWERROR(OWERROR_READ_STATUS_NOT_COMPLETE);
      return FALSE;
   }

   if(extra[0] == LOCKED_FLAG)
      return TRUE;

   return FALSE;
}

/**
 * Redirect the specifed page in the current memory bank to a new page.
 * Not supported by all devices.  See the method 'canRedirectPage()'.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * page     the page to be redirect
 * newPage  the new page to be redirected to
 * SNum     the serial number for the part.
 *
 * @return 'true' if page is redirected.
 */
SMALLINT redirectPageAppReg(int portnum, int page, int newPage, uchar *SNum)
{
   OWERROR(OWERROR_PAGE_REDIRECTION_NOT_SUPPORTED);
   return FALSE;
}

/**
 * Gets the page redirection of the specified page.
 * Not supported by all devices.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to be used.
 *
 * @return where the redirected page is.
 */
SMALLINT getRedirectedPageAppReg(int portnum, int page, uchar *SNum)
{
   return 0;
}

/**
 * Lock the redirection option for the specifed page in the current
 * memory bank. Not supported by all devices.  See the method
 * 'canLockRedirectPage()'.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to be used.
 *
 * @return  'true' if redirected page is locked.
 */
SMALLINT lockRedirectPageAppReg(int portnum, int page, uchar *SNum)
{
   // only needs to be implemented if supported by hardware
   OWERROR(OWERROR_LOCK_REDIRECTION_NOT_SUPPORTED);
   return FALSE;
}

/**
 * Query to see if the specified page has redirection locked.
 * Not supported by all devices.  See the method 'canRedirectPage()'.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to be used.
 *
 * @return  return 'true' if redirection is locked for this page
 */
SMALLINT isRedirectPageLockedAppReg(int portnum, int page, uchar *SNum)
{
   return FALSE;
}

//--------
//-------- Bank specific methods
//--------

/**
 * Read the status register for this memory bank.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * extra    the extra information
 *
 * @return  'true' if the status was read.
 *
 */
SMALLINT readStatus(int portnum, uchar *extra)
{
   // select the device
   if(!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // do the read status sequence
   if(!owWriteByte(portnum,READ_STATUS_COMMAND))
      return FALSE;

   // validation key
   if(!owWriteByte(portnum,0x00))
      return FALSE;

   extra[0] = owReadByte(portnum);

   return TRUE;
}