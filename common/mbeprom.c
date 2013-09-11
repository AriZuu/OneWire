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
// mbEPROM.c - Include memory bank EPROM functions.
//
// Version: 2.10
//

// Include Files
#include "ownet.h"
#include "mbeprom.h"

// general command defines 
#define READ_MEMORY_COMMAND_EPROM      0xF0
#define MAIN_READ_PAGE_COMMAND_EPROM   0xA5
#define STATUS_READ_PAGE_COMMAND_EPROM 0xAA
#define MAIN_WRITE_COMMAND_EPROM       0x0F
#define STATUS_WRITE_COMMAND_EPROM     0x55

// Local defines
#define SIZE_EPROM        2048
#define PAGE_LENGTH_EPROM 32

// Global variables
char *bankDescription_eprom     = "Main Memory";
SMALLINT  writeVerification_eprom    = TRUE;
SMALLINT  generalPurposeMemory_eprom = TRUE;
SMALLINT  readWrite_eprom            = FALSE;
SMALLINT  writeOnce_eprom            = TRUE;
SMALLINT  readOnly_eprom             = FALSE;
SMALLINT  nonVolatile_eprom          = TRUE;
SMALLINT  needsProgramPulse_eprom    = TRUE;
SMALLINT  needsPowerDelivery_eprom   = FALSE;
SMALLINT  hasExtraInfo_eprom         = TRUE;
SMALLINT  extraInfoLength_eprom      = 1;
char *extraInfoDesc_eprom       = "Inverted redirection page";
SMALLINT  pageAutoCRC_eprom          = TRUE;
SMALLINT  lockOffset                 = 0;
SMALLINT  lockRedirectOffset         = 0;
SMALLINT  canredirectPage            = FALSE;
SMALLINT  canlockPage                = FALSE;
SMALLINT  canlockRedirectPage        = FALSE;
SMALLINT  numPages                   = 64;
SMALLINT  CRCbytes                   = 2;


