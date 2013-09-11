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
// weather.H - Include file for Weather station struct
// Version  2.00
//

// Typedefs
typedef struct temptag
{
	uchar dsdir[8];
	uchar ds1820[8];
	uchar ds2423[8];
	uchar ds2401[8][8];
	uchar ctrl[16];
   int   weather_a;
   int   weather_b;
   int   found_dir;
   int   found_1820;
   int   found_2423;
   int   found_2401;
   int   north;
} WeatherStruct;

// local function prototypes
int FindSetupWeather(int portnum, WeatherStruct *wet);
int SetupWet(int portnum, WeatherStruct *wet, int nor);
int ReadWet(int portnum, WeatherStruct *wet, float *temp, int *dir, double *revol);
int GetDir(int portnum, WeatherStruct *wet);
int TrueDir(int portnum, WeatherStruct *wet);
int ParseData(char *inbuf, int insize, uchar *outbuf, int maxsize);
