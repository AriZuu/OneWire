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
//  read_write.c - Reads and writes to memory locations of different buttons.
//  version 1.00
//

// Include Files
#include <stdio.h>
#include "rawmem.h"
#include "mbnv.h"
#include "mbappreg.h"
#include "mbeprom.h"
#include "mbnvcrc.h"
#include "mbshaee.h"
#include "mbscrcrc.h"
#include "mbscrex.h"
#include "mbsha.h"
#include "mbscree.h"
#include "mbscr.h"
#include "mbee77.h"
#include "mbscrx77.h"
#include "mbee.h"
#include "pw77.h"

/**
 * Reads memory in this bank with no CRC checking (device or
 * data). The resulting data from this API may or may not be what is on
 * the 1-Wire device.  It is recommended that the data contain some kind
 * of checking (CRC) like in the owReadPagePacket method.
 * Some 1-Wire devices provide thier own CRC as in owReadPageCRC
 * The owReadPageCRC method is not supported on all memory types, see hasPageAutoCRC
 * in the same interface.  If neither is an option then this method could be called more
 * then once to at least verify that the same data is read consistently.  The
 * readContinue parameter is used to eliminate the overhead in re-accessing
 * a part already being read from. For example, if pages 0 - 4 are to
 * be read, readContinue would be set to false for page 0 and would be set
 * to true for the next four calls.
 *
 * Note: Using readContinue = true  can only be used if the new
 *       read continuous where the last one led off
 *       and it is inside a 'beginExclusive/endExclusive'
 *       block.
 *
 * bank          to tell what memory bank of the ibutton to use.
 * portnum       the port number of the port being used for the
 *               1-Wire Network.
 * SNum          the serial number for the part that the operation is
 *               to be done on.
 * str_add       starting address
 * rd_cont       true then device read is
 *               continued without re-selecting
 * buff          location for data read
 * len           length in bytes to read
 *
 * @return 'true' if the read was complete
 */
SMALLINT owRead(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                SMALLINT rd_cont, uchar *buff, int len)
{
   SMALLINT ret = 0;
   uchar extra[5];

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = readEE(bank,portnum,SNum,str_add,rd_cont,buff,len);
         else if(bank == 0)
            ret = readAppReg(bank,portnum,SNum,str_add,rd_cont,buff,len);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23: //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = readNV(bank,portnum,SNum,str_add,rd_cont,buff,len);
         else if(bank == 0)
            ret = readScratch(bank,portnum,SNum,str_add,rd_cont,buff,len);
         break;

      case (0x18):  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = readNV(bank,portnum,SNum,str_add,rd_cont,buff,len);
         else if(bank == 0)
            ret = readScratch(bank,portnum,SNum,str_add,rd_cont,buff,len);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = readNV(bank,portnum,SNum,str_add,rd_cont,buff,len);
         else if(bank == 0)
            ret = readScratch(bank,portnum,SNum,str_add,rd_cont,buff,len);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = readNV(bank,portnum,SNum,str_add,rd_cont,buff,len);
         else if(bank == 0)
            ret = readScratch(bank,portnum,SNum,str_add,rd_cont,buff,len);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = readSHAEE(bank,portnum,SNum,str_add,rd_cont,buff,len);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = readEPROM(bank,portnum,SNum,str_add,rd_cont,buff,len);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
         {
            ret = readEEPsw77(bank,portnum,SNum,str_add,rd_cont,buff,len);
         }
         else if(bank == 0)
         {
            ret = readScratchPadCRC77(portnum,SNum,buff,len,&extra[0]);
         }
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Writes memory in this bank. It is recommended that a structure with some
 * built in error checking is used to provide data integrity on read.
 * The method owWritePagePacket, which automatically wraps the data in a length
 * and CRC, could be used for this purpose.
 *
 * When using on Write-Once devices care must be taken to write into
 * into empty space.  If owWrite is used to write over an unlocked
 * page on a Write-Once device it will fail.
 *
 * bank        to tell what memory bank of the ibutton to use.
 * portnum     the port number of the port being used for the
 *             1-Wire Network.
 * SNum        the serial number for the part that the operation is
 *             to be done on.
 * str_add     starting address
 * buff        data to write
 * len         length in bytes to write
 *
 * @return 'true' if the write was complete
 */
SMALLINT owWrite(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                 uchar *buff, int len)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = writeEE(bank,portnum,SNum,str_add,buff,len);
         else if(bank == 0)
            ret = writeAppReg(bank,portnum,SNum,str_add,buff,len);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = writeNV(bank,portnum,SNum,str_add,buff,len);
         else if(bank == 0)
            ret = writeScratch(bank,portnum,SNum,str_add,buff,len);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = writeNV(bank,portnum,SNum,str_add,buff,len);
         else if(bank == 0)
            ret = writeScratch(bank,portnum,SNum,str_add,buff,len);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = writeNV(bank,portnum,SNum,str_add,buff,len);
         else if(bank == 0)
            ret = writeScratch(bank,portnum,SNum,str_add,buff,len);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = writeNV(bank,portnum,SNum,str_add,buff,len);
         else if(bank == 0)
            ret = writeScratch(bank,portnum,SNum,str_add,buff,len);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = writeSHAEE(bank,portnum,SNum,str_add,buff,len);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = writeEPROM(bank,portnum,SNum,str_add,buff,len);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
         {
            ret = writeEE77(bank,portnum,SNum,str_add,buff,len);
         }
         else if(bank == 0)
         {
            ret = writeScratchPadEx77(portnum,SNum,str_add,buff,len);
         }
         break;


      default:
         break;
   }

   return ret;
}

/**
 * Reads a page in this memory bank with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * owReadPagePacket function or have the 1-Wire device provide the
 * CRC as in owReadPageCRC.
 * However device CRC generation is not
 * supported on all memory types, see owHasPageAutoCRC}.
 * If neither is an option then this method could be called more
 * then once to at least verify that the same data is read consistently.
 *
 * The readContinue parameter is used to eliminate the overhead in re-accessing
 * a part already being read from. For example, if pages 0 - 4 are to
 * be read, readContinue would be set to false for page 0 and would be set
 * to true for the next four calls.
 * Note: Using readContinue = true  can only be used if the new
 *       read continues where the last one left off
 *       and it is inside a 'beginExclusive/endExclusive'
 *       block.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * page       page number to read packet from
 * rd_cont    true, then device read
 *            is continued without re-selecting
 * buff       location for data read
 *
 * @return 'true' if the read was complete
 */
