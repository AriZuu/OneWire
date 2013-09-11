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
//  mbSCR.c - Reads and writes to memory locations for the Scratch memory bank.
//  version 1.00
//

// Include Files
#include "ownet.h"
#include "mbscr.h"
#include "mbscree.h"
#include "mbscrex.h"
#include "mbscrcrc.h"
#include "mbsha.h"

// External functions
extern SMALLINT owBlock(int,int,uchar *,int);
extern SMALLINT owReadByte(int);
extern SMALLINT owWriteByte(int,int);
extern void     output_status(int, char *);
extern void     msDelay(int);
extern void     owSerialNum(int,uchar *,int);
extern SMALLINT owAccess(int);
extern SMALLINT owWriteByte(int,int);
extern void     setcrc16(int,ushort);
extern ushort   docrc16(int,ushort);
extern SMALLINT owWriteBytePower(int,int);
extern SMALLINT owLevel(int,int);

// General command defines
#define READ_MEMORY_COMMAND      0xF0
#define WRITE_SCRATCHPAD_COMMAND 0x0F
#define READ_SCRATCHPAD_COMMAND  0xAA
#define COPY_SCRATCHPAD_COMMAND  0x55

// Local defines
#define SIZE_SCR        32
#define PAGE_LENGTH_SCR 32

// Global variables
char    *bankDescriptionSCR      = "Scratchpad";
SMALLINT writeVerificationSCR    = TRUE;
SMALLINT generalPurposeMemorySCR = FALSE;
SMALLINT readWriteSCR            = TRUE;
SMALLINT writeOnceSCR            = FALSE;
SMALLINT readOnlySCR             = FALSE;
SMALLINT nonVolatileSCR          = FALSE;
SMALLINT needProgramPulseSCR     = FALSE;
SMALLINT needPowerDeliverySCR    = FALSE;
SMALLINT ExtraInfoSCR            = TRUE;
SMALLINT extraInfoLengthSCR      = 3;
char    *extraInfoDescSCR        = "Target address, offset";
SMALLINT pageAutoCRCSCR          = FALSE;


