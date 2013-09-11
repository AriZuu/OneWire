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
// ps02.c - contains functions for communication to the DS1991
//

/**
 * DS1991 Write Scratchpad Command
 */
#define WRITE_SCRATCHPAD_COMMAND  0x96;

/**
 * DS1991 Read Scratchpad Command
 */
#define READ_SCRATCHPAD_COMMAND  0x69;

/**
 * DS1991 Copy Scratchpad Command
 */
#define COPY_SCRATCHPAD_COMMAND  0x3C;

/**
 * DS1991 Write Password Command
 */
#define WRITE_PASSWORD_COMMAND  0x5A;

/**
 * DS1991 Write SubKey Command
 */
#define WRITE_SUBKEY_COMMAND  0x99;

/**
 * DS1991 Read SubKey Command
 */
#define READ_SUBKEY_COMMAND  0x66;


// functions
void initBlockCodes(void);
SMALLINT writeScratchpad (int portnum, int addr, uchar *data, int len);
SMALLINT readScratchpad(int portnum, uchar *data);
SMALLINT copyScratchpad(int portnum, int key, uchar *passwd, int blockNum);
SMALLINT readSubkey(int portnum, uchar *data, int key, uchar *passwd);
SMALLINT writePassword(int portnum, int key, uchar *oldName, 
                       uchar *newName, uchar *newPasswd);
SMALLINT writeSubkey (int portnum, int key, int addr, uchar *passwd,
                      uchar *data, int len);



