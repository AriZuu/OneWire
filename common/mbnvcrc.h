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
SMALLINT readPageNVCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                       SMALLINT rd_cont, uchar *buff);
SMALLINT readPageExtraNVCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                            SMALLINT rd_cont, uchar *buff, uchar *extra);
SMALLINT readPageExtraCRCNVCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                               uchar *read_buff, uchar *extra);
SMALLINT readPageCRCNVCRC(SMALLINT bank, int portnum, uchar *SNum, int page,
                          uchar *buff);
SMALLINT readPagePacketExtraNVCRC(SMALLINT bank, int portnum, uchar *SNum, 
                                  int page, SMALLINT rd_cont, uchar *buff,
                                  int *len, uchar *extra);

// Internal functions
SMALLINT readCRC(SMALLINT bank, int portnum, uchar *SNum, int page, SMALLINT rd_cont,
                 uchar *readBuf, uchar *extra, int extra_len);
SMALLINT readPossible(SMALLINT bank, uchar *SNum);
SMALLINT numVerifyBytes(SMALLINT bank, uchar *SNum);


