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
//  atod20.h - header file for the Module(s) to do conversion, setup, read, 
//             and write on the DS2450 - 1-Wire Quad A/D Converter.
// --------------------------------------------------------------------------

// functions defined in atod20.c
int SetupAtoDControl(int,uchar *,uchar *,char *);
int WriteAtoD(int,int,uchar *,uchar *,int,int);
int DoAtoDConversion(int,int,uchar *);
int ReadAtoDResults(int,int,uchar *,float *,uchar *);
int Select(int,int);

// family codes of device(s)
#define ATOD_FAM           0x20

// setup
#define NO_OUTPUT          0x00
#define OUTPUT_ON          0x80
#define OUTPUT_OFF         0xC0
#define RANGE_512          0x01
#define RANGE_256          0x00
#define ALARM_HIGH_ENABLE  0x08
#define ALARM_LOW_ENABLE   0x04
#define ALARM_HIGH_DISABLE 0x00
#define ALARM_LOW_DISABLE  0x00
#define BITS_16            0x00
#define BITS_8             0x08
