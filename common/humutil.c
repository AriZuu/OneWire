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
//  humutil.c - functions to set mission and mission states for DS1922.
//  Version 2.00
//

#include <time.h>
#include "math.h"
#include "humutil.h"
#include "mbee77.h"
#include "pw77.h"

// Temperature resolution in degrees Celsius
double temperatureResolution = 0.5;

// max and min temperature
double maxTemperature = 85, minTemperature = -40;

// should we update the Real time clock?
SMALLINT updatertc = FALSE;


#define MAX_READ_RETRY_CNT  15

// *****************************************************************************
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Register addresses and control bits
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *****************************************************************************

/** Address of the Real-time Clock Time value*/
#define RTC_TIME  0x200
/** Address of the Real-time Clock Date value*/
#define RTC_DATE  0x203

/** Address of the Sample Rate Register */
#define SAMPLE_RATE  0x206 // 2 bytes, LSB first, MSB no greater than 0x3F

/** Address of the Temperature Low Alarm Register */
#define TEMPERATURE_LOW_ALARM_THRESHOLD  0x208
/** Address of the Temperature High Alarm Register */
#define TEMPERATURE_HIGH_ALARM_THRESHOLD  0x209

/** Address of the Data Low Alarm Register */
#define DATA_LOW_ALARM_THRESHOLD  0x20A
/** Address of the Data High Alarm Register */
#define DATA_HIGH_ALARM_THRESHOLD  0x20B

/** Address of the last temperature conversion's LSB */
#define LAST_TEMPERATURE_CONVERSION_LSB  0x20C
/** Address of the last temperature conversion's MSB */
#define LAST_TEMPERATURE_CONVERSION_MSB  0x20D

/** Address of the last data conversion's LSB */
#define LAST_DATA_CONVERSION_LSB  0x20E
/** Address of the last data conversion's MSB */
#define LAST_DATA_CONVERSION_MSB  0x20F

/** Address of Temperature Control Register */
#define TEMPERATURE_CONTROL_REGISTER  0x210
/** Temperature Control Register Bit: Enable Data Low Alarm */
#define TCR_BIT_ENABLE_TEMPERATURE_LOW_ALARM  0x01
/** Temperature Control Register Bit: Enable Data Low Alarm */
#define TCR_BIT_ENABLE_TEMPERATURE_HIGH_ALARM  0x02

/** Address of Data Control Register */
#define DATA_CONTROL_REGISTER  0x211
/** Data Control Register Bit: Enable Data Low Alarm */
#define DCR_BIT_ENABLE_DATA_LOW_ALARM  0x01
/** Data Control Register Bit: Enable Data High Alarm */
#define DCR_BIT_ENABLE_DATA_HIGH_ALARM  0x02

/** Address of Real-Time Clock Control Register */
#define RTC_CONTROL_REGISTER  0x212
/** Real-Time Clock Control Register Bit: Enable Oscillator */
#define RCR_BIT_ENABLE_OSCILLATOR  0x01
/** Real-Time Clock Control Register Bit: Enable High Speed Sample */
#define RCR_BIT_ENABLE_HIGH_SPEED_SAMPLE  0x02

/** Address of Mission Control Register */
#define MISSION_CONTROL_REGISTER  0x213
/** Mission Control Register Bit: Enable Temperature Logging */
#define MCR_BIT_ENABLE_TEMPERATURE_LOGGING  0x01
/** Mission Control Register Bit: Enable Data Logging */
#define MCR_BIT_ENABLE_DATA_LOGGING  0x02
/** Mission Control Register Bit: Set Temperature Resolution */
#define MCR_BIT_TEMPERATURE_RESOLUTION  0x04
/** Mission Control Register Bit: Set Data Resolution */
#define MCR_BIT_DATA_RESOLUTION  0x08
/** Mission Control Register Bit: Enable Rollover */
#define MCR_BIT_ENABLE_ROLLOVER  0x10
/** Mission Control Register Bit: Start Mission on Temperature Alarm */
#define MCR_BIT_START_MISSION_ON_TEMPERATURE_ALARM  0x20

/** Address of Alarm Status Register */
#define ALARM_STATUS_REGISTER  0x214
/** Alarm Status Register Bit: Temperature Low Alarm */
#define ASR_BIT_TEMPERATURE_LOW_ALARM  0x01
/** Alarm Status Register Bit: Temperature High Alarm */
#define ASR_BIT_TEMPERATURE_HIGH_ALARM  0x02
/** Alarm Status Register Bit: Data Low Alarm */
#define ASR_BIT_DATA_LOW_ALARM  0x04
/** Alarm Status Register Bit: Data High Alarm */
#define ASR_BIT_DATA_HIGH_ALARM  0x08
/** Alarm Status Register Bit: Battery On Reset */
#define ASR_BIT_BATTERY_ON_RESET  0x80

/** Address of General Status Register */
#define GENERAL_STATUS_REGISTER  0x215
/** General Status Register Bit: Sample In Progress */
#define GSR_BIT_SAMPLE_IN_PROGRESS  0x01
/** General Status Register Bit: Mission In Progress */
#define GSR_BIT_MISSION_IN_PROGRESS  0x02
/** General Status Register Bit: Conversion In Progress */
#define GSR_BIT_CONVERSION_IN_PROGRESS  0x04
/** General Status Register Bit: Memory Cleared */
#define GSR_BIT_MEMORY_CLEARED  0x08
/** General Status Register Bit: Waiting for Temperature Alarm */
#define GSR_BIT_WAITING_FOR_TEMPERATURE_ALARM  0x10
/** General Status Register Bit: Forced Conversion In Progress */
#define GSR_BIT_FORCED_CONVERSION_IN_PROGRESS  0x20

