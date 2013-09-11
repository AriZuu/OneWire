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
//  DS550LNK.C - Link Layer functions required by general 1-Wire drive
//           implementation for DS80C550 microcontroller.
//
//  Version: 2.00
//
//  History: 1.00 -> 1.01  Added function msDelay.
//           1.02 -> 1.03  Added function msGettick.
//           1.03 -> 2.00  Changed 'MLan' to 'ow'. Added support for
//                         multiple ports.
//
//           2.10 -> 3.00  Added owReadBitPower and owWriteBytePower

// -- Keil c51 control directives
// Using inline assembly means we have to dump the output of the entire file to
// an intermediate assembly assembly source file, then assemble that file so
// we can have our object for linking.
#pragma SRC(ds550lnk.a51)
// -- End directives

#include "ownet.h"
#include "ser550.h"

// exportable link-level functions
SMALLINT owTouchReset(int);
SMALLINT owTouchBit(int,SMALLINT);
SMALLINT owTouchByte(int,SMALLINT);
SMALLINT owWriteByte(int,SMALLINT);
SMALLINT owReadByte(int);
SMALLINT owSpeed(int,SMALLINT);
SMALLINT owLevel(int,SMALLINT);
SMALLINT owProgramPulse(int);
void msDelay(int);
void usDelay(unsigned int);
long msGettick(void);
SMALLINT owWriteBytePower(int,SMALLINT);
SMALLINT owReadBitPower(int,SMALLINT);
SMALLINT hasPowerDelivery(int);
SMALLINT hasOverDrive(int);
SMALLINT hasProgramPulse(int);

static SMALLINT USpeed = 0; // current 1-Wire Net communication speed
static SMALLINT ULevel = 0; // current 1-Wire Net level

//sbit P3_0 = P3^0;

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
   uchar result;
   portnum = 0;

   // Code from appnote 126.
   OW_PORT = 0; // drive bus low.
   usDelay(500); // 500-(3% error) ~= 480 us
   OW_PORT = 1; // bus high.

   // corrected 6/18/2002.  Was set for 120 us, should be 70-75 us.
   usDelay(73); // 73-(3% error) ~= 70 us
   result = !OW_PORT; // get presence detect pulse.
   usDelay(423); // 586-(3% error) ~= 410 us

   return result;
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
SMALLINT owTouchBit(int portnum, SMALLINT sendbit)
{
   unsigned char result;
   portnum = 0;

   // start timeslot.
   OW_PORT = 0;
   usDelay(2);

   // send bit out.
   OW_PORT = sendbit;
   usDelay(5);

   // sample result @ 15 us.
   result = OW_PORT;

   usDelay(62); // 62-(3% error) ~= 60 us

   // timeslot done.
   OW_PORT = 1;
   usDelay(5);

   return result;
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
// Returns:  8 bits read from sendbyte
//
SMALLINT owTouchByte(int portnum, SMALLINT sendbyte)
{
   uchar i;
   uchar result = 0;
   portnum = 0;

   for (i = 0; i < 8; i++)
   {
       result |= (owTouchBit(portnum,sendbyte & 1) << i);
       sendbyte >>= 1;
   }

   return result;
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
SMALLINT owWriteByte(int portnum, SMALLINT sendbyte)
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
SMALLINT owSpeed(int portnum, SMALLINT new_speed)
{
   portnum = 0;

   new_speed = USpeed;
   // not supported in micro code

   return USpeed;
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
// Note: Strong and Program not supported on 550 target.
SMALLINT owLevel(int portnum, SMALLINT new_level)
{
   portnum = 0;
   switch(new_level)
   {
      case MODE_BREAK:
         OW_PORT = 0; //send a break on one-wire port
         break;
      default:
      case MODE_NORMAL:
         OW_PORT = 1;
         break;
   }
   ULevel = new_level;
   return new_level;
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
   portnum = 0;
   // Not supported
   return 0;
}

//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
//
void msDelay(int delay)
{
   while(delay--)
   {
      usDelay(1030); // -3% error puts us right at 1ms.
	}
}

//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' microseconds
//
// Adapted from Kevin Vigor's TINI 1Wire code.
//
// Works only @33Mhz.  We need 8.25 machine cycles per iteration to get the microsecond
// delay.  Can't have .25 of a cycle, so this function uses 8 cycles per iteration. This
// produces an error of 3% ( .03 = (8.25-8)/8 ).  Most of the use of this function by
// 1-Wire routines calls for delays under 50 microseconds, in which case it is under by
// less than 1.5 microseconds.  It is assumed the overhead of calling the function will
// add enough to account for the difference.  For values much greater than 50 (only in
// reset function), a "fudge factor" is added to account for any error.
//
//
void usDelay(unsigned int delay)
{
#pragma asm
   ; delay is in r6/r7
   ; double-check in output assembly
   mov  a, r7
   orl  a, r6			; quick out for zero case.
   jz   _usDelayDone

   inc  r6
   cjne r7, #0, _usDelayLoop
   dec  r6

   _usDelayLoop:
   nop
   nop
   nop
   nop
   nop
   djnz r7, _usDelayLoop
   djnz r6, _usDelayLoop
   _usDelayDone:

#pragma endasm
   delay = 0;
}

//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
long msGettick(void)
{
   // not fully supported yet
   // returns timer 1, which is in 8-bit auto-reload (presumably) for the
   // serial port.  may not be good enough.
   return TL1;
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
   portnum = 0;
   sendbyte = 0;
	// not supported on the by the DS520
	return FALSE;
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
   portnum = 0;
   applyPowerResponse = 0;
	// not supported on the by the DS520
	return FALSE;
}

//--------------------------------------------------------------------------
// This procedure indicates wether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  if adapter is capable of delivering power.
//
SMALLINT owHasPowerDelivery(int portnum)
{
   portnum = 0;
   // add adapter specific code here
   return FALSE;
}

//--------------------------------------------------------------------------
// This procedure indicates wether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  if adapter is capable of over drive.
//
SMALLINT owHasOverDrive(int portnum)
{
   portnum = 0;
   // add adapter specific code here
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
   portnum = 0;
   // add adapter specific code here
   return FALSE;
}