/**
 * Read  memory in the current bank with no CRC checking (device or
 * data). The resulting data from this API may or may not be what is on
 * the 1-Wire device.  It is recommends that the data contain some kind
 * of checking (CRC) like in the readPagePacketScratch() method or have
 * the 1-Wire device provide the CRC as in readPageCRCScratch().  readPageCRCScratch()
 * however is not supported on all memory types, see 'hasPageAutoCRCScratch()'.
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
SMALLINT readScratch(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                     SMALLINT rd_cont, uchar *buff, int len)
{
   uchar extra[3];
   uchar read_buf[PAGE_LENGTH_SCR];
   int i;

   // check if read exceeds memory
   if ((str_add + len) > getSizeScratch(bank,SNum))
   {
      OWERROR(OWERROR_READ_OUT_OF_RANGE);
      return FALSE;
   }

   owSerialNum(portnum,SNum,FALSE);

   // read the scratchpad, discard extra information
   switch(SNum[0])
   {
      case 0x18: case 0x21:
         if(!readScratchPadCRC(portnum,read_buf,len+str_add,extra))
            return FALSE;
         break;

      default:
         if(!readScratchpd(portnum,read_buf,len+str_add))
            return FALSE;
         break;
   }

   for(i=str_add;i<(len+str_add);i++)
      buff[i-str_add] = read_buf[i];

   return TRUE;
}

/**
 * Write  memory in the current bank.  It is recommended that
 * when writing  data that some structure in the data is created
 * to provide error free reading back with readScratch().  Or the
 * method 'writePagePacketScratch()' could be used which automatically
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
SMALLINT writeScratch(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                      uchar *buff, int len)
{
   int i;
   uchar raw_buf[PAGE_LENGTH_SCR];
   uchar extra[3];

   // return if nothing to do
   if (len == 0)
      return TRUE;

   // check if write exceeds memory
   if(len > getSizeScratch(bank,SNum))
   {
      OWERROR(OWERROR_WRITE_OUT_OF_RANGE);
      return FALSE;
   }

   owSerialNum(portnum,SNum,FALSE);

   // write the page of data to scratchpad
   switch(SNum[0])
   {
      case 0x18:
         if(!writeScratchPadSHA(portnum,str_add,buff,len))
            return FALSE;
         break;

      case 0x21:
         if(!writeScratchPadEx(portnum,str_add,buff,len))
            return FALSE;
         break;

      case 0x23:
         if(!writeScratchPadEE(portnum,str_add,buff,len))
            return FALSE;
         break;

      default:
         if(!writeScratchpd(portnum,str_add,buff,len))
            return FALSE;
         break;
   }

   // read to verify ok
   switch(SNum[0])
   {
      case 0x18: case 0x21:
         if(readScratchPadCRC(portnum,raw_buf,PAGE_LENGTH_SCR,extra))
            return FALSE;
         break;

      default:
         if(!readScratchpdExtra(portnum,raw_buf,PAGE_LENGTH_SCR,extra))
            return FALSE;
         break;
   }

   // check to see if the same
   for (i=0;i<len;i++)
      if(raw_buf[i] != buff[i])
      {
         OWERROR(OWERROR_READ_SCRATCHPAD_VERIFY);
         return FALSE;
      }

   // check to make sure that the address is correct
   if ((((extra[0] & 0x00FF) | ((extra[1] << 8) & 0x00FF00))
           & 0x00FFFF) != str_add)
   {
      OWERROR(OWERROR_ADDRESS_READ_BACK_FAILED);
      return FALSE;
   }

   return TRUE;
}

/**
 * Read  page in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketScratch() method or have the 1-Wire device provide the
 * CRC as in readPageCRCScratch().  readPageCRCScratch() however is not
 * supported on all memory types, see 'hasPageAutoCRCScratch()'.
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
 * len      length in bytes to write
 *
 * @return - returns '0' if the read page wasn't completed.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageScratch(SMALLINT bank, int portnum, uchar *SNum, int page,
                         SMALLINT rd_cont, uchar *buff)
{
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   return readScratch(bank,portnum,SNum,page,rd_cont,buff,PAGE_LENGTH_SCR);
}

/**
 * Read  page with extra information in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketScratch() method or have the 1-Wire device provide the
 * CRC as in readPageCRCScratch().  readPageCRCScratch() however is not
 * supported on all memory types, see 'hasPageAutoCRCScratch()'.
 * If neither is an option then this method could be called more
 * then once to at least verify that the same thing is read consistantly.
 * See the method 'hasExtraInfoScratch()' for a description of the optional
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
 * len      length in bytes to write
 * extra    the extra information
 *
 * @return - returns '0' if the read page wasn't completed with extra info.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageExtraScratch(SMALLINT bank, int portnum, uchar *SNum, int page,
                              SMALLINT rd_cont, uchar *buff, uchar *extra)
{
   // check if read exceeds memory
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   owSerialNum(portnum,SNum,FALSE);

   // read the scratchpad, discard extra information
   switch(SNum[0])
   {
      case 0x18: case 0x21:
         if(!readPageExtraScratchCRC(bank,portnum,SNum,page,FALSE,buff,extra))
            return FALSE;
         break;

      default:
         if(!readScratchpdExtra(portnum,buff,PAGE_LENGTH_SCR,extra))
            return FALSE;
         break;
   }

   return TRUE;
}

/**
 * Read a complete memory page with CRC verification provided by the
 * device with extra information.  Not supported by all devices.
 * See the method 'hasPageAutoCRC()'.
 * See the method 'haveExtraInfo()' for a description of the optional
 * extra information.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to read
 * buff     byte array containing data that was read.
 * extra    the extra information
 *
 * @return - returns '0' if the read page wasn't completed with extra info.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageExtraCRCScratch(SMALLINT bank, int portnum, uchar *SNum, int page,
                                 uchar *read_buff, uchar *extra)
{
   switch(SNum[0])
   {
      case 0x21: case 0x18:
         if(!readPageExtraScratchCRC(bank,portnum,SNum,page,FALSE,read_buff,extra))
            return FALSE;
         break;

      default:
         OWERROR(OWERROR_CRC_EXTRA_INFO_NOT_SUPPORTED);
         return FALSE;
         break;
   }

   return TRUE;
}
/**
 * Read a complete memory page with CRC verification provided by the
 * device.  Not supported by all devices.  See the method
 * 'hasPageAutoCRCScratch()'.
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
SMALLINT readPageCRCScratch(SMALLINT bank, int portnum, uchar *SNum, int page, uchar *buff)
{
   switch(SNum[0])
   {
      case 0x21: case 0x18:
         if(!readPageScratchCRC(bank,portnum,SNum,page,FALSE,buff))
            return FALSE;
         break;

      default:
         OWERROR(OWERROR_CRC_NOT_SUPPORTED);
         return FALSE;
         break;
   }

   return TRUE;
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
SMALLINT readPagePacketScratch(SMALLINT bank, int portnum, uchar *SNum, int page,
                               SMALLINT rd_cont, uchar *buff, int *len)
{
   uchar raw_buf[PAGE_LENGTH_SCR];
   int i;
   ushort lastcrc16;

   // read the scratchpad, discard extra information
   if(!readScratch(bank,portnum,SNum,0,rd_cont,raw_buf,PAGE_LENGTH_SCR))
      return FALSE;

   // check if length is realistic
   if((raw_buf[0] > (PAGE_LENGTH_SCR-3)) || (raw_buf[0] <= 0))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort)((getStartingAddressScratch(bank,SNum)/PAGE_LENGTH_SCR) + page));
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
 * method 'readPagePacketScratch()' for a description of the packet structure.
 * See the method 'hasExtraInfoScratch()' for a description of the optional
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
SMALLINT readPagePacketExtraScratch(SMALLINT bank, int portnum, uchar *SNum,
                                    int page, SMALLINT rd_cont, uchar *buff,
                                    int *len, uchar *extra)
{
   uchar raw_buf[PAGE_LENGTH_SCR];
   int i;
   ushort lastcrc16;

   // read the scratchpad, discard extra information
   switch(SNum[0])
   {
      case 0x18: case 0x21:
         if(readScratchPadCRC(portnum,&raw_buf[0],page,extra))
            return FALSE;
         break;

      default:
         if(!readScratchpdExtra(portnum,&raw_buf[0],page,extra))
            return FALSE;
   }

   // check if length is realistic
   if((raw_buf[0] > (PAGE_LENGTH_SCR-3)) || (raw_buf[0] <= 0))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort)((getStartingAddressScratch(bank,SNum)/PAGE_LENGTH_SCR) + page));
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
 * Write a Universal Data Packet.  See the method 'readPagePacketScratch()'
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
SMALLINT writePagePacketScratch(SMALLINT bank, int portnum, uchar *SNum, int page,
                                uchar *buff, int len)
{
   uchar raw_buf[64];
   int i;
   ushort crc;

   // make sure length does not exceed max
   if ((len > (PAGE_LENGTH_SCR - 3)) || (len <= 0))
   {
      OWERROR(OWERROR_PACKET_LENGTH_EXCEEDS_PAGE);
      return FALSE;
   }

   // see if this bank is general read/write
   if (!isGeneralPurposeMemoryScratch(bank,SNum))
   {
      OWERROR(OWERROR_NOT_GENERAL_PURPOSE);
      return FALSE;
   }

   // construct the packet to write
   raw_buf[0] = (uchar) len + 3;

   for(i=1;i<len+1;i++)
      raw_buf[i] = buff[i-1];

   setcrc16(portnum,(ushort)((getStartingAddressScratch(bank,SNum)/PAGE_LENGTH_SCR) + page));
   for(i=0;i<len+1;i++)
      crc = docrc16(portnum,raw_buf[i]);

   raw_buf[len + 1] = (uchar) ~crc & 0xFF;
   raw_buf[len + 2] = (uchar) ((~crc & 0xFFFF) >> 8) & 0xFF;

   // write the packet, return result
   if(!writeScratch(bank,portnum,SNum,page,raw_buf,len+3))
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
SMALLINT getNumberPagesScratch(SMALLINT bank, uchar *SNum)
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
int getSizeScratch(SMALLINT bank, uchar *SNum)
{
   return SIZE_SCR;
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
int getStartingAddressScratch(SMALLINT bank, uchar *SNum)
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
SMALLINT getPageLengthScratch(SMALLINT bank, uchar *SNum)
{
   return PAGE_LENGTH_SCR;
}

/**
 * Query to see get a string description of the current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  String containing the memory bank description
 */