/** Address of the Mission Start Delay */
#define MISSION_START_DELAY  0x216 // 3 bytes, LSB first

/** Address of the Mission Timestamp Time value*/
#define MISSION_TIMESTAMP_TIME  0x219
/** Address of the Mission Timestamp Date value*/
#define MISSION_TIMESTAMP_DATE  0x21C

/** Address of Device Configuration Register */
#define DEVICE_CONFIGURATION_BYTE  0x226

// 1 byte, alternating ones and zeroes indicates passwords are enabled
/** Address of the Password Control Register. */
#define PASSWORD_CONTROL_REGISTER  0x227

// 8 bytes, write only, for setting the Read Access Password
/** Address of Read Access Password. */
#define READ_ACCESS_PASSWORD  0x228

// 8 bytes, write only, for setting the Read Access Password
/** Address of the Read Write Access Password. */
#define READ_WRITE_ACCESS_PASSWORD  0x230

// 3 bytes, LSB first
/** Address of the Mission Sample Count */
#define MISSION_SAMPLE_COUNT  0x220

// 3 bytes, LSB first
/** Address of the Device Sample Count */
#define DEVICE_SAMPLE_COUNT  0x223

// first year that calendar starts counting years from
#define FIRST_YEAR_EVER  2000

// maximum size of the mission log
#define MISSION_LOG_SIZE  8192

// mission log size for odd combination of resolutions (i.e. 8-bit temperature
// & 16-bit data or 16-bit temperature & 8-bit data
#define ODD_MISSION_LOG_SIZE  7680

#define TEMPERATURE_CHANNEL  0
#define DATA_CHANNEL         1

/** 1-Wire command for Clear Memory With Password */
#define CLEAR_MEMORY_PW_COMMAND     0x96
/** 1-Wire command for Start Mission With Password */
#define START_MISSION_PW_COMMAND    0xCC
/** 1-Wire command for Stop Mission With Password */
#define STOP_MISSION_PW_COMMAND     0x33
/** 1-Wire command for Forced Conversion */
#define FORCED_CONVERSION           0x55

// *****************************************************************************
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Sensor read/write
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *****************************************************************************

/**
 * Retrieves the 1-Wire device sensor state.  This state is
 * returned as a byte array.  Pass this byte array to the 'get'
 * and 'set' methods.  If the device state needs to be changed then call
 * the 'writeDevice' to finalize the changes.
 *
 * @return 1-Wire device sensor state
 */
SMALLINT readDevice(int portnum, uchar *SNum, uchar *buffer, configLog *config)
{
   int i;
   int retryCnt = MAX_READ_RETRY_CNT;
   uchar temp_buff[96];
   // reference humidities that the calibration was calculated over
   double ref1 = 20.0, ref2 = 60.0, ref3 = 90.0;
   // the average value for each reference point
   double read1 = 0.0, read2 = 0.0, read3 = 0.0;
   // the average error for each reference point
   double error1 = 0.0, error2 = 0.0, error3 = 0.0;
   double ref1sq, ref2sq, ref3sq;

   config->adDeviceBits = 10;
   config->adReferenceVoltage = 5.02;

   do
   {
      if(readPageCRCEE77(0,portnum,SNum,16,&temp_buff[0]) &&
         readPageCRCEE77(0,portnum,SNum,17,&temp_buff[32]) &&
         readPageCRCEE77(0,portnum,SNum,18,&temp_buff[64]))
      {
         for(i=0;i<96;i++)
            buffer[i] = temp_buff[i];
      }
      else
      {
         retryCnt++;
      }
   }
   while(retryCnt<MAX_READ_RETRY_CNT);

   switch(config->configByte)
   {
      case 0x00:
         config->lowTemp  = -40;
         config->highTemp = 125;
         break;

      case 0x20:
         config->lowTemp  = -40;
         config->highTemp = 125;
         config->hasHumidity = TRUE;
         break;
      
      case 0x40:
         config->lowTemp  = -40;
         config->highTemp = 125;
         break;

      case 0x60:
         config->lowTemp  = 0;
         config->highTemp = 125;
         break;

      default:
         config->lowTemp  = -40;
         config->highTemp = 125;
         break;
   }

   if(config->hasHumidity)
   {
      config->useHumidityCalibration = TRUE;

      ref1 = decodeHumidity(&buffer[72],2,TRUE,*config);
      read1 = decodeHumidity(&buffer[74],2,TRUE,*config);
      error1 = read1 - ref1;
      ref2 = decodeHumidity(&buffer[76],2,TRUE,*config);
      read2 = decodeHumidity(&buffer[78],2,TRUE,*config);
      error2 = read2 - ref2;
      ref3 = decodeHumidity(&buffer[80],2,TRUE,*config);
      read3 = decodeHumidity(&buffer[82],2,TRUE,*config);
      error3 = read3 - ref3;

      ref1sq = ref1*ref1;
      ref2sq = ref2*ref2;
      ref3sq = ref3*ref3;

      config->humCoeffB =
         ( (ref2sq-ref1sq)*(error3-error1) + ref3sq*(error1-error2)
           + ref1sq*(error2-error1) ) /
         ( (ref2sq-ref1sq)*(ref3-ref1) + (ref3sq-ref1sq)*(ref1-ref2) );
      config->humCoeffA =
         ( error2 - error1 + config->humCoeffB*(ref1-ref2) ) /
         ( ref2sq - ref1sq );
      config->humCoeffC = 
         error1 - config->humCoeffA*ref1sq - config->humCoeffB*ref1;
   }

   config->useTemperatureCalibration = TRUE;

   ref2 = decodeHumidity(&buffer[64],2,TRUE,*config);
   read2 = decodeHumidity(&buffer[66],2,TRUE,*config);
   error2 = read2 - ref2;
   ref3 = decodeHumidity(&buffer[68],2,TRUE,*config);
   read3 = decodeHumidity(&buffer[70],2,TRUE,*config);
   error3 = read3 - ref3;
   ref1 = 60.0;
   error1 = error2;
   read1 = ref1 + error1;

   ref1sq = ref1*ref1;
   ref2sq = ref2*ref2;
   ref3sq = ref3*ref3;

   config->tempCoeffB =
      ( (ref2sq-ref1sq)*(error3-error1) + ref3sq*(error1-error2)
        + ref1sq*(error2-error1) ) /
      ( (ref2sq-ref1sq)*(ref3-ref1) + (ref3sq-ref1sq)*(ref1-ref2) );
   config->tempCoeffA =
      ( error2 - error1 + config->tempCoeffB*(ref1-ref2) ) /
      ( ref2sq - ref1sq );
   config->tempCoeffC =
         error1 - config->tempCoeffA*ref1sq - config->tempCoeffB*ref1;

   config->useTempCalforHumidity = FALSE;

   return TRUE;
}

