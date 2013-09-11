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
// mbEPROM.h - Include memory bank EPROM functions.
//
// Version: 2.10
//

#include "ownet.h"

// Local function definitions
SMALLINT readEPROM(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                   SMALLINT rd_cont, uchar *buff,  int len);
SMALLINT writeEPROM(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                    uchar *buff, int len);
SMALLINT readPageEPROM(SMALLINT bank, int portnum, uchar *SNum, int page,
                       SMALLINT rd_cont, uchar *buff);
SMALLINT readPageExtraEPROM(SMALLINT bank, int portnum, uchar *SNum, int page,
                            SMALLINT rd_cont, uchar *buff, uchar *extra);
SMALLINT readPageExtraCRCEPROM(SMALLINT bank, int portnum, uchar *SNum, int page,
                               uchar *read_buff, uchar *extra);
SMALLINT readPageCRCEPROM(SMALLINT bank, int portnum, uchar *SNum, int page,
                          uchar *buff);
SMALLINT readPagePacketEPROM(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                             SMALLINT rd_cont, uchar *buff, int *len);
SMALLINT readPagePacketExtraEPROM(SMALLINT bank, int portnum, uchar *SNum, 
                                  int str_add, SMALLINT rd_cont, uchar *buff,
                                  int *len, uchar *extra);
SMALLINT writePagePacketEPROM(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                              uchar *buff, int len);
SMALLINT getNumberPagesEPROM(SMALLINT bank, uchar *SNum);
int      getSizeEPROM(SMALLINT bank, uchar *SNum);
SMALLINT getPageLengthEPROM(SMALLINT bank, uchar *SNum);
int      getStartingAddressEPROM(SMALLINT bank, uchar *SNum);
char    *getBankDescriptionEPROM(SMALLINT bank, uchar *SNum);
SMALLINT isGeneralPurposeMemoryEPROM(SMALLINT bank, uchar *SNum);
SMALLINT isReadWriteEPROM(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isWriteOnceEPROM(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isReadOnlyEPROM(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isNonVolatileEPROM(SMALLINT bank, uchar *SNum);
SMALLINT needsProgramPulseEPROM(SMALLINT bank, uchar *SNum);
SMALLINT needsPowerDeliveryEPROM(SMALLINT bank, uchar *SNum);
SMALLINT hasExtraInfoEPROM(SMALLINT bank, uchar *SNum);
SMALLINT getExtraInfoLengthEPROM(SMALLINT bank, uchar *SNum);
char    *getExtraInfoDescEPROM(SMALLINT bank, uchar *SNum);
SMALLINT getMaxPacketDataLengthEPROM(SMALLINT bank, uchar *SNum);
SMALLINT hasPageAutoCRCEPROM(SMALLINT bank, uchar *SNum);
SMALLINT canRedirectPageEPROM(SMALLINT bank, uchar *SNum);
SMALLINT canLockPageEPROM(SMALLINT bank, uchar *SNum);
SMALLINT canLockRedirectPageEPROM(SMALLINT bank, uchar *SNum);

// Local functions
uchar    writeMemCmd(SMALLINT bank, uchar *SNum);
SMALLINT numCRCbytes(SMALLINT bank, uchar *SNum);
SMALLINT writeVerify(SMALLINT bank, int portnum, uchar *SNum, int page);
SMALLINT crcAfterAdd(SMALLINT bank, uchar *SNum);
uchar    readPageWithCRC(SMALLINT bank, uchar *SNum);
SMALLINT isRedirectPageLocked(SMALLINT bank, int portnum, uchar *SNum, int page);
SMALLINT lockRedirectPage(SMALLINT bank, int portnum, uchar *SNum, int page);
SMALLINT getRedirectedPage(SMALLINT bank, int portnum, uchar *SNum, int page);
SMALLINT redirectPage(SMALLINT bank, int portnum, uchar *SNum, int page, int newPage);
SMALLINT isPageLocked(SMALLINT bank, int portnum, uchar *SNum, int page);
SMALLINT lockPage(SMALLINT bank, int portnum, uchar *SNum, int page);
SMALLINT redirectOffset(SMALLINT bank, uchar *SNum);
