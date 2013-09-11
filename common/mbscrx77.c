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
// mbSCRx77.h - Memory bank class for the Scratchpad section of DS1977
//
// Version: 2.10
//

// Include Files
#include "ownet.h"
#include "mbscrx77.h"
#include "pw77.h"

// general command defines 
#define WRITE_SCRATCHPAD_COMMAND_EX  0x0F
#define COPY_SCRATCHPAD_CMD_PW       0x99
#define READ_SCRATCHPAD_COMMAND_CRC  0xAA
#define MATCH_COMMAND                0x55

// Local defines
#define SIZE_SCRATCH_EX           64
#define PAGE_LENGTH_SCRATCH_EX    64
#define SIZE_SCRATCH_EX_HYGRO     32
#define PAGE_LENGTH_SCRATCH_HYGRO 32

SMALLINT generalPurposeMemorySCREx77 = FALSE;
SMALLINT readWriteSCREx77            = TRUE;
SMALLINT writeOnceSCREx77            = FALSE;
SMALLINT readOnlySCREx77             = FALSE;
SMALLINT nonVolatileSCREx77          = FALSE;
SMALLINT needProgramPulseSCREx77     = FALSE;
SMALLINT needPowerDeliverySCREx77    = TRUE;
SMALLINT ExtraInfoSCREx77            = TRUE;
SMALLINT extraInfoLengthSCREx77      = 4;
char     *extraInfoDescSCREx77       = "Address, Ending offset and Data Status";
SMALLINT pageAutoCRCScratchEx77      = TRUE;


/**
 * Write to the scratchpad page of memory a NV device.
 *
 * portnum     the port number of the port being used for the
 *               1-Wire Network.
 * SNum        the serial number for the part.
 * str_add     starting address
 * writeBuf    byte array containing data to write
 * len         length in bytes to write
 *
 * @return 'true' if writing was successful
 */
