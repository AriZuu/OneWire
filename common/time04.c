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
//  time04.c - 1-Wire Clock device functions for (DS2404, DS1994, DS1427).
//             These functions allow the programmer to: 
//             write the Real-Time Clock (RTC) (and enable osc), 
//             write the Clock Alarm (RTCA) (and set alarm flag), and 
//             optionally write-protect the Clock Alarm.
//

#include "ownet.h"
#include <time.h>
#include <stdio.h>
#include <memory.h>
#include "mbscr.h"
#include "mbnv.h"
#include "time04.h"

// Include files for the Palm and Visor
#ifdef __MC68K__
#include <DateTime.h>
#include <TimeMgr.h>
#else
#include <string.h>
#endif

// Function Prototypes

// The "getters" 
SMALLINT getRTC(int, uchar *, timedate *);
SMALLINT getRTCA(int, uchar *, timedate *);
SMALLINT getControlRegisterBit(int, uchar *, int, SMALLINT *);
SMALLINT getStatusRegisterBit(int, uchar *, int, SMALLINT *);

// The "setters"
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

//----------------------------------------------------------------------
// Retrieves the local time from the 1-Wire Real-Time Clock (RTC)
// in the form of a timedate structure:
//
// typedef struct          
// {
//    ushort  second;
//    ushort  minute;
//    ushort  hour;
//    ushort  day;
//    ushort  month;
//    ushort  year;
// } timedate;
//
// Parameters:
//  portnum  the port number of the port being used for the
//           1-Wire network.
//  SNum     The 1-Wire address of the device to communicate.
//  * td     timedate struct to return the time/date.
//
// Returns:  TRUE  if the read worked
//           FALSE if there was an error in reading
//
//  * td     Returns the time/date in a timedate struct.
//
SMALLINT getRTC(int portnum, uchar * SNum, timedate * td)
{
   uchar timebuffer[4];
   ulong timelong = 0;

   // read 4 bytes from 1-Wire clock device (memory bank 2) 
   // starting at address 0x03.
   if (!readNV(2, portnum, SNum, 0x03, FALSE, &timebuffer[0], 4))
   {
	   return FALSE;
   }
   timelong = uchar_to_bin(&timebuffer[0],4);
   SecondsToDate(td,timelong);
   return TRUE;
}

//----------------------------------------------------------------------
// Retrieves the Real-Time Clock Alarm (RTCA) "trigger" time from the 
// 1-Wire device in the form of a timedate structure:
//
// typedef struct          
// {
//    ushort  second;
//    ushort  minute;
//    ushort  hour;
//    ushort  day;
//    ushort  month;
//    ushort  year;
// } timedate;
//
// Parameters:
//  portnum  the port number of the port being used for the
//           1-Wire network.
//  SNum     The 1-Wire address of the device to communicate.
//  * td     Pointer to a timedate struct to return the time/date.
//
// Returns:  TRUE  if the write worked
//           FALSE if there was an error in reading
//
//  * td     Returns the alarm time/date in a timedate struct.
//
SMALLINT getRTCA(int portnum, uchar * SNum, timedate * td)
{
   uchar timebuffer[4];
   ulong timelong = 0;

   // read 4 bytes for alarm trigger from 1-Wire clock device (memory bank 2) 
   // starting at address 0x11.
   if (!readNV(2, portnum, SNum, 0x11, FALSE, &timebuffer[0], 4))
   {
	   return FALSE;
   }
   timelong = uchar_to_bin(&timebuffer[0],4);
   SecondsToDate(td,timelong);
   return TRUE;
}

