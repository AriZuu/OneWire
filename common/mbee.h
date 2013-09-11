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
// mbEE.h - Include memory bank EE functions.
//
// Version: 2.10
//

#include "ownet.h"

// Local function definitions
SMALLINT readEE(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                SMALLINT rd_cont, uchar *buff,  int len);
SMALLINT writeEE(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                 uchar *buff, int len);
SMALLINT readPageEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                    SMALLINT rd_cont, uchar *buff);
SMALLINT readPageExtraEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                         SMALLINT rd_cont, uchar *buff, uchar *extra);
SMALLINT readPageExtraCRCEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                            uchar *read_buff, uchar *extra);
SMALLINT readPageCRCEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                       uchar *buff);
SMALLINT readPagePacketEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                          SMALLINT rd_cont, uchar *buff, int *len);
SMALLINT readPagePacketExtraEE(SMALLINT bank, int portnum, uchar *SNum,
                               int page, SMALLINT rd_cont, uchar *buff,
                               int *len, uchar *extra);
SMALLINT writePagePacketEE(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                           uchar *buff, int len);
SMALLINT getNumberPagesEE(SMALLINT bank, uchar *SNum);
int      getSizeEE(SMALLINT bank, uchar *SNum);
SMALLINT getPageLengthEE(SMALLINT bank, uchar *SNum);
int      getStartingAddressEE(SMALLINT bank, uchar *SNum);
char *getBankDescriptionEE(SMALLINT bank, uchar *SNum);
SMALLINT isGeneralPurposeMemoryEE(SMALLINT bank, uchar *SNum);
SMALLINT isReadWriteEE(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isWriteOnceEE(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isReadOnlyEE(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isNonVolatileEE(SMALLINT bank, uchar *SNum);
SMALLINT needsProgramPulseEE(SMALLINT bank, uchar *SNum);
SMALLINT needsPowerDeliveryEE(SMALLINT bank, uchar *SNum);
SMALLINT hasExtraInfoEE(SMALLINT bank, uchar *SNum);
SMALLINT getExtraInfoLengthEE(SMALLINT bank, uchar *SNum);
char *getExtraInfoDescEE(SMALLINT bank, uchar *SNum);
SMALLINT getMaxPacketDataLengthEE(SMALLINT bank, uchar *SNum);
SMALLINT hasPageAutoCRCEE(SMALLINT bank, uchar *SNum);
SMALLINT canRedirectPageEE(SMALLINT bank, uchar *SNum);
SMALLINT canLockPageEE(SMALLINT bank, uchar *SNum);
SMALLINT canLockRedirectPageEE(SMALLINT bank, uchar *SNum);

SMALLINT canRedirectPageAppReg(SMALLINT bank, uchar *SNum);
SMALLINT canLockPageAppReg(SMALLINT bank, uchar *SNum);
SMALLINT canLockRedirectPageAppReg(SMALLINT bank, uchar *SNum);

// Local functions
SMALLINT writeScratchpad (int portnum,int startAddr, uchar *writeBuf, int len);
SMALLINT readScratchpad (int portnum, uchar *readBuf, int startAddr, int len);
SMALLINT copyScratchpad (int portnum);
