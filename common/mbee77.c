//---------------------------------------------------------------------------
// Copyright (C) 2003 Dallas Semiconductor Corporation, All Rights Reserved.
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
//  mbEE77.c - Reads and writes to memory locations for the EE memory bank
//             for the DS1977.
//  version 1.00
//

// Include Files
#include "ownet.h"
#include "mbee77.h"
#include "mbscrx77.h"
#include "pw77.h"

// General command defines
#define READ_MEMORY_PSW_COMMAND  0x69
#define WRITE_SCRATCHPAD_COMMAND 0x0F
#define READ_SCRATCHPAD_COMMAND  0xAA
#define COPY_SCRATCHPAD_COMMAND  0x55
// Local defines
#define SIZE        32768
#define PAGE_LENGTH 64
#define PAGE_LENGTH_HYGRO 32

// Global variables
char     *bankDescription77      = "Main Memory";
SMALLINT  writeVerification77    = TRUE;
SMALLINT  generalPurposeMemory77 = TRUE;
SMALLINT  readWrite77            = TRUE;
SMALLINT  writeOnce77            = FALSE;
SMALLINT  readOnly77             = FALSE;
SMALLINT  nonVolatile77          = TRUE;
SMALLINT  needsProgramPulse77    = FALSE;
SMALLINT  needsPowerDelivery77   = TRUE;
SMALLINT  hasExtraInfo77         = FALSE;
SMALLINT  extraInfoLength77      = 0;
char     *extraInfoDesc77        = "";
SMALLINT  pageAutoCRC77          = FALSE;