SMALLINT writeScratchPadEx77(int portnum, uchar *SNum, int str_add, uchar *writeBuf, int len)
{
   int send_len = 0;
   int i,checkcrc = FALSE;
   uchar raw_buf[85];
   ushort lastcrc16;

   if(SNum[0] != 0x37)
   {
      if(((str_add + len) & 0x1F) != 0) 
      {
         OWERROR(OWERROR_EOP_WRITE_SCRATCHPAD_FAILED);
         return FALSE;
      }
   }

   // reset
   if(!owTouchReset(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // add match command
   raw_buf[send_len++] = MATCH_COMMAND;
   for (i = 0; i < 8; i++)
      raw_buf[send_len++] = SNum[i];

   // build block to send
   raw_buf[send_len++] = WRITE_SCRATCHPAD_COMMAND_EX;
   raw_buf[send_len++] = str_add & 0xFF;
   raw_buf[send_len++] = ((str_add & 0xFFFF) >> 8) & 0xFF;

   for(i = 0; i < len; i++)
      raw_buf[send_len++] = writeBuf[i];

   // check if full page (can utilize CRC)
   if(SNum[0] == 0x37)
   {
      if (((str_add+len) % PAGE_LENGTH_SCRATCH_EX) == 0)
      {
         for(i = 0; i < 2; i++)
            raw_buf[send_len++] = 0xFF;

         checkcrc = TRUE;
      }
   }
   else
   {
      if (((str_add+len) % PAGE_LENGTH_SCRATCH_HYGRO) == 0)
      {
         for(i = 0; i < 2; i++)
            raw_buf[send_len++] = 0xFF;

         checkcrc = TRUE;
      }
   }

   // send block, return result 
   if(!owBlock(portnum,FALSE,raw_buf,send_len))  
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   // verify echo of bytes was correct
   for (i = 12; i < send_len - (checkcrc ? 2 : 0); i++)  
   {
      if (raw_buf[i] != writeBuf[i - 12])  
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }
   }

   // check if crc is available
   if(checkcrc)
   {
      // verify the CRC is correct
      setcrc16(portnum,0);
      for(i = 9; i < send_len; i++)  
         lastcrc16 = docrc16(portnum,raw_buf[i]);

      if(lastcrc16 != 0xB001)
      {
         OWERROR(OWERROR_CRC_FAILED);
         return FALSE;
      }
   }

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
 * SNum          the serial number for the part.
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
SMALLINT readScratchPadCRC77(int portnum, uchar *SNum, uchar *buff, int len, uchar *extra)
{
   int i,num_crc,addr,send_len=0,addr_offset;
   uchar raw_buf[110];
   ushort lastcrc16;

   // reset
   if(!owTouchReset(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // add match command
   raw_buf[send_len++] = MATCH_COMMAND;
   for (i = 0; i < 8; i++)
      raw_buf[send_len++] = SNum[i];

   // build block
   raw_buf[send_len++] = READ_SCRATCHPAD_COMMAND_CRC;

   addr_offset = send_len;
   if(SNum[0] == 0x37)
   {
      for(i = 0; i < 69; i++)
         raw_buf[send_len++] = 0xFF;
   }
   else
   {
      for(i=0;i<37;i++)
         raw_buf[send_len++] = 0xFF;
   }

   // send block, command + (extra) + page data + CRC
   if(!owBlock(portnum,FALSE,&raw_buf[0],send_len))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   // get the starting offset to see when the crc will show up
   addr = raw_buf[addr_offset];
   addr = (addr | ((raw_buf[addr_offset+1] << 8) & 0xFF00)) & 0xFFFF;
   if(SNum[0] == 0x37)
   {
      num_crc = 76 - (addr & 0x003F) + 3;
   }
   else
   {
      num_crc = 44 - (addr & 0x001F) + 3;
   }

   // check crc of entire block (minus match)
   setcrc16(portnum,0);
   for(i = addr_offset - 1; i < num_crc; i++)
      lastcrc16 = docrc16(portnum,raw_buf[i]);

   if(lastcrc16 != 0xB001)
   {
      OWERROR(OWERROR_CRC_FAILED);
      return FALSE;
   }

   // extract the extra info
   for(i = addr_offset; i < (addr_offset + 4); i++)
      extra[i - addr_offset] = raw_buf[i];

   // extract the page data
   if(SNum[0] == 0x37)
   {
      for(i = (addr_offset + 3); i < ((addr_offset + 3) + 64); i++)
         buff[i - (addr_offset + 3)] = raw_buf[i];
   }
   else
   {
      for(i = (addr_offset + 3); i < ((addr_offset + 3) + 32); i++)
         buff[i - (addr_offset + 3)] = raw_buf[i];
   }

   return TRUE;
}


/**
 * Copy the scratchpad page to memory.
 *
 * portnum       the port number of the port being used for the
 *               1-Wire Network.
 * SNum          the serial number for the part.
 * str_add       starting address
 * len           length in bytes that was written already
 *
 * @return       'true' if the coping was successful.
 */
SMALLINT copyScratchPadPsw77(int portnum, uchar *SNum, int str_add, int len, uchar *psw)
{
   uchar raw_buf[25];
   SMALLINT i,send_len=0,rslt;

   if(SNum[0] != 0x37)
   {
      if(((str_add + len) & 0x1F) != 0)
      {
         OWERROR(OWERROR_EOP_COPY_SCRATCHPAD_FAILED);
         return FALSE;
      }
   }

   // reset
   if(!owTouchReset(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // add match command
   raw_buf[send_len++] = MATCH_COMMAND;
   for (i = 0; i < 8; i++)
      raw_buf[send_len++] = SNum[i];

   // command, address, offset, password (except last byte)
   raw_buf[send_len++] = COPY_SCRATCHPAD_CMD_PW;
   raw_buf[send_len++] = str_add & 0xFF;
   raw_buf[send_len++] = ((str_add & 0xFFFF) >> 8) & 0xFF;
   if(SNum[0] == 0x37)
      raw_buf[send_len++] = (str_add + len - 1) & 0x3F;
   else
      raw_buf[send_len++] = (str_add + len - 1) & 0x1F;

   for (i = 0; i < 7; i++)
      raw_buf[send_len++] = psw[i];
      
   // send block (check copy indication complete)
   if(!owBlock(portnum,FALSE,raw_buf,send_len))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   // send last byte of password and enable strong pullup
   if (!owWriteBytePower(portnum, psw[7]))
   {
      OWERROR(OWERROR_WRITE_BYTE_FAILED);
      return FALSE;
   }

   // delay for copy to complete
   msDelay(10);  //????????10, could be 5

   // check result
   rslt = owReadByte(portnum);
  
   if (((rslt & 0xF0) != 0xA0) && ((rslt & 0xF0) != 0x50))
   {
      OWERROR(OWERROR_COPY_SCRATCHPAD_NOT_FOUND);
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
 *
 * @return - returns '0' if the read page wasn't completed.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page,
                             SMALLINT rd_cont, uchar *buff)
{
   uchar extra[4];

   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   if(SNum[0] == 0x37)
      return readScratchPadCRC77(portnum,SNum,buff,PAGE_LENGTH_SCRATCH_EX,extra);
   else
      return readScratchPadCRC77(portnum,SNum,buff,PAGE_LENGTH_SCRATCH_HYGRO,extra);
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
 * extra    the extra information
 *
 * @return - returns '0' if the read page wasn't completed with extra info.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageExtraScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page,
                                  SMALLINT rd_cont, uchar *buff, uchar *extra)
{
   // check if read exceeds memory
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   // read the scratchpad
   if(SNum[0] == 0x37)
      return readScratchPadCRC77(portnum,SNum,buff,PAGE_LENGTH_SCRATCH_EX,extra);
   else
      return readScratchPadCRC77(portnum,SNum,buff,PAGE_LENGTH_SCRATCH_HYGRO,extra);
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
SMALLINT readPageExtraCRCScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page,
                                     uchar *read_buff, uchar *extra)
{
   // check if read exceeds memory
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   if(SNum[0] == 0x37)
      return readScratchPadCRC77(portnum,SNum,read_buff,PAGE_LENGTH_SCRATCH_EX,extra);
   else
      return readScratchPadCRC77(portnum,SNum,read_buff,PAGE_LENGTH_SCRATCH_HYGRO,extra);
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
SMALLINT readPageCRCScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page, uchar *buff)
{
   uchar extra[4];

   // check if read exceeds memory
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   if(SNum[0] == 0x37)
      return readScratchPadCRC77(portnum,SNum,buff,PAGE_LENGTH_SCRATCH_EX,extra);
   else
      return readScratchPadCRC77(portnum,SNum,buff,PAGE_LENGTH_SCRATCH_HYGRO,extra);
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
SMALLINT readPagePacketScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page,
                                   SMALLINT rd_cont, uchar *buff, int *len)
{ 
   uchar raw_buf[PAGE_LENGTH_SCRATCH_EX];
   uchar extra[4];
   int i;
   ushort lastcrc16;

   // check if read exceeds memory
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   // read the scratchpad, discard extra information
   if(SNum[0] == 0x37)
   {
      if(!readScratchPadCRC77(portnum,SNum,raw_buf,PAGE_LENGTH_SCRATCH_EX,extra))
         return FALSE;
   }
   else
   {
      if(!readScratchPadCRC77(portnum,SNum,raw_buf,PAGE_LENGTH_SCRATCH_HYGRO,extra))
         return FALSE;
   }

   // check if length is realistic
   if(SNum[0] == 0x37)
   {
      if((raw_buf[0] > (PAGE_LENGTH_SCRATCH_EX-3)) || (raw_buf[0] <= 0))
      {
         OWERROR(OWERROR_INVALID_PACKET_LENGTH);
         return FALSE;
      }
   }
   else
   {
      if((raw_buf[0] > (PAGE_LENGTH_SCRATCH_HYGRO-3)) || (raw_buf[0] <= 0))
      {
         OWERROR(OWERROR_INVALID_PACKET_LENGTH);
         return FALSE;
      }
   }

   // verify the CRC is correct
   if(SNum[0] == 0x37)
   {
      setcrc16(portnum,
         (ushort)((getStartingAddressScratchEx77(bank,SNum)/PAGE_LENGTH_SCRATCH_EX) + page));
   }
   else
   {
      setcrc16(portnum,
         (ushort)((getStartingAddressScratchEx77(bank,SNum)/PAGE_LENGTH_SCRATCH_HYGRO) + page));
   }

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
SMALLINT readPagePacketExtraScratchEx77(SMALLINT bank, int portnum, uchar *SNum,
                                      int page, SMALLINT rd_cont, uchar *buff,
                                      int *len, uchar *extra)
{
   uchar raw_buf[PAGE_LENGTH_SCRATCH_EX];
   int i;
   ushort lastcrc16;

   // check if read exceeds memory
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   // read the scratchpad
   if(SNum[0] == 0x37)
   {
      if(!readScratchPadCRC77(portnum,SNum,raw_buf,PAGE_LENGTH_SCRATCH_EX,extra))
         return FALSE;
   }
   else
   {
      if(!readScratchPadCRC77(portnum,SNum,raw_buf,PAGE_LENGTH_SCRATCH_HYGRO,extra))
         return FALSE;
   }

   // check if length is realistic
   if(SNum[0] == 0x37)
   {
      if((raw_buf[0] > (PAGE_LENGTH_SCRATCH_EX-3)) || (raw_buf[0] <= 0))
      {
         OWERROR(OWERROR_INVALID_PACKET_LENGTH);
         return FALSE;
      }
   }
   else
   {
      if((raw_buf[0] > (PAGE_LENGTH_SCRATCH_HYGRO-3)) || (raw_buf[0] <= 0))
      {
         OWERROR(OWERROR_INVALID_PACKET_LENGTH);
         return FALSE;
      }
   }

   // verify the CRC is correct
   if(SNum[0] == 0x37)
   {
      setcrc16(portnum,
         (ushort)((getStartingAddressScratchEx77(bank,SNum)/PAGE_LENGTH_SCRATCH_EX) + page));
   }
   else
   {
      setcrc16(portnum,
         (ushort)((getStartingAddressScratchEx77(bank,SNum)/PAGE_LENGTH_SCRATCH_HYGRO) + page));
   }

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
 * buff     byte array containing data that to write.
 * len      length of the packet
 *
 * @return - returns '0' if the write page packet wasn't completed
 *                   '1' if the operation is complete.
 */
SMALLINT writePagePacketScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page,
                                    uchar *buff, int len)
{
   uchar raw_buf[64];
   int i;
   ushort crc;

   // check if read exceeds memory
   if(page != 0)
   {
      OWERROR(OWERROR_INVALID_PAGE_NUMBER);
      return FALSE;
   }

   // make sure length does not exceed max
   if(SNum[0] == 0x37)
   {
      if ((len > (PAGE_LENGTH_SCRATCH_EX - 3)) || (len <= 0))
      {
         OWERROR(OWERROR_PACKET_LENGTH_EXCEEDS_PAGE);
         return FALSE;
      }
   }
   else
   {
      if ((len > (PAGE_LENGTH_SCRATCH_HYGRO - 3)) || (len <= 0))
      {
         OWERROR(OWERROR_PACKET_LENGTH_EXCEEDS_PAGE);
         return FALSE;
      }
   }

   // see if this bank is general read/write
   if (!isGeneralPurposeMemoryScratchEx77(bank,SNum))
   {
      OWERROR(OWERROR_NOT_GENERAL_PURPOSE);
      return FALSE;
   }

   // construct the packet to write
   raw_buf[0] = (uchar) len;

   for(i=1;i<len+1;i++)
      raw_buf[i] = buff[i-1];

   if(SNum[0] == 0x37)
   {
      setcrc16(portnum,
         (ushort)((getStartingAddressScratchEx77(bank,SNum)/PAGE_LENGTH_SCRATCH_EX) + page));
   }
   else
   {
      setcrc16(portnum,
         (ushort)((getStartingAddressScratchEx77(bank,SNum)/PAGE_LENGTH_SCRATCH_HYGRO) + page));
   }
   for(i=0;i<len+1;i++)
      crc = docrc16(portnum,raw_buf[i]);

   raw_buf[len + 1] = (uchar) ~crc & 0xFF;
   raw_buf[len + 2] = (uchar) ((~crc & 0xFFFF) >> 8) & 0xFF;

   // write the packet, return result
   if(!writeScratchPadEx77(portnum,SNum,0,raw_buf,len))
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
SMALLINT getNumberPagesScratchEx77(SMALLINT bank, uchar *SNum)
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
int getSizeScratchEx77(SMALLINT bank, uchar *SNum)
{
   return SIZE_SCRATCH_EX;
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
int getStartingAddressScratchEx77(SMALLINT bank, uchar *SNum)
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
SMALLINT getPageLengthScratchEx77(SMALLINT bank, uchar *SNum)
{
   if(SNum[0] == 0x37)
      return PAGE_LENGTH_SCRATCH_EX;
   else
      return PAGE_LENGTH_SCRATCH_HYGRO;
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
SMALLINT isGeneralPurposeMemoryScratchEx77(SMALLINT bank, uchar *SNum)
{
   return generalPurposeMemorySCREx77;
}

/**
 * Query to see if current memory bank is read/write.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank is read/write
 */
SMALLINT isReadWriteScratchEx77(SMALLINT bank, int portnum, uchar *SNum)
{
   return readWriteSCREx77;
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
SMALLINT isWriteOnceScratchEx77(SMALLINT bank, int portnum, uchar *SNum)
{
   return writeOnceSCREx77;
}

/**
 * Query to see if current memory bank is read only.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can only be read
 */
SMALLINT isReadOnlyScratchEx77(SMALLINT bank, int portnum, uchar *SNum)
{
   return readOnlySCREx77;
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
SMALLINT isNonVolatileScratchEx77(SMALLINT bank, uchar *SNum)
{
   return nonVolatileSCREx77;
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
SMALLINT needsProgramPulseScratchEx77(SMALLINT bank, uchar *SNum)
{
   return needProgramPulseSCREx77;
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
SMALLINT needsPowerDeliveryScratchEx77(SMALLINT bank, uchar *SNum)
{
   return needPowerDeliverySCREx77;
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
SMALLINT hasExtraInfoScratchEx77(SMALLINT bank, uchar *SNum)
{
   return ExtraInfoSCREx77;
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
SMALLINT getExtraInfoLengthScratchEx77(SMALLINT bank, uchar *SNum)
{
   return extraInfoLengthSCREx77;
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
char *getExtraInfoDescScratchEx77(SMALLINT bank, uchar *SNum)
{
   return extraInfoDescSCREx77;
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
SMALLINT hasPageAutoCRCScratchEx77(SMALLINT bank, uchar *SNum)
{
   return pageAutoCRCScratchEx77;
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
SMALLINT getMaxPacketDataLengthScratchEx77(SMALLINT bank, uchar *SNum)
{
   if(SNum[0] == 0x37)
      return PAGE_LENGTH_SCRATCH_EX - 3;
   else
      return PAGE_LENGTH_SCRATCH_HYGRO - 3;
}

/**
 * Query to see get a string description of the current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  String containing the memory bank description
 */
char *getBankDescriptionScratchEx77(SMALLINT bank, uchar *SNum)
{
   return "Scratchpad Ex";
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
SMALLINT canRedirectPageScratchEx77(SMALLINT bank, uchar *SNum)
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
SMALLINT canLockPageScratchEx77(SMALLINT bank, uchar *SNum)
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
SMALLINT canLockRedirectPageScratchEx77(SMALLINT bank, uchar *SNum)
{
   return FALSE;
}