//----------------------------------------------------------------------
// Retrieves a specific bit from the control register of the 1-Wire 
// clock.  Any one of the 8 control register bits can be retrieved. 
// Their definitions can be found on page 9 of the DS1994 datasheet.
// To retrieve a particular bit, please specify one of the following 
// tm_04.h defines:
//
// DSEL           0x80  Delay select bit.
// STOPSTART      0x40  STOP/START (in Manual Mode).
// AUTOMAN        0x20  Automatic/Manual Mode.
// OSC            0x10  Oscillator enable (start the clock ticking...)
// RO             0x08  Read only (during expiration: 1 for read-only, 0 for destruct).
// WPC            0x04  Write-Protect cycle counter bit.
// WPI            0x02  Write-Protect interval timer bit.
// WPR            0x01  Write-Protect RTC and clock alarm bit.
//
// Parameters:
//  portnum      The port number of the port being used for the
//               1-Wire network.
//  SNum         The 1-Wire address of the device with which to communicate.
//  controlbit   The desired bit to be returned.  (One of the above 8 bits).
//  * returnbit  A pointer to receive the bit indicated.  Will return TRUE or FALSE.
//
// Returns:      TRUE  if the read worked.
//               FALSE if there was an error in reading the control register.
//
//  * returnbit  Returns TRUE or FALSE.  TRUE if the selected bit is 
//               one and FALSE if the selected bit is a zero.
//
SMALLINT getControlRegisterBit(int portnum, uchar * SNum, int controlbit, SMALLINT * returnbit)
{
   uchar buf = 0;
   // read the control register from 1-Wire clock device (memory bank 2) 
   // starting at address 0x01.
   if (!readNV(2, portnum, SNum, 0x01, FALSE, &buf, 1))
   {
	   return FALSE;
   }
   // mask off pertinent control bit from the control register
   if ((buf & controlbit) > 0)
   {
      *returnbit = TRUE;
   }
   else
   {
      *returnbit = FALSE;
   }
   return TRUE;
}

//----------------------------------------------------------------------
// Retrieves a specific bit from the status register of the 1-Wire 
// clock.  Any one of the 6 status register bits (the 2 most significant 
// bits are "don't care" bits) can be retrieved. Their definitions can 
// be found on page 8 of the DS1994 datasheet. To retrieve a 
// particular bit, please specify one of the following tm_04.h defines:
//
// CCEInverse     0x20  Cycle counter alarm trigger (inverse - 0 for on and 1 for off).
// ITEInverse     0x10  Interval Timer alarm trigger (inverse).
// RTEInverse     0x08  RTC alarm trigger (inverse).
// CCF            0x04  Cycle counter alarm indicator.
// ITF            0x02  Interval Timer alarm indicator.
// RTF            0x01  RTC alarm indicator.
//
// Parameters:
//  portnum      The port number of the port being used for the
//               1-Wire network.
//  SNum         The 1-Wire address of the device with which to communicate.
//  statusbit    The desired bit to be returned.  (One of the above 8 bits).
//  * returnbit  A pointer to receive the bit indicated.  Will return TRUE or FALSE.
//
// Returns:      TRUE  if the read worked.
//               FALSE if there was an error in reading the status register.
//
//  * returnbit  Returns TRUE or FALSE.  TRUE if the selected bit is 
//               one and FALSE if the selected bit is a zero.
//
SMALLINT getStatusRegisterBit(int portnum, uchar * SNum, int statusbit, SMALLINT * returnbit)
{
   uchar buf = 0;
   // read the control register from 1-Wire clock device (memory bank 2) 
   // starting at address 0x00.
   if (!readNV(2, portnum, SNum, 0x00, FALSE, &buf, 1))
   {
	   return FALSE;
   }
   if ((buf & statusbit) > 0)
   {
      *returnbit = TRUE;
   }
   else
   {
      *returnbit = FALSE;
   }
   return TRUE;
}

//----------------------------------------------------------------------
// Sets the Real-Time clock (RTC) of the 1-Wire device from the 
// input parameter settime.  The settime parameter should be the 
// number of seconds elapsed since Jan. 1, 1970. Also included is 
// an option to start or stop the oscillator.
//
// Parameters:
//  portnum   The port number of the port being used for the
//            1-Wire network.
//  SNum      The 1-Wire address of the device to communicate.
//  settime   The number of seconds since Jan. 1, 1970 as an unsigned 
//            long integer.
//  OscEnable Sets the Oscillator enable bit of the control register.  A 
//            TRUE will turn the oscillator one and a FALSE will turn 
//            the oscillator off.
//
// Returns:  TRUE  if the write worked.
//           FALSE if there was an error in writing to the part.
//
SMALLINT setRTC(int portnum, uchar * SNum, ulong settime, SMALLINT OscEnable)
{
   uchar timebuffer[4];

   timebuffer[0] = 0;
   timebuffer[1] = 0;
   timebuffer[2] = 0;
   timebuffer[3] = 0;

   //convert the number of seconds (settime) into a 4-byte buffer (timebuffer)
   memcpy(&timebuffer[0], &settime, 4);
   // write 4 bytes to 1-Wire clock device (memory bank 2) 
   // starting at address 0x03.
   if (!writeNV(2, portnum, SNum, 0x03, &timebuffer[0], 4))
   {
	   return FALSE;  // write failed
   }
   if (setOscillator(portnum, SNum, OscEnable) != TRUE)
   {
	   return FALSE;  // write failed
   }
   return TRUE;
}