SMALLINT owReadPage(SMALLINT bank, int portnum, uchar *SNum, int page,
                    SMALLINT rd_cont, uchar *buff)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = readPageEE(bank,portnum,SNum,page,rd_cont,buff);
         else if(bank == 0)
            ret = readPageAppReg(bank,portnum,SNum,page,rd_cont,buff);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = readPageNV(bank,portnum,SNum,page,rd_cont,buff);
         else if(bank == 0)
            ret = readPageScratch(bank,portnum,SNum,page,rd_cont,buff);
         break;

      case (0x18):  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank == 3)
            ret = readPageNV(bank,portnum,SNum,page,rd_cont,buff);
         else if(bank > 0)
            ret = readPageNVCRC(bank,portnum,SNum,page,rd_cont,buff);
         else if(bank == 0)
            ret = readPageScratch(bank,portnum,SNum,page,rd_cont,buff);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = readPageNVCRC(bank,portnum,SNum,page,rd_cont,buff);
         else if(bank == 0)
            ret = readPageScratch(bank,portnum,SNum,page,rd_cont,buff);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = readPageNVCRC(bank,portnum,SNum,page,rd_cont,buff);
         else if(bank == 0)
            ret = readPageScratch(bank,portnum,SNum,page,rd_cont,buff);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = readPageSHAEE(bank,portnum,SNum,page,rd_cont,buff);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = readPageEPROM(bank,portnum,SNum,page,rd_cont,buff);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
         {
            ret = readPageEE77(bank,portnum,SNum,page,rd_cont,buff);
         }
         else if(bank == 0)
         {
            ret = readPageScratchEx77(bank,portnum,SNum,page,rd_cont,buff);
         }
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Reads a page in this memory bank with extra information with no
 * CRC checking (device or data). The resulting data from this API
 * may or may not be what is on the 1-Wire device.  It is recommends
 * that the data contain some kind of checking (CRC) like in the
 * owReadPagePacket function or have the 1-Wire device provide the
 * CRC as in owReadPageCRC}.
 * However device CRC generation is not
 * supported on all memory types, see owHasPageAutoCRC}.
 * If neither is an option then this method could be called more
 * then once to at least verify that the same data is read consistently.The
 * readContinue parameter is used to eliminate the overhead in re-accessing
 * a part already being read from. For example, if pages 0 - 4 are to
 * be read, readContinue would be set to false for page 0 and would be set
 * to true for the next four calls.
 *
 * Note: Using readContinue = true  can only be used if the new
 *       read continues where the last one left off
 *       and it is inside a 'beginExclusive/endExclusive'
 *       block.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * page       page number to read packet from
 * rd_cont    true  then device read
 *            is continued without re-selecting
 * buff       location for data read
 * extra      location for extra info read
 *
 * @return 'true' if the read was complete
 */
SMALLINT owReadPageExtra(SMALLINT bank, int portnum, uchar *SNum, int page,
                         SMALLINT rd_cont, uchar *buff, uchar *extra)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = readPageExtraEE(bank,portnum,SNum,page,rd_cont,buff,extra);
         else if(bank == 0)
            ret = readPageExtraAppReg(bank,portnum,SNum,page,rd_cont,buff,extra);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = readPageExtraNV(bank,portnum,SNum,page,rd_cont,buff,extra);
         else if(bank == 0)
            ret = readPageExtraScratch(bank,portnum,SNum,page,rd_cont,buff,extra);
         break;

      case (0x18):  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank == 3)
            ret = readPageExtraNV(bank,portnum,SNum,page,rd_cont,buff,extra);
         else if(bank > 0)
            ret = readPageExtraNVCRC(bank,portnum,SNum,page,rd_cont,buff,extra);
         else if(bank == 0)
            ret = readPageExtraScratch(bank,portnum,SNum,page,rd_cont,buff,extra);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = readPageExtraNVCRC(bank,portnum,SNum,page,rd_cont,buff,extra);
         else if(bank == 0)
            ret = readPageExtraScratch(bank,portnum,SNum,page,rd_cont,buff,extra);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = readPageExtraNVCRC(bank,portnum,SNum,page,rd_cont,buff,extra);
         else if(bank == 0)
            ret = readPageExtraScratch(bank,portnum,SNum,page,rd_cont,buff,extra);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = readPageExtraSHAEE(bank,portnum,SNum,page,rd_cont,buff,extra);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = readPageExtraEPROM(bank,portnum,SNum,page,rd_cont,buff,extra);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
         {
            ret = readPageExtraEE77(bank,portnum,SNum,page,rd_cont,buff,extra);
         }
         else if(bank == 0)
         {
            ret = readPageExtraScratchEx77(bank,portnum,SNum,page,rd_cont,
                                           buff,extra);
         }
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Reads a complete memory page with CRC verification provided by the
 * device with extra information.  Not supported by all devices. The
 * rd_Continue parameter is used to eliminate the overhead in re-accessing
 * a part already being read from. For example, if pages 0 - 4 are to
 * be read, readContinue would be set to false for page 0 and would be set
 * to true for the next four calls.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * page       page number to read
 * read_buff  location for data read
 * extra      location for extra info read
 *
 * @return 'true' if the read was complete
 */
SMALLINT owReadPageExtraCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                            uchar *read_buff, uchar *extra)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = readPageExtraCRCEE(bank,portnum,SNum,page,read_buff,extra);
         else if(bank == 0)
            ret = readPageExtraCRCAppReg(bank,portnum,SNum,page,read_buff,extra);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = readPageExtraCRCNV(bank,portnum,SNum,page,read_buff,extra);
         else if(bank == 0)
            ret = readPageExtraCRCScratch(bank,portnum,SNum,page,read_buff,extra);
         break;

      case (0x18):  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank == 3)
            ret = readPageExtraCRCNV(bank,portnum,SNum,page,read_buff,extra);
         else if(bank > 0)
            ret = readPageExtraCRCNVCRC(bank,portnum,SNum,page,read_buff,extra);
         else if(bank == 0)
            ret = readPageExtraCRCScratch(bank,portnum,SNum,page,read_buff,extra);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = readPageExtraCRCNVCRC(bank,portnum,SNum,page,read_buff,extra);
         else if(bank == 0)
            ret = readPageExtraCRCScratch(bank,portnum,SNum,page,read_buff,extra);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = readPageExtraCRCNVCRC(bank,portnum,SNum,page,read_buff,extra);
         else if(bank == 0)
            ret = readPageExtraCRCScratch(bank,portnum,SNum,page,read_buff,extra);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = readPageExtraCRCSHAEE(bank,portnum,SNum,page,read_buff,extra);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = readPageExtraCRCEPROM(bank,portnum,SNum,page,read_buff,extra);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
         {
            ret = readPageExtraCRCEE77(bank,portnum,SNum,page,read_buff,extra);
         }
         else if(bank == 0)
         {
            ret = readPageExtraCRCScratchEx77(bank,portnum,SNum,page,read_buff,extra);
         }
         break;

      default:
         break;
   }

   return ret;
}


/**
 * Reads a complete memory page with CRC verification provided by the
 * device.  Not supported by all devices.  The
 * readContinue parameter is used to eliminate the overhead in re-accessing
 * a part already being read from. For example, if pages 0 - 4 are to
 * be read, readContinue would be set to false for page 0 and would be set
 * to true for the next four calls.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * page       page number to read
 * buff       location for data read
 *
 * @return 'true' if the read was complete
 */
