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

#include "owfile.h"


SMALLINT owRead(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                SMALLINT rd_cont, uchar *buff, int len);
SMALLINT owWrite(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                 uchar *buff, int len);
SMALLINT owReadPage(SMALLINT bank, int portnum, uchar *SNum, int page,
                    SMALLINT rd_cont, uchar *buff);
SMALLINT owReadPageExtra(SMALLINT bank, int portnum, uchar *SNum, int page,
                         SMALLINT rd_cont, uchar *buff, uchar *extra);
SMALLINT owReadPageExtraCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                            uchar *read_buff, uchar *extra);
SMALLINT owReadPageCRC(SMALLINT bank, int portnum, uchar *SNum, int page, uchar *buff);
SMALLINT owReadPagePacket(SMALLINT bank, int portnum, uchar *SNum, int page,
                          SMALLINT rd_cont, uchar *buff, int *len);
SMALLINT owReadPagePacketExtra(SMALLINT bank, int portnum, uchar *SNum, int page,
                               SMALLINT rd_cont, uchar *buff, int *len, uchar *extra);
SMALLINT owWritePagePacket(SMALLINT bank, int portnum, uchar *SNum, int page,
                           uchar *buff, int len);
SMALLINT owGetNumberBanks(uchar family);
SMALLINT owGetNumberPages(SMALLINT bank, uchar *SNum);
int owGetSize(SMALLINT bank, uchar *SNum);
SMALLINT owGetPageLength(SMALLINT bank, uchar *SNum);
int owGetStartingAddress(SMALLINT bank, uchar *SNum);
char *owGetBankDescription(SMALLINT bank, uchar *SNum);
char *owGetDescription(uchar *SNum);
SMALLINT owIsGeneralPurposeMemory(SMALLINT bank, uchar *SNum);
SMALLINT owIsReadWrite(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT owIsWriteOnce(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT owIsReadOnly(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT owIsNonVolatile(SMALLINT bank, uchar *SNum);
SMALLINT owNeedsProgramPulse(SMALLINT bank, uchar *SNum);
SMALLINT owNeedsPowerDelivery(SMALLINT bank, uchar *SNum);
SMALLINT owHasExtraInfo(SMALLINT bank, uchar *SNum);
SMALLINT owGetExtraInfoLength(SMALLINT bank, uchar *SNum);
char *owGetExtraInfoDesc(SMALLINT bank, uchar *SNum);
SMALLINT owGetMaxPacketDataLength(SMALLINT bank, uchar *SNum);
SMALLINT owHasPageAutoCRC(SMALLINT bank, uchar *SNum);
SMALLINT owCanRedirectPage(SMALLINT bank, uchar *SNum);
SMALLINT owCanLockPage(SMALLINT bank, uchar *SNum);
SMALLINT owCanLockRedirectPage(SMALLINT bank, uchar *SNum);
char *owGetName(uchar *SNum);
char *owGetAlternateName(uchar *SNum);
SMALLINT owSetReadOnlyPassword(int portnum, uchar *SNum, uchar *pass);
SMALLINT owSetReadWritePassword(int portnum, uchar *SNum, uchar *pass);
SMALLINT owSetBMReadOnlyPassword(int portnum, uchar *SNum, uchar *pass);
SMALLINT owSetBMReadWritePassword(int portnum, uchar *SNum, uchar *pass);
SMALLINT owSetPasswordMode(int portnum, uchar *SNum, int mode);
SMALLINT owNeedPassword(uchar *SNum);

SMALLINT getBank(int portnum, uchar *SNum, PAGE_TYPE page, uchar flag);
SMALLINT getPage(int portnum, uchar *SNum, PAGE_TYPE page, uchar flag);