//----------------------------------------------------------------------
// Sets the Real-Time clock (RTC) of the 1-Wire device to the time 
// on the PC, with an option to start the oscillator.
//
// Parameters:
//  portnum   The port number of the port being used for the
//            1-Wire network.
//  SNum      The 1-Wire address of the device to communicate.
//  OscEnable Sets the Oscillator enable bit of the control register.  A 
//            TRUE will turn the oscillator one and a FALSE will turn 
//            the oscillator off.
//
// Returns:  TRUE  if the write worked.
//           FALSE if there was an error in writing to the part.
//
SMALLINT setRTCFromPC(int portnum, uchar * SNum, SMALLINT OscEnable)
{
   ulong timelong = 0;
   timedate td;

   // get seconds since Jan 1, 1970
   getPCTime(&td); // first, get timedate
   timelong = DateToSeconds(&td); // convert timedate to seconds since Jan 1, 1970
   // write 4 bytes to 1-Wire clock device (memory bank 2) 
   // starting at address 0x03.
   if (setRTC(portnum, SNum, timelong, OscEnable) != TRUE)
   {
	   return FALSE;
   }
   return TRUE;
}

//----------------------------------------------------------------------
// Sets the oscillator bit in the control register of the 1-Wire
// clock.  It can be turned on (1) or off (0).
//
// Parameters:
//  portnum   The port number of the port being used for the
//            1-Wire network.
//  SNum      The 1-Wire address of the device to communicate.
//  OscEnable Sets the Oscillator enable bit of the control register.  A 
//            TRUE will turn the oscillator one and a FALSE will turn 
//            the oscillator off.
//
// Returns:  TRUE  if the write worked.
//           FALSE if there was an error in writing to the part.
//
SMALLINT setOscillator(int portnum, uchar * SNum, SMALLINT OscEnable)
{
   if (setControlRegister(portnum,SNum,-99,-99,-99,OscEnable,-99,-99,-99,-99) == FALSE)
   {
	   return FALSE;
   }
   return TRUE;
}

//----------------------------------------------------------------------
// Sets the Real-Time clock alarm (RTCA) time of the 1-Wire 
// device from the input parameter settime.  The settime parameter should be the 
// number of seconds elapsed since Jan. 1, 1970. Also included is 
// an option to enable or disable the alarm.
//
// Parameters:
//  portnum        The port number of the port being used for the
//                 1-Wire network.
//  SNum           The 1-Wire address of the device to communicate.
//  settime        The number of seconds since Jan. 1, 1970 as an 
//                 unsigned long integer.
//  AlarmEnable    Enables/Disables the Real-Time clock alarm.  TRUE will 
//                 enable the alarm and FALSE will disable the alarm. 
//
// Returns:        TRUE  if the write worked.
//                 FALSE if there was an error in writing to the part.
//
SMALLINT setRTCA(int portnum, uchar * SNum, ulong setalarm, SMALLINT AlarmEnable)
{
   uchar timebuffer[4];

   timebuffer[0] = 0;
   timebuffer[1] = 0;
   timebuffer[2] = 0;
   timebuffer[3] = 0;

   //convert the number of seconds (settime) into a 4-byte buffer (timebuffer)
   memcpy(&timebuffer[0], &setalarm, 4);
   // write 4 bytes to 1-Wire clock device (memory bank 2) 
   // starting at address 0x11.
   if (!writeNV(2, portnum, SNum, 0x11, &timebuffer[0], 4))
   {
	   return FALSE;
   }
   if (setRTCAEnable(portnum, SNum, AlarmEnable) != TRUE)
   {
	   return FALSE;
   }
   return TRUE;
}

