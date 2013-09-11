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
//  thermo.h - Include file for Thermochron demo.
//
//  Version: 2.00
//    
//    History:
//           1.03 -> 2.00  Reorganization of Public Domain Kit 


#ifndef THERMO_TYPES

#define THERMO_TYPES

// defines
#define STATUS_PAGE    16
#define THERMO_FAM     0x21

#include <stdlib.h>

// Typedefs
#ifndef OW_UCHAR
   #define OW_UCHAR
   typedef unsigned char  uchar;
   #ifdef WIN32
      typedef unsigned short ushort;
      typedef unsigned long  ulong;
   #endif
#endif

// structure to hold the mission status 
typedef struct 
{
   uchar serial_num[8];          // serial number of thermochron

   uchar mission_in_progress;    // 1 mission in progres, 0 mission over

   uchar sample_rate;            // minutes between samples

   uchar rollover_enable;        // 1 if roll-over enabled
   uchar rollover_occurred;      // 1 if roll-over occurred

   ushort start_delay;           // minutes before mission starts

   ulong mission_start_time;     // date/time when mission started
   ulong current_time;           // current real-time clock value
   ulong download_time;          // download stations time of reading

   ulong mission_samples;        // number of samples in this mission
   ulong samples_total;          // total number of samples taken by device
    
   uchar high_threshold;         // raw temp of high threshold
   uchar low_threshold;          // raw temp of low threshold

   // skip alarm modes and status for now

   uchar status_raw[32];

} MissionStatus;

// structure to hold the histogram data 
typedef struct 
{
   ushort bin_count[63];    // counter per bin 0 to 62 
   float  start_range[63];  // start temp range (C) in bin 0 to 62
   float  end_range[63];    // end temp range (C) in bin 0 to 62

   uchar  hist_raw[128];    // raw data for histogram

} Histogram;

// structure to hold the histogram data 
typedef struct 
{
   int num_low;               // number of low events
   ulong low_start_time[12];  // start time of event 0 to 12
   ulong low_end_time[12];    // end time of event 0 to 12 
   int num_high;              // number of high events
   ulong high_start_time[12]; // start time of event 0 to 12
   ulong high_end_time[12];   // end time of event 0 to 12

   uchar  alarm_raw[96];     // raw data for alarm events

} TempAlarmEvents;

// structure to hold the log data 
typedef struct 
{
   int   num_log;             // number of logs
   float temp[2048];          // temperature log in (C) 
   ulong start_time;          // start time of log
   int   interval;            // interval in seconds between logs

   uchar log_raw[2048];       // raw data for log

} Log;

// structure to hold all of the thermochron data state
typedef struct
{
   MissionStatus MissStat;    // mission state
   Histogram HistData;        // histogram data
   TempAlarmEvents AlarmData; // temperature alarm event data
   Log LogData;               // log data

} ThermoStateType;

// type structure to holde time/date 
typedef struct          
{
     ushort  second;
     ushort  minute;
     ushort  hour;
     ushort  day;
     ushort  month;
     ushort  year;
} timedate;

// structure to hold each state in the StateMachine
typedef struct
{
   int  Step;
   char StepDescription[50];

} ThermoScript;
#endif

// step constants
enum { ST_SETUP=0, ST_READ_STATUS, ST_READ_ALARM, ST_READ_HIST,
       ST_READ_LOG, ST_CLEAR_MEM, ST_CLEAR_VERIFY, ST_WRITE_TIME,
       ST_WRITE_CONTROL, ST_WRITE_RATE, ST_FINISH, ST_GET_SESSION, 
       ST_FIND_THERMO, ST_REL_SESSION, ST_READ_PAGES, ST_WRITE_MEM,
       ST_CLEAR_SETUP };

// status contants
enum { STATUS_STEP_COMPLETE, STATUS_COMPLETE, STATUS_INPROGRESS,
       STATUS_ERROR_HALT, STATUS_ERROR_TRANSIENT };

// download steps
static ThermoScript Download[] = 
    {{ ST_READ_STATUS,  "Setup to read the mission status"},
     { ST_READ_PAGES,   "Read the status page"},
     { ST_READ_ALARM,   "Setup to read alarm pages"},
     { ST_READ_PAGES,   "Read the alarm pages"},
     { ST_READ_HIST,    "Setup to read histogram pages"},
     { ST_READ_PAGES,   "Read the histogram pages"},
     { ST_READ_LOG,     "Setup to read log pages"},
     { ST_READ_PAGES,   "Read the log pages"},
     { ST_FINISH,       "Finished"}}; 

// read status only steps
static ThermoScript GetStatus[] = 
    {{ ST_READ_STATUS,  "Setup to read the mission status"},
     { ST_READ_PAGES,   "Read the status page"},
     { ST_FINISH,       "Finished"}}; 

// mission steps (assume already did StatusThermo)
static ThermoScript Mission[] = 
    {{ ST_CLEAR_SETUP,  "Setup clear memory"},
     { ST_WRITE_MEM,    "Write clear memory bit"},
     { ST_CLEAR_MEM,    "Clear the memory"},
     { ST_READ_STATUS,  "Setup to read the mission status"},
     { ST_READ_PAGES,   "Read the status page"},
     { ST_CLEAR_VERIFY, "Verify memory is clear"},
     { ST_WRITE_TIME,   "Setup to write the real time clock"},
     { ST_WRITE_MEM,    "Write the real time clock"},
     { ST_WRITE_CONTROL,"Setup to write the control"},
     { ST_WRITE_MEM,    "Write the control"},
     { ST_WRITE_RATE,   "Setup to write the sample rate to start mission"},
     { ST_WRITE_MEM,    "Write the sample rate"},
     { ST_READ_STATUS,  "Read the new mission status"},
     { ST_FINISH,       "Finished"}};

// Local Function Prototypes 
int DownloadThermo(int,uchar *,ThermoStateType *,FILE *);
int ReadThermoStatus(int,uchar *,ThermoStateType *,FILE *);
int MissionThermo(int,uchar *,ThermoStateType *,FILE *);
static int RunThermoScript(int,ThermoStateType *,ThermoScript script[],FILE *fp);
void MissionStatusToString(MissionStatus *,int,char *);
void  SecondsToDate(timedate *, ulong);
ulong DateToSeconds(timedate *);
uchar BCDToBin(uchar);
void  InterpretStatus(MissionStatus *);
void  FormatMission(MissionStatus *);
void  InterpretHistogram(Histogram *);
void  HistogramToString(Histogram *, int, char *);
void  InterpretAlarms(TempAlarmEvents *, MissionStatus *);
void  AlarmsToString(TempAlarmEvents *, char *);
void  InterpretLog(Log *, MissionStatus *);
void  LogToString(Log *, int, char *);
void  DebugToString(MissionStatus *, TempAlarmEvents *, Histogram *, Log *, char *); 
float TempToFloat(uchar, int);
float CToF(float);
uchar ToBCD(short); 