SMALLINT owReadPageCRC(SMALLINT bank, int portnum, uchar *SNum, int page, uchar *buff)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = readPageCRCEE(bank,portnum,SNum,page,buff);
         else if(bank == 0)
            ret = readPageCRCAppReg(bank,portnum,SNum,page,buff);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = readPageCRCNV(bank,portnum,SNum,page,buff);
         else if(bank == 0)
            ret = readPageCRCScratch(bank,portnum,SNum,page,buff);
         break;

      case (0x18):  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank == 3)
            ret = readPageCRCNV(bank,portnum,SNum,page,buff);
         else if(bank > 0)
            ret = readPageCRCNVCRC(bank,portnum,SNum,page,buff);
         else if(bank == 0)
            ret = readPageCRCScratch(bank,portnum,SNum,page,buff);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = readPageCRCNVCRC(bank,portnum,SNum,page,buff);
         else if(bank == 0)
            ret = readPageCRCScratch(bank,portnum,SNum,page,buff);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = readPageCRCNVCRC(bank,portnum,SNum,page,buff);
         else if(bank == 0)
            ret = readPageCRCScratch(bank,portnum,SNum,page,buff);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = readPageCRCSHAEE(bank,portnum,SNum,page,buff);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = readPageCRCEPROM(bank,portnum,SNum,page,buff);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
         {
            ret = readPageCRCEE77(bank,portnum,SNum,page,buff);
         }
         else
         {
            ret = readPageCRCScratchEx77(bank,portnum,SNum,page,buff);
         }
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Reads a Universal Data Packet.
 *
 * The Universal Data Packet always starts on page boundaries but
 * can end anywhere in the page.  The structure specifies the length of
 * data bytes not including the length byte and the CRC16 bytes.
 * There is one length byte. The CRC16 is first initialized to
 * the page number.  This provides a check to verify the page that
 * was intended is being read.  The CRC16 is then calculated over
 * the length and data bytes.  The CRC16 is then inverted and stored
 * low byte first followed by the high byte.  The structure is
 * used by this method to verify the data but only
 * the data payload is returned. The
 * readContinue parameter is used to eliminate the overhead in re-accessing
 * a part already being read from. For example, if pages 0 - 4 are to
 * be read, readContinue would be set to false for page 0 and would be set
 * to true for the next four calls.
 *
 * See Dallas Semiconductor Application Note 114
 * for details: http://www.dalsemi.com/datasheets/pdfs/app114.pdf
 * http://www.dalsemi.com/datasheets/pdfs/app114.pdf
 *
 * <P> Note: Using rd_cont = true  can only be used if the new
 *           read continues where the last one left off
 *           and it is inside a 'beginExclusive/endExclusive'
 *           block.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * page       page number to read packet from
 * rd_cont    true then device read
 *            is continued without re-selecting
 * buff       location for data read
 * len        the length of the data
 *
 * @return 'true' if the read was complete
 */
SMALLINT owReadPagePacket(SMALLINT bank, int portnum, uchar *SNum, int page,
                          SMALLINT rd_cont, uchar *buff, int *len)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = readPagePacketEE(bank,portnum,SNum,page,rd_cont,buff,len);
         else if(bank == 0)
            ret = readPagePacketAppReg(bank,portnum,SNum,page,rd_cont,buff,len);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = readPagePacketNV(bank,portnum,SNum,page,rd_cont,buff,len);
         else if(bank == 0)
            ret = readPagePacketScratch(bank,portnum,SNum,page,rd_cont,buff,len);
         break;

      case (0x18):  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = readPagePacketNV(bank,portnum,SNum,page,rd_cont,buff,len);
         else if(bank == 0)
            ret = readPagePacketScratch(bank,portnum,SNum,page,rd_cont,buff,len);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = readPagePacketNV(bank,portnum,SNum,page,rd_cont,buff,len);
         else if(bank == 0)
            ret = readPagePacketScratch(bank,portnum,SNum,page,rd_cont,buff,len);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = readPagePacketNV(bank,portnum,SNum,page,rd_cont,buff,len);
         else if(bank == 0)
            ret = readPagePacketScratch(bank,portnum,SNum,page,rd_cont,buff,len);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = readPagePacketSHAEE(bank,portnum,SNum,page,rd_cont,buff,len);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = readPagePacketEPROM(bank,portnum,SNum,page,rd_cont,buff,len);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
         {
            ret = readPagePacketEE77(bank,portnum,SNum,page,rd_cont,buff,len);
         }
         else if(bank == 0)
         {
            ret = readPagePacketScratchEx77(bank,portnum,SNum,page,rd_cont,buff,len);
         }
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Reads a Universal Data Packet and extra information.  See the
 * function owReadPagePacket}
 * for a description of the packet structure.  The
 * rd_cont parameter is used to eliminate the overhead in re-accessing
 * a part already being read from. For example, if pages 0 - 4 are to
 * be read, readContinue would be set to false for page 0 and would be set
 * to true for the next four calls.
 *
 * Note: Using readContinue = true  can only be used if the new
 *       read continues where the last one left off
 *       and it is inside a 'beginExclusive/endExclusive'
 *       block.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * page       page number to read packet from
 * rd_cont    true then device read
 *            is continued without re-selecting
 * buff       location for data read
 * len        the length of the data
 * extra      location for extra info read
 *
 * @return 'true' if the read was complete
 */