//----------------------------------------------------------------------
// Sets the Real-Time clock alarm (RTCA) time on the 1-Wire 
// device to the PC's clock with an offset in seconds. Also included 
// is an option to enable or disable the alarm.
//
// Parameters:
//  portnum        The port number of the port being used for the
//                 1-Wire network.
//  SNum           The 1-Wire address of the device to communicate.
//  PCOffset       The number of seconds from the PC's time/date to 
//                 to set the RTCA.
//  AlarmEnable    Enables/Disables the Real-Time clock alarm.  TRUE will 
//                 enable the alarm and FALSE will disable the alarm. 
//
// Returns:        TRUE  if the write worked.
//                 FALSE if there was an error in writing to the part.
//
SMALLINT setRTCAFromPCOffset(int portnum, uchar * SNum, ulong PCOffset, SMALLINT AlarmEnable)
{
   ulong timelong = 0;
   timedate td;

   // get seconds since Jan 1, 1970
   getPCTime(&td); // first, get timedate
   timelong = DateToSeconds(&td); // convert timedate to seconds since Jan 1, 1970
   // write 4 bytes to 1-Wire clock device (memory bank 2) 
   // starting at address 0x11.
   if (setRTCA(portnum, SNum, (timelong + PCOffset), AlarmEnable) != TRUE)
   {
	   return FALSE;
   }
   return TRUE;
}

//----------------------------------------------------------------------
// Enables or disables the Real-Time clock alarm on the 1-Wire 
// clock.  When enabled, the clock will alarm when the alarm 
// value is reached.  If disabled, the clock will not alarm. 
//
// Parameters:
//  portnum        The port number of the port being used for the
//                 1-Wire network.
//  SNum           The 1-Wire address of the device to communicate.
//  AlarmEnable    Sets the alarm enable bit of the control 
//                 register.  TRUE will enable the alarm and FALSE will 
//                 disable the alarm. 
//
// Returns:        TRUE  if the write worked.
//                 FALSE if there was an error in writing to the part.
//
SMALLINT setRTCAEnable(int portnum, uchar * SNum, SMALLINT AlarmEnable)
{
   SMALLINT MyRTENot = FALSE;
   // make sure MyRTENot is the inverse of AlarmEnable
   if (AlarmEnable == FALSE) MyRTENot = TRUE;

   if (setStatusRegister(portnum,SNum,-99,-99,MyRTENot) == FALSE)
   {
	   return FALSE;
   }
   return TRUE;
}

//----------------------------------------------------------------------
// Write-Protects the 1-Wire clock so that the real-time clock and alarm 
// registers cannot be changed.  This function can also be used to set the 
// expiration mode on the device.  TRUE sets the expiration to read-only, 
// meaning that the nv ram of the part becomes read-only after expiration, and 
// FALSE sets the expiration to "destruct", meaning only the part's serial 
// number can be read (and nothing else) after expiration.
//
// Parameters:
//  portnum    The port number of the port being used for the
//             1-Wire network.
//  SNum       The 1-Wire address of the 1-Wire device.
//  RTCProtect Sets the write-protect bit for the real-time clock 
//             and alarms located in the control register.  If set 
//             to TRUE, the part's Real-Time Clock and Alarm registers 
//             can no longer be written, including the WPR bit.  If 
//             set to FALSE (if possible), the part will not be write-
//             protected.
//  RTCAExpire Sets the expiration bit (RO) of the control register.
//             If set to TRUE, the part expires and all memory 
//             becomes read-only.  If FALSE, the only thing that can 
//             be accessed is the 1-Wire Net Address.
//
// Returns:    TRUE  if the write worked.
//             FALSE if there was an error in writing to the part.
//
SMALLINT setWriteProtectionAndExpiration(int portnum, uchar * SNum, SMALLINT RTCProtect, SMALLINT RTCAExpire)
{
   if (setControlRegister(portnum, SNum, -99,-99,-99,-99,RTCAExpire,-99,-99,RTCProtect) == FALSE)
   {
	   return FALSE;
   }
   return TRUE;
}