// used to 'enable' passwords
uchar ENABLE_BYTE  = 0xAA;
// used to 'disable' passwords
uchar DISABLE_BYTE = 0x00;


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
 * psw      8 byte password
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
SMALLINT readEEPsw77(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                SMALLINT rd_cont, uchar *buff, int len)
{
   SMALLINT i, j, send_len=0, finished;
   uchar  raw_buf[15];
   uchar  temp[64];
   ushort lastcrc16 = 0;
   int pgs, extra, add;

   for(i=0;i<len;i++)
      buff[i] = 0xFF;

   // check if read exceeds memory
   if((str_add + len)>SIZE)
   {
      OWERROR(OWERROR_READ_OUT_OF_RANGE);
      return FALSE;
   }

   // set serial number of device to read
   owSerialNum(portnum,SNum,FALSE);

   // select the device
   if (!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // command, address, offset, password (except last byte)
   if(SNum[0] == 0x37)
      raw_buf[send_len++] = READ_MEMORY_PSW_COMMAND;
   else
      raw_buf[send_len++] = 0x66;

   raw_buf[send_len++] = str_add & 0xFF;
   raw_buf[send_len++] = ((str_add & 0xFFFF) >> 8) & 0xFF;

   for (i = 0; i < 8; i++)
      raw_buf[send_len++] = psw[i];


   // do the first block for command, address. password
   if(SNum[0] == 0x37)
   {
      if(!owBlock(portnum,FALSE,raw_buf,(send_len-1)))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

      if (!owWriteBytePower(portnum, raw_buf[(send_len-1)]))
      {
         OWERROR(OWERROR_WRITE_BYTE_FAILED);
         return FALSE;
      }

      msDelay(10);

      owLevel(portnum, MODE_NORMAL);
   }
   else
   {
      if(!owBlock(portnum,FALSE,raw_buf,send_len))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }
   }

   // pre-fill readBuf with 0xFF
   finished = FALSE;
   if(SNum[0] == 0x37)
   {
      pgs   = len / PAGE_LENGTH;
      extra = len % PAGE_LENGTH;
      add   = PAGE_LENGTH - (str_add % PAGE_LENGTH);
   }
   else
   {
      pgs   = len / PAGE_LENGTH_HYGRO;
      extra = len % PAGE_LENGTH_HYGRO;
      add   = PAGE_LENGTH_HYGRO - (str_add % PAGE_LENGTH_HYGRO);
   }

   if( (len < PAGE_LENGTH) && (SNum[0] == 0x37)  )
   {
      if( ((str_add % PAGE_LENGTH) > ((str_add+len) % PAGE_LENGTH)) &&
          (((str_add+len) % PAGE_LENGTH) != 0) )
      {
         for(i=0;i<add;i++)
            buff[i] = 0xFF;

         if(!owBlock(portnum,FALSE,&buff[0],add))
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }

         raw_buf[0] = owReadByte(portnum);
         raw_buf[1] = owReadBytePower(portnum);

         msDelay(10);

         owLevel(portnum, MODE_NORMAL);

         for(i=add;i<len;i++)
            buff[i] = 0xFF;

         if(!owBlock(portnum,FALSE,&buff[add],extra))
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }

         finished = TRUE;
      }
      else
      {

         for(i=0;i<len;i++)
            buff[i] = 0xFF;

         if(!owBlock(portnum,FALSE,&buff[0],len))
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }

         finished = TRUE;
      }
   }

   for (i = 0; i < pgs; i++)
   {
      if(SNum[0] != 0x37)
      {
         if(i == 0)
         {
            for(j=0;j<add;j++)
               buff[j] = 0xFF;

            if(!owBlock(portnum,FALSE,&buff[0],add))
            {
               OWERROR(OWERROR_BLOCK_FAILED);
               return FALSE;
            }

            raw_buf[0] = owReadByte(portnum);
            raw_buf[1] = owReadByte(portnum);
         }
         else
         {
            for(j=(add+(i-1)*PAGE_LENGTH_HYGRO);j<(add+i*PAGE_LENGTH_HYGRO);j++)
               buff[j] = 0xFF;

            if(!owBlock(portnum,FALSE,&buff[add+(i-1)*PAGE_LENGTH_HYGRO],
                        PAGE_LENGTH_HYGRO))
            {
               OWERROR(OWERROR_BLOCK_FAILED);
               return FALSE;
            }

            raw_buf[0] = owReadByte(portnum);
            raw_buf[1] = owReadByte(portnum);
         }
      }
      else
      {
         if( i == 0 )
         {
            for(j=0;j<add;j++)
               buff[j] = 0xFF;

            if(!owBlock(portnum,FALSE,&buff[0],add))
            {
               OWERROR(OWERROR_BLOCK_FAILED);
               return FALSE;
            }

            raw_buf[0] = owReadByte(portnum);
            raw_buf[1] = owReadBytePower(portnum);

            msDelay(10);

            owLevel(portnum, MODE_NORMAL);
         }
         else
         {
            for(j=(add+((i-1)*PAGE_LENGTH));j<(add+(i*PAGE_LENGTH));j++)
               buff[j] = 0xFF;

            if(!owBlock(portnum,FALSE,&buff[(add+((i-1)*PAGE_LENGTH))],PAGE_LENGTH))
            {
               OWERROR(OWERROR_BLOCK_FAILED);
               return FALSE;
            }

            raw_buf[0] = owReadByte(portnum);
            raw_buf[1] = owReadBytePower(portnum);

            msDelay(10);

            owLevel(portnum, MODE_NORMAL);
         }
      }
   }

   if(SNum[0] != 0x37)
   {
      for(j=0;j<PAGE_LENGTH_HYGRO;j++)
         temp[j] = 0xFF;
      for(i=(add+(pgs*PAGE_LENGTH_HYGRO));i<len;i++)
         buff[i] = 0xFF;

      // send second block to read data, return result
      if(!owBlock(portnum,FALSE,&temp[0],PAGE_LENGTH_HYGRO))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

      for(i=(add+((pgs-1)*PAGE_LENGTH_HYGRO));i<len;i++)
         buff[i] = temp[j-(add+((pgs-1)*PAGE_LENGTH_HYGRO))];

   }
   else
   {
      if(!finished)
      {
         for(j=0;j<PAGE_LENGTH;j++)
            temp[j] = 0xFF;

         if(!owBlock(portnum,FALSE,&temp[0],PAGE_LENGTH))
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }

         for(j=(add+((pgs-1)*PAGE_LENGTH));j<len;j++)
            buff[j] = temp[j-(add+((pgs-1)*PAGE_LENGTH))];
      }
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
 * psw      8 byte password
 * str_add  starting address
 * buff     byte array containing data to write
 * len      length in bytes to write
 *
 * !!!!!!!!!!!!!MUST FIT ON PAGE RIGHT NOW!!!!!!!!!!!!!
 *
 * @return 'true' if the write was complete.
 */