/**
 * Writes the 1-Wire device sensor state that
 * have been changed by 'set' methods.  Only the state registers that
 * changed are updated.  This is done by referencing a field information
 * appended to the state data.
 *
 * @param  state 1-Wire device sensor state
 *
 */
SMALLINT writeDevice(int portnum, uchar *SNum, uchar *state, SMALLINT updatertc)
{
   int start = 0;
   int str_add = 0;

   if(getFlag(portnum, SNum, GENERAL_STATUS_REGISTER, GSR_BIT_MISSION_IN_PROGRESS))
   {
      printf("get flag error.\n");
      return FALSE;
   }

   start = updatertc ? 0 : 6;
   str_add = start + 512;

   if(!writeEE77(0,portnum,SNum, str_add,&state[start],(64-start)))
   {
      printf("writing error.\n");
      return FALSE;
   }

   return TRUE;
}

/**
 * Reads a single byte from the DS1922.  Note that the preferred manner
 * of reading from the DS1922 Thermocron is through the <code>readDevice()</code>
 * method or through the <code>MemoryBank</code> objects returned in the
 * <code>getMemoryBanks()</code> method.
 *
 * memAddr the address to read from  (in the range of 0x200-0x21F)
 */
uchar readByte(int portnum, uchar *SNum, int memAddr)
{
   uchar buffer[32];
   int page;

   page = memAddr/32;

   readPageCRCEE77(0,portnum,SNum,page,&buffer[0]);

   return buffer[memAddr%32];
}


/**
 * Gets the status of the specified flag from the specified register.
 * This method actually communicates with the Thermocron.  To improve
 * performance if you intend to make multiple calls to this method,
 * first call readDevice() and use the
 * getFlag(int, byte, byte[]) method instead.</p>
 *
 * The DS1922 Thermocron has two sets of flags.  One set belongs
 * to the control register.  When reading from the control register,
 * valid values for bitMask.
 *
 * @param register address of register containing the flag (valid values
 * are CONTROL_REGISTER and STATUS_REGISTER)
 * @param bitMask the flag to read (see above for available options)
 *
 * @return the status of the flag, where true
 * signifies a "1" and false signifies a "0"
 */
SMALLINT getFlag (int portnum, uchar *SNum, int reg, uchar bitMask)
{
   return ((readByte(portnum,SNum,reg) & bitMask) != 0);
}

/**
 * <p>Sets the status of the specified flag in the specified register.
 * If a mission is in progress a <code>OneWireIOException</code> will be thrown
 * (one cannot write to the registers while a mission is commencing).  This method
 * is the preferred manner of setting the DS1922 status and control flags.
 * The method <code>writeDevice()</code> must be called to finalize
 * changes to the device.  Note that multiple 'set' methods can
 * be called before one call to <code>writeDevice()</code>.</p>
 *
 * <p>For more information on valid values for the <code>bitMask</code>
 * parameter, see the {@link #getFlag(int,byte) getFlag(int,byte)} method.</p>
 *
 * @param register address of register containing the flag (valid values
 * are <code>CONTROL_REGISTER</code> and <code>STATUS_REGISTER</code>)
 * @param bitMask the flag to read (see {@link #getFlag(int,byte) getFlag(int,byte)}
 * for available options)
 * @param flagValue new value for the flag (<code>true</code> is logic "1")
 * @param state current state of the device returned from <code>readDevice()</code>
 */
void setFlag (int reg, uchar bitMask, SMALLINT flagValue, uchar *state)
{
   uchar flags;

   reg = reg&0x3F;

   flags = state[reg];

   if (flagValue)
      flags = (uchar)(flags | bitMask);
   else
      flags = (uchar)(flags & ~(bitMask));

   // write the regs back
   state[reg] = flags;
}

// *****************************************************************************
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// DS1922 Device Specific Functions
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *****************************************************************************

/**
 * Stops the currently running mission.
 *
 */
SMALLINT stopMission(int portnum, uchar *SNum)
{
   int i;
   uchar buffer[10];

   if (!getFlag(portnum, SNum, GENERAL_STATUS_REGISTER, GSR_BIT_MISSION_IN_PROGRESS))
   {
      OWERROR(OWERROR_HYGRO_STOP_MISSION_UNNECESSARY);
      return FALSE;
   }
            
   if(owAccess(portnum))
   {
      buffer[0] = STOP_MISSION_PW_COMMAND;
      for(i=0;i<8;i++)
         buffer[i+1] = psw[i];
      buffer[9] = 0xFF;

      if(!owBlock(portnum,FALSE,&buffer[0],10))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }
   }

   if(getFlag(portnum, SNum, GENERAL_STATUS_REGISTER, GSR_BIT_MISSION_IN_PROGRESS))
   {
      OWERROR(OWERROR_HYGRO_STOP_MISSION_ERROR);
      return FALSE;
   }

   return TRUE;
}