//----------------------------------------------------------------------
// Writes specific bits to the status register of the 1-Wire 
// clock.  Any of the 3 status register bits (the two most significant 
// bits are "don't care" bits and the first three lease significant bits 
// are read-only) can be retrieved.
//
// Note:  If specific bits are desired to be left alone, place values 
//        other than TRUE (1) or FALSE (0) in them (such as -99).
//
// Parameters:
//  portnum      The port number of the port being used for the
//               1-Wire network.
//  SNum         The 1-Wire address of the device with which to communicate.
//  CCENot       Cycle counter alarm trigger (inverse - 0 for on and 1 for off).
//  ITENot       Interval Timer alarm trigger (inverse).
//  RTENot       RTC alarm trigger enable (inverse).
//
// Returns:      TRUE  if the write worked.
//               FALSE if there was an error in writing the status register.
//
//
SMALLINT setStatusRegister(int portnum, uchar * SNum, SMALLINT CCENot, SMALLINT ITENot, SMALLINT RTENot)
{
   uchar statusregister[1]; // to store status byte
   SMALLINT writetopart = FALSE;
   statusregister[0] = 0;

   // read 1 bytes from 1-Wire clock device (memory bank 2) 
   // starting at address 0x01.  This is the status register.
   if (!readNV(2, portnum, SNum, 0x00, FALSE, &statusregister[0], 1))
   {
	   return FALSE;
   }
   // if necessary, update status register
   if (((statusregister[0] & 0x20) > 0) && (CCENot == FALSE)) // turn CCENot to 0
   {
      // The CCENot bit is in the 5th bit position of the status register.
      // 'And'ing with 0xDF (223 decimal or 11011111 binary) will 
	   // guarantee a 0-bit in the position.
      statusregister[0] = statusregister[0] & 0xDF;
	   writetopart = TRUE;
   }
   if (((statusregister[0] & 0x20) == 0) && (CCENot == TRUE)) // turn CCENot to 1
   {
      // The CCENot bit is in the 5th bit position of the status register.
      // 'Or'ing with 0x20 (32) will guarantee a 1-bit in the position.
      statusregister[0] = statusregister[0] | 0x20;
	   writetopart = TRUE;
   }
   if (((statusregister[0] & 0x10) > 0) && (ITENot == FALSE)) // turn ITENot to 0
   {
      // The ITENot bit is in 4th bit position of the status register.
      // 'And'ing with 0xEF (239 decimal or 11101111 binary) will 
	   // guarantee a 0-bit in the position.
      statusregister[0] = statusregister[0] & 0xEF;
	   writetopart = TRUE;
   }
   if (((statusregister[0] & 0x10) == 0) && (ITENot == TRUE)) // turn ITENot to 1
   {
      // The ITENot bit is in 4th bit position (16) of byte.
      // 'Or'ing with 0x10 (16) will guarantee a 1-bit in the position.
      statusregister[0] = statusregister[0] | 0x10;
	   writetopart = TRUE;
   }
   if (((statusregister[0] & 0x08) > 0) && (RTENot == FALSE)) // turn RTENot to 0
   {
      // The RTENot bit is in the 3rd bit position of the status register.
      // 'And'ing with 0xF7 (247 decimal or 11110111 binary) will 
	   // guarantee a 0-bit in the position.
      statusregister[0] = statusregister[0] & 0xF7;
	   writetopart = TRUE;
   }
   if (((statusregister[0] & 0x08) == 0) && (RTENot == TRUE)) // turn RTENot to 1
   {
      // The RTENot bit is in the 3rd bit position of the status register.
      // 'Or'ing with 0x08 will guarantee a 1-bit in the position.
      statusregister[0] = statusregister[0] | 0x08;
	   writetopart = TRUE;
   }

   // write new status register (if any) to part.
   if (writetopart == TRUE)
   {
      if (!writeNV(2, portnum, SNum, 0x00, &statusregister[0], 1))
      {
	      return FALSE;
      }	 
   }
   return TRUE;
}