SMALLINT owReadPagePacketExtra(SMALLINT bank, int portnum, uchar *SNum, int page,
                               SMALLINT rd_cont, uchar *buff, int *len, uchar *extra)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = readPagePacketExtraEE(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         else if(bank == 0)
            ret = readPagePacketExtraAppReg(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = readPagePacketExtraNV(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         else if(bank == 0)
            ret = readPagePacketExtraScratch(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         break;

      case (0x18):  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank == 3)
            ret = readPagePacketExtraNV(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         else if(bank > 0)
            ret = readPagePacketExtraNVCRC(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         else if(bank == 0)
            ret = readPagePacketExtraScratch(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = readPagePacketExtraNVCRC(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         else if(bank == 0)
            ret = readPagePacketExtraScratch(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = readPagePacketExtraNVCRC(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         else if(bank == 0)
            ret = readPagePacketExtraScratch(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = readPagePacketExtraSHAEE(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = readPagePacketExtraEPROM(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
         {
            ret = readPagePacketExtraEE77(bank,portnum,SNum,page,rd_cont,buff,len,extra);
         }
         else if(bank == 0)
         {
            ret = readPagePacketExtraScratchEx77(bank,portnum,SNum,page,rd_cont,
                                                          buff,len,extra);
         }
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Writes a Universal Data Packet.  See the funtion owReadPagePacket
 * for a description of the packet structure.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * page       page number to write packet to
 * buff       data to write
 * len        number of bytes to write with a max of owGetMaxPacketDataLength
 *            elements
 *
 * @return 'true' if the write was complete
 */
SMALLINT owWritePagePacket(SMALLINT bank, int portnum, uchar *SNum, int page,
                           uchar *buff, int len)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = writePagePacketEE(bank,portnum,SNum,page,buff,len);
         else if(bank == 0)
            ret = writePagePacketAppReg(bank,portnum,SNum,page,buff,len);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = writePagePacketNV(bank,portnum,SNum,page,buff,len);
         else if(bank == 0)
            ret = writePagePacketScratch(bank,portnum,SNum,page,buff,len);
         break;

      case (0x18):  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = writePagePacketNV(bank,portnum,SNum,page,buff,len);
         else if(bank == 0)
            ret = writePagePacketScratch(bank,portnum,SNum,page,buff,len);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = writePagePacketNV(bank,portnum,SNum,page,buff,len);
         else if(bank == 0)
            ret = writePagePacketScratch(bank,portnum,SNum,page,buff,len);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = writePagePacketNV(bank,portnum,SNum,page,buff,len);
         else if(bank == 0)
            ret = writePagePacketScratch(bank,portnum,SNum,page,buff,len);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = writePagePacketSHAEE(bank,portnum,SNum,page,buff,len);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = writePagePacketEPROM(bank,portnum,SNum,page,buff,len);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
         {
            ret = writePagePacketEE77(bank,portnum,SNum,page,buff,len);
         }
         else if(bank == 0)
         {
            ret = writePagePacketScratchEx77(bank,portnum,SNum,page,buff,len);
         }
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Gets the number of banks for a certain button given it's family code.
 *
 * family     the family code
 *
 * @return the number of banks
 */
SMALLINT owGetNumberBanks(uchar family)
{
   SMALLINT ret = 0;

   switch(family & 0x7F)
   {
      case 0x14: case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x09: case 0x12: case 0x23:
         ret = 2;
         break;

      case 0x1A:
         ret = 3;
         break;

      case 0x33: case 0xB3: case 0x18: case 0x1D:
         ret = 4;
         break;

      case 0x0B: case 0x0F: case 0x13:
         ret = 5;
         break;

      case 0x21:
         ret = 6;
         break;

      case 0x37: case 0x77:
         ret = 2;
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Gets the number of pages in this memory bank.
 * The page numbers are then always 0 to (owGetNumberPages() - 1).
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  number of pages in this memory bank
 */
SMALLINT owGetNumberPages(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = getNumberPagesEE(bank,SNum);
         else if(bank == 0)
            ret = getNumberPagesAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = getNumberPagesNV(bank,SNum);
         else if(bank == 0)
            ret = getNumberPagesScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = getNumberPagesNV(bank,SNum);
         else if(bank == 0)
            ret = getNumberPagesScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = getNumberPagesNV(bank,SNum);
         else if(bank == 0)
            ret = getNumberPagesScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = getNumberPagesNV(bank,SNum);
         else if(bank == 0)
            ret = getNumberPagesScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = getNumberPagesSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = getNumberPagesEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
         {
            ret = getNumberPagesEE77(bank,SNum);
         }
         else if(bank == 0)
         {
            ret = getNumberPagesScratchEx77(bank,SNum);
         }
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Gets the size of this memory bank in bytes.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  number of bytes in current memory bank
 */
int owGetSize(SMALLINT bank, uchar *SNum)
{
   int ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = getSizeEE(bank,SNum);
         else if(bank == 0)
            ret = getSizeAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = getSizeNV(bank,SNum);
         else if(bank == 0)
            ret = getSizeScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = getSizeNV(bank,SNum);
         else if(bank == 0)
            ret = getSizeScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = getSizeNV(bank,SNum);
         else if(bank == 0)
            ret = getSizeScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = getSizeNV(bank,SNum);
         else if(bank == 0)
            ret = getSizeScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = getSizeSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = getSizeEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = getSizeEE77(bank,SNum);
         else if(bank == 0)
            ret = getSizeScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Gets raw page length in bytes in this memory bank.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return   page length in bytes in this memory bank
 */
SMALLINT owGetPageLength(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  // EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = getPageLengthEE(bank,SNum);
         else if(bank == 0)
            ret = getPageLengthAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = getPageLengthNV(bank,SNum);
         else if(bank == 0)
            ret = getPageLengthScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = getPageLengthNV(bank,SNum);
         else if(bank == 0)
            ret = getPageLengthScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = getPageLengthNV(bank,SNum);
         else if(bank == 0)
            ret = getPageLengthScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = getPageLengthNV(bank,SNum);
         else if(bank == 0)
            ret = getPageLengthScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = getPageLengthSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F:
      case 0x12: case 0x13:
         ret = getPageLengthEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = getPageLengthEE77(bank,SNum);
         else if(bank == 0)
            ret = getPageLengthScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Gets the starting physical address of this bank.  Physical
 * banks are sometimes sub-divided into logical banks due to changes
 * in attributes.  Note that this method is for information only.  The read
 * and write methods will automatically calculate the physical address
 * when writing to a logical memory bank.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  physical starting address of this logical bank
 */
int owGetStartingAddress(SMALLINT bank, uchar *SNum)
{
   int ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = getStartingAddressEE(bank,SNum);
         else if(bank == 0)
            ret = getStartingAddressAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = getStartingAddressNV(bank,SNum);
         else if(bank == 0)
            ret = getStartingAddressScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = getStartingAddressNV(bank,SNum);
         else if(bank == 0)
            ret = getStartingAddressScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = getStartingAddressNV(bank,SNum);
         else if(bank == 0)
            ret = getStartingAddressScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = getStartingAddressNV(bank,SNum);
         else if(bank == 0)
            ret = getStartingAddressScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = getStartingAddressSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = getStartingAddressEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = getStartingAddressEE77(bank,SNum);
         else if(bank == 0)
            ret = getStartingAddressScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Gets a string description of this memory bank.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  the memory bank description
 */
char *owGetBankDescription(SMALLINT bank, uchar *SNum)
{
   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            return getBankDescriptionEE(bank,SNum);
         else if(bank == 0)
            return getBankDescriptionAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            return getBankDescriptionNV(bank,SNum);
         else if(bank == 0)
            return getBankDescriptionScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            return getBankDescriptionNV(bank,SNum);
         else if(bank == 0)
            return getBankDescriptionScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            return getBankDescriptionNV(bank,SNum);
         else if(bank == 0)
            return getBankDescriptionScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            return getBankDescriptionNV(bank,SNum);
         else if(bank == 0)
            return getBankDescriptionScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         return getBankDescriptionSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         return getBankDescriptionEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            return getBankDescriptionEE77(bank,SNum);
         else if(bank == 0)
            return getBankDescriptionScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return "";
}

/**
 * Retrieves the Dallas Semiconductor part number of the 1-Wire device
 * as a String.  For example 'Crypto iButton' or 'DS1992'.
 *
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return 1-Wire device name
 */
char *owGetName(uchar *SNum)
{
   switch(SNum[0] & 0x7F)
   {
      case 0x14:
         return "DS1971";
         break;

      case 0x04:
         return "DS1994";
         break;

      case 0x06:
         return "DS1993";
         break;

      case 0x08:
         return "DS1992";
         break;

      case 0x09:
         return "DS1982";
         break;

      case 0x0A:
         return "DS1995";
         break;

      case 0x0B:
         return "DS1985";
         break;

      case 0x0C:
         return "DS1996";
         break;

      case 0x18:
         return "DS1963S";
         break;

      case 0x1A:
         return "DS1963L";
         break;

      case 0x1D:
         return "DS2423";
         break;

      case 0x23:
         return "DS1973";
         break;

      case 0x33: case 0xB3:
         return "DS1961S";
         break;

      case 0x37: case 0x77:
         return "DS1977";
         break;

      default:
         return "No Name";
   }
}

/**
 * Retrieves the alternate Dallas Semiconductor part numbers or names.
 * A 'family' of 1-Wire Network devices may have more than one part number
 * depending on packaging.  There can also be nicknames such as
 * 'Crypto iButton'.
 *
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return 1-Wire device alternate names
 */
char *owGetAlternateName(uchar *SNum)
{
   switch(SNum[0] & 0x7F)
   {
      case 0x14:
         return "DS2430A";
         break;

      case 0x04:
         return "DS2404, Time-in-a-can, DS1427";
         break;

      case 0x09:
         return "DS2502";
         break;

      case 0x0B:
         return "DS2505";
         break;

      case 0x18:
         return "SHA-1 iButton";
         break;

      case 0x1A:
         return "Monetary iButton";
         break;

      case 0x23:
         return "DS2433";
         break;

      case 0x33: case 0xB3:
         return "DS2432";
         break;

      default:
         return "No Alternate Name";
   }
}

/**
 * Retrieves a short description of the function of the 1-Wire device type.
 *
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return device functional description
 */
char *owGetDescription(uchar *SNum)
{
   switch(SNum[0] & 0x7F)
   {
      case 0x14:
         return "Electrically Erasable Programmable Read Only Memory\n"
                "*\t(EEPROM) organized as one page of 256 bits and 64 bit\n"
                "*\tone-time programmable application register.";
         break;

      case 0x04:
         return "4096 bit read/write nonvolatile memory partitioned\n"
                "*\tinto sixteen pages of 256 bits each and a real\n"
                "*\ttime clock/calendar in binary format.";
         break;

      case 0x06:
         return "4096 bit read/write nonvolatile memory partitioned\n"
                "*\tinto sixteen pages of 256 bits each.";
         break;

      case 0x08:
         return "1024 bit read/write nonvolatile memory partitioned\n"
                "*\tinto four pages of 256 bits each.";
         break;

      case 0x09:
         return "1024 bit Electrically Programmable Read Only Memory\n"
                "*\t(EPROM) partitioned into four 256 bit pages.\n"
                "*\tEach memory page can be permanently write-protected\n"
                "*\tto prevent tampering.  Architecture allows software\n"
                "*\tto patch data by supersending a used page in favor of\n"
                "*\ta newly programmed page.";
         break;

      case 0x0A:
         return "16384 bit read/write nonvolatile memory partitioned\n"
                "*\tinto sixty-four pages of 256 bits each.";
         break;

      case 0x0B:
         return "16384 bit Electrically Programmable Read Only Memory\n"
                "*\t(EPROM) partitioned into sixty-four 256 bit pages.\n"
                "*\tEach memory page can be permanently write-protected\n"
                "*\tto prevent tampering.  Architecture allows software\n"
                "*\tto patch data by supersending a used page in favor of\n"
                "*\ta newly programmed page.\n";
         break;

      case 0x0C:
         return "65536 bit read/write nonvolatile memory partitioned\n"
                "*\tinto two-hundred fifty-six pages of 256 bits each.";
         break;

      case 0x18:
         return "4096 bits of read/write nonvolatile memory. Memory\n"
                "*\tis partitioned into sixteen pages of 256 bits each.\n"
                "*\tHas overdrive mode.  One-chip 512-bit SHA-1 engine\n"
                "*\tand secret storage.\n";
         break;

      case 0x1A:
         return "4096 bit read/write nonvolatile memory with\n"
                "*\tfour 32-bit read-only non rolling-over page write\n"
                "*\tcycle counters and tamper-detect bits for small\n"
                "*\tmoney storage.\n";
         break;

      case 0x1D:
         return "1-Wire counter with 4096 bits of read/write, nonvolatile\n"
                "*\tmemory.  Memory is partitioned into sixteen pages of 256\n"
                "*\tbits each.  256 bit scratchpad ensures data transfer\n"
                "*\tintegrity.  Has overdrive mode.  Last four pages each have\n"
                "*\t32 bit read-only non rolling-over counter.  The first two\n"
                "*\tcounters increment on a page write cycle and the second two\n"
                "*\thave active-low external triggers.\n";
         break;

      case 0x23:
         return "4096 bit Electrically Erasable Programmable Read\n"
                "*\tOnly Memory (EEPROM) organized as sixteen pages of 256 bits.";
         break;

      case 0x33: case 0xB3:
         return "1K-Bit protected 1-Wire EEPROM with SHA-1 Engine.";
         break;

      case 0x37: case 0x77:
         return "32Kbyte EEPROM with read-only password and full\n"
                "*\taccess password.";
         break;

      default:
         return "No given description";
   }
}

/**
 * Checks to see if this memory bank is general purpose
 * user memory.  If it is NOT then it may be Memory-Mapped and writing
 * values to this memory may affect the behavior of the 1-Wire
 * device.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  true if this memory bank is general purpose
 */
SMALLINT owIsGeneralPurposeMemory(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = isGeneralPurposeMemoryEE(bank,SNum);
         else if(bank == 0)
            ret = isGeneralPurposeMemoryAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = isGeneralPurposeMemoryNV(bank,SNum);
         else if(bank == 0)
            ret = isGeneralPurposeMemoryScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = isGeneralPurposeMemoryNV(bank,SNum);
         else if(bank == 0)
            ret = isGeneralPurposeMemoryScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = isGeneralPurposeMemoryNV(bank,SNum);
         else if(bank == 0)
            ret = isGeneralPurposeMemoryScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = isGeneralPurposeMemoryNV(bank,SNum);
         else if(bank == 0)
            ret = isGeneralPurposeMemoryScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = isGeneralPurposeMemorySHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = isGeneralPurposeMemoryEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = isGeneralPurposeMemoryEE77(bank,SNum);
         else if(bank == 0)
            ret = isGeneralPurposeMemoryScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Checks to see if this memory bank is read/write.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return    true if this memory bank is read/write
 */
SMALLINT owIsReadWrite(SMALLINT bank, int portnum, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = isReadWriteEE(bank,portnum,SNum);
         else if(bank == 0)
            ret = isReadWriteAppReg(bank,portnum,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = isReadWriteNV(bank,portnum,SNum);
         else if(bank == 0)
            ret = isReadWriteScratch(bank,portnum,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = isReadWriteNV(bank,portnum,SNum);
         else if(bank == 0)
            ret = isReadWriteScratch(bank,portnum,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = isReadWriteNV(bank,portnum,SNum);
         else if(bank == 0)
            ret = isReadWriteScratch(bank,portnum,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = isReadWriteNV(bank,portnum,SNum);
         else if(bank == 0)
            ret = isReadWriteScratch(bank,portnum,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = isReadWriteSHAEE(bank,portnum,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = isReadWriteEPROM(bank,portnum,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = isReadWriteEE77(bank,portnum,SNum);
         else if(bank == 0)
            ret = isReadWriteScratchEx77(bank,portnum,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Checks to see if this memory bank is write once such
 * as with EPROM technology.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return    true if this memory bank can only be written once
 */
SMALLINT owIsWriteOnce(SMALLINT bank, int portnum, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = isWriteOnceEE(bank,portnum,SNum);
         else if(bank == 0)
            ret = isWriteOnceAppReg(bank,portnum,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = isWriteOnceNV(bank,portnum,SNum);
         else if(bank == 0)
            ret = isWriteOnceScratch(bank,portnum,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = isWriteOnceNV(bank,portnum,SNum);
         else if(bank == 0)
            ret = isWriteOnceScratch(bank,portnum,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = isWriteOnceNV(bank,portnum,SNum);
         else if(bank == 0)
            ret = isWriteOnceScratch(bank,portnum,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = isWriteOnceNV(bank,portnum,SNum);
         else if(bank == 0)
            ret = isWriteOnceScratch(bank,portnum,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = isWriteOnceSHAEE(bank,portnum,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = isWriteOnceEPROM(bank,portnum,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = isWriteOnceEE77(bank,portnum,SNum);
         else if(bank == 0)
            ret = isWriteOnceScratchEx77(bank,portnum,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Query to see if current memory bank is read only.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return    true if current memory bank can only be read
 */
SMALLINT owIsReadOnly(SMALLINT bank, int portnum, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = isReadOnlyEE(bank,portnum,SNum);
         else if(bank == 0)
            ret = isReadOnlyAppReg(bank,portnum,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = isReadOnlyNV(bank,portnum,SNum);
         else if(bank == 0)
            ret = isReadOnlyScratch(bank,portnum,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = isReadOnlyNV(bank,portnum,SNum);
         else if(bank == 0)
            ret = isReadOnlyScratch(bank,portnum,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = isReadOnlyNV(bank,portnum,SNum);
         else if(bank == 0)
            ret = isReadOnlyScratch(bank,portnum,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = isReadOnlySHAEE(bank,portnum,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = isReadOnlyEPROM(bank,portnum,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = isReadOnlyEE77(bank,portnum,SNum);
         else if(bank == 0)
            ret = isReadOnlyScratchEx77(bank,portnum,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Query to see if current memory bank non-volatile.  Memory is
 * non-volatile if it retains its contents even when removed from
 * the 1-Wire network.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return    true if current memory bank non volatile.
 */
SMALLINT owIsNonVolatile(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = isNonVolatileEE(bank,SNum);
         else if(bank == 0)
            ret = isNonVolatileAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = isNonVolatileNV(bank,SNum);
         else if(bank == 0)
            ret = isNonVolatileScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = isNonVolatileNV(bank,SNum);
         else if(bank == 0)
            ret = isNonVolatileScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = isNonVolatileNV(bank,SNum);
         else if(bank == 0)
            ret = isNonVolatileScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = isNonVolatileNV(bank,SNum);
         else if(bank == 0)
            ret = isNonVolatileScratch(bank,SNum);
         break;

      case 0x33: case 0xB3: //SHAEE Memory Bank  //SHAEE Memory Bank
         ret = isNonVolatileSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = isNonVolatileEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = isNonVolatileEE77(bank,SNum);
         else if(bank == 0)
            ret = isNonVolatileScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Checks to see if this  memory bank requires a
 * 'ProgramPulse' in order to write.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return    true if writing to this memory bank
 *            requires a 'ProgramPulse' from the 1-Wire Adapter.
 */
SMALLINT owNeedsProgramPulse(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = needsProgramPulseEE(bank,SNum);
         else if(bank == 0)
            ret = needsProgramPulseAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = needsProgramPulseNV(bank,SNum);
         else if(bank == 0)
            ret = needsProgramPulseScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = needsProgramPulseNV(bank,SNum);
         else if(bank == 0)
            ret = needsProgramPulseScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = needsProgramPulseNV(bank,SNum);
         else if(bank == 0)
            ret = needsProgramPulseScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = needsProgramPulseNV(bank,SNum);
         else if(bank == 0)
            ret = needsProgramPulseScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = needsProgramPulseSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = needsProgramPulseEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = needsProgramPulseEE77(bank,SNum);
         else if(bank == 0)
            ret = needsProgramPulseScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Checks to see if this memory bank requires 'PowerDelivery'
 * in order to write.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return    true if writing to this memory bank
 *            requires 'PowerDelivery' from the 1-Wire Adapter
 */
SMALLINT owNeedsPowerDelivery(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = needsPowerDeliveryEE(bank,SNum);
         else if(bank == 0)
            ret = needsPowerDeliveryAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = needsPowerDeliveryNV(bank,SNum);
         else if(bank == 0)
            ret = needsPowerDeliveryScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = needsPowerDeliveryNV(bank,SNum);
         else if(bank == 0)
            ret = needsPowerDeliveryScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = needsPowerDeliveryNV(bank,SNum);
         else if(bank == 0)
            ret = needsPowerDeliveryScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = needsPowerDeliveryNV(bank,SNum);
         else if(bank == 0)
            ret = needsPowerDeliveryScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = needsPowerDeliverySHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = needsPowerDeliveryEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = needsPowerDeliveryEE77(bank,SNum);
         else if(bank == 0)
            ret = needsPowerDeliveryScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Checks to see if this memory bank's pages deliver extra
 * information outside of the normal data space,  when read.  Examples
 * of this may be a redirection byte, counter, tamper protection
 * bytes, or SHA-1 result.  If this method returns true then the
 * methods with an 'extraInfo' parameter can be used:
 * owReadPageExtra,
 * owReadPageExtraCRC, and
 * owReadPagePacketExtra.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  <CODE> true </CODE> if reading the this memory bank's
 *                 pages provides extra information
 */
SMALLINT owHasExtraInfo(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = hasExtraInfoEE(bank,SNum);
         else if(bank == 0)
            ret = hasExtraInfoAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = hasExtraInfoNV(bank,SNum);
         else if(bank == 0)
            ret = hasExtraInfoScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = hasExtraInfoNV(bank,SNum);
         else if(bank == 0)
            ret = hasExtraInfoScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = hasExtraInfoNV(bank,SNum);
         else if(bank == 0)
            ret = hasExtraInfoScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = hasExtraInfoNV(bank,SNum);
         else if(bank == 0)
            ret = hasExtraInfoScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = hasExtraInfoSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = hasExtraInfoEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = hasExtraInfoEE77(bank,SNum);
         else if(bank == 0)
            ret = hasExtraInfoScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Gets the length in bytes of extra information that
 * is read when reading a page in this memory bank.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  number of bytes in Extra Information read when reading
 *          pages from this memory bank
 */
SMALLINT owGetExtraInfoLength(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = getExtraInfoLengthEE(bank,SNum);
         else if(bank == 0)
            ret = getExtraInfoLengthAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = getExtraInfoLengthNV(bank,SNum);
         else if(bank == 0)
            ret = getExtraInfoLengthScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = getExtraInfoLengthNV(bank,SNum);
         else if(bank == 0)
            ret = getExtraInfoLengthScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = getExtraInfoLengthNV(bank,SNum);
         else if(bank == 0)
            ret = getExtraInfoLengthScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = getExtraInfoLengthNV(bank,SNum);
         else if(bank == 0)
            ret = getExtraInfoLengthScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = getExtraInfoLengthSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = getExtraInfoLengthEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = getExtraInfoLengthEE77(bank,SNum);
         else if(bank == 0)
            ret = getExtraInfoLengthScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Gets a string description of what is contained in
 * the Extra Information returned when reading pages in this
 * memory bank.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return extra information description.
 */
char *owGetExtraInfoDesc(SMALLINT bank, uchar *SNum)
{
   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            return getExtraInfoDescEE(bank,SNum);
         else if(bank == 0)
            return getExtraInfoDescAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            return getExtraInfoDescNV(bank,SNum);
         else if(bank == 0)
            return getExtraInfoDescScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            return getExtraInfoDescNV(bank,SNum);
         else if(bank == 0)
            return getExtraInfoDescScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            return getExtraInfoDescNV(bank,SNum);
         else if(bank == 0)
            return getExtraInfoDescScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            return getExtraInfoDescNV(bank,SNum);
         else if(bank == 0)
            return getExtraInfoDescScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         return getExtraInfoDescSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         return getExtraInfoDescEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            return getExtraInfoDescEE77(bank,SNum);
         else if(bank == 0)
            return getExtraInfoDescScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return "";
}

/**
 * Gets Maximum data page length in bytes for a packet
 * read or written in this memory bank.  See the
 * owReadPagePacket and owWritePagePacket
 * function.  This function is only usefull
 * if this memory bank is general purpose memory.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  max packet page length in bytes in this memory bank
 */
SMALLINT owGetMaxPacketDataLength(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = getMaxPacketDataLengthEE(bank,SNum);
         else if(bank == 0)
            ret = getMaxPacketDataLengthAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = getMaxPacketDataLengthNV(bank,SNum);
         else if(bank == 0)
            ret = getMaxPacketDataLengthScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = getMaxPacketDataLengthNV(bank,SNum);
         else if(bank == 0)
            ret = getMaxPacketDataLengthScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = getMaxPacketDataLengthNV(bank,SNum);
         else if(bank == 0)
            ret = getMaxPacketDataLengthScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = getMaxPacketDataLengthSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = getMaxPacketDataLengthEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = getMaxPacketDataLengthEE77(bank,SNum);
         else if(bank == 0)
            ret = getMaxPacketDataLengthScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Checks to see if this memory bank's pages can be read with
 * the contents being verified by a device generated CRC.
 * This is used to see if the owReadPageCRC function can be used.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  true if this memory bank can be
 *          read with self generated CRC
 */
SMALLINT owHasPageAutoCRC(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = hasPageAutoCRCEE(bank,SNum);
         else if(bank == 0)
            ret = hasPageAutoCRCAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = hasPageAutoCRCNV(bank,SNum);
         else if(bank == 0)
            ret = hasPageAutoCRCScratch(bank,SNum);
         break;

      case 0x18:  //NV Memory Bank and Scratch SHA Memory Bank
         if(bank > 0)
            ret = hasPageAutoCRCNV(bank,SNum);
         else if(bank == 0)
            ret = hasPageAutoCRCScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = hasPageAutoCRCNV(bank,SNum);
         else if(bank == 0)
            ret = hasPageAutoCRCScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = hasPageAutoCRCNV(bank,SNum);
         else if(bank == 0)
            ret = hasPageAutoCRCScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = hasPageAutoCRCSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:  //EPROM Memory Bank
         ret = hasPageAutoCRCEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = hasPageAutoCRCEE77(bank,SNum);
         else if(bank == 0)
            ret = hasPageAutoCRCScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Checks to see if this memory bank has pages that can be redirected
 * to a new page.  This is used in Write-Once memory
 * to provide a means to update.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  true if this memory bank pages can be redirected
 *          to a new page
 */
SMALLINT owCanRedirectPage(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = canRedirectPageEE(bank,SNum);
         else if(bank == 0)
            ret = canRedirectPageAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = canRedirectPageNV(bank,SNum);
         else if(bank == 0)
            ret = canRedirectPageScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = canRedirectPageNV(bank,SNum);
         else if(bank == 0)
            ret = canRedirectPageScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = canRedirectPageNV(bank,SNum);
         else if(bank == 0)
            ret = canRedirectPageScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = canRedirectPageSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:
         ret = canRedirectPageEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = canRedirectPageEE77(bank,SNum);
         else if(bank == 0)
            ret = canRedirectPageScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Checks to see if this memory bank has pages that can be locked.  A
 * locked page would prevent any changes to it's contents.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  true if this memory bank has pages that can be
 *          locked
 */
SMALLINT owCanLockPage(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = canLockPageEE(bank,SNum);
         else if(bank == 0)
            ret = canLockPageAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = canLockPageNV(bank,SNum);
         else if(bank == 0)
            ret = canLockPageScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = canLockPageNV(bank,SNum);
         else if(bank == 0)
            ret = canLockPageScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = canLockPageNV(bank,SNum);
         else if(bank == 0)
            ret = canLockPageScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = canLockPageSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:
         ret = canLockPageEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = canLockPageEE77(bank,SNum);
         else if(bank == 0)
            ret = canLockPageScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Checks to see if this memory bank has pages that can be locked from
 * being redirected.  This would prevent a Write-Once memory from
 * being updated.
 *
 * bank       to tell what memory bank of the ibutton to use.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 *
 * @return  true if this memory bank has pages that can
 *          be locked from being redirected to a new page
 */
SMALLINT owCanLockRedirectPage(SMALLINT bank, uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x14:  //EE Memory Bank and AppReg Memory Bank
         if(bank > 0)
            ret = canLockRedirectPageEE(bank,SNum);
         else if(bank == 0)
            ret = canLockRedirectPageAppReg(bank,SNum);
         break;

      case 0x04: case 0x06: case 0x08: case 0x0A:
      case 0x0C: case 0x23:  //NV Memory Bank and Scratch Memory Bank
         if(bank > 0)
            ret = canLockRedirectPageNV(bank,SNum);
         else if(bank == 0)
            ret = canLockRedirectPageScratch(bank,SNum);
         break;

      case 0x1A: case 0x1D:  //NVCRC Memory Bank and Scratch Ex Memory Bank
         if(bank > 0)
            ret = canLockRedirectPageNV(bank,SNum);
         else if(bank == 0)
            ret = canLockRedirectPageScratch(bank,SNum);
         break;

      case 0x21:  //NVCRC Memory Bank and Scratch CRC Memory Bank
         if(bank > 0)
            ret = canLockRedirectPageNV(bank,SNum);
         else if(bank == 0)
            ret = canLockRedirectPageScratch(bank,SNum);
         break;

      case 0x33: case 0xB3:  //SHAEE Memory Bank
         ret = canLockRedirectPageSHAEE(bank,SNum);
         break;

      case 0x09: case 0x0B: case 0x0F: case 0x12: case 0x13:
         ret = canLockRedirectPageEPROM(bank,SNum);
         break;

      case 0x37: case 0x77:
         if(bank > 0)
            ret = canLockRedirectPageEE77(bank,SNum);
         else if(bank == 0)
            ret = canLockRedirectPageScratchEx77(bank,SNum);
         break;

      default:
         break;
   }

   return ret;
}

/**
 * Tells whether a password is possibly needed.
 *
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * pass       the password to set for read/write operations.
 *
 * @return    'true' if the password was set..
 */
SMALLINT owNeedPassword(uchar *SNum)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x37: case 0x77:
         ret = TRUE;
         break;

      default:
         ret = FALSE;
         break;
   }

   return ret;
}

/**
 * Sets the password on the part for read-only operations.
 *
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * pass       the password to set for read-only operations.
 *
 * @return    'true' if the password was set..
 */
SMALLINT owSetReadOnlyPassword(int portnum, uchar *SNum, uchar *pass)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x37: case 0x77:
         if(setPassword(portnum,SNum,pass,READ_PSW))
         {
            ret = verifyPassword(portnum,SNum,pass,READ_PSW);

            if(ret)
               setBMPassword(pass);
         }
         else
         {
            ret = FALSE;
         }
         break;

      default:
         OWERROR(OWERROR_NO_READ_ONLY_PASSWORD);
         ret = FALSE;
         break;
   }

   return ret;
}

/**
 * Sets the password on the part for read/write operations.
 *
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * pass       the password to set for read/write operations.
 *
 * @return    'true' if the password was set..
 */
SMALLINT owSetReadWritePassword(int portnum, uchar *SNum, uchar *pass)
{
   SMALLINT ret = FALSE;

   switch(SNum[0] & 0x7F)
   {
      case 0x37: case 0x77:
         if(setPassword(portnum,SNum,pass,READ_WRITE_PSW))
         {
            ret = verifyPassword(portnum,SNum,pass,READ_WRITE_PSW);

            if(ret)
               setBMPassword(pass);
         }
         else
         {
            ret = FALSE;
         }
         break;

      default:
         OWERROR(OWERROR_NO_READ_WRITE_PASSWORD);
         ret = FALSE;
         break;
   }

   return ret;
}

/**
 * Sets the password for read-only operations.
 *
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * pass       the password for read-only operations.
 *
 * @return    'true' if the password was set..
 */
SMALLINT owSetBMReadOnlyPassword(int portnum, uchar *SNum, uchar *pass)
{
   SMALLINT ret = 0;

   switch(SNum[0] & 0x7F)
   {
      case 0x37: case 0x77:
         setBMPassword(pass);
         ret = TRUE;
         break;

      default:
         OWERROR(OWERROR_NO_READ_ONLY_PASSWORD);
         ret = FALSE;
         break;
   }

   return ret;
}

/**
 * Sets the password for read/write operations.
 *
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * pass       the password for read/write operations.
 *
 * @return    'true' if the password was set..
 */
SMALLINT owSetBMReadWritePassword(int portnum, uchar *SNum, uchar *pass)
{
   SMALLINT ret = FALSE;

   switch(SNum[0] & 0x7F)
   {
      case 0x37: case 0x77:
         setBMPassword(pass);
         ret = TRUE;
         break;

      default:
         OWERROR(OWERROR_NO_READ_WRITE_PASSWORD);
         ret = FALSE;
         break;
   }

   return ret;
}

/**
 * Sets the password for read/write operations.
 *
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * pass       the password for read/write operations.
 *
 * @return    'true' if the password was set..
 */
SMALLINT owSetPasswordMode(int portnum, uchar *SNum, int mode)
{
   SMALLINT ret = FALSE;

   switch(SNum[0] & 0x7F)
   {
      case 0x37: case 0x77:
         ret = setPasswordMode(portnum,SNum,mode);
         break;

      default:
         OWERROR(OWERROR_NO_READ_WRITE_PASSWORD);
         ret = FALSE;
         break;
   }

   return ret;
}

/**
 * Gets the memory bank given the page for the part.
 *
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * page       the page number for the part not memory bank
 * flag       the flag to indicate status memory or regular memory
 *
 * @return    the bank number that the page is in.
 */
SMALLINT getBank(int portnum, uchar *SNum, PAGE_TYPE page, uchar flag)
{
   SMALLINT bank = 1;
   SMALLINT i,starting_bank=1;
   int      tmp_pg;
   int      num_pg;

   if(flag == STATUSMEM)
   {
      tmp_pg = page;

      for(i=starting_bank;i<owGetNumberBanks(SNum[0]);i++)
      {
         if((i+1) == owGetNumberBanks(SNum[0]))
            return i;
         else
         {
            if(owGetPageLength(bank,SNum) != 0)
               num_pg = owGetStartingAddress(i+1,SNum)/
                        owGetPageLength(bank,SNum);
            else
               return FALSE;
         }

         if(num_pg >= tmp_pg)
         {
            if(num_pg == tmp_pg)
               return i+1;
            else
               return i;
         }

      }
   }
   else
   {
      if(owIsWriteOnce(bank,portnum,SNum))
      {
         tmp_pg = page;

         for(i=0;i<owGetNumberBanks(SNum[0]);i++)
         {
            if(owIsNonVolatile(i,SNum) && owIsGeneralPurposeMemory(i,SNum))
            {
               num_pg = owGetNumberPages(i,SNum);

               if(num_pg >= tmp_pg)
                  return i;
               else
                  tmp_pg = tmp_pg - num_pg;
            }
         }
      }
      else
      {
         tmp_pg = page;

         for(i=0;i<owGetNumberBanks(SNum[0]);i++)
         {
            if(owIsNonVolatile(i,SNum) && owIsGeneralPurposeMemory(i,SNum) &&
               owIsReadWrite(i,portnum,SNum))
            {
               num_pg = owGetNumberPages(i,SNum);

               if(num_pg > tmp_pg)
                  return i;
               else
               tmp_pg = tmp_pg - num_pg;
            }
         }
      }
   }

   return FALSE;
}

/**
 * Gets the memory bank given the page for the part.
 *
 * portnum    the port number of the port being used for the
 *            1-Wire Network.
 * SNum       the serial number for the part that the operation is
 *            to be done on.
 * page       the page number for the part not memory bank
 * flag       the flag to indicate status memory or regular memory
 *
 * @return    the bank number that the page is in.
 */
SMALLINT getPage(int portnum, uchar *SNum, PAGE_TYPE page, uchar flag)
{
   SMALLINT bank = 1;
   SMALLINT i,starting_bank=1;
   int      tmp_pg;
   int      num_pg;

   if(flag == STATUSMEM)
   {
      tmp_pg = page;

      for(i=starting_bank;i<owGetNumberBanks(SNum[0]);i++)
      {
         if((i+1) == owGetNumberBanks(SNum[0]))
         {
            if(owGetPageLength(bank,SNum) != 0)
               return (tmp_pg - (owGetStartingAddress(i,SNum)/
                                 owGetPageLength(bank,SNum)));
            else
               return FALSE;
         }
         else
         {
            if(owGetPageLength(bank,SNum) != 0)
               num_pg = owGetStartingAddress(i+1,SNum)/owGetPageLength(bank,SNum);
            else
               return FALSE;
         }

         if(num_pg >= tmp_pg)
         {
            if(num_pg == tmp_pg)
               return 0;
            else if(owGetStartingAddress(i,SNum) == 0x00)
               return tmp_pg;
            else
            {
               if(owGetPageLength(bank,SNum) != 0)
                  return (tmp_pg - (owGetStartingAddress(i,SNum)/
                                    owGetPageLength(bank,SNum)));
               else
                  return FALSE;
            }
         }
      }
   }
   else
   {
      if(owIsWriteOnce(bank,portnum,SNum))
      {
         tmp_pg = page;

         for(i=0;i<owGetNumberBanks(SNum[0]);i++)
         {
            if(owIsNonVolatile(i,SNum) && owIsGeneralPurposeMemory(i,SNum))
            {
               num_pg = owGetNumberPages(i,SNum);

               if(num_pg >= tmp_pg)
                  return tmp_pg;
               else
                  tmp_pg = tmp_pg - num_pg;
            }
         }
      }
      else
      {
         tmp_pg = page;

         for(i=0;i<owGetNumberBanks(SNum[0]);i++)
         {
            if(owIsNonVolatile(i,SNum) && owIsGeneralPurposeMemory(i,SNum) &&
               owIsReadWrite(i,portnum,SNum))
            {
               num_pg = owGetNumberPages(i,SNum);

               if(num_pg > tmp_pg)
                  return tmp_pg;
               else
                  tmp_pg = tmp_pg - num_pg;
            }
         }
      }
   }

   return FALSE;
}