/**
 * Erases the log memory from this missioning device.
 */
SMALLINT clearMemory(int portnum, uchar *SNum)
{
   int i;
   uchar buffer[10];

   if(owAccess(portnum))
   {
      buffer[0] = CLEAR_MEMORY_PW_COMMAND;
      for(i=0;i<8;i++)
         buffer[i+1] = psw[i];
      buffer[9] = 0xFF;

      if(!owBlock(portnum,FALSE,&buffer[0],10))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

      // wait 2 ms for Clear Memory to comlete
      msDelay(2);

      if(!getFlag(portnum, SNum, GENERAL_STATUS_REGISTER, GSR_BIT_MEMORY_CLEARED))
      {
         printf("error.\n");//       "OneWireContainer41-Clear Memory failed.  Check read/write password.");
         return FALSE;
      }
   }

   return TRUE;
}
   
// *****************************************************************************
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Mission Interface Functions
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *****************************************************************************


/**
 * Begins a new mission on this missioning device.
 *
 * @param sampleRate indicates the sampling rate, in seconds, that
 *        this missioning device should log samples.
 * @param missionStartDelay indicates the amount of time, in seconds,
 *        that should pass before the mission begins.
 * @param rolloverEnabled if <code>false</code>, this device will stop
 *        recording new samples after the data log is full.  Otherwise,
 *        it will replace samples starting at the beginning.
 * @param syncClock if <code>true</code>, the real-time clock of this
 *        missioning device will be synchronized with the current time
 *        according to this <code>java.util.Date</code>.
 * @param startUponTempAlarm if <code>true</code>, the mission will begin
 *        after a temperature alarm has occurred.
 */
SMALLINT startMission(int portnum, uchar *SNum, startMissionData set, 
                      configLog *config)
{
   int i;
   time_t tlong;
   struct tm *tstruct;
   uchar buffer[10];
   uchar state[96];
   int sampleRate;

   sampleRate = set.sampleRate;

   if(getFlag(portnum,SNum,GENERAL_STATUS_REGISTER,GSR_BIT_MISSION_IN_PROGRESS))
   {
      printf("get flag error.\n");
      return FALSE;
   }
   else
   {
      if(!readDevice(portnum,SNum,&state[0],config))
      {
         printf("read device error.\n");
         return FALSE;
      }
   }

   setFlag(MISSION_CONTROL_REGISTER, MCR_BIT_ENABLE_TEMPERATURE_LOGGING,
           set.tempEnabled, &state[0]);
   setFlag(MISSION_CONTROL_REGISTER, MCR_BIT_ENABLE_DATA_LOGGING,
           set.dataEnabled, &state[0]);
   setFlag(MISSION_CONTROL_REGISTER,MCR_BIT_TEMPERATURE_RESOLUTION, 
           set.highTempResolution, &state[0]);
   setFlag(MISSION_CONTROL_REGISTER, MCR_BIT_DATA_RESOLUTION, 
           set.highDataResolution, &state[0]);

   if(set.tempAlarm)
   {
      setFlag(MISSION_CONTROL_REGISTER,MCR_BIT_START_MISSION_ON_TEMPERATURE_ALARM,
         set.tempAlarm,&state[0]);
   }
   else
   {
      setFlag(MISSION_CONTROL_REGISTER,MCR_BIT_START_MISSION_ON_TEMPERATURE_ALARM,
         set.tempAlarm,&state[0]);
   }

   if(set.highTempAlarm)
   {
      setFlag(TEMPERATURE_CONTROL_REGISTER, TCR_BIT_ENABLE_TEMPERATURE_HIGH_ALARM,
              set.highTempAlarm, &state[0]);
      state[TEMPERATURE_HIGH_ALARM_THRESHOLD&0x3F] = (uchar) ((set.highTemp*2)+82);
   }
   else
   {
      setFlag(TEMPERATURE_CONTROL_REGISTER, TCR_BIT_ENABLE_TEMPERATURE_HIGH_ALARM,
              set.highTempAlarm, &state[0]);
   }

   if(set.lowTempAlarm)
   {
      setFlag(TEMPERATURE_CONTROL_REGISTER, TCR_BIT_ENABLE_TEMPERATURE_LOW_ALARM,
              set.lowTempAlarm, &state[0]);
      state[TEMPERATURE_LOW_ALARM_THRESHOLD&0x3F] = (uchar) ((set.lowTemp*2)+82);
   }
   else
   {
      setFlag(TEMPERATURE_CONTROL_REGISTER, TCR_BIT_ENABLE_TEMPERATURE_LOW_ALARM,
              set.lowTempAlarm, &state[0]);
   }

   if(set.highDataAlarm)
   {
      setFlag(DATA_CONTROL_REGISTER, DCR_BIT_ENABLE_DATA_HIGH_ALARM,
         set.highDataAlarm, &state[0]); 

      state[DATA_HIGH_ALARM_THRESHOLD&0x3F] = (uchar) (((int)(((((((1 - config->humCoeffB)
                 - sqrt( ((config->humCoeffB - 1)*(config->humCoeffB - 1))
                 - 4*config->humCoeffA*(config->humCoeffC + set.highData) ) )
                 / (2*config->humCoeffA))*.0307) + .958)*((1<<config->adDeviceBits)-1))/
                 config->adReferenceVoltage)&0x3FC)>>(config->adDeviceBits-8));
   }
   else
   {
      setFlag(DATA_CONTROL_REGISTER, DCR_BIT_ENABLE_DATA_HIGH_ALARM,
         set.highDataAlarm, &state[0]); 
   }

   if(set.lowDataAlarm)
   {
      setFlag(DATA_CONTROL_REGISTER, DCR_BIT_ENABLE_DATA_LOW_ALARM,
         set.lowDataAlarm, &state[0]); 

      state[DATA_LOW_ALARM_THRESHOLD&0x3F] = (uchar) (((int)(((((((1 - config->humCoeffB)
                 - sqrt( ((config->humCoeffB - 1)*(config->humCoeffB - 1))
                 - 4*config->humCoeffA*(config->humCoeffC + set.lowData) ) )
                 / (2*config->humCoeffA))*.0307) + .958)*((1<<config->adDeviceBits)-1))/
                 config->adReferenceVoltage)&0x3FC)>>(config->adDeviceBits-8));
   }
   else
   {
      setFlag(DATA_CONTROL_REGISTER, DCR_BIT_ENABLE_DATA_LOW_ALARM,
         set.lowDataAlarm, &state[0]);
   }

   if(sampleRate%60==0 || sampleRate>0x03FFF)
   {
      //convert to minutes
      sampleRate = (sampleRate/60) & 0x03FFF;
      setFlag(RTC_CONTROL_REGISTER, RCR_BIT_ENABLE_HIGH_SPEED_SAMPLE, FALSE, &state[0]);
   }
   else
   {
      setFlag(RTC_CONTROL_REGISTER, RCR_BIT_ENABLE_HIGH_SPEED_SAMPLE, TRUE, &state[0]);
   }

   state[6] = (uchar) (sampleRate & 0x00FF);
   state[7] = (uchar) ((sampleRate & 0x3F00) >> 8);

   state[22] = (uchar) (set.startDelay & 0x00FF);
   state[23] = (uchar) ((set.startDelay & 0x3F00) >> 8);

   setFlag(MISSION_CONTROL_REGISTER,
           MCR_BIT_ENABLE_ROLLOVER, set.enableRollover, &state[0]);

   if(set.timeSync)
   {
      time(&tlong);
      tlong++;

      tstruct = localtime(&tlong);

      setTime(RTC_TIME&0x3F, tstruct->tm_hour, tstruct->tm_min, tstruct->tm_sec,
              FALSE, &state[0]);

      setDate(RTC_DATE&0x3F, tstruct->tm_year, (tstruct->tm_mon + 1), 
              tstruct->tm_mday, &state[0]);

      if(!getFlag(portnum, SNum, RTC_CONTROL_REGISTER, 
                  RCR_BIT_ENABLE_OSCILLATOR))
      {
         setFlag(RTC_CONTROL_REGISTER, RCR_BIT_ENABLE_OSCILLATOR, TRUE, &state[0]);
      }
   }
   else if(!getFlag(portnum, SNum, RTC_CONTROL_REGISTER, 
                    RCR_BIT_ENABLE_OSCILLATOR))
   {
      //System.out.println("Enabling Oscillator");
      setFlag(RTC_CONTROL_REGISTER, RCR_BIT_ENABLE_OSCILLATOR, TRUE, &state[0]);
   }

   //System.out.println("Clearing memory");
   clearMemory(portnum,SNum);

   //System.out.println("Updating Device state");
   if(!writeDevice(portnum,SNum,state,TRUE))
   {
      return FALSE;
   }

   //System.out.println("Starting mission");
   if(owAccess(portnum))
   {
      buffer[0] = START_MISSION_PW_COMMAND;
      for(i=0;i<8;i++)
         buffer[i+1] = psw[i];
      buffer[9] = 0xFF;

      if(!owBlock(portnum,FALSE,&buffer[0],10))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }
   }

   return TRUE;
}

