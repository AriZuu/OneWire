//---------------------------------------------------------------------------
// Copyright (C) 2003 Dallas Semiconductor Corporation, All Rights Reserved.
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
//  Win32P.C - Link Layer functions general 1-Wire driver
//           implimentation for DS1410E/DS1410D parallel adapter.
//
//  Version: 3.00
//
//  History: 2.00 -> 3.00 Added call to setup() before each low level call
//                        to get base port address correct if multiple
//                        ports open
 
#include "ownet.h"
#include <windows.h>
#include "sacwd32.h"

// exportable link-level functions
SMALLINT owTouchReset(int);
SMALLINT owTouchBit(int,int);
SMALLINT owTouchByte(int,int);
SMALLINT owWriteByte(int,int sendbyte);
SMALLINT owReadByte(int);
SMALLINT owSpeed(int,int);
SMALLINT owLevel(int,int);
SMALLINT owProgramPulse(int);
void msDelay(int);
long msGettick(void);
SMALLINT owWriteBytePower(int,SMALLINT);
SMALLINT owHasPowerDelivery(int);
SMALLINT owHasProgramPulse(int);
SMALLINT owHasOverDrive(int);


// from Sacwd32
extern uchar DOWReset(void);  
extern uchar ToggleOverdrive(void); 

// globals
SMALLINT Level[MAX_PORTNUM]; // current 1-Wire Net level
SMALLINT Speed[MAX_PORTNUM]; // current 1-Wire Net communication speed
sa_struct SauthGB[MAX_PORTNUM];
extern int  PortNum[MAX_PORTNUM];

// global for DS1994/DS2404/DS1427.  If TRUE, puts a delay in owTouchReset to compensate for alarming clocks.
SMALLINT FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = FALSE; // default owTouchReset to FALSE


