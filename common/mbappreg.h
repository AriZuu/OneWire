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
// ownet.h - Include file for 1-Wire Net library
//
// Version: 2.10
//

#include "ownet.h"

//Local function definitions
SMALLINT readAppReg(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                    SMALLINT rd_cont, uchar *buff,  int len);
SMALLINT writeAppReg(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                     uchar *buff, int len);
SMALLINT readPageAppReg(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                        SMALLINT rd_cont, uchar *buff);
SMALLINT readPageExtraAppReg(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                             SMALLINT rd_cont, uchar *buff, uchar *extra);
SMALLINT readPageExtraCRCAppReg(SMALLINT bank, int portnum, uchar *SNum, int page,
                       uchar *read_buff, uchar *extra);
SMALLINT readPageCRCAppReg(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                           uchar *buff);
SMALLINT readPagePacketAppReg(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                              SMALLINT rd_cont, uchar *buff, int *len);
SMALLINT readPagePacketExtraAppReg(SMALLINT bank, int portnum, uchar *SNum,
                                   int str_add, SMALLINT rd_cont, uchar *buff,
                                   int *len, uchar *extra);
SMALLINT writePagePacketAppReg(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                               uchar *buff, int len);
SMALLINT getNumberPagesAppReg(SMALLINT bank, uchar *SNum);
int      getSizeAppReg(SMALLINT bank, uchar *SNum);
int      getStartingAddressAppReg(SMALLINT bank, uchar *SNum);
SMALLINT getPageLengthAppReg(SMALLINT bank, uchar *SNum);
char *getBankDescriptionAppReg(SMALLINT bank, uchar *SNum);
SMALLINT isGeneralPurposeMemoryAppReg(SMALLINT bank, uchar *SNum);
SMALLINT isReadWriteAppReg(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isWriteOnceAppReg(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isReadOnlyAppReg(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isNonVolatileAppReg(SMALLINT bank, uchar *SNum);
SMALLINT needsProgramPulseAppReg(SMALLINT bank, uchar *SNum);
SMALLINT needsPowerDeliveryAppReg(SMALLINT bank, uchar *SNum);
SMALLINT hasExtraInfoAppReg(SMALLINT bank, uchar *SNum);
SMALLINT getExtraInfoLengthAppReg(SMALLINT bank, uchar *SNum);
char *getExtraInfoDescAppReg(SMALLINT bank, uchar *SNum);
SMALLINT getMaxPacketDataLengthAppReg(SMALLINT bank, uchar *SNum);
SMALLINT hasPageAutoCRCAppReg(SMALLINT bank, uchar *SNum);

// OTP
SMALLINT canRedirectPageAppReg(SMALLINT bank, uchar *SNum);
SMALLINT canLockPageAppReg(SMALLINT bank, uchar *SNum);
SMALLINT canLockRedirectPageAppReg(SMALLINT bank, uchar *SNum);

// Local functions
SMALLINT readStatus(int portnum, uchar *extra);
// OTP Local functions
SMALLINT lockPageAppReg(int portnum, int page, uchar *SNum);
SMALLINT isPageLockedAppReg(int portnum, int page, uchar *SNum);
SMALLINT redirectPageAppReg(int portnum, int page, int newPage, uchar *SNum);
SMALLINT isPageRedirectedAppReg(int portnum, int page, uchar *SNum);
SMALLINT getRedirectPageAppReg(int portnum, int page, uchar *SNum);
SMALLINT lockRedirectPageAppReg(int portnum, int page, uchar *SNum);