/**
 * Loads the results of the currently running mission.  Must be called
 * before all mission result/status methods.
 */
SMALLINT loadMissionResults(int portnum, uchar *SNum, configLog config)
{
   uchar state[96];
   uchar page[8192];
   configLog tempConfig;
   uchar upper, lower;
   double val,valsq,error,humCal,tempVal;
   int timeOffset;
   int month,day,year;
   int hour,min,sec;
   int tempone,temptwo,tempthr;
   int pm = 0, i;
   int sampleCnt = 0;
   int sampleRate = 0;
   int tempBytes = 0;
   int dataBytes = 0;
   int maxSamples = 0;
   int wrapCount = 0;
   int offsetDepth = 0;
   int temperatureLogSize = 0;
   // default size of the log, could be different if using an odd
   // sample size combination.
   int logSize;
   // check for rollover

   FILE * fidTemp;
   FILE * fidData;

   // read the register contents
   if(!readDevice(portnum,SNum,&state[0],&tempConfig))
   {
      printf("read device error.\n");
      return FALSE;
   }

   // get the number of samples
   tempone = (int)(state[32]&0x0000FF);
   temptwo = (int)(state[33]<<8)&0x00FF00;
   tempthr = (int)(state[34]<<16)&0xFF0000;
   sampleCnt = tempone + temptwo + tempthr;

   if(sampleCnt == 0)
   {
      printf("No, samples yet for this mission.\n");
      return FALSE;
   }

   // sample rate, in seconds
   tempone = (int)(state[SAMPLE_RATE&0x3F]&0x00FF);
   temptwo = (int)((state[(SAMPLE_RATE&0x3F)+1]<<8)&0xFF00);
   sampleRate = tempone + temptwo;
   if(!getFlag(portnum,SNum,RTC_CONTROL_REGISTER,RCR_BIT_ENABLE_HIGH_SPEED_SAMPLE))
      // if sample rate is in minutes, convert to seconds
      sampleRate = sampleRate*60;

   // get day
   lower  = state[MISSION_TIMESTAMP_DATE&0x3F];
   upper  = ((lower >> 4) & 0x0f);
   lower  = (lower & 0x0f);
   day    = upper*10 + lower;

   // get month
   lower = state[(MISSION_TIMESTAMP_DATE&0x3F) + 1];
   upper = ((lower >> 4) & 0x01);
   lower = (lower & 0x0f);
   month = upper*10 + lower;

   // get year
   if((state[(MISSION_TIMESTAMP_DATE&0x3F) + 1]&0x80)==0x80)
      year = 100;
   lower = state[(MISSION_TIMESTAMP_DATE&0x3F) + 2];
   upper = ((lower >> 4) & 0x0f);
   lower = (lower & 0x0f);
   year  = upper*10 + lower + FIRST_YEAR_EVER + year;

   // get seconds
   lower = state[MISSION_TIMESTAMP_TIME&0x3F];
   upper = ((lower >> 4) & 0x07);
   lower = (lower & 0x0f);
   sec   = (int) lower + (int) upper * 10;

   // get minutes
   lower = state [(MISSION_TIMESTAMP_TIME&0x3F) + 1];
   upper = ((lower >> 4) & 0x07);
   lower = (lower & 0x0f);
   min   = (int) lower + (int) upper * 10;

   // get hours
   lower = state[(MISSION_TIMESTAMP_TIME&0x3F) + 2];
   upper = ((lower >> 4) & 0x07);
   lower = (lower & 0x0f);
   if ((upper&0x04) != 0)
   {
      // extract the AM/PM byte (PM is indicated by a 1)
      if((upper&0x02)>0)
         pm = 12;

      // isolate the 10s place
      upper &= 0x01;
   }
   hour = (int)(upper*10) + (int)lower + (int)pm;

   // figure out how many bytes for each temperature sample
   // if it's being logged, add 1 to the size
   if(getFlag(portnum,SNum,MISSION_CONTROL_REGISTER,
              MCR_BIT_ENABLE_TEMPERATURE_LOGGING))
   {
      tempBytes += 1;
      // if it's 16-bit resolution, add another 1 to the size
      if(getFlag(portnum,SNum,MISSION_CONTROL_REGISTER,
                 MCR_BIT_TEMPERATURE_RESOLUTION))
         tempBytes += 1;
   }


   // figure out how many bytes for each data sample
   // if it's being logged, add 1 to the size
   if(getFlag(portnum,SNum,MISSION_CONTROL_REGISTER,
              MCR_BIT_ENABLE_DATA_LOGGING))
   {
      dataBytes += 1;
      // if it's 16-bit resolution, add another 1 to the size
      if(getFlag(portnum,SNum,MISSION_CONTROL_REGISTER,
                 MCR_BIT_DATA_RESOLUTION))
         dataBytes += 1;
   }

   // figure max number of samples
   switch(tempBytes + dataBytes)
   {
      case 1:
         maxSamples = 8192;
         logSize = MISSION_LOG_SIZE;
         break;
      case 2:
         maxSamples = 4096;
         logSize = MISSION_LOG_SIZE;
         break;
      case 3:
         maxSamples = 2560;
         logSize = ODD_MISSION_LOG_SIZE;
         break;
      case 4:
         maxSamples = 2048;
         logSize = MISSION_LOG_SIZE;
         break;
      default:
      case 0:
         // assert! should never, ever get here
         break;
   }

   if( getFlag(portnum,SNum,MISSION_CONTROL_REGISTER,MCR_BIT_ENABLE_ROLLOVER)
       && (sampleCnt>maxSamples) )// intentional assignment
   {
      wrapCount = sampleCnt / maxSamples;
      offsetDepth = sampleCnt % maxSamples;
      sampleCnt = maxSamples;
   }


   //DEBUG: For bad SOICS
   if(!getFlag(portnum,SNum,MISSION_CONTROL_REGISTER,MCR_BIT_ENABLE_ROLLOVER)
      && (sampleCnt>maxSamples))
   {
      printf("Device Error: rollover was not enabled, but it did occur.\n");
      return FALSE;
   }

   // figure out where the temperature bytes end, that's where
   // the data bytes begin
   temperatureLogSize = tempBytes * maxSamples;

   // calculate first log entry time offset, in seconds
   timeOffset = ((wrapCount * maxSamples) + offsetDepth) * sampleRate;

   if((tempBytes*sampleCnt)>0)
   {
      for(i=0;i<((tempBytes*sampleCnt/32)+((tempBytes*sampleCnt%32)>0?1:0));i++)
      {
         if(!readPageCRCEE77(2,portnum,SNum,i+128,&page[i*32]))
            return FALSE;
      }

      fidTemp = fopen("temp.log","a+");

      fprintf(fidTemp,"mission start date and time: %d/%d/%d %d:%d:%d \n",month,
              day,year,hour,min,sec);
      fprintf(fidTemp,"time offset(s), temperature reading(C)\n");

      for(i=0;i<sampleCnt;i++)
      {
         fprintf(fidTemp,"%d, ",timeOffset + i*sampleRate);
         if(config.useTemperatureCalibration)
         {
            val = decodeTemperature(&page[(offsetDepth+i*tempBytes)%maxSamples],
               tempBytes,TRUE,config);
            valsq = val*val;
            error = config.tempCoeffA*valsq + config.tempCoeffB*val + config.tempCoeffC;
            val = val - error;
            fprintf(fidTemp,"%f \n",val);
         }
         else
         {
            fprintf(fidTemp,"%f \n",decodeTemperature(&page[(offsetDepth+i*tempBytes)%maxSamples],
               tempBytes,TRUE,config));
         }
      }
      fflush(fidTemp);
      fclose(fidTemp);
      printf("The temperature data is in temp.log file.\n");
   }
   



   if((sampleCnt*dataBytes)>0)
   {
      for(i=0; i<((dataBytes*sampleCnt/32)+((dataBytes*sampleCnt%32)>0?1:0));i++)
      {
         if(!readPageCRCEE77(2,portnum,SNum,((temperatureLogSize/32)+i+128),
            &page[temperatureLogSize+i*32]))
            return FALSE;
      }

      fidData = fopen("data.log","a+");

      fprintf(fidData,"mission start date and time: %d/%d/%d %d:%d:%d \n",month,
              day,year,hour,min,sec);
      fprintf(fidData,"time offset(s), data reading(percentage humidity or Volts)\n");

      for(i=0;i<sampleCnt;i++)
      {
         fprintf(fidData,"%d, ",timeOffset + i*sampleRate);
         if(config.hasHumidity)
         {
            if(config.useHumidityCalibration)
            {
               val = decodeHumidity(&page[temperatureLogSize + 
                     ((offsetDepth+i*dataBytes)%maxSamples)],dataBytes,FALSE,config);
               valsq = val*val;
               error = config.humCoeffA*valsq + config.humCoeffB*val + config.humCoeffC;
               val = val - error;

               if(config.useTempCalforHumidity)
               {
                  tempVal = val;

                  if((temperatureLogSize > 0) && config.useTemperatureCalibration)
                  {
                     val = decodeTemperature(&page[temperatureLogSize +
                        ((offsetDepth+i*tempBytes)%maxSamples)],tempBytes,TRUE,config);
                     valsq = val*val;
                     error = config.tempCoeffA*valsq + config.tempCoeffB*val + config.tempCoeffC;
                     humCal = val - error;
                  }
                  else if(temperatureLogSize > 0)
                  {
                     humCal = decodeTemperature(&page[temperatureLogSize +
                        ((offsetDepth+i*tempBytes)%maxSamples)],tempBytes,TRUE,config);
                  }
                  else
                     humCal = 25.0;

                  if(humCal<=15)
                     val = ((tempVal*0.0307 + 0.958) - (0.8767-0.0035*humCal + 
                        0.000043*humCal*humCal))/(0.035 + 0.000067*humCal - 
                        0.000002*humCal*humCal);
                  else
                     val = ((tempVal*0.0307 + 0.958) - (0.8767-0.0035*humCal + 
                        0.000043*humCal*humCal))/(0.0317 + 0.000067*humCal - 
                        0.000002*humCal*humCal);
               }
            
               if(val < 0.0)
                  val = 0.0;

               fprintf(fidData,"%f \n",val);
            }
            else          
            {
               val = decodeHumidity(&page[temperatureLogSize +
                  ((offsetDepth+i*dataBytes)%maxSamples)],dataBytes,FALSE,config);

               if(val < 0.0)
                  val = 0.0;

               fprintf(fidData,"%f \n",val);
            }
         }
         else
         {
            fprintf(fidData,"%f \n",getADVoltage(&page[temperatureLogSize + 
               (offsetDepth+i*dataBytes)%maxSamples],dataBytes,FALSE,config));
         }
      }
      fflush(fidData);
      fclose(fidData);
      printf("The humidity percentage or voltage is in data.log file.\n");
   }

   return TRUE;
}


