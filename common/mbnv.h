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
SMALLINT readNV(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                SMALLINT rd_cont, uchar *buff,  int len);
SMALLINT writeNV(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                 uchar *buff, int len);
SMALLINT readPageNV(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                    SMALLINT rd_cont, uchar *buff);
SMALLINT readPageExtraNV(SMALLINT bank, int portnum, uchar *SNum, int page,
                         SMALLINT rd_cont, uchar *buff, uchar *extra);
SMALLINT readPageExtraCRCNV(SMALLINT bank, int portnum, uchar *SNum, int page,
                            uchar *read_buff, uchar *extra);
SMALLINT readPageCRCNV(SMALLINT bank, int portnum, uchar *SNum, int page,
                       uchar *buff);
SMALLINT readPagePacketNV(SMALLINT bank, int portnum, uchar *SNum, int page,
                          SMALLINT rd_cont, uchar *buff, int *len);
SMALLINT readPagePacketExtraNV(SMALLINT bank, int portnum, uchar *SNum, 
                               int str_add, SMALLINT rd_cont, uchar *buff,
                               int *len, uchar *extra);
SMALLINT writePagePacketNV(SMALLINT bank, int portnum, uchar *SNum, int page,
                           uchar *buff, int len);
SMALLINT getNumberPagesNV(SMALLINT bank, uchar *SNum);
int getSizeNV(SMALLINT bank, uchar *SNum);
SMALLINT getPageLengthNV(SMALLINT bank, uchar *SNum);
int getStartingAddressNV(SMALLINT bank, uchar *SNum);
char *getBankDescriptionNV(SMALLINT bank, uchar *SNum);
SMALLINT isGeneralPurposeMemoryNV(SMALLINT bank, uchar *SNum);
SMALLINT isReadWriteNV(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isWriteOnceNV(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isReadOnlyNV(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isNonVolatileNV(SMALLINT bank, uchar *SNum);
SMALLINT needsProgramPulseNV(SMALLINT bank, uchar *SNum);
SMALLINT needsPowerDeliveryNV(SMALLINT bank, uchar *SNum);
SMALLINT hasExtraInfoNV(SMALLINT bank, uchar *SNum);
SMALLINT getExtraInfoLengthNV(SMALLINT bank, uchar *SNum);
char *getExtraInfoDescNV(SMALLINT bank, uchar *SNum);
SMALLINT getMaxPacketDataLengthNV(SMALLINT bank, uchar *SNum);
SMALLINT hasPageAutoCRCNV(SMALLINT bank, uchar *SNum);
SMALLINT canRedirectPageNV(SMALLINT bank, uchar *SNum);
SMALLINT canLockPageNV(SMALLINT bank, uchar *SNum);
SMALLINT canLockRedirectPageNV(SMALLINT bank, uchar *SNum);