//----------------------------------------------------------------------
// Writes specific bits to the control register of the 1-Wire 
// clock.  Any of the 8 control register bits can be written.
//
// Note:  If specific bits are desired to be left alone, place values 
//        other than TRUE (1) or FALSE (0) in them (such as -99)
//        when calling the function.
//
// Parameters:
//  portnum      The port number of the port being used for the
//               1-Wire network.
//  SNum         The 1-Wire address of the device with which to communicate.
//  dsel         Delay select bit
//  stopstart    STOP/START (in Manual Mode).
//  automan      Automatic/Manual Mode.
//  osc          Oscillator enable (start the clock ticking...)
//  ro           Read only (during expiration: 1 for read-only, 0 for destruct).
//  wpc          Write-Protect cycle counter bit.
//  wpi          Write-Protect interval timer bit.
//  wpr          Write-Protect RTC and clock alarm bit.
//
// Returns:      TRUE  if the write worked.
//               FALSE if there was an error in writing the status register.
//
//
SMALLINT setControlRegister(int portnum, uchar * SNum, SMALLINT dsel, SMALLINT startstop, SMALLINT automan, SMALLINT osc, SMALLINT ro, SMALLINT wpc, SMALLINT wpi, SMALLINT wpr)
{
   uchar controlregister[2]; // to store both status and control bytes
   SMALLINT writetopart = FALSE;
   SMALLINT writeprotect = FALSE;
   int i = 0;
   controlregister[0] = 0;
   controlregister[1] = 0;

   // read 1 bytes from 1-Wire clock device (memory bank 2) 
   // starting at address 0x01.  This is the control register.
   if (!readNV(2, portnum, SNum, 0x00, FALSE, &controlregister[0], 2))
   {
	   return FALSE;
   }

   // if necessary, update control register
   if (((controlregister[1] & 0x80) > 0) && (dsel == FALSE)) // turn dsel to 0
   {
      // The dsel bit is in 7th bit position (MSb) of byte.
      // 'And'ing with 0x7F (127 decimal or 01111111 binary) will 
	   // guarantee a 0-bit in the position.
      controlregister[1] = controlregister[1] & 0x7F;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x80) == 0) && (dsel == TRUE)) // turn dsel to 1
   {
      // The dsel bit is in 7th bit position (MSb) of byte.
      // 'Or'ing with 0x80 (16) will guarantee a 1-bit in the position and 
      // turn the dsel bit to 1.
      controlregister[1] = controlregister[1] | 0x80;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x40) > 0) && (startstop == FALSE)) // turn startstop to 0
   {
      // The startstop bit is in the 6th bit position of the control register.
      // 'And'ing with 0xBF (191 decimal or 10111111 binary) will 
	   // guarantee a 0-bit in the position.
      controlregister[1] = controlregister[1] & 0xBF;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x40) == 0) && (startstop == TRUE)) // turn startstop to 1
   {
      // The startstop bit is in the 6th bit position of the control register.
      // 'Or'ing with 0x40 (64) will guarantee a 1-bit in the position.
      controlregister[1] = controlregister[1] | 0x40;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x20) > 0) && (automan == FALSE)) // turn automan to 0
   {
      // The automan bit is in the 5th bit position of the control register.
      // 'And'ing with 0xDF (223 decimal or 11011111 binary) will 
	   // guarantee a 0-bit in the position.
      controlregister[1] = controlregister[1] & 0xDF;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x20) == 0) && (automan == TRUE)) // turn automan to 1
   {
      // The automan bit is in the 5th bit position of the control register.
      // 'Or'ing with 0x20 (32) will guarantee a 1-bit in the position.
      controlregister[1] = controlregister[1] | 0x20;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x10) > 0) && (osc == FALSE)) // turn oscillator off
   {
      // The oscillator enable bit is in 4th bit position (16) of byte.
      // 'And'ing with 0xEF (239 decimal or 11101111 binary) will 
	   // guarantee a 0-bit in the position and turn the oscillator off.
      controlregister[1] = controlregister[1] & 0xEF;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x10) == 0) && (osc == TRUE)) // turn oscillator on
   {
      // The oscillator enable bit is in 4th bit position (16) of byte.
      // 'Or'ing with 0x10 (16) will guarantee a 1-bit in the position and 
      // turn the oscillator on.
      controlregister[1] = controlregister[1] | 0x10;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x08) > 0) && (ro == FALSE)) // turn ro to 0
   {
      // The ro bit is in the 3rd bit position of the control register.
      // 'And'ing with 0xF7 (247 decimal or 11110111 binary) will 
	   // guarantee a 0-bit in the position.
      controlregister[1] = controlregister[1] & 0xF7;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x08) == 0) && (ro == TRUE)) // turn ro to 1
   {
      // The ro bit is in the 3rd bit position of the control register.
      // 'Or'ing with 0x08 will guarantee a 1-bit in the position.
      controlregister[1] = controlregister[1] | 0x08;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x04) > 0) && (wpc == FALSE)) // turn wpc to 0
   {
      // The wpc bit is in the 2nd bit position of the control register.
      // 'And'ing with 0xFB (251 decimal or 11111011 binary) will 
	   // guarantee a 0-bit in the position.
      controlregister[1] = controlregister[1] & 0xFB;
	   writeprotect = TRUE;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x04) == 0) && (wpc == TRUE)) // turn wpc to 1
   {
      // The wpc bit is in the 2nd bit position of the control register.
      // 'Or'ing with 0x04 will guarantee a 1-bit in the position.
      controlregister[1] = controlregister[1] | 0x04;
	   writeprotect = TRUE;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x02) > 0) && (wpi == FALSE)) // turn wpi to 0
   {
      // The wpi bit is in the 1st bit position of the control register.
      // 'And'ing with 0xFD (253 decimal or 11111101 binary) will 
	   // guarantee a 0-bit in the position.
      controlregister[1] = controlregister[1] & 0xFD;
	   writeprotect = TRUE;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x02) == 0) && (wpi == TRUE)) // turn wpi to 1
   {
      // The wpi bit is in the 1st bit position of the control register.
      // 'Or'ing with 0x02 will guarantee a 1-bit in the position.
      controlregister[1] = controlregister[1] | 0x02;
	   writeprotect = TRUE;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x01) > 0) && (wpr == FALSE)) // turn wpr to 0
   {
      // The wpr bit is in the 0th bit position of the control register.
      // 'And'ing with 0xFE (254 decimal or 11111110 binary) will 
	   // guarantee a 0-bit in the position.
      controlregister[1] = controlregister[1] & 0xFE;
	   writeprotect = TRUE;
	   writetopart = TRUE;
   }
   if (((controlregister[1] & 0x01) == 0) && (wpr == TRUE)) // turn wpr to 1
   {
      // The wpr bit is in the 0th bit position of the control register.
      // 'Or'ing with 0x01 will guarantee a 1-bit in the position.
      controlregister[1] = controlregister[1] | 0x01;
	   writeprotect = TRUE;
	   writetopart = TRUE;
   }

   // write new control register (if any) to part.
   if (writetopart == TRUE)
   {
	   // if writeprotect is true, then copyscratchpad 3 times...
	   if (writeprotect == TRUE)
      {
         owSerialNum(portnum, SNum, 1);
		   // write to scratchpad
		   if (!writeScratchpd(portnum, 0x200, &controlregister[0], 2))
         {
		      return FALSE;
         }

         // copy the scratchpad 3 times to write protect
         for (i = 0; i < 3; i++)
         {
            if (!owAccess(portnum)) return FALSE;            // if Access is unsuccessful
               
            if(!((0x55 != (uchar)owTouchByte(portnum,0x55)) &&  // sent the copy command
                 (0x00 != (uchar)owTouchByte(portnum, 0x00)) && // write the target address 1
                 (0x02 != (uchar)owTouchByte(portnum, 0x02))))  // write the target address 2
            {
               return FALSE;
            }
			   if (i == 0)
            {
			      if(!owTouchByte(portnum,0x01))
                  return FALSE;
            }
			   else
            {
			      if(! owTouchByte(portnum, 0x81))  // The AA bit gets set on consecutive
				      return FALSE;                  // copyscratchpads (pg. 12 of datasheet)
            }
         }
      }
	   else
      {
         if (!writeNV(2, portnum, SNum, 0x00, &controlregister[0], 2))
         {
	         return FALSE;
         }
      }	 
   }
   return TRUE;
}