/**
 * Read  memory in the current bank with no CRC checking (device or
 * data). The resulting data from this API may or may not be what is on
 * the 1-Wire device.  It is recommends that the data contain some kind
 * of checking (CRC) like in the readPagePacketEPROM() method or have
 * the 1-Wire device provide the CRC as in readPageCRCEPROM().  readPageCRCEPROM()
 * however is not supported on all memory types, see 'hasPageAutoCRCEPROM()'.
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
SMALLINT readEPROM(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                   SMALLINT rd_cont, uchar *buff, int len)
{
   int i;
   int start_pg, end_pg, num_bytes, pg;
   uchar raw_buf[256];

   // check if read exceeds memory
   if ((str_add + len) > getSizeEPROM(bank,SNum))
   {
      OWERROR(OWERROR_READ_OUT_OF_RANGE);
      return FALSE;
   }

   // check if status memory
   if(readPageWithCRC(bank,SNum) == STATUS_READ_PAGE_COMMAND_EPROM)
   {
      // no regular read memory so must use readPageCRC
      start_pg = str_add / getPageLengthEPROM(bank,SNum);
      end_pg   = ((str_add + len) / getPageLengthEPROM(bank,SNum)) - 1;

      if (((str_add + len) % getPageLengthEPROM(bank,SNum)) > 0)
         end_pg++;

      // loop to read the pages
      for (pg = start_pg; pg <= end_pg; pg++)
         if(!readPageCRCEPROM(bank,portnum,SNum,pg,
                     &raw_buf[(pg-start_pg)*getPageLengthEPROM(bank,SNum)]))
         {
            return FALSE;
         }

      // extract out the data
      for(i=0;i<len;i++)
         buff[i] = raw_buf[i];
   }
   // regular memory so use standard read memory command
   else
   {

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
         raw_buf[0] = READ_MEMORY_COMMAND_EPROM;
         raw_buf[1] = (str_add + getStartingAddressEPROM(bank,SNum)) & 0xFF;
         raw_buf[2] = (((str_add + getStartingAddressEPROM(bank,SNum)) & 0xFFFF) >> 8)
                      & 0xFF;
         raw_buf[3] = 0xFF;

         // check if get a 1 byte crc in a normal read.
         if(SNum[0] == 0x09)
            num_bytes = 4;
         else
            num_bytes = 3;

         // do the first block for command, address
         if(!owBlock(portnum,FALSE,raw_buf,num_bytes))
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }
      }

      // pre-fill readBuf with 0xFF 
      for (i=0;i<len;i++)
         buff[i] = 0xFF;

      // send second block to read data, return result
      if(!owBlock(portnum,FALSE,buff,len))
      {
         OWERROR(OWERROR_DEVICE_SELECT_FAIL);
         return FALSE;
      }
   }

   return TRUE;
}

/**
 * Write  memory in the current bank.  It is recommended that
 * when writing  data that some structure in the data is created
 * to provide error free reading back with readEPROM().  Or the
 * method 'writePagePacketEPROM()' could be used which automatically
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
SMALLINT writeEPROM(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                    uchar *buff, int len)
{
   int      i;
   int      result;
   SMALLINT write_continue;
   int      crc_type;
   int      verify[256];

   // return if nothing to do
   if (len == 0)
      return TRUE;

   // check if power delivery is available
   if(!owHasProgramPulse(portnum))
   {
      OWERROR(OWERROR_NO_PROGRAM_PULSE);
      return FALSE;
   }

   // check if trying to write read only bank
   if(isReadOnlyEPROM(bank,portnum,SNum))
   {
      OWERROR(OWERROR_READ_ONLY);
      return FALSE;
   }

   for(i=0;i<len;i++)
   {
      verify[0] = writeVerify(bank,portnum,SNum,
          ((str_add+i+getStartingAddressEPROM(bank,SNum))/getPageLengthEPROM(bank,SNum)));

      if(isPageLocked(bank,portnum,SNum,(str_add+i+getStartingAddressEPROM(bank,SNum))/
                                         getPageLengthEPROM(bank,SNum)))
      {
         OWERROR(OWERROR_PAGE_LOCKED);
         return FALSE;
      }
   }

   // check if write exceeds memory
   if((str_add + len) > (getPageLengthEPROM(bank,SNum) * getNumberPagesEPROM(bank,SNum)))
   {
      OWERROR(OWERROR_WRITE_OUT_OF_RANGE);
      return FALSE;
   }

   // loop while still have bytes to write
   write_continue = TRUE;

   crc_type = numCRCbytes(bank,SNum) - 1;

   for (i=0;i<len;i++)
   {
      result = owProgramByte(portnum,buff[i],str_add+i+getStartingAddressEPROM(bank,SNum),
                             writeMemCmd(bank,SNum),crc_type,write_continue);
      if(verify[i])
      {
         if((result == -1) || ((uchar) result != buff[i]))
         {
            OWERROR(OWERROR_READBACK_EPROM_FAILED);
            return FALSE;
         }
         else
            write_continue = FALSE;
      }
   }

   return TRUE;
}

/**
 * Read  page in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketEPROM() method or have the 1-Wire device provide the
 * CRC as in readPageCRCEPROM().  readPageCRCEPROM() however is not
 * supported on all memory types, see 'hasPageAutoCRCEPROM()'.
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
SMALLINT readPageEPROM(SMALLINT bank, int portnum, uchar *SNum, int page,
                       SMALLINT rd_cont, uchar *buff)
{
   uchar extra[1];

   if(hasPageAutoCRCEPROM(bank,SNum))
   {
      if(!readPageExtraCRCEPROM(bank,portnum,SNum,page,buff,extra))
         return FALSE;
   }
   else
   {
      if(!readEPROM(bank,portnum,SNum,(page*getPageLengthEPROM(bank,SNum)),FALSE,
                    buff,getPageLengthEPROM(bank,SNum)))
         return FALSE;
   }

   return TRUE;
}

/**
 * Read  page with extra information in the current bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * readPagePacketEPROM() method or have the 1-Wire device provide the
 * CRC as in readPageCRCEPROM().  readPageCRCEPROM() however is not
 * supported on all memory types, see 'hasPageAutoCRCEPROM()'.
 * If neither is an option then this method could be called more
 * then once to at least verify that the same thing is read consistantly.
 * See the method 'hasExtraInfoEPROM()' for a description of the optional
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
SMALLINT readPageExtraEPROM(SMALLINT bank, int portnum, uchar *SNum, int page,
                            SMALLINT rd_cont, uchar *buff, uchar *extra)
{
   // check if current bank is not scratchpad bank, or not page 0
   if(!hasExtraInfoEPROM(bank,SNum))
   {
      OWERROR(OWERROR_EXTRA_INFO_NOT_SUPPORTED);
      return FALSE;
   }

   return readPageExtraCRCEPROM(bank,portnum,SNum,page,buff,extra);
}

/**
 * Read a complete memory page with CRC verification provided by the
 * device with extra information.  Not supported by all devices.
 * See the method 'hasPageAutoCRCEPROM()'.
 * See the method 'haveExtraInfoEPROM()' for a description of the optional
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
SMALLINT readPageExtraCRCEPROM(SMALLINT bank, int portnum, uchar *SNum, int page,
                               uchar *read_buff, uchar *extra)
{
   int    i, len = 0, lastcrc = 0, addr, extractPnt;
   uchar raw_buf[PAGE_LENGTH_EPROM + 6];

   owSerialNum(portnum,SNum,FALSE);

   // only needs to be implemented if supported by hardware
   if(!hasPageAutoCRCEPROM(bank,SNum))
   {
      OWERROR(OWERROR_CRC_NOT_SUPPORTED);
      return FALSE;
   }

   // check if read exceeds memory
   if(page > (getNumberPagesEPROM(bank,SNum)))
   {
      OWERROR(OWERROR_READ_OUT_OF_RANGE);
      return FALSE;
   }


   // select the device
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // build start reading memory block with: command, address, (extraInfo?), (crc?)
   len = 3 + getExtraInfoLengthEPROM(bank,SNum);

   if(crcAfterAdd(bank,SNum))
   {
      len += numCRCbytes(bank,SNum);
   }

   for(i=0;i<len;i++)
      raw_buf[i] = 0xFF;

   raw_buf[0] = readPageWithCRC(bank,SNum);

   addr = page * getPageLengthEPROM(bank,SNum) + getStartingAddressEPROM(bank,SNum);

   raw_buf[1] = addr & 0xFF;
   raw_buf[2] = ((addr & 0xFFFF) >> 8) & 0xFF;

   // do the first block 
   if(!owBlock(portnum,FALSE,&raw_buf[0],len))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   // check CRC
   if (numCRCbytes(bank,SNum) == 2)
   {
      setcrc16(portnum,0);
      for(i=0;i<len;i++)
         lastcrc = docrc16(portnum,raw_buf[i]);
   }
   else
   {
      setcrc8(portnum,0);
      for(i=0;i<len;i++)
         lastcrc = docrc8(portnum,raw_buf[i]);
   }

   if((getExtraInfoLengthEPROM(bank,SNum) > 0) || (crcAfterAdd(bank,SNum)))
   {
      // check CRC
      if(numCRCbytes(bank,SNum) == 2)
      {
         if (lastcrc != 0xB001)
         {
            OWERROR(OWERROR_CRC_FAILED);
            return FALSE;
         }
      }
      else
      {
         if (lastcrc != 0)
         {
            return FALSE;
         }
      }

      lastcrc = 0;

      // extract the extra information
      extractPnt = len - getExtraInfoLengthEPROM(bank,SNum) - numCRCbytes(bank,SNum);
      if(getExtraInfoLengthEPROM(bank,SNum))
         for(i=extractPnt;i<(extractPnt+getExtraInfoLengthEPROM(bank,SNum));i++)
            extra[i-extractPnt] = raw_buf[i];
   }

   // pre-fill with 0xFF 
   for(i=0;i<(getPageLengthEPROM(bank,SNum) + numCRCbytes(bank,SNum));i++)
      raw_buf[i] = 0xFF;

   // send block to read data + crc
   if(!owBlock(portnum,FALSE,&raw_buf[0],(getPageLengthEPROM(bank,SNum)+numCRCbytes(bank,SNum))))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   // check the CRC
   if (numCRCbytes(bank,SNum) == 2)
   {
      setcrc16(portnum, (ushort) lastcrc);
      for(i=0;i<(getPageLengthEPROM(bank,SNum)+numCRCbytes(bank,SNum));i++)
         lastcrc = docrc16(portnum,raw_buf[i]);

      if (lastcrc != 0xB001)
      {
         OWERROR(OWERROR_CRC_FAILED);
         return FALSE;
      }
   }
   else
   {
      setcrc8(portnum, (uchar) lastcrc);
      for(i=0;i<(getPageLengthEPROM(bank,SNum)+numCRCbytes(bank,SNum));i++)
         lastcrc = docrc8(portnum,raw_buf[i]);

      if (lastcrc != 0)
      {
         OWERROR(OWERROR_CRC_FAILED);
         return FALSE;
      }
   }

   // extract the page data
   for(i=0;i<getPageLengthEPROM(bank,SNum);i++)
   {
      read_buff[i] = raw_buf[i];
   }

   return TRUE;
}

/**
 * Read a complete memory page with CRC verification provided by the
 * device.  Not supported by all devices.  See the method
 * 'hasPageAutoCRCEPROM()'.
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
SMALLINT readPageCRCEPROM(SMALLINT bank, int portnum, uchar *SNum, int page, uchar *buff)
{
   uchar extra[1];

   return readPageExtraCRCEPROM(bank,portnum,SNum,page,buff,extra);
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
SMALLINT readPagePacketEPROM(SMALLINT bank, int portnum, uchar *SNum, int page,
                             SMALLINT rd_cont, uchar *buff, int *len)
{
   uchar  raw_buf[PAGE_LENGTH_EPROM];
   uchar  extra[1];
   ushort lastcrc16;
   int i;

   // read entire page with read page CRC
   if(!readPageExtraCRCEPROM(bank,portnum,SNum,page,&raw_buf[0],&extra[0]))
      return FALSE;

   // check if length is realistic
   if ((raw_buf[0] & 0xFF) > getMaxPacketDataLengthEPROM(bank,SNum))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort) ((getStartingAddressEPROM(bank,SNum)/PAGE_LENGTH_EPROM) + page));
   for(i=0;i<(raw_buf[0]+3);i++)
      lastcrc16 = docrc16(portnum,raw_buf[i]);

   if(lastcrc16 == 0xB001)
   {
      // extract the data out of the packet
      for(i=0;i<raw_buf[0];i++)
         buff[i] = raw_buf[i+1];

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
 * Read a Universal Data Packet and extra information.  See the
 * method 'readPagePacketEPROM()' for a description of the packet structure.
 * See the method 'hasExtraInfoEPROM()' for a description of the optional
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
SMALLINT readPagePacketExtraEPROM(SMALLINT bank, int portnum, uchar *SNum,
                                  int page, SMALLINT rd_cont, uchar *buff,
                                  int *len, uchar *extra)
{
   uchar raw_buf[PAGE_LENGTH_EPROM];
   ushort lastcrc16;
   int i;

   // check if current bank is not scratchpad bank, or not page 0
   if (!hasExtraInfoEPROM(bank,SNum))
   {
      OWERROR(OWERROR_EXTRA_INFO_NOT_SUPPORTED);
      return FALSE;
   }

   // read entire page with read page CRC
   if(!readPageExtraCRCEPROM(bank,portnum,SNum,page,raw_buf,extra))
      return FALSE;

   // check if length is realistic
   if ((raw_buf[0] & 0xFF) > getMaxPacketDataLengthEPROM(bank,SNum))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort) ((getStartingAddressEPROM(bank,SNum)/PAGE_LENGTH_EPROM) + page));
   for(i=0;i<(raw_buf[0]+3);i++)
      lastcrc16 = docrc16(portnum,raw_buf[i]);

   if(lastcrc16 == 0xB001)
   {
      // extract the data out of the packet
      for(i=0;i<raw_buf[0];i++)
         buff[i] = raw_buf[i+1];

      // return the length
      *len = raw_buf [0];
   }
   else
   {
      OWERROR(OWERROR_CRC_FAILED);
      return FALSE;
   }

   return TRUE;
}

/**
 * Write a Universal Data Packet.  See the method 'readPagePacketEPROM()'
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
SMALLINT writePagePacketEPROM(SMALLINT bank, int portnum, uchar *SNum, int page,
                              uchar *buff, int len)
{
   uchar raw_buf[PAGE_LENGTH_EPROM];
   int   crc, i;

   // make sure length does not exceed max
   if (len > getMaxPacketDataLengthEPROM(bank,SNum))
   {
      OWERROR(OWERROR_INVALID_PACKET_LENGTH);
      return FALSE;
   }

   // see if this bank is general read/write
   if (!isGeneralPurposeMemoryEPROM(bank,SNum))
   {
      OWERROR(OWERROR_NOT_GENERAL_PURPOSE);
      return FALSE;
   }

   // construct the packet to write
   raw_buf[0] = (uchar) len;

   for(i=0;i<len;i++)
      raw_buf[i+1] = buff[i];

   setcrc16(portnum,(ushort) ((getStartingAddressEPROM(bank,SNum)/PAGE_LENGTH_EPROM) + page));
   for(i=0;i<len+1;i++)
      crc = docrc16(portnum,raw_buf[i]);

   raw_buf[len + 1] = ~crc & 0xFF;
   raw_buf[len + 2] = ((~crc & 0xFFFF) >> 8) & 0xFF;

   // write the packet, return result
   if(!writeEPROM(bank,portnum,SNum,page*getPageLengthEPROM(bank,SNum)+
                  getStartingAddressEPROM(bank,SNum),&raw_buf[0],len+3))
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
SMALLINT getNumberPagesEPROM(SMALLINT bank, uchar *SNum)
{
   int pages = numPages;

   switch(SNum[0])
   {
      case 0x09:
         if(bank == 0)
            pages = 4;
         else if(bank == 1)
            pages = 1;
         break;

      case 0x0B:
         if((bank == 1) || (bank == 2) || (bank == 3))
            pages = 1;
         else if(bank == 4)
            pages = 8;
         break;

      case 0x0F:
         if(bank == 0)
            pages = 256;
         else if((bank == 1) || (bank == 2))
            pages = 4;
         else if(bank == 3)
            pages = 3;
         else if(bank == 4)
            pages = 32;
         break;

      case 0x12:
         if(bank == 0)
            pages = 4;
         else if(bank == 1)
            pages = 1;
         break;

      case 0x13:
         if(bank == 0)
            pages = 16;
         else if(bank == 4)
            pages = 2;
         else if((bank == 1) || (bank == 2) || (bank == 3))
            pages = 1;
         break;

      default:
         pages = numPages;
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
int getSizeEPROM(SMALLINT bank, uchar *SNum)
{
   int size = SIZE_EPROM;

   switch(SNum[0])
   {
      case 0x09:
         if(bank == 0)
            size = 128;
         else if(bank == 1)
            size = 8;
         break;

      case 0x0B:
         if((bank == 1) || (bank == 2) || (bank == 3))
            size = 8;
         else if(bank == 4)
            size = 64;
         break;

      case 0x0F:
         if(bank == 0)
            size = 8192;
         else if((bank == 1) || (bank == 2))
            size = 32;
         else if(bank == 3)
            size = 24;
         else if(bank == 4)
            size = 256;
         break;

      case 0x12:
         if(bank == 0)
            size = 128;
         else if(bank == 1)
            size = 8;
         break;

      case 0x13:
         if(bank == 0)
            size = 512;
         else if(bank == 4)
            size = 16;
         else if((bank == 1) || (bank == 2) || (bank == 3))
            size = 8;
         break;

      default:
         size = SIZE_EPROM;
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
int getStartingAddressEPROM(SMALLINT bank, uchar *SNum)
{
   int addr = 0;

   switch(SNum[0])
   {
      case 0x0B:
         if(bank == 2)
            addr = 32;
         else if(bank == 3)
            addr = 64;
         else if(bank == 4)
            addr = 256;
         break;

      case 0x0F:
         if(bank == 2)
            addr = 32;
         else if(bank == 3)
            addr = 64;
         else if(bank == 4)
            addr = 256;
         break;

      case 0x13:
         if(bank == 2)
            addr = 32;
         else if(bank == 3)
            addr = 64;
         else if(bank == 4)
            addr = 256;
         break;

      default:
         addr = 0;
         break;
   }

   return addr;
}

/**
 * Query to get page length in bytes in current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return   page length in bytes in current memory bank
 */