char *getBankDescriptionScratch(SMALLINT bank, uchar *SNum)
{
   switch(SNum[0])
   {
      case 0x18:
         return getBankDescriptionScratchSHA(bank,SNum);
         break;

      case 0x1A: case 0x1D:
         return getBankDescriptionScratchEx(bank,SNum);
         break;

      case 0x23:
         return getBankDescriptionScratchCRC(bank,SNum);
         break;

      default:
         return bankDescriptionSCR;
         break;
   }

   return "No description given.";
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
SMALLINT isGeneralPurposeMemoryScratch(SMALLINT bank, uchar *SNum)
{
   return generalPurposeMemorySCR;
}

/**
 * Query to see if current memory bank is read/write.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank is read/write
 */
SMALLINT isReadWriteScratch(SMALLINT bank, int portnum, uchar *SNum)
{
   return readWriteSCR;
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
SMALLINT isWriteOnceScratch(SMALLINT bank, int portnum, uchar *SNum)
{
   return writeOnceSCR;
}

/**
 * Query to see if current memory bank is read only.
 *
 * @return  'true' if current memory bank can only be read
 */
SMALLINT isReadOnlyScratch(SMALLINT bank, int portnum, uchar *SNum)
{
   return readOnlySCR;
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
SMALLINT isNonVolatileScratch(SMALLINT bank, uchar *SNum)
{
   return nonVolatileSCR;
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
SMALLINT needsProgramPulseScratch(SMALLINT bank, uchar *SNum)
{
   return needProgramPulseSCR;
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
SMALLINT needsPowerDeliveryScratch(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret;

   switch(SNum[0])
   {
      case 0x23:
         ret = TRUE;
         break;

      default:
         ret = needPowerDeliverySCR;
         break;
   }

   return ret;
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
SMALLINT hasExtraInfoScratch(SMALLINT bank, uchar *SNum)
{
   return ExtraInfoSCR;
}

/**
 * Query to get the length in bytes of extra information that
 * is read when read a page in the current memory bank.  See
 * 'hasExtraInfoScratch()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  number of bytes in Extra Information read when reading
 *          pages in the current memory bank.
 */
SMALLINT getExtraInfoLengthScratch(SMALLINT bank, uchar *SNum)
{
   return extraInfoLengthSCR;
}

/**
 * Query to get a string description of what is contained in
 * the Extra Informationed return when reading pages in the current
 * memory bank.  See 'hasExtraInfoScratch()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return string describing extra information.
 */
char *getExtraInfoDescScratch(SMALLINT bank, uchar *SNum)
{
   return extraInfoDescSCR;
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
SMALLINT hasPageAutoCRCScratch(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret;

   switch(SNum[0])
   {
      case 0x21: case 0x18:
         ret = hasPageAutoCRCScratchCRC(bank,SNum);
         break;

      default:
         ret = pageAutoCRCSCR;
         break;
   }

   return ret;
}

/**
 * Query to get Maximum data page length in bytes for a packet
 * read or written in the current memory bank.  See the 'ReadPagePacket()'
 * and 'WritePagePacket()' methods.  This method is only usefull
 * if the current memory bank is general purpose memory.
 *
 * @return  max packet page length in bytes in current memory bank
 */
SMALLINT getMaxPacketDataLengthScratch(SMALLINT bank, uchar *SNum)
{
   return PAGE_LENGTH_SCR - 3;
}

//--------
//-------- Bank specific methods
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
SMALLINT canRedirectPageScratch(SMALLINT bank, uchar *SNum)
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
SMALLINT canLockPageScratch(SMALLINT bank, uchar *SNum)
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
SMALLINT canLockRedirectPageScratch(SMALLINT bank, uchar *SNum)
{
   return FALSE;
}

//--------
//-------- ScratchPad methods
//--------

/**
 * Read the scratchpad page of memory from a NV device
 * This method reads and returns the entire scratchpad after the byte
 * offset regardless of the actual ending offset
 *
 * portnum       the port number of the port being used for the
 *               1-Wire Network.
 * readBuf       byte array to place read data into
 *                       length of array is always pageLength.
 * str_add       offset into readBuf to pug data
 * len           length in bytes to read
 * extraInfo     byte array to put extra info read into
 *               (TA1, TA2, e/s byte)
 *               length of array is always extraInfoLength.
 *               Can be 'null' if extra info is not needed.
 *
 * @return  'true' if reading scratch pad was successfull.
 */
SMALLINT readScratchpd(int portnum, uchar *buff, int len)
{
   int i;
   uchar raw_buf[4];

   // select the device
   if(!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // build first block
   raw_buf[0] = READ_SCRATCHPAD_COMMAND;

   for(i=1;i<extraInfoLengthSCR+1;i++)
      raw_buf[i] = 0xFF;

   // do the first block for TA1, TA2, and E/S
   if(!owBlock(portnum,FALSE,raw_buf,extraInfoLengthSCR+1))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   // build the next block
   for(i=0;i<len;i++)
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
 * Read the scratchpad page of memory from a NV device
 * This method reads and returns the entire scratchpad after the byte
 * offset regardless of the actual ending offset
 *
 * portnum       the port number of the port being used for the
 *               1-Wire Network.
 * readBuf       byte array to place read data into
 *                       length of array is always pageLength.
 * str_add       offset into readBuf to pug data
 * len           length in bytes to read
 * extraInfo     byte array to put extra info read into
 *               (TA1, TA2, e/s byte)
 *               length of array is always extraInfoLength.
 *               Can be 'null' if extra info is not needed.
 *
 * @return  'true' if reading scratch pad was successfull.
 */
SMALLINT readScratchpdExtra(int portnum, uchar *buff, int len, uchar *extra)
{
   int i;
   uchar raw_buf[PAGE_LENGTH_SCR+4];

   // select the device
   if(!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // build first block
   raw_buf[0] = READ_SCRATCHPAD_COMMAND;

   for(i=1;i<extraInfoLengthSCR+1;i++)
      raw_buf[i] = 0xFF;

   // do the first block for TA1, TA2, and E/S
   if(!owBlock(portnum,FALSE,raw_buf,extraInfoLengthSCR+1))
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
   for(i=1;i<extraInfoLengthSCR+1;i++)
      extra[i-1] = raw_buf[i];

   return TRUE;
}

/**
 * Write to the scratchpad page of memory a NV device.
 *
 * @param  startAddr     starting address
 * @param  writeBuf      byte array containing data to write
 * @param  offset        offset into readBuf to place data
 * @param  len           length in bytes to write
 *
 * @return 'true' if writing was successful
 */
SMALLINT writeScratchpd(int portnum,int str_add, uchar *writeBuf, int len)
{
   uchar raw_buf[PAGE_LENGTH_SCR+3];
   int i;

   // select the device
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // build block to send
   raw_buf[0] = WRITE_SCRATCHPAD_COMMAND;
   raw_buf[1] = (uchar) str_add & 0xFF;
   raw_buf[2] = (uchar) ((str_add & 0xFFFF) >> 8) & 0xFF;

   for(i=0;i<len;i++)
      raw_buf[i+3] = writeBuf[i];

   // send block, return result
   if(!owBlock(portnum,FALSE,&raw_buf[0],len + 3))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
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
SMALLINT copyScratchpd (int portnum, int str_add, int len)
{
   uchar raw_buf[5];

   // select the device
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // build block to send
   raw_buf[0] = COPY_SCRATCHPAD_COMMAND;
   raw_buf[1] = str_add & 0xFF;
   raw_buf[2] = ((str_add & 0xFFFF) >> 8) & 0xFF;
   raw_buf[3] = (str_add + len - 1) & 0x1F;
   raw_buf[4] = 0xFF;

   // send block (check copy indication complete)
   if(!owBlock(portnum,FALSE,raw_buf,5))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   if((raw_buf[4] & 0x0F0) != 0)
   {
      OWERROR(OWERROR_COPY_SCRATCHPAD_NOT_FOUND);
      return FALSE;
   }

   return TRUE;
}