//----------------------------------------------------------------------
// Retrieves the local time from the PC 
// in the form of a timedate structure:
//
// typedef struct          
// {
//    ushort  second;
//    ushort  minute;
//    ushort  hour;
//    ushort  day;
//    ushort  month;
//    ushort  year;
// } timedate;
//
// Parameters:
//  * td     timedate struct to return the time/date.
//
// Returns:  
//  * td     returns the time/date in a timedate struct.
//
void getPCTime(timedate * td)
{
   time_t tlong = 0; // number of seconds since Jan. 1, 1970
   struct tm *tstruct;  // structure containing time info

   tlong = time(NULL); // get seconds since Jan. 1, 1970
   tstruct = localtime(&tlong); // transform seconds to struct containing date/time info.

   // Populate timedate structure
   td->day = tstruct->tm_mday;
   td->month = tstruct->tm_mon + 1;
   td->year = tstruct->tm_year + 1900;
   td->hour = tstruct->tm_hour;
   td->minute = tstruct->tm_min;
   td->second = tstruct->tm_sec;
}

//----------------------------------------------------------------------
// Take a 4-byte long uchar array (consisting of 
// the number of seconds elapsed since Jan. 1 1970) 
// and convert it into a timedate structure:
//
// typedef struct          
// {
//    ushort  second;
//    ushort  minute;
//    ushort  hour;
//    ushort  day;
//    ushort  month;
//    ushort  year;
// } timedate;
//
// Parameters:
//  * td     timedate struct to return the time/date.
//  x        the number of seconds elapsed since Jan 1, 1970.
//
// Returns:  
//  * td     returns the time/date in a timedate struct.
//
static int dm[] = { 0,0,31,59,90,120,151,181,212,243,273,304,334,365 };