SMALLINT writeEE77(SMALLINT bank, int portnum, uchar *SNum,
                   int str_add, uchar *buff, int len)
{
   uchar raw_buf[64],extra[5];
   SMALLINT i;
   int startx, nextx, abs_addr, pl, room_left;
   int endingOffset, numBytes;
   uchar wr_buff[256];

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

   for(i=0;i<len;i++)
      wr_buff[i] = buff[i];

   if(SNum[0] != 0x37)
   {
      endingOffset = str_add + len;
      numBytes     = getPageLengthEE77(bank,SNum) - (endingOffset & 0x1F);

      if(!readEEPsw77(bank,portnum,SNum,endingOffset,FALSE,&wr_buff[len],numBytes))
         return FALSE;
   }

   // loop while still have pages to write
   startx = 0;
   nextx  = 0;
   abs_addr  = getStartingAddressEE77(bank,SNum) + str_add;
   pl        = getPageLengthEE77(bank,SNum);

   do
   {
      // calculate room left in current page
      room_left = pl - ((abs_addr + startx) % pl);

      // check if block left will cross end of page
      if ((len - startx) > room_left)
         nextx = startx + room_left;
      else
         nextx = len;

      // write the page of data to scratchpad
      writeScratchPadEx77(portnum,SNum,(abs_addr + startx),&wr_buff[startx],nextx - startx);

      // read to verify ok
      if(!readScratchPadCRC77(portnum,SNum,&raw_buf[0],pl,&extra[0]))
         return FALSE;

      // check to see if the same
      for (i = 0; i < (nextx - startx); i++)
      {
         if (raw_buf[i] != wr_buff[i + startx])
         {
            OWERROR(OWERROR_READ_SCRATCHPAD_VERIFY);
            return FALSE;
         }
      }

      // check to make sure that the address is correct
      if ((((extra[0] & 0x00FF) | ((extra[1] << 8) & 0x00FF00))
              & 0x00FFFF) != (abs_addr + startx))
      {
         OWERROR(OWERROR_READ_SCRATCHPAD_VERIFY);
         return FALSE;
      }

      // do the copy
      copyScratchPadPsw77(portnum,SNum,(abs_addr + startx),(nextx - startx),psw);

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
 * psw      8 byte password
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
SMALLINT readPageEE77(SMALLINT bank, int portnum, uchar *SNum, int page,
                    SMALLINT rd_cont, uchar *buff)
{
   return readPageCRCEE77(bank,portnum,SNum,page,buff);
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
 * psw      8 byte password
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
SMALLINT readPageExtraEE77(SMALLINT bank, int portnum, uchar *SNum, int page,
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
 * psw      8 byte password
 * page      the page to read
 * read_buff byte array containing data that was read.
 * extra     the extra information
 *
 * @return - returns '0' if the read page wasn't completed with extra info.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageExtraCRCEE77(SMALLINT bank, int portnum, uchar *SNum, int page,
                            uchar *read_buff, uchar *extra)
{
   OWERROR(OWERROR_CRC_NOT_SUPPORTED);
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
 * psw      8 byte password
 * page     the page to read
 * rd_cont  if 'true' then device read is continued without
 *          re-selecting.  This can only be used if the new
 *          read() continious where the last one led off
 *          and it is inside a 'beginExclusive/endExclusive'
 *          block.
 * buff     byte array containing data that was read
 *
 * @return - returns '0' if the read page wasn't completed.
 *                   '1' if the operation is complete.
 */
SMALLINT readPageCRCEE77(SMALLINT bank, int portnum, uchar *SNum,
                         int page, uchar *buff)
{
   SMALLINT i, send_len=0, lsCRC16;
   uchar  raw_buf[15];
   int str_add;
   ushort lastcrc16;

   // check if not continuing on the next page
/*   if (!rd_cont)
   {*/
      // set serial number of device to read
      owSerialNum(portnum,SNum,FALSE);

      // select the device
      if (!owAccess(portnum))
      {
         OWERROR(OWERROR_DEVICE_SELECT_FAIL);
         return FALSE;
      }

      // command, address, offset, password (except last byte)
      raw_buf[send_len++] = READ_MEMORY_PSW_COMMAND;
      if(SNum[0] == 0x37)
      {
         str_add = page * PAGE_LENGTH;
      }
      else
      {
         str_add = page * PAGE_LENGTH_HYGRO;
      }
      raw_buf[send_len++] = str_add & 0xFF;
      raw_buf[send_len++] = ((str_add & 0xFFFF) >> 8) & 0xFF;

      // calculate the CRC16
      setcrc16(portnum,0);
      for(i = 0; i < send_len; i++)
         lastcrc16 = docrc16(portnum,raw_buf[i]);

      for (i = 0; i < 8; i++)
         raw_buf[send_len++] = psw[i];


      if(SNum[0] == 0x37)
      {
         // send block (check copy indication complete)
         if(!owBlock(portnum,FALSE,raw_buf,(send_len-1)))
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
      }
      else
      {
         if(!owBlock(portnum,FALSE,raw_buf,send_len))
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }
      }
/*   }
   // get the next page
   else
   {
      // previous read left last CRC16 byte to read before next page
      owReadBytePower(portnum);

      // calculate the CRC16
      setcrc16(portnum,0);
   }*/

   // set the read bytes
   if(SNum[0] == 0x37)
   {
      // delay for read to complete
      msDelay(5);  //????????5 could be 2

      // turn off strong pullup
      owLevel(portnum, MODE_NORMAL);


      for (i = 0; i < PAGE_LENGTH; i++)
         buff[i] = 0xFF;
   }
   else
   {
      for(i=0; i<PAGE_LENGTH_HYGRO; i++)
         buff[i] = 0xFF;
   }

   // read the data
   if(SNum[0] == 0x37)
   {
      if(!owBlock(portnum,FALSE,buff,PAGE_LENGTH))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }
   }
   else
   {
      if(!owBlock(portnum,FALSE,buff,PAGE_LENGTH_HYGRO))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }
   }
    // read the first CRC16 byte
   lsCRC16 = owReadByte(portnum);

   // calculate CRC on data read
   if(SNum[0] == 0x37)
   {
      for(i = 0; i < PAGE_LENGTH; i++)
         lastcrc16 = docrc16(portnum,buff[i]);
   }
   else
   {
      for(i = 0; i < PAGE_LENGTH_HYGRO; i++)
         lastcrc16 = docrc16(portnum,buff[i]);
   }

   // check lsByte of the CRC
   if ((lastcrc16 & 0xFF) != (~lsCRC16 & 0xFF))
   {
      OWERROR(OWERROR_CRC_FAILED);
      return FALSE;
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
 * psw      8 byte password
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
SMALLINT readPagePacketEE77(SMALLINT bank, int portnum, uchar *SNum, int page,
                            SMALLINT rd_cont, uchar *buff, int *len)
{
   uchar raw_buf[PAGE_LENGTH];
   ushort lastcrc16;
   int i;

   // read the page
   if(!readPageCRCEE77(bank,portnum,SNum,page,raw_buf))
      return FALSE - 1;

   // check if length is realistic
   if(SNum[0] == 0x37)
   {
      if((raw_buf[0] > (PAGE_LENGTH - 3)) || (raw_buf[0] <= 0))
      {
         OWERROR(OWERROR_INVALID_PACKET_LENGTH);
         return FALSE;
      }
   }
   else
   {
      if((raw_buf[0] > (PAGE_LENGTH_HYGRO - 3)) || (raw_buf[0] <= 0))
      {
         OWERROR(OWERROR_INVALID_PACKET_LENGTH);
         return FALSE;
      }
   }

   // verify the CRC is correct
   setcrc16(portnum,(ushort)page);
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
 * psw      8 byte password
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
SMALLINT readPagePacketExtraEE77(SMALLINT bank, int portnum, uchar *SNum,
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
 * psw      8 byte password
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
SMALLINT writePagePacketEE77(SMALLINT bank, int portnum, uchar *SNum, int page,
                           uchar *buff, int len)
{
   uchar raw_buf[PAGE_LENGTH];
   int i;
   ushort crc;

   // make sure length does not exceed max
   if(SNum[0] == 0x37)
   {
      if ((len > (PAGE_LENGTH - 3)) || (len <= 0))
      {
         OWERROR(OWERROR_PACKET_LENGTH_EXCEEDS_PAGE);
         return FALSE;
      }
   }
   else
   {
      if ((len > (PAGE_LENGTH_HYGRO - 3)) || (len <= 0))
      {
         OWERROR(OWERROR_PACKET_LENGTH_EXCEEDS_PAGE);
         return FALSE;
      }
   }

   // construct the packet to write
   raw_buf[0] = (uchar) len;

   for(i=1;i<len+1;i++)
      raw_buf[i] = buff[i-1];

   if(SNum[0] == 0x37)
      setcrc16(portnum,(ushort) ((getStartingAddressEE77(bank,SNum)/PAGE_LENGTH) + page));
   else
      setcrc16(portnum,(ushort) ((getStartingAddressEE77(bank,SNum)/PAGE_LENGTH_HYGRO) + page));

   for(i=0;i<len+1;i++)
      crc = docrc16(portnum,raw_buf[i]);

   raw_buf[len + 1] = (uchar) ~crc & 0xFF;
   raw_buf[len + 2] = (uchar) ((~crc & 0xFFFF) >> 8) & 0xFF;

   // write the packet, return result
   if(SNum[0] == 0x37)
   {
      if(!writeEE77(bank,portnum,SNum, page * PAGE_LENGTH,raw_buf,len + 3))
         return FALSE;
   }
   else
   {
      if(!writeEE77(bank,portnum,SNum, page * PAGE_LENGTH_HYGRO,raw_buf,len + 3))
         return FALSE;
   }

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
SMALLINT getNumberPagesEE77(SMALLINT bank, uchar *SNum)
{
   return 511;
}

/**
 * Query to get the memory bank size in bytes.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  memory bank size in bytes.
 */
int getSizeEE77(SMALLINT bank, uchar *SNum)
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
int getStartingAddressEE77(SMALLINT bank, uchar *SNum)
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
SMALLINT getPageLengthEE77(SMALLINT bank, uchar *SNum)
{
   if(SNum[0] == 0x37)
      return PAGE_LENGTH;
   else
      return PAGE_LENGTH_HYGRO;
}

/**
 * Query to see get a string description of the current memory bank.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  String containing the memory bank description
 */
char *getBankDescriptionEE77(SMALLINT bank, uchar *SNum)
{
   return bankDescription77;
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
SMALLINT isGeneralPurposeMemoryEE77(SMALLINT bank, uchar *SNum)
{
   return generalPurposeMemory77;
}

/**
 * Query to see if current memory bank is read/write.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank is read/write
 */
SMALLINT isReadWriteEE77(SMALLINT bank, int portnum, uchar *SNum)
{
   return readWrite77;
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
SMALLINT isWriteOnceEE77(SMALLINT bank, int portnum, uchar *SNum)
{
   return writeOnce77;
}

/**
 * Query to see if current memory bank is read only.
 *
 * bank     to tell what memory bank of the ibutton to use.
 * SNum     the serial number for the part.
 *
 * @return  'true' if current memory bank can only be read
 */
SMALLINT isReadOnlyEE77(SMALLINT bank, int portnum, uchar *SNum)
{
   return readOnly77;
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
SMALLINT isNonVolatileEE77(SMALLINT bank, uchar *SNum)
{
   return nonVolatile77;
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
SMALLINT needsProgramPulseEE77(SMALLINT bank, uchar *SNum)
{
   return needsProgramPulse77;
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
SMALLINT needsPowerDeliveryEE77(SMALLINT bank, uchar *SNum)
{
   return needsPowerDelivery77;
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
SMALLINT hasExtraInfoEE77(SMALLINT bank, uchar *SNum)
{
   return hasExtraInfo77;
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
SMALLINT getExtraInfoLengthEE77(SMALLINT bank, uchar *SNum)
{
   return extraInfoLength77;
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
char *getExtraInfoDescEE77(SMALLINT bank, uchar *SNum)
{
   return extraInfoDesc77;
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
SMALLINT hasPageAutoCRCEE77(SMALLINT bank, uchar *SNum)
{
   return pageAutoCRC77;
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
SMALLINT getMaxPacketDataLengthEE77(SMALLINT bank, uchar *SNum)
{
   if(SNum[0] == 0x37)
      return PAGE_LENGTH - 3;
   else
      return PAGE_LENGTH_HYGRO - 3;
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
SMALLINT canRedirectPageEE77(SMALLINT bank, uchar *SNum)
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
SMALLINT canLockPageEE77(SMALLINT bank, uchar *SNum)
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
SMALLINT canLockRedirectPageEE77(SMALLINT bank, uchar *SNum)
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
SMALLINT needPassword(int portnum, uchar *SNum)
{
   uchar passone[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
   uchar passtwo[8] = {0x01, 0x02, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08};

   if(verifyPassword(portnum,SNum,&passone[0],READ_WRITE_PSW) &&
      verifyPassword(portnum,SNum,&passtwo[0],READ_WRITE_PSW))
   {
      return FALSE;
   }

   return TRUE;
}
