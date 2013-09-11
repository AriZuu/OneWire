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
// mbSCR.h - Memory bank class for the Scratchpad section of NVRAM iButtons and
//           1-Wire devices.
//
// Version: 2.10
//

#include "ownet.h"


// Local function definitions
SMALLINT readScratch(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                     SMALLINT rd_cont, uchar *buff,  int len);
SMALLINT writeScratch(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                      uchar *buff, int len);
SMALLINT readPageScratch(SMALLINT bank, int portnum, uchar *SNum, int page,
                         SMALLINT rd_cont, uchar *buff);
SMALLINT readPageExtraScratch(SMALLINT bank, int portnum, uchar *SNum, int page,
                              SMALLINT rd_cont, uchar *buff, uchar *extra);
SMALLINT readPageExtraCRCScratch(SMALLINT bank, int portnum, uchar *SNum, int page,
                                 uchar *read_buff, uchar *extra);
SMALLINT readPageCRCScratch(SMALLINT bank, int portnum, uchar *SNum, int page, uchar *buff);
SMALLINT readPagePacketScratch(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                               SMALLINT rd_cont, uchar *buff, int *len);
SMALLINT readPagePacketExtraScratch(SMALLINT bank, int portnum, uchar *SNum, 
                                    int str_add, SMALLINT rd_cont, uchar *buff,
                                    int *len, uchar *extra);
SMALLINT writePagePacketScratch(SMALLINT bank, int portnum, uchar *SNum, int page,
                                uchar *buff, int len);
SMALLINT getNumberPagesScratch(SMALLINT bank, uchar *SNum);
int getSizeScratch(SMALLINT bank, uchar *SNum);
int getStartingAddressScratch(SMALLINT bank, uchar *SNum);
SMALLINT getPageLengthScratch(SMALLINT bank, uchar *SNum);
char *getBankDescriptionScratch(SMALLINT bank, uchar *SNum);
SMALLINT isGeneralPurposeMemoryScratch(SMALLINT bank, uchar *SNum);
SMALLINT isReadWriteScratch(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isWriteOnceScratch(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isReadOnlyScratch(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isNonVolatileScratch(SMALLINT bank, uchar *SNum);
SMALLINT needsProgramPulseScratch(SMALLINT bank, uchar *SNum);
SMALLINT needsPowerDeliveryScratch(SMALLINT bank, uchar *SNum);
SMALLINT hasExtraInfoScratch(SMALLINT bank, uchar *SNum);
SMALLINT getExtraInfoLengthScratch(SMALLINT bank, uchar *SNum);
char *getExtraInfoDescScratch(SMALLINT bank, uchar *SNum);
SMALLINT getMaxPacketDataLengthScratch(SMALLINT bank, uchar *SNum);
SMALLINT hasPageAutoCRCScratch(SMALLINT bank, uchar *SNum);
SMALLINT canRedirectPageScratch(SMALLINT bank, uchar *SNum);
SMALLINT canLockPageScratch(SMALLINT bank, uchar *SNum);
SMALLINT canLockRedirectPageScratch(SMALLINT bank, uchar *SNum);

// Scratchpad functions
SMALLINT writeScratchpd(int portnum, int str_add, uchar *writeBuf, int len);
SMALLINT readScratchpd(int portnum, uchar *buff, int len);
SMALLINT readScratchpdExtra(int portnum, uchar *buff, int len, uchar *extra);
SMALLINT copyScratchpd(int portnum, int str_add, int len);

