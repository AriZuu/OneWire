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

#include "ownet.h"


// Local function definitions
SMALLINT writeScratchPadEx77(int pornum, uchar *SNum, int str_add, uchar *writeBuf, int len);
SMALLINT readScratchPadCRC77(int portnum, uchar *SNum, uchar *buff, int len, uchar *extra);
SMALLINT copyScratchPadPsw77(int portnum, uchar *SNum, int str_add, int len, uchar *psw);
char *getBankDescriptionScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT canLockRedirectPageScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT canLockPageScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT canRedirectPageScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT getMaxPacketDataLengthScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT hasPageAutoCRCScratchEx77(SMALLINT bank, uchar *SNum);
char *getExtraInfoDescScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT getExtraInfoLengthScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT hasExtraInfoScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT needsPowerDeliveryScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT needsProgramPulseScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT isNonVolatileScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT isReadOnlyScratchEx77(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isWriteOnceScratchEx77(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isReadWriteScratchEx77(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isGeneralPurposeMemoryScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT getPageLengthScratchEx77(SMALLINT bank, uchar *SNum);
int getStartingAddressScratchEx77(SMALLINT bank, uchar *SNum);
int getSizeScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT getNumberPagesScratchEx77(SMALLINT bank, uchar *SNum);
SMALLINT writePagePacketScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page,
                                    uchar *buff, int len);
SMALLINT readPagePacketExtraScratchEx77(SMALLINT bank, int portnum, uchar *SNum,
                                      int page, SMALLINT rd_cont, uchar *buff,
                                      int *len, uchar *extra);
SMALLINT readPagePacketScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page,
                                   SMALLINT rd_cont, uchar *buff, int *len);
SMALLINT readPageCRCScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page, uchar *buff);
SMALLINT readPageExtraCRCScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page,
                                     uchar *read_buff, uchar *extra);
SMALLINT readPageExtraScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page,
                                  SMALLINT rd_cont, uchar *buff, uchar *extra);
SMALLINT readPageScratchEx77(SMALLINT bank, int portnum, uchar *SNum, int page,
                             SMALLINT rd_cont, uchar *buff);


