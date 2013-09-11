//---------------------------------------------------------------------------
// Copyright (C) 2001 Dallas Semiconductor Corporation, All Rights Reserved.
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
// tm_04.c - contains functions for communication to the DS2404/DS1994/DS1427
//


#define TIME_FAM       0x04

// define status register bit locations
#define CCEInverse     0x20
#define ITEInverse     0x10
#define RTEInverse     0x08
#define CCF            0x04
#define ITF            0x02
#define RTF            0x01

// define control register bit locations
#define DSEL           0x80
#define STOPSTART      0x40
#define AUTOMAN        0x20
#define OSC            0x10
#define RO             0x08
#define WPC            0x04
#define WPI            0x02
#define WPR            0x01

// type structure to hold time/date 
typedef struct          
{
     ushort  second;
     ushort  minute;
     ushort  hour;
     ushort  day;
     ushort  month;
     ushort  year;
} timedate;

// Functions

// The "getter" functions 
SMALLINT getRTC(int, uchar *, timedate *);
SMALLINT getRTCA(int, uchar *, timedate *);
SMALLINT getControlRegisterBit(int, uchar *, int, SMALLINT *);
SMALLINT getStatusRegisterBit(int, uchar *, int, SMALLINT *);

// The "setters" functions
SMALLINT setRTC(int, uchar *, ulong, SMALLINT);
SMALLINT setRTCFromPC(int, uchar *, SMALLINT);
SMALLINT setOscillator(int, uchar *, SMALLINT);
SMALLINT setRTCA(int, uchar *, ulong, SMALLINT);
SMALLINT setRTCAFromPCOffset(int, uchar *, ulong, SMALLINT);
SMALLINT setRTCAEnable(int, uchar *, SMALLINT);
SMALLINT setWriteProtectionAndExpiration(int, uchar *, SMALLINT, SMALLINT);
SMALLINT setControlRegister(int, uchar *, SMALLINT, SMALLINT, SMALLINT, SMALLINT, SMALLINT, SMALLINT, SMALLINT, SMALLINT);
SMALLINT setStatusRegister(int, uchar *, SMALLINT, SMALLINT, SMALLINT);

// The "time conversion" functions
void getPCTime(timedate *);
void SecondsToDate(timedate *, ulong);
ulong DateToSeconds(timedate *);

// Utility functions
ulong uchar_to_bin(uchar *, int);