// *****************************************************************************
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Temperature Interface Functions
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *****************************************************************************

/**
 * Performs a temperature conversion.  Use the <code>state</code>
 * information to calculate the conversion time.
 *
 * @param  state byte array with device state information
 *
 */
SMALLINT doTemperatureConvert(int portnum, uchar *SNum, uchar *state)
{
   uchar buffer[2];

   /* check for mission in progress */
   if(getFlag(portnum,SNum,GENERAL_STATUS_REGISTER,GSR_BIT_MISSION_IN_PROGRESS))
   {
      printf("Can't force temperature read during a mission.\n");
      return FALSE;
   }

   owSerialNum(portnum,SNum,FALSE);

   if(!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // perform the conversion
   buffer[0] = (uchar) FORCED_CONVERSION;
   buffer[1] = 0xFF;

   // send block (check copy indication complete)
   if(!owBlock(portnum,FALSE,&buffer[0],2))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   msDelay(750);

   // grab the data
   state[LAST_TEMPERATURE_CONVERSION_LSB&0x3F]
      = readByte(portnum,SNum,LAST_TEMPERATURE_CONVERSION_LSB);
   state[LAST_TEMPERATURE_CONVERSION_MSB&0x3F]
      = readByte(portnum,SNum,LAST_TEMPERATURE_CONVERSION_MSB);

   return TRUE;
}

// *****************************************************************************
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// A-to-D Interface Functions
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *****************************************************************************

/**
 * Performs a voltage conversion on one specified channel.
 * Use the method <code>getADVoltage(int,byte[])</code> to read
 * the result of this conversion, using the same <code>channel</code>
 * argument as this method uses.
 *
 * @param channel channel number in the range [0 to (<code>getNumberADChannels()</code> - 1)]
 * @param state current state of the device returned from <code>readDevice()</code>
 */
SMALLINT doADConvert (int portnum, uchar *SNum, uchar *state)
{
   uchar buffer[2];

   /* check for mission in progress */
   if(getFlag(portnum,SNum,GENERAL_STATUS_REGISTER,GSR_BIT_MISSION_IN_PROGRESS))
   {
      printf("Can't force temperature read during a mission.\n");
      return FALSE;
   }

   /* check that the RTC is running */
   if(!getFlag(portnum,SNum,RTC_CONTROL_REGISTER,RCR_BIT_ENABLE_OSCILLATOR))
   {
      printf("Can't force A/D conversion if the oscillator is not enabled.\n");
      return FALSE;
   }

   owSerialNum(portnum,SNum,FALSE);

   if(!owAccess(portnum))
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   // perform the conversion
   buffer[0] = (uchar) FORCED_CONVERSION;
   buffer[1] = 0xFF;

   // send block (check copy indication complete)
   if(!owBlock(portnum,FALSE,&buffer[0],2))
   {
      OWERROR(OWERROR_BLOCK_FAILED);
      return FALSE;
   }

   msDelay(750);

   // grab the data
   state[LAST_DATA_CONVERSION_LSB&0x3F]
      = readByte(portnum,SNum,LAST_DATA_CONVERSION_LSB);
   state [LAST_DATA_CONVERSION_MSB&0x3F]
      = readByte(portnum,SNum,LAST_DATA_CONVERSION_MSB);

   return TRUE;
}


double getADVoltage(uchar *data, int length, SMALLINT reverse, configLog config)
{
   double dval;
   // get the 10-bit value of vout
   int ival = 0;

   if(reverse && length==2)
   {
      ival = ((data[0]&0x0FF)<<(config.adDeviceBits-8));
      ival |= ((data[1]&0x0FF)>>(16-config.adDeviceBits));
   }
   else if(length==2)
   {
      ival = ((data[1]&0x0FF)<<(config.adDeviceBits-8));
      ival |= ((data[0]&0x0FF)>>(16-config.adDeviceBits));
   }
   else
   {
      ival = ((data[0]&0x0FF)<<(config.adDeviceBits-8));
   }

   dval = (ival*config.adReferenceVoltage)/(1<<config.adDeviceBits);

   return dval;
}

//--------
//-------- Clock 'set' Methods
//--------

/**
 * Set the time in the DS1922 time register format.
 */
void setTime(int timeReg, int hours, int minutes, int seconds,
             SMALLINT AMPM, uchar *state)
{
   uchar upper, lower;

   /* format in bytes and write seconds */
   upper            = (uchar) (((seconds / 10) << 4) & 0xf0);
   lower            = (uchar) ((seconds % 10) & 0x0f);
   state[timeReg++] = (uchar) (upper | lower);

   /* format in bytes and write minutes */
   upper            = (uchar) (((minutes / 10) << 4) & 0xf0);
   lower            = (uchar) ((minutes % 10) & 0x0f);
   state[timeReg++] = (uchar) (upper | lower);

   /* format in bytes and write hours/(12/24) bit */
   if (AMPM)
   {
      upper = 0x04;

      if (hours > 11)
         upper = (uchar) (upper | 0x02);

      // this next logic simply checks for a decade hour
      if (((hours % 12) == 0) || ((hours % 12) > 9))
         upper = (uchar) (upper | 0x01);

      if (hours > 12)
         hours = hours - 12;

      if (hours == 0)
         lower = 0x02;
      else
         lower = (uchar) ((hours % 10) & 0x0f);
   }
   else
   {
      upper = (uchar) (hours / 10);
      lower = (uchar) (hours % 10);
   }

   upper          = (uchar) ((upper << 4) & 0xf0);
   lower          = (uchar) (lower & 0x0f);
   state[timeReg] = (uchar) (upper | lower);
}

/**
 * Set the current date in the DS1922's real time clock.
 *
 * year - The year to set to, i.e. 2001.
 * month - The month to set to, i.e. 1 for January, 12 for December.
 * day - The day of month to set to, i.e. 1 to 31 in January, 1 to 30 in April.
 */
void setDate (int timeReg, int year, int month, int day, uchar *state)
{
   uchar upper, lower;

   /* write the day byte (the upper holds 10s of days, lower holds single days) */
   upper             = (uchar) (((day / 10) << 4) & 0xf0);
   lower             = (uchar) ((day % 10) & 0x0f);
   state[timeReg++] = (uchar) (upper | lower);

   /* write the month bit in the same manner, with the MSBit indicating
      the century (1 for 2000, 0 for 1900) */
   upper             = (uchar) (((month / 10) << 4) & 0xf0);
   lower             = (uchar) ((month % 10) & 0x0f);
   state[timeReg++] = (uchar) (upper | lower);

   // now write the year
   year             = year + 1900;
   year             = year - FIRST_YEAR_EVER;

   if(year>100)
   {
      state[timeReg-1] |= 0x80;
      year -= 100;
   }
   upper            = (uchar) (((year / 10) << 4) & 0xf0);
   lower            = (uchar) ((year % 10) & 0x0f);
   state[timeReg]  = (uchar) (upper | lower);
}




/**
 * helper method for decoding temperature values
 */
double decodeTemperature(uchar *data, int length, SMALLINT reverse, configLog config)
{
   double whole, fraction = 0;

   if(reverse && length==2)
   {
      fraction = ((data[1]&0x0FF)/512.0);
      whole = (data[0]&0x0FF)/2.0 + (config.lowTemp-1);
   }
   else if(length==2)
   {
      fraction = ((data[0]&0x0FF)/512.0);
      whole = (data[1]&0x0FF)/2.0 + (config.lowTemp-1);
   }
   else
   {
      whole = (data[0]&0x0FF)/2.0 + (config.lowTemp-1);
   }

   return whole + fraction;
}


/**
 * helper method for decoding humidity values
 */
double decodeHumidity(uchar *data, int length, SMALLINT reverse, configLog config)
{
   double val;

   // get the 10-bit value of Vout
   val = getADVoltage(data, length, reverse, config);

   // convert Vout to a humidity reading
   // this formula is from HIH-3610 sensor datasheet
   val = (val-.958)/.0307;

   return val;
}
