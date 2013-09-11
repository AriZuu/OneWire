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
//--------------------------------------------------------------------------
//
//  humutil.h - header file for humachron functions.
//  Version 2.00
//


#include "ownet.h"

typedef struct
{
   int      sampleRate;
   int      startDelay;
   SMALLINT enableRollover;
   SMALLINT timeSync;
   SMALLINT tempEnabled;
   SMALLINT dataEnabled;
   SMALLINT highDataResolution;
   SMALLINT highTempAlarm;
   SMALLINT lowTempAlarm;
   SMALLINT highDataAlarm;
   SMALLINT lowDataAlarm;
   SMALLINT tempAlarm;
   SMALLINT highTempResolution;
   float    highTemp;
   float    lowTemp;
   float    highData;
   float    lowData;
} startMissionData;

typedef struct
{
   int      highTemp;
   int      lowTemp;
   uchar    configByte;
   SMALLINT hasHumidity;
   SMALLINT useHumidityCalibration;
   SMALLINT useTemperatureCalibration;
   SMALLINT useTempCalforHumidity;
   int      adDeviceBits;
   double   adReferenceVoltage;
   double   humCoeffA;
   double   humCoeffB;
   double   humCoeffC;
   double   tempCoeffA;
   double   tempCoeffB;
   double   tempCoeffC;
} configLog;

SMALLINT startMission(int portnum, uchar *SNum, startMissionData set,
                      configLog *config);
SMALLINT readDevice(int portnum, uchar *SNum, uchar *buffer, configLog *config);
SMALLINT getFlag (int portnum, uchar *SNum, int reg, uchar bitMask);
uchar readByte(int portnum, uchar *SNum, int memAddr);
void setFlag (int reg, uchar bitMask, SMALLINT flagValue, uchar *state);
void setTime(int timeReg, int hours, int minutes, int seconds,
             SMALLINT AMPM, uchar *state);
void setDate (int timeReg, int year, int month, int day, uchar *state);
SMALLINT stopMission(int portnum, uchar *SNum);
double decodeHumidity(uchar *data, int length, SMALLINT reverse, 
                      configLog config);
double getADVoltage(uchar *state, int length, SMALLINT reverse, 
                    configLog config);
SMALLINT loadMissionResults(int portnum, uchar *SNum, configLog config);
SMALLINT doADConvert (int portnum, uchar *SNum, uchar *state);
SMALLINT doTemperatureConvert(int portnum, uchar *SNum, uchar *state);
//void setADVoltage(double voltage, uchar *data, int length);
double decodeTemperature(uchar *data, int length, SMALLINT reverse, 
                         configLog config);