//--------------------------------------------------------------------------
// Reset all of the devices on the 1-Wire Net and return the result.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
//
// Returns: TRUE(1):  presense pulse(s) detected, device(s) reset
//          FALSE(0): no presense pulses detected
//
SMALLINT owTouchReset(int portnum)
{
   int rt;

   // make sure normal level
   owLevel(portnum,MODE_NORMAL);

   // do the reset
   setup((uchar)PortNum[portnum],&SauthGB[PortNum[portnum]]);
   rt = DOWReset();

   // wait at least 8ms (for DS1994/DS2404)
   if (FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE)
   {
      Sleep(8);
      return TRUE;  // return TRUE as DOWReset will be FALSE
   }

   return rt;
}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and return the
// result 1 bit read from the 1-Wire Net.  The parameter 'sendbit'
// least significant bit is used and the least significant bit
// of the result is the return bit.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'sendbit'    - the least significant bit is the bit to send
//
// Returns: 0:   0 bit read from sendbit
//          1:   1 bit read from sendbit
//
SMALLINT owTouchBit(int portnum, int sendbit)
{
   // make sure normal level
   owLevel(portnum,MODE_NORMAL);

   return databit((uchar)sendbit,&SauthGB[PortNum[portnum]]);
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and return the
// result 8 bits read from the 1-Wire Net.  The parameter 'sendbyte'
// least significant 8 bits are used and the least significant 8 bits
// of the result is the return byte.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'sendbyte'   - 8 bits to send (least significant byte)
//
// Returns:  8 bytes read from sendbyte
//
SMALLINT owTouchByte(int portnum, int sendbyte)
{
   // make sure normal level
   owLevel(portnum,MODE_NORMAL);

   return databyte((uchar)sendbyte,&SauthGB[PortNum[portnum]]);
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).  
// The parameter 'sendbyte' least significant 8 bits are used.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'sendbyte'   - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same 
//
SMALLINT owWriteByte(int portnum, int sendbyte)
{
   return (owTouchByte(portnum,sendbyte) == sendbyte) ? TRUE : FALSE;
}

//--------------------------------------------------------------------------
// Send 8 bits of read communication to the 1-Wire Net and and return the
// result 8 bits read from the 1-Wire Net.   
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
//
// Returns:  8 bytes read from 1-Wire Net
//
SMALLINT owReadByte(int portnum)
{
   return owTouchByte(portnum,0xFF);
}

//--------------------------------------------------------------------------
// Set the 1-Wire Net communucation speed.  
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'new_speed'  - new speed defined as
//                MODE_NORMAL     0x00
//                MODE_OVERDRIVE  0x01
//
// Returns:  current 1-Wire Net speed 
//
SMALLINT owSpeed(int portnum, int new_speed)
{
   // check if already correct level
   if (new_speed == Speed[portnum]) 
      return Speed[portnum];

   // normal
   if (new_speed == MODE_NORMAL)
   {
      if (Speed[portnum] != MODE_NORMAL)
      {
         setup((uchar)PortNum[portnum],&SauthGB[PortNum[portnum]]);
         // OverdriveOff
         if(ToggleOverdrive())
            ToggleOverdrive();
      }

      Speed[portnum] = MODE_NORMAL;
   }
   // overdrive
   else if (new_speed == MODE_OVERDRIVE)
   {
      setup((uchar)PortNum[portnum],&SauthGB[PortNum[portnum]]);
      if (!ToggleOverdrive())
        if (!ToggleOverdrive()) // DS1410D 
           return MODE_NORMAL;   
      
      Speed[portnum] = MODE_OVERDRIVE;
   }

   return Speed[portnum];
}

//--------------------------------------------------------------------------
// Set the 1-Wire Net line level.  The values for NewLevel are
// as follows:
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'new_level'  - new level defined as
//                MODE_NORMAL     0x00
//                MODE_STRONG5    0x02
//                MODE_PROGRAM    0x04
//                MODE_BREAK      0x08 
//
// Returns:  current 1-Wire Net level  
//
SMALLINT owLevel(int portnum, int new_level)
{
   // check if already correct level
   if (new_level == Level[portnum])
      return Level[portnum];

   // check if just putting back to normal
   if (new_level == MODE_NORMAL)
   {
      // only turn off strong5 if not using overdrive
      if (Speed[portnum] != MODE_OVERDRIVE)
      {
         // Overdrive Off
         setup((uchar)PortNum[portnum],&SauthGB[PortNum[portnum]]);
         if(ToggleOverdrive())
            ToggleOverdrive();
      }

      Level[portnum] = MODE_NORMAL;
   }
   else if (new_level == MODE_STRONG5)
   {
      // only turn on strong5 if not using overdrive
      if (Speed[portnum] != MODE_OVERDRIVE)
      {
         setup((uchar)PortNum[portnum],&SauthGB[PortNum[portnum]]);
         if (!ToggleOverdrive())
           if (!ToggleOverdrive()) // DS1410D 
              return MODE_NORMAL;   
      }

      Level[portnum] = MODE_STRONG5;
   }

   return Level[portnum];
}

//--------------------------------------------------------------------------
// This procedure creates a fixed 480 microseconds 12 volt pulse 
// on the 1-Wire Net for programming EPROM iButtons.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
//
// Returns:  TRUE  successful
//           FALSE program voltage not available  
//
SMALLINT owProgramPulse(int portnum)
{
   // not supported with this adapter
   return 0;
}

//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
// 
void msDelay(int len)
{
   Sleep(len);
}

//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
long msGettick(void)
{
   return GetTickCount();
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).  
// The parameter 'sendbyte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same 
//
SMALLINT owWriteBytePower(int portnum, SMALLINT sendbyte)
{
   if (owTouchByte(portnum,sendbyte) != sendbyte)
      return FALSE;

   if (owLevel(portnum,MODE_STRONG5) != MODE_STRONG5)
      return FALSE;

   return TRUE;
}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and verify that the
// response matches the 'applyPowerResponse' bit and apply power delivery
// to the 1-Wire net.  Note that some implementations may apply the power
// first and then turn it off if the response is incorrect.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'applyPowerResponse' - 1 bit response to check, if correct then start
//                        power delivery 
//
// Returns:  TRUE: bit written and response correct, strong pullup now on
//           FALSE: response incorrect
//
SMALLINT owReadBitPower(int portnum, SMALLINT applyPowerResponse)
{
   SMALLINT rt=FALSE;

   if (owTouchBit(portnum, applyPowerResponse) == applyPowerResponse)
   {
      owLevel(portnum,MODE_STRONG5);
      rt = TRUE;
   }
   return rt;
}

//--------------------------------------------------------------------------
// Read 8 bits of communication to the 1-Wire Net and then turn on 
// power delivery.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  byte read
//           FALSE: power delivery failed 
//
SMALLINT owReadBytePower(int portnum)
{
   SMALLINT getbyte;
   
   getbyte = owTouchByte(portnum,0xFF);

   if (owLevel(portnum,MODE_STRONG5) != MODE_STRONG5)
      return FALSE;

   return getbyte;
}

//--------------------------------------------------------------------------
// This procedure indicates wether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  because all userial adapters have over drive. 
//
SMALLINT owHasPowerDelivery(int portnum)
{
   return TRUE;
}

//--------------------------------------------------------------------------
// This procedure indicates wether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  because all userial adapters have over drive. 
//
SMALLINT owHasOverDrive(int portnum)
{
   return TRUE;
}
//--------------------------------------------------------------------------
// This procedure creates a fixed 480 microseconds 12 volt pulse 
// on the 1-Wire Net for programming EPROM iButtons.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  program volatage available
//           FALSE program voltage not available  
SMALLINT owHasProgramPulse(int portnum)
{
   return FALSE;
}