SMALLINT getPageLengthEPROM(SMALLINT bank, uchar *SNum)
{
   int len = PAGE_LENGTH_EPROM;

   switch(SNum[0])
   {
      case 0x09:
         if(bank == 1)
            len = 8;
         break;

      case 0x0B:
         if((bank > 0) && (bank < 5))
            len = 8;

      case 0x0F:
         if((bank > 0) && (bank <5))
            len = 8;
         break;

      case 0x12:
         if(bank == 1)
            len = 8;
         break;

      case 0x13:
         if((bank > 0) && (bank < 5))
            len = 8;
         break;

      default:
         len = PAGE_LENGTH_EPROM;
         break;
   }

   return len;
}

/**
 * Query to see get a string description of the current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  String containing the memory bank description
 */
char *getBankDescriptionEPROM(SMALLINT bank, uchar *SNum)
{
   switch(SNum[0])
   {
      case 0x09:
         if(bank == 1)
            return "Write protect pages and page redirection";
         break;

      case 0x0B:
         if(bank == 1)
            return "Write protect pages";
         else if(bank == 2)
            return "Write protect redirection";
         else if(bank == 3)
            return "Bitmap of used pages for file structure";
         else if(bank == 4)
            return "Page redirection bytes";
         break;

      case 0x0F:
         if(bank == 1)
            return "Write protect pages";
         else if(bank == 2)
            return "Write protect redirection";
         else if(bank == 3)
            return "Bitmap of used pages for file structure";
         else if(bank == 4)
            return "Pages redirection bytes";
         break;

      case 0x12:
         if(bank == 1)
            return "Write protect pages, page redirection, switch control";
         break;

      case 0x13:
         if(bank == 1)
            return "Write protect pages";
         else if(bank == 2)
            return "Write protect redirection";
         else if(bank == 3)
            return "Bitmap of used pages for file structure";
         else if(bank == 4)
            return "Page redirection bytes";
         break;

      default:
         return bankDescription_eprom;
   }

   return bankDescription_eprom;
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
SMALLINT isGeneralPurposeMemoryEPROM(SMALLINT bank, uchar *SNum)
{
   int gp_mem = generalPurposeMemory_eprom;

   switch(SNum[0])
   {
      case 0x09:
         if(bank == 1)
            gp_mem = FALSE;
         break;

      case 0x0B:
         if((bank > 0) && (bank < 5))
            gp_mem = FALSE;

      case 0x0F:
         if((bank > 0) && (bank <5))
            gp_mem = FALSE;
         break;

      case 0x12:
         if(bank == 1)
            gp_mem = FALSE;
         break;

      case 0x13:
         if((bank > 0) && (bank < 5))
            gp_mem = FALSE;
         break;

      default:
         gp_mem = generalPurposeMemory_eprom;
         break;
   }

   return gp_mem;
}

/**
 * Query to see if current memory bank is read/write.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank is read/write
 */
SMALLINT isReadWriteEPROM(SMALLINT bank, int portnum, uchar *SNum)
{
   return readWrite_eprom;
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
SMALLINT isWriteOnceEPROM(SMALLINT bank, int portnum, uchar *SNum)
{
   return writeOnce_eprom;
}

/**
 * Query to see if current memory bank is read only.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can only be read
 */
SMALLINT isReadOnlyEPROM(SMALLINT bank, int portnum, uchar *SNum)
{
   return readOnly_eprom;
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
SMALLINT isNonVolatileEPROM(SMALLINT bank, uchar *SNum)
{
   return nonVolatile_eprom;
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
SMALLINT needsProgramPulseEPROM(SMALLINT bank, uchar *SNum)
{
   return needsProgramPulse_eprom;
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
SMALLINT needsPowerDeliveryEPROM(SMALLINT bank, uchar *SNum)
{
   return needsPowerDelivery_eprom;
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
SMALLINT hasExtraInfoEPROM(SMALLINT bank, uchar *SNum)
{
   SMALLINT hasExtra = hasExtraInfo_eprom;

   switch(SNum[0])
   {
      case 0x09:
         hasExtra = FALSE;
         break;

      case 0x0B:
         if((bank > 0) && (bank < 5))
            hasExtra = FALSE;
         break;

      case 0x0F:
         if((bank > 0) && (bank < 5))
            hasExtra = FALSE;
         break;

      case 0x12:
         if(bank == 1)
            hasExtra = FALSE;
         break;

      case 0x13:
         if((bank > 0) && (bank < 5))
            hasExtra = FALSE;
         break;

      default:
         hasExtra = hasExtraInfo_eprom;
         break;
   }

   return hasExtra;
}

/**
 * Query to get the length in bytes of extra information that
 * is read when read a page in the current memory bank.  See
 * 'hasExtraInfoEPROM()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  number of bytes in Extra Information read when reading
 *          pages in the current memory bank.
 */
SMALLINT getExtraInfoLengthEPROM(SMALLINT bank, uchar *SNum)
{
   SMALLINT len = extraInfoLength_eprom;

   switch(SNum[0])
   {
      case 0x09:
         len = 0;
         break;

      case 0x0B:
         if((bank > 0) && (bank < 5))
            len = 0;
         break;

      case 0x0F:
         if((bank > 0) && (bank < 5))
            len = 0;
         break;

      case 0x12:
         if(bank == 1)
            len = 0;
         break;

      case 0x13:
         if((bank > 0) && (bank < 5))
            len = 0;
         break;

      default:
         len = extraInfoLength_eprom;
         break;
   }

   return len;
}

/**
 * Query to get a string description of what is contained in
 * the Extra Informationed return when reading pages in the current
 * memory bank.  See 'hasExtraInfoEPROM()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return string describing extra information.
 */
char *getExtraInfoDescEPROM(SMALLINT bank, uchar *SNum)
{
   switch(SNum[0])
   {
      case 0x09:
         return " ";
         break;

      case 0x0B:
         if((bank > 0) && (bank < 5))
            return " ";
         break;

      case 0x0F:
         if((bank > 0) && (bank < 5))
            return " ";
         break;

      case 0x12:
         if(bank == 1)
            return " ";
         break;

      case 0x13:
         if((bank > 0) && (bank < 5))
            return " ";
         break;

      default:
         return extraInfoDesc_eprom;
   }


   return extraInfoDesc_eprom;
}

/**
 * Query to see if current memory bank pages can be read with
 * the contents being verified by a device generated CRC.
 * This is used to see if the 'ReadPageCRCEPROM()' can be used.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can be read with self
 *          generated CRC.
 */
SMALLINT hasPageAutoCRCEPROM(SMALLINT bank, uchar *SNum)
{
   return pageAutoCRC_eprom;
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
SMALLINT getMaxPacketDataLengthEPROM(SMALLINT bank, uchar *SNum)
{
   return getPageLengthEPROM(bank,SNum) - 3;
}

//--------
//-------- OTPMemoryBank I/O methods
//--------

/**
 * Lock the specifed page in the current memory bank.  Not supported
 * by all devices.  See the method 'canLockPageEPROM()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to be locked
 *
 * @return  true if the page was locked 
 *          false otherwise
 */
SMALLINT lockPage(SMALLINT bank, int portnum, uchar *SNum, int page)
{
   int nbyt,nbit;
   SMALLINT write_bank;
   uchar wr_byte[1];

   // setting the bank to write
   // and since all the page lock info is in
   // bank 1 for all the EPROM parts.
   write_bank = 1;

   if(isPageLocked(write_bank,portnum,SNum,page))
   {
      OWERROR(OWERROR_PAGE_ALREADY_LOCKED);
      return FALSE;
   }

   // create byte to write to mlLock to lock page
   nbyt    = (page >> 3);
   nbit    = page - (nbyt << 3);

   wr_byte[0] = (uchar) ~(0x01 << nbit);

   // write the lock bit
   if(!writeEPROM(write_bank,portnum,SNum,nbyt + lockOffset,wr_byte,1))
      return FALSE;

   // read back to verify
   if(!isPageLocked(write_bank,portnum,SNum,page))
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
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to check if locked
 *
 * @return  true if page locked.
 */
SMALLINT isPageLocked(SMALLINT bank, int portnum, uchar *SNum, int page)
{
   int pg_len, read_pg, index, nbyt, nbit;
   SMALLINT read_bank;
   uchar read_buf[PAGE_LENGTH_EPROM];

   // setting the bank to read for information
   // and since all the page lock info is in
   // bank 1 for all the EPROM parts.
   read_bank = 1;

   // read page that locked bit is on 
   pg_len  = getPageLengthEPROM(read_bank,SNum);
   read_pg = (page + lockOffset) / (pg_len * 8);

   if(!readPageCRCEPROM(read_bank,portnum,SNum,read_pg,read_buf))
      return FALSE;

   // return boolean on locked bit
   index = (page + lockOffset) - (read_pg * 8 * pg_len);
   nbyt  = (index >> 3);
   nbit  = index - (nbyt << 3);

   return !(((read_buf[nbyt] >> nbit) & 0x01) == 0x01);
}

/**
 * Redirect the specifed page in the current memory bank to a new page.
 * Not supported by all devices.  See the method 'canRedirectPage()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     the page to be locked
 * newPage  new page number to redirect to
 *
 * @return  true if the page was redirected
 */
SMALLINT redirectPage(SMALLINT bank, int portnum, uchar *SNum, int page, int newPage)
{
   uchar wr_byte[1];  // create byte to redirect page
   SMALLINT write_bank;

   if(isRedirectPageLocked(bank,portnum,SNum,page))
   {
      OWERROR(OWERROR_REDIRECTED_PAGE);
      return FALSE;
   }

   // setting the bank to write
   switch(SNum[0])
   {
      case 0x09: case 0x12:
         write_bank = 1;
         break;

      case 0x0B: case 0x0F: case 0x13:
         write_bank = 4;
         break;

      default:
         write_bank = 4;
         break;
   }

   wr_byte[0] = (uchar) ~newPage;

   // write the redirection byte
   if(!writeEPROM(write_bank,portnum,SNum,page + redirectOffset(bank,SNum),wr_byte,1))
      return FALSE;

   return TRUE;
}


/**
 * Gets the page redirection of the specified page.
 * Not supported by all devices. 
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     number of page check for redirection
 *
 * @return  the new page number or 0 if not redirected
 */
SMALLINT getRedirectedPage(SMALLINT bank, int portnum, uchar *SNum, int page)
{
   int pg_len, read_pg;
   SMALLINT read_bank;
   uchar read_buf[PAGE_LENGTH_EPROM];

   // setting the bank to read for information
   switch(SNum[0])
   {
      case 0x09: case 0x12:
         read_bank = 1;
         break;

      case 0x0B: case 0x0F: case 0x13:
         read_bank = 4;
         break;

      default:
         read_bank = 4;
         break;
   }

   // read page that redirect byte is on 
   pg_len  = getPageLengthEPROM(read_bank,SNum);
   read_pg = (page + redirectOffset(bank,SNum)) / pg_len;

   if(!readPageCRCEPROM(read_bank,portnum,SNum,read_pg,read_buf))
      return FALSE;

   // return page
   return (SMALLINT) (~read_buf[(page + redirectOffset(bank,SNum)) % pg_len] & 0x000000FF);
}

/**
 * Lock the redirection option for the specifed page in the current
 * memory bank. Not supported by all devices.  See the method
 * 'canLockRedirectPage()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     number of page to redirect
 *
 * @return  true if the redirected page is locked.
 */
SMALLINT lockRedirectPage(SMALLINT bank, int portnum, uchar *SNum, int page)
{
   int nbyt, nbit;
   SMALLINT write_bank;
   uchar wr_byte[1];

   if(isRedirectPageLocked(bank,portnum,SNum,page))
   {
      OWERROR(OWERROR_LOCKING_REDIRECTED_PAGE_AGAIN);
      return FALSE;
   }

   // setting the bank to write
   switch(SNum[0])
   {
      case 0x0B: case 0x0F: case 0x13:
         write_bank = 2;
         break;

      default:
         write_bank = 0;
         break;
   }

   // create byte to write to mlLock to lock page
   nbyt    = (page >> 3);
   nbit    = page - (nbyt << 3);

   wr_byte[0] = (uchar) ~(0x01 << nbit);

   // write the lock bit
   if(!writeEPROM(write_bank,portnum,SNum,nbyt + lockRedirectOffset,wr_byte,1))
      return FALSE;
   
   // read back to verify
   if (!isRedirectPageLocked(bank,portnum,SNum,page) || (write_bank == 0))
   {
      OWERROR(OWERROR_COULD_NOT_LOCK_REDIRECT);
      return FALSE;
   }

   return TRUE;
}

/**
 * Query to see if the specified page has redirection locked.
 * Not supported by all devices.  See the method 'canRedirectPage()'.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part.
 * page     number of page to check for redirection
 *
 * @return  true if redirection is locked for this page
 */
SMALLINT isRedirectPageLocked(SMALLINT bank, int portnum, uchar *SNum, int page)
{
   int pg_len, read_pg, index, nbyt, nbit;
   SMALLINT read_bank;
   uchar read_buf[PAGE_LENGTH_EPROM];

   // setting the bank to read for information
   switch(SNum[0])
   {
      case 0x0B: case 0x0F: case 0x13:
         read_bank = 2;
         break;

      default:
         return FALSE;
   }

   // read page that lock redirect bit is on 
   pg_len  = getPageLengthEPROM(read_bank,SNum);
   read_pg = (page + lockRedirectOffset) / (pg_len * 8);

   if(!readPageCRCEPROM(read_bank,portnum,SNum,read_pg,read_buf))
      return FALSE;

   // return boolean on lock redirect bit
   index = (page + lockRedirectOffset) - (read_pg * 8 * pg_len);
   nbyt  = (index >> 3);
   nbit  = index - (nbyt << 3);

   return !(((read_buf[nbyt] >> nbit) & 0x01) == 0x01);
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
SMALLINT canRedirectPageEPROM(SMALLINT bank, uchar *SNum)
{
   SMALLINT canRedirect = canredirectPage;

   switch(SNum[0])
   {
      case 0x09:
         if(bank == 0)
            canRedirect = TRUE;
         break;

      case 0x0B:
         if(bank == 0)
            canRedirect = TRUE;
         break;

      case 0x0F:
         if(bank == 0)
            canRedirect = TRUE;
         break;

      case 0x12:
         if(bank == 0)
            canRedirect = TRUE;
         break;

      case 0x13:
         if(bank == 0)
            canRedirect = TRUE;
         break;

      default:
         canRedirect = canredirectPage;
         break;
   }

   return canRedirect;
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
SMALLINT canLockPageEPROM(SMALLINT bank, uchar *SNum)
{
   SMALLINT canLock = canlockPage;

   switch(SNum[0])
   {
      case 0x09:
         if(bank == 0)
            canLock = TRUE;
         break;

      case 0x0B:
         if(bank == 0)
            canLock = TRUE;
         break;

      case 0x0F:
         if(bank == 0)
            canLock = TRUE;
         break;

      case 0x12:
         if(bank == 0)
            canLock = TRUE;
         break;

      case 0x13:
         if(bank == 0)
            canLock = TRUE;
         break;

      default:
         canLock = canlockPage;
         break;
   }

   return canLock;
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
SMALLINT canLockRedirectPageEPROM(SMALLINT bank, uchar *SNum)
{
   SMALLINT lockRedirect = canlockRedirectPage;

   switch(SNum[0])
   {
      case 0x0B:
         if(bank == 0)
            lockRedirect = TRUE;
         break;

      case 0x0F:
         if(bank == 0)
            lockRedirect = TRUE;
         break;

      case 0x13:
         if(bank == 0)
            lockRedirect = TRUE;
         break;

      default:
         lockRedirect = canlockRedirectPage;
         break;
   }

   return lockRedirect;
}


//--------
//-------- Local functions
//--------

/**
 * Send the command for writing to memory for a device given
 * the memory bank and serial number.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  the command for writing to the memory bank.
 */
uchar writeMemCmd(SMALLINT bank, uchar *SNum)
{
   uchar command = MAIN_WRITE_COMMAND_EPROM;
   
   switch(SNum[0])
   {
      case 0x09:
         if(bank == 1)
            command = STATUS_WRITE_COMMAND_EPROM;
         break;

      case 0x0B:
         if((bank > 0) && (bank < 5))
            command = STATUS_WRITE_COMMAND_EPROM;
         break;

      case 0x0F:
         if((bank > 0) && (bank < 5))
            command = STATUS_WRITE_COMMAND_EPROM;
         break;

      case 0x12:
         if(bank == 1)
            command = STATUS_WRITE_COMMAND_EPROM;
         break;

      case 0x13:
         if((bank > 0) && (bank < 5))
            command = STATUS_WRITE_COMMAND_EPROM;
         break;

      default:
         command = MAIN_WRITE_COMMAND_EPROM;
         break;
   }

   return command;
}

/**
 * Get the number of CRC bytes for the device.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  the number of CRC bytes.
 */
SMALLINT numCRCbytes(SMALLINT bank, uchar *SNum)
{
   SMALLINT numbytes = CRCbytes;

   switch(SNum[0])
   {
      case 0x09:
         if((bank == 0) || (bank == 1))
            numbytes = 1;
         break;

      default:
         numbytes = CRCbytes;
         break;
   }

   return numbytes;
}

/**
 * Get the status of the page the pages write verification
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  true if write verification is used and
 *          false if it is not used.
 */
SMALLINT writeVerify(SMALLINT bank, int portnum, uchar *SNum, int page)
{
   if(isPageLocked(bank,portnum,SNum,page) || 
      isRedirectPageLocked(bank,portnum,SNum,page))
      return FALSE;

   return writeVerification_eprom;
}

/**
 * Get crc after sending command,address
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  true if crc is can be gotten after sending command,address
 *          false otherwise
 */
SMALLINT crcAfterAdd(SMALLINT bank, uchar *SNum)
{
   SMALLINT crcAfter = TRUE;

   switch(SNum[0])
   {
      case 0x0B:
         if((bank > 0) && (bank < 5))
            crcAfter = FALSE;
         break;

      case 0x0F:
         if((bank > 0) && (bank < 5))
            crcAfter = FALSE;
         break;

      case 0x12:
         if(bank == 1)
            crcAfter = FALSE;
         break;

      case 0x13:
         if((bank > 0) && (bank < 5))
            crcAfter = FALSE;
         break;

      default:
         crcAfter = TRUE;
         break;
   }

   return crcAfter;
}

/**
 * Get the byte command for reading the page with crc.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  the byte command for reading the page
 */
uchar readPageWithCRC(SMALLINT bank, uchar *SNum)
{
   uchar readPage = MAIN_READ_PAGE_COMMAND_EPROM;

   switch(SNum[0])
   {
      case 0x09:
         if(bank == 0)
            readPage = 0xC3;
         else if(bank == 1)
            readPage = STATUS_READ_PAGE_COMMAND_EPROM;
         break;

      case 0x0B:
         if((bank > 0) && (bank < 5))
            readPage = STATUS_READ_PAGE_COMMAND_EPROM;
         break;

      case 0x0F:
         if((bank > 0) && (bank < 5))
            readPage = STATUS_READ_PAGE_COMMAND_EPROM;
         break;

      case 0x12:
         if(bank == 1)
            readPage = STATUS_READ_PAGE_COMMAND_EPROM;
         break;

      case 0x13:
         if((bank > 0) && (bank < 5))
            readPage = STATUS_READ_PAGE_COMMAND_EPROM;
         break;

      default:
         readPage = MAIN_READ_PAGE_COMMAND_EPROM;
         break;
   }

   return readPage;
}

/**
 * Get the offset into the redirection status information.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  the offset into the redirection status
 */
SMALLINT redirectOffset(SMALLINT bank, uchar *SNum)
{
   int offset = 0;

   switch(SNum[0])
   {
      case 0x09:
         if(bank == 0)
            offset = 1;
         break;
         
      case 0x12:
         if(bank == 0)
            offset = 1;
         break;

      default:
         offset = 0;
         break;
   }

   return offset;
}
