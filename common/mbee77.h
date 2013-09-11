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
// mbEE77.h - Include memory bank EE functions for the DS1977.
//
// Version: 2.10
//

#include "ownet.h"

// Local function definitions
SMALLINT readEEPsw77(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                SMALLINT rd_cont, uchar *buff,  int len);
SMALLINT writeEE77(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                 uchar *buff, int len);
SMALLINT readPageEE77(SMALLINT bank, int portnum, uchar *SNum, int page,
                    SMALLINT rd_cont, uchar *buff);
SMALLINT readPageExtraEE77(SMALLINT bank, int portnum, uchar *SNum, int page,
                         SMALLINT rd_cont, uchar *buff, uchar *extra);
SMALLINT readPageExtraCRCEE77(SMALLINT bank, int portnum, uchar *SNum, int page,
                            uchar *read_buff, uchar *extra);
SMALLINT readPageCRCEE77(SMALLINT bank, int portnum, uchar *SNum, int page, uchar *buff);
SMALLINT readPagePacketEE77(SMALLINT bank, int portnum, uchar *SNum, int page,
                          SMALLINT rd_cont, uchar *buff, int *len);
SMALLINT readPagePacketExtraEE77(SMALLINT bank, int portnum, uchar *SNum,
                               int page, SMALLINT rd_cont, uchar *buff,
                               int *len, uchar *extra);
SMALLINT writePagePacketEE77(SMALLINT bank, int portnum, uchar *SNum, int page,
                           uchar *buff, int len);
SMALLINT getNumberPagesEE77(SMALLINT bank, uchar *SNum);
int      getSizeEE77(SMALLINT bank, uchar *SNum);
SMALLINT getPageLengthEE77(SMALLINT bank, uchar *SNum);
int      getStartingAddressEE77(SMALLINT bank, uchar *SNum);
char *getBankDescriptionEE77(SMALLINT bank, uchar *SNum);
SMALLINT isGeneralPurposeMemoryEE77(SMALLINT bank, uchar *SNum);
SMALLINT isReadWriteEE77(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isWriteOnceEE77(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isReadOnlyEE77(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isNonVolatileEE77(SMALLINT bank, uchar *SNum);
SMALLINT needsProgramPulseEE77(SMALLINT bank, uchar *SNum);
SMALLINT needsPowerDeliveryEE77(SMALLINT bank, uchar *SNum);
SMALLINT hasExtraInfoEE77(SMALLINT bank, uchar *SNum);
SMALLINT getExtraInfoLengthEE77(SMALLINT bank, uchar *SNum);
char *getExtraInfoDescEE77(SMALLINT bank, uchar *SNum);
SMALLINT getMaxPacketDataLengthEE77(SMALLINT bank, uchar *SNum);
SMALLINT hasPageAutoCRCEE77(SMALLINT bank, uchar *SNum);
SMALLINT canRedirectPageEE77(SMALLINT bank, uchar *SNum);
SMALLINT canLockPageEE77(SMALLINT bank, uchar *SNum);
SMALLINT canLockRedirectPageEE77(SMALLINT bank, uchar *SNum);

SMALLINT canRedirectPageAppReg(SMALLINT bank, uchar *SNum);
SMALLINT canLockPageAppReg(SMALLINT bank, uchar *SNum);
SMALLINT canLockRedirectPageAppReg(SMALLINT bank, uchar *SNum);
SMALLINT needPassword(int portnum, uchar *SNum);

