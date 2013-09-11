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


// general command defines 
#define READ_MEMORY_COMMAND 0xF0
#define WRITE_SCRATCHPAD_COMMAND 0x0F
#define READ_SCRATCHPAD_COMMAND 0xAA
#define COPY_SCRATCHPAD_COMMAND 0x55

// Local defines
#define SIZE_SHAEE 32
#define PAGE_LENGTH_SHAEE 32

// Local function definitions
SMALLINT readSHAEE(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                   SMALLINT rd_cont, uchar *buff,  int len);
SMALLINT writeSHAEE(SMALLINT bank, int portnum, uchar *SNum, int str_add, 
                    uchar *buff, int len);
SMALLINT readPageSHAEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                       SMALLINT rd_cont, uchar *buff);
SMALLINT readPageExtraSHAEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                            SMALLINT rd_cont, uchar *buff, uchar *extra);
SMALLINT readPageExtraCRCSHAEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                               uchar *read_buff, uchar *extra);
SMALLINT readPageCRCSHAEE(SMALLINT bank, int portnum, uchar *SNum, int page,
                          uchar *buff);
SMALLINT readPagePacketSHAEE(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                             SMALLINT rd_cont, uchar *buff, int *len);
SMALLINT readPagePacketExtraSHAEE(SMALLINT bank, int portnum, uchar *SNum, 
                                  int str_add, SMALLINT rd_cont, uchar *buff, 
                                  int *len, uchar *extra);
SMALLINT writePagePacketSHAEE(SMALLINT bank, int portnum, uchar *SNum, int str_add,
                              uchar *buff, int len);
SMALLINT getNumberPagesSHAEE(SMALLINT bank, uchar *SNum);
int getSizeSHAEE(SMALLINT bank, uchar *SNum);
SMALLINT getPageLengthSHAEE(SMALLINT bank, uchar *SNum);
int getStartingAddressSHAEE(SMALLINT bank, uchar *SNum);
char *getBankDescriptionSHAEE(SMALLINT bank, uchar *SNum);
SMALLINT isGeneralPurposeMemorySHAEE(SMALLINT bank, uchar *SNum);
SMALLINT isReadWriteSHAEE(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isWriteOnceSHAEE(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isReadOnlySHAEE(SMALLINT bank, int portnum, uchar *SNum);
SMALLINT isNonVolatileSHAEE(SMALLINT bank, uchar *SNum);
SMALLINT needsProgramPulseSHAEE(SMALLINT bank, uchar *SNum);
SMALLINT needsPowerDeliverySHAEE(SMALLINT bank, uchar *SNum);
SMALLINT hasExtraInfoSHAEE(SMALLINT bank, uchar *SNum);
SMALLINT getExtraInfoLengthSHAEE(SMALLINT bank, uchar *SNum);
char    *getExtraInfoDescSHAEE(SMALLINT bank, uchar *SNum);
SMALLINT getMaxPacketDataLengthSHAEE(SMALLINT bank, uchar *SNum);
SMALLINT hasPageAutoCRCSHAEE(SMALLINT bank, uchar *SNum);
SMALLINT canRedirectPageSHAEE(SMALLINT bank, uchar *SNum);
SMALLINT canLockPageSHAEE(SMALLINT bank, uchar *SNum);
SMALLINT canLockRedirectPageSHAEE(SMALLINT bank, uchar *SNum);

SMALLINT loadFirstSecret(int portnum, int addr, uchar *SNum, uchar *secret, int data_len);
SMALLINT computeNextSecret(int portnum, int addr, uchar *SNum, uchar *secret);
void     setBusMasterSecret(uchar *new_secret);
void     setChallenge(uchar *new_challenge);
void     returnBusMasterSecret(uchar *return_secret);

// Local functions
SMALLINT writeSpad(int portnum, int addr, uchar *out_buf, int len);
SMALLINT readSpad(int portnum, ushort *addr, uchar *es, uchar *data);
SMALLINT copySpad(int portnum, int addr, uchar *SNum, uchar *extra_buf,
                  uchar *memory);
SMALLINT ReadAuthPage(SMALLINT bank, int portnum, int addr, uchar *SNum,
                      uchar *data, uchar *extra);
void ComputeSHA(unsigned int *MT,long *A,long *B, long *C, long *D,long *E);