void SecondsToDate(timedate *td, ulong x)
{
   short tmp,i,j;
   ulong y;
   
   // check to make sure date is not over 2070 (sanity check)
   if (x > 0xBBF81E00L)
      x = 0;
   
   y = x/60;  td->second = (ushort)(x-60*y);
   x = y/60;  td->minute = (ushort)(y-60*x);
   y = x/24;  td->hour   = (ushort)(x-24*y);
   x = 4*(y+731);  td->year = (ushort)(x/1461);
   i = (int)((x-1461*(ulong)(td->year))/4);  td->month = 13;
   
   do
   {
      td->month -= 1;
      tmp = (td->month > 2) && ((td->year & 3)==0) ? 1 : 0;
      j = dm[td->month]+tmp;
   
   } while (i < j);
   
   td->day = i-j+1;
   
   // slight adjustment to algorithm 
   if (td->day == 0) 
      td->day = 1;
   
   td->year = (td->year < 32)  ? td->year + 68 + 1900: td->year - 32 + 2000;
}

//----------------------------------------------------------------------
// Takes a timedate struct and converts it into 
// the number of seconds since Jan. 1, 1970.
//
// typedef struct          
// {
//    ushort  second;
//    ushort  minute;
//    ushort  hour;
//    ushort  day;
//    ushort  month;
//    ushort  year;
// } timedate;
//
// Parameters:
//  * td     timedate struct containing time/date to convert to seconds.
//
// Returns:  
//  ulong    the time/date as the number of seconds elapsed since Jan. 1, 1970.
//
ulong DateToSeconds(timedate *td)
{
   ulong Sv,Bv,Xv;

   // convert the date/time values.
   if (td->year >= 2000) 
      Sv = td->year + 32 - 2000;
   else 
      Sv = td->year - 68 - 1900;

   if ((td->month > 2) && ( (Sv & 3) == 0))
     Bv = 1;
   else
     Bv = 0;

   Xv = 365 * (Sv-2) + (Sv-1)/4 + dm[td->month] + td->day + Bv - 1;

   Xv = 86400 * Xv + (ulong)(td->second) + 60*((ulong)(td->minute) + 60*(ulong)(td->hour));

   return Xv;
}

//----------------------------------------------------------------------
// Converts a binary uchar buffer into a ulong return integer
// number. 'length' indicates the length of the uchar buffer.
//
// Parameters:
//  buffer    a pointer to the input buffer to convert.
//  length    the length of the input buffer.
//
// Returns:  
//  ulong     the ulong integer result.
//
ulong uchar_to_bin(uchar  *buffer, int length)
{
   int i;
   ulong l = 0;

   for (i = (length-1); i >= 0; i--)
        l = (l << 8) | (int)(buffer[i]);

   return l;
